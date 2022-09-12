
"""

Ping.fm interface for Gwibber
SegPhault (Ryan Paul) - 11/09/2008

"""

import network, util

PROTOCOL_INFO = {
  "name": "Ping.fm",
  "version": 0.1,
  
  "config": [
    "username",
    "private:secret_key",
    "send_enabled",
    "color",
  ],

  "authtype": "token",
  "color": "#4E9A06",

  "features": [
    "send",
  ],

  "default_streams": []
}

API_KEY = "7c3d2c111be8979ac236eecddf6679e7"

class Client:
  def __init__(self, acct):
    self.account = acct

  def __call__(self, opname, **args):
    return getattr(self, opname)(**args)

  def send_enabled(self):
    return self.account["send_enabled"] and self.account["app_key"]

  def send(self, message):
    args = {
        "post_method": "microblog",
        "user_app_key": self.account["secret_key"],
        "api_key": API_KEY,
        "body": message
    }

    data = network.Download("http://api.ping.fm/v1/user.post", util.compact(args), post=True)
    return []
