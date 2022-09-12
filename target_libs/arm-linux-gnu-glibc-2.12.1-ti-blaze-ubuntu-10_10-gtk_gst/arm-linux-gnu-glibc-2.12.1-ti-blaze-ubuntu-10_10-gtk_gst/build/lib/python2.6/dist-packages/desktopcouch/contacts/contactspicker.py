# Copyright 2009 Canonical Ltd.
#
# This file is part of desktopcouch-contacts.
#
# desktopcouch is free software: you can redistribute it and/or modify
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
# Authors: Rodrigo Moya <rodrigo.moya@canonical.com

"""A widget to allow users to pick contacts"""

import gtk
import desktopcouch
from desktopcouch.contacts.record import CONTACT_RECORD_TYPE
from desktopcouch.records.couchgrid import CouchGrid


class ContactsPicker(gtk.VBox):
    """A contacts picker"""

    def __init__(self, uri=None, ctx=desktopcouch.local_files.DEFAULT_CONTEXT):
        """Create a new ContactsPicker widget."""

        gtk.VBox.__init__(self)

        # Create search entry and button
        hbox = gtk.HBox()
        self.pack_start(hbox, False, False, 3)

        self.search_entry = gtk.Entry()
        hbox.pack_start(self.search_entry, True, True, 3)

        self.search_button = gtk.Button(
            stock=gtk.STOCK_FIND, use_underline=True)
        hbox.pack_start(self.search_button, False, False, 3)

        # Create CouchGrid to contain list of contacts
        self.contacts_list = CouchGrid('contacts', uri=uri, ctx=ctx)
        self.contacts_list.editable = False
        self.contacts_list.keys = [ "first_name", "last_name" ]
        self.contacts_list.record_type = CONTACT_RECORD_TYPE

        #Pimp out the columns with some nicer titles
        #TODO: this should be set up for translatability
        columns = self.contacts_list.get_columns()
        columns[0].set_title("First Name")
        columns[1].set_title("Last Name")

        self.contacts_list.show()
        self.pack_start(self.contacts_list, True, True, 3)

    def get_contacts_list(self):
        """get_contacts_list - gtk.Widget value
        Gets the CouchGrid inside the ContactsPicker widget
        """
        return self.contacts_list

if __name__ == "__main__":
    """creates a test ContactsPicker if called directly"""

    # Create and show a test window
    win = gtk.Dialog("ContactsPicker widget test", None, 0,
                     (gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))
    win.connect("response", gtk.main_quit)
    win.resize(300, 450)

    # Create the contacts picker widget
    picker = ContactsPicker()
    win.get_content_area().pack_start(picker, True, True, 3)

    # Run the test application
    win.show_all()
    gtk.main()
