import gtk, webkit, gnomekeyring
import urllib, urllib2, json, urlparse, uuid
from oauth import oauth

from gtk import Builder
import gwibber.microblog
from gwibber.microblog import twitter
from gwibber.microblog.util import resources
import gettext
from gettext import gettext as _
if hasattr(gettext, 'bind_textdomain_codeset'):
    gettext.bind_textdomain_codeset('gwibber','UTF-8')
gettext.textdomain('gwibber')

gtk.gdk.threads_init()

sigmeth = oauth.OAuthSignatureMethod_HMAC_SHA1()

class AccountWidget(gtk.VBox):
  """AccountWidget: A widget that provides a user interface for configuring twitter accounts in Gwibber
  """
  
  def __init__(self, account=None, dialog=None):
    """Creates the account pane for configuring Twitter accounts"""
    gtk.VBox.__init__( self, False, 20 )
    self.ui = gtk.Builder()
    self.ui.set_translation_domain ("gwibber")
    self.ui.add_from_file (gwibber.resources.get_ui_asset("gwibber-accounts-twitter.ui"))
    self.ui.connect_signals(self)
    self.vbox_settings = self.ui.get_object("vbox_settings")
    self.pack_start(self.vbox_settings, False, False)
    self.show_all()

    self.account = account or {}
    self.dialog = dialog
    has_secret_key = True
    if self.account.has_key("id"):
      try:
        value = gnomekeyring.find_items_sync(gnomekeyring.ITEM_GENERIC_SECRET, {"id": str("%s/%s" % (self.account["id"], "secret_token"))})[0].secret
      except gnomekeyring.NoMatchError:
        has_secret_key = False

    try:
      if self.account.has_key("access_token") and self.account.has_key("secret_token") and self.account.has_key("username") and has_secret_key:
        self.ui.get_object("hbox_twitter_auth").hide()
        self.ui.get_object("fb_auth_done_label").set_label(_("%s has been authorized by Twitter") % self.account["username"])
        self.ui.get_object("hbox_twitter_auth_done").show()
      else:
        self.ui.get_object("hbox_twitter_auth_done").hide()
        self.ui.get_object("twitter_auth_button").modify_bg(gtk.STATE_NORMAL, gtk.gdk.color_parse("red"))
        if self.dialog:
          self.dialog.get_object('vbox_create').hide()
    except:
      self.ui.get_object("hbox_twitter_auth_done").hide()
      if self.dialog:
        self.dialog.get_object("vbox_create").hide()


  def on_twitter_auth_clicked(self, widget, data=None):
    self.winsize = self.window.get_size()

    web = webkit.WebView()
    web.get_settings().set_property("enable-plugins", False)
    web.load_html_string(_("<p>Please wait...</p>"), "file:///")

    self.consumer = oauth.OAuthConsumer(*resources.get_twitter_keys())

    request = oauth.OAuthRequest.from_consumer_and_token(self.consumer, http_method="POST",
        callback="http://gwibber.com/0/auth.html",
        http_url="https://api.twitter.com/oauth/request_token")

    request.sign_request(sigmeth, self.consumer, token=None)

    tokendata = urllib2.urlopen(request.http_url, request.to_postdata()).read()
    self.token = oauth.OAuthToken.from_string(tokendata)

    url = "http://api.twitter.com/oauth/authorize?oauth_token=" + self.token.key

    web.open(url)
    web.set_size_request(550, 400)
    web.connect("title-changed", self.on_twitter_auth_title_change)

    scroll = gtk.ScrolledWindow()
    scroll.add(web)

    self.pack_start(scroll, True, True, 0)
    self.show_all()

    self.ui.get_object("vbox1").hide()
    self.ui.get_object("expander1").hide()

  def on_twitter_auth_title_change(self, web=None, title=None, data=None):
    if title.get_title() == "Success":
      try:
        url = web.get_main_frame().get_uri()
        data = urlparse.parse_qs(url.split("?", 1)[1])
        
        token = data["oauth_token"][0]
        verifier = data["oauth_verifier"][0]

        request = oauth.OAuthRequest.from_consumer_and_token(
          self.consumer, self.token,
          http_url="https://api.twitter.com/oauth/access_token",
          parameters={"oauth_verifier": str(verifier)})
        request.sign_request(sigmeth, self.consumer, self.token)

        tokendata = urllib2.urlopen(request.http_url, request.to_postdata()).read()
        data = urlparse.parse_qs(tokendata)

        self.account["access_token"] = data["oauth_token"][0]
        self.account["secret_token"] = data["oauth_token_secret"][0]
        self.account["username"] = data["screen_name"][0]
        self.account["user_id"] = data["user_id"][0]

        self.ui.get_object("hbox_twitter_auth").hide()
        self.ui.get_object("fb_auth_done_label").set_label(_("%s has been authorized by Twitter") % str(self.account["username"]))
        self.ui.get_object("hbox_twitter_auth_done").show()
        if self.dialog and self.account.has_key("id"):
          self.dialog.get_object("vbox_save").show()
        elif self.dialog:
          self.dialog.get_object("vbox_create").show()
      except:
        pass

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
