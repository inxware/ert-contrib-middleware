# Copyright 2010 Canonical Ltd.
#
# This file is part of desktopcouch.
#
# desktopcouch is free software: you can redistribute it and/or modify
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
# Authors: Manuel de la pena <manuel.delapena@canonical.com>

import logging
from desktopcouch.pair.couchdb_pairing.couchdb_io import (
    put_static_paired_service)
from desktopcouch.records.server import CouchDatabase

U1_PAIR_RECORD = "ubuntu_one_pair_record"
MAP_JS = """function(doc) {
    if (doc.service_name == "ubuntuone") {
        if (doc.application_annotations &&
            doc.application_annotations["Ubuntu One"] &&
            doc.application_annotations["Ubuntu One"]["%s"] &&
            doc.application_annotations["Ubuntu One"]["%s"]["deleted"]) {
                emit(doc._id, 1);
            } else {
                emit(doc._id, 0)
            }
    }
}""" % ("private_application_annotations", "private_application_annotations")

def pair_with_ubuntuone(management_db=None):
    """Adds a pairing record with ubuntu one when is needed."""
    if not management_db:
        management_db = CouchDatabase("management", create=True)
    # we indeed have credentials to add to the pairing records
    # but first we ensure that the required view is present
    if not management_db.view_exists(U1_PAIR_RECORD, U1_PAIR_RECORD):
        management_db.add_view(U1_PAIR_RECORD, MAP_JS, None, U1_PAIR_RECORD)
    view_results = management_db.execute_view(U1_PAIR_RECORD, U1_PAIR_RECORD)
    pairing_found = False
    # Results should contain either one row or no rows
    # If there is one row, its value will be 0, meaning that there is
    #   already an Ubuntu One pairing record, or 1, meaning that there
    #   was an Ubuntu One pairing record but it has since been unpaired
    # Only create a new record if there is not one already. Specifically,
    #   do not add the record if there is a deleted one, as this means
    #   that the user explicitly unpaired it!
    for row in view_results:
        pairing_found = True
        if row.value == 1:
            logging.debug("Not adding desktopcouch pairing since the user "
                "has explicitly unpaired with Ubuntu One")
        else:
            logging.debug("Not adding desktopcouch pairing since we are "
                "already paired")
    if not pairing_found:
        put_static_paired_service(None, "ubuntuone")
        logging.debug("Pairing desktopcouch with Ubuntu One")

