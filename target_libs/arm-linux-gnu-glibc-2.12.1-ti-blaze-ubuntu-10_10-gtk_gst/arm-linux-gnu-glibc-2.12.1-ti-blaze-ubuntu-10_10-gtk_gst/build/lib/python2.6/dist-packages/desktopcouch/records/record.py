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
#          Nicola Larosa <nicola.larosa@canonical.com>
#          Mark G. Saye <mark.saye@canonical.com>

"""A dictionary based record representation."""

import re
import uuid

RX = re.compile('[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}')

def is_internal(key):
    """Test whether this is an internal key."""
    return key.startswith(('_', 'application_annotations'))

def is_uuid_dict(dictionary):
    """Test whether the passed object is a dictionary like object with
    uuids for keys.
    """
    return dictionary and not [
        key for key in dictionary if (not RX.match(key) and key != '_order')]

def record_factory(data):
    """Create a RecordDict or RecordList from passed data."""
    if isinstance(data, dict):
        return _build_from_dict(data)
    if isinstance(data, (list, tuple)):
        return _build_from_list(data)
    return data

def _build_from_list(data):
    """Create a RecordList from passed data."""
    if not data:
        raise ValueError("Can't set empty list values.'")
    result = MergeableList({})
    for value in data:
        result.append(record_factory(value))
    return result

def _build_from_dict(data):
    """Create a RecordDict from passed data."""
    result = RecordDict({})
    for key, value in data.items():
        if RX.match(key):
            raise IllegalKeyException(
                "Can't set '%s'. uuid-like keys are not allowed." % key)
        result[key] = record_factory(value)
    return result

def validate(data):
    """Validate underlying data for Record objects."""
    if isinstance(data, dict):
        if is_uuid_dict(data):
            return
        if any(RX.match(key) for key in data):
            raise IllegalKeyException(
                'Mixing uuid-like keys and regular keys in a '
                'single dictionary is not allowed.')
        for value in data.values():
            validate(value)

class IllegalKeyException(Exception):
    """Exception when attempting to set a key we don't allow."""
    pass

class NoRecordTypeSpecified(Exception):
    """Exception when creating a record without specifying record type"""
    def __str__(self):
        return "Record type must be specified and should be a URL"


# XXX: the doc string for this class makes no sense at all.

class Attachment(object):
    """This represents the value of the attachment.  The name is
    stored as the key that points to this name, and the

    We have a new BLOB to attach.  Attachment(file_object, str_mime_type)
    We know the traits of a BLOB.  Attachment()
    """
    def __init__(self, content=None, content_type=None):
        super(Attachment, self).__init__()
        if content is not None:
            if hasattr(content, "read") and hasattr(content, "seek"):
                pass  # is a file-like object.
            elif isinstance(content, basestring):
                pass  # is a string-like object.
            else:
                raise TypeError("Expected string or file (or None) as content.")

        self.content = content
        self.content_type = content_type
        self.needs_synch = content is not None

    def get_content_and_type(self):
        assert self.content is not None
        if hasattr(self.content, "read"):
            if hasattr(self.content, "seek"):
                self.content.seek(0, 0)
            return self.content.read(), self.content_type
        else:
            return self.content, self.content_type


