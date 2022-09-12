# utils.py 
#  
#  Copyright (c) 2004-2008 Canonical
#  
#  Author: Michael Vogt <mvo@debian.org>
# 
#  This program is free software; you can redistribute it and/or 
#  modify it under the terms of the GNU General Public License as 
#  published by the Free Software Foundation; either version 2 of the
#  License, or (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
#  USA

from gettext import gettext as _
from stat import *

import apt_pkg
apt_pkg.init_config()

import locale
import logging
import re
import os
import os.path
import subprocess
import sys
import time
import urllib2
import urlparse


class ExecutionTime(object):
    """
    Helper that can be used in with statements to have a simple
    measure of the timming of a particular block of code, e.g.
    with ExecutinTime("db flush"):
        db.flush()
    """
    def __init__(self, info=""):
        self.info = info
    def __enter__(self):
        self.now = time.time()
    def __exit__(self, type, value, stack):
        print "%s: %s" % (self.info, time.time() - self.now)

# helpers inspired after textwrap - unfortunately
# we can not use textwrap directly because it break
# packagenames with "-" in them into new lines
def wrap(t, width=70, subsequent_indent=""):
    out = ""
    for s in t.split():
        if (len(out)-out.rfind("\n")) + len(s) > width:
            out += "\n" + subsequent_indent
        out += s + " "
    return out
    
def twrap(s, **kwargs):
    msg = ""
    paras = s.split("\n")
    for par in paras:
        s = wrap(par, **kwargs)
        msg += s+"\n"
    return msg

def lsmod():
  " return list of loaded modules (or [] if lsmod is not found) "
  modules=[]
  # FIXME raise?
  if not os.path.exists("/sbin/lsmod"):
    return []
  p=subprocess.Popen(["/sbin/lsmod"], stdout=subprocess.PIPE)
  lines=p.communicate()[0].split("\n")
  # remove heading line: "Modules Size Used by"
  del lines[0]
  # add lines to list, skip empty lines
  for line in lines:
    if line:
      modules.append(line.split()[0])
  return modules


def check_and_fix_xbit(path):
  " check if a given binary has the executable bit and if not, add it"
  if not os.path.exists(path):
    return
  mode = S_IMODE(os.stat(path)[ST_MODE])
  if not ((mode & S_IXUSR) == S_IXUSR):
    os.chmod(path, mode | S_IXUSR)

def country_mirror():
  " helper to get the country mirror from the current locale "
  # special cases go here
  lang_mirror = { 'c'     : '',
                }
  # no lang, no mirror
  if not 'LANG' in os.environ:
    return ''
  lang = os.environ['LANG'].lower()
  # check if it is a special case
  if lang[:5] in lang_mirror:
    return lang_mirror[lang[:5]]
  # now check for the most comon form (en_US.UTF-8)
  if "_" in lang:
    country = lang.split(".")[0].split("_")[1]
    if "@" in country:
       country = country.split("@")[0]
    return country+"."
  else:
    return lang[:2]+"."
  return ''

def get_dist():
  " return the codename of the current runing distro "
  # support debug overwrite
  dist = os.environ.get("META_RELEASE_FAKE_CODENAME")
  if dist:
      logging.warn("using fake release name '%s' (because of META_RELEASE_FAKE_CODENAME environment) " % dist)
      return dist
  # then check the real one
  from subprocess import Popen, PIPE
  p = Popen(["lsb_release","-c","-s"],stdout=PIPE)
  res = p.wait()
  if res != 0:
    sys.stderr.write("lsb_release returned exitcode: %i\n" % res)
    return "unknown distribution"
  dist = p.stdout.readline().strip()
  return dist

class HeadRequest(urllib2.Request):
    def get_method(self):
        return "HEAD"

