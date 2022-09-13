# Copyright 2009 Canonical Ltd.
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
# Authors: Chad Miller <chad.miller@canonical.com>
"""Communicate with CouchDB."""

import logging
import urllib
import socket
import uuid
import datetime
from itertools import cycle

from desktopcouch import find_port as desktopcouch_find_port, local_files
from desktopcouch.records import server
from desktopcouch.records.record import Record

RECTYPE_BASE = "http://www.freedesktop.org/wiki/Specifications/desktopcouch/"
PAIRED_SERVER_RECORD_TYPE = RECTYPE_BASE + "paired_server"
MY_ID_RECORD_TYPE = RECTYPE_BASE + "server_identity"

def obsfuscate(d):
    def maybe_hide(k, v):
        if hasattr(k, "endswith") and k.endswith("secret"):
            return "".join(rep for rep, vi in zip(cycle('Hidden'), v))
        else:
            return v

    if not hasattr(d, "iteritems"):
        return d
    return dict((k, maybe_hide(k, obsfuscate(v))) for k, v in d.iteritems())

def mkuri(hostname, port, has_ssl=False, path="", auth_pair=None):
    """Create a URI from parts."""
    protocol = "https" if has_ssl else "http"
    if auth_pair:
        auth = (":".join(map(urllib.quote, auth_pair)) + "@")
    else:
        auth = ""
    if (protocol, port) in (("http", 80), ("https", 443)):
        return "%s://%s%s/%s" % (protocol, auth, hostname, path)
    else:
        port = str(port)
        return "%s://%s%s:%s/%s" % (protocol, auth, hostname, port, path)

def _get_db(name, create=True, uri=None,
        ctx=local_files.DEFAULT_CONTEXT):
    """Get (and create?) a database."""
    return server.CouchDatabase(name, create=create, uri=uri, ctx=ctx)

def put_paired_host(oauth_data, uri=None,
        ctx=local_files.DEFAULT_CONTEXT, **kwargs):
    """Create a new paired-host record.  OAuth information is required, and
    after the uri, keyword parameters are added to the record."""
    pairing_id = str(uuid.uuid4())
    data = {
        "ctime": datetime.datetime.now().strftime("%Y-%m-%d %H:%M"),
        "record_type": PAIRED_SERVER_RECORD_TYPE,
        "pairing_identifier": pairing_id,
    }
    if oauth_data:
        data["oauth"] = {
            "consumer_key": str(oauth_data["consumer_key"]),
            "consumer_secret": str(oauth_data["consumer_secret"]),
            "token": str(oauth_data["token"]),
            "token_secret": str(oauth_data["token_secret"]),
        }
    data.update(kwargs)
    d = _get_db("management", uri=uri, ctx=ctx)
    r = Record(data)
    record_id = d.put_record(r)
    return record_id

def put_static_paired_service(oauth_data, service_name, uri=None,
        ctx=local_files.DEFAULT_CONTEXT):
    """Create a new service record."""
    return put_paired_host(oauth_data, uri=uri, service_name=service_name,
            pull_from_server=True, push_to_server=True, ctx=ctx)

def put_dynamic_paired_host(hostname, remote_uuid, oauth_data, uri=None,
        ctx=local_files.DEFAULT_CONTEXT):
    """Create a new dynamic-host record."""
    return put_paired_host(oauth_data, uri=uri, pairing_identifier=remote_uuid,
            push_to_server=True, server=hostname, ctx=ctx)

def get_static_paired_hosts(uri=None,
        ctx=local_files.DEFAULT_CONTEXT, port=None):
    """Retreive a list of static hosts' information in the form of
    (ID, service name, to_push, to_pull) ."""
    if not uri and port is not None:
        uri = "http://localhost:%d/" % (port,)
    db = _get_db("management", uri=uri, ctx=ctx)
    results = db.get_records(create_view=True)
    found = dict()
    for row in results[PAIRED_SERVER_RECORD_TYPE]:
        try:
            if row.value["service_name"] != "":
                pairing_id = row.value["pairing_identifier"]
                to_push = row.value.get("push_to_server", True)
                to_pull = row.value.get("pull_from_server", False)
                service_name = row.value["service_name"]
                found[service_name] = pairing_id, to_pull, to_push
        except KeyError, e:
            pass
    unique_hosts = [(v1, sn, v2, v3) for
            (sn), (v1, v2, v3) in found.items()]
    logging.debug("static pairings are %s", unique_hosts)
    return unique_hosts

