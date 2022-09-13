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
# Author: Chad Miller <chad.miller@canonical.com>
#         Stuart Langridge <stuart.langridge@canonical.com>

"""
Stop local CouchDB server.
"""

import subprocess, sys
from desktopcouch import local_files

def stop_couchdb(ctx=local_files.DEFAULT_CONTEXT):
    local_exec = ctx.couch_exec_command + ["-d"]
    try:
        retcode = subprocess.call(local_exec, shell=False)
        if retcode < 0:
            print >> sys.stderr, "Child was terminated by signal", -retcode
        elif retcode > 0:
            print >> sys.stderr, "Child returned", retcode
    except OSError, e:
        print >> sys.stderr, "Execution failed: %s: %s" % (e, local_exec)
        exit(1)


if __name__ == "__main__":
    stop_couchdb()
