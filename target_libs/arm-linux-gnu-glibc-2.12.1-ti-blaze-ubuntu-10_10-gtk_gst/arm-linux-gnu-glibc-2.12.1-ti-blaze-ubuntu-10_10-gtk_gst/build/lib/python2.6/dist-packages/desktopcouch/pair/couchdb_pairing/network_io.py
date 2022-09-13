# Copyright 2009 Canonical Ltd.
#
# This file is part of desktopcouch.
#
#  desktopcouch is free software: you can redistribute it and/or modify
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
"""All inter-tool communication."""

import logging
import hashlib

from twisted.internet import reactor
from twisted.internet.protocol import ServerFactory, ReconnectingClientFactory
from twisted.protocols import basic
from twisted.internet import ssl

import dbus

try:
    from dbus_io import get_remote_hostname
except ImportError:
    logging.exception("Can't import dbus_io, because avahi not installed?")
    get_remote_hostname = lambda addr: None
    

hash = hashlib.sha512


def dict_to_bytes(d):
    """Convert a dictionary of string key/values into a string."""
    parts = list()

    for k, v in d.iteritems():
        assert isinstance(k, str), k
        l = len(k)
        parts.append(chr(l>>8))
        parts.append(chr(l&255))
        parts.append(k)

        assert isinstance(v, str), v
        l = len(v)
        parts.append(chr(l>>8))
        parts.append(chr(l&255))
        parts.append(v)
        
    blob = "".join(parts)
    l = len(blob)
    blob_size = list()
    blob_size.append(chr(l>>24))
    blob_size.append(chr(l>>16&255))
    blob_size.append(chr(l>>8&255))
    blob_size.append(chr(l&255))

    return "CMbydi0" + "".join(blob_size) + blob


def bytes_to_dict(b):
    """Convert a string from C{dict_to_bytes} back into a dictionary."""
    if b[:7] != "CMbydi0":
        raise ValueError("magic bytes missing.  Invalid string.  %r", b[:10])
    b = b[7:]

    blob_size = 0
    for c in b[:4]:
        blob_size = (blob_size << 8) + ord(c)

    blob = b[4:]
    if blob_size != len(blob):
        raise ValueError("bytes are corrupt; expected %d, got %d" % (blob_size,
            len(blob)))

    d = {}
    blob_cursor = 0

    while blob_cursor < blob_size:
        k_len = (ord(blob[blob_cursor+0])<<8) + ord(blob[blob_cursor+1])
        k = blob[blob_cursor+2:blob_cursor+2+k_len]
        blob_cursor += k_len + 2 
        v_len = (ord(blob[blob_cursor+0])<<8) + ord(blob[blob_cursor+1])
        v = blob[blob_cursor+2:blob_cursor+2+v_len]
        blob_cursor += v_len + 2
        d[k] = v
    return d


class ListenForInvitations():
    """Narrative "Alice".

    This is the first half of a TCP listening socket.  We spawn off
    processors when we accept invitation-connections."""

    def __init__(self, get_secret_from_user, on_close, hostid, oauth_data):
        """Initialize."""
        self.logging = logging.getLogger(self.__class__.__name__)

        self.factory = ProcessAnInvitationFactory(get_secret_from_user,
                on_close, hostid, oauth_data)
        self.listening_port = reactor.listenTCP(0, self.factory)

    def get_local_port(self):
        """We created a socket, and the caller needs to know what our port
        number is, so it can advertise it."""

        port = self.listening_port.getHost().port
        self.logging.info("local port to receive invitations is %s", port)
        return port

    def close(self):
        """Called from the UI when a window is destroyed and we do not need
        this connection any more."""
        self.listening_port.stopListening()


class ProcessAnInvitationProtocol(basic.LineReceiver):
    """Narrative "Alice".

    Listen for messages, and when we receive one, call the display callback
    function with the inviter details plus a key."""

    def __init__(self):
        """Initialize."""
        self.logging = logging.getLogger(self.__class__.__name__)
        self.expected_hash = None
        self.public_seed = None

    def connectionMade(self):
        """Called when a connection is made.  No obligation here."""
        basic.LineReceiver.connectionMade(self)

    def connectionLost(self, reason):
        """Called when a connection is lost."""
        self.logging.debug("connection lost")
        basic.LineReceiver.connectionLost(self, reason)

    def lineReceived(self, rich_message):
        """Handler for receipt of a message from the Bob end."""
        d = bytes_to_dict(rich_message)

        self.expected_hash = d.pop("secret_message")
        self.public_seed = d.pop("public_seed")
        remote_hostid = d.pop("hostid")
        remote_oauth = d

        self.factory.get_secret_from_user(self.transport.getPeer().host,
                self.check_secret_from_user,
                self.send_secret_to_remote,
                remote_hostid, remote_oauth)
    
    def send_secret_to_remote(self, secret_message):
        """A callback for the invitation protocol to start a new phase 
        involving the other end getting the hash-digest of the public 
        seed and a secret we receive as a parameter."""
        h = hash()
        h.update(self.public_seed)
        h.update(secret_message)
        all_dict = dict()
        all_dict.update(self.factory.oauth_info)
        all_dict["hostid"] = self.factory.hostid
        all_dict["secret_message"] = h.hexdigest()
        self.sendLine(dict_to_bytes(all_dict))
        
    def check_secret_from_user(self, secret_message):
        """A callback for the invitation protocol to verify the secret
        that the user gives, against the hash we received over the
        network."""

        h = hash()
        h.update(secret_message)
        digest = h.hexdigest()

        if digest == self.expected_hash:
            h = hash()
            h.update(self.public_seed)
            h.update(secret_message)
            all_dict = dict()
            all_dict.update(self.factory.oauth_info)
            all_dict["hostid"] = self.factory.hostid
            all_dict["secret_message"] = h.hexdigest()
            self.sendLine(dict_to_bytes(all_dict))

            self.logging.debug("User knew secret!")

            self.transport.loseConnection()
            return True

        self.logging.info("User secret %r is wrong.", secret_message)
        return False


