# UpdateManager.py 
#  
#  Copyright (c) 2004-2008 Canonical
#                2004 Michiel Sikkes
#                2005 Martin Willemoes Hansen
#                2010 Mohamed Amine IL Idrissi
#  
#  Author: Michiel Sikkes <michiel@eyesopened.nl>
#          Michael Vogt <mvo@debian.org>
#          Martin Willemoes Hansen <mwh@sysrq.dk>
#          Mohamed Amine IL Idrissi <ilidrissiamine@gmail.com>
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

import pygtk
pygtk.require('2.0')
import gtk
import gtk.gdk
import gconf
import gobject
gobject.threads_init()
import glib

import warnings
warnings.filterwarnings("ignore", "Accessed deprecated property", DeprecationWarning)
import apt
import apt_pkg

import gettext
import copy
import string
import sys
import os
import os.path
import stat
import re
import locale
import logging
import tempfile
import pango
import subprocess
import pwd
import urllib2
import httplib
import socket
import time
import thread
import xml.sax.saxutils

import dbus
import dbus.service
import dbus.glib

import GtkProgress
import backend

from gettext import gettext as _
from gettext import ngettext


from Core.utils import *
from Core.UpdateList import UpdateList
from Core.MyCache import MyCache, NotEnoughFreeSpaceError
from Core.MetaRelease import Dist
from Core.AlertWatcher import AlertWatcher
from SafeGConfClient import SafeGConfClient

from DistUpgradeFetcher import DistUpgradeFetcherGtk
from ChangelogViewer import ChangelogViewer
from SimpleGtkbuilderApp import SimpleGtkbuilderApp
from HelpViewer import HelpViewer
from MetaReleaseGObject import MetaRelease

#import pdb

# FIXME:
# - kill "all_changes" and move the changes into the "Update" class

# list constants
(LIST_CONTENTS, LIST_NAME, LIST_PKG, LIST_ORIGIN, LIST_TOGGLE_CHECKED) = range(5)

# actions for "invoke_manager"
(INSTALL, UPDATE) = range(2)

# file that signals if we need to reboot
REBOOT_REQUIRED_FILE = "/var/run/reboot-required"

# NetworkManager enums
NM_STATE_UNKNOWN = 0
NM_STATE_ASLEEP = 1
NM_STATE_CONNECTING = 2
NM_STATE_CONNECTED = 3
NM_STATE_DISCONNECTED = 4

class UpdateManagerDbusControler(dbus.service.Object):
    """ this is a helper to provide the UpdateManagerIFace """
    def __init__(self, parent, bus_name,
                 object_path='/org/freedesktop/UpdateManagerObject'):
        dbus.service.Object.__init__(self, bus_name, object_path)
        self.parent = parent

    @dbus.service.method('org.freedesktop.UpdateManagerIFace')
    def bringToFront(self):
        self.parent.window_main.present()
        return True

