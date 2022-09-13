#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Exception classes"""
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

__all__ = ("AptDaemonError", "ForeignTransaction", "InvalidMetaDataError",
           "InvalidProxyError", "RepositoryInvalidError",
           "TransactionAlreadyRunning", "TransactionCancelled",
           "TransactionFailed", "TransactionRoleAlreadySet",
           "NotAuthorizedError", "convert_dbus_exception",
           "get_native_exception")

import inspect
import dbus
from functools import wraps


class AptDaemonError(dbus.DBusException):

    """Internal error of the aptdaemon"""

    _dbus_error_name = "org.debian.apt"


class TransactionRoleAlreadySet(AptDaemonError):

    """Error if a transaction has already been configured."""

    _dbus_error_name = "org.debian.apt.TransactionRoleAlreadySet"


class TransactionAlreadyRunning(AptDaemonError):

    """Error if a transaction has already been configured."""

    _dbus_error_name = "org.debian.apt.TransactionAlreadyRunning"


class ForeignTransaction(AptDaemonError):

    """Error if a transaction was initialized by a different user."""

    _dbus_error_name = "org.debian.apt.TransactionAlreadyRunning"


class TransactionFailed(AptDaemonError):

    """Internal error if a transaction could not be processed successfully."""

    _dbus_error_name = "org.debian.apt.TransactionFailed"

    def __init__(self, code, details=""):
        AptDaemonError.__init__(self, "%s: %s" % (code, details))
        self.code = code
        self.details = details


class InvalidMetaDataError(AptDaemonError):

    """Invalid meta data given"""

    _dbus_error_name = "org.debian.apt.InvalidMetaData"


class InvalidProxyError(AptDaemonError):

    """Invalid proxy given"""

    _dbus_error_name = "org.debian.apt.InvalidProxy"

    def __init__(self, proxy):
        AptDaemonError.__init__(self, "InvalidProxyError: %s" % proxy)


class TransactionCancelled(AptDaemonError):

    """Internal error if a transaction was cancelled."""

    _dbus_error_name = "org.debian.apt.TransactionCancelled"


class RepositoryInvalidError(AptDaemonError):

    """The added repository is invalid"""

    _dbus_error_name = "org.debian.apt.RepositoryInvalid"


class PolicyKitError(dbus.DBusException):
    pass

class NotAuthorizedError(PolicyKitError):

    _dbus_error_name = "org.freedesktop.PolicyKit.Error.NotAuthorized"

    def __init__(self, subject, action_id):
        dbus.DBusException.__init__(self, "%s: %s" % (subject, action_id))
        self.action_id = action_id
        self.subject = subject


class AuthorizationFailed(NotAuthorizedError):

    _dbus_error_name = "org.freedesktop.PolicyKit.Error.Failed"


def convert_dbus_exception(func):
    """A decorator which maps a raised DBbus exception to a native one.

    This decorator requires introspection to the decorated function. So it
    cannot be used on any already decorated method.
    """
    argnames, varargs, kwargs, defaults = inspect.getargspec(func)
    @wraps(func)
    def _convert_dbus_exception(*args, **kwargs):
        try:
            error_handler = kwargs["error_handler"]
        except KeyError:
            _args = list(args)
            try:
                index = argnames.index("error_handler")
                error_handler = _args[index]
            except (IndexError, ValueError):
                pass
            else:
                _args[index] = \
                        lambda err: error_handler(get_native_exception(err))
                args = tuple(_args)
        else:
            kwargs["error_handler"] = \
                    lambda err: error_handler(get_native_exception(err))
        try:
            return func(*args, **kwargs)
        except dbus.exceptions.DBusException, error:
            raise get_native_exception(error)
    return _convert_dbus_exception

def get_native_exception(error):
    """Map a DBus exception to a native one. This allows to make use of
    try/except on the client side without having to check for the error name.
    """
    if not isinstance(error, dbus.DBusException):
        return error
    dbus_name = error.get_dbus_name()
    dbus_msg = error.get_dbus_message()
    if dbus_name == TransactionFailed._dbus_error_name:
        return TransactionFailed(*dbus_msg.split(":", 1))
    elif dbus_name == AuthorizationFailed._dbus_error_name:
        return AuthorizationFailed(*dbus_msg.split(":", 1))
    elif dbus_name == NotAuthorizedError._dbus_error_name:
        return NotAuthorizedError(*dbus_msg.split(":", 1))
    for error_cls in [AptDaemonError, TransactionRoleAlreadySet,
                      TransactionAlreadyRunning, ForeignTransaction,
                      InvalidMetaDataError, InvalidProxyError,
                      TransactionCancelled, RepositoryInvalidError]:
        if dbus_name == error_cls._dbus_error_name:
            return error_cls(dbus_msg)
    return error

# vim:ts=4:sw=4:et
