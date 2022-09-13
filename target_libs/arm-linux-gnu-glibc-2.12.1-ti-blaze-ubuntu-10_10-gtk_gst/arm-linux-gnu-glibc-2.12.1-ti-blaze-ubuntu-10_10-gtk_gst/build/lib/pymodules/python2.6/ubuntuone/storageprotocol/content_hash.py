# ubuntuone.storageprotocol.content_hash - content hash handling
#
# Author: Lucio Torre <lucio.torre@canonical.com>
#         Natalia B. Bidart <natalia.bidart@canonical.com>
#
# Copyright 2009-2010 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranties of
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Hash Handling Stuffs."""

import copy
import hashlib
import zlib


class ContentHash(object):
    """Encapsulates the generation of content hashes.

    We cant subclass openssl hash classes, so we do some
    composition to get similar methods.

    """
    method = None
    method_name = ""
    def __init__(self):
        self.hash_object = self.method()

    digest_size = property(lambda self: self.hash_object.digest_size)
    block_size = property(lambda self: self.hash_object.block_size)
    update = property(lambda self: self.hash_object.update)
    digest = property(lambda self: self.hash_object.digest)
    hexdigest = property(lambda self: self.hash_object.hexdigest)

    def copy(self):
        """Copies the generated hash."""
        cp = copy.copy(self)
        cp.hash_object = self.hash_object.copy()
        return cp

    def content_hash(self):
        """Adds hex digest to content hash."""
        return self.method_name +":"+ self.hash_object.hexdigest()

class SHA1ContentHash(ContentHash):
    """Generates SHA1 of ConentHash."""

    method = hashlib.sha1
    method_name = "sha1"

# we can change this variable to change the method
content_hash_factory = SHA1ContentHash

def crc32(data, previous_crc32=0):
    """A correct crc32 function.

    Always returns positive values.

    """
    return zlib.crc32(data, previous_crc32) & 0xFFFFFFFFL

