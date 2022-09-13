#!/usr/bin/python
"""
The module provides a client to the PackageKit DBus interface. It allows to
perform basic package manipulation tasks in a cross distribution way, e.g.
to search for packages, install packages or codecs.
"""
# Copyright (C) 2008 Canonical Ltd.
# Copyright (C) 2008 Aidan Skinner <aidan@skinner.me.uk>
# Copyright (C) 2008 Martin Pitt <martin.pitt@ubuntu.com>
# Copyright (C) 2008 Tim Lauridsen <timlau@fedoraproject.org>
# Copyright (C) 2008-2009 Sebastian Heinlein <devel@glatzor.de>
#
# Licensed under the GNU General Public License Version 2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import locale
import weakref

import dbus
import dbus.glib
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
import gobject

import enums
import debconf
import defer
from errors import convert_dbus_exception, TransactionFailed

__all__ = ("AptTransaction", "AptClient", "get_transaction", "get_daemon")


class MemoizedTransaction(type):

    """Metaclass to cache transactions."""

    cache = weakref.WeakValueDictionary()

    def __call__(mcs, *args):
        tid = args[0]
        try:
            return mcs.cache[tid]
        except KeyError:
            mcs.cache[tid] = value = \
                    super(MemoizedTransaction, mcs).__call__(*args)
            return value


class MemoizedMixIn(MemoizedTransaction, gobject.GObjectMeta):

    """Helper meta class for merging"""


