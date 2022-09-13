# Copyright 2010 Canonical Ltd.
#
# This file is part of desktopcouch-bookmarks.
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
#
# Authors: Zachery Bir <zachery.bir@canonical.com>
#          Joshua Blount <joshua.blount@canonical.com>


"""A dictionary based bookmark record representation."""

from desktopcouch.records.record import Record

BOOKMARK_RECORD_TYPE = 'http://www.freedesktop.org/wiki/Specifications/desktopcouch/bookmark'
FOLDER_RECORD_TYPE = 'http://www.freedesktop.org/wiki/Specifications/desktopcouch/folder'
SEPARATOR_RECORD_TYPE = 'http://www.freedesktop.org/wiki/Specifications/desktopcouch/separator'
FEED_RECORD_TYPE = 'http://www.freedesktop.org/wiki/Specifications/desktopcouch/feed'

class Bookmark(Record):
    """An Ubuntuone Bookmark Record."""

    def __init__(self, data=None, record_id=None):
        super(Bookmark, self).__init__(
            record_id=record_id, data=data,
            record_type=BOOKMARK_RECORD_TYPE)

class Folder(Record):
    """An Ubuntuone Folder Record."""

    def __init__(self, data=None, record_id=None):
        super(Folder, self).__init__(
            record_id=record_id, data=data,
            record_type=FOLDER_RECORD_TYPE)

class Separator(Record):
    """An Ubuntuone Separator Record."""

    def __init__(self, data=None, record_id=None):
        super(Separator, self).__init__(
            record_id=record_id, data=data,
            record_type=SEPARATOR_RECORD_TYPE)

class Feed(Record):
    """An Ubuntuone Feed Record."""

    def __init__(self, data=None, record_id=None):
        super(Feed, self).__init__(
            record_id=record_id, data=data,
            record_type=FEED_RECORD_TYPE)
