import network, util
from gettext import lgettext as _

PROTOCOL_INFO = {
  "name": "Digg",
  "version": "1.0",
  
  "config": [
    "receive_enabled",
    "username",
    "color",
  ],

  "authtype": "none",
  "color": "#E5E025",

  "features": [
    "receive",
  ],

  "default_streams": [
    "receive",
  ],
}

URL_PREFIX = "http://services.digg.com"

class Client:
  def __init__(self, acct):
    self.account = acct

  def _story(self, data):
    m = {}; 
    m["mid"] = str(data["id"])
    m["service"] = "digg"
    m["account"] = self.account["id"]
    m["time"] = data["submit_date"]

    m["text"] = data["title"] + "\n" + data["description"]
    m["content"] = "<b>%(title)s</b><br />%(description)s" % data
    m["html"] = "<b>%(title)s</b><br />%(description)s" % data
    user = data["friends"]["users"][0]

    m["sender"] = {}
    m["sender"]["nick"] = user["name"]
    m["sender"]["id"] = user["name"]
    m["sender"]["image"] = user["icon"]
    m["sender"]["url"] = "http://digg.com/users/%s" % user["name"]
    m["sender"]["is_me"] = user["name"] == self.account["username"]
    if user.get("fullname", 0): m["sender"]["name"] = user["fullname"]
    
    m["url"] = data["link"]
    m["likes"] = {"count": data["diggs"]}

    m["html"] = util.linkify(m["text"],
      ((util.PARSE_HASH, '#<a class="hash" href="%s#search?q=\\1">\\1</a>' % URL_PREFIX),
      (util.PARSE_NICK, '@<a class="nick" href="%s/\\1">\\1</a>' % URL_PREFIX)))

    m["content"] = util.linkify(m["text"],
      ((util.PARSE_HASH, '#<a class="hash" href="gwibber:/tag?acct=%s&query=\\1">\\1</a>' % m["account"]),
      (util.PARSE_NICK, '@<a class="nick" href="gwibber:/user?acct=%s&name=\\1">\\1</a>' % m["account"])))

    return m

  def _get(self, path, parse="story", post=False, single=False, **args):
    url = "/".join((URL_PREFIX, path)) + "?appkey=http://gwibber.com&type=json"
    
    data = network.Download(url, util.compact(args) or None, post).get_json()

    if not data: return []

    if "stories" in data:
        if single: return [getattr(self, "_%s" % parse)(data["stories"])]
        if parse: return [getattr(self, "_%s" % parse)(m) for m in data["stories"]]
        else: return []
    else:
        if ("status" in data) and ("message" in data):
            print "Digg: got message with status %s and message %s" % (data["status"], data["message"])
        return []

  def __call__(self, opname, **args):
    return getattr(self, opname)(**args)

  def receive(self):
    return self._get("user/%s/friends/dugg" % self.account["username"])
