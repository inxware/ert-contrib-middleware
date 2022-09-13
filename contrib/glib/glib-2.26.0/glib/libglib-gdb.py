import sys
import gdb

# Update module path.
dir = '/home/data/pdrezet/workspace/LUCID_MERGE_PATRICKS/INX/comp-lib-support/glib/glib-2.26.0/../target_lib_builds/linux_x86/share/glib-2.0/gdb'
if not dir in sys.path:
    sys.path.insert(0, dir)

from glib import register
register (gdb.current_objfile ())
