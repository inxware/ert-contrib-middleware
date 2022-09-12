# AlertWatcher.py 
#
#  Copyright (c) 2010 Mohamed Amine IL Idrissi
#  
#  Author: Mohamed Amine IL Idrissi <ilidrissiamine@gmail.com>
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

import gobject
import dbus
from dbus.mainloop.glib import DBusGMainLoop

class AlertWatcher(gobject.GObject):
    """ a class that checks for alerts and reports them, like a battery
    or network warning """
    
    __gsignals__ = {"network-alert": (gobject.SIGNAL_RUN_FIRST,
                                      gobject.TYPE_NONE,
                                      (gobject.TYPE_INT,)),
                    "battery-alert": (gobject.SIGNAL_RUN_FIRST,
                                      gobject.TYPE_NONE,
                                      (gobject.TYPE_BOOLEAN,))
                    }
    
    def __init__(self):
        gobject.GObject.__init__(self)
        DBusGMainLoop(set_as_default=True)
        self.bus = dbus.Bus(dbus.Bus.TYPE_SYSTEM)
        self.network_state = 3 # make it always connected if NM isn't available
        
    def check_alert_state(self):
        try:
            obj = self.bus.get_object("org.freedesktop.NetworkManager",
                                      "/org/freedesktop/NetworkManager")
            obj.connect_to_signal("StateChanged", self._network_alert,
                                  dbus_interface="org.freedesktop.NetworkManager")
            interface = dbus.Interface(obj, "org.freedesktop.DBus.Properties")
            self.network_state = interface.Get("org.freedesktop.NetworkManager", "State")
            self._network_alert(self.network_state)
		
            obj = self.bus.get_object('org.freedesktop.UPower',
                                      '/org/freedesktop/UPower')
            obj.connect_to_signal("Changed", self._power_changed,
                                  dbus_interface="org.freedesktop.UPower")
            self._power_changed()
        except dbus.exceptions.DBusException, e:
            pass
    
    def _network_alert(self, state):
        self.network_state = state
        self.emit("network-alert", state)
        
    def _power_changed(self):
        obj = self.bus.get_object("org.freedesktop.UPower", \
                                "/org/freedesktop/UPower")
        interface = dbus.Interface(obj, "org.freedesktop.DBus.Properties")
        on_battery = interface.Get("org.freedesktop.UPower", "OnBattery")
        self.emit("battery-alert", on_battery)
        
    