class UpdateManager(SimpleGtkbuilderApp):

  def __init__(self, datadir, options):
    self.setupDbus()
    gtk.window_set_default_icon_name("update-manager")
    self.datadir = datadir
    SimpleGtkbuilderApp.__init__(self, datadir+"glade/UpdateManager.ui",
                                 "update-manager")
    gettext.bindtextdomain("update-manager", "/usr/share/locale")
    gettext.textdomain("update-manager")
    try:
        locale.setlocale(locale.LC_ALL, "")
    except:
        logging.exception("setlocale failed")

    # Used for inhibiting power management
    self.sleep_cookie = None
    self.sleep_dev = None

    self.image_logo.set_from_icon_name("update-manager", gtk.ICON_SIZE_DIALOG)
    self.window_main.set_sensitive(False)
    self.window_main.grab_focus()
    self.button_close.grab_focus()
    self.dl_size = 0

    # create text view
    self.textview_changes = ChangelogViewer()
    self.textview_changes.show()
    self.scrolledwindow_changes.add(self.textview_changes)
    changes_buffer = self.textview_changes.get_buffer()
    changes_buffer.create_tag("versiontag", weight=pango.WEIGHT_BOLD)

    # expander
    self.expander_details.connect("notify::expanded", self.activate_details)

    # useful exit stuff
    self.window_main.connect("delete_event", self.close)
    self.button_close.connect("clicked", lambda w: self.exit())

    # the treeview (move into it's own code!)
    self.store = gtk.ListStore(str, str, gobject.TYPE_PYOBJECT, 
                               gobject.TYPE_PYOBJECT, bool)
    self.treeview_update.set_model(self.store)
    self.treeview_update.set_headers_clickable(True);
    self.treeview_update.set_direction(gtk.TEXT_DIR_LTR)

    tr = gtk.CellRendererText()
    tr.set_property("xpad", 6)
    tr.set_property("ypad", 6)
    cr = gtk.CellRendererToggle()
    cr.set_property("activatable", True)
    cr.set_property("xpad", 6)
    cr.connect("toggled", self.toggled)

    column_install = gtk.TreeViewColumn("Install", cr, active=LIST_TOGGLE_CHECKED)
    column_install.set_cell_data_func (cr, self.install_column_view_func)
    column = gtk.TreeViewColumn("Name", tr, markup=LIST_CONTENTS)
    column.set_resizable(True)
    major,minor,patch = gtk.pygtk_version

    column_install.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
    column_install.set_fixed_width(30)
    column.set_sizing(gtk.TREE_VIEW_COLUMN_FIXED)
    column.set_fixed_width(100)
    self.treeview_update.set_fixed_height_mode(False)

    self.treeview_update.append_column(column_install)
    column_install.set_visible(True)
    self.treeview_update.append_column(column)
    self.treeview_update.set_search_column(LIST_NAME)
    self.treeview_update.connect("button-press-event", self.show_context_menu)
    self.treeview_update.connect("row-activated", self.row_activated)

    # setup the help viewer and disable the help button if there
    # is no viewer available
    #self.help_viewer = HelpViewer("update-manager")
    #if self.help_viewer.check() == False:
    #    self.button_help.set_sensitive(False)

    if not os.path.exists("/usr/bin/software-properties-gtk"):
        self.button_settings.set_sensitive(False)

    self.gconfclient = SafeGConfClient()
    init_proxy(self.gconfclient)
    # init show version
    self.show_versions = self.gconfclient.get_bool("/apps/update-manager/show_versions")
    # init summary_before_name
    self.summary_before_name = self.gconfclient.get_bool("/apps/update-manager/summary_before_name")
    # keep track when we run (for update-notifier)
    self.gconfclient.set_int("/apps/update-manager/launch_time", int(time.time()))

    # get progress object
    self.progress = GtkProgress.GtkOpProgressInline(
        self.progressbar_cache_inline, self.window_main)

    #set minimum size to prevent the headline label blocking the resize process
    self.window_main.set_size_request(500,-1) 
    # restore state
    self.restore_state()
    # deal with no-focus-on-map
    if options.no_focus_on_map:
        self.window_main.set_focus_on_map(False)
        if self.progress._window:
            self.progress._window.set_focus_on_map(False)
    # show the main window
    self.window_main.show()
    # get the install backend
    self.install_backend = backend.get_backend(self.window_main)
    self.install_backend.connect("action-done", self._on_backend_done)
    # it can only the iconified *after* it is shown (even if the docs
    # claim otherwise)
    if options.no_focus_on_map:
        self.window_main.iconify()
        self.window_main.stick()
        self.window_main.set_urgency_hint(True)
        self.initial_focus_id = self.window_main.connect(
            "focus-in-event", self.on_initial_focus_in)
    
    # Alert watcher
    self.alert_watcher = AlertWatcher()
    self.alert_watcher.connect("network-alert", self._on_network_alert)
    self.alert_watcher.connect("battery-alert", self._on_battery_alert)

  def on_initial_focus_in(self, widget, event):
      """callback run on initial focus-in (if started unmapped)"""
      widget.unstick()
      widget.set_urgency_hint(False)
      self.window_main.disconnect(self.initial_focus_id)
      return False

  def warn_on_battery(self):
      """check and warn if on battery"""
      if on_battery():
          self.dialog_on_battery.set_transient_for(self.window_main)
          self.dialog_on_battery.set_title("")
          res = self.dialog_on_battery.run()
          self.dialog_on_battery.hide()
          if res != gtk.RESPONSE_YES:
              sys.exit()

  def install_column_view_func(self, cell_layout, renderer, model, iter):
    pkg = model.get_value(iter, LIST_PKG)
    # hide it if we are only a header line
    renderer.set_property("visible", pkg != None)
    if pkg is None:
        return
    current_state = renderer.get_property("active")
    to_install = pkg.marked_install or pkg.marked_upgrade
    renderer.set_property("active", to_install)
    # we need to update the store as well to ensure orca knowns
    # about state changes (it will not read view_func changes)
    if to_install != current_state:
        self.store[iter][LIST_TOGGLE_CHECKED] = to_install
    if pkg.name in self.list.held_back:
        renderer.set_property("activatable", False)
    else: 
        renderer.set_property("activatable", True)

  def setupDbus(self):
    """ this sets up a dbus listener if none is installed alread """
    # check if there is another g-a-i already and if not setup one
    # listening on dbus
    try:
        bus = dbus.SessionBus()
    except:
        print "warning: could not initiate dbus"
        return
    try:
        proxy_obj = bus.get_object('org.freedesktop.UpdateManager', 
                                   '/org/freedesktop/UpdateManagerObject')
        iface = dbus.Interface(proxy_obj, 'org.freedesktop.UpdateManagerIFace')
        iface.bringToFront()
        #print "send bringToFront"
        sys.exit(0)
    except dbus.DBusException, e:
         #print "no listening object (%s) "% e
         bus_name = dbus.service.BusName('org.freedesktop.UpdateManager',bus)
         self.dbusControler = UpdateManagerDbusControler(self, bus_name)


  def on_checkbutton_reminder_toggled(self, checkbutton):
    self.gconfclient.set_bool("/apps/update-manager/remind_reload",
                              not checkbutton.get_active())

  def close(self, widget, data=None):
    if self.window_main.get_property("sensitive") is False:
        return True
    else:
        self.exit()

  
  def set_changes_buffer(self, changes_buffer, text, name, srcpkg):
    changes_buffer.set_text("")
    lines = text.split("\n")
    if len(lines) == 1:
      changes_buffer.set_text(text)
      return
    
    for line in lines:
      end_iter = changes_buffer.get_end_iter()
      version_match = re.match(r'^%s \((.*)\)(.*)\;.*$' % re.escape(srcpkg), line)
      #bullet_match = re.match("^.*[\*-]", line)
      author_match = re.match("^.*--.*<.*@.*>.*$", line)
      if version_match:
        version = version_match.group(1)
	upload_archive = version_match.group(2).strip()
        version_text = _("Version %s: \n") % version
        changes_buffer.insert_with_tags_by_name(end_iter, version_text, "versiontag")
      elif (author_match):
        pass
      else:
        changes_buffer.insert(end_iter, line+"\n")
        

  def on_treeview_update_cursor_changed(self, widget):
    path = widget.get_cursor()[0]
    # check if we have a path at all
    if path == None:
      return
    model = widget.get_model()
    iter = model.get_iter(path)

    # set descr
    pkg = model.get_value(iter, LIST_PKG)
    if pkg == None or pkg.description == None:
      changes_buffer = self.textview_changes.get_buffer()
      changes_buffer.set_text("")
      desc_buffer = self.textview_descr.get_buffer()
      desc_buffer.set_text("")
      self.notebook_details.set_sensitive(False)
      return
    long_desc = pkg.description
    self.notebook_details.set_sensitive(True)
    # do some regular expression magic on the description
    # Add a newline before each bullet
    p = re.compile(r'^(\s|\t)*(\*|0|-)',re.MULTILINE)
    long_desc = p.sub('\n*', long_desc)
    # replace all newlines by spaces
    p = re.compile(r'\n', re.MULTILINE)
    long_desc = p.sub(" ", long_desc)
    # replace all multiple spaces by newlines
    p = re.compile(r'\s\s+', re.MULTILINE)
    long_desc = p.sub("\n", long_desc)

    desc_buffer = self.textview_descr.get_buffer()
    desc_buffer.set_text(long_desc)

    # now do the changelog
    name = model.get_value(iter, LIST_NAME)
    if name == None:
      return

    changes_buffer = self.textview_changes.get_buffer()
    
    # check if we have the changes already
    if self.cache.all_changes.has_key(name):
      changes = self.cache.all_changes[name]
      self.set_changes_buffer(changes_buffer, changes[0], name, changes[1])
    else:
      if self.expander_details.get_expanded():
        lock = thread.allocate_lock()
        lock.acquire()
        t=thread.start_new_thread(self.cache.get_news_and_changelog,(name,lock))
        changes_buffer.set_text("%s\n" % _("Downloading list of changes..."))
        iter = changes_buffer.get_iter_at_line(1)
        anchor = changes_buffer.create_child_anchor(iter)
        button = gtk.Button(stock="gtk-cancel")
        self.textview_changes.add_child_at_anchor(button, anchor)
        button.show()
        id = button.connect("clicked",
                            lambda w,lock: lock.release(), lock)
        # wait for the dl-thread
        while lock.locked():
          time.sleep(0.01)
          while gtk.events_pending():
            gtk.main_iteration()
        # download finished (or canceld, or time-out)
        button.hide()
        button.disconnect(id);
    # check if we still are in the right pkg (the download may have taken
    # some time and the user may have clicked on a new pkg)
    path  = widget.get_cursor()[0]
    if path == None:
      return
    now_name = widget.get_model()[path][LIST_NAME]
    if name != now_name:
        return
    # display NEWS.Debian first, then the changelog
    changes = ""
    srcpkg = self.cache[name].sourcePackageName
    if self.cache.all_news.has_key(name):
        changes += self.cache.all_news[name]
    if self.cache.all_changes.has_key(name):
        changes += self.cache.all_changes[name]
    if changes:
        self.set_changes_buffer(changes_buffer, changes, name, srcpkg)

  def show_context_menu(self, widget, event):
    """
    Show a context menu if a right click was performed on an update entry
    """
    if event.type == gtk.gdk.BUTTON_PRESS and event.button == 3:
        menu = gtk.Menu()
        item_select_none = gtk.MenuItem(_("_Uncheck All"))
        item_select_none.connect("activate", self.select_none_updgrades)
        menu.add(item_select_none)
        num_updates = self.cache.installCount
        if num_updates == 0:
            item_select_none.set_property("sensitive", False)
        item_select_all = gtk.MenuItem(_("_Check All"))
        item_select_all.connect("activate", self.select_all_updgrades)
        menu.add(item_select_all)
        menu.popup(None, None, None, 0, event.time)
        menu.show_all()
        return True

  def select_all_updgrades(self, widget):
    """
    Select all updates
    """
    self.setBusy(True)
    self.cache.saveDistUpgrade()
    self.treeview_update.queue_draw()
    self.refresh_updates_count()
    self.setBusy(False)

  def select_none_updgrades(self, widget):
    """
    Select none updates
    """
    self.setBusy(True)
    self.cache.clear()
    self.treeview_update.queue_draw()
    self.refresh_updates_count()
    self.setBusy(False)

  def setBusy(self, flag):
      """ Show a watch cursor if the app is busy for more than 0.3 sec.
      Furthermore provide a loop to handle user interface events """
      if self.window_main.window is None:
          return
      if flag == True:
          self.window_main.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.WATCH))
      else:
          self.window_main.window.set_cursor(None)
      while gtk.events_pending():
          gtk.main_iteration()

  def refresh_updates_count(self):
      self.button_install.set_sensitive(self.cache.installCount)
      try:
          inst_count = self.cache.installCount
          self.dl_size = self.cache.requiredDownload
          count_str = ""
          download_str = ""
          if inst_count > 0:
              count_str = ngettext("%(count)s update has been selected.", 
                                   "%(count)s updates have been selected.",
                                   inst_count) % { 'count' : inst_count }
          if self.dl_size != 0:
              download_str = _("%s will be downloaded.") % (humanize_size(self.dl_size))
              self.image_downsize.set_sensitive(True)
              # do not set the buttons to sensitive/insensitive until NM
              # can deal with dialup connections properly
              #if self.alert_watcher.network_state != NM_STATE_CONNECTED:
              #    self.button_install.set_sensitive(False)
              #else:
              #    self.button_install.set_sensitive(True)
              self.button_install.set_sensitive(True)
          else:
              if inst_count > 0:
                  download_str = ngettext("The update has already been downloaded, but not installed",
                  "The updates have already been downloaded, but not installed", inst_count)
                  self.button_install.set_sensitive(True)
              else:
                  download_str = _("There are no updates to install")
                  self.button_install.set_sensitive(False)
              self.image_downsize.set_sensitive(False)
          # TRANSLATORS: this allows to switch the order of the count of
          #              updates and the download size string (if needed)
          self.label_downsize.set_text(_("%(count_str)s %(download_str)s") % {
                  'count_str' : count_str,
                  'download_str' : download_str})
          self.hbox_downsize.show()
          self.vbox_alerts.show()
      except SystemError, e:
          print "requiredDownload could not be calculated: %s" % e
          self.label_downsize.set_markup(_("Unknown download size."))
          self.image_downsize.set_sensitive(False)
          self.hbox_downsize.show()
          self.vbox_alerts.show()

  def _get_last_apt_get_update_text(self):
      """
      return a human readable string with the information when
      the last apt-get update was run
      """
      if not os.path.exists("/var/lib/apt/periodic/update-success-stamp"):
          return _("It is unknown when the package information was "
                   "updated last. Please try clicking on the 'Check' "
                   "button to update the information.")
      # calculate when the last apt-get update (or similar operation)
      # was performed
      mtime = os.stat("/var/lib/apt/periodic/update-success-stamp")[stat.ST_MTIME]
      ago_days = int( (time.time() - mtime) / (24*60*60))
      ago_hours = int((time.time() - mtime) / (60*60) )
      if ago_days > 0:
          return ngettext("The package information was last updated %(days_ago)s day ago.",
                          "The package information was last updated %(days_ago)s days ago.",
                          ago_days) % { "days_ago" : ago_days, }
      elif ago_hours > 0:
          return ngettext("The package information was last updated %(hours_ago)s hour ago.",
                          "The package information was last updated %(hours_ago)s hours ago.",
                          ago_hours) % { "hours_ago" : ago_hours, }
      else:
          return _("The package information was last updated less than one hour ago.")
      return None

  def update_last_updated_text(self):
      """timer that updates the last updated text """
      #print "update_last_updated_text"
      num_updates = self.cache.installCount
      if num_updates == 0:
          if self._get_last_apt_get_update_text() is not None:
              text_label_main = self._get_last_apt_get_update_text()
              self.label_main_details.set_text(text_label_main)
          return True
      # stop the timer if there are upgrades now
      return False

  def update_count(self):
      """activate or disable widgets and show dialog texts correspoding to
         the number of available updates"""
      self.refresh_updates_count()
      num_updates = self.cache.installCount
      text_label_main = _("Software updates correct errors, eliminate security vulnerabilities and provide new features.")
      if num_updates == 0:
          text_header= "<big><b>%s</b></big>"  % _("Your system is up-to-date")
          self.label_downsize.set_text("\n")
          if self.cache.keepCount() == 0:
              self.notebook_details.set_sensitive(False)
              self.treeview_update.set_sensitive(False)
          self.button_install.set_sensitive(False)
          self.button_close.grab_default()
          self.textview_changes.get_buffer().set_text("")
          self.textview_descr.get_buffer().set_text("")
          if self._get_last_apt_get_update_text() is not None:
              text_label_main = self._get_last_apt_get_update_text()
          # add timer to ensure we update the information when the 
          # last package count update was performed
          glib.timeout_add_seconds(10, self.update_last_updated_text)
      else:
          # show different text on first run (UX team suggestion)
          firstrun = self.gconfclient.get_bool("/apps/update-manager/first_run")
          if firstrun:
              text_header = "<big><b>%s</b></big>" % _("Welcome to Ubuntu")
              text_label_main = _("These software updates have been issued since Ubuntu was released. If you don't want to install them now, choose \"Update Manager\" from the Administration Menu later.")
              self.gconfclient.set_bool("/apps/update-manager/first_run", False)
          else:
              text_header = "<big><b>%s</b></big>" % _("Software updates are available for this computer")
              text_label_main = _("If you don't want to install them now, choose \"Update Manager\" from the Administration menu later.")
          self.notebook_details.set_sensitive(True)
          self.treeview_update.set_sensitive(True)
          self.button_install.grab_default()
          self.treeview_update.set_cursor(1)
      self.label_header.set_markup(text_header)
      self.label_main_details.set_text(text_label_main)
      return True

  def activate_details(self, expander, data):
    expanded = self.expander_details.get_expanded()
    self.vbox_updates.set_child_packing(self.expander_details,
                                        expanded,
                                        True,
                                        0,
                                        True)
    self.gconfclient.set_bool("/apps/update-manager/show_details",expanded)
    if expanded:
      self.on_treeview_update_cursor_changed(self.treeview_update)

  def on_button_reload_clicked(self, widget):
    #print "on_button_reload_clicked"
    self.check_metarelease()
    self.invoke_manager(UPDATE)

  #def on_button_help_clicked(self, widget):
  #  self.help_viewer.run()

  def on_button_settings_clicked(self, widget):
    #print "on_button_settings_clicked"
    try:
        apt_pkg.pkgsystem_unlock()
    except SystemError:
        pass
    cmd = ["/usr/bin/gksu", 
           "--desktop", "/usr/share/applications/software-properties-gtk.desktop", 
           "--", "/usr/bin/software-properties-gtk","--open-tab","2",
           "--toplevel", "%s" % self.window_main.window.xid ]
    self.window_main.set_sensitive(False)
    p = subprocess.Popen(cmd)
    while p.poll() is None:
        while gtk.events_pending():
            gtk.main_iteration()
        time.sleep(0.05)
    self.fillstore()

  def on_button_install_clicked(self, widget):
    #print "on_button_install_clicked"
    err_sum = _("Not enough free disk space")
    err_long= _("The upgrade needs a total of %s free space on disk '%s'. "
                "Please free at least an additional %s of disk "
                "space on '%s'. "
                "Empty your trash and remove temporary "
                "packages of former installations using "
                "'sudo apt-get clean'.")
    # check free space and error if its not enough
    try:
        self.cache.checkFreeSpace()
    except NotEnoughFreeSpaceError, e:
        for req in e.free_space_required_list:
            self.error(err_sum, err_long % (req.size_total,
                                            req.dir,
                                            req.size_needed,
                                            req.dir))
        return
    except SystemError, e:
        logging.exception("free space check failed")
    self.invoke_manager(INSTALL)
    
  def on_button_restart_required_clicked(self, button=None):
      self._request_reboot_via_session_manager()

  def show_reboot_required_info(self):
    self.frame_restart_required.show()
    self.label_restart_required.set_text(_("The computer needs to restart to "
                                       "finish installing updates. Please "
                                       "save your work before continuing."))

  def _request_reboot_via_session_manager(self):
    try:
        bus = dbus.SessionBus()
        proxy_obj = bus.get_object("org.gnome.SessionManager",
                                   "/org/gnome/SessionManager")
        iface = dbus.Interface(proxy_obj, "org.gnome.SessionManager")
        iface.RequestReboot()
        # FIXME: try sesion restart with hal?
    except dbus.DBusException, e:
        pass

  def invoke_manager(self, action):
    # check first if no other package manager is runing

    # don't display apt-listchanges, we already showed the changelog
    os.environ["APT_LISTCHANGES_FRONTEND"]="none"

    # Do not suspend during the update process
    (self.sleep_dev, self.sleep_cookie) = inhibit_sleep()

    # set window to insensitive
    self.window_main.set_sensitive(False)
    self.window_main.window.set_cursor(gtk.gdk.Cursor(gtk.gdk.WATCH))
