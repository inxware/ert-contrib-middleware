#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Provides AptWorker which processes transactions."""
# Copyright (C) 2008-2009 Sebastian Heinlein <devel@glatzor.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

__author__  = "Sebastian Heinlein <devel@glatzor.de>"

__all__ = ("AptWorker", "DummyWorker")

import logging
import os
import sys
import time
import traceback

import apt
import apt.cache
import apt.debfile
import apt_pkg
import aptsources
import aptsources.distro
from aptsources.sourceslist import SourcesList
from gettext import gettext as _
import gobject
import pkg_resources
import re
from softwareproperties.AptAuth import AptAuth
import subprocess

from enums import *
from errors import *
import lock
from progress import DaemonOpenProgress, \
                     DaemonInstallProgress, \
                     DaemonAcquireProgress, \
                     DaemonDpkgInstallProgress, \
                     DaemonDpkgRecoverProgress

log = logging.getLogger("AptDaemon.Worker")


class AptWorker(gobject.GObject):

    """Worker which processes transactions from the queue."""

    __gsignals__ = {"transaction-done":(gobject.SIGNAL_RUN_FIRST,
                                        gobject.TYPE_NONE,
                                        (gobject.TYPE_PYOBJECT,))}

    def __init__(self):
        """Initialize a new AptWorker instance."""
        gobject.GObject.__init__(self)
        self.trans = None
        self.last_action_timestamp = time.time()
        self._cache = None
        self._status_orig = apt_pkg.config.find_file("Dir::State::status")
        self.plugins = {}
        self._load_plugins()

    def _load_plugins(self):
        """Load the plugins from setuptools' entry points."""
        plugin_dirs = [os.path.join(os.path.dirname(__file__), "plugins")]
        env = pkg_resources.Environment(plugin_dirs)
        dists, errors = pkg_resources.working_set.find_plugins(env)
        for dist in dists:
            pkg_resources.working_set.add(dist)
        for name in ["modify_cache_after", "modify_cache_before"]:
            for ept in pkg_resources.iter_entry_points("aptdaemon.plugins",
                                                       name):
                try:
                    self.plugins.setdefault(name, []).append(ept.load())
                except:
                    log.critical("Failed to load %s plugin: "
                                 "%s" % (name, ept.dist))
                else:
                    log.debug("Loaded %s plugin: %s", name, ept.dist)

    def _call_plugins(self, name, resolver=None):
        """Call all plugins of a given type."""
        if not resolver:
            # If the resolver of the original task isn't available we create
            # a new one and protect the already marked changes
            resolver = apt.cache.ProblemResolver(self._cache)
            for pkg in self._cache.get_changes():
                resolver.clear(pkg)
                resolver.protect(pkg)
                if pkg.marked_delete:
                    resolver.remove(pkg)
        if not name in self.plugins:
            log.debug("There isn't any registered %s plugin" % name)
            return False
        for plugin in self.plugins[name]:
            log.debug("Calling %s plugin: %s", name, plugin)
            try:
                plugin(resolver, self._cache)
            except Exception, error:
                log.critical("Failed to call %s plugin:\n%s" % (plugin, error))
        return True

    def run(self, transaction):
        """Process the given transaction in the background.

        Keyword argument:
        transaction -- core.Transcation instance to run
        """
        log.info("Processing transaction %s", transaction.tid)
        if self.trans:
            raise Exception("There is already a running transaction")
        self.trans = transaction
        gobject.idle_add(self._process_transaction)

    def _emit_transaction_done(self, trans):
        """Emit the transaction-done signal.

        Keyword argument:
        trans -- the finished transaction
        """
        log.debug("Emitting transaction-done: %s", trans.tid)
        self.emit("transaction-done", trans)

    def _process_transaction(self):
        """Run the worker"""
        self.last_action_timestamp = time.time()
        self.trans.status = STATUS_RUNNING
        self.trans.progress = 0
        try:
            self._lock_cache()
            # Prepare the package cache
            if self.trans.role == ROLE_FIX_INCOMPLETE_INSTALL or \
               not self.is_dpkg_journal_clean():
                self.fix_incomplete_install()
            # Process transaction which don't require a cache
            if self.trans.role == ROLE_ADD_VENDOR_KEY_FILE:
                self.add_vendor_key_from_file(**self.trans.kwargs)
            elif self.trans.role == ROLE_ADD_VENDOR_KEY_FROM_KEYSERVER:
                self.add_vendor_key_from_keyserver(**self.trans.kwargs)
            elif self.trans.role == ROLE_REMOVE_VENDOR_KEY:
                self.remove_vendor_key(**self.trans.kwargs)
            elif self.trans.role == ROLE_ADD_REPOSITORY:
                self.add_repository(**self.trans.kwargs)
            elif self.trans.role == ROLE_ENABLE_DISTRO_COMP:
                self.enable_distro_comp(**self.trans.kwargs)
            else:
                self._open_cache()
            # Process transaction which can handle a broken dep cache
            if self.trans.role == ROLE_FIX_BROKEN_DEPENDS:
                self.fix_broken_depends()
            elif self.trans.role == ROLE_UPDATE_CACHE:
                self.update_cache(**self.trans.kwargs)
            # Process the transactions which require a consistent cache
            elif self._cache and self._cache.broken_count:
                broken = [pkg.name for pkg in self._cache if pkg.is_now_broken]
                raise TransactionFailed(ERROR_CACHE_BROKEN, " ".join(broken))
            elif self.trans.role == ROLE_INSTALL_PACKAGES:
                self.install_packages(self.trans.packages[0])
            elif self.trans.role == ROLE_INSTALL_FILE:
                self.install_file(**self.trans.kwargs)
            elif self.trans.role == ROLE_REMOVE_PACKAGES:
                self.remove_packages(self.trans.packages[2])
            elif self.trans.role == ROLE_UPGRADE_SYSTEM:
                self.upgrade_system(**self.trans.kwargs)
            elif self.trans.role == ROLE_UPGRADE_PACKAGES:
                self.upgrade_packages(self.trans.packages[4])
            elif self.trans.role == ROLE_COMMIT_PACKAGES:
                self.commit_packages(*self.trans.packages)
        except TransactionCancelled:
            self.trans.exit = EXIT_CANCELLED
        except TransactionFailed, excep:
            self.trans.error = excep
            self.trans.exit = EXIT_FAILED
        except (KeyboardInterrupt, SystemExit):
            self.trans.exit = EXIT_CANCELLED
        except Exception, excep:
            self.trans.error = TransactionFailed(ERROR_UNKNOWN,
                                                 traceback.format_exc())
            self.trans.exit = EXIT_FAILED
        else:
            self.trans.exit = EXIT_SUCCESS
        finally:
            self.trans.progress = 100
            self.last_action_timestamp = time.time()
            tid = self.trans.tid[:]
            trans = self.trans
            self.trans = None
            self._emit_transaction_done(trans)
            lock.release()
            log.info("Finished transaction %s", tid)
        return False

    def commit_packages(self, install, reinstall, remove, purge, upgrade):
        """Perform a complex package operation.

        Keyword arguments:
        install - list of package names to install
        reinstall - list of package names to reinstall
        remove - list of package names to remove
        purge - list of package names to purge including configuration files
        upgrade - list of package names to upgrade
        """
        log.info("Committing packages: %s, %s, %s, %s, %s",
                 install, reinstall, remove, purge, upgrade)
        #FIXME python-apt 0.8 introduced a with statement
        ac = self._cache.actiongroup()
        resolver = apt.cache.ProblemResolver(self._cache)
        self._mark_packages_for_installation(install, resolver)
        self._mark_packages_for_installation(reinstall, resolver,
                                             reinstall=True)
        self._mark_packages_for_removal(remove, resolver)
        self._mark_packages_for_removal(purge, resolver, purge=True)
        self._mark_packages_for_upgrade(upgrade, resolver)
        self._resolve_depends(resolver)
        ac.release()
        self._commit_changes()

    def _resolve_depends(self, resolver):
        """Resolve the dependencies using the given ProblemResolver."""
        self._call_plugins("modify_cache_before", resolver)
        resolver.install_protect()
        try:
            resolver.resolve()
        except SystemError:
            broken = [pkg.name for pkg in self._cache if pkg.is_inst_broken]
            raise TransactionFailed(ERROR_DEP_RESOLUTION_FAILED,
                                    " ".join(broken))
        if self._call_plugins("modify_cache_after", resolver):
            try:
                resolver.resolve()
            except SystemError:
                broken = [pkg.name for pkg in self._cache if pkg.is_inst_broken]
                raise TransactionFailed(ERROR_DEP_RESOLUTION_FAILED,
                                        " ".join(broken))
 
    def install_packages(self, package_names):
        """Install packages.

        Keyword argument:
        package_names -- list of package name which should be installed
        """
        log.debug("Installing packages: %s", package_names)
        self.trans.status = STATUS_RESOLVING_DEP
        ac = self._cache.actiongroup()
        resolver = apt.cache.ProblemResolver(self._cache)
        self._mark_packages_for_installation(package_names, resolver)
        self._resolve_depends(resolver)
        ac.release()
        self._commit_changes()

    def _check_unauthenticated(self):
        """Check if any of the cache changes get installed from an
        unauthenticated repository"""
        if self.trans.allow_unauthenticated:
            return
        unauthenticated = []
        for pkg in self._cache:
            if (pkg.marked_install or
                pkg.marked_downgrade or
                pkg.marked_upgrade or
                pkg.marked_reinstall):
                trusted = False
                for origin in pkg.candidate.origins:
                    trusted |= origin.trusted
                if not trusted:
                    unauthenticated.append(pkg.name)
        if unauthenticated:
            raise TransactionFailed(ERROR_PACKAGE_UNAUTHENTICATED,
                                    " ".join(sorted(unauthenticated)))

    def _mark_packages_for_installation(self, package_names, resolver,
                                        reinstall=False):
        """Mark packages for installation."""
        for pkg_name in package_names:
            try:
                pkg = self._cache[pkg_name]
            except KeyError:
                raise TransactionFailed(ERROR_NO_PACKAGE,
                                        "Package %s isn't available" % pkg_name)
            if reinstall:
                if not pkg.is_installed:
                    raise TransactionFailed(ERROR_PACKAGE_NOT_INSTALLED,
                                            "Package %s isn't installed" % \
                                            pkg.name)
            else:
                #FIXME: Turn this into a non-critical message
                if pkg.is_installed:
                    raise TransactionFailed(ERROR_PACKAGE_ALREADY_INSTALLED,
                                            "Package %s is already installed" %\
                                            pkg_name)
            pkg.mark_install(False, True, True)
            resolver.clear(pkg)
            resolver.protect(pkg)

    def enable_distro_comp(self, component):
        """Enable given component in the sources list.

        Keyword arguments:
        component -- a component, e.g. main or universe
        """
        old_umask = os.umask(0022)
        try:
            sourceslist = SourcesList()
            distro = aptsources.distro.get_distro()
            distro.get_sources(sourceslist)
            distro.enable_component(component)
            sourceslist.save()
        finally:
            os.umask(old_umask)

    def add_repository(self, rtype, uri, dist, comps, comment, sourcesfile):
        """Add given repository to the sources list.

        Keyword arguments:
        rtype -- the type of the entry (deb, deb-src)
        uri -- the main repository uri (e.g. http://archive.ubuntu.com/ubuntu)
        dist -- the distribution to use (e.g. karmic, "/")
        comps -- a (possible empty) list of components (main, restricted)
        comment -- an (optional) comment
        sourcesfile -- an (optinal) filename in sources.list.d 
        """
        if sourcesfile:
            if not sourcesfile.endswith(".list"):
                sourcesfile += ".list"
            d = apt_pkg.config.find_dir("Dir::Etc::sourceparts")
            sourcesfile = os.path.join(d, os.path.basename(sourcesfile))
        else:
            sourcesfile = None
        # if there is a password in the uri, protect the file from
        # non-admin users
        password_in_uri = re.match("(http|https|ftp)://\S+?:\S+?@\S+", uri)
        if password_in_uri:
            old_umask = os.umask(0027)
        else:
            old_umask = os.umask(0022)
        try:
            sources = SourcesList()
            entry = sources.add(rtype, uri, dist, comps, comment,
                                file=sourcesfile)
            if entry.invalid:
                #FIXME: Introduce new error codes
                raise RepositoryInvalidError()
        except:
            logging.exception("adding repository")
            raise
        else:
            sources.save()
            # set to sourcesfile root.admin only if there is a password
            if password_in_uri and sourcesfile:
                import grp
                try:
                    os.chown(sourcesfile, 0, grp.getgrnam("admin")[2])
                except Exception, e:
                    logging.warn("os.chmod() failed '%s'" % e)
        finally:
            os.umask(old_umask)

    def add_vendor_key_from_keyserver(self, keyid, keyserver):
        """Add the signing key from the given (keyid, keyserver) to the
        trusted vendors.

        Keyword argument:
        keyid - the keyid of the key (e.g. 0x0EB12F05)
        keyserver - the keyserver (e.g. keyserver.ubuntu.com)
        """
        log.info("Adding vendor key from keyserver: %s %s", keyid, keyserver)
        self.trans.status = STATUS_DOWNLOADING
        self.trans.progress = 101
        last_pulse = time.time()
        #FIXME: Use gobject.spawn_async and deferreds in the worker
        #       Alternatively we could use python-pyme directly for a better
        #       error handling. Or the --status-fd of gpg
        proc = subprocess.Popen(["/usr/bin/apt-key", "adv",
                                 "--keyserver", keyserver,
                                 "--recv", keyid], stderr=subprocess.STDOUT,
                                 stdout=subprocess.PIPE, close_fds=True)
        while proc.poll() is None:
            while gobject.main_context_default().pending():
                gobject.main_context_default().iteration()
            time.sleep(0.05)
            if time.time() - last_pulse > 0.3:
                self.trans.progress = 101
                last_pulse = time.time()
        if proc.returncode != 0:
            stdout = unicode(proc.stdout.read(), 
                             # that can return "None", in this case, just
                             # assume something
                             sys.stdin.encoding or "UTF-8",
                             errors="replace")
            #TRANSLATORS: The first %s is the key id and the second the server
            raise TransactionFailed(ERROR_KEY_NOT_INSTALLED,
                                    _("Failed to download and install the key "
                                      "%s from %s:\n%s") % (keyid, keyserver,
                                                            stdout))

    def add_vendor_key_from_file(self, path):
        """Add the signing key from the given file to the trusted vendors.

        Keyword argument:
        path -- absolute path to the key file
        """
        log.info("Adding vendor key from file: %s", path)
        self.trans.progress = 101
        self.trans.status = STATUS_COMMITTING
        try:
            #FIXME: use gobject.spawn_async or reactor.spawn
            #FIXME: use --dry-run before?
            auth = AptAuth()
            auth.add(os.path.expanduser(path))
        except Exception, error:
            raise TransactionFailed(ERROR_KEY_NOT_INSTALLED,
                                    "Key file %s couldn't be installed: %s" % \
                                    (path, error))

    def remove_vendor_key(self, fingerprint):
        """Remove repository key.

        Keyword argument:
        fingerprint -- fingerprint of the key to remove
        """
        log.info("Removing vendor key: %s", fingerprint)
        self.trans.progress = 101
        self.trans.status = STATUS_COMMITTING
        try:
            #FIXME: use gobject.spawn_async or reactor.spawn
            #FIXME: use --dry-run before?
            auth = AptAuth()
            auth.rm(fingerprint)
        except Exception, error:
            raise TransactionFailed(ERROR_KEY_NOT_REMOVED,
                                    "Key with fingerprint %s couldn't be "
                                    "removed: %s" % (fingerprint, error))

    def install_file(self, path):
        """Install local package file.

        Keyword argument:
        path -- absolute path to the package file
        """
        log.info("Installing local package file: %s", path)
        # Check if the dpkg can be installed at all
        self.trans.status = STATUS_RESOLVING_DEP
        deb = apt.debfile.DebPackage(path.encode("UTF-8"), self._cache)
        if not deb.check():
            raise TransactionFailed(ERROR_DEP_RESOLUTION_FAILED,
                                    deb._failure_string)
        # Check for required changes and apply them before
        (install, remove, unauth) = deb.required_changes
        self._call_plugins("modify_cache_after")
        if len(install) > 0 or len(remove) > 0:
            dpkg_range = (64, 99)
            self._commit_changes(fetch_range=(5, 33),
                                 install_range=(34, 63))
        # Install the dpkg file
        if deb.install(DaemonDpkgInstallProgress(self.trans,
                                                 begin=64, end=95)):
            raise TransactionFailed(ERROR_UNKNOWN, deb._failure_string)

    def remove_packages(self, package_names):
        """Remove packages.

        Keyword argument:
        package_names -- list of package name which should be installed
        """
        log.info("Removing packages: '%s'", package_names)
        self.trans.status = STATUS_RESOLVING_DEP
        ac = self._cache.actiongroup()
        resolver = apt.cache.ProblemResolver(self._cache)
        self._mark_packages_for_removal(package_names, resolver)
        self._resolve_depends(resolver)
        ac.release()
        self._commit_changes(fetch_range=(10, 10), install_range=(10, 90))
        #FIXME: should we use a persistant cache? make a check?
        #self._open_cache(prange=(90,99))
        #for p in pkgs:
        #    if self._cache.has_key(p) and self._cache[p].is_installed:
        #        self.ErrorCode(ERROR_UNKNOWN, "%s is still installed" % p)
        #        self.Finished(EXIT_FAILED)
        #        return

    def _mark_packages_for_removal(self, package_names, resolver, purge=False):
        """Mark packages for installation."""
        for pkg_name in package_names:
            try:
                pkg = self._cache[pkg_name]
            except KeyError:
                raise TransactionFailed(ERROR_NO_PACKAGE,
                                        "Package %s isn't available" % pkg_name)
            if not pkg.is_installed and not pkg.installed_files:
                raise TransactionFailed(ERROR_PACKAGE_NOT_INSTALLED,
                                        "Package %s isn't installed" % pkg_name)
            if pkg.essential == True:
                raise TransactionFailed(ERROR_NOT_REMOVE_ESSENTIAL_PACKAGE,
                                        "Package %s cannot be removed." % \
                                        pkg_name)
            pkg.mark_delete(False, purge)
            resolver.clear(pkg)
            resolver.protect(pkg)
            resolver.remove(pkg)

    def _check_obsoleted_dependencies(self):
        """Mark obsoleted dependencies of to be removed packages for removal."""
        if not self.trans.remove_obsoleted_depends:
            return
        installed_deps = set()
        ac = self._cache.actiongroup()
        for pkg in self._cache:
            if pkg.marked_delete:
                installed_deps = self._installed_dependencies(pkg.name,
                                                              installed_deps)
        for dep_name in installed_deps:
            if dep_name in self._cache:
                pkg = self._cache[dep_name]
                if pkg.is_installed and pkg.is_auto_removable:
                    pkg.mark_delete(False)
        ac.release()

    def _installed_dependencies(self, pkg_name, all_deps=None):
        """Recursively return all installed dependencies of a given package."""
        #FIXME: Should be part of python-apt, since it makes use of non-public
        #       API. Perhaps by adding a recursive argument to
        #       apt.package.Version.get_dependencies()
        if not all_deps:
            all_deps = set()
        if not pkg_name in self._cache:
            return all_deps
        cur = self._cache[pkg_name]._pkg.current_ver
        if not cur:
            return all_deps
        for t in ("PreDepends", "Depends", "Recommends"):
            try:
                for dep in cur.depends_list[t]:
                    dep_name = dep[0].target_pkg.name
                    if not dep_name in all_deps:
                        all_deps.add(dep_name)
                        all_deps |= self._installed_dependencies(dep_name,
                                                                 all_deps)
            except KeyError:
                pass
        return all_deps

    def upgrade_packages(self, package_names):
        """Upgrade packages.

        Keyword argument:
        package_names -- list of package name which should be upgraded
        """
        log.info("Upgrading packages: %s", package_names)
        self.trans.status = STATUS_RESOLVING_DEP
        ac = self._cache.actiongroup()
        resolver = apt.cache.ProblemResolver(self._cache)
        self._mark_packages_for_upgrade(package_names, resolver)
        self._resolve_depends(resolver)
        ac.release()
        self._commit_changes()

    def _mark_packages_for_upgrade(self, package_names, resolver):
        """Mark packages for upgrade."""
        for pkg_name in package_names:
            try:
                pkg = self._cache[pkg_name]
            except KeyError:
                raise TransactionFailed(ERROR_NO_PACKAGE,
                                        "Package %s isn't available" % pkg_name)
            if not pkg.is_installed:
                raise TransactionFailed(ERROR_PACKAGE_NOT_INSTALLED,
                                        "Package %s isn't installed" % pkg_name)
            pkg.mark_install(False, True, pkg.is_auto_installed)
            resolver.clear(pkg)
            resolver.protect(pkg)

    def update_cache(self, sources_list):
        """Update the cache."""
        log.info("Updating cache")
        progress = DaemonAcquireProgress(self.trans, begin=10, end=95)
        if sources_list:
            # for security reason (lp722228) we only allow files inside
            # sources.list.d as basedir
            basedir = apt_pkg.config.find_dir("Dir::Etc::sourceparts")
            system_sources = apt_pkg.config.find_file("Dir::Etc::sourcelist")
            if "/" in sources_list:
                sources_list = os.path.abspath(sources_list)
                if not (sources_list.startswith(basedir) or
                        sources_list == system_sources):
                    raise AptDaemonError("only alternative sources.list files inside '%s' are allowed (not '%s')" % (basedir, sources_list))
            else:
                sources_list = os.path.join(basedir, sources_list)
        # do it
        try:
            self._cache.update(progress, sources_list=sources_list)
        except apt.cache.FetchFailedException, error:
            raise TransactionFailed(ERROR_REPO_DOWNLOAD_FAILED,
                                    str(error.message))
        except apt.cache.FetchCancelledException:
            raise TransactionCancelled()

    def upgrade_system(self, safe_mode=True):
        """Upgrade the system.

        Keyword argument:
        safe_mode -- if additional software should be installed or removed to
                     satisfy the dependencies the an updates
        """
        log.info("Upgrade system with safe mode: %s" % safe_mode)
        # Check for available updates
        self.trans.status = STATUS_RESOLVING_DEP
        updates = filter(lambda p: p.is_upgradable,
                         self._cache)
        #FIXME: What to do if already uptotdate? Add error code?
        self._cache.upgrade(dist_upgrade=not safe_mode)
        self._call_plugins("modify_cache_after")
        # Check for blocked updates
        outstanding = []
        changes = self._cache.get_changes()
        for pkg in updates:
            if not pkg in changes or not pkg.marked_upgrade:
                outstanding.append(pkg)
        #FIXME: Add error state if system could not be fully updated
        self._commit_changes()

    def fix_incomplete_install(self):
        """Run dpkg --configure -a to recover from a failed installation."""
        log.info("Fixing incomplete installs")
        self.trans.status = STATUS_CLEANING_UP
        progress = DaemonDpkgRecoverProgress(self.trans)
        progress.start_update()
        progress.run()
        progress.finish_update()
        if progress._child_exit != 0:
            raise TransactionFailed(ERROR_PACKAGE_MANAGER_FAILED,
                                    progress.output)

    def fix_broken_depends(self):
        """Try to fix broken dependencies."""
        log.info("Fixing broken depends")
        self.trans.status = STATUS_RESOLVING_DEP
        try:
            self._cache._depcache.fix_broken()
        except SystemError:
            broken = [pkg.name for pkg in self._cache if pkg.is_inst_broken]
            raise TransactionFailed(ERROR_DEP_RESOLUTION_FAILED,
                                    " ".join(broken))
        self._commit_changes()

    def _open_cache(self, begin=0, end=5, quiet=False):
        """Open the APT cache.

        Keyword arguments:
        start -- the begin of the progress range
        end -- the end of the the progress range
        quiet -- if True do no report any progress
        """
        self.trans.status = STATUS_LOADING_CACHE
        apt_pkg.config.set("Dir::State::status", self._status_orig)
        apt_pkg.init_system()
        try:
            progress = DaemonOpenProgress(self.trans, begin=begin, end=end,
                                          quiet=quiet)
            if not isinstance(self._cache, apt.cache.Cache):
                self._cache = apt.cache.Cache(progress)
            else:
                self._cache.open(progress)
        except Exception, excep:
            raise TransactionFailed(ERROR_NO_CACHE, excep.message)

    def _lock_cache(self):
        """Lock the APT cache."""
        try:
            lock.acquire()
        except lock.LockFailedError, error:
            logging.error("Failed to lock the cache")
            self.trans.paused = True
            self.trans.status = STATUS_WAITING_LOCK
            if error.process:
                #TRANSLATORS: %s is the name of a package manager
                msg = "Waiting for %s to exit" % error.process
                self.trans.status_details = msg
            lock_watch = gobject.timeout_add_seconds(3, self._watch_lock)
            while self.trans.paused and not self.trans.cancelled:
                gobject.main_context_default().iteration()
            gobject.source_remove(lock_watch)
            if self.trans.cancelled:
                raise TransactionCancelled()

    def _watch_lock(self):
        """Unpause the transaction if the lock can be obtained."""
        try:
            lock.acquire()
        except lock.LockFailedError:
            return True
        self.trans.paused = False
        return False

    def is_dpkg_journal_clean(self):
        """Return False if there are traces of incomplete dpkg status
        updates."""
        status_updates = os.path.join(os.path.dirname(self._status_orig),
                                      "updates/")
        for dentry in os.listdir(status_updates):
            if dentry.isdigit():
                return False
        return True

    def _commit_changes(self, fetch_range=(5, 50), install_range=(50, 90)):
        """Commit previously marked changes to the cache.

        Keyword arguments:
        fetch_range -- tuple containing the start and end point of the
                       download progress
        install_range -- tuple containing the start and end point of the
                         install progress
        """
        changes = self._cache.get_changes()
        if not changes:
            return
        # Do not allow to remove essential packages
        for pkg in changes:
            if pkg.marked_delete and (pkg.essential == True or \
                                      (pkg.installed and \
                                       pkg.installed.priority == "required") or\
                                      pkg.name == "aptdaemon"):
                raise TransactionFailed(ERROR_NOT_REMOVE_ESSENTIAL_PACKAGE,
                                        "Package %s cannot be removed." % \
                                        pkg.name)
        self._check_obsoleted_dependencies()
        self._check_unauthenticated()
        if self.trans.cancelled:
            raise TransactionCancelled()
        self.trans.cancellable = False
        fetch_progress = DaemonAcquireProgress(self.trans,
                                               begin=fetch_range[0],
                                               end=fetch_range[1])
        inst_progress = DaemonInstallProgress(self.trans,
                                              begin=install_range[0],
                                              end=install_range[1])
        try:
            self._cache.commit(fetch_progress, inst_progress)
        except apt.cache.FetchFailedException, error:
            raise TransactionFailed(ERROR_PACKAGE_DOWNLOAD_FAILED,
                                    str(error.message))
        except apt.cache.FetchCancelledException:
            raise TransactionCancelled()
        except SystemError, excep:
            # Run dpkg --configure -a to recover from a failed transaction
            self.trans.status = STATUS_CLEANING_UP
            progress = DaemonDpkgRecoverProgress(self.trans, begin=90, end=95)
            progress.start_update()
            progress.run()
            progress.finish_update()
            output = inst_progress.output + progress.output
            raise TransactionFailed(ERROR_PACKAGE_MANAGER_FAILED,
                                    "%s: %s" % (excep, output))

    def simulate(self, trans, status_path=None):
        """Return the dependencies which will be installed by the transaction,
        the content of the dpkg status file after the transaction would have
        been applied, the download size and the required disk space.

        Keyword arguments:
        trans -- the transaction which should be simulated
        status_path -- the path to a dpkg status file on which the transaction
                 should be applied
        """
        log.info("Simulating trans: %s" % trans.tid)
        trans.status = STATUS_RESOLVING_DEP
        try:
            lock.lists_lock.acquire()
            return self._simulate_helper(trans, status_path)
        except lock.LockFailedError, error:
            if error.process:
                msg = "The package indexes are currently changed " \
                      "by %s." % error.process
            else:
                msg = "Another package manager changes the package indexes" \
                      "currently."
            trans.error = TransactionFailed(ERROR_NO_LOCK, msg)
        except TransactionFailed, excep:
            trans.error = excep
        except Exception, excep:
            trans.error = TransactionFailed(ERROR_UNKNOWN,
                                            traceback.format_exc())
        finally:
            lock.lists_lock.release()
            trans.status = STATUS_SETTING_UP
        trans.exit = EXIT_FAILED
        trans.progress = 100
        self.last_action_timestamp = time.time()
        raise trans.error

    def _simulate_helper(self, trans, status_path):
        #FIXME: A lot of redundancy
        #FIXME: Add checks for obsolete dependencies and unauthenticated
        def get_base_records(sec, additional=None):
            records = ["Priority", "Installed-Size", "Architecture",
                       "Version", "Replaces", "Depends", "Conflicts",
                       "Breaks", "Recommends", "Suggests", "Provides",
                       "Pre-Depends", "Essential"]
            if additional:
                records.extend(additional)
            ret = ""
            for record in records:
                try:
                    ret += "%s: %s\n" % (record, sec[record])
                except KeyError:
                    pass
            return ret

        status = ""
        depends = [[], [], [], [], [], [], []]
        skip_pkgs = []
        size = 0
        installs = reinstalls = removals = purges = upgrades = downgrades = \
                kepts = upgradables = []

        # Only handle transaction which change packages
        #FIXME: Add support for ROLE_FIX_INCOMPLETE_INSTALL,
        #       ROLE_FIX_BROKEN_DEPENDS
        if trans.role not in [ROLE_INSTALL_PACKAGES, ROLE_UPGRADE_PACKAGES,
                              ROLE_UPGRADE_SYSTEM, ROLE_REMOVE_PACKAGES,
                              ROLE_COMMIT_PACKAGES, ROLE_INSTALL_FILE]:
            return depends, status, 0, 0

        # Fast forward the cache
        if not status_path:
            status_path = self._status_orig
        apt_pkg.config.set("Dir::State::status", status_path)
        apt_pkg.init_system()
        #FIXME: open cache in background after startup
        if not self._cache:
            self._cache = apt.cache.Cache()
        else:
            self._cache.open()

        if self._cache.broken_count:
            broken = [pkg.name for pkg in self._cache if pkg.is_now_broken]
            raise TransactionFailed(ERROR_CACHE_BROKEN, " ".join(broken))

        # Mark the changes and apply
        if trans.role == ROLE_UPGRADE_SYSTEM:
            #FIXME: Should be part of python-apt to avoid using private API
            upgradables = [self._cache[pkgname] \
                           for pkgname in self._cache._set \
                           if self._cache._depcache.is_upgradable(\
                                   self._cache._cache[pkgname])]
            upgradables = [pkg for pkg in self._cache if pkg.is_upgradable]
            self._cache.upgrade(not trans.kwargs["safe_mode"])
            self._call_plugins("modify_cache_after")
        elif trans.role == ROLE_INSTALL_FILE:
            deb = apt.debfile.DebPackage(trans.kwargs["path"].encode("UTF-8"),
                                         self._cache)
            if not deb.check():
                raise TransactionFailed(ERROR_DEP_RESOLUTION_FAILED,
                                        deb._failure_string)
            status += "Package: %s\n" % deb.pkgname
            status += "Status: install ok installed\n"
            status += get_base_records(deb)
            status += "\n"
            skip_pkgs.append(deb.pkgname)
            try:
                # Some manually created packages use thousands seperators
                # or invalid values - see LP #656633
                size = int(deb["Installed-Size"].replace(",","")) * 1024
            except (KeyError, AttributeError, ValueError):
                pass
            try:
                pkg = self._cache[deb.pkgname]
            except KeyError:
                trans.packages[PKGS_INSTALL] = [deb.pkgname]
            else:
                if pkg.is_installed:
                    # if we failed to get the size from the deb file do nor
                    # try to get the delta
                    if size != 0:
                        size -= pkg.installed.installed_size
                    trans.packages[PKGS_REINSTALL] = [deb.pkgname]
                else:
                    trans.packages[PKGS_INSTALL] = [deb.pkgname]
            installs, reinstalls, removal, purges, upgrades = trans.packages
            self._call_plugins("modify_cache_after")
        else:
            ac = self._cache.actiongroup()
            installs, reinstalls, removals, purges, upgrades = trans.packages
            resolver = apt.cache.ProblemResolver(self._cache)
            self._mark_packages_for_installation(installs, resolver)
            self._mark_packages_for_installation(reinstalls, resolver,
                                                 reinstall=True)
            self._mark_packages_for_removal(removals, resolver)
            self._mark_packages_for_removal(purges, resolver, purge=True)
            self._mark_packages_for_upgrade(upgrades, resolver)
            self._resolve_depends(resolver)
            ac.release()
        changes = self._cache.get_changes()
        changes_names = []

        # get the additional dependencies
        for pkg in changes:
            if pkg.marked_upgrade and pkg.is_installed and \
               not pkg.name in upgrades:
                depends[PKGS_UPGRADE].append(pkg.name)
            elif pkg.marked_reinstall and not pkg.name in reinstalls:
                depends[PKGS_REINSTALL].append(pkg.name)
            elif pkg.marked_downgrade and not pkg.name in downgrades:
                depends[PKGS_DOWNGRADE].append(pkg.name)
            elif pkg.marked_install and not pkg.name in installs:
                depends[PKGS_INSTALL].append(pkg.name)
            elif pkg.marked_delete and not pkg.name in removals:
                depends[PKGS_REMOVE].append(pkg.name)
            #FIXME: add support for purges
            changes_names.append(pkg.name)
        # Check for skipped upgrades
        for pkg in upgradables:
            if not pkg in changes or not pkg.marked_upgrade:
                depends[PKGS_KEEP].append(pkg.name)

        # merge the changes into the dpkg status
        for sec in apt_pkg.TagFile(open(status_path)):
            pkg_name = sec["Package"]
            if pkg_name in skip_pkgs:
                continue
            status += "Package: %s\n" % pkg_name
            if pkg_name in changes_names:
                pkg = self._cache[sec["Package"]]
                if pkg.marked_delete:
                    status += "Status: deinstall ok config-files\n"
                    version = pkg.installed
                else:
                    # Install, Upgrade, downgrade and reinstall all use the
                    # candidate version
                    version = pkg.candidate
                    status += "Status: install ok installed\n"
                # Corner-case: a purge of an already removed package won't
                # have an installed version
                if version:
                    status += get_base_records(version.record)
                changes.remove(pkg)
            else:
                status += get_base_records(sec, ["Status"])
            status += "\n"
        # Add changed and not yet known (installed) packages to the status
        for pkg in changes:
            version = pkg.candidate
            status += "Package: %s\n" % pkg.name
            status += "Status: install ok installed\n"
            status += get_base_records(pkg.candidate.record)
            status += "\n"

        return depends, status, self._cache.required_download, \
               size + self._cache.required_space


