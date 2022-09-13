#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Handles the apt system lock"""
# Copyright (C) 2010 Sebastian Heinlein <devel@glatzor.de>
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

__all__ = ("LockFailedError", "lists_lock", "status_lock", "archive_lock",
           "acquire", "release", "FileLock")

import fcntl
import os
import struct

import apt_pkg


class LockFailedError(Exception):

    """The locking of file failed."""

    def __init__(self, flock, process=None):
        """Return a new LockFailedError instance.

        Keyword arguments:
        flock -- the path of the file lock
        process -- the process which holds the lock or None
        """
        Exception.__init__(self)
        self.flock = flock
        self.process = process


class FileLock(object):

    """Represents a file lock."""

    def __init__(self, path):
        self.path = path
        self.fd = None

    @property
    def locked(self):
        return self.fd is not None

    def acquire(self):
        """Return the file descriptor of the lock file or raise
        LockFailedError if the lock cannot be obtained.
        """
        fd_lock = apt_pkg.get_lock(self.path)
        if fd_lock < 0:
            process = get_locking_process_name(self.path)
            raise LockFailedError(self.path, process)
        else:
            self.fd = fd_lock
            return fd_lock

    def release(self):
        """Relase the lock."""
        if self.fd:
            os.close(self.fd)
            self.fd = None


def get_locking_process_name(lock_path):
    """Return the name of a process which holds a lock. It will be None if
    the name cannot be retrivied.
    """
    with open(lock_path,"r") as fd_lock_read:
        # Get the pid of the locking application
        flk = struct.pack('hhQQi', fcntl.F_WRLCK, os.SEEK_SET, 0, 0, 0)
        flk_ret = fcntl.fcntl(fd_lock_read, fcntl.F_GETLK, flk)
        pid = struct.unpack("hhQQi", flk_ret)[4]
        # Get the command of the pid
        with open("/proc/%s/status" % pid, "r") as fd_status:
            try:
                for key, value in (line.split(":") for line in \
                                   fd_status.readlines()):
                    if key == "Name":
                        return value.strip()
            except:
                return None
    return None

#: The lock for dpkg status file
_status_dir = os.path.dirname(apt_pkg.config.find_file("Dir::State::status"))
status_lock = FileLock(os.path.join(_status_dir, "lock"))

#: The lock for the package archive
_archives_dir = apt_pkg.config.find_dir("Dir::Cache::Archives")
archive_lock = FileLock(os.path.join(_archives_dir, "lock"))

#: The lock for the repository indexes
lists_lock = FileLock(os.path.join(apt_pkg.config.find_dir("Dir::State::lists"),
                                   "lock"))

def acquire():
    """Acquire an exclusive lock for the package management system."""
    try:
        for lock in archive_lock, status_lock, lists_lock:
            if not lock.locked:
                lock.acquire()
    except:
        release()
        raise

def release():
    """Release an exclusive lock for the package management system."""
    for lock in archive_lock, status_lock, lists_lock:
        lock.release()

# vim:ts=4:sw=4:et