#
    # do it
    if action == UPDATE:
        self.install_backend.update()
    elif action == INSTALL:
        # If the progress dialog should be closed automatically afterwards
        gconfclient =  gconf.client_get_default()
        close_on_done = gconfclient.get_bool("/apps/update-manager/"
                                             "autoclose_install_window")
        # Get the packages which should be installed and update
        pkgs_install = []
        pkgs_upgrade = []
        for pkg in self.cache:
            if pkg.marked_install:
                pkgs_install.append(pkg.name)
            elif pkg.marked_upgrade:
                pkgs_upgrade.append(pkg.name)
        self.install_backend.commit(pkgs_install, pkgs_upgrade, close_on_done)

  def _on_backend_done(self, backend, action, authorized):
    # check if there is a new reboot required notification
    if (action == INSTALL and
        os.path.exists(REBOOT_REQUIRED_FILE)):
        self.show_reboot_required_info()
    if authorized:
        msg = _("Reading package information")
        self.label_cache_progress_title.set_label("<b><big>%s</big></b>" % msg)
        self.fillstore()

    # Allow suspend after synaptic is finished
    if self.sleep_cookie:
        allow_sleep(self.sleep_dev, self.sleep_cookie)
        self.sleep_cookie = self.sleep_dev = None
    self.window_main.set_sensitive(True)
    self.window_main.window.set_cursor(None)

  def _on_network_alert(self, watcher, state):
      # do not set the buttons to sensitive/insensitive until NM
      # can deal with dialup connections properly
      if state == NM_STATE_CONNECTING:
          self.label_offline.set_text(_("Connecting..."))
          #self.button_reload.set_sensitive(False)
          self.refresh_updates_count()
          self.hbox_offline.show()
          self.vbox_alerts.show()
      elif state == NM_STATE_CONNECTED:
          #self.button_reload.set_sensitive(True)
          self.refresh_updates_count()
          self.hbox_offline.hide()
      else:
          self.label_offline.set_text(_("You may not be able to check for updates or download new updates."))
          #self.button_reload.set_sensitive(False)
          self.refresh_updates_count()
          self.hbox_offline.show()
          self.vbox_alerts.show()
          
  def _on_battery_alert(self, watcher, on_battery):
      if on_battery:
          self.hbox_battery.show()
          self.vbox_alerts.show()
      else:
          self.hbox_battery.hide()    

  def row_activated(self, treeview, path, column):
      iter = self.store.get_iter(path)
      pkg = self.store.get_value(iter, LIST_PKG)
      origin = self.store.get_value(iter, LIST_ORIGIN)
      if pkg is not None:
          return
      self.setBusy(True)
      actiongroup = apt_pkg.ActionGroup(self.cache._depcache)
      for pkg in self.list.pkgs[origin]:
          if pkg.marked_install or pkg.marked_upgrade:
              #print "marking keep: ", pkg.name
              pkg.mark_keep()
          elif not (pkg.name in self.list.held_back):
              #print "marking install: ", pkg.name
              pkg.mark_install(autoFix=False,autoInst=False)
      # check if we left breakage
      if self.cache._depcache.broken_count:
          Fix = apt_pkg.ProblemResolver(self.cache._depcache)
          Fix.resolve_by_keep()
      self.refresh_updates_count()
      self.treeview_update.queue_draw()
      del actiongroup
      self.setBusy(False)


  def toggled(self, renderer, path):
    """ a toggle button in the listview was toggled """
    iter = self.store.get_iter(path)
    pkg = self.store.get_value(iter, LIST_PKG)
    # make sure that we don't allow to toggle deactivated updates
    # this is needed for the call by the row activation callback
    if pkg is None or pkg.name in self.list.held_back:
        return False
    self.setBusy(True)
    # update the cache
    if pkg.marked_install or pkg.marked_upgrade:
        pkg.mark_keep()
        if self.cache._depcache.broken_count:
            Fix = apt_pkg.ProblemResolver(self.cache._depcache)
            Fix.resolve_by_keep()
    else:
        pkg.mark_install()
    self.treeview_update.queue_draw()
    self.refresh_updates_count()
    self.setBusy(False)

  def on_treeview_update_row_activated(self, treeview, path, column, *args):
    """
    If an update row was activated (by pressing space), toggle the 
    install check box
    """
    self.toggled(None, path)

  def on_window_main_size_allocate(self,arg1,arg2):
    """ 
    recalculates headline labels on window resize to work around
    problems with gtk word wrapping 
    (http://bugzilla.gnome.org/show_bug.cgi?id=101968)
    """
    # this number is based on border width 
    # (2*main_border_with + icon width + 2*icon_border_with)
    #     2*18 + 48 + 2*12 
    border_space = 96
    width, height = self.window_main.get_size()
    #print "on_window_main_size_allocate", width, height
    self.label_main_details.set_size_request(width - border_space ,-1)
    self.label_header.set_size_request(width - border_space,-1)
    self.label_downsize.set_size_request(width - border_space,-1)

  def exit(self):
    """ exit the application, save the state """
    self.save_state()
    #gtk.main_quit()
    sys.exit(0)

  def save_state(self):
    """ save the state  (window-size for now) """
    (x,y) = self.window_main.get_size()
    self.gconfclient.set_pair("/apps/update-manager/window_size",
                              gconf.VALUE_INT, gconf.VALUE_INT, x, y)

  def restore_state(self):
    """ restore the state (window-size for now) """
    expanded = self.gconfclient.get_bool("/apps/update-manager/show_details")
    self.expander_details.set_expanded(expanded)
    self.vbox_updates.set_child_packing(self.expander_details,
                                        expanded,
                                        True,
                                        0,
                                        True)
    (x,y) = self.gconfclient.get_pair("/apps/update-manager/window_size",
                                      gconf.VALUE_INT, gconf.VALUE_INT)
    if x > 0 and y > 0:
      self.window_main.resize(x,y)

  def fillstore(self):
    # use the watch cursor
    self.setBusy(True)
    # clean most objects
    self.dl_size = 0
    try:
        self.initCache()
    except SystemError, e:
        msg = ("<big><b>%s</b></big>\n\n%s\n'%s'" %
               (_("Could not initialize the package information"),
                _("An unresolvable problem occurred while "
                  "initializing the package information.\n\n"
                  "Please report this bug against the 'update-manager' "
                  "package and include the following error message:\n"),
                e)
               )
        dialog = gtk.MessageDialog(self.window_main,
                                   0, gtk.MESSAGE_ERROR,
                                   gtk.BUTTONS_CLOSE,"")
        dialog.set_markup(msg)
        dialog.vbox.set_spacing(6)
        dialog.run()
        dialog.destroy()
        sys.exit(1)
    self.store.clear()
    self.list = UpdateList(self)
    while gtk.events_pending():
        gtk.main_iteration()
    # fill them again
    try:
        self.list.update(self.cache)
    except SystemError, e:
        msg = ("<big><b>%s</b></big>\n\n%s\n'%s'" %
               (_("Could not calculate the upgrade"),
                _("An unresolvable problem occurred while "
                  "calculating the upgrade.\n\n"
                  "Please report this bug against the 'update-manager' "
                  "package and include the following error message:"),
                e)
               )
        dialog = gtk.MessageDialog(self.window_main,
                                   0, gtk.MESSAGE_ERROR,
                                   gtk.BUTTONS_CLOSE,"")
        dialog.set_markup(msg)
        dialog.vbox.set_spacing(6)
        dialog.run()
        dialog.destroy()
    if self.list.num_updates > 0:
      #self.treeview_update.set_model(None)
      self.scrolledwindow_update.show()
      origin_list = self.list.pkgs.keys()
      origin_list.sort(lambda x,y: cmp(x.importance,y.importance))
      origin_list.reverse()
      for origin in origin_list:
        self.store.append(['<b><big>%s</big></b>' % origin.description,
                           origin.description, None, origin,True])
        for pkg in self.list.pkgs[origin]:
          name = xml.sax.saxutils.escape(pkg.name)
          if not pkg.is_installed:
              name += _(" (New install)")
          summary = xml.sax.saxutils.escape(pkg.summary)
          if self.summary_before_name:
              contents = "%s\n<small>%s</small>" % (summary, name)
          else:
              contents = "<b>%s</b>\n<small>%s</small>" % (name, summary)
          #TRANSLATORS: the b stands for Bytes
          size = _("(Size: %s)") % humanize_size(pkg.packageSize)
          if pkg.installedVersion != None:
              version = _("From version %(old_version)s to %(new_version)s") %\
                  {"old_version" : pkg.installedVersion,
                   "new_version" : pkg.candidateVersion}
          else:
              version = _("Version %s") % pkg.candidateVersion
          if self.show_versions:
              contents = "%s\n<small>%s %s</small>" % (contents, version, size)
          else:
              contents = "%s <small>%s</small>" % (contents, size)
          self.store.append([contents, pkg.name, pkg, None, True])
      self.treeview_update.set_model(self.store)
    self.update_count()
    self.setBusy(False)
    self.check_all_updates_installable()
    self.refresh_updates_count()
    return False

  def dist_no_longer_supported(self, meta_release):
    msg = "<big><b>%s</b></big>\n\n%s" % \
          (_("Your Ubuntu release is not supported anymore"),
	   _("You will not get any further security fixes or critical "
             "updates. "
             "Please Upgrade to a later version of Ubuntu Linux."))
    dialog = gtk.MessageDialog(self.window_main, 0, gtk.MESSAGE_WARNING,
                               gtk.BUTTONS_CLOSE,"")
    dialog.set_title("")
    dialog.set_markup(msg)
    button = gtk.LinkButton(_("Upgrade information"))
    button.set_uri("http://www.ubuntu.com/getubuntu/upgrading")
    button.show()
    dialog.get_content_area().pack_end(button)
    dialog.run()
    dialog.destroy()

  def error(self, summary, details):
      " helper function to display a error message "
      msg = ("<big><b>%s</b></big>\n\n%s\n" % (summary, details) )
      dialog = gtk.MessageDialog(self.window_main,
                                 0, gtk.MESSAGE_ERROR,
                                 gtk.BUTTONS_CLOSE,"")
      dialog.set_markup(msg)
      dialog.vbox.set_spacing(6)
      dialog.run()
      dialog.destroy()

  def on_button_dist_upgrade_clicked(self, button):
      #print "on_button_dist_upgrade_clicked"
      if self.new_dist.upgrade_broken:
          return self.error(
              _("Release upgrade not possible right now"),
              _("The release upgrade can not be performed currently, "
                "please try again later. The server reported: '%s'") % self.new_dist.upgrade_broken)
      fetcher = DistUpgradeFetcherGtk(new_dist=self.new_dist, parent=self, progress=GtkProgress.GtkFetchProgress(self, _("Downloading the release upgrade tool")))
      if self.options.sandbox:
          fetcher.run_options.append("--sandbox")
      fetcher.run()
      
  def new_dist_available(self, meta_release, upgradable_to):
    self.frame_new_release.show()
    self.label_new_release.set_markup(_("<b>New Ubuntu release '%s' is available</b>") % upgradable_to.version)
    self.new_dist = upgradable_to
    

  # fixme: we should probably abstract away all the stuff from libapt
  def initCache(self): 
    # get the lock
    try:
        apt_pkg.pkgsystem_lock()
    except SystemError, e:
        pass
        #d = gtk.MessageDialog(parent=self.window_main,
        #                      flags=gtk.DIALOG_MODAL,
        #                      type=gtk.MESSAGE_ERROR,
        #                      buttons=gtk.BUTTONS_CLOSE)
        #d.set_markup("<big><b>%s</b></big>\n\n%s" % (
        #    _("Only one software management tool is allowed to "
        #      "run at the same time"),
        #    _("Please close the other application e.g. 'aptitude' "
        #      "or 'Synaptic' first.")))
        #print "error from apt: '%s'" % e
        #d.set_title("")
        #res = d.run()
        #d.destroy()
        #sys.exit()

    try:
        if hasattr(self, "cache"):
            self.cache.open(self.progress)
            self.cache._initDepCache()
        else:
            self.cache = MyCache(self.progress)
    except AssertionError:
        # if the cache could not be opened for some reason,
        # let the release upgrader handle it, it deals
        # a lot better with this
        self.ask_run_partial_upgrade()
        # we assert a clean cache
        msg=("<big><b>%s</b></big>\n\n%s"% \
             (_("Software index is broken"),
              _("It is impossible to install or remove any software. "
                "Please use the package manager \"Synaptic\" or run "
		"\"sudo apt-get install -f\" in a terminal to fix "
		"this issue at first.")))
        dialog = gtk.MessageDialog(self.window_main,
                                   0, gtk.MESSAGE_ERROR,
                                   gtk.BUTTONS_CLOSE,"")
        dialog.set_markup(msg)
        dialog.vbox.set_spacing(6)
        dialog.run()
        dialog.destroy()
        sys.exit(1)
    else:
        self.progress.all_done()

  def check_auto_update(self):
      # Check if automatic update is enabled. If not show a dialog to inform
      # the user about the need of manual "reloads"
      remind = self.gconfclient.get_bool("/apps/update-manager/remind_reload")
      if remind == False:
          return

      update_days = apt_pkg.Config.find_i("APT::Periodic::Update-Package-Lists")
      if update_days < 1:
          self.dialog_manual_update.set_transient_for(self.window_main)
          self.dialog_manual_update.set_title("")
          res = self.dialog_manual_update.run()
          self.dialog_manual_update.hide()
          if res == gtk.RESPONSE_YES:
              self.on_button_reload_clicked(None)

  def check_all_updates_installable(self):
    """ Check if all available updates can be installed and suggest
        to run a distribution upgrade if not """
    if self.list.distUpgradeWouldDelete > 0:
        self.ask_run_partial_upgrade()

  def ask_run_partial_upgrade(self):
      self.dialog_dist_upgrade.set_transient_for(self.window_main)
      self.dialog_dist_upgrade.set_title("")
      res = self.dialog_dist_upgrade.run()
      self.dialog_dist_upgrade.hide()
      if res == gtk.RESPONSE_YES:
          os.execl("/usr/bin/gksu",
                   "/usr/bin/gksu", "--desktop",
                   "/usr/share/applications/update-manager.desktop",
                   "--", "/usr/bin/update-manager", "--dist-upgrade")
      return False

  def check_metarelease(self):
      " check for new meta-release information "
      gconfclient = SafeGConfClient()
      self.meta = MetaRelease(self.options.devel_release,
                              self.options.use_proposed)
      self.meta.connect("dist_no_longer_supported",self.dist_no_longer_supported)
      # check if we are interessted in dist-upgrade information
      # (we are not by default on dapper)
      if self.options.check_dist_upgrades or \
             gconfclient.get_bool("/apps/update-manager/check_dist_upgrades"):
          self.meta.connect("new_dist_available",self.new_dist_available)
      

  def main(self, options):
    self.options = options

    # check for new distributin information
    self.check_metarelease()

    while gtk.events_pending():
      gtk.main_iteration()

    self.fillstore()
    self.check_auto_update()
    self.alert_watcher.check_alert_state()
    gtk.main()
