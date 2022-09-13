import network, util
from util import exceptions
from gettext import lgettext as _

PROTOCOL_INFO = {
  "name": "Qaiku",
  "version": "1.0",
  
  "config": [
    "private:password",
    "username",
    "color",
    "receive_enabled",
    "send_enabled",
  ],
 
  "authtype": "login",
  "color": "#729FCF",

  "features": [
    "send",
    "receive",
    "reply",
    "responses",
    "thread",
    "send_thread",
    "user_messages",
  ],

  "default_streams": [
    "receive",
    "responses",
  ],
}

URL_PREFIX = "http://www.qaiku.com"

class Client:
  def __init__(self, acct):
    self.account = acct

  def _common(self, data):
    m = {}; 
    m["mid"] = str(data["id"])
    m["service"] = "qaiku"
    m["account"] = self.account["id"]
    m["time"] = util.parsetime(data["created_at"])
    m["text"] = data["text"]
    m["to_me"] = ("@%s" % self.account["username"]) in data["text"]

    m["html"] = data["html"]

    # TODO: Change Qaiku's @-links to people to Gwibber-internal ones
    m["content"] = data["html"]

    if (data["external_url"]):
      # Qaiku posts can have external links in them, display that under the message
      m["content"] += "<p><a href=\"" + data["external_url"] + "\">" + data["external_url"] + "</a></p>"

    # TODO: Display picture Qaikus

    if "channel" in data and data["channel"]:
      # Put message's Qaiku channel as "source" so it will be displayed in the UI
      m["source"] = "<a href=\"http://www.qaiku.com/channels/show/" + data["channel"] + "/\">#" + data["channel"] + "</a>"

    if "in_reply_to_status_id" in data and data["in_reply_to_status_id"]:
      m["reply"] = {}
      m["reply"]["id"] = data["in_reply_to_status_id"]
      m["reply"]["nick"] = data["in_reply_to_screen_name"]
      m["reply"]["url"] = data["in_reply_to_status_url"]

    return m

  def _message(self, data):
    m = self._common(data)
    user = data["user"]
    img = user["profile_image_url"]

    m["sender"] = {}
    m["sender"]["name"] = user["name"]
    m["sender"]["nick"] = user["screen_name"]
    m["sender"]["id"] = user["id"]
    m["sender"]["location"] = user.get("location", "")
    m["sender"]["followers"] = user["followers_count"]
    m["sender"]["image"] = "/".join((URL_PREFIX, img)) if img[0] == "/" else img
    m["sender"]["url"] = user["url"]
    m["sender"]["is_me"] = m["sender"]["nick"] == self.account["username"]
    m["url"] = "/".join((m["sender"]["url"], "show", m["mid"]))

    return m

  def _get(self, path, parse="message", post=False, single=False, **args):
    url = "/".join((URL_PREFIX, "api", path))
    url += ("&" if "?" in url else "?") + "apikey=%s" % self.account["password"]
    data = network.Download(url, util.compact(args) or None, post).get_json()

    if single: return [getattr(self, "_%s" % parse)(data)]
    if parse: return [getattr(self, "_%s" % parse)(m) for m in data]
    else: return []

  def __call__(self, opname, **args):
    return getattr(self, opname)(**args)

  def receive(self):
    return self._get("statuses/friends_timeline.json")

  def user_messages(self, id=None):
    return self._get("statuses/user_timeline.json", screen_name=id)

  def responses(self):
    return self._get("statuses/mentions.json")

  def send(self, message):
    return self._get("statuses/update.json", post=True, single=True, status=message, source='gwibbernet')

  def send_thread(self, message, target):
    recipient = target.get("reply", {}).get("id", 0) or target.get("mid", 0)
    return self._get("statuses/update.json", post=True, single=True,
        status=message, in_reply_to_status_id=recipient, source='gwibbernet')
