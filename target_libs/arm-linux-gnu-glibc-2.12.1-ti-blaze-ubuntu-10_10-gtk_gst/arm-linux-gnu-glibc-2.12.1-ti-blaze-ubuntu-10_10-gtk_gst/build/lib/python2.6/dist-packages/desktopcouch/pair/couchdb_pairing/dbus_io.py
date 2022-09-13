# Copyright 2009 Canonical Ltd.
#
# This file is part of desktopcouch.
#
# desktopcouch is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# as published by the Free Software Foundation.
#
# desktopcouch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with desktopcouch.  If not, see <http://www.gnu.org/licenses/>.
#
# Authors: Chad Miller <chad.miller@canonical.com>
"""Communicate with DBUS and also the APIs it proxies, like Zeroconf."""

import logging

import dbus
import avahi

from desktopcouch.pair.couchdb_pairing import couchdb_io

invitations_discovery_service_type = "_couchdb_pairing_invitations._tcp"
location_discovery_service_type = "_couchdb_location._tcp"
desktopcouch_dbus_interface = "org.desktopcouch.CouchDB"

def get_local_hostname():
    """Get the name of this host, as Unicode host and domain parts."""
    bus, server = get_dbus_bus_server()
    return server.GetHostName(), server.GetDomainName()

def get_remote_hostname(ip_address_as_ascii, iface=avahi.IF_UNSPEC):
    """Look up the name of a host by its address.  This is like DNS
    PTR in-arpa, but all zeroconf-y."""
    bus, server = get_dbus_bus_server()
    try:
        interface, protocol, aprotocol, address, hostname, flags = \
                server.ResolveAddress(iface, avahi.PROTO_UNSPEC, 
                        ip_address_as_ascii, dbus.UInt32(0))
    except dbus.exceptions.DBusException:
        return ip_address_as_ascii

    return hostname

def get_dbus_bus_server(interface="root"):
    """Common sequence of steps to get a Bus and Server object from DBUS."""
    bus = dbus.SystemBus()
    root_name = bus.get_object(avahi.DBUS_NAME, avahi.DBUS_PATH_SERVER)
    server = dbus.Interface(root_name, avahi.DBUS_INTERFACE_SERVER)
    return bus, server

class Advertisement(object):
    """Represents an advertised service that exists on this host."""
    def __init__(self, port, name, stype="", domain="", host="", text={}):
        super(Advertisement, self).__init__()
        
        self.logging = logging.getLogger(self.__class__.__name__)
        self.name = name
        self.stype = stype
        self.domain = domain
        self.host = host
        self.port = int(port)
        if hasattr(text, "keys"):
            self.text = avahi.dict_to_txt_array(text)
        else:
            self.text = text

        self.group = None
    
    def publish(self):
        """Start the advertisement."""
        bus, server = get_dbus_bus_server()

        entry_group = bus.get_object(avahi.DBUS_NAME, server.EntryGroupNew())
        g = dbus.Interface(entry_group, avahi.DBUS_INTERFACE_ENTRY_GROUP)

        g.AddService(avahi.IF_UNSPEC, avahi.PROTO_UNSPEC, dbus.UInt32(0),
                self.name, self.stype, self.domain, self.host,
                dbus.UInt16(self.port), self.text)

        g.Commit()
        self.logging.info("starting advertising %s on port %d",
                self.stype, self.port)
        self.group = g
    
    def unpublish(self):
        """End the advertisement."""
        try:
            self.logging.info("ending advertising %s on port %d",
                    self.stype, self.port)
            self.group.Reset()
        except DBusError, e:
            self.logging.warn("Couldn't reset DBus at shutdown. %s" % (e,))
        self.group = None

    def die(self):
        """Quit."""
        self.unpublish()

class LocationAdvertisement(Advertisement):
    """An advertised couchdb location.  See Advertisement class."""
    def __init__(self, *args, **kwargs):
        kwargs["stype"] = location_discovery_service_type
        super(LocationAdvertisement, self).__init__(*args, **kwargs)

class PairAdvertisement(Advertisement):
    """An advertised couchdb pairing opportunity.  See Advertisement class."""
    def __init__(self, *args, **kwargs):
        kwargs["stype"] = invitations_discovery_service_type
        super(PairAdvertisement, self).__init__(*args, **kwargs)

def avahitext_to_dict(avahitext):
    text = {}
    for l in avahitext:
        try:
            k, v = "".join(chr(i) for i in l).split("=", 1)
            text[k] = v
        except ValueError, e:
            logging.error("k/v field could not be decoded.  %s", e)
    return text


nearby_desktop_couch_instances = dict()  # k=uuid, v=(addr, port)

def cb_found_desktopcouch_server(uuid, host_address, port):
    nearby_desktop_couch_instances[uuid] = (unicode(host_address), int(port))

