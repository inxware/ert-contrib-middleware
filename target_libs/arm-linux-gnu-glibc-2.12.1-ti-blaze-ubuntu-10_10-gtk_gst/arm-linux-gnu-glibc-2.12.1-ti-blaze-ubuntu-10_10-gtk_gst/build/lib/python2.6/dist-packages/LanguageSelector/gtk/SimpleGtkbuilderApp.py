"""
 SimpleGladeApp.py
 Module that provides an object oriented abstraction to pygtk and libglade.
 Copyright (C) 2004 Sandino Flores Moreno
"""

# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

import os
import sys
import re

import gtk
import gtk.glade

# based on SimpleGladeApp
class SimpleGtkbuilderApp:

    def __init__(self, path):
        # Temporary workaround for https://bugzilla.gnome.org/show_bug.cgi?id=574520
        gtk.glade.bindtextdomain("language-selector", "/usr/share/locale")
        gtk.glade.textdomain("language-selector")
        self.builder = gtk.Builder()
        self.builder.add_from_file(path)
        # Temporary workaround for https://bugzilla.gnome.org/show_bug.cgi?id=574520
        self.builder.set_translation_domain('language-selector')
        self.builder.connect_signals(self)
        for o in self.builder.get_objects():
            if issubclass(type(o), gtk.Buildable):
                name = gtk.Buildable.get_name(o)
                setattr(self, name, o)
            else:
                # see LP: #402780 - sometimes this dies with broken pipe?
                try:
                    print >>sys.stderr, "WARNING: can not get name for '%s'" % o
                except:
                    pass

    def run(self):
        """
        Starts the main loop of processing events checking for Control-C.

        The default implementation checks wheter a Control-C is pressed,
        then calls on_keyboard_interrupt().

        Use this method for starting programs.
        """
        try:
            gtk.main()
        except KeyboardInterrupt:
            self.on_keyboard_interrupt()

    def on_keyboard_interrupt(self):
        """
        This method is called by the default implementation of run()
        after a program is finished by pressing Control-C.
        """
        pass