class RecordData(object):
    """Base class for all the Record data objects."""

    def __init__(self, data):
        validate(data)
        self._data = data
        self._attachments = dict()
        self._detached = set()

    def __getitem__(self, key):
        value = self._data[key]
        if isinstance(value, dict):
            if is_uuid_dict(value):
                result = MergeableList(value)
            else:
                result = RecordDict(value)
            return result
        return value

    def __setitem__(self, key, item):
        if isinstance(item, (list, tuple, dict)):
            item = record_factory(item)
        if hasattr(item, '_data'):
            item = item._data
        self._data[key] = item

    def __delitem__(self, key):
        del self._data[key]

    def __len__(self):
        return len(self._data)

    def attach_by_reference(self, filename, getter_function):
        """This should only be called by the server code to refer to a BLOB
        that is in the database, so that we do not have to download it until
        the user wants it."""
        a = Attachment(None, None)
        a.get_content_and_type = getter_function
        self._attachments[filename] = a

    def attach(self, content, filename, content_type):
        """Attach a file-like or string-like object, with a particular name and
        content type, to a document to be stored in the database.  One can not
        clobber names that already exist."""
        if filename in self._attachments:
            raise KeyError("%r already exists" % (filename,))
        self._attachments[filename] = Attachment(content, content_type)

    def detach(self, filename):
        """Remove a BLOB attached to a document."""
        try:
            self._attachments.pop(filename)
            self._detached.add(filename)
        except KeyError:
            raise KeyError("%r is not attached to this document" % (filename,))

    def list_attachments(self):
        return self._attachments.keys()

    def attachment_data(self, filename):
        """Retreive the attached data, the BLOB and content_type."""
        try:
            a = self._attachments[filename]
        except KeyError:
            raise KeyError("%r is not attached to this document" % (filename,))
        return a.get_content_and_type()

class RecordDict(RecordData):
    """An object that represents an desktop CouchDB record fragment,
    but behaves like a dictionary.
    """

    def __setitem__(self, key, item):
        if RX.match(key):
            raise IllegalKeyException(
                "Can't set '%s'. uuid-like keys are not allowed." % key)
        super(RecordDict, self).__setitem__(key, item)

    def __iter__(self):
        for key in self._data:
            yield key

    def __contains__(self, key):
        return key in self._data

    def __cmp__(self, value):
        if isinstance(value, RecordDict):
            return cmp(self._data, value._data)
        return -1

    def get(self, key, default=None):
        """Override get method."""
        if not key in self._data:
            return default
        return self[key]

    def update(self, data):
        """Set items from a dict"""
        for key, val in data.items():
            self[key] = val

    def items(self):
        """Return all items, even private or internal ones."""
        return self.all_items()

    def all_items(self):
        """Return all items, even private or internal ones."""
        data = []
        for key, value in self._data.items():
            data.append((key, value))
        return data

    def public_items(self):
        """Return items that are NOT internal or private structures."""
        return [(key, self[key]) for key in self]

    def keys(self):
        """Return all keys."""
        return self._data.keys()

    def setdefault(self, key, default):
        """remap setdefault"""
        if not key in self:
            self[key] = default
        return self[key]

    def has_key(self, key):
        return key in self



class MergeableList(RecordData):
    """An object that represents a list of complex values."""

    def __getitem__(self, index):
        if not isinstance(index, int):
            raise TypeError(
                "list indices must be integers, not %s" % type(index))
        key = self.get_uuid_for_index(index)
        return super(MergeableList, self).__getitem__(key)

    def __setitem__(self, index, item):
        if not isinstance(index, int):
            raise TypeError(
                "list indices must be integers, not %s" % type(index))
        key = self.get_uuid_for_index(index)
        super(MergeableList, self).__setitem__(key, item)

    def __delitem__(self, index):
        if not isinstance(index, int):
            raise TypeError(
                "list indices must be integers, not %s" % type(index))
        key = self.get_uuid_for_index(index)
        if key in self._data.get('_order', []):
            self._data['_order'].remove(key)
        super(MergeableList, self).__delitem__(key)

    def __iter__(self):
        return (self[index] for index in range(len(self)))

    def __len__(self):
        length = len(self._data)
        if '_order' in self._data:
            return length - 1
        return length

    def __contains__(self, value):
        if hasattr(value, '_data'):
            value = value._data
        return value in self._data.values()

    def __cmp__(self, value):
        """
        Implement the compare to be able to compare with other mergeable
        lists, tuples and lists.
        """
        if (hasattr(value, "__iter__")
            and hasattr(value, "__getitem__")
            and hasattr(value, "__len__")):
            # we should have the same value in the same order
            length  = len(value)
            if len(self) == length:
                for index, current_value in enumerate(self):
                    cmp_value = cmp(self[index], value[index])
                    if cmp_value != 0:
                        return cmp_value
                return 0
        return -1

    def _get_ordered_keys(self):
        """Get list of uuid keys ordered by 'order' property or uuid key."""
        result = []
        order = self._data.get('_order', [])
        keys = sorted([key for key in self._data if key != '_order'])
        for key in order:
            keys.remove(key)
            result.append(key)
        result.extend(keys)
        return result

    def get_uuid_for_index(self, index):
        """Return uuid key for index."""
        return self._get_ordered_keys()[index]

    def get_value_for_uuid(self, uuid):
        """Allow API consumers that do know about the UUIDs to get
        values directly.
        """
        return super(MergeableList, self).__getitem__(uuid)

    def append(self, value):
        """Append a value to end of MergeableList."""
        new_uuid = str(uuid.uuid4())
        self._data.setdefault('_order', sorted(self._data.keys())).append(
            new_uuid)
        super(MergeableList, self).__setitem__(new_uuid, value)

    def remove(self, value):
        if len(self) == 1:
            raise ValueError("MergeableList cannot be empty.")
        index = 0
        for current_value in self:
            # important! use the data in self first 'cause mergeable lists
            # can be compared with list and tuples but no the other way around
            if cmp(current_value, value) == 0:
                del self[index]
                return
            index += 1
        raise ValueError("list.remove(x): x not in list")

    def pop(self, index):
        if len(self) == 1:
            raise ValueError("MergeableList cannot be empty.")
        value = self[index]
        del self[index]
        return value

    def index(self, key):
        """Get value by index."""
        return self.__getitem__(key)


