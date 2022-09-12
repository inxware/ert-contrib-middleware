# ubuntuone.syncdaemon.interfaces - ActionQueue interface
#
# Authors: John Lenton <john.lenton@canonical.com>
#          Lucio Torre <lucio.torre@canonical.com>
#
# Copyright 2009 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranties of
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
This is the interface of the ActionQueue
"""

from zope.interface import Interface, Attribute

# pylint: disable-msg=W0232,E0213,E0211

class IContentQueue(Interface):
    """
    The content queue is the part of access queue that manages uploads
    and downloads of content.
    """

    downloading = Attribute("The set of active downloads")
    uploading = Attribute("The set of active uploads")

    def cancel_download(share_id, node_id):
        """
        Try to cancel any download for the given node.

        The return value is whether we've been able to cancel a
        download.
        """

    def cancel_upload(share_id, node_id):
        """
        Try to cancel any upload for the given node.

        The return value is whether we're sure that we've been able to
        cancel an upload. We might succeed without knowing it,
        however.
        """

    def download(share_id, node_id, server_hash, fileobj):
        """
        Go get the content for the given node and dump it into
        file-like object fileobj.
        """

    def upload(share_id, node_id, previous_hash, hash, crc32,
               size, deflated_size, fileobj):
        """
        Put the content of file-like object fileobj up in the given
        node.
        """

class IMetaQueue(Interface):
    """
    The MetaQueue is the part of AccessQueue that manages transfers of
    metadata.
    """

    def make_file(share_id, parent_id, name, marker):
        """
        Ask the server to create a file called name in the given
        parent; and use marker as a marker in the ensuing
        notification.
        """

    def make_dir(share_id, parent_id, name, marker):
        """
        Ask the server to make a directory called name in the given
        parent, and use marker as a marker in the ensuing
        notification.
        """

    def move(share_id, node_id, old_parent_id, new_parent_id, new_name):
        """
        Ask the server to move a node to the given parent and name.
        """

    def unlink(share_id, parent_id, node_id):
        """
        Unlink the given node.
        """

    def list_shares():
        """
        Get a list of the shares, and put the result on the event queue.
        """

    def create_share(node, share_to, name, access_level):
        """
        Ask the server to create a share.
        """

    def inquire_free_space(share_id):
        """
        Inquire after free space in the given volume and put the result on
        the event queue.
        """

    def inquire_account_info():
        """Ask the state of the user's account (purchased space, etc)."""

class IActionQueue(IContentQueue, IMetaQueue):
    """
    The access queue itself.
    """

    def connect(host, port, use_ssl=False):
        """Open a (possibly SSL) connection to the API server on (host, port).

        Once you've connected, authenticate.
        """

    def disconnect():
        """Close the connection."""

class IMarker(Interface):
    """
    A marker interface for telling server uuids apart from markers.
    """