class ProcessAnInvitationFactory(ServerFactory):
    """Hold configuration values for all the connections, and fire off a
    protocol to handle the data sent and received."""

    protocol = ProcessAnInvitationProtocol

    def __init__(self, get_secret_from_user, on_close, hostid, oauth_info):
        self.logging = logging.getLogger(self.__class__.__name__)
        self.get_secret_from_user = get_secret_from_user
        self.on_close = on_close
        self.hostid = hostid
        self.oauth_info = oauth_info


class SendInvitationProtocol(basic.LineReceiver):
    """Narrative "Bob"."""

    def __init__(self):
        """Initialize."""
        self.logging = logging.getLogger(self.__class__.__name__)
        self.logging.debug("initialized")

    def connectionMade(self):
        """Fire when a connection is made to the listener.  No obligation
        here."""
        self.logging.debug("connection made")

        h = hash()
        h.update(self.factory.secret_message)
        d = dict(secret_message=h.hexdigest(),
                public_seed=self.factory.public_seed,
                hostid=self.factory.local_hostid)
        d.update(self.factory.local_oauth_info)
        self.sendLine(dict_to_bytes(d))

        h = hash()
        h.update(self.factory.public_seed)
        h.update(self.factory.secret_message)
        self.expected_hash_of_secret = h.hexdigest()


    def lineReceived(self, rich_message):
        """Handler for receipt of a message from the Alice end."""
        d = bytes_to_dict(rich_message)
        message = d.pop("secret_message")

        if message == self.expected_hash_of_secret:
            remote_host = self.transport.getPeer().host
            try:
                remote_hostname = get_remote_hostname(remote_host)
            except dbus.exceptions.DBusException:
                remote_hostname = None
            remote_hostid = d.pop("hostid")
            self.factory.auth_complete_cb(remote_hostname, remote_hostid, d)
            self.transport.loseConnection()
        else:
            self.logging.warn("Expected %r from invitation.",
                    self.expected_hash_of_secret)

    def connectionLost(self, reason):
        """When a connected socked is broken, this is fired."""
        self.logging.info("connection lost.")
        basic.LineReceiver.connectionLost(self, reason)


class SendInvitationFactory(ReconnectingClientFactory):
    """Hold configuration values for all the connections, and fire off a
    protocol to handle the data sent and received."""

    protocol = SendInvitationProtocol

    def __init__(self, auth_complete_cb, secret_message, public_seed,
            on_close, local_hostid, local_oauth_info):
        self.logging = logging.getLogger(self.__class__.__name__)
        self.auth_complete_cb = auth_complete_cb
        self.secret_message = secret_message
        self.public_seed = public_seed
        self.on_close = on_close
        self.local_hostid = local_hostid
        self.local_oauth_info = local_oauth_info
        self.logging.debug("initialized")

    def close(self):
        """Called from the UI when a window is destroyed and we do not need
        this connection any more."""
        self.logging.warn("close not handled properly")  # FIXME

    def clientConnectionFailed(self, connector, reason):
        """When we fail to connect to the listener, this is fired."""
        self.logging.warn("connect failed. %s", reason)
        ReconnectingClientFactory.clientConnectionFailed(self, connector,
                reason)

    def clientConnectionLost(self, connector, reason):
        """When a connected socked is broken, this is fired."""
        self.logging.info("connection lost. %s", reason)
        ReconnectingClientFactory.clientConnectionLost(self, connector, reason)

        
def start_send_invitation(host, port, auth_complete_cb, secret_message,
        public_seed, on_close, local_hostid, local_oauth):
    """Instantiate the factory to hold configuration data about sending an
    invitation and let the reactor add it to its event-handling loop by way of
    starting a TCP connection."""
    factory = SendInvitationFactory(auth_complete_cb, secret_message,
            public_seed, on_close, local_hostid, local_oauth)
    reactor.connectTCP(host, port, factory)

    return factory