def get_database_names_replicatable(uri, oauth_tokens=None, service=False,
        ctx=local_files.DEFAULT_CONTEXT, user_id=None):
    """Find a list of local databases, minus dbs that we do not want to
    replicate (explicitly or implicitly)."""
    if not uri:
        port = desktopcouch_find_port(ctx=ctx)
        uri = "http://localhost:%s" % port
    couchdb_server = server.OAuthCapableServer(uri, oauth_tokens=oauth_tokens,
            ctx=ctx)
    try:
        if user_id is None:
            all_dbs = set([db_name for db_name in couchdb_server])
            logging.debug(
                "Got list of databases from %s:\n    %s",
                couchdb_server, all_dbs)
        else:
            all_dbs = set(couchdb_server.resource.get(
                '/_all_dbs', user_id=user_id)[1])
            logging.debug(
                "Got list of databases from %s/_all_dbs?user_id=%s:\n    %s",
                couchdb_server, user_id, all_dbs)

    except socket.error, e:
        logging.error("Can't get list of databases from %s", couchdb_server)
        return set()

    excluded = set()
    excluded.add("management")
    excluded.add("users")
    if not service:
        excluded_msets = _get_management_data(PAIRED_SERVER_RECORD_TYPE,
                "excluded_names", uri=uri, ctx=ctx)
        for excluded_mset in excluded_msets:
            excluded.update(excluded_mset)

    return all_dbs - excluded

def get_my_host_unique_id(uri=None, create=True,
        ctx=local_files.DEFAULT_CONTEXT):
    """Returns a list of ids we call ourselves.  We complain in the log if it's
    more than one, but it's really no error.  If there are zero (id est, we've
    never paired with anyone), then returns None."""

    db = _get_db("management", uri=uri, ctx=ctx)
    ids = _get_management_data(MY_ID_RECORD_TYPE, "self_identity", uri=uri)
    ids = list(set(ids))  # uniqify
    if len(ids) > 1:
        logging.error("DANGER!  We have more than one record claiming to be "
                "this host's unique identifier.  Which is right?  We will try "
                "to use them all, but this smells really funny.")
        return ids
    if len(ids) == 1:
        return ids

    if not create:
        return None
    else:
        data = { "self_identity": str(uuid.uuid4()),
                "record_type": MY_ID_RECORD_TYPE,
            }
        data["_id"] = data["self_identity"]
        record_id = db.put_record(Record(data))
        logging.debug("set new self-identity value: %r", data["self_identity"])
        return [data["self_identity"]]

def get_all_known_pairings(uri=None,
        ctx=local_files.DEFAULT_CONTEXT):
    """Info dicts about all pairings, even if marked "unpaired", keyed on
    hostid with another dict as the value."""
    d = {}
    db = _get_db("management", uri=uri, ctx=ctx)
    for row in db.get_records(PAIRED_SERVER_RECORD_TYPE):
        v = dict()
        v["record_id"] = row.id
        v["active"] = True
        if "oauth" in row.value:
            v["oauth"] = row.value["oauth"]
        if "unpaired" in row.value:
            v["active"] = not row.value["unpaired"]
        hostid = row.value["pairing_identifier"]
        d[hostid] = v
    return d

def _get_management_data(record_type, key, uri=None,
        ctx=local_files.DEFAULT_CONTEXT):
    db = _get_db("management", uri=uri, ctx=ctx)
    results = db.get_records(create_view=True)
    values = list()
    for record in results[record_type]:
        if key in record.value:  # EAFP, rather than LBYL?  Nones default?
            value = record.value[key]
            if value is not None:
                if "_order" in value:  # MergableList, so decode.
                    values.append([value[k] for k in value["_order"]])
                else:
                    values.append(value)
            else:
                logging.debug("skipping record empty %s", key)
        else:
            logging.debug("skipping record %s with no %s", obsfuscate(record.value), key)
    logging.debug("found %d %s records", len(values), key)
    return values