def url_downloadable(uri, debug_func=None):
  """
  helper that checks if the given uri exists and is downloadable
  (supports optional debug_func function handler to support 
   e.g. logging)

  Supports http (via HEAD) and ftp (via size request)
  """
  if not debug_func:
      lambda x: True
  debug_func("url_downloadable: %s" % uri)
  (scheme, netloc, path, querry, fragment) = urlparse.urlsplit(uri)
  debug_func("s='%s' n='%s' p='%s' q='%s' f='%s'" % (scheme, netloc, path, querry, fragment))
  if scheme == "http":
    try:
        http_file = urllib2.urlopen(HeadRequest(uri))
        http_file.close()
        if http_file.code == 200:
            return True
        return False
    except Exception, e:
      debug_func("error from httplib: '%s'" % e)
      return False
  elif scheme == "ftp":
    import ftplib
    try:
      f = ftplib.FTP(netloc)
      f.login()
      f.cwd(os.path.dirname(path))
      size = f.size(os.path.basename(path))
      f.quit()
      if debug_func:
        debug_func("ftplib.size() returned: %s" % size)
      if size != 0:
        return True
    except Exception, e:
      if debug_func:
        debug_func("error from ftplib: '%s'" % e)
      return False
  return False

def init_proxy(gconfclient=None):
  """ init proxy settings 

  * first check for http_proxy environment (always wins),
  * then check the apt.conf http proxy, 
  * then look into synaptics conffile
  * then into gconf  (if gconfclient was supplied)
  """
  SYNAPTIC_CONF_FILE = "/root/.synaptic/synaptic.conf"
  proxy = None
  # generic apt config wins
  if apt_pkg.config.find("Acquire::http::Proxy") != '':
    proxy = apt_pkg.config.find("Acquire::http::Proxy")
  # then synaptic
  elif os.path.exists(SYNAPTIC_CONF_FILE):
    cnf = apt_pkg.Configuration()
    apt_pkg.read_config_file(cnf, SYNAPTIC_CONF_FILE)
    use_proxy = cnf.find_b("Synaptic::useProxy", False)
    if use_proxy:
      proxy_host = cnf.find("Synaptic::httpProxy")
      proxy_port = str(cnf.find_i("Synaptic::httpProxyPort"))
      if proxy_host and proxy_port:
        proxy = "http://%s:%s/" % (proxy_host, proxy_port)
  # then gconf
  elif gconfclient:
    try: # see LP: #281248
      if gconfclient.get_bool("/system/http_proxy/use_http_proxy"):
        host = gconfclient.get_string("/system/http_proxy/host")
        port = gconfclient.get_int("/system/http_proxy/port")
        use_auth = gconfclient.get_bool("/system/http_proxy/use_authentication")
        if host and port:
          if use_auth:
            auth_user = gconfclient.get_string("/system/http_proxy/authentication_user")
            auth_pw = gconfclient.get_string("/system/http_proxy/authentication_password")
            proxy = "http://%s:%s@%s:%s/" % (auth_user,auth_pw,host, port)
          else:
            proxy = "http://%s:%s/" % (host, port)
    except Exception, e:
      print "error from gconf: %s" % e
  # if we have a proxy, set it
  if proxy:
    # basic verification
    if not re.match("http://\w+", proxy):
      print >> sys.stderr, "proxy '%s' looks invalid" % proxy
      return
    proxy_support = urllib2.ProxyHandler({"http":proxy})
    opener = urllib2.build_opener(proxy_support)
    urllib2.install_opener(opener)
    os.putenv("http_proxy",proxy)
  return proxy

def on_battery():
  """
  Check via dbus if the system is running on battery.
  This function is using UPower per default, if UPower is not
  available it falls-back to DeviceKit.Power. 
  """
  try:
    import dbus
    bus = dbus.Bus(dbus.Bus.TYPE_SYSTEM)
    try:
        devobj = bus.get_object('org.freedesktop.UPower', 
                                '/org/freedesktop/UPower')
        dev = dbus.Interface(devobj, 'org.freedesktop.DBus.Properties')
        return dev.Get('org.freedesktop.UPower', 'OnBattery')
    except dbus.exceptions.DBusException, e:
        if e._dbus_error_name != 'org.freedesktop.DBus.Error.ServiceUnknown':
            raise
        devobj = bus.get_object('org.freedesktop.DeviceKit.Power', 
                                '/org/freedesktop/DeviceKit/Power')
        dev = dbus.Interface(devobj, "org.freedesktop.DBus.Properties")
        return dev.Get("org.freedesktop.DeviceKit.Power", "on_battery")
  except Exception, e:
    #import sys
    #print >>sys.stderr, "on_battery returned error: ", e
    return False

