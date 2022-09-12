# SafeGConfClient.py 
#  
#  Copyright (c) 2010 Canonical
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

import gconf
import gobject

class SafeGConfClient(object):
    """A gconfclient that does not crash if gconf is not avaialble"""
    def __init__(self):
        self.gconfclient = gconf.client_get_default()

    def get_bool(self, key, default=False):
        try:
            return self.gconfclient.get_bool(key)
        except gobject.GError, e:
            return default
    def get_int(self, key, default=0):
        try:
            return self.gconfclient.get_int(key)
        except gobject.GError, e:
            return default
    def get_pair(self, key, vtype1, vtype2):
        try:
            return self.gconfclient.get_pair(key, vtype1, vtype2)
        except gobject.GError, e:
            return (0, 0)
    def get_string(self, key, default=""):
        try:
            return self.gconfclient.get_string(key)
        except gobject.GError, e:
            return default
    def set_int(self, key, value):
        try:
            self.gconfclient.set_int(key, value)
        except gobject.GError, e:
            pass
    def set_bool(self, key, value):
        try:
            self.gconfclient.set_bool(key, value)
        except gobject.GError, e:
            pass
    def set_pair(self, key, vtype1, vtype2, value1, value2):
        try:
            self.gconfclient.set_pair(key, vtype1, vtype2, value1, value2)
        except gobject.GError, e:
            pass
    def set_string(self, key, value):
        try:
            self.gconfclient.set_string(key, value)
        except gobject.GError, e:
            pass