class Record(RecordDict):
    """An object that represents a Desktop CouchDB record."""

    def __init__(self, data=None, record_type=None, record_id=None):
        if data is None:
            data = {}
        if record_type is None:
            if 'record_type' in data:
                record_type = data['record_type']
            else:
                raise NoRecordTypeSpecified

        super(Record, self).__init__(data)
        self._data['record_type'] = record_type

        if record_id is not None:
            # Explicit setting, so try to use it.
            if data.get('_id', None) is None:
                # Overwrite _id if it is None.
                self._data['_id'] = record_id
            else:
                # _id already set.  Verify that the explicit value agrees.
                if self._data['_id'] != record_id:
                    raise ValueError("record_id doesn't match value in data.")

    def __getitem__(self, key):
        if is_internal(key):
            raise KeyError(key)
        return super(Record, self).__getitem__(key)

    def __setitem__(self, key, item):
        if is_internal(key):
            raise IllegalKeyException(
                "Can't set '%s'. This is an internal key." % key)
        super(Record, self).__setitem__(key, item)

    def __iter__(self):
        return (
            key for key in super(Record, self).__iter__() if not is_internal(
                key))

    def __contains__(self, key):
        if is_internal(key):
            return False
        return super(Record, self).__contains__(key)

    def __delitem__(self, key):
        if is_internal(key):
            raise KeyError(key)
        super(Record, self).__delitem__(key)

    def get(self, key, default=None):
        """Get a value from the record by key."""
        if is_internal(key):
            return default
        return super(Record, self).get(key, default)

    def keys(self):
        """Return all keys"""
        return [
            key for key in super(Record, self).keys() if not is_internal(key)]

    def _get_record_id(self):
        """Gets the record id."""
        if '_id' in self._data:
            return self._data['_id']
        return None

    def _set_record_id(self, value):
        """Sets the record id."""
        self._data['_id'] = value

    def _set_record_revision(self, value):
        """Sets the record revision."""
        self._data['_rev'] = value

    record_id = property(_get_record_id, _set_record_id)

    @property
    def application_annotations(self):
        """Get the section with application specific data."""
        return RecordDict(self._data.setdefault('application_annotations', {}))

    @property
    def record_type(self):
        """Get the record type."""
        return self._data.setdefault('record_type', None)  # Set if unset.

    @property
    def record_revision(self):
        """Get the record revision."""
        return self._data.get('_rev', None)  # Retreive only; comes from DB.
