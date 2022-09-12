import gtk, webkit, gnomekeyring
import urllib, urllib2, json, urlparse, uuid
from oauth import oauth

from gtk import Builder
import gwibber.microblog
from gwibber.microblog import buzz
import gettext
from gettext import gettext as _
if hasattr(gettext, 'bind_textdomain_codeset'):
    gettext.bind_textdomain_codeset('gwibber','UTF-8')
gettext.textdomain('gwibber')

gtk.gdk.threads_init()

sigmeth = oauth.OAuthSignatureMethod_HMAC_SHA1()

class AccountWidget(gtk.VBox):
  """AccountWidget: A widget that provides a user interface for configuring facebook accounts in Gwibber
  """
  
  def __init__(self, account=None, dialog=None):
    """Creates the account pane for configuring Buzz accounts"""
    gtk.VBox.__init__( self, False, 20 )
    self.ui = gtk.Builder()
    self.ui.set_translation_domain ("gwibber")
    self.ui.add_from_file (gwibber.resources.get_ui_asset("gwibber-accounts-buzz.ui"))
    self.ui.connect_signals(self)
    self.vbox_settings = self.ui.get_object("vbox_settings")
    self.pack_start(self.vbox_settings, False, False)
    self.show_all()

    self.account = account or {}
    self.dialog = dialog

    if self.account.get("access_token", 0) and self.account.get("username", 0):
      self.ui.get_object("hbox_buzz_auth").hide()
      self.ui.get_object("fb_auth_done_label").set_label(_("%s has been authorized by Buzz") % self.account["username"])
      self.ui.get_object("hbox_buzz_auth_done").show()
    else:
      self.ui.get_object("hbox_buzz_auth_done").hide()
      if self.dialog:
        self.dialog.get_object('vbox_create').hide()

  def on_buzz_auth_clicked(self, widget, data=None):
    self.winsize = self.window.get_size()

    web = webkit.WebView()
    web.get_settings().set_property("enable-plugins", False)
    web.load_html_string(_("<p>Please wait...</p>"), "file:///")

    self.consumer = oauth.OAuthConsumer("anonymous", "anonymous")

    params = {
      "oauth_consumer_key": self.consumer.key,
      "oauth_timestamp": oauth.generate_timestamp(),
      "oauth_nonce": oauth.generate_nonce(),
      "oauth_version": oauth.OAuthRequest.version,
      "oauth_callback": "http://gwibber.com/0/auth.html",
      "scope": "https://www.googleapis.com/auth/buzz",
    }

    request = oauth.OAuthRequest("POST", "https://www.google.com/accounts/OAuthGetRequestToken", params)
    request.sign_request(sigmeth, self.consumer, token=None)

    tokendata = urllib2.urlopen(request.http_url, request.to_postdata()).read()
    self.token = oauth.OAuthToken.from_string(tokendata)

    url = "https://www.google.com/accounts/OAuthAuthorizeToken?oauth_token=" + self.token.key

    web.open(url)
    web.set_size_request(450, 340)
    web.connect("title-changed", self.on_buzz_auth_title_change)

    scroll = gtk.ScrolledWindow()
    scroll.add(web)

    self.pack_start(scroll, True, True, 0)
    self.show_all()

    self.ui.get_object("vbox1").hide()
    self.ui.get_object("expander1").hide()

  def on_buzz_auth_title_change(self, web=None, title=None, data=None):
    if title.get_title() == "Success":
      url = web.get_main_frame().get_uri()
      data = urlparse.parse_qs(url.split("?", 1)[1])
      verifier = data["oauth_verifier"][0]
      self.token.set_verifier(verifier)

      request = oauth.OAuthRequest.from_consumer_and_token(self.consumer, self.token, verifier=verifier,
          http_url="https://www.google.com/accounts/OAuthGetAccessToken")
      request.sign_request(sigmeth, self.consumer, self.token)

      tokendata = urllib2.urlopen(request.to_url()).read()
      data = urlparse.parse_qs(tokendata)

      self.account["access_token"] = data["oauth_token"][0]
      self.account["secret_token"] = data["oauth_token_secret"][0]

      token = oauth.OAuthToken(self.account["access_token"], self.account["secret_token"])

      request = oauth.OAuthRequest.from_consumer_and_token(self.consumer, token,
          http_url="https://www.googleapis.com/buzz/v1/people/@me/@self", parameters={"alt": "json"})
      request.sign_request(sigmeth, self.consumer, token)

      data = json.loads(urllib2.urlopen(request.to_url()).read())["data"]
      self.account["username"] = data["displayName"]
      self.account["user_id"] = data["id"]

      self.ui.get_object("hbox_buzz_auth").hide()
      self.ui.get_object("fb_auth_done_label").set_label(_("%s has been authorized by Buzz") % str(self.account["username"]))
      self.ui.get_object("hbox_buzz_auth_done").show()
      if self.dialog:
        self.dialog.get_object("vbox_create").show()

      web.hide()
      self.window.resize(*self.winsize)
      self.ui.get_object("vbox1").show()
      self.ui.get_object("expander1").show()

    if title.get_title() == "Failure":
      d = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_ERROR,
        gtk.BUTTONS_OK, _("Authorization failed. Please try again."))
      if d.run(): d.destroy()

      web.hide()
      self.window.resize(*self.winsize)
