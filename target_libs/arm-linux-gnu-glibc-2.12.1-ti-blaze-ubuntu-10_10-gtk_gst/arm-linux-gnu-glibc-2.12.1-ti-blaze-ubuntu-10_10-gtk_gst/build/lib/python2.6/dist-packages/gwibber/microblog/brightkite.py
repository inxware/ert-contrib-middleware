"""

BrightKite interface for Gwibber
SegPhault (Ryan Paul) - 10/19/2008

"""

from . import support
import urllib2, urllib, base64, re, simplejson
from xml.dom import minidom

PROTOCOL_INFO = {
  "name": "BrightKite",
  "version": 0.2,
  
  "config": [
    "password",
    "username",
    "message_color",
    "receive_enabled",
    "send_enabled"
  ],

  "features": [
    "receive",
    "responses",
    "thread",
  ],
}

NICK_PARSE = re.compile("@([A-Za-z0-9]+)")

class Message:
  def __init__(self, client, data):
    self.client = client
    self.account = client.account
    self.service = client.account["service"]
    self.username = client.account["username"]

    self.sender = data["creator"]["fullname"]
    self.sender_nick = data["creator"]["login"]
    self.sender_id = data["creator"]["login"]
    self.image = data["creator"]["small_avatar_url"]
    
    self.time = support.parse_time(data["created_at"])
    self.text = data["body"] or ""
    self.bgcolor = "message_color"
    self.id = data["id"]
    
    self.url = "http://brightkite.com/objects/%s" % data["id"]
    self.profile_url = "http://brightkite.com/people/%s" % self.sender_nick
    
    self.html_string = '<span class="text">%s</span>' % \
      NICK_PARSE.sub('@<a class="inlinenick" href="http://brightkite.com/people/\\1">\\1</a>',
        support.linkify(self.text))
    
    self.is_reply = ("@%s" % self.username) in self.text
    self.can_thread = data["comments_count"] > 0

    # Geolocation
    self.location_lon = data["place"]["longitude"]
    self.location_lat = data["place"]["latitude"]
    self.location_id = data["place"]["id"]
    self.location_name = data["place"]["name"]
    self.location_fullname = data["place"]["display_location"]
    self.geo_position = (self.location_lat, self.location_lon)

    if "photo" in data:
      self.thumbnails = [{"src": data["photo"], "href": data["photo"]}]

class Comment:
  def __init__(self, client, data):
    self.client = client
    self.account = client.account
    self.service = client.account["service"]
    self.username = client.account["username"]

    self.sender = data["user"]["fullname"]
    self.sender_nick = data["user"]["login"]
    self.sender_id = data["user"]["login"]
    self.image = data["user"]["small_avatar_url"]
    
    self.time = support.parse_time(data["created_at"])
    self.text = data["comment"]
    self.bgcolor = "message_color"
    
    self.url = "http://brightkite.com/objects/%s" % data["place_object"]["id"]
    self.profile_url = "http://brightkite.com/people/%s" % self.sender_nick
    
    self.html_string = '<span class="text">%s</span>' % \
      NICK_PARSE.sub('@<a class="inlinenick" href="http://brightkite.com/people/\\1">\\1</a>',
        support.linkify(self.text))
    
    self.is_reply = ("@%s" % self.username) in self.text

class FriendPosition:
  def __init__(self, client, data):
    self.client = client
    self.account = client.account
    self.service = client.account["service"]
    self.username = client.account["username"]
    self.sender = data["fullname"]
    self.sender_nick = data["login"]
    self.sender_id = self.sender_nick
    self.time = support.parse_time(data["last_checked_in"])
    self.text = data["place"]["display_location"]
    self.image = data["small_avatar_url"]
    self.image_small = data["smaller_avatar_url"]
    self.bgcolor = "message_color"
    self.url = "http://brightkite.com" # TODO
    self.profile_url = "http://brightkite.com" # TODO
    self.is_reply = False

    # Geolocation
    self.location_lon = data["place"]["longitude"]
    self.location_lat = data["place"]["latitude"]
    self.location_id = data["place"]["id"]
    self.location_name = data["place"]["name"]
    self.location_fullname = data["place"]["display_location"]

class Client:
  def __init__(self, acct):
    self.account = acct

  def get_auth(self):
    return "Basic %s" % base64.encodestring(
      ("%s:%s" % (self.account["username"], self.account["password"]))).strip()

  def connect(self, url, data = None):
    return urllib2.urlopen(urllib2.Request(
      url, data, {"Authorization": self.get_auth()}))

  def get_friend_positions(self):
    return simplejson.load(self.connect(
      "http://brightkite.com/me/friends.json"))

  def get_messages(self):
    return simplejson.load(self.connect(
      "http://brightkite.com/me/friendstream.json"))

  def get_responses(self):
    return simplejson.load(self.connect(
      "http://brightkite.com/me/mentionsstream.json"))

  def get_thread_data(self, msg):
    return simplejson.load(self.connect(
      "http://brightkite.com/objects/%s/comments.json" % msg.id))

  def get_search(self, query):
    return minidom.parseString(urllib2.urlopen(
      urllib2.Request("http://identi.ca/search/notice/rss",
        urllib.urlencode({"q": query}))).read()).getElementsByTagName("item")

  def thread(self, msg):
    yield msg
    for data in self.get_thread_data(msg):
      yield Comment(self, data)

  def friend_positions(self):
    for data in self.get_friend_positions():
      yield FriendPosition(self, data)

  def search(self, query):
    for data in self.get_search(query):
      yield SearchResult(self, data, query)

  def responses(self):
    for data in self.get_responses():
      yield Message(self, data)

  def receive(self):
    for data in self.get_messages():
      yield Message(self, data)

  def send(self, message):
    return self.connect("http://identi.ca/api/statuses/update.json",
        urllib.urlencode({"status":message}))