def create_database(dst_host, dst_port, dst_name, use_ssl=False,
        oauth_tokens=None):
    """Given parts, create a database."""
    dst_url = mkuri(dst_host, dst_port, use_ssl)
    return server.CouchDatabase(dst_name, dst_url, create=True,
            oauth_tokens=oauth_tokens)

def replicate(source_database, target_database, target_host=None,
        target_port=None, source_host=None, source_port=None,
        source_ssl=False, target_ssl=False, source_oauth=None,
        target_oauth=None, local_uri=None):
    """This replication is instant and blocking, and does not persist. """

    try:
        if target_host:
            # Target databases must exist before replicating to them.
            logging.debug(
                "creating %r %s:%d %s", target_database, target_host,
                target_port, obsfuscate(target_oauth))
            create_database(
                target_host, target_port, target_database, use_ssl=target_ssl,
                oauth_tokens=target_oauth)
        else:
            server.CouchDatabase(target_database, create=True, uri=local_uri)
        logging.debug("db exists, and we're ready to replicate")
    except:
        logging.exception("can't create/verify %r %s:%d  oauth=%s",
                target_database, target_host, target_port, obsfuscate(target_oauth))
    if source_host:
        source = mkuri(source_host, source_port, source_ssl, urllib.quote(
            source_database, safe=""))
    else:
        source = urllib.quote(source_database, safe="")

    if target_host:
        target = mkuri(target_host, target_port, target_ssl, urllib.quote(
            target_database, safe=""))
    else:
        target = urllib.quote(target_database, safe="")

    if source_oauth:
        assert "consumer_secret" in source_oauth
        source = dict(url=source, auth=dict(oauth=source_oauth))

    if target_oauth:
        assert "consumer_secret" in target_oauth
        target = dict(url=target, auth=dict(oauth=target_oauth))

    record = dict(source=source, target=target)
    try:
        # regardless of source and target, we talk to our local couchdb  :(
        if not local_uri:
            port = int(desktopcouch_find_port())
            local_uri = mkuri("localhost", port,)

        logging.debug(
                "asking %r to replicate %s to %s", obsfuscate(local_uri),
                obsfuscate(source), obsfuscate(target),)

        ### All until python-couchdb gets a Server.replicate() function
        local_server = server.OAuthCapableServer(local_uri)
        resp, data = local_server.resource.post(path='/_replicate',
                content=record)

        logging.debug("replicate result:  %r  %r", obsfuscate(resp), obsfuscate(data))
        ###
    except:
        logging.exception("can't replicate %r  %r <== %r", source_database,
                local_uri, obsfuscate(record))

def get_pairings(uri=None, ctx=local_files.DEFAULT_CONTEXT):
    """Get a list of paired servers."""
    db = _get_db("management", create=True, uri=None, ctx=ctx)

    design_doc = "paired_servers"
    if not db.view_exists("paired_servers", design_doc):
        map_js = """function(doc) {
            if (doc.record_type == %r && ! doc.unpaired)  // unset or False
                if (doc.application_annotations &&
                    doc.application_annotations["Ubuntu One"] &&
                    doc.application_annotations["Ubuntu One"].private_application_annotations &&
                    doc.application_annotations["Ubuntu One"].private_application_annotations.deleted) {
                        // don't emit deleted or unpaired items
                    } else {
                        if (doc.server) {
                            emit(doc.server, doc);
                        } else if (doc.service_name) {
                            emit(doc.service_name, doc);
                        }
                    }
                }"""  % (PAIRED_SERVER_RECORD_TYPE,)
        db.add_view("paired_servers", map_js, None, design_doc)

    return db.execute_view("paired_servers")

def remove_pairing(record_id, is_reconciled, uri=None,
        ctx=local_files.DEFAULT_CONTEXT):
    """Remove a pairing record (or mark it as dead so it can be cleaned up
    properly later)."""
    db = _get_db("management", create=True, uri=None, ctx=ctx)
    if is_reconciled:
        db.delete_record(record_id)
    else:
        db.update_fields(record_id, { "unpaired": True })

def expunge_pairing(host_id, uri=None):
    try:
        d = get_all_known_pairings(uri)
        record_id = d[host_id]["record_id"]
        remove_pairing(record_id, True, uri)
    except KeyError, e:
        logging.warn("no key.  %s", e)