class AptTransaction(gobject.GObject):

    """Represents an aptdaemon transaction. It allows asynchronous and
    synchronous processing.
    """

    __metaclass__ = MemoizedMixIn

    __gsignals__ = {"finished": (gobject.SIGNAL_RUN_FIRST,
                                 gobject.TYPE_NONE,
                                 (gobject.TYPE_STRING,)),
                    "dependencies-changed": (gobject.SIGNAL_RUN_FIRST,
                                             gobject.TYPE_NONE,
                                             (gobject.TYPE_PYOBJECT,
                                              gobject.TYPE_PYOBJECT,
                                              gobject.TYPE_PYOBJECT,
                                              gobject.TYPE_PYOBJECT,
                                              gobject.TYPE_PYOBJECT,
                                              gobject.TYPE_PYOBJECT,
                                              gobject.TYPE_PYOBJECT)),
                    "download-changed": (gobject.SIGNAL_RUN_FIRST,
                                         gobject.TYPE_NONE,
                                         (gobject.TYPE_INT,)),
                    "space-changed": (gobject.SIGNAL_RUN_FIRST,
                                      gobject.TYPE_NONE,
                                      (gobject.TYPE_INT,)),
                    "error": (gobject.SIGNAL_RUN_FIRST,
                              gobject.TYPE_NONE,
                              (gobject.TYPE_STRING, gobject.TYPE_STRING)),
                    "role-changed": (gobject.SIGNAL_RUN_FIRST,
                                     gobject.TYPE_NONE,
                                     (gobject.TYPE_STRING,)),
                    "terminal-attached-changed": (gobject.SIGNAL_RUN_FIRST,
                                                    gobject.TYPE_NONE,
                                                    (gobject.TYPE_BOOLEAN,)),
                    "cancellable-changed": (gobject.SIGNAL_RUN_FIRST,
                                            gobject.TYPE_NONE,
                                            (gobject.TYPE_BOOLEAN,)),
                    "meta-data-changed": (gobject.SIGNAL_RUN_FIRST,
                                          gobject.TYPE_NONE,
                                          (gobject.TYPE_PYOBJECT,)),
                    "status-changed": (gobject.SIGNAL_RUN_FIRST,
                                       gobject.TYPE_NONE,
                                       (gobject.TYPE_STRING,)),
                    "status-details-changed": (gobject.SIGNAL_RUN_FIRST,
                                               gobject.TYPE_NONE,
                                               (gobject.TYPE_STRING,)),
                    "progress-changed": (gobject.SIGNAL_RUN_FIRST,
                                 gobject.TYPE_NONE,
                                 (gobject.TYPE_INT,)),
                    "progress-details-changed": (gobject.SIGNAL_RUN_FIRST,
                                         gobject.TYPE_NONE,
                                         (gobject.TYPE_INT, gobject.TYPE_INT,
                                          gobject.TYPE_INT, gobject.TYPE_INT,
                                          gobject.TYPE_INT, gobject.TYPE_INT)),
                    "progress-download-changed": (gobject.SIGNAL_RUN_FIRST,
                                         gobject.TYPE_NONE,
                                         (gobject.TYPE_STRING,
                                          gobject.TYPE_STRING,
                                          gobject.TYPE_STRING,
                                          gobject.TYPE_INT,
                                          gobject.TYPE_INT,
                                          gobject.TYPE_STRING)),
                    "packages-changed": (gobject.SIGNAL_RUN_FIRST,
                                         gobject.TYPE_NONE,
                                         (gobject.TYPE_PYOBJECT,
                                          gobject.TYPE_PYOBJECT,
                                          gobject.TYPE_PYOBJECT,
                                          gobject.TYPE_PYOBJECT,
                                          gobject.TYPE_PYOBJECT)),
                    "paused": (gobject.SIGNAL_RUN_FIRST,
                               gobject.TYPE_NONE,
                               ()),
                    "resumed": (gobject.SIGNAL_RUN_FIRST,
                                gobject.TYPE_NONE,
                                ()),
                    "allow-unauthenticated-changed": (gobject.SIGNAL_RUN_FIRST,
                                                      gobject.TYPE_NONE,
                                                      (gobject.TYPE_BOOLEAN,)),
                    "remove-obsoleted-depends-changed": (gobject.SIGNAL_RUN_FIRST,
                                                 gobject.TYPE_NONE,
                                                 (gobject.TYPE_BOOLEAN,)),
                    "locale-changed": (gobject.SIGNAL_RUN_FIRST,
                                       gobject.TYPE_NONE,
                                       (gobject.TYPE_STRING,)),
                    "terminal-changed": (gobject.SIGNAL_RUN_FIRST,
                                         gobject.TYPE_NONE,
                                         (gobject.TYPE_STRING,)),
                    "debconf-socket-changed": (gobject.SIGNAL_RUN_FIRST,
                                               gobject.TYPE_NONE,
                                               (gobject.TYPE_STRING,)),
                    "http-proxy-changed": (gobject.SIGNAL_RUN_FIRST,
                                           gobject.TYPE_NONE,
                                           (gobject.TYPE_STRING,)),
                    "medium-required": (gobject.SIGNAL_RUN_FIRST,
                                        gobject.TYPE_NONE,
                                        (gobject.TYPE_STRING,
                                         gobject.TYPE_STRING)),
                    "config-file-conflict": (gobject.SIGNAL_RUN_FIRST,
                                             gobject.TYPE_NONE,
                                             (gobject.TYPE_STRING,
                                              gobject.TYPE_STRING)),
                    }

    def __init__(self, tid, bus=None):
        gobject.GObject.__init__(self)
        self.tid = tid
        self.role = enums.ROLE_UNSET
        self.error = None
        self.error_code = None
        self.error_details = None
        self.exit = enums.EXIT_UNFINISHED
        self.cancellable = False
        self.term_attached = False
        self.required_medium = None
        self.config_file_conflict = None
        self.status = None
        self.status_details = ""
        self.progress = 0
        self.paused = False
        self.http_proxy = None
        self.dependencies = [[], [], [], [], [], [], []]
        self.packages = [[], [], [], [], []]
        self.meta_data = {}
        self.remove_obsoleted_depends = False
        self.download = 0
        self.downloads = {}
        self.space = 0
        self._locale = ""
        self._method = None
        self._args = []
        self._debconf_helper = None
        # Connect the signal handlers to the DBus iface
        if not bus:
            bus = dbus.SystemBus()
        self._proxy = bus.get_object("org.debian.apt", tid)
        self._iface = dbus.Interface(self._proxy, "org.debian.apt.transaction")
        # Watch for a crashed daemon which orphaned the dbus object
        self._owner_watcher = bus.watch_name_owner("org.debian.apt",
                                                   self._on_name_owner_changed)
        # main signals
        self._signal_matcher = \
            self._iface.connect_to_signal("PropertyChanged",
                                          self._on_property_changed,
                                          utf8_strings=True)

    def _on_name_owner_changed(self, connection):
        """Fail the transaction if the daemon died."""
        if connection == "" and self.exit == enums.EXIT_UNFINISHED:
            self._on_property_changed("Error", (enums.ERROR_DAEMON_DIED,
                                                "It seems that the daemon "
                                                "died."))
            self._on_property_changed("Cancellable", False)
            self._on_property_changed("TerminalAttached", False)
            self._on_property_changed("ExitState", enums.EXIT_FAILED)

    def _on_property_changed(self, property_name, value):
        """Callback for the PropertyChanged signal."""
        if property_name == "TerminalAttached":
            self.term_attached = value
            self.emit("terminal-attached-changed", value)
        elif property_name == "Cancellable":
            self.cancellable = value
            self.emit("cancellable-changed", value)
        elif property_name == "DebconfSocket":
            self.emit("debconf-socket-changed", value)
        elif property_name == "RemoveObsoletedDepends":
            self.emit("remove-obsoleted-depends-changed", value)
            self.remove_obsoleted_depends = value
        elif property_name == "AllowUnauthenticated":
            self.emit("allow-unauthenticated-changed", value)
        elif property_name == "Terminal":
            self.emit("terminal-changed", value)
        elif property_name == "Dependencies":
            self.dependencies = value
            self.emit("dependencies-changed", *value)
        elif property_name == "Packages":
            self.packages = value
            self.emit("packages-changed", *value)
        elif property_name == "Locale":
            self.locale = value
            self.emit("locale-changed", value)
        elif property_name == "Role":
            self.role = value
            self.emit("role-changed", value)
        elif property_name == "Status":
            self.status = value
            self.emit("status-changed", value)
        elif property_name == "StatusDetails":
            self.status_details = value
            self.emit("status-details-changed", value)
        elif property_name == "ProgressDownload":
            uri, status, desc, size, download, msg = value
            if uri:
                self.downloads[uri] = (status, desc, size, download, msg)
                self.emit("progress-download-changed", *value)
        elif property_name == "Progress":
            self.progress = value
            self.emit("progress-changed", value)
        elif property_name == "ConfigFileConflict":
            self.config_file_conflict = value
            if value != ("", ""):
                self.emit("config-file-conflict", *value)
        elif property_name == "MetaData":
            self.meta_data = value
            self.emit("meta-data-changed", value)
        elif property_name == "Paused":
            self.paused = value
            if value:
                self.emit("paused")
            else:
                self.emit("resumed")
        elif property_name == "RequiredMedium":
            self.required_medium = value
            if value != ("", ""):
                self.emit("medium-required", *value)
        elif property_name == "ProgressDetails":
            self.progress_details = value
            self.emit("progress-details-changed", *value)
        elif property_name == "Download":
            self.download = value
            self.emit("download-changed", value)
        elif property_name == "Space":
            self.space = value
            self.emit("space-changed", value)
        elif property_name == "HttpProxy":
            self.http_proxy = value
            self.emit("http-proxy-changed", value)
        elif property_name == "Error":
            self.error_code, self.error_details = value
            if self.error_code != "":
                self.error = TransactionFailed(self.error_code,
                                               self.error_details)
                self.emit("error", *value)
        elif property_name == "ExitState":
            self.exit = value
            if value != enums.EXIT_UNFINISHED:
                self.emit("finished", value)
                self._owner_watcher.cancel()
                if self._debconf_helper:
                    self._debconf_helper.stop()
                self.disconnect()

    @defer.deferable
    @convert_dbus_exception
    def sync(self, reply_handler=None, error_handler=None):
        """Sync the client transaction properites with the backend one.

        Keyword arguments:
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        def sync_properties(prop_dict):
            for property_name, value in prop_dict.iteritems():
                self._on_property_changed(property_name, value)
            if reply_handler:
                reply_handler(self)
        if reply_handler and error_handler:
            self._proxy.GetAll("org.debian.apt.transaction",
                               dbus_interface=dbus.PROPERTIES_IFACE,
                               reply_handler=sync_properties,
                               error_handler=error_handler)
        else:
            properties = self._proxy.GetAll("org.debian.apt.transaction",
                                           dbus_interface=dbus.PROPERTIES_IFACE)
            sync_properties(properties)

    @defer.deferable
    @convert_dbus_exception
    def run_after(self, transaction, reply_handler=None, error_handler=None):
        """Chain this transaction after the given one. The transaction will
        fail if the previous one fails.

        To start processing of the chain you have to call the run() method
        of the first transaction. The others will be queued after it
        automatically.

        Keyword arguments:
        transaction - an AptTransaction on which this one depends
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        try:
            return self._iface.RunAfter(transaction.tid,
                                        error_handler=error_handler,
                                        reply_handler=reply_handler)
        except Exception, error:
            if error_handler:
                error_handler(error)
            else:
                raise

    @defer.deferable
    @convert_dbus_exception
    def run(self, reply_handler=None, error_handler=None):
        """Start processing the transaction.

        Keyword arguments:
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        try:
            return self._iface.Run(error_handler=error_handler,
                                   reply_handler=reply_handler)
        except Exception, error:
            if error_handler:
                error_handler(error)
            else:
                raise

    @defer.deferable
    @convert_dbus_exception
    def simulate(self, reply_handler=None, error_handler=None):
        """Simulate the transaction to calculate the dependencies, the
        required download size and the required disk space.

        The corresponding properties of the transaction will be updated.

        Also TransactionFailed exceptions could be raised, if e.g. a
        requested package could not be installed or the cache is currently
        broken.

        Keyword arguments:
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        self._iface.Simulate(reply_handler=reply_handler,
                             error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def cancel(self, reply_handler=None, error_handler=None):
        """Cancel the running transaction.

        Keyword arguments:
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        self._iface.Cancel(reply_handler=reply_handler,
                           error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def set_http_proxy(self, proxy, reply_handler=None, error_handler=None):
        """Set the HttpProxy property of the transaction.

        Keyword arguments:
        proxy - the URL of the proxy server, e.g. "http://proxy:8080"
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        if reply_handler:
            _reply_handler = lambda: reply_handler(self)
        else:
            _reply_handler = None
        self._proxy.Set("org.debian.apt.transaction", "HttpProxy", proxy,
                        dbus_interface=dbus.PROPERTIES_IFACE,
                        reply_handler=_reply_handler,
                        error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def set_remove_obsoleted_depends(self, remove_obsoleted_depends,
                                     reply_handler=None, error_handler=None):
        """Set the RemoveObsoletedDepends protperty of the transaction.

        Keyword arguments:
        remove_obsoleted_depends - if True also remove no longer required
            dependencies which have been installed automatically before
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        if reply_handler:
            _reply_handler = lambda: reply_handler(self)
        else:
            _reply_handler = None
        self._proxy.Set("org.debian.apt.transaction", 
                        "RemoveObsoletedDepends", remove_obsoleted_depends,
                        dbus_interface=dbus.PROPERTIES_IFACE,
                        reply_handler=_reply_handler,
                        error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def set_allow_unauthenticated(self, allow_unauthenticated, 
                            reply_handler=None, error_handler=None):
        """Set AllowUnauthencitaed property of the transaction.

        Keyword arguments:
        allow_unauthenticated - if True the installation of packages which
            have not been signed by a trusted software vendor are allowed
            to be installed. Defaults to False.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        if reply_handler:
            _reply_handler = lambda: reply_handler(self)
        else:
            _reply_handler = None
        self._proxy.Set("org.debian.apt.transaction", 
                        "AllowUnauthenticated", allow_unauthenticated,
                        dbus_interface=dbus.PROPERTIES_IFACE,
                        reply_handler=_reply_handler,
                        error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def set_debconf_frontend(self, frontend, reply_handler=None,
                             error_handler=None):
        """Set the DebconfSocket property of the transaction.

        Keyword arguments:
        debconf_socket - a socket to which the debconf proxy frontend should
            be attached. A debconf frontend running in the user's session
            should listen to the socket.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        if reply_handler:
            _reply_handler = lambda: reply_handler(self)
        else:
            _reply_handler = None
        self._debconf_helper = debconf.DebconfProxy(frontend)
        self._proxy.Set("org.debian.apt.transaction", "DebconfSocket",
                        self._debconf_helper.socket_path,
                        dbus_interface=dbus.PROPERTIES_IFACE,
                        reply_handler=_reply_handler,
                        error_handler=error_handler)
        self._debconf_helper.start()

    @defer.deferable
    @convert_dbus_exception
    def set_meta_data(self, **kwargs):
        """Store additional meta information of the transaction in the
        MetaData property of the transaction.

        The method accepts of key=value pairs. The key has to be prefixed with
        an underscore separated identifier of the client application.

        In the following example Software-Center sets an application name
        and icon:

        >>> Transaction.set_meta_data(sc_icon="shiny", sc_app="xterm")

        Special keyword arguments:
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        reply_handler = kwargs.pop("reply_handler", None)
        error_handler = kwargs.pop("error_handler", None)
        if reply_handler:
            _reply_handler = lambda: reply_handler(self)
        else:
            _reply_handler = None
        self._proxy.Set("org.debian.apt.transaction", "MetaData", kwargs,
                        dbus_interface=dbus.PROPERTIES_IFACE,
                        reply_handler=_reply_handler,
                        error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def set_terminal(self, ttyname, reply_handler=None, error_handler=None):
        """Set the Terminal property of the transaction to attach a
        controlling terminal to the dpkg call.

        Keyword arguments:
        ttyname - the path to the master tty
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        if reply_handler:
            _reply_handler = lambda: reply_handler(self)
        else:
            _reply_handler = None
        self._proxy.Set("org.debian.apt.transaction", "Terminal", ttyname,
                        dbus_interface=dbus.PROPERTIES_IFACE,
                        reply_handler=_reply_handler,
                        error_handler=error_handler)

    def disconnect(self):
        """Stop monitoring the progress of the transaction."""
        if hasattr(self, "_signal_matcher"):
            self._signal_matcher.remove()
            del self._signal_matcher

    @defer.deferable
    @convert_dbus_exception
    def set_locale(self, locale_name, reply_handler=None, error_handler=None):
        """Set the language for status and error messages.

        Keyword arguments:
        locale_name - a supported locale, e.g. "de_DE@UTF-8"
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        if reply_handler:
            _reply_handler = lambda: reply_handler(self)
        else:
            _reply_handler = None
        self._proxy.Set("org.debian.apt.transaction", "Locale", locale_name,
                        dbus_interface=dbus.PROPERTIES_IFACE,
                        reply_handler=_reply_handler,
                        error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def provide_medium(self, medium, reply_handler=None, error_handler=None):
        """Continue a paused transaction which waited for a medium to install
        from.

        Keyword arguments:
        medium - the name of the provided medium
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        self._iface.ProvideMedium(medium, reply_handler=reply_handler,
                                  error_handler=error_handler)

    @defer.deferable
    @convert_dbus_exception
    def resolve_config_file_conflict(self, config, answer, reply_handler=None,
                                     error_handler=None):
        """Continue a paused transaction that waits for the resolution of a
        configuration file conflict.

        Keyword arguments:
        config - the path of the corresponding config file
        answer - can be either "keep" or "replace"
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        self._iface.ResolveConfigFileConflict(config, answer,
                                              reply_handler=reply_handler,
                                              error_handler=error_handler)


class AptClient:

    """Provides a complete client for aptdaemon."""

    def __init__(self):
        """Return a new AptClient instance."""
        self.bus = dbus.SystemBus()
        # Catch an invalid locale
        try:
            self._locale = "%s.%s" % locale.getdefaultlocale()
        except ValueError:
            self._locale = None
        self.terminal = None

    @convert_dbus_exception
    def get_trusted_vendor_keys(self, reply_handler=None, error_handler=None):
        """Return the list of trusted vendors.

        Keyword arguments:
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        daemon = get_aptdaemon()
        keys = daemon.GetTrustedVendorKeys(reply_handler=reply_handler,
                                           error_handler=error_handler)
        return keys

    @defer.deferable
    @convert_dbus_exception
    def upgrade_system(self, safe_mode=True, wait=False, reply_handler=None,
                       error_handler=None):
        """Return a transaction which upgrades the software installed on
        the system.

        Keyword arguments:
        safe_mode - if True skip upgrades which require the installation or
            removal of other packages. Defaults to True.
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("UpgradeSystem", [safe_mode],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def install_packages(self, package_names, wait=False, reply_handler=None,
                         error_handler=None):
        """Return a transaction which will the install the given packages.

        Keyword arguments:
        package_names - a list of package names
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("InstallPackages", [package_names],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def add_repository(self, src_type, uri, dist, comps=[], comment="",
                       sourcesfile="", wait=False, reply_handler=None,
                       error_handler=None):
        """Add repository to the sources list.

        Keyword arguments:
        src_type -- the type of the entry (deb, deb-src)
        uri -- the main repository uri (e.g. http://archive.ubuntu.com/ubuntu)
        dist -- the distribution to use (e.g. karmic, "/")
        comps -- a (possible empty) list of components (main, restricted)
        comment -- an (optional) comment
        sourcesfile -- an (optinal) filename in sources.list.d 
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
         """
        # dbus can not deal with empty lists and will error
        if comps == []:
            comps = [""]
        return self._run_transaction("AddRepository",
                                     [src_type, uri, dist, comps, comment,
                                      sourcesfile],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def add_vendor_key_from_keyserver(self, keyid, keyserver, wait=False, 
                                      reply_handler=None, error_handler=None):
        """Return a transaction which add the GPG key of a software vendor
        to the list of trusted ones by downloading it from a given keyserver.

        Keyword arguments:
        keyid - the keyid of the key (e.g. 0x0EB12F05)
        keyserver - the keyserver (e.g. keyserver.ubuntu.com)
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("AddVendorKeyFromKeyserver", 
                                     [keyid,keyserver],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def add_vendor_key_from_file(self, path, wait=False, reply_handler=None,
                                 error_handler=None):
        """Return a transaction which add the GPG key of a software vendor
        to the list of trusted ones.

        Keyword arguments:
        path - location of the key file
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("AddVendorKeyFromFile", [path],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def remove_vendor_key(self, fingerprint, wait=False, reply_handler=None,
                          error_handler=None):
        """Return a transaction which will remove the GPG of a software vendor
        from the list of trusted ones.

        Keyword arguments:
        fingerprint - identifier of the GPG key
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("RemoveVendorKey", [fingerprint],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def install_file(self, path, wait=False, reply_handler=None,
                     error_handler=None):
        """Return a transaction which installs a local package file.

        Keyword arguments:
        path - location of the package file
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("InstallFile", [path],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def upgrade_packages(self, package_names, wait=False, reply_handler=None,
                         error_handler=None):
        """Return a transaction which upgrades the given packages.

        Keyword arguments:
        package_names - list of package names which should be upgraded
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("UpgradePackages", [package_names],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def remove_packages(self, package_names, wait=False,
                        reply_handler=None, error_handler=None):
        """Return a transaction which removes the given packages.

        Keyword arguments:
        package_names - list of package names which should be removed
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("RemovePackages", [package_names],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def commit_packages(self, install, reinstall, remove, purge, upgrade,
                        wait=False, reply_handler=None, error_handler=None):
        """Return a transaction which performs a complex package managagement
        task which consits of installing, reinstalling, removing,
        purging and upgrading packages at the same time.

        Keyword arguments:
        install - list of package names to install
        reinstall - list of package names to reinstall
        remove - list of package names to remove
        purge - list of package names to purge
        upgrade - list of package names to upgrade
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        def check_empty_list(lst):
            if not lst:
                return [""]
            else:
                return lst
        pkgs = [check_empty_list(lst) for lst in [install, reinstall, remove,
                                                  purge, upgrade]]
        return self._run_transaction("CommitPackages", pkgs,
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def fix_broken_depends(self, wait=False, reply_handler=None,
                           error_handler=None):
        """Return a transacation which tries to fix broken dependencies.
        Use with care since currently there isn't any preview of the
        changes.

        Keyword arguments:
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("FixBrokenDepends", [],
                                     wait, reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def fix_incomplete_install(self, wait=False, reply_handler=None,
                               error_handler=None):
        """Return a transaction which tries to complete cancelled installations
        by calling dpkg --configure -a.

        Keyword arguments:
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("FixIncompleteInstall", [], wait,
                                     reply_handler, error_handler)

    @defer.deferable
    @convert_dbus_exception
    def update_cache(self, sources_list=None, wait=False,
                     reply_handler=None, error_handler=None):
        """Return a transaction which queries the software sources
        (package repositories) for available packages.

        Keyword arguments:
        sources_list - Path to a sources.list which contains repositories
            that should be updated only. The other repositories will
            be ignored in this case.
        wait - if True run the transaction immediately and return its exit
            state instead of the transaction itself.
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        if sources_list:
            return self._run_transaction("UpdateCachePartially", [sources_list],
                                         wait, reply_handler, error_handler)
        else:
            return self._run_transaction("UpdateCache", [], wait, reply_handler,
                                         error_handler)

    @defer.deferable
    @convert_dbus_exception
    def enable_distro_component(self, component, wait=False, reply_handler=None,
                                error_handler=None):
        """Add component to the sources list.

        Keyword arguments:
        component -- a components e.g. main or universe
        reply_handler - callback function. If specified in combination with
            error_handler the method will be called asynchrounsouly.
        error_handler - in case of an error the given callback gets the
            corresponding DBus exception instance
        """
        return self._run_transaction("EnableDistroComponent", [component],
                                     wait, reply_handler, error_handler)

    def _run_transaction(self, method_name, args, wait, reply_handler,
                         error_handler):
        async = reply_handler and error_handler
        try:
            deferred = self._run_transaction_helper(method_name, args, wait,
                                                    async)
        except Exception, error:
            if async:
                error_handler(error)
            else:
                raise
        if async:
            def on_error(error):
                """Convert the DeferredException to a normal exception."""
                try:
                    error.raise_exception()
                except Exception, error:
                    error_handler(error)
            deferred.add_callbacks(reply_handler)
            deferred.add_errback(on_error)
            return deferred
        else:
            # Iterate on the main loop - we cannot use a sub loop here,
            # since the D-Bus python bindings only work on the main loop
            context = gobject.main_context_default()
            while not hasattr(deferred, "result"):
                context.iteration(True)
           # If there has been an error in the helper raise it
            if isinstance(deferred.result, defer.DeferredException):
                deferred.result.raise_exception()
            trans = deferred.result
            if trans.error:
                raise trans.error
            if wait:
                # Wait until the transaction is complete and the properties
                # of the transaction have been updated
                while trans.exit == enums.EXIT_UNFINISHED:
                    context.iteration(True)
                return trans.exit
            else:
                return trans

    @defer.inline_callbacks
    def _run_transaction_helper(self, method_name, args, wait, async):
        daemon = get_aptdaemon()
        dbus_method = daemon.get_dbus_method(method_name)
        if async:
            deferred = defer.Deferred()
            dbus_method(reply_handler=deferred.callback,
                        error_handler=deferred.errback, *args, timeout=86400)
            tid = yield deferred
        else:
            tid = dbus_method(*args, timeout=86400)
        trans = AptTransaction(tid, self.bus)
        if self._locale:
            yield trans.set_locale(self._locale)
        if self.terminal:
            yield trans.set_terminal(self.terminal)
        yield trans.sync()
        if wait and async:
            deferred_wait = defer.Deferred()
            trans.connect("finished",
                          # avoid calling callback() twice, this can happen
                          # e.g. when sync() is called on a transaction,
                          # that emits all statues (including exit-state) again
                          lambda trans, exit: (not deferred_wait.called and
                                               deferred_wait.callback(exit)))
            yield trans.run()
            yield deferred_wait
        elif wait:
            yield trans.run()
        defer.return_value(trans)


@defer.deferable
@convert_dbus_exception
def get_transaction(tid, bus=None, reply_handler=None, error_handler=None):
    """Return an AptTransaction instance connected to the given transaction.

    Keyword arguments:
    tid -- the identifier of the transaction
    bus -- the D-Bus connection that should be used (defaults to the system bus)
    """
    if not bus:
        bus = dbus.SystemBus()
    trans = AptTransaction(tid, bus)
    if error_handler and reply_handler:
        trans.sync(reply_handler=reply_handler, error_handler=error_handler)
    else:
        trans.sync()
        return trans

def get_aptdaemon(bus=None):
    """Return the aptdaemon D-Bus interface.

    Keyword argument:
    bus -- the D-Bus connection that should be used (defaults to the system bus)
    """
    if not bus:
        bus = dbus.SystemBus()
    return dbus.Interface(bus.get_object("org.debian.apt",
                                         "/org/debian/apt",
                                         False),
                          "org.debian.apt")

# vim:ts=4:sw=4:et
