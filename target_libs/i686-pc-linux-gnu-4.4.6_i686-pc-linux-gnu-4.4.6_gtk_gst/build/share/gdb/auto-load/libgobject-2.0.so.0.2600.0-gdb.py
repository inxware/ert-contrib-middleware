import sys
import gdb

# Update module path.
dir = '/home/pdrezet/workspace/INX/comp-lib-support/inx_build_scripts/placeholder/placeholder/../../../target_libs/i686-linux-gnu-glibc-2.11.3-raymarine/build/share/glib-2.0/gdb'
if not dir in sys.path:
    sys.path.insert(0, dir)

from gobject import register
register (gdb.current_objfile ())
