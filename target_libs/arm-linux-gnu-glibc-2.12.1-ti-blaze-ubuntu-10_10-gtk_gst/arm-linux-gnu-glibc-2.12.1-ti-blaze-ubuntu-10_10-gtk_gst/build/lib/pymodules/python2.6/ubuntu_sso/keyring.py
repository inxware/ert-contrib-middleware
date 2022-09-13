# Copyright (C) 2010 Canonical
#
# Authors:
#  Andrew Higginson
#  Alejandro J. Cura <alecu@canonical.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 3.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

"""Handle keys in the gnome kerying."""

import socket
import urllib
import urlparse

import gnomekeyring

from ubuntu_sso.logger import setup_logging


logger = setup_logging("ubuntu_sso.keyring")
TOKEN_SEPARATOR = ' @ '
SEPARATOR_REPLACEMENT = ' AT '

U1_APP_NAME = "Ubuntu One"
U1_KEY_NAME = "UbuntuOne token for https://ubuntuone.com"
U1_KEY_ATTR = {
    "oauth-consumer-key": "ubuntuone",
    "ubuntuone-realm": "https://ubuntuone.com",
}


def get_old_token_name(app_name):
    """Build the token name (old style)."""
    quoted_app_name = urllib.quote(app_name)
    computer_name = socket.gethostname()
    quoted_computer_name = urllib.quote(computer_name)
    return "%s - %s" % (quoted_app_name, quoted_computer_name)


def get_token_name(app_name):
    """Build the token name."""
    computer_name = socket.gethostname()
    computer_name = computer_name.replace(TOKEN_SEPARATOR,
                                          SEPARATOR_REPLACEMENT)
    return TOKEN_SEPARATOR.join((app_name, computer_name)).encode('utf-8')


class Keyring(object):
    """A Keyring for a given application name."""

    def __init__(self, app_name):
        """Initialize this instance given the app_name."""
        if not gnomekeyring.is_available():
            raise gnomekeyring.NoKeyringDaemonError
        self.app_name = app_name
        self.token_name = get_token_name(self.app_name)

    def _find_keyring_item(self, attr=None):
        """Return the keyring item or None if not found."""
        if attr is None:
            attr = self._get_keyring_attr()
        try:
            items = gnomekeyring.find_items_sync(
                                        gnomekeyring.ITEM_GENERIC_SECRET,
                                        attr)
        except gnomekeyring.NoMatchError:
            # if no items found, return None
            return None

        # we priorize the item in the "login" keyring
        for item in items:
            if item.keyring == "login":
                return item

        # if not on the "login" keyring, we return the first item
        return items[0]

    def _get_keyring_attr(self):
        """Build the keyring attributes for this credentials."""
        attr = {"key-type": "Ubuntu SSO credentials",
                "token-name": self.token_name}
        return attr

    def set_ubuntusso_attr(self, cred):
        """Set the credentials of the Ubuntu SSO item."""
        # Creates the secret from the credentials
        secret = urllib.urlencode(cred)

        # Add our SSO credentials to the keyring
        gnomekeyring.item_create_sync(None, gnomekeyring.ITEM_GENERIC_SECRET,
                        self.app_name, self._get_keyring_attr(), secret, True)

    def _migrate_old_token_name(self):
        """Migrate credentials with old name, store them with new name."""
        attr = self._get_keyring_attr()
        attr['token-name'] = get_old_token_name(self.app_name)
        item = self._find_keyring_item(attr=attr)
        if item is not None:
            self.set_ubuntusso_attr(dict(urlparse.parse_qsl(item.secret)))
            gnomekeyring.item_delete_sync(item.keyring, item.item_id)

        return self._find_keyring_item()

    def get_ubuntusso_attr(self):
        """Return the secret of the SSO item in a dictionary."""
        # If we have no attributes, return None
        item = self._find_keyring_item()
        if item is None:
            item = self._migrate_old_token_name()

        if item is not None:
            return dict(urlparse.parse_qsl(item.secret))
        else:
            # if no item found, try getting the old credentials
            if self.app_name == U1_APP_NAME:
                return try_old_credentials(self.app_name)
        # nothing was found
        return None

    def delete_ubuntusso_attr(self):
        """Delete a set of credentials from the keyring."""
        item = self._find_keyring_item()
        if item is not None:
            gnomekeyring.item_delete_sync(item.keyring, item.item_id)


class UbuntuOneOAuthKeyring(Keyring):
    """A particular Keyring for Ubuntu One."""

    def _get_keyring_attr(self):
        """Build the keyring attributes for this credentials."""
        return U1_KEY_ATTR


def try_old_credentials(app_name):
    """Try to get old U1 credentials and format them as new."""
    logger.debug('trying to get old credentials.')
    old_creds = UbuntuOneOAuthKeyring(U1_KEY_NAME).get_ubuntusso_attr()
    if old_creds is not None:
        # Old creds found, build a new credentials dict with them
        creds = {
            'consumer_key': "ubuntuone",
            'consumer_secret': "hammertime",
            'name': U1_KEY_NAME,
            'token': old_creds["oauth_token"],
            'token_secret': old_creds["oauth_token_secret"],
        }
        logger.debug('found old credentials')
        return creds
    logger.debug('try_old_credentials: No old credentials for this app.')


if __name__ == "__main__":
    # pylint: disable=C0103

    kr1 = Keyring("Test key 1")
    kr2 = Keyring("Test key 2")

    kr1.delete_ubuntusso_attr()
    kr2.delete_ubuntusso_attr()

    d1 = {"name": "test-1", "ha": "hehddddeff", "hi": "hggehes", "ho": "he"}
    d2 = {"name": "test-2", "hi": "ho", "let's": "go"}

    kr1.set_ubuntusso_attr(d1)
    kr2.set_ubuntusso_attr(d2)

    r1 = kr1.get_ubuntusso_attr()
    r2 = kr2.get_ubuntusso_attr()

    assert r1 == d1
    assert r2 == d2
