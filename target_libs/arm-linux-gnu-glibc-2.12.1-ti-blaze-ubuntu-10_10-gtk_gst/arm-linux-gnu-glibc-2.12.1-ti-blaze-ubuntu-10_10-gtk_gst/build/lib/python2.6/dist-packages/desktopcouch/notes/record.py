# Copyright 2009-2010 Canonical Ltd.
#
# This file is part of desktopcouch-notes.
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
# Authors: Rodrigo Moya <rodrigo.moya@canonical.com>

"""A dictionary based note record representation."""

from time import strptime
from desktopcouch.records.record import Record

NOTE_RECORD_TYPE = 'http://www.freedesktop.org/wiki/Specifications/desktopcouch/note'
# keep in sync with the above notes record type
FIELDS = {
    # String fields
    'note_format': 'string',
    'title': 'string',
    'content': 'string',
    # Date fields
    'last_change_date': 'date',
    'create_date': 'date'
}

class NoteBase(Record):
    """
    A base for Note records.

    Use make_note_class to create the Note class with the required fields.
    """

    def __init__(self, data=None, record_id=None):
        super(NoteBase, self).__init__(
            record_id=record_id, data=data, record_type=NOTE_RECORD_TYPE)

def make_note_class(fields):
    """Note class factory function. field_names is a list of strings."""
    NoteClass = type('Note', (NoteBase,), {})
    for field_name in fields:

        def fget(self, _field_name=field_name):
            return self.get(_field_name)

        def fset(self, value, _field_name=field_name):
            field_type = fields[_field_name]
            if field_type == 'date':
                # Check that it is a date with the correct format
                date_value = strptime(value, "%Y-%m-%dT%H:%M:%S")
                if date_value is None:
                    return
            self[_field_name] = value

        setattr(NoteClass, field_name, property(fget, fset))
    return NoteClass

Note = make_note_class(FIELDS)
