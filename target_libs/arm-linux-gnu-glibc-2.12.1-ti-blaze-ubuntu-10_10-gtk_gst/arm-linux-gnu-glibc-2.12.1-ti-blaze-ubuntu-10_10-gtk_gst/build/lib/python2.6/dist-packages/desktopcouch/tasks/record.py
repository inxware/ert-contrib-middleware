# Copyright 2010 Canonical Ltd.
#
# This file is part of desktopcouch-tasks.
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


"""A dictionary based task record representation."""

from desktopcouch.records.record import Record

TASK_RECORD_TYPE = 'http://www.freedesktop.org/wiki/Specifications/desktopcouch/task'
# keep in sync with the above tasks record type
FIELD_NAMES = (
    'summary'
)

class TaskBase(Record):
    """
    A base for Task records.

    Use make_task_class to create the Note class with the required fields.
    """

    def __init__(self, data=None, record_id=None):
        super(TaskBase, self).__init__(
            record_id=record_id, data=data, record_type=TASK_RECORD_TYPE)

def make_task_class(field_names):
    """Task class factory function. field_names is a list of strings."""
    TaskClass = type('Task', (TaskBase,), {})
    for field_name in field_names:

        def fget(self, _field_name=field_name):
            return self.get(_field_name)

        def fset(self, value, _field_name=field_name):
            self[_field_name] = value

        setattr(TaskClass, field_name, property(fget, fset))
    return TaskClass

Task = make_task_class(FIELD_NAMES)