def _inhibit_sleep_old_interface():
  """
  Send a dbus signal to org.gnome.PowerManager to not suspend
  the system, this is to support upgrades from pre-gutsy g-p-m
  """
  import dbus
  bus = dbus.Bus(dbus.Bus.TYPE_SESSION)
  devobj = bus.get_object('org.gnome.PowerManager', 
                          '/org/gnome/PowerManager')
  dev = dbus.Interface(devobj, "org.gnome.PowerManager")
  cookie = dev.Inhibit('UpdateManager', 'Updating system')
  return (dev, cookie)

def _inhibit_sleep_new_interface():
  """
  Send a dbus signal to gnome-power-manager to not suspend
  the system
  """
  import dbus
  bus = dbus.Bus(dbus.Bus.TYPE_SESSION)
  devobj = bus.get_object('org.freedesktop.PowerManagement', 
                          '/org/freedesktop/PowerManagement/Inhibit')
  dev = dbus.Interface(devobj, "org.freedesktop.PowerManagement.Inhibit")
  cookie = dev.Inhibit('UpdateManager', 'Updating system')
  return (dev, cookie)

def inhibit_sleep():
  """
  Send a dbus signal to power-manager to not suspend
  the system, try both the new freedesktop and the
  old gnome dbus interface
  """
  try:
    return _inhibit_sleep_old_interface()
  except Exception, e:
    try:
      return _inhibit_sleep_new_interface()
    except Exception, e:
      #print "could not send the dbus Inhibit signal: %s" % e
      return (False, False)

def allow_sleep(dev, cookie):
  """Send a dbus signal to gnome-power-manager to allow a suspending
  the system"""
  try:
    dev.UnInhibit(cookie)
  except Exception, e:
    print "could not send the dbus UnInhibit signal: %s" % e


def str_to_bool(str):
  if str == "0" or str.upper() == "FALSE":
    return False
  return True

def utf8(str):
  return unicode(str, 'latin1').encode('utf-8')

def get_lang():
    import logging
    try:
        (locale_s, encoding) = locale.getdefaultlocale()
        return locale_s
    except Exception, e: 
        logging.exception("gedefaultlocale() failed")
        return None

def error(parent, summary, message):
  import gtk
  d = gtk.MessageDialog(parent=parent,
                        flags=gtk.DIALOG_MODAL,
                        type=gtk.MESSAGE_ERROR,
                        buttons=gtk.BUTTONS_CLOSE)
  d.set_markup("<big><b>%s</b></big>\n\n%s" % (summary, message))
  d.realize()
  d.window.set_functions(gtk.gdk.FUNC_MOVE)
  d.set_title("")
  res = d.run()
  d.destroy()
  return False

def humanize_size(bytes):
    """
    Convert a given size in bytes to a nicer better readable unit
    """
    if bytes == 0:
        # TRANSLATORS: download size is 0
        return _("0 KB")
    elif bytes < 1024:
        # TRANSLATORS: download size of very small updates
        return _("1 KB")
    elif bytes < 1024 * 1024:
        # TRANSLATORS: download size of small updates, e.g. "250 KB"
        return locale.format(_("%.0f KB"), bytes/1024)
    else:
        # TRANSLATORS: download size of updates, e.g. "2.3 MB"
        return locale.format(_("%.1f MB"), bytes / 1024 / 1024)

if __name__ == "__main__":
  #print mirror_from_sources_list()
  print on_battery()
