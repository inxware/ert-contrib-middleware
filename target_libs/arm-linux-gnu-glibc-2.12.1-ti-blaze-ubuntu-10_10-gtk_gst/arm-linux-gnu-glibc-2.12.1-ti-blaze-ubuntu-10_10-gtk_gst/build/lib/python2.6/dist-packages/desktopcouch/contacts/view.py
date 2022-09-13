# Copyright 2010 Canonical Ltd.
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
# Authors: Chad Miller <chad.miller@canonical.com>

from record import CONTACT_RECORD_TYPE

__all__ = [ "find_contacts_exact", "find_contacts_starting", ]


view_map_prefixed_fields = """
    function(doc) {
        function emit_dict(prefix, dict) {
            for (var k in dict) {
                var v = dict[k];
                if (v == undefined) 
                    continue;

                if (v.substring) {
                    emit(prefix+k+":"+v, null);
                    switch (k) {   // Weird cases that may be useful.
                        case "birth_date": emit(prefix+k+":"+v.substring(v.indexOf("-")), null); break; // drop year
                    }

                } else {
                    for (subk in v)
                        emit_dict(prefix+k, v[subk])
                }
            }
        }

        try {
            if (doc['application_annotations']['Ubuntu One']
                    ['private_application_annotations']['deleted'])
                return;
        } catch (e) {
            // nothing
        }

        if (doc['record_type'] != '%(CONTACT_RECORD_TYPE)s')
        {
            return;
        }

        emit_dict("", doc);
    }
""" % locals()


def _cur_find_contacts(db, include_docs):
    name = "contacts_all_fields_prefixed"
    if not db.view_exists(name, "contacts"):
        db.add_view(name, view_map_prefixed_fields, None, "contacts")

    viewdata = db.execute_view(name, "contacts", include_docs=include_docs)
    return viewdata

def find_contacts_exact(db, include_docs=False, **kwargs):
    if len(kwargs) != 1:
        raise ValueError("expected exactly one keyword")
    pair = kwargs.popitem()
    return _cur_find_contacts(db, include_docs)["%s:%s" % pair]

def find_contacts_starting(db, include_docs=False, **kwargs):
    if len(kwargs) != 1:
        raise ValueError("expected exactly one keyword")
    pair = kwargs.popitem()
    return _cur_find_contacts(db, include_docs)[u"%s:%s" % pair : u"%s:%s\u9999\u9999\u9999\u9999" % pair]

