import re, network, util, mx.DateTime

PROTOCOL_INFO = {
  "name": "Flickr",
  "version": 1.0,
  
  "config": [
    "username",
    "color",
    "receive_enabled",
  ],

  "authtype": "none",
  "color": "#C31A00",

  "features": [
    "images",
  ],

  "default_streams": [
    "images"
  ]
}

API_KEY = "36f660117e6555a9cbda4309cfaf72d0"
REST_SERVER = "http://api.flickr.com/services/rest"
BUDDY_ICON_URL = "http://farm%s.static.flickr.com/%s/buddyicons/%s.jpg"
IMAGE_URL = "http://farm%s.static.flickr.com/%s/%s_%s_%s.jpg"
IMAGE_PAGE_URL = "http://www.flickr.com/photos/%s/%s"

def parse_time(t):
  return mx.DateTime.DateTimeFromTicks(int(t)).gmtime().ticks()

class Client:
  def __init__(self, acct):
    self.account = acct

  def _message(self, data):
    m = {}
    m["mid"] = str(data["id"])
    m["service"] = "flickr"
    m["account"] = self.account["id"]
    m["time"] = parse_time(data["dateupload"])
    m["source"] = False
    m["text"] = data["title"]

    m["sender"] = {}
    m["sender"]["name"] = data["username"]
    m["sender"]["nick"] = data["ownername"]
    m["sender"]["is_me"] = data["ownername"] == self.account["username"]
    m["sender"]["id"] = data["owner"]
    m["sender"]["image"] = BUDDY_ICON_URL % (data["iconfarm"], data["iconserver"], data["owner"])
    m["sender"]["url"] = "http://www.flickr.com/people/%s" % (data["owner"])

    m["url"] = IMAGE_PAGE_URL % (data["owner"], data["id"])
    m["images"] = [{
      "full": IMAGE_URL % (data["farm"], data["server"], data["id"], data["secret"], "b"),
      "src": IMAGE_URL % (data["farm"], data["server"], data["id"], data["secret"], "m"),
      "thumb": IMAGE_URL % (data["farm"], data["server"], data["id"], data["secret"], "t"),
      "url": IMAGE_PAGE_URL % (data["owner"], data["id"])
    }]

    m["content"] = data["title"]
    m["html"] = data["title"]

    return m

  def _get(self, method, parse="message", post=False, **args):
    args.update({"api_key": API_KEY, "method": method})
    data = network.Download(REST_SERVER, args, post).get_json()
    if parse == "message":
      return [self._message(m) for m in data["photos"]["photo"]]
    else: return data

  def _getNSID(self):
    return self._get("flickr.people.findByUsername",
        parse=None, nojsoncallback="1", format="json",
        username=self.account["username"])["user"]["nsid"]

  def __call__(self, opname, **args):
    return getattr(self, opname)(**args)

  def images(self):
    return self._get("flickr.photos.getContactsPublicPhotos",
      user_id=self._getNSID(), format="json", nojsoncallback="1",
      extras="date_upload,owner_name,icon_server")
