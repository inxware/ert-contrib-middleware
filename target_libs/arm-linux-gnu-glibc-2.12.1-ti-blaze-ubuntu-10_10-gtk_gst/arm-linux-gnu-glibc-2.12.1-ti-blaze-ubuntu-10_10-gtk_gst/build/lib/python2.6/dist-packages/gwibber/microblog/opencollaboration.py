"""

Open Collaboration Services module for Gwibber
SegPhault (Ryan Paul) - 06/17/2009

"""

from . import can, support
import urllib2, urllib, base64, json
from gettext import lgettext as _

PROTOCOL_INFO = {
  "name": "OCS",
  "version": 0.1,
  
  "config": [
    "private:password",
    "username",
    "domain",
    "message_color",
    "receive_enabled",
    "send_enabled",
  ],

  "features": [
    "send",
    "receive",
  ],
}

class Message:
  def __init__(self, client, data):
    self.id = str(data["id"])
    self.client = client
    self.account = client.account
    self.service = client.account["service"]
    self.username = client.account["username"]

    self.sender = "%s %s" % (data["firstname"], data["lastname"])
    self.sender_nick = data["personid"]
    self.sender_id = data["personid"]
    self.time = support.parse_time(data["timestamp"])
    self.image = data["avatarpic"]
    self.bgcolor = "message_color"
    self.url = data["link"]
    self.text = data["message"]
    self.profile_url = client.url(data["profilepage"])
    self.is_reply = False

class SearchResult:
  def __init__(self, client, data):
    self.client = client
    self.account = client.account
    self.service = client.account["service"]
    self.username = client.account["username"]

    """
    self.id = data.id
    self.sender = data.personid.string #"%s %s" % (data.firstname.string, data.lastname.string)
    self.sender_nick = data.personid.string
    self.sender_id = data.personid.string
    self.time = support.parse_time(data.changed.string)
    self.image = "http://www.opendesktop.org/usermanager/nopic.png"
    self.bgcolor = "message_color"
    self.url = data.detailpage.string
    self.text = data.find("name").string
    self.profile_url = "http://opendesktop.org"
    self.is_reply = False

    self.thumbnails = [{
      "src": data.previewpic1.string,
      "href": self.url,
    }]
    """
    
    self.id = data["id"]
    self.sender = data["personid"] #"%s %s" % (data["firstname"], data["lastname"])
    self.sender_nick = data["personid"]
    self.sender_id = data["personid"]
    self.time = support.parse_time(data["changed"])
    self.image = "http://www.opendesktop.org/usermanager/nopic.png"
    self.bgcolor = "message_color"
    self.url = data["detailpage"]
    self.text = data["name"]
    self.profile_url = "http://opendesktop.org"
    self.is_reply = False

    self.thumbnails = [{
      "src": data["previewpic1"],
      "url": self.url,
    }]

class Client:
  def __init__(self, acct):
    self.account = acct

  def send_enabled(self):
    return self.account["send_enabled"] and \
      self.account["username"] != None and \
      self.account["private:password"] != None

  def receive_enabled(self):
    return self.account["receive_enabled"] and \
      self.account["username"] != None and \
      self.account["private:password"] != None

  def url(self, path):
    d = self.account["domain"]
    if d.startswith("http://") or d.startswith("https://"): return d + path
    if self.account["allow_insecure"]: return "http://" + d + path
    return "https://" + d + path

  def get_auth(self):
    return "Basic %s" % base64.encodestring(
      ("%s:%s" % (self.account["username"], self.account["private:password"]))).strip()

  def connect(self, url, data = None):
    return urllib2.urlopen(urllib2.Request(
      self.url(url), data, headers = {"Authorization": self.get_auth()}))

  def get_messages(self):
    return json.load(self.connect("/v1/activity?" +
      urllib.urlencode({"format": "json"})))["data"]

  def get_search_data(self, query):
    return json.load(self.connect("/v1/content/data?" +
      urllib.urlencode({
        "format": "json",
        "search": query,
        "sortmodes": "high",
        "categories": "120x121x101x100x170x171x172x173x174x175x176x177x178x179",
      })))["data"]

  def receive(self):
    for data in self.get_messages():
      yield Message(self, data)

  def search(self, query):
    for data in self.get_search_data(query):
      yield SearchResult(self, data)

  def send(self, message):
    print self.connect("/v1/activity", urllib.urlencode({"message": message}))
