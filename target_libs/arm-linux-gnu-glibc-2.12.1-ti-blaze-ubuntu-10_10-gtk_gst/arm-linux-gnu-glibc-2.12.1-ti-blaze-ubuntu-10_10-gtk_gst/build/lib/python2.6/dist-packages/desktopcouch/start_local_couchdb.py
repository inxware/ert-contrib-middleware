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
#
# Author: Stuart Langridge <stuart.langridge@canonical.com>
#         Eric Casteleijn <eric.casteleijn@canonical.com>

"""
Start local CouchDB server.
Steps:
    1. Work out which folders to use (running from source tree, or installed)
    2. Actually start CouchDB
    3. Check design documents in central folder against design documents in
       Couch, and overwrite any new ones
    4. Activate the pairing of ubuntu one by retrieving the oauth tokens using
       ubuntu sso.
    5. Write an HTML "bookmark" file which directs people to the local
       running CouchDB.

This script is normally called by advertisePort.py, which advertises the local
CouchDB port over D-Bus. That advertisePort script is started by D-Bus
activation.
"""

from __future__ import with_statement

import errno
import glob
import itertools
import logging
import os
import platform
import re 
import subprocess
import sys
import time

from gnomekeyring import NoKeyringDaemonError

import desktopcouch
from desktopcouch import local_files, read_pidfile, process_is_couchdb
from desktopcouch.records.server import CouchDatabase
from desktopcouch.pair.couchdb_pairing.ubuntuone_pairing import (
    pair_with_ubuntuone)
import xdg.BaseDirectory

def run_couchdb(ctx=local_files.DEFAULT_CONTEXT):
    """Actually start the CouchDB process.  Return its PID."""
    pid = read_pidfile(ctx)
    if pid is not None and not process_is_couchdb(pid):
        print "Removing stale, deceptive pid file."
        os.remove(ctx.file_pid)
    local_exec = ctx.couch_exec_command + ['-b']
    try:
        # subprocess is buggy.  Chad patched, but that takes time to propagate.
        proc = subprocess.Popen(local_exec)
        while True:
            try:
                retcode = proc.wait()
                break
            except OSError, e:
                if e.errno == errno.EINTR:
                    continue
                raise
        if retcode < 0:
            print >> sys.stderr, "Child was terminated by signal", -retcode
        elif retcode > 0:
            print >> sys.stderr, "Child returned", retcode
    except OSError, e:
        print >> sys.stderr, "Execution failed: %s: %s" % (e, local_exec)
        exit(1)

    # give the process a chance to start
    pid = None
    for timeout in (0.4, 0.1, 0.1, 0.2, 0.5, 1, 3, 5):
        pid = read_pidfile(ctx=ctx)
        if pid is not None and process_is_couchdb(pid):
            break
        time.sleep(timeout)

    if pid is None:
        raise RuntimeError("Can not start couchdb.")
        

    # Loop for a number of times until the port has been found, this
    # has to be done because there's a slice of time between PID being written
    # and the listening port being active.
    port = None
    for timeout in (0.1, 0.1, 0.2, 0.5, 1, 3, 5, 8):
        try:
            port = desktopcouch.find_port(pid=pid, ctx=ctx)
            # returns valid port, or raises exception
            break
        except RuntimeError, e:
            pass

        try:
            # Send no signal, merely test PID existence.
            os.kill(pid, 0)
        except OSError, e:
            raise RuntimeError("Couchdb PID%d exited.  Permissions?" % (pid,))

        time.sleep(timeout)

    if port is None:
        # Now we return valid port or raise exception.
        raise RuntimeError("Can not find port of couchdb.")

    ctx.sanitize_log_files()
    return pid, port

