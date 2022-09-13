#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Core components of aptdaemon.

This module provides the following core classes of the aptdaemon:
AptDaemon - complete daemon for managing software via DBus interface
Transaction - represents a software management operation
TransactionQueue - queue for aptdaemon transactions

The main function allows to run the daemon as a command.
"""
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

__all__ = ("Transaction", "TransactionQueue", "AptDaemon",
           "APTDAEMON_TRANSACTION_DBUS_INTERFACE", "APTDAEMON_DBUS_INTERFACE"
           "APTDAEMON_DBUS_PATH", "APTDAEMON_DBUS_SERVICE",
           "APTDAEMON_IDLE_CHECK_INTERVAL", "APTDAEMON_IDLE_TIMEOUT",
           "TRANSACTION_IDLE_TIMEOUT", "TRANSACTION_DEL_TIMEOUT")

import collections
from functools import wraps
import gettext
from gettext import gettext as _
import locale
import logging
import logging.handlers
from optparse import OptionParser
import os
import Queue
import signal
import sys
import tempfile
import time
import threading
import uuid

import gobject
import dbus.exceptions
import dbus.service
import dbus.mainloop.glib
import dbus.glib
from softwareproperties.AptAuth import AptAuth
import apt_pkg

import errors
import enums
from defer import dbus_deferred_method, Deferred, inline_callbacks, return_value
import policykit1
from worker import AptWorker, DummyWorker
from loop import mainloop

# Setup i18n
gettext.textdomain("aptdaemon")

APTDAEMON_DBUS_INTERFACE = 'org.debian.apt'
APTDAEMON_DBUS_PATH = '/org/debian/apt'
APTDAEMON_DBUS_SERVICE = 'org.debian.apt'

APTDAEMON_TRANSACTION_DBUS_INTERFACE = 'org.debian.apt.transaction'

APTDAEMON_IDLE_CHECK_INTERVAL = 60
APTDAEMON_IDLE_TIMEOUT = 5 * 60

# Maximum allowed time between the creation of a transaction and its queuing
TRANSACTION_IDLE_TIMEOUT = 300
# Keep the transaction for the given time alive on the bus after it has
# finished
TRANSACTION_DEL_TIMEOUT = 5

# Setup the DBus main loop
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

# Required for daemon mode
os.putenv("PATH",
          "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin")

# Setup logging to syslog and the console
log = logging.getLogger("AptDaemon")
try:
    _syslog_handler = logging.handlers.SysLogHandler(address="/dev/log",
                             facility=logging.handlers.SysLogHandler.LOG_DAEMON)
    _syslog_handler.setLevel(logging.INFO)
    _syslog_formatter = logging.Formatter("%(name)s: %(levelname)s: "
                                          "%(message)s")
    _syslog_handler.setFormatter(_syslog_formatter)
except:
    pass
else:
    log.addHandler(_syslog_handler)
_console_handler = logging.StreamHandler()
_console_formatter = logging.Formatter("%(asctime)s %(name)s [%(levelname)s]: "
                                       "%(message)s",
                                       "%T")
_console_handler.setFormatter(_console_formatter)
log.addHandler(_console_handler)
#FIXME: Use LoggerAdapter (requires Python 2.6)
log_trans = logging.getLogger("AptDaemon.Trans")

# Required for translations from APT
try:
    locale.setlocale(locale.LC_ALL, "")
except locale.Error:
    log.warning("Failed to unset LC_ALL. Translations are not available.")


class Transaction(dbus.service.Object):

    """Represents a transaction on the D-Bus.

    A transaction represents a single package management task, e.g. installation
    or removal of packages. This class allows to expose information and to
    controll the transaction via DBus using policykit for managing
    privileges.
    """

    def __init__(self, role, queue, uid, sender, connect=True, bus=None,
                 packages=None, kwargs=None):
        """Initialize a new Transaction instance.

        Keyword arguments:
        queue -- TransactionQueue instance of the daemon
        uid -- the id of the user who created the transaction
        sender -- the DBus name of the sender who created the transaction
        connect -- if the Transaction should connect to DBus (default is True)
        bus -- the DBus connection which should be used (defaults to system bus)
        """
        id = uuid.uuid4().get_hex()
        self.tid = "/org/debian/apt/transaction/%s" % id
        if connect == True:
            if bus is None:
                bus = dbus.SystemBus()
            bus_name = dbus.service.BusName(APTDAEMON_DBUS_SERVICE, bus)
            dbus_path = self.tid
        else:
            bus = None
            bus_name = None
            dbus_path = None
        dbus.service.Object.__init__(self, bus_name, dbus_path)
        if not packages:
            packages = [[], [], [], [], []]
        if not kwargs:
            kwargs = {}
        self.queue = queue
        self.uid = uid
        self.locale = ""
        self.allow_unauthenticated = False
        self.remove_obsoleted_depends = False
        self.http_proxy = ""
        self.terminal = ""
        self.debconf = ""
        self.kwargs = kwargs
        # The transaction which should be executed after this one
        self.after = None
        self._role = role
        self._progress = 0
        self._progress_details = (0, 0, 0, 0, 0, 0)
        self._progress_download = ("", "", "", 0, 0, "")
        self._exit = enums.EXIT_UNFINISHED
        self._status = enums.STATUS_SETTING_UP
        self._status_details = ""
        self._error = ("", "")
        self._cancellable = True
        self._term_attached = False
        self._required_medium = ("", "")
        self._config_file_conflict = ("", "")
        self._config_file_conflict_resolution = ""
        self.cancelled = False
        self.paused = False
        self._meta_data = dbus.Dictionary(signature="ss")
        self._dpkg_status = None
        self._download = 0
        self._space = 0
        self._depends = [dbus.Array([], signature=dbus.Signature('s')) \
                         for i in range(7)]
        self._packages = [pkgs or dbus.Array([], signature=dbus.Signature('s'))\
                          for pkgs in packages]
        # Add a timeout which removes the transaction from the bus if it
        # hasn't been setup and run for the TRANSACTION_IDLE_TIMEOUT period
        self._idle_watch = gobject.timeout_add_seconds(
            TRANSACTION_IDLE_TIMEOUT, self._remove_from_connection_no_raise)
        # Handle a disconnect of the client application
        self.sender_alive = True
        if bus:
            self._sender_watch = bus.watch_name_owner(sender,
                                                     self._sender_owner_changed)
        else:
            self._sender_watch = None

    def _sender_owner_changed(self, connection):
        """Callback if the owner of the original sender changed, e.g.
        disconnected."""
        if not connection:
            self.sender_alive = False

    def _remove_from_connection_no_raise(self):
        """Version of remove_from_connection that does not raise if the
        object isn't exported.
        """
        log_trans.debug("Removing transaction")
        try:
            self.remove_from_connection()
        except LookupError, error:
            log_trans.debug("remove_from_connection() raised LookupError: "
                            "'%s'" % error)
        # Forget a not yet queued transaction
        try:
            self.queue.limbo.pop(self.tid)
        except KeyError:
            pass

    def _set_meta_data(self, data):
        # Perform some checks
        if self.status != enums.STATUS_SETTING_UP:
            raise errors.TransactionAlreadyRunning()
        if not isinstance(data, dbus.Dictionary):
            raise errors.InvalidMetaDataError("The data value has to be a "
                                              "dictionary: %s" % data)
        for key, value in data.iteritems():
            if key in self._meta_data:
                raise errors.InvalidMetaDataError("The key %s already "
                                                  "exists. It is not allowed "
                                                  "to overwrite existing "
                                                  "data." % key)
            if not len(key.split("_")) > 1:
                raise errors.InvalidMetaDataError("The key %s has to be of the "
                                                  "format IDENTIFIER-KEYNAME")
            if not isinstance(value, dbus.String):
                raise errors.InvalidMetaDataError("The value has to be a "
                                                  "string: %s" % value)
        # Merge new data into existing one:
        self._meta_data.update(data)
        self.PropertyChanged("MetaData", self._meta_data)

    def _get_meta_data(self):
        return self._meta_data

    meta_data = property(_get_meta_data, _set_meta_data,
                         doc="Allows client applications to store meta data "
                             "for the transaction in a dictionary.")

    def _set_role(self, enum):
        if self._role != enums.ROLE_UNSET:
            raise errors.TransactionRoleAlreadySet()
        self.PropertyChanged("Role", enum)
        self._role = enum

    def _get_role(self):
        return self._role

    role = property(_get_role, _set_role, doc="Operation type of transaction.")

    def _set_progress_details(self, details):
        items_done, items_total, bytes_done, bytes_total, speed, eta = details
        self._progress_details = details
        self.PropertyChanged("ProgressDetails", details)

    def _get_progress_details(self):
        return self._progress_details

    progress_details = property(_get_progress_details, _set_progress_details,
                                doc = "Tuple containing detailed progress "
                                      "information: items done, total items, "
                                      "bytes done, total bytes, speed and "
                                      "remaining time")

    def _set_error(self, excep):
        self._error = excep
        self.PropertyChanged("Error", (excep.code, excep.details))

    def _get_error(self):
        return self._error

    error = property(_get_error, _set_error, doc="Raised exception.")

    def _set_exit(self, enum):
        self.status = enums.STATUS_FINISHED
        self._exit = enum
        self.PropertyChanged("ExitState", enum)
        self.Finished(enum)
        if self._sender_watch:
            self._sender_watch.cancel()
        # Remove the transaction from the Bus after it is complete. A short
        # timeout helps lazy clients
        gobject.timeout_add_seconds(TRANSACTION_DEL_TIMEOUT,
                                    self._remove_from_connection_no_raise)

    def _get_exit(self):
        return self._exit

    exit = property(_get_exit, _set_exit,
                    doc="The exit state of the transaction.")

    def _get_download(self):
        return self._download

    def _set_download(self, size):
        self.PropertyChanged("Download", size)
        self._download = size

    download = property(_get_download, _set_download,
                        doc="The download size of the transaction.")

    def _get_space(self):
        return self._space

    def _set_space(self, size):
        self.PropertyChanged("Space", size)
        self._space = size

    space = property(_get_space, _set_space,
                     doc="The required disk space of the transaction.")

    def _get_packages(self):
        return self._packages

    def _set_packages(self, packages):
        self._packages = [dbus.Array(pkgs, signature=dbus.Signature('s')) \
                          for pkgs in packages]
        self.PropertyChanged("Packages", self._packages)

    packages = property(_get_packages, _set_packages,
                        doc="Packages which will be explictly install, "
                            "upgraded, removed, purged or reinstalled.")

    def _get_depends(self):
        return self._depends

    def _set_depends(self, depends):
        self._depends = [dbus.Array(deps, signature=dbus.Signature('s')) \
                         for deps in depends]
        self.PropertyChanged("Dependencies", self._depends)

    depends = property(_get_depends, _set_depends,
                       doc="The additional dependencies: installs, removals, "
                           "upgrades and downgrades.")

    def _get_status(self):
        return self._status

    def _set_status(self, enum):
        self.PropertyChanged("Status", enum)
        self._status = enum

    status = property(_get_status, _set_status,
                      doc="The status of the transaction.")

    def _get_status_details(self):
        return self._status_details

    def _set_status_details(self, text):
        self._status_details = text
        self.PropertyChanged("StatusDetails", text)

    status_details = property(_get_status_details, _set_status_details,
                              doc="The status message from apt.")

    def _get_progress(self):
        return self._progress

    def _set_progress(self, percent):
        self._progress = percent
        self.PropertyChanged("Progress", percent)

    progress = property(_get_progress, _set_progress,
                      doc="The progress of the transaction in percent.")

    def _get_progress_download(self):
        return self._progress_download

    def _set_progress_download(self, progress_download):
        self._progress_download = progress_download
        self.PropertyChanged("ProgressDownload", progress_download)

    progress_download = property(_get_progress_download,
                                 _set_progress_download,
                                 doc="The last progress update of a currently"
                                     "running download. A tuple of URI, "
                                     "status, short description, full size, "
                                     "partially downloaded size and a status "
                                     "message.")

    def _get_cancellable(self):
        return self._cancellable

    def _set_cancellable(self, cancellable):
        self._cancellable = cancellable
        self.PropertyChanged("Cancellable", cancellable)

    cancellable = property(_get_cancellable, _set_cancellable,
                           doc="If it's currently allowed to cancel the "
                               "transaction.")

    def _get_term_attached(self):
        return self._term_attached

    def _set_term_attached(self, attached):
        self._term_attached = attached
        self.PropertyChanged("TerminalAttached", attached)

    term_attached = property(_get_term_attached, _set_term_attached,
                             doc="If the controlling terminal is currently "
                                 "attached to the dpkg call of the "
                                 "transaction.")

    def _get_required_medium(self):
        return self._required_medium

    def _set_required_medium(self, medium):
        label, drive = medium
        self._required_medium = medium
        self.PropertyChanged("RequiredMedium", medium)
        self.MediumRequired(label, drive)

    required_medium = property(_get_required_medium, _set_required_medium,
                               doc="Tuple containing the label and the drive "
                                   "of a required CD/DVD to install packages "
                                   "from.")

    def _get_config_file_conflict(self):
        return self._config_file_conflict

    def _set_config_file_conflict(self, prompt):
        if prompt is None:
            self._config_file_conflict = None
            return
        old, new = prompt
        self._config_file_conflict = prompt
        self.PropertyChanged("ConfigFileConflict", prompt)
        self.ConfigFileConflict(old, new)

    config_file_conflict = property(_get_config_file_conflict,
                                    _set_config_file_conflict,
                                    doc="Tuple containing the old and the new "
                                      "path of the configuration file")

    # Signals

    @dbus.service.signal(dbus_interface=APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                         signature="sv")
    def PropertyChanged(self, property, value):
        """Set and emit if a property of the transaction changed.

        Keyword argument:
        property -- the name of the property
        value -- the new value of the property
        """
        log_trans.debug("Emitting PropertyChanged: %s, %s" % (property, value))

    @dbus.service.signal(dbus_interface=APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                         signature="s")
    def Finished(self, enum):
        """Mark and emit the transaction as finished.

        Keyword argument:
        enum -- the exit state of the transaction, e.g. EXIT_FAILED
        """
        log_trans.debug("Emitting Finished: %s" % \
                        enums.get_exit_string_from_enum(exit))

    @dbus.service.signal(dbus_interface=APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                         signature="ss")
    def MediumRequired(self, medium, drive):
        """Set and emit the required medium change.

        This method/signal should be used to inform the user to
        insert the installation CD/DVD:

        Keyword arguments:
        medium -- the CD/DVD label
        drive -- mount point of the drive
        """
        log_trans.debug("Emitting MediumRequired: %s, %s" % (medium, drive))

    @dbus.service.signal(dbus_interface=APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                         signature="ss")
    def ConfigFileConflict(self, old, new):
        """Set and emit the ConfigFileConflict signal.

        This method/signal should be used to inform the user to
        answer a config file prompt.

        Keyword arguments:
        old -- current version of the configuration prompt
        new -- new version of the configuration prompt
        """
        log_trans.debug("Emitting ConfigFileConflict: %s, %s" % (old, new))

    # Methods

    def _set_locale(self, locale_str):
        """Set the language and encoding.

        Keyword arguments:
        locale -- specifies language, territory and encoding according
                  to RFC 1766,  e.g. "de_DE.UTF-8"
        """

        if self.status != enums.STATUS_SETTING_UP:
            raise errors.TransactionAlreadyRunning()
        try:
            (lang, encoding) = locale._parse_localename(locale_str)
        except ValueError:
            raise
        else:
            self.locale = "%s.%s" % (lang, encoding)
            self.PropertyChanged("locale", self.locale)

    @inline_callbacks
    def _set_http_proxy(self, url, sender):
        """Set an http network proxy.

        Keyword arguments:
        url -- the URL of the proxy server, e.g. http://proxy:8080
        """
        if url != "" and (not url.startswith("http://") or not ":" in url):
            raise errors.InvalidProxyError(proxy)
        action = policykit1.PK_ACTION_SET_PROXY
        yield policykit1.check_authorization_by_name(sender, action)
        self.http_proxy = url
        self.PropertyChanged("HttpProxy", self.http_proxy)

    def _set_remove_obsoleted_depends(self, remove_obsoleted_depends):
        """Set the handling of the removal of automatically installed
        dependencies which are now obsoleted.

        Keyword arguments:
        remove_obsoleted_depends -- If True also remove automatically installed
            dependencies of to removed packages
        """
        self.remove_obsoleted_depends = remove_obsoleted_depends
        self.PropertyChanged("RemoveObsoletedDepends",
                             self.remove_obsoleted_depends)

    def _set_allow_unauthenticated(self, allow_unauthenticated):
        """Set the handling of unauthenticated packages 

        Keyword arguments:
        allow_unauthenticated -- True to allow packages that come from a 
                           repository without a valid authentication signature
        """
        self.allow_unauthenticated = allow_unauthenticated
        self.PropertyChanged("AllowUnauthenticated", self.allow_unauthenticated)

    @dbus.service.method(APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                         in_signature="s", out_signature="",
                         sender_keyword="sender")
    def RunAfter(self, tid, sender):
        """Queue the transaction for processing after the given transaction.

        The transaction will also fail if the previous one failed.
        """
        log_trans.info("Queuing transaction %s", self.tid)
        try:
            trans_before = self.queue.limbo[tid]
        except KeyError:
            raise Exception("The given transaction doesn't exist or is "
                            "already queued!")
        if trans_before.after:
            raise Exception("There is already an after transaction!")
        trans_before.after = self

    @dbus_deferred_method(APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                          in_signature="", out_signature="",
                          sender_keyword="sender")
    def Run(self, sender):
        """Queue the transaction for processing."""
        log_trans.info("Queuing transaction %s", self.tid)
        return self._run(sender)

    @inline_callbacks
    def _run(self, sender):
        yield self._check_foreign_user(sender)
        self.queue.put(self.tid)
        next = self.after
        while next:
            self.queue.put(next.tid)
            next = next.after

    @dbus_deferred_method(APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                          in_signature="", out_signature="",
                          sender_keyword="sender")
    def Cancel(self, sender):
        """Cancel the transaction.

        Keyword argument:
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log_trans.info("Cancelling transaction %s", self.tid)
        return self._cancel(sender)

    @inline_callbacks
    def _cancel(self, sender):
        try:
            yield self._check_foreign_user(sender)
        except errors.ForeignTransaction:
            action = policykit1.PK_ACTION_CANCEL_FOREIGN
            yield policykit1.check_authorization_by_name(sender, action)
        try:
            self.queue.remove(self)
            log_trans.debug("Removed transaction from queue")
        except ValueError:
            pass
        else:
            self.status = enums.STATUS_CANCELLING
            self.exit = enums.EXIT_CANCELLED
            return
        if self.cancellable:
            log_trans.debug("Setting cancel event")
            self.cancelled = True
            self.status = enums.STATUS_CANCELLING
            self.paused = False
            return
        raise errors.AptDaemonError("Could not cancel transaction")

    @dbus_deferred_method(APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                          in_signature="", out_signature="",
                          sender_keyword="sender")
    def Simulate(self, sender):
        """Simulate a transaction to update its dependencies, future status,
        download size and required disk space.

        Call this method if you want to show changes before queuing the
        transaction.
        """
        log_trans.info("Simulate was called")
        return self._simulate(sender)

    @inline_callbacks
    def _simulate(self, sender):
        if self.status != enums.STATUS_SETTING_UP:
            raise errros.TransactionAlreadyRunning()
        yield self._check_foreign_user(sender)
        self.depends, self._dpkg_status, self.download, self.space = \
                self.queue.worker.simulate(self, self.queue.future_status)
        if self._idle_watch is not None:
            gobject.source_remove(self._idle_watch)
            self._idle_watch = None

    def _set_terminal(self, ttyname):
        """Set the controlling terminal.

        The worker will be attached to the specified slave end of a pty
        master/slave pair. This allows to interact with the 

        Can only be changed before the transaction is started.

        Keyword arguments:
        ttyname -- file path to the slave file descriptor
        """
        if self.status != enums.STATUS_SETTING_UP:
            raise errros.TransactionAlreadyRunning()
        if not os.access(ttyname, os.W_OK):
            raise errors.AptDaemonError("Pty device does not exist: "
                                        "%s" % ttyname)
        if not os.stat(ttyname)[4] == self.uid:
            raise errors.AptDaemonError("Pty device '%s' has to be owned by"
                                        "the owner of the transaction "
                                        "(uid %s) " % (ttyname, self.uid))
        try:
            slave_fd = os.open(ttyname, os.O_RDWR | os.O_NOCTTY)
            if os.isatty(slave_fd):
                self.terminal = ttyname
                self.PropertyChanged("Terminal", self.terminal)
            else:
                raise errors.AptDaemonError("%s isn't a tty" % ttyname)
        finally:
            os.close(slave_fd)

    def _set_debconf(self, debconf_socket):
        """Set the socket of the debconf proxy.

        The worker process forwards all debconf commands through this
        socket by using the passthrough frontend. On the client side
        debconf-communicate should be connected to the socket.

        Can only be changed before the transaction is started.

        Keyword arguments:
        debconf_socket: absolute path to the socket
        """
        if self.status != enums.STATUS_SETTING_UP:
            raise errors.TransactionAlreadyRunning()
        if not os.access(debconf_socket, os.W_OK):
            raise errors.AptDaemonError("socket does not exist: "
                                        "%s" % debconf_socket)
        if not os.stat(debconf_socket)[4] == self.uid:
            raise errors.AptDaemonError("socket '%s' has to be owned by the "
                                        "owner of the transaction " % ttyname)
        self.debconf = debconf_socket
        self.PropertyChanged("DebconfSocket", self.debconf)

    @dbus_deferred_method(APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                          in_signature="s", out_signature="",
                          sender_keyword="sender")
    def ProvideMedium(self, medium, sender):
        """Continue paused transaction with the inserted medium.

        If a media change is required to install packages from CD/DVD
        the transaction will be paused and could be resumed with this
        method.

        Keyword arguments:
        medium -- the label of the CD/DVD
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log_trans.info("Medium %s was provided", medium)
        return self._provide_medium(medium, sender)

    @inline_callbacks
    def _provide_medium(self, medium, sender):
        yield self._check_foreign_user(sender)
        if not self.required_medium:
            raise errors.AptDaemonError("There isn't any required medium.")
        if not self.required_medium[0] == medium:
            raise errros.AptDaemonError("The medium '%s' isn't "
                                        "requested." % medium)
        self.paused = False

    @dbus_deferred_method(APTDAEMON_TRANSACTION_DBUS_INTERFACE,
                          in_signature="ss", out_signature="",
                          sender_keyword="sender")
    def ResolveConfigFileConflict(self, config, answer, sender):
        """Resolve a configuration file conflict and continue the transaction.

        If a config file prompt is detected the transaction will be
        paused and could be resumed with this method.

        Keyword arguments:
        config -- the path to the original config file
        answer -- the answer to the configuration file question, can be
                  "keep" or "replace"
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log_trans.info("Resolved conflict of %s with %s", config, answer)
        return self._resolve_config_file_conflict(config, answer, sender)

    @inline_callbacks
    def _resolve_config_file_conflict(self, config, answer, sender):
        yield self._check_foreign_user(sender)
        if not self.config_file_conflict:
            raise errors.AptDaemonError("There isn't any config file prompt "
                                        "required")
        if answer not in ["keep", "replace"]:
            # FIXME: should we re-send the config file prompt
            #        message or assume the client is buggy and
            #        just use a safe default (like keep)?
            raise errors.AptDaemonError("Invalid value: %s" % answer)
        if not self.config_file_conflict[0] == config:
            raise errors.AptDaemonError("Invalid config file: %s" % config)
        self.config_file_conflict_resolution = answer
        self.paused = False

    @dbus_deferred_method(dbus.PROPERTIES_IFACE,
                          in_signature="ssv", out_signature="",
                          sender_keyword="sender")
    def Set(self, iface, property, value, sender):
        """Set a property.

        Only the user who intiaited the transaction is
        allowed to modify it.

        Keyword arguments:
        iface -- the interface which provides the property
        property -- the property which should be modified
        value -- the new value of the property
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.debug("Set() was called: %s, %s" % (property, value))
        return self._set(iface, property, value, sender)

    @inline_callbacks
    def _set(self, iface, property, value, sender):
        yield self._check_foreign_user(sender)
        if iface == APTDAEMON_TRANSACTION_DBUS_INTERFACE:
            if property == "MetaData":
                self._set_meta_data(value)
            elif property == "Terminal":
                self._set_terminal(value)
            elif property == "DebconfSocket":
                self._set_debconf(value)
            elif property == "Locale":
                self._set_locale(value)
            elif property == "RemoveObsoletedDepends":
                self._set_remove_obsoleted_depends(value)
            elif property == "AllowUnauthenticated":
                self._set_allow_unauthenticated(value)
            elif property == "HttpProxy":
                self._set_http_proxy(value, sender)
            else:
                raise dbus.exceptions.DBusException("Unknown or read only "
                                                    "property: %s" % property)
        else:
            raise dbus.exceptions.DBusException("Unknown interface: %s" % iface)

    @dbus.service.method(dbus.PROPERTIES_IFACE,
                         in_signature="s", out_signature="a{sv}")
    def GetAll(self, iface):
        """Get all available properties of the given interface."""
        log.debug("GetAll() was called: %s" % iface)
        return self._get_properties(iface)

    @dbus.service.method(dbus.PROPERTIES_IFACE,
                         in_signature="ss", out_signature="v")
    def Get(self, iface, property):
        """Return the value of the given property provided by the given 
        interface.
        """
        log.debug("Get() was called: %s, %s" % (iface, property))
        return self._get_properties(iface)[property]

    def _get_properties(self, iface):
        #FIXME: Provide introspection information by overriding the Introspect
        #       method of the dbus.service.Object
        if iface == APTDAEMON_TRANSACTION_DBUS_INTERFACE:
            return {"Role": self.role,
                    "Progress": self.progress,
                    "ProgressDetails": self.progress_details,
                    "ProgressDownload": self.progress_download,
                    "Status": self.status,
                    "StatusDetails": self.status_details,
                    "Cancellable": self.cancellable,
                    "TerminalAttached": self.term_attached,
                    "RequiredMedium": self.required_medium,
                    "ConfigFileConflict": self.config_file_conflict,
                    "ExitState": self.exit,
                    "Error": self._error,
                    "Locale": self.locale,
                    "Terminal": self.terminal,
                    "DebconfSocket": self.debconf,
                    "Paused": self.paused,
                    "AllowUnauthenticated": self.allow_unauthenticated,
                    "RemoveObsoletedDepends": self.remove_obsoleted_depends,
                    "HttpProxy": self.http_proxy,
                    "Packages": self.packages,
                    "MetaData": self.meta_data,
                    "Dependencies": self.depends,
                    "Download": self.download,
                    "Space": self.space
                    }
        else:
            return {}

    @inline_callbacks
    def _check_foreign_user(self, dbus_name):
        """Check if the transaction is owned by the given caller."""
        uid = yield policykit1.get_uid_from_dbus_name(dbus_name)
        if self.uid != uid:
            raise errors.ForeignTransaction()

    def _set_kwargs(self, kwargs):
        """Set the kwargs which will be send to the AptWorker."""
        self.kwargs = kwargs


class TransactionQueue(gobject.GObject):

    """Queue for transactions."""

    __gsignals__ = {"queue-changed":(gobject.SIGNAL_RUN_FIRST,
                                     gobject.TYPE_NONE,
                                     ()),
                    "future-status-changed":(gobject.SIGNAL_RUN_FIRST,
                                     gobject.TYPE_NONE,
                                     ())}

    def __init__(self, dummy):
        """Intialize a new TransactionQueue instance."""
        gobject.GObject.__init__(self)
        self._queue = collections.deque()
        self._proc_count = 0
        if dummy:
            self.worker = DummyWorker()
        else:
            self.worker = AptWorker()
        self.future_status = None
        self.future_status_fd = None
        # Used to keep track of not yet queued transactions
        self.limbo = {}
        self.worker.connect("transaction-done", self._on_transaction_done)

    def __len__(self):
        return len(self._queue)

    def _emit_queue_changed(self):
        """Emit the queued-changed signal."""
        log.debug("emitting queue changed")
        self.emit("queue-changed")

    def _emit_future_status_changed(self):
        """Emit the future-status-changed signal."""
        log.debug("emitting future-status changed")
        self.emit("future-status-changed")
        #FIXME: All not yet queued transactions should listen to this signal
        #       and update be re-simulated if already done so

    def put(self, tid):
        """Add an item to the queue."""
        trans = self.limbo.pop(tid)
        #FIXME: Add a timestamp to check if the future status of the trans
        #       is really the later one
        # Simulate the new transaction if this has not been done before:
        if trans._dpkg_status is None:
            trans.depends, trans._dpkg_status, trans.download, trans.space = \
                self.worker.simulate(trans, self.future_status)
        # Replace the old future status with the new one
        if self.future_status_fd is not None:
            os.close(self.future_status_fd)
            os.remove(self.future_status)
        self.future_status_fd, self.future_status = \
            tempfile.mkstemp(prefix="future-status-")
        os.write(self.future_status_fd, trans._dpkg_status)
        self._emit_future_status_changed()

        if trans._idle_watch is not None:
            gobject.source_remove(trans._idle_watch)
        if self.worker.trans:
            trans.status = enums.STATUS_WAITING
            self._queue.append(trans)
        else:
            self.worker.run(trans)
        self._emit_queue_changed()

    def _on_transaction_done(self, worker, trans):
        """Mark the last item as done and request a new item."""
        #FIXME: Check if the transaction failed because of a broken system or
        #       if dpkg journal is dirty. If so allready queued transactions
        #       except the repair transactions should be removed from the queue
        if trans.exit in [enums.EXIT_FAILED, enums.EXIT_CANCELLED]:
            if trans.exit == enums.EXIT_FAILED:
                exit = enums.EXIT_PREVIOUS_FAILED
            else:
                exit = enums.EXIT_CANCELLED
            _trans = trans.after
            while _trans:
                self.remove(_trans)
                _trans.exit = exit
                msg = enums.get_role_error_from_enum(trans.role)
                _trans.status_details =  msg
                _trans = _trans.after
        try:
            next = self._queue.popleft()
        except IndexError:
            log.debug("There isn't any queued transaction")
            # Reset the future status to the system one
            if self.future_status_fd is not None:
                os.close(self.future_status_fd)
                os.remove(self.future_status)
            self.future_status_fd = None
            self.future_status = None
            self._emit_future_status_changed()
        else:
            self.worker.run(next)
        self._emit_queue_changed()

    def remove(self, transaction):
        """Remove the specified item from the queue."""
        # FIXME: handle future status
        self._queue.remove(transaction)
        self._emit_queue_changed()

    def clear(self):
        """Remove all items from the queue."""
        # FIXME: handle future status
        for transaction in self._queue:
            transaction._remove_from_connection_no_raise()
        self._queue.clear()

    @property
    def items(self):
        """Return a list containing all queued items."""
        return list(self._queue)


class AptDaemon(dbus.service.Object):

    """Provides a system daemon to process package management tasks.

    The daemon is transaction based. Each package management tasks runs
    in a separate transaction. The transactions can be created,
    monitored and managed via the D-Bus interface.
    """

    def __init__(self, options, connect=True, bus=None):
        """Initialize a new AptDaemon instance.

        Keyword arguments:
        options -- command line options of the type optparse.Values
        connect -- if the daemon should connect to the D-Bus (default is True)
        bus -- the D-Bus to connect to (defaults to the system bus)
        """
        log.info("Initializing daemon")
        signal.signal(signal.SIGQUIT, self._sigquit)
        signal.signal(signal.SIGTERM, self._sigquit)
        self.options = options
        if connect == True:
            if bus is None:
                bus = dbus.SystemBus()
            bus_path = APTDAEMON_DBUS_PATH
            # Check if another object has already registered the name on
            # the bus. Quit the other daemon if replace would be set
            try:
                bus_name = dbus.service.BusName(APTDAEMON_DBUS_SERVICE,
                                                bus,
                                                do_not_queue=True)
            except dbus.exceptions.NameExistsException:
                if self.options.replace == False:
                    log.critical("Another daemon is already running")
                    sys.exit(1)
                log.warn("Replacing already running daemon")
                the_other_guy = bus.get_object(APTDAEMON_DBUS_SERVICE,
                                               APTDAEMON_DBUS_PATH)
                the_other_guy.Quit(dbus_interface=APTDAEMON_DBUS_INTERFACE,
                                   timeout=300)
                time.sleep(1)
                bus_name = dbus.service.BusName(APTDAEMON_DBUS_SERVICE,
                                                bus,
                                                do_not_queue=True)
        else:
            bus_name = None
            bus_path =None
        dbus.service.Object.__init__(self, bus_name, bus_path)
        self.queue = TransactionQueue(options.dummy)
        self.queue.connect("queue-changed", self._on_queue_changed)
        log.debug("Setting up worker thread")
        log.debug("Daemon was initialized")

    def _on_queue_changed(self, queue):
        """Callback for a changed transaction queue."""
        if self.queue.worker.trans:
            current = self.queue.worker.trans.tid
        else:
            current = ""
        queued = [trans.tid for trans in self.queue.items]
        self.ActiveTransactionsChanged(current, queued)

    @dbus.service.signal(dbus_interface=APTDAEMON_DBUS_INTERFACE,
                         signature="sas")
    def ActiveTransactionsChanged(self, current, queued):
        """Emit the ActiveTransactionsChanged signal.

        Keyword arguments:
        current -- the tid of the currently running transaction or an empty
                   string
        queued -- list of the ids of the queued transactions
        """
        log.debug("Emitting ActiveTransactionsChanged signal: %s, %s",
                  current, queued)

    def run(self):
        """Start the daemon and listen for calls."""
        if self.options.disable_timeout == False:
            log.debug("Using inactivity check")
            gobject.timeout_add_seconds(APTDAEMON_IDLE_CHECK_INTERVAL,
                                        self._check_for_inactivity)
        log.debug("Waiting for calls")
        try:
            mainloop.run()
        except KeyboardInterrupt:
            self.Quit(None)

    @inline_callbacks
    def _create_trans(self, role, action, sender, packages=None, kwargs=None):
        """Helper method which returns the tid of a new transaction."""
        # Check silently if one of the high level privileges has been granted
        # before to reduce clicks to install packages from third party
        # repositories: AddRepository -> UpdateCache -> InstallPackages
        authorized = yield self._check_alternative_privileges(role, sender)
        if not authorized:
            yield policykit1.check_authorization_by_name(sender, action)
        uid = yield policykit1.get_uid_from_dbus_name(sender)
        trans = Transaction(role, self.queue, uid, sender, packages=packages,
                            kwargs=kwargs)
        self.queue.limbo[trans.tid] = trans
        return_value(trans.tid)

    @inline_callbacks
    def _check_alternative_privileges(self, role, sender):
        """Check non-interactively if one of the high level privileges
        has been granted.
        """
        if role not in [enums.ROLE_ADD_REPOSITORY, 
                        enums.ROLE_ADD_VENDOR_KEY_FROM_KEYSERVER,
                        enums.ROLE_UPDATE_CACHE,
                        enums.ROLE_INSTALL_PACKAGES]:
            return_value(False)
        flags = policykit1.CHECK_AUTH_NONE
        for action in [policykit1.PK_ACTION_INSTALL_PACKAGES_FROM_NEW_REPO,
                       policykit1.PK_ACTION_INSTALL_PURCHASED_PACKAGES]:
            try:
                yield policykit1.check_authorization_by_name(sender,
                                                             action,
                                                             flags=flags)
            except errors.NotAuthorizedError, error:
                continue
            else:
                return_value(True)
        return_value(False)

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="", out_signature="s",
                          sender_keyword="sender")
    def FixIncompleteInstall(self, sender):
        """Return the id of a newly create transaction which will try to
        fix incomplete installations by running dpkg --configure -a.

        Keyword argument:
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("FixIncompleteInstall() called")
        action = policykit1.PK_ACTION_INSTALL_OR_REMOVE_PACKAGES
        return self._create_trans(enums.ROLE_FIX_INCOMPLETE_INSTALL,
                                  action, sender)

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="", out_signature="s",
                          sender_keyword="sender")
    def FixBrokenDepends(self, sender):
        """Return the id of a newly create transaction which will try to
        fix broken dependencies.

        Keyword argument:
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("FixBrokenDepends() called")
        action = policykit1.PK_ACTION_INSTALL_OR_REMOVE_PACKAGES
        return self._create_trans(enums.ROLE_FIX_BROKEN_DEPENDS,
                                  action, sender)

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="", out_signature="s",
                          sender_keyword="sender")
    def UpdateCache(self, sender):
        """Return the id of a newly create transaction which will update
        the package cache.

        Download the latest information about available packages from the
        repositories.

        Keyword argument:
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("UpdateCache() was called")
        kwargs = {"sources_list": None}
        return self._create_trans(enums.ROLE_UPDATE_CACHE,
                                  policykit1.PK_ACTION_UPDATE_CACHE, sender,
                                  kwargs=kwargs)

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="s", out_signature="s",
                          sender_keyword="sender")
    def UpdateCachePartially(self, sources_list, sender):
        """Return the id of a newly create transaction which will update
        cache from the repositories defined in the given sources.list.

        Downloads the latest information about available packages from the
        repositories.

        Keyword argument:
        sources_list -- path to a sources.list, e.g.
            /etc/apt/sources.list.d/ppa-aptdaemon.list
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("UpdateCachePartially() was called")
        kwargs = {"sources_list": sources_list}
        return self._create_trans(enums.ROLE_UPDATE_CACHE,
                                  policykit1.PK_ACTION_UPDATE_CACHE, sender,
                                  kwargs=kwargs)

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="as", out_signature="s",
                          sender_keyword="sender")
    def RemovePackages(self, package_names, sender):
        """Return the id of a newly create transaction which will remove
        the given packages.

        Keyword arguments:
        package_names -- list of package names to remove
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("RemovePackages() was called: '%s'", package_names)
        action = policykit1.PK_ACTION_INSTALL_OR_REMOVE_PACKAGES
        return self._create_trans(enums.ROLE_REMOVE_PACKAGES,
                                  action, sender,
                                  packages=([], [], package_names, [], []))

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="b", out_signature="s",
                          sender_keyword="sender")
    def UpgradeSystem(self, safe_mode, sender):
        """Upgrade the whole system.

        If in safe mode only already installed packages will be updated.
        Updates which require to remove installed packages or to install
        additional packages will be skipped.

        Keyword arguments:
        safe_mode -- boolean value if the safe mode should be used
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("UpgradeSystem() was called with safe mode: "
                 "%s" % safe_mode)
        return self._create_trans(enums.ROLE_UPGRADE_SYSTEM,
                                  policykit1.PK_ACTION_UPGRADE_PACKAGES,
                                  sender,
                                  kwargs={"safe_mode": safe_mode})

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="asasasasas", out_signature="s",
                          sender_keyword="sender")
    def CommitPackages(self, install, reinstall, remove, purge, upgrade,
                       sender):
        """Perform a complex package operation.

        Keyword arguments:
        install -- package names to be installed
        reinstall -- packages names to be reinstalled
        remove -- package names to be removed
        purge -- package names to be removed including configuration
        upgrade -- package names to be upgraded
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        #FIXME: take sha1 or md5 cash into accout to allow selecting a version
        #       or an origin different from the candidate
        log.info("CommitPackages() was called: %s, %s, %s, %s, %s",
                 install, reinstall, remove, purge, upgrade)
        return self._commit_packages(install, reinstall, remove, purge, upgrade,
                                     sender)

    @inline_callbacks
    def _commit_packages(self, install, reinstall, remove, purge, upgrade,
                         sender):
        def check_empty_list(lst):
            if lst == [""]:
                return []
            else:
                return lst
        packages = [check_empty_list(lst) for lst in [install, reinstall,
                                                      remove, purge, upgrade]]
        if install != [""] or reinstall != [""] or \
           remove !=  [""] or purge != [""]:
            action = policykit1.PK_ACTION_INSTALL_OR_REMOVE_PACKAGES
            yield policykit1.check_authorization_by_name(sender, action)
        elif upgrade != [""]:
            action = policykit1.PK_ACTION_UPGRADE_PACKAGES
            yield policykit1.check_authorization_by_name(sender, action)
        uid = yield policykit1.get_uid_from_dbus_name(sender)
        trans = Transaction(enums.ROLE_COMMIT_PACKAGES, self.queue, uid,
                            sender, packages=packages)
        self.queue.limbo[trans.tid] = trans
        return_value(trans.tid)

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="as", out_signature="s",
                          sender_keyword="sender")
    def InstallPackages(self, package_names, sender):
        """Install the given packages.

        Keyword arguments:
        package_names -- list of package names to install
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("InstallPackages() was called: %s" % package_names)
        action = policykit1.PK_ACTION_INSTALL_OR_REMOVE_PACKAGES
        return self._create_trans(enums.ROLE_INSTALL_PACKAGES,
                                  action, sender,
                                  packages=(package_names, [], [], [], []))

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                         in_signature="as", out_signature="s",
                         sender_keyword="sender")
    def UpgradePackages(self, package_names, sender):
        """Upgrade the given packages to their latest version.

        Keyword arguments:
        package_names -- list of package names to upgrade
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("UpgradePackages() was called: %s" % package_names)
        return self._create_trans(enums.ROLE_UPGRADE_PACKAGES,
                                  policykit1.PK_ACTION_UPGRADE_PACKAGES,
                                  sender,
                                  packages=([], [], [], [], package_names))

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="ss", out_signature="s",
                          sender_keyword="sender")
    def AddVendorKeyFromKeyserver(self, keyid, keyserver, sender):
        """Download and install the given keyid via keyserver

        Keyword arguments:
        keyid - the keyid of the key (e.g. 0x0EB12F05)
        keyserver - the keyserver (e.g. keyserver.ubuntu.com)
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        #FIXME: Should not be a transaction
        log.info("InstallVendorKeyFromKeyserver() was called: %s %s" % (keyid, keyserver))
        return self._create_trans(enums.ROLE_ADD_VENDOR_KEY_FROM_KEYSERVER,
                                  policykit1.PK_ACTION_CHANGE_REPOSITORY,
                                  sender,
                                  kwargs={"keyid": keyid,
                                          "keyserver": keyserver})

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="s", out_signature="s",
                          sender_keyword="sender")
    def AddVendorKeyFromFile(self, path, sender):
        """Install the given key file.

        Keyword arguments:
        path -- the absolute path to the key file
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        #FIXME: Should not be a transaction
        log.info("InstallVendorKeyFile() was called: %s" % path)
        return self._create_trans(enums.ROLE_ADD_VENDOR_KEY_FILE,
                                  policykit1.PK_ACTION_CHANGE_REPOSITORY,
                                  sender,
                                  kwargs={"path": path})

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="s", out_signature="s",
                          sender_keyword="sender")
    def RemoveVendorKey(self, fingerprint, sender):
        """Remove the given key.

        Keyword arguments:
        fingerprint -- the fingerprint of the key to remove
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        #FIXME: Should not be a transaction
        log.info("RemoveVendorKey() was called: %s" % fingerprint)
        return self._create_trans(enums.ROLE_REMOVE_VENDOR_KEY,
                                  policykit1.PK_ACTION_CHANGE_REPOSITORY,
                                  sender,
                                  kwargs={"fingerprint": fingerprint})

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="s", out_signature="s",
                          sender_keyword="sender")
    def InstallFile(self, path, sender):
        """Install the given package file.

        Keyword arguments:
        path -- the absolute path to the package file
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("InstallFile() was called: %s" % path)
        #FIXME: Perform some checks
        #FIXME: Should we already extract the package name here?
        return self._create_trans(enums.ROLE_INSTALL_FILE,
                                  policykit1.PK_ACTION_INSTALL_FILE,
                                  sender,
                                  kwargs={"path": path})

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="sssasss", out_signature="s",
                          sender_keyword="sender")
    def AddRepository(self, rtype, uri, dist, comps, comment, sourcesfile,
                      sender):
        """Add given repository to the sources list.

        Keyword arguments:
        rtype -- the type of the entry (deb, deb-src)
        uri -- the main repository uri (e.g. http://archive.ubuntu.com/ubuntu)
        dist -- the distribution to use (e.g. karmic, "/")
        comps -- a (possible empty) list of components (main, restricted)
        comment -- an (optional) comment
        sourcesfile -- an (optinal) filename in sources.list.d 
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("AddRepository() was called: type='%s' uri='%s' "
                 "dist='%s' comps='%s' comment='%s' sourcesfile='%s'",
                 type, uri, dist, comps, comment, sourcesfile)
        return self._create_trans(enums.ROLE_ADD_REPOSITORY,
                                  policykit1.PK_ACTION_CHANGE_REPOSITORY,
                                  sender,
                                  kwargs={"rtype": rtype, "uri": uri,
                                          "dist": dist, "comps": comps,
                                          "comment": comment,
                                          "sourcesfile": sourcesfile})

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="s", out_signature="s",
                          sender_keyword="sender")
    def EnableDistroComponent(self, component, sender):
        """Enable given component in the sources list.

        Keyword arguments:
        component -- a components, e.g. main or universe
        sender -- the unique D-Bus name of the sender (provided by D-Bus)
        """
        log.info("EnableComponent() was called: component='%s' ", component)
        return self._create_trans(enums.ROLE_ENABLE_DISTRO_COMP,
                                  policykit1.PK_ACTION_CHANGE_REPOSITORY,
                                  sender,
                                  kwargs={"component": component})

    @dbus_deferred_method(APTDAEMON_DBUS_INTERFACE,
                          in_signature="", out_signature="as",
                          sender_keyword="sender")
    def GetTrustedVendorKeys(self, sender):
        """Return a list of installed repository keys."""
        log.info("GetTrustedVendorKeys() was called")
        return self._get_trusted_vendor_keys(sender)

    @inline_callbacks
    def _get_trusted_vendor_keys(self, sender):
        aptauth = AptAuth()
        action = policykit1.PK_ACTION_GET_TRUSTED_VENDOR_KEYS
        yield policykit1.check_authorization_by_name(sender, action)
        return_value([key.decode("utf-8", "ignore") for key in aptauth.list()])

    @dbus.service.method(APTDAEMON_DBUS_INTERFACE,
                         in_signature="", out_signature="sas")
    def GetActiveTransactions(self):
        """Return the currently running transaction and the list of queued
        transactions.
        """
        log.debug("GetActiveTransactions() was called")
        queued = [trans.tid for trans in self.queue.items]
        if self.queue.worker.trans:
            current = self.queue.worker.trans.tid
        else:
            current = ""
        return current, queued

    @dbus.service.method(APTDAEMON_DBUS_INTERFACE,
                         in_signature="", out_signature="",
                         sender_keyword="caller_name")
    def Quit(self, caller_name):
        """Request a shutdown of the daemon.

        Keyword argument:
        caller_name -- the D-Bus name of the caller (provided by D-Bus)
        """
        log.info("Shutdown was requested")
        log.debug("Quitting main loop...")
        mainloop.quit()
        log.debug("Exit")

    def _sigquit(self, signum, frame):
        """Internal callback for the quit signal."""
        self.Quit(None)

    def _check_for_inactivity(self):
        """Shutdown the daemon if it has been inactive for time specified
        in APTDAEMON_IDLE_TIMEOUT.
        """
        log.debug("Checking for inactivity")
        timestamp = self.queue.worker.last_action_timestamp
        if not self.queue.worker.trans and \
           not gobject.main_context_default().pending() and \
           time.time() - timestamp > APTDAEMON_IDLE_TIMEOUT and \
           not self.queue:
            log.info("Quiting due to inactivity")
            self.Quit(None)
            return False
        return True


def main():
    """Allow to run the daemon from the command line."""
    parser = OptionParser()
    #FIXME: Workaround a bug in optparser which doesn't handle unicode/str
    #       correctly, see http://bugs.python.org/issue4391
    #       Shoudl be resolved by Python3
    enc = locale.getpreferredencoding()
    parser.add_option("-t", "--disable-timeout",
                      default=False,
                      action="store_true", dest="disable_timeout",
                      help=_("Do not shutdown the daemon because of "
                             "inactivity").decode(enc))
    parser.add_option("-d", "--debug",
                      default=False,
                      action="store_true", dest="debug",
                      help=_("Show internal processing "
                             "information").decode(enc))
    parser.add_option("-r", "--replace",
                      default=False,
                      action="store_true", dest="replace",
                      help=_("Quit and replace an already running "
                             "daemon").decode(enc))
    parser.add_option("-p", "--profile",
                      default=False,
                      action="store", type="string", dest="profile",
                      help=_("Store profile stats in the specified "
                             "file").decode(enc))
    parser.add_option("--dummy",
                      default=False,
                      action="store_true", dest="dummy",
                      help=_("Do not make any changes to the system (Only "
                             "of use to developers)").decode(enc))
    options, args = parser.parse_args()
    if options.debug == True:
        log.setLevel(logging.DEBUG)
    else:
        log.setLevel(logging.INFO)
        _console_handler.setLevel(logging.INFO)
    daemon = AptDaemon(options)
    if options.profile:
        import profile
        profiler = profile.Profile()
        profiler.runcall(daemon.run)
        profiler.dump_stats(options.profile)
        profiler.print_stats()
    else:
        daemon.run()

# vim:ts=4:sw=4:et
