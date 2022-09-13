# Copyright 2009-2010 Canonical Ltd.
#
# This file is part of desktopcouch-contacts.
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
# Authors: Eric Casteleijn <eric.casteleijn@canonical.com>
#          Nicola Larosa <nicola.larosa@canonical.com>
#          Mark G. Saye <mark.saye@canonical.com>

"""A dictionary based contact record representation."""

from desktopcouch.records.record import Record

CONTACT_RECORD_TYPE = (
    'http://www.freedesktop.org/wiki/Specifications/desktopcouch/contact')
# keep in sync with the above contacts record type
SINGLE_FIELDS = frozenset((
    'first_name', 'middle_name', 'last_name', 'title', 'suffix', 'birth_date',
    'nick_name', 'spouse_name', 'wedding_date', 'company', 'department',
    'job_title', 'manager_name', 'assistant_name', 'office',
))
LIST_FIELDS = frozenset((
    'addresses', 'phone_numbers', 'email_addresses', 'urls', 'im_addresses',
))
ALL_FIELDS = SINGLE_FIELDS | LIST_FIELDS


class ContactBase(Record):
    """
    A base for Contact records.

    Use make_contact_class to create the Contact class with the required fields.
    """

    def __init__(self, data=None, record_id=None):
        super(ContactBase, self).__init__(
            record_id=record_id, data=data, record_type=CONTACT_RECORD_TYPE)


def make_contact_class(field_names):
    """Contact class factory function. field_names is a strings iterable."""
    ContactClass = type('Contact', (ContactBase,), {})
    for field_name in field_names:

        # without the _field_name param, only the last one gets used
        def fget(self, _field_name=field_name):
            return self.get(_field_name)

        def fset(self, value, _field_name=field_name):
            self[_field_name] = value

        setattr(ContactClass, field_name, property(fget, fset))
    return ContactClass


Contact = make_contact_class(ALL_FIELDS)