class DummyWorker(AptWorker):

    """Allows to test the daemon without making any changes to the system."""

    def run(self, transaction):
        """Process the given transaction in the background.

        Keyword argument:
        transaction -- core.Transcation instance to run
        """
        log.info("Processing transaction %s", transaction.tid)
        if self.trans:
            raise Exception("There is already a running transaction")
        self.trans = transaction
        self.last_action_timestamp = time.time()
        self.trans.status = STATUS_RUNNING
        self.trans.progress = 0
        self.trans.cancellable = True
        gobject.timeout_add(200, self._process_transaction, transaction)

    def _process_transaction(self, trans):
        """Run the worker"""
        if trans.cancelled:
            trans.exit = EXIT_CANCELLED
        elif trans.progress == 100:
            trans.exit = EXIT_SUCCESS
        elif trans.role == ROLE_UPDATE_CACHE:
            trans.exit = EXIT_FAILED
        elif trans.role == ROLE_UPGRADE_PACKAGES:
            trans.exit = EXIT_SUCCESS
        elif trans.role == ROLE_UPGRADE_SYSTEM:
            trans.exit = EXIT_CANCELLE
        else:
            if trans.role == ROLE_INSTALL_PACKAGES:
                if trans.progress == 1:
                    trans.status = STATUS_RESOLVING_DEP
                elif trans.progress == 5:
                    trans.status = STATUS_DOWNLOADING
                elif trans.progress == 50:
                    trans.status = STATUS_COMMITTING
                    trans.status_details = "Heyas!"
                elif trans.progress == 55:
                    trans.paused = True
                    trans.status = STATUS_WAITING_CONFIG_FILE_PROMPT
                    trans.config_file_conflict = "/etc/fstab", "/etc/mtab"
                    while trans.paused:
                        gobject.main_context_default().iteration()
                    trans.config_file_conflict_resolution = None
                    trans.config_file_conflict = None
                    trans.status = STATUS_COMMITTING
                elif trans.progress == 60:
                    trans.required_medium = ("Debian Lenny 5.0 CD 1",
                                             "USB CD-ROM")
                    trans.paused = True
                    trans.status = STATUS_WAITING_MEDIUM
                    while trans.paused:
                        gobject.main_context_default().iteration()
                    trans.status = STATUS_DOWNLOADING
                elif trans.progress == 70:
                    trans.status_details = "Servus!"
                elif trans.progress == 90:
                    trans.status_deatils = ""
                    trans.status = STATUS_CLEANING_UP
            elif trans.role == ROLE_REMOVE_PACKAGES:
                if trans.progress == 1:
                    trans.status = STATUS_RESOLVING_DEP
                elif trans.progress == 5:
                    trans.status = STATUS_COMMITTING
                    trans.status_details = "Heyas!"
                elif trans.progress == 50:
                    trans.status_details = "Hola!"
                elif trans.progress == 70:
                    trans.status_details = "Servus!"
                elif trans.progress == 90:
                    trans.status_deatils = ""
                    trans.status = STATUS_CLEANING_UP
            trans.progress += 1
            return True
        trans.status = STATUS_FINISHED
        self.last_action_timestamp = time.time()
        tid = self.trans.tid[:]
        trans = self.trans
        self.trans = None
        self._emit_transaction_done(trans)
        log.info("Finished transaction %s", tid)
        return False

    def simulate(self, trans, status_path=None):
        depends = [[], [], [], [], [], [], []]
        return depends, "", 0, 0


# vim:ts=4:sw=4:et
