import network, util
from util import log
from util import exceptions
log.logger.name = "FriendFeed"


PROTOCOL_INFO = {
  "name": "FriendFeed",
  "version": 0.1,
  
  "config": [
    "private:secret_key",
    "username",
    "color",
    "receive_enabled",
    "send_enabled",
  ],

  "authtype": "login",
  "color": "#198458",

  "features": [
    "send",
    "receive",
    "public",
    "search",
    "reply",
    "thread",
    "send_thread",
    "like",
    "delete",
    "search_url",
    "user_messages",
  ],

  "default_streams": [
    "receive"
  ]
}

URL_PREFIX = "https://friendfeed.com/api"

class Client:
  def __init__(self, acct):
    self.account = acct

  def _sender(self, user):
    return {
      "name": user["name"],
      "nick": user["nickname"],
      "is_me": self.account["username"] == user["nickname"],
      "id": user["id"],
      "url": user["profileUrl"],
      "image": "http://friendfeed.com/%s/picture?size=medium" % user["nickname"],
    }

  def _message(self, data):
    m = {
      "mid": data["id"],
      "service": "friendfeed",
      "account": self.account["id"],
      "time": util.parsetime(data["published"]),
      "source": data.get("via", {}).get("name", None),
      "text": data["title"],
      "html": util.linkify(data["title"]),
      "content": util.linkify(data["title"]),
      "url": data["link"],
      "sender": self._sender(data["user"]),
    }
    
    if data.get("service", 0):
      m["origin"] = {
        "id": data["service"]["id"],
        "name": data["service"]["name"],
        "icon": data["service"]["iconUrl"],
        "url": data["service"]["profileUrl"],
      }

    if data.get("likes", 0):
      m["likes"] = {"count": len(data["likes"])}

    if data.get("comments", 0):
      m["comments"] = []
      for item in data["comments"][-3:]:
        m["comments"].append({
          "text": item["body"],
          "time": util.parsetime(item["date"]),
          "sender": self._sender(item["user"]),
        })

    for i in data["media"]:
      if i.get("thumbnails", 0):
        m["images"] = []
        for t in i["thumbnails"]:
          m["images"].append({"src": t["url"], "url": i["link"]})

    if data.get("geo", 0):
      m["location"] = data["geo"]

    return m

  def _get(self, path, post=False, parse="message", kind="entries", single=False, **args):
    url = "/".join((URL_PREFIX, path))
    data = network.Download(url, util.compact(args), post,
        self.account["username"], self.account["secret_key"]).get_json()

    if isinstance(data, dict) and data.get("errorCode", 0):
      if "unauthorized" in data["errorCode"]:
        raise exceptions.GwibberServiceError("auth", self.account["service"], self.account["username"], data["errorCode"])
   
    if single: return [getattr(self, "_%s" % parse)(data)]
    if parse: return [getattr(self, "_%s" % parse)(m) for m in data["entries"]]
    else: return []

    if parse:
      data = data[kind][0] if single else data[kind]
      return [getattr(self, "_%s" % parse)(m) for m in data]

  def __call__(self, opname, **args):
    return getattr(self, opname)(**args)

  def receive(self, count=util.COUNT, since=None):
    return self._get("feed/home", num=count, start=since)

  def public(self, count=util.COUNT, since=None):
    return self._get("feed/public", num=count, start=since)

  def thread(self, id, count=util.COUNT, since=None):
    self._get("feed/entry/%s" % id, num=count, start=since)

  def search(self, query, count=util.COUNT, since=None):
    return self._get("feed/search", q=query, num=count, start=since)

  def search_url(self, query, count=util.COUNT, since=None):
    return self._get("feed/url", url=query, num=util.COUNT, start=None)

  def user_messages(self, id, count=util.COUNT, since=None):
    return self._get("feed/user/%s" % id, num=count, start=since)

  def delete(self, message):
    self._get("entry/delete", True, None, entry=message["mid"])
    return []

  def like(self, message):
    self._get("entry/like", True, None, entry=message["mid"])
    return []

  def send(self, message):
    self._get("share", True, single=True, title=message)
    return []

  def send_thread(self, message, target):
    self._get("comment", True, None, body=message, entry=target["mid"])
    return []

