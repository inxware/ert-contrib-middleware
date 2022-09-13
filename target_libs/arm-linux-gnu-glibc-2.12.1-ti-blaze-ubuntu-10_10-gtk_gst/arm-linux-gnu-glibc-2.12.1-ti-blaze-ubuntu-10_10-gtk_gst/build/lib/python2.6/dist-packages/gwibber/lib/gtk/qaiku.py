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
# Twitter widgets for Gwibber
#

import gtk
from gtk import Builder
import gwibber.microblog

class AccountWidget(gtk.VBox):
  """AccountWidget: A widget that provides a user interface for configuring qaiku accounts in Gwibber
  """
  
  def __init__(self, account=None, dialog=None):
    """Creates the account pane for configuring qaiku accounts"""
    gtk.VBox.__init__( self, False, 20 )
    self.ui = gtk.Builder()
    self.ui.set_translation_domain ("gwibber")
    self.ui.add_from_file (gwibber.resources.get_ui_asset("gwibber-accounts-qaiku.ui"))
    self.ui.connect_signals(self)
    self.vbox_settings = self.ui.get_object("vbox_settings")
    self.pack_start(self.vbox_settings, False, False)
    self.show_all()
    if dialog:
       dialog.get_object("vbox_create").show()
