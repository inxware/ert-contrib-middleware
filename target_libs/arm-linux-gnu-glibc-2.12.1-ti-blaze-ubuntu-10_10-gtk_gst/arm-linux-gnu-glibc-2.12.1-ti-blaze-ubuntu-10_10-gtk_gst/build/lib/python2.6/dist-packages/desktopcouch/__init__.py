# Copyright 2009 Canonical Ltd.
#
# This file is part of desktopcouch.
#
#  desktopcouch is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# as published by the Free Software Foundation.
#
# desktopcouch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with desktopcouch.  If not, see <http://www.gnu.org/licenses/>.
"Desktop Couch helper files"

from __future__ import with_statement
import os
import re
import xdg.BaseDirectory
import dbus
import logging
import platform
import os
from desktopcouch import local_files


def process_is_couchdb_linux(pid):
    """Find if the process with the given pid is couchdb."""
    if pid is None:
        return False
    pid = int(pid)  # ensure it's a number
    proc_dir = "/proc/%d" % (pid,)
    try:
        # check to make sure it is actually a desktop-couch instance
        with open(os.path.join(proc_dir, 'cmdline')) as cmd_file:
            cmd = cmd_file.read()
        if 'beam' not in cmd:
            return False

        # make sure it's our process.
        if not os.access(os.path.join(proc_dir, "mem"), os.W_OK):
            return False

    except IOError:
        return False

    return True

os_name = platform.system()
try:
    process_is_couchdb = {
        "Linux": process_is_couchdb_linux
        } [os_name]
except KeyError:
    raise NotImplementedError("os %r is not yet supported" % (os_name,))

def read_pidfile(ctx=local_files.DEFAULT_CONTEXT):
    """Read the pid file for the required information."""
    try:
        pid_file = ctx.file_pid
        if not os.path.exists(pid_file):
            return None
        with open(pid_file) as fp:
            try:
                contents = fp.read()
                if contents == "\n":
                    return None  # not yet written to pid file
                return int(contents)
            except ValueError:
                logging.warn("Pid file does not contain int: %r", contents)
                return None
    except IOError, e:
        logging.warn("Reading pid file caused error.  %s", e)
        return None


def find_pid(start_if_not_running=True, ctx=local_files.DEFAULT_CONTEXT):
    """Find the current OS process ID of the running couchdb.  API users
    should not use this, and instead go straight to find_port() ."""
    # Work out whether CouchDB is running by looking at its pid file
    pid = read_pidfile(ctx=ctx)
    if not process_is_couchdb(pid):
        if start_if_not_running:
            # start CouchDB by running the startup script
            logging.info("Desktop CouchDB is not running; starting it.")
            from desktopcouch import start_local_couchdb
            pid = start_local_couchdb.start_couchdb(ctx=ctx)
            # now load the design documents and pair records updates,
            # because it's started
            start_local_couchdb.update_design_documents()
            start_local_couchdb.update_pairing_service()
            if not process_is_couchdb(pid):
                logging.error("CouchDB process did not start up")
                raise RuntimeError("desktop-couch not started")
        else:
            return None

    return pid

def find_port(pid=None, ctx=local_files.DEFAULT_CONTEXT):
    """Ask the service daemon through DBUS what the port is.  This should start
    it up if it isn't running."""

    return _direct_access_find_port(pid=pid, ctx=ctx)
####
####
####
####  This fails in multithreaded execution in the client, apparenly
####  in DBus.  Chad's guess is that there is some collision in getting
####  a unique sequence ID for the asynchronous DBus method-call/
####  method-response.  Once that is worked out, resume using the
####  rest of this function instead of the direct access above.
    # Hrm, we don't use 'pid' or 'ctx' any more, since we go through DBus.
    if ctx != local_files.DEFAULT_CONTEXT or pid is not None:
        return _direct_access_find_port(pid=pid, ctx=ctx)

    bus = dbus.SessionBus()
    proxy = bus.get_object('org.desktopcouch.CouchDB', '/')
    return proxy.getPort()
####
####  ^^


def __find_port__linux(pid=None, ctx=local_files.DEFAULT_CONTEXT,
        retries_left=3):
    """This returns a valid port or raises a RuntimeError exception.  It never
    returns anything else."""
    if pid is None:
        pid = find_pid(start_if_not_running=True, ctx=ctx)

    if pid is None:
        if retries_left:
            return __find_port__linux(pid, ctx, retries_left - 1)
        raise RuntimeError("Have no PID to use to look up port.")

    proc_dir = "/proc/%d" % (pid,)

    # enumerate the process' file descriptors
    fd_dir = os.path.join(proc_dir, 'fd')
    fd_paths = list()
    try:
        for dirent in os.listdir(fd_dir):
            try:
                dirent_path = os.path.join(fd_dir, dirent)
                fd_paths.append(os.readlink(dirent_path))
            except OSError:
                logging.debug("dirent %r disappeared before " +
                    "we could read it. ", dirent_path)
                continue
    except OSError:
        if retries_left:
            return __find_port__linux(pid, ctx, retries_left - 1)
        logging.exception("Unable to find file descriptors in %s" % proc_dir)
        raise RuntimeError("Unable to find file descriptors in %s" % proc_dir)

    # identify socket fds
    socket_matches = [re.match('socket:\\[([0-9]+)\\]', p) for p in fd_paths]
    # extract their inode numbers
    socket_inodes = [m.group(1) for m in socket_matches if m is not None]

    # construct a subexpression which matches any one of these inodes
    inode_subexp = "|".join(map(re.escape, socket_inodes))
    # construct regexp to match /proc/net/tcp entries which are listening
    # sockets having one of the given inode numbers
    listening_regexp = re.compile(r'''
        \s*\d+:\s*                # sl
        [0-9A-F]{8}:              # local_address part 1
        ([0-9A-F]{4})\s+          # local_address part 2
        00000000:0000\s+          # rem_address
        0A\s+                     # st (0A = listening)
        [0-9A-F]{8}:              # tx_queue
        [0-9A-F]{8}\s+            # rx_queue
        [0-9A-F]{2}:              # tr
        [0-9A-F]{8}\s+            # tm->when
        [0-9A-F]{8}\s*            # retrnsmt
        \d+\s+\d+\s+              # uid, timeout
        (?:%s)\s+                 # inode
        ''' % (inode_subexp,), re.VERBOSE)

    # extract the TCP port from the first matching line in /proc/$pid/net/tcp
    port = None
    with open(os.path.join(proc_dir, 'net', 'tcp')) as tcp_file:
        for line in tcp_file:
            match = listening_regexp.match(line)
            if match is not None:
                port = str(int(match.group(1), 16))
                break

    if port is None:
        if retries_left:
            return __find_port__linux(pid, ctx, retries_left - 1)
        raise RuntimeError("Unable to find listening port")

    return port

import platform
os_name = platform.system()
try:
    _direct_access_find_port = {
        "Linux": __find_port__linux
        } [os_name]
except KeyError:
    logging.error("os %r is not yet supported" % (os_name,))
    raise NotImplementedError("os %r is not yet supported" % (os_name,))
