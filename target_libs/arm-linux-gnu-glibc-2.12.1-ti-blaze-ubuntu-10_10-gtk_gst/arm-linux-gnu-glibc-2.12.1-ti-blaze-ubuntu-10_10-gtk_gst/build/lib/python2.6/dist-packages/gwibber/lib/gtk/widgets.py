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
# widgets for Gwibber
#

from dbus.mainloop.glib import DBusGMainLoop
import gobject, gtk
from gwibber.microblog.util.const import *
# Try to import * from custom, install custom.py to include packaging 
# customizations like distro API keys, etc
try:
  from gwibber.microblog.util.custom import *
except:
  pass

import gwibber.gwui, gwibber.microblog.util, gwibber.resources
import gettext
from gettext import lgettext as _
if hasattr(gettext, 'bind_textdomain_codeset'):
    gettext.bind_textdomain_codeset('gwibber','UTF-8')
gettext.textdomain('gwibber')

class GwibberPosterVBox(gtk.VBox):
  def __init__(self):
    gtk.VBox.__init__(self)
    DBusGMainLoop(set_as_default=True)
    loop = gobject.MainLoop()
    self.service = gwibber.microblog.util.getbus("Service")
    self.stream_model = gwibber.gwui.Model()

    self.input = gwibber.gwui.Input()
    self.input.connect("submit", self.on_input_activate)
    self.input.connect("changed", self.on_input_changed)
    self.input_splitter = gtk.VPaned()
    self.input_splitter.add1(self.input)

    self.button_send = gtk.Button(_("Send"))
    self.button_send.connect("clicked", self.on_button_send_clicked)
    self.message_target = gwibber.gwui.AccountTargetBar(self.stream_model)
    self.message_target.pack_end(self.button_send, False)

    content = gtk.VBox(spacing=5)
    content.pack_start(self.input_splitter, True)
    content.pack_start(self.message_target, False)
    content.set_border_width(5)

    layout = gtk.VBox()
    layout.pack_start(content, True)
    self.add(layout)

  def on_input_changed(self, w, text, cnt):
    self.input.textview.set_overlay_text(str(MAX_MESSAGE_LENGTH - cnt))

  def on_input_activate(self, w, text, cnt):
    self.service.SendMessage(text)
    w.clear()

  def on_button_send_clicked(self, w):
    self.service.SendMessage(self.input.get_text())
    self.input.clear()

