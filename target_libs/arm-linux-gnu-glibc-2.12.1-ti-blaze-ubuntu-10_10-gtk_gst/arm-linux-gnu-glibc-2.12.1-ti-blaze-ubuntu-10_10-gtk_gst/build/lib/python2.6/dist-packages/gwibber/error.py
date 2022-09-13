#
# Copyright (C) 2010 Canonical Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Copyright (C) 2010 Ken VanDine <ken.vandine@canonical.com>
#
# Error dialog for Gwibber
#

import subprocess, os

import gettext
from gettext import lgettext as _
if hasattr(gettext, 'bind_textdomain_codeset'):
    gettext.bind_textdomain_codeset('gwibber','UTF-8')
gettext.textdomain('gwibber')

import gtk

class GwibberErrorService:

    def __init__(self):
        self.notified = {}

    def ShowDialog(self, message=None, title=None, condition="error", service=None, username=None, type=None):
        """show_dialog raises a gtk.MessageDialog to the user
           displaying errors or information.

           arguments are:
             message - a string to present to the user
             title - OPTIONAL: a string which will set the title of the window
             condition - a string, must be either "error" or "info"
             service
             username
             type - auth, network, keyring
        """

        if type == "keyring": service = "any"
        # Don't notify for the same error again
        if self.notified.has_key(service):
            if self.notified[service] == type:
                return

        if type == "keyring":
            if os.path.exists(os.path.join("bin", "gwibber-accounts")):
                cmd = os.path.join("bin", "gwibber-accounts")
            else:
                cmd = "gwibber-accounts"
            ret = subprocess.call([cmd])
            self.notified[service] = type
            return ret

        self.notified[service] = type
        if condition == "info":
            condition = gtk.MESSAGE_INFO
        else:
            condition = gtk.MESSAGE_ERROR

        if title is None:
            title = _("Gwibber Error")
        dialog = gtk.MessageDialog(
            parent = None,
            type = condition,
            buttons = gtk.BUTTONS_CLOSE,
            message_format = message)
        dialog.set_title(title)
        dialog.set_position(gtk.WIN_POS_CENTER_ALWAYS)
        dialog.run()
        dialog.destroy()
