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
# facebook widgets for Gwibber
#

import gtk
import urllib
import webkit
import string

from gtk import Builder
import gwibber.microblog
from gwibber.microblog import facebook
from gwibber.microblog.util import facelib
from gwibber.microblog.util.const import *
# Try to import * from custom, install custom.py to include packaging 
# customizations like distro API keys, etc
try:
  from gwibber.microblog.util.custom import *
except:
  pass

import json, urlparse, gnomekeyring, uuid
import gettext
from gettext import gettext as _
if hasattr(gettext, 'bind_textdomain_codeset'):
    gettext.bind_textdomain_codeset('gwibber','UTF-8')
gettext.textdomain('gwibber')

"""
gtk.gdk.threads_init()

APP_KEY = "71b85c6d8cb5bbb9f1a3f8bbdcdd4b05"
"""

class AccountWidget(gtk.VBox):
  """AccountWidget: A widget that provides a user interface for configuring facebook accounts in Gwibber
  """
  
  def __init__(self, account=None, dialog=None):
    """Creates the account pane for configuring facebook accounts"""
    gtk.VBox.__init__( self, False, 20 )
    self.ui = gtk.Builder()
    self.ui.set_translation_domain ("gwibber")
    self.ui.add_from_file (gwibber.resources.get_ui_asset("gwibber-accounts-facebook.ui"))
    self.ui.connect_signals(self)
    self.vbox_settings = self.ui.get_object("vbox_settings")
    self.pack_start(self.vbox_settings, False, False)
    self.show_all()
    if account:
      self.account = account
    else:
      self.account = {}
    self.dialog = dialog
    has_secret_key = True
    if self.account.has_key("id"):
      try:
        value = gnomekeyring.find_items_sync(gnomekeyring.ITEM_GENERIC_SECRET, {"id": str("%s/%s" % (self.account["id"], "secret_key"))})[0].secret
      except gnomekeyring.NoMatchError:
        has_secret_key = False
    try:
      if self.account["session_key"] and self.account["secret_key"] and self.account["username"] and has_secret_key:
        self.ui.get_object("hbox_facebook_auth").hide()
        self.ui.get_object("fb_auth_done_label").set_label(_("%s has been authorized by Facebook") % str(self.account["username"]))
        self.ui.get_object("hbox_facebook_auth_done").show()
      else:
        self.ui.get_object("hbox_facebook_auth_done").hide()
        self.ui.get_object("facebook_auth_button").modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("red"))
        if self.dialog:
          self.dialog.get_object('vbox_create').hide()
    except:
      self.ui.get_object("hbox_facebook_auth_done").hide()
      if self.dialog:
        self.dialog.get_object("vbox_create").hide()

  def on_facebook_auth_clicked(self, widget, data=None):
    (self.win_w, self.win_h) = self.window.get_size()

    web = webkit.WebView()
    web.get_settings().set_property("enable-plugins", False)
    web.load_html_string(_("<p>Please wait...</p>"), "file:///")

    url = urllib.urlencode({
      "api_key": FB_APP_KEY,
      "connect_display": "popup",
      "v": "1.0",
      "next": "http://www.facebook.com/connect/login_success.html",
      "cancel_url": "http://www.facebook.com/connect/login_failure.html",
      "fbconnect": "true",
      "return_session": "true",
      "req_perms": "publish_stream,read_stream,status_update,offline_access,user_photos,friends_photos",
    })
    web.set_size_request(450, 340)
    web.open("http://www.facebook.com/login.php?" + url)
    web.connect("title-changed", self.on_facebook_auth_title_change)

    scroll = gtk.ScrolledWindow()
    scroll.add(web)

    self.pack_start(scroll, True, True, 0)
    self.show_all()
    self.ui.get_object("vbox1").hide()
    self.ui.get_object("expander1").hide()

  def on_facebook_auth_title_change(self, web=None, title=None, data=None):
    if title.get_title() == "Success":
      try:
        url = web.get_main_frame().get_uri()
        data = json.loads(urlparse.parse_qs(url.split("?", 1)[1])["session"][0])
        self.account["session_key"] = str(data["session_key"])

        fbuid = self.account["session_key"].split("-")[1]
        fbc = facelib.Facebook(FB_APP_KEY, "")
        fbc.session_key = self.account["session_key"]
        fbc.secret_key = str(data["secret"])
        self.account["username"] = str(fbc.users.getInfo(fbuid)[0]["name"])
        self.account["secret_key"] = str(data["secret"])
        
        if not self.account.has_key("username"):
            self.account["username"] = str(fbc.users.getInfo(fbuid)[0]["uid"])

        """
        if "_id" not in self.account:
          valid = string.ascii_letters + string.digits + "-"
          aId = "facebook-%s" % self.account["username"]
          self.account["_id"] = "".join((x for x in aId if x in valid)).lower()
        
        self.account["secret_key"] = ":KEYRING:%s" % \
            gnomekeyring.item_create_sync(
              gnomekeyring.get_default_keyring_sync(),
                gnomekeyring.ITEM_GENERIC_SECRET,
                "Gwibber pref: %s/%s" % (self.account["_id"], "secret_key"),
                {"id": str("%s/%s" % (self.account["_id"], "secret_key"))},
                str(data["secret"]), True)
        """

        self.ui.get_object("hbox_facebook_auth").hide()
        self.ui.get_object("fb_auth_done_label").set_label(_("%s has been authorized by Facebook") % str(self.account["username"]))
        self.ui.get_object("hbox_facebook_auth_done").show()
        if self.dialog and self.account.has_key("id"):
          self.dialog.get_object("vbox_save").show()
        elif self.dialog:
          self.dialog.get_object("vbox_create").show()
      except:
        #FIXME: We should do this in the same window
        pass
      web.hide()
      self.window.resize(self.win_w, self.win_h)
      self.ui.get_object("vbox1").show()
      self.ui.get_object("expander1").show()

    if title.get_title() == "Failure":
      gtk.gdk.threads_enter()
      d = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_ERROR,
        gtk.BUTTONS_OK, _("Facebook authorization failed. Please try again."))
      if d.run(): d.destroy()
      gtk.gdk.threads_leave()

      web.hide()
      self.window.resize(self.win_w, self.win_h)
