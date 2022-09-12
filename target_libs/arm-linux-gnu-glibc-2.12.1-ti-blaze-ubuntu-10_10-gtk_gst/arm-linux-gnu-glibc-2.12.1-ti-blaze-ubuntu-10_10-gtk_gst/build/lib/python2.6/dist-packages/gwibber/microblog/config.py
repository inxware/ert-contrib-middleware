
try: import gconf
except: from gnome import gconf

from . import gwp
from util.const import *
# Try to import * from custom, install custom.py to include packaging 
# customizations like distro API keys, etc
try:
  from util.custom import *
except:
  pass

GCONF_DIR = "/apps/gwibber"
GCONF_PREFERENCES_DIR = GCONF_DIR + "/preferences"
GCONF = gconf.client_get_default()

def obtain_get_function(value):
  return {
    "string": value.get_string,
    "int": value.get_int,
    "float": value.get_float,
    "bool": value.get_bool
  }[value.type.value_nick]

def obtain_set_function(value):
  return {
    "str": GCONF.set_string,
    "int": GCONF.set_int,
    "float": GCONF.set_float,
    "bool": GCONF.set_bool
  }[type(value).__name__]

def dbustype(value):
  return {
    "str": gconf.VALUE_STRING,
    "int": gconf.VALUE_INT,
    "float": gconf.VALUE_FLOAT,
    "bool": gconf.VALUE_BOOL,
  }[type(value).__name__]

class Wrapper:
  def __init__(self, path):
    self.path = path

  def __getitem__(self, key):
    value = GCONF.get("%s/%s" % (self.path, key))
    if value:
      if value.type.value_nick == "list":
        return [obtain_get_function(i)() for i in value.get_list()]
      return obtain_get_function(value)()

  def __setitem__(self, key, value):
    path = "%s/%s" % (self.path, key)
    
    if isinstance(value, list) or isinstance(value, tuple):
      if len(set(type(x) for x in value)) > 1: raise TypeError
      return GCONF.set_list(path, dbustype(value[0]), value)
    
    obtain_set_function(value)(path, value)

  def __delitem__(self, key):
    GCONF.unset("%s/%s" % (self.path, key))

  def bind(self, widget, key, **args):
    gwp.create_persistency_link(widget, "%s/%s" % (self.path, key), **args)
    return widget

  def notify(self, key, method):
    GCONF.notify_add("%s/%s" % (self.path, key), method)

class Preferences(Wrapper):
  def __init__(self, path = GCONF_PREFERENCES_DIR, defaults = DEFAULT_SETTINGS):
    Wrapper.__init__(self, path)
    self.defaults = defaults

  def __getitem__(self, key):
    value = Wrapper.__getitem__(self, key)
    return self.defaults.get(key, None) if value == None else value

  def __setitem__(self, key, value):
    if self.defaults.get(key, None) != value:
      Wrapper.__setitem__(self, key, value)

  def bind(self, widget, key, **args):
    if key in self.defaults:
      args["default"] = self.defaults[key]
    return Wrapper.bind(self, widget, key, **args)