def cb_lost_desktopcouch_server(uuid):
    try:
        del nearby_desktop_couch_instances[uuid]
    except KeyError:
        pass

def get_seen_paired_hosts(uri=None):
    pairing_encyclopedia = couchdb_io.get_all_known_pairings(uri=uri)
    return (
            (
                uuid, 
                addr, 
                port, 
                not pairing_encyclopedia[uuid]["active"], 
                pairing_encyclopedia[uuid]["oauth"],
            ) 
            for uuid, (addr, port) 
            in nearby_desktop_couch_instances.items() 
            if uuid in pairing_encyclopedia)

def maintain_discovered_servers(add_cb=cb_found_desktopcouch_server, 
        del_cb=cb_lost_desktopcouch_server):

    def remove_item_handler(cb, interface, protocol, name, stype, domain,
            flags):
        """A service disappeared."""

        if name.startswith("desktopcouch "):
            hostid = name[13:]
            logging.debug("lost sight of %r", hostid)
            cb(hostid)
        else:
            logging.error("annc doesn't look like one of ours.  %r", name)

    def new_item_handler(cb, interface, protocol, name, stype, domain, flags):
        """A service appeared."""

        def handle_error(*args):
            """An error in resolving a new service."""
            logging.error("zeroconf ItemNew error for services, %s", args)

        def handle_resolved(cb, *args):
            """Successfully resolved a new service, which we decode and send
            back to our calling environment with the callback function."""

            name, host, port = args[2], args[5], args[8]
            if name.startswith("desktopcouch "):
                cb(name[13:], host, port)
            else:
                logging.error("annc doesn't look like one of ours.  %r", name)
            return True

        server.ResolveService(interface, protocol, name, stype, 
            domain, avahi.PROTO_UNSPEC, dbus.UInt32(0), 
            reply_handler=lambda *a: handle_resolved(cb, *a),
            error_handler=handle_error)

    bus, server = get_dbus_bus_server()
    domain_name = get_local_hostname()[1]
    browser = server.ServiceBrowserNew(avahi.IF_UNSPEC, avahi.PROTO_UNSPEC,
            location_discovery_service_type, domain_name, dbus.UInt32(0))
    browser_name = bus.get_object(avahi.DBUS_NAME, browser)

    sbrowser = dbus.Interface(browser_name,
            avahi.DBUS_INTERFACE_SERVICE_BROWSER)
    sbrowser.connect_to_signal("ItemNew",
            lambda *a: new_item_handler(add_cb, *a))
    sbrowser.connect_to_signal("ItemRemove",
            lambda *a: remove_item_handler(del_cb, *a))
    sbrowser.connect_to_signal("Failure", 
            lambda *a: logging.error("avahi error %r", a))


def discover_services(add_commport_name_cb, del_commport_name_cb,
        show_local=False):
    """Start looking for services.  Use two callbacks to handle seeing
    new services and seeing services disappear."""

    def remove_item_handler(cb, interface, protocol, name, stype, domain, flags):
        """A service disappeared."""

        if not show_local and flags & avahi.LOOKUP_RESULT_LOCAL:
            return
        cb(name)

    def new_item_handler(cb, interface, protocol, name, stype, domain, flags):
        """A service appeared."""

        def handle_error(*args):
            """An error in resolving a new service."""
            logging.error("zeroconf ItemNew error for services, %s", args)

        def handle_resolved(cb, *args):
            """Successfully resolved a new service, which we decode and send
            back to our calling environment with the callback function."""
            text = avahitext_to_dict(args[9])
            name, host, port = args[2], args[5], args[8]
            cb(name, text.get("description", "?"),
                    host, port, text.get("version", None))

        if not show_local and flags & avahi.LOOKUP_RESULT_LOCAL:
            return

        server.ResolveService(interface, protocol, name, stype, 
            domain, avahi.PROTO_UNSPEC, dbus.UInt32(0), 
            reply_handler=lambda *a: handle_resolved(cb, *a), 
            error_handler=handle_error)

    bus, server = get_dbus_bus_server()
    domain_name = get_local_hostname()[1]

    browser = server.ServiceBrowserNew(avahi.IF_UNSPEC, avahi.PROTO_UNSPEC,
            invitations_discovery_service_type, domain_name, dbus.UInt32(0))
    browser_name = bus.get_object(avahi.DBUS_NAME, browser)

    sbrowser = dbus.Interface(browser_name,
            avahi.DBUS_INTERFACE_SERVICE_BROWSER)
    sbrowser.connect_to_signal("ItemNew", 
            lambda *a: new_item_handler(add_commport_name_cb, *a))
    sbrowser.connect_to_signal("ItemRemove",
            lambda *a: remove_item_handler(del_commport_name_cb, *a))
    sbrowser.connect_to_signal("Failure",
            lambda *a: logging.error("avahi error %r", a))