def update_design_documents(ctx=local_files.DEFAULT_CONTEXT):
    """Check system design documents and update any that need updating

    A database should be created if
    $XDG_DATA_DIRs/desktop-couch/databases/dbname/database.cfg exists
    
    Design docs are defined by the existence of
    $XDG_DATA_DIRs/desktop-couch/databases/dbname/_design/designdocname/views/viewname/map.js
    
    reduce.js may also exist in the same folder.
    """
    ctx_data_dir = os.path.split(ctx.db_dir)[0]
    for base in itertools.chain([ctx_data_dir], xdg.BaseDirectory.xdg_data_dirs):
        # FIXME: base may have magic chars.  assert not glob.has_magic(base) ?
        db_spec = os.path.join(
            base, "desktop-couch", "databases", "*", "database.cfg")
        for database_path in glob.glob(db_spec):
            database_root = os.path.split(database_path)[0]
            database_name = os.path.split(database_root)[1]
            # Just the presence of database.cfg is enough to create the database
            db = CouchDatabase(database_name, create=True, ctx=ctx)
            # look for design documents
            dd_spec = os.path.join(
                database_root, "_design", "*", "views", "*", "map.js")
            # FIXME: dd_path may have magic chars.
            for dd_path in glob.glob(dd_spec):
                view_root = os.path.split(dd_path)[0]
                view_name = os.path.split(view_root)[1]
                dd_root = os.path.split(os.path.split(view_root)[0])[0]
                dd_name = os.path.split(dd_root)[1]

                def load_js_file(filename_no_extension):
                    fn = os.path.join(
                        view_root, "%s.js" % (filename_no_extension))
                    if not os.path.isfile(fn): return None
                    fp = open(fn)
                    data = fp.read()
                    fp.close()
                    return data

                mapjs = load_js_file("map")
                reducejs = load_js_file("reduce")

                # XXX check whether this already exists or not, rather
                # than inefficiently just overwriting it regardless
                db.add_view(view_name, mapjs, reducejs, dd_name)

def update_bookmark_file(port, ctx=local_files.DEFAULT_CONTEXT):
    """Write out an HTML document that the user can bookmark to find their DB"""
    bookmark_file = os.path.join(ctx.db_dir, "couchdb.html")

    try:
        username, password = re.findall("<!-- !!([^!]+)!!([^!]+)!! -->", open(bookmark_file).read())[-1]
    except ValueError:
        raise IOError("Bookmark file is corrupt.  Username/password are missing.")

    if os.path.exists(
            os.path.join(os.path.split(__file__)[0], "../data/couchdb.tmpl")):
        bookmark_template = os.path.join(
            os.path.split(__file__)[0], "../data/couchdb.tmpl")
    else:
        for base in xdg.BaseDirectory.xdg_data_dirs:
            template_path = os.path.join(base, "desktopcouch", "couchdb.tmpl")
            if os.path.exists(template_path):
                bookmark_template = os.path.join(
                    os.path.split(__file__)[0], template_path)

    fp = open(bookmark_template)
    html = fp.read()
    fp.close()

    fp = open(bookmark_file, "w")
    try:
        if port is not None:
            out = html.replace("[[COUCHDB_PORT]]", str(port))
            out = out.replace("[[COUCHDB_USERNAME]]", username)
            out = out.replace("[[COUCHDB_PASSWORD]]", password)
            fp.write(out)
            print "Browse your desktop CouchDB at file://%s" % \
                    os.path.realpath(bookmark_file)
    finally:
        fp.close()


def update_pairing_service(sso=None, test_import_fails=False):
    """Update the pairing records between desktopcouch and Ubuntu One."""
    if not sso:
        try:
            # get the credentials using ubuntu sso dbus
            if test_import_fails:
                raise ImportError
            from ubuntu_sso.main import SSOCredentials
            sso = SSOCredentials()
        except ImportError:
            logging.info("Ubuntu SSO dbus service could not be used.")

    if sso: 
        try:
            credentials = sso.find_credentials('Ubuntu One')
        except NoKeyringDaemonError:
            logging.info(
                "NoKeyringDaemonError exception raised.")
            credentials = None
        if credentials and len(credentials) > 0:
            pair_with_ubuntuone()
 
def start_couchdb(ctx=local_files.DEFAULT_CONTEXT):
    """Execute each step to start a desktop CouchDB."""
    pid = None
    saved_exception = None
    for retry in range(10):
        try:
            pid, port = run_couchdb(ctx=ctx)
            break
        except RuntimeError, e:
            saved_exception = e
            logging.exception("Starting couchdb failed on try %d" % (retry,))
            time.sleep(1)
            continue

    if pid is None:
        raise saved_exception

    # Note that we do not call update_design_documents and 
    # update_pairing_service here. This is because
    # Couch won't actually have started yet, so when update_design_documents
    # calls the Records API, that will call back into get_port and we end up
    # starting Couch again. Instead, get_port calls update_design_documents
    # *after* Couch startup has occurred.
    update_bookmark_file(port, ctx=ctx)
    return pid


if __name__ == "__main__":
    start_couchdb()
    print "Desktop CouchDB started"

