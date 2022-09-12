import gtk, dbus, resources, os, mx.DateTime, webbrowser
from microblog.util.const import *
# Try to import * from custom, install custom.py to include packaging 
# customizations like distro API keys, etc
try:
  from microblog.util.custom import *
except:
  pass

import gettext
from gettext import ngettext

from dbus.mainloop.glib import DBusGMainLoop
DBusGMainLoop(set_as_default=True)

class Color:
  def __init__(self, hex):
    self.hex = hex
    self.dec = int(hex.replace("#", ""), 16)
    self.r = (self.dec >> 16) & 0xff
    self.g = (self.dec >> 8) & 0xff
    self.b = self.dec & 0xff
    self.rgb = "%s, %s, %s" % (self.r, self.g, self.b)

  def darker(self, pct):
    return Color("#%02x%02x%02x" % (self.r * pct, self.g * pct, self.b * pct))

  @classmethod
  def from_gtk_color(self, c):
    if isinstance(c, gtk.gdk.Color): c = c.to_string()
    return self("#" + "".join([c[1:3], c[5:7], c[9:11]]))

def get_style():
  w = gtk.Window()
  w.realize()
  return w.get_style()

def get_theme_colors(w = gtk.Window()):
  w.realize()
  style = w.get_style()
  output = {}
  
  for i in ["base", "text", "fg", "bg"]:
    c = getattr(style, i)[gtk.STATE_NORMAL]
    output[i] = Color.from_gtk_color(c)
    c = getattr(style, i)[gtk.STATE_SELECTED]
    output["%s_selected" % i] = Color.from_gtk_color(c)

  return output

def pixbuf(path):
  return gtk.gdk.pixbuf_new_from_file(resources.get_ui_asset(path))

def nop(*a): pass

load_url = webbrowser.open

def remove_urls(s):
  return ' '.join(x for x in s.strip('.').split()
    if not x.startswith('http://') and not x.startswith("https://"))

try:
  import gtkspell
except:
  gtkspell = None

def create_tomboy_note(text):
  obj = dbus.SessionBus().get_object("org.gnome.Tomboy", "/org/gnome/Tomboy/RemoteControl")
  tomboy = dbus.Interface(obj, "org.gnome.Tomboy.RemoteControl")
  
  n = tomboy.CreateNote()
  tomboy.DisplayNote(n)
  tomboy.SetNoteContents(n, text)

def generate_time_string(t):
  if isinstance(t, str): return t
  t = mx.DateTime.TimestampFromTicks(t)
  d = mx.DateTime.gmt() - t

  # Aliasing the function doesn't work here with intltool...
  if d.days >= 365:
    years = round(d.days / 365)
    return gettext.ngettext("%(year)d year ago", "%(year)d years ago", years) % {"year": years}
  elif d.days >= 1 and d.days < 365:
    days = round(d.days)
    return gettext.ngettext("%(day)d day ago", "%(day)d days ago", days) % {"day": days}
  elif d.seconds >= 3600 and d.days < 1:
    hours = round(d.seconds / 60 / 60)
    return gettext.ngettext("%(hour)d hour ago", "%(hour)d hours ago", hours) % {"hour": hours}
  elif d.seconds < 3600 and d.seconds >= 60:
    minutes = round(d.seconds / 60)
    return gettext.ngettext("%(minute)d minute ago", "%(minute)d minutes ago", minutes) % {"minute": minutes}
  elif round(d.seconds) < 60:
    seconds = round(d.seconds)
    if seconds < 0: return gettext.gettext("Just now")
    return gettext.ngettext("%(sec)d second ago", "%(sec)d seconds ago", seconds) % {"sec": seconds}
  else: return "BUG: %s" % str(d)

