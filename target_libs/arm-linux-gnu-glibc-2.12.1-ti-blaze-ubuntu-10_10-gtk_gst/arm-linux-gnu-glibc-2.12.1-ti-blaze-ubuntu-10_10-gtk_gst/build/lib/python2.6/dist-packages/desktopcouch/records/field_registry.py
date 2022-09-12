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
#
# Authors: Eric Casteleijn <eric.casteleijn@canonical.com>
#          Vincenzo Di Somma <vincenzo.di.somma@canonical.com>

"""Registry utility classes"""

import copy

ANNOTATION_NAMESPACE = 'application_annotations'


class Transformer(object):
    """Two way transformation of between Records and app specific
    data.
    """

    def __init__(self, app_name, field_registry):
        self.app_name = app_name
        self.field_registry = field_registry

    def from_app(self, data, record):
        """Transform from application data to record data."""
        for key, val in data.items():
            if key in self.field_registry:
                self.field_registry[key].setValue(record, val)
            else:
                record.application_annotations.setdefault(
                    self.app_name, {}).setdefault(
                    'application_fields', {})[key] = val

    def to_app(self, record, data):
        """Transform from record data to application data."""
        annotations = record.application_annotations.get(self.app_name, {}).get(
            'application_fields', {})
        for key, value in annotations.items():
            data[key] = value
        for key in self.field_registry:
            data[key] = self.field_registry[key].getValue(record)


class SimpleFieldMapping(object):
    """Simple field mapping of fieldname to fieldname."""

    def __init__(self, fieldname):
        """initialize the fieldname"""
        self._fieldname = fieldname

    def getValue(self, record):
        """get the value for the registered field"""
        return record.get(self._fieldname)

    def deleteValue(self, record):
        """Delete a value"""
        if self._fieldname in record:
            del record[self._fieldname]

    def setValue(self, record, value):
        """set the value for the registered field"""
        if value is None:
            self.deleteValue(record)
            return
        record[self._fieldname] = value


class MergeableListFieldMapping(object):
    """Mapping between MergeableLists and application fields."""

    def __init__(
        self, app_name, uuid_field, root_list, field_name, default_values=None):
        """initialize the default values"""
        self._app_name = app_name
        self._uuid_field = uuid_field
        self._root_list = root_list
        self._field_name = field_name
        self._default_values = default_values or {}

    def get_application_annotations(self, record):
        """get the application config"""
        annotation = record.application_annotations.setdefault(
            self._app_name, {})
        private = annotation.setdefault('private_application_annotations', {})
        return private

    def _uuidLookup(self, record):
        """return the uuid key to lookup the registered field"""
        return self.get_application_annotations(record).get(self._uuid_field)

    def getValue(self, record):
        """get the value for the registered field"""
        uuid_key = self._uuidLookup(record)
        try:
            return record[
                self._root_list].get_value_for_uuid(uuid_key).get(
                self._field_name)
        except KeyError:
            return None

    def deleteValue(self, record):
        """Delete a value"""
        root_list = record.get(self._root_list, None)
        if not root_list:
            return
        uuid_key = self._uuidLookup(record)
        if not uuid_key:
            return
        if self._field_name in root_list._data.get(uuid_key, []):
             del root_list._data[uuid_key][self._field_name]

    def setValue(self, record, value):
        """set the value for the registered field"""
        if not value:
            self.deleteValue(record)
            return
        root_list = record.get(self._root_list, None)
        application_annotations = self.get_application_annotations(record)
        values = copy.deepcopy(self._default_values)
        values.update({self._field_name: value})
        if not root_list:
            # This is the first value we add to the list
            record[self._root_list] = [values]
            uuid_key = record[self._root_list].get_uuid_for_index(-1)
            application_annotations[self._uuid_field] = uuid_key
            return
        uuid_key = self._uuidLookup(record)
        if not uuid_key:
            # We add a new value to an existing list
            root_list.append(values)
            uuid_key = root_list.get_uuid_for_index(-1)
            application_annotations[self._uuid_field] = uuid_key
            return
        try:
            # Look up the mapped value in the list
            record_dict = root_list.get_value_for_uuid(uuid_key)
        except KeyError:
            # The list no longer has the mapped value, add it anew
            root_list.append(values)
            uuid_key = root_list.get_uuid_for_index(-1)
            application_annotations[self._uuid_field] = uuid_key
            return
        record_dict[self._field_name] = value


