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
#          Mark G. Saye <mark.saye@canonical.com>
#          Stuart Langridge <stuart.langridge@canonical.com>
#          Chad Miller <chad.miller@canonical.com>

"""The Desktop Couch Records API."""

import httplib2, urlparse, cgi, copy
from time import time, sleep
import socket

# please keep desktopcouch python 2.5 compatible for now

# pylint can't deal with failing imports even when they're handled
# pylint: disable-msg=F0401
try:
    # Python 2.5
    import simplejson as json
except ImportError:
    # Python 2.6+
    import json
# pylint: enable-msg=F0401

from oauth import oauth

from couchdb import Server
from couchdb.client import ResourceNotFound, ResourceConflict, uri as couchdburi
from couchdb.design import ViewDefinition
from record import Record
import logging

#DEFAULT_DESIGN_DOCUMENT = "design"
DEFAULT_DESIGN_DOCUMENT = None  # each view in its own eponymous design doc.

class FieldsConflict(Exception):
    """Raised in case of an unrecoverable couchdb conflict."""

    #pylint: disable-msg=W0231
    def __init__(self, conflicts):
        self.conflicts = conflicts
    #pylint: enable-msg=W0231

    def __str__(self):
        return "<CouchDB Conflict Error: %s>" % self.conflicts


class NoSuchDatabase(Exception):
    "Exception for trying to use a non-existent database"

    def __init__(self, dbname):
        self.database = dbname
        super(NoSuchDatabase, self).__init__()

    def __str__(self):
        return ("Database %s does not exist on this server. (Create it by "
                "passing create=True)") % self.database


class OAuthAuthentication(httplib2.Authentication):
    """An httplib2.Authentication subclass for OAuth"""
    def __init__(self, oauth_data, host, request_uri, headers, response,
        content, http, scheme):
        self.oauth_data = oauth_data
        self.scheme = scheme
        httplib2.Authentication.__init__(self, None, host, request_uri,
              headers, response, content, http)

    def request(self, method, request_uri, headers, content):
        """Modify the request headers to add the appropriate
        Authorization header."""
        consumer = oauth.OAuthConsumer(self.oauth_data['consumer_key'],
           self.oauth_data['consumer_secret'])
        access_token = oauth.OAuthToken(self.oauth_data['token'],
           self.oauth_data['token_secret'])
        sig_method = oauth.OAuthSignatureMethod_HMAC_SHA1
        full_http_url = "%s://%s%s" % (self.scheme, self.host, request_uri)
        schema, netloc, path, params, query, fragment = \
                urlparse.urlparse(full_http_url)
        querystr_as_dict = dict(cgi.parse_qsl(query))
        req = oauth.OAuthRequest.from_consumer_and_token(
            consumer,
            access_token,
            http_method = method,
            http_url = full_http_url,
            parameters = querystr_as_dict
        )
        req.sign_request(sig_method(), consumer, access_token)
        headers.update(httplib2._normalize_headers(req.to_header()))


class OAuthCapableHttp(httplib2.Http):
    """Subclass of httplib2.Http which specifically uses our OAuth
       Authentication subclass (because httplib2 doesn't know about it)"""
    def __init__(self, scheme="http", cache=None, timeout=None, proxy_info=None):
        self.__scheme = scheme
        super(OAuthCapableHttp, self).__init__(cache, timeout, proxy_info)

    def add_oauth_tokens(self, consumer_key, consumer_secret,
                         token, token_secret):
        self.oauth_data = {
            "consumer_key": consumer_key,
            "consumer_secret": consumer_secret,
            "token": token,
            "token_secret": token_secret
        }

    def _auth_from_challenge(self, host, request_uri, headers, response,
            content):
        """Since we know we're talking to desktopcouch, and we know that it
           requires OAuth, just return the OAuthAuthentication here rather
           than checking to see which supported auth method is required."""
        yield OAuthAuthentication(self.oauth_data, host, request_uri, headers,
                response, content, self, self.__scheme)

def row_is_deleted(row):
    """Test if a row is marked as deleted.  Smart views 'maps' should not
    return rows that are marked as deleted, so this function is not often
    required."""
    try:
        return row['application_annotations']['Ubuntu One']\
                ['private_application_annotations']['deleted']
    except KeyError:
        return False


class CouchDatabaseBase(object):
    """An small records specific abstraction over a couch db database."""

    def __init__(self, database, uri, record_factory=None, create=False,
                 server_class=Server, **server_class_extras):
        self._database_name = database
        self._create = create
        self._server_class = server_class
        self._server_class_extras = server_class_extras
        self.record_factory = record_factory or Record
        self.server_uri = uri
        self._server = None
        self.db = None
        self._reconnect()
        self._changes_since = self.db.info()["update_seq"]
        self._changes_last_used = 0  # Immediate run works.

    @staticmethod
    def _is_reconnection_fail(ex):
        return isinstance(ex, AttributeError) and \
                ex.args == ("'NoneType' object has no attribute 'makefile'",)

    def with_reconnects(self, func, *args, **kwargs):
        for retry in (3, 2, 1, None):
            try:
                return func(*args, **kwargs)
            except Exception, e:
                if self._is_reconnection_fail(e) and retry:
                    logging.warn("DB connection failed.  Reconnecting.")
                    self._reconnect()
                    continue
                elif isinstance(e, socket.error):
                    logging.warn("Other socket error %s.  Reconnecting.", e)
                    sleep(0.3)  
                    self._reconnect()
                    continue
                else:
                    raise
        raise ValueError("failed to (re-)connect to couchdb server")

    def _reconnect(self, uri=None):
        logging.info("Connecting to %s.", self.server_uri or "discovered local port")

        self._server = self._server_class(uri or self.server_uri,
                **self._server_class_extras)
        if self._database_name not in self._server:
            if self._create:
                self._server.create(self._database_name)
            else:
                raise NoSuchDatabase(self._database_name)
        if self.db is None:
            self.db = self._server[self._database_name]
        else:
            # Monkey-patch the object the user already uses.  Oook!
            new_db = self._server[self._database_name]
            self.db.resource = new_db.resource

    def _temporary_query(self, map_fun, reduce_fun=None, language='javascript',
            wrapper=None, **options):
        """Pass-through to CouchDB library.  Deprecated."""
        return self.with_reconnects(self.db.query, map_fun, reduce_fun, language,
              wrapper, **options)

    def get_record(self, record_id):
        """Get a record from back end storage."""
        try:
            couch_record = self.with_reconnects(self.db.__getitem__, record_id)
        except ResourceNotFound:
            return None
        data = {}
        if row_is_deleted(couch_record):
            return None
        data.update(couch_record)
        record = self.record_factory(data=data)

        def make_getter(source_db, document_id, attachment_name, content_type):
            """Closure storing the database for lower levels to use when needed.
            """
            def getter():
                return source_db.get_attachment(document_id, attachment_name), \
                        content_type
            return getter
        if "_attachments" in data:
            for att_name, att_attributes in data["_attachments"].iteritems():
                record.attach_by_reference(att_name,
                        make_getter(self.db, record_id, att_name,
                                att_attributes["content_type"]))
        return record

    def put_record(self, record):
        """Put a record in back end storage."""
        if not record.record_id:
            # Do not rely on couchdb to create an ID for us.
            from uuid import uuid4
            record.record_id = uuid4().hex
        try:
            self.with_reconnects(self.db.__setitem__,
                    record.record_id, record._data)
        except ResourceConflict:
            if record.record_revision is None and not row_is_deleted(record):
                old_record = self.with_reconnects(self.db.__getitem__,
                    record.record_id)
                if row_is_deleted(old_record):
                    # User asked to make new record that already exists
                    # but we have marked deleted internally.  Instead of
                    # complaining, pull up the previous revision ID and
                    # add that to the user's record, and re-send it.
                    record._set_record_revision(old_record.rev)

                    self.with_reconnects(self.db.__setitem__,
                            record.record_id, record._data)
                else:
                    raise
            else:
                raise

        for attachment_name in getattr(record, "_detached", []):
            self.with_reconnects(self.db.delete_attachment,
                record._data, attachment_name)

        for attachment_name in record.list_attachments():
            data, content_type = record.attachment_data(attachment_name)
            self.with_reconnects(self.db.put_attachment,
                record._data, data, attachment_name, content_type)

        return record.record_id

    def put_records_batch(self, batch):
        """Put a batch of records in back end storage."""
        # used to access fast the record
        records_hash = {}
        for record in batch:
            if not record.record_id:
                # Do not rely on couchdb to create an ID for us.
                from uuid import uuid4
                record.record_id = uuid4().hex
            records_hash[record.record_id] = record
        # although with a single record we need to test for the
        # revisison, with a batch we do not, but we have to make sure
        # that we did not get an error
        batch_put_result = self.with_reconnects(
                self.db.update, [record._data for record in batch])
        for current_tuple in batch_put_result:
            success, docid, rev_or_exc = current_tuple
            if success:
                record = records_hash[docid]
                # set the new rev
                record._data["_rev"] = rev_or_exc

                for attachment_name in getattr(record, "_detached", []):
                    self.with_reconnects(self.db.delete_attachment,
                        record._data, attachment_name)

                for attachment_name in record.list_attachments():
                    data, content_type = record.attachment_data(attachment_name)
                    self.with_reconnects(self.db.put_attachment,
                        {"_id":record.record_id, "_rev":record["_rev"]},
                        data, attachment_name, content_type)
        # all success record have the blobs added we return result of
        # update
        return batch_put_result

    def update_fields(self, record_id, fields, cached_record=None):
        """Safely update a number of fields. 'fields' being a
        dictionary with path_tuple: new_value for only the fields we
        want to change the value of, where path_tuple is a tuple of
        fieldnames indicating the path to the possibly nested field
        we're interested in. old_record a the copy of the record we
        most recently read from the database.

        In the case the underlying document was changed, we try to
        merge, but only if none of the old values have changed. (i.e.,
        do not overwrite changes originating elsewhere.)

        This is slightly hairy, so that other code won't have to be.
        """
        # Initially, the record in memory and in the db are the same
        # as far as we know. (If they're not, we'll get a
        # ResourceConflict later on, from which we can recover.)
        if cached_record is None:
            cached_record = self.with_reconnects(self.db.__getitem__,
                    record_id)
        if isinstance(cached_record, Record):
            cached_record = cached_record._data
        record = copy.deepcopy(cached_record)
        # Loop until either failure or success has been determined
        while True:
            modified = False
            conflicts = {}
            # loop through all the fields that need to be modified
            for path, new_value in fields.items():
                if not isinstance(path, tuple):
                    path = (path,)
                # Walk down in both copies of the record to the leaf
                # node that represents the field, creating the path in
                # the in memory record if necessary.
                db_parent = record
                cached_parent = cached_record
                for field in path[:-1]:
                    db_parent = db_parent.setdefault(field, {})
                    cached_parent = cached_parent.get(field, {})
                # Get the values of the fields in the two copies.
                db_value = db_parent.get(path[-1])
                cached_value = cached_parent.get(path[-1])
                # If the value we intend to store is already in the
                # database, we need do nothing, which is our favorite.
                if db_value == new_value:
                    continue
                # If the value in the db is different than the value
                # our copy holds, we have a conflict. We could bail
                # here, but we can give better feedback if we gather
                # all the conflicts, so we continue the for loop
                if db_value != cached_value:
                    conflicts[path] = (db_value, new_value)
                    continue
                # Otherwise, it is safe to update the field with the
                # new value.
                modified = True
                db_parent[path[-1]] = new_value
            # If we had conflicts, we can bail.
            if conflicts:
                raise FieldsConflict(conflicts)
            # If we made changes to the document, we'll need to save
            # it.
            if modified:
                try:
                    self.with_reconnects(self.db.__setitem__, record_id,
                            record)
                except ResourceConflict:
                    # We got a conflict, meaning the record has
                    # changed in the database since we last loaded it
                    # into memory. Let's get a fresh copy and try
                    # again.
                    record = self.with_reconnects(self.db.__getitem__,
                            record_id)
                    continue
            # If we get here, nothing remains to be done, and we can
            # take a well deserved break.
            break

    def delete_record(self, record_id):
        """Delete record with given id"""
        record = self.with_reconnects(self.db.__getitem__, record_id)
        record.setdefault('application_annotations', {}).setdefault(
            'Ubuntu One', {}).setdefault('private_application_annotations', {})[
            'deleted'] = True
        self.with_reconnects(self.db.__setitem__, record_id, record)

    def record_exists(self, record_id):
        """Check if record with given id exists."""
        try:
            record = self.with_reconnects(self.db.__getitem__, record_id)
        except ResourceNotFound:
            return False
        return not row_is_deleted(record)

    def delete_view(self, view_name, design_doc=DEFAULT_DESIGN_DOCUMENT):
        """Remove a view, given its name.  Raises a KeyError on a unknown
        view.  Returns a dict of functions the deleted view defined."""
        if design_doc is None:
            design_doc = view_name

        doc_id = "_design/%(design_doc)s" % locals()

        # No atomic updates.  Only read & mutate & write.  Le sigh.
        # First, get current contents.
        try:
            view_container = self.with_reconnects(
                    self.db.__getitem__, doc_id)["views"]
        except (KeyError, ResourceNotFound):
            raise KeyError

        deleted_data = view_container.pop(view_name)  # Remove target

        if len(view_container) > 0:
            # Construct a new list of objects representing all views to have.
            views = [
                    ViewDefinition(design_doc, k, v.get("map"), v.get("reduce"))
                    for k, v
                    in view_container.iteritems()
                    ]
            # Push back a new batch of view.  Pray to Eris that this doesn't
            # clobber anything we want.

            # sync_many does nothing if we pass an empty list.  It even gets
            # its design-document from the ViewDefinition items, and if there
            # are no items, then it has no idea of a design document to
            # update.  This is a serious flaw.  Thus, the "else" to follow.
            ViewDefinition.sync_many(self.db, views, remove_missing=True)
        else:
            # There are no views left in this design document.

            # Remove design document.  This assumes there are only views in
            # design documents.  :(
            self.with_reconnects(self.db.__delitem__, doc_id)

        assert not self.view_exists(view_name, design_doc)

        return deleted_data

    def execute_view(self, view_name, design_doc=DEFAULT_DESIGN_DOCUMENT,
            **params):
        """Execute view and return results."""

        class ReconnectingViewWrapper(object):
            """A view from python-couchdb is an object with attributes that
            cause HTTP requests to be fired off when accessed.  If we wish
            to be able to reconnect on disappearance of the server, then
            we must intercept calls from user to python-couchdb."""

            def __init__(wrapper, obj, *args, **kwargs):
                wrapper.obj = self.with_reconnects(obj, *args, **kwargs)

            def ___call__(wrapper, **options):
                return self.with_reconnects(wrapper.obj.__call__, **options)

            def __iter__(wrapper):
                return self.view_with_reconnects(wrapper.obj.__iter__,
                                                 wrapper.obj)

            def __len__(wrapper):
                return self.view_with_reconnects(wrapper.obj.__len__,
                                                 wrapper.obj)

            def __getattr__(wrapper, key):
                return self.with_reconnects(getattr, wrapper.obj, key)

            def __getitem__(wrapper, key):
                return ReconnectingViewWrapper(wrapper.obj.__getitem__, key)

        if design_doc is None:
            design_doc = view_name

        view_id_fmt = "_design/%(design_doc)s/_view/%(view_name)s"
        wrapper = ReconnectingViewWrapper(self.db.view, 
                                          view_id_fmt % locals(),
                                          **params)
        return wrapper

    def view_with_reconnects(self, func, view, *args, **kwargs):
        for retry in (3, 2, 1, None):
            try:
                return func(*args, **kwargs)
            except Exception, e:
                if self._is_reconnection_fail(e) and retry:
                    logging.warn("DB connection failed.  Reconnecting.")
                    self._reconnect_view(view_result=view)
                    continue
                elif isinstance(e, socket.error):
                    logging.warn("Other socket error %s.  Reconnecting.", e)
                    sleep(0.3)  
                    self._reconnect()
                    continue
                else:
                    raise
        raise ValueError("failed to (re-)connect to couchdb server")

    def _reconnect_view(self, uri=None, view_result=None):
        logging.info("Connecting to %s.",
                     self.server_uri or "discovered local port")
        self._server = self._server_class(
            uri or '/'.join(self.db.resource.uri.split('/')[:-1]),
                **self._server_class_extras)
        if self._database_name not in self._server:
            if self._create:
                self._server.create(self._database_name)
            else:
                raise NoSuchDatabase(self._database_name)
        if self.db is None:
            self.db = self._server[self._database_name]
        else:
            # Monkey-patch the object the user already uses.  Oook!
            view_url = urlparse.urlparse(view_result.view.resource.uri)
            db_url = urlparse.urlparse(self.db.resource.uri)
            view_result.view.resource.uri = urlparse.urlunparse(
                [view_url[0], db_url[1]] + list(view_url[2:]))

    def add_view(self, view_name, map_js, reduce_js,
            design_doc=DEFAULT_DESIGN_DOCUMENT):
        """Create a view, given a name and the two parts (map and reduce).
        Return the document id."""
        if design_doc is None:
            design_doc = view_name

        view = ViewDefinition(design_doc, view_name, map_js, reduce_js)
        self.with_reconnects(view.sync, self.db)
        assert self.view_exists(view_name, design_doc)

    def view_exists(self, view_name, design_doc=DEFAULT_DESIGN_DOCUMENT):
        """Does a view with a given name, in a optional design document
        exist?"""
        if design_doc is None:
            design_doc = view_name

        doc_id = "_design/%(design_doc)s" % locals()

        try:
            view_container = \
                    self.with_reconnects(self.db.__getitem__, doc_id)["views"]
            return view_name in view_container
        except (KeyError, ResourceNotFound):
            return False

    def list_views(self, design_doc):
        """Return a list of view names for a given design document.  There is
        no error if the design document does not exist or if there are no views
        in it."""
        doc_id = "_design/%(design_doc)s" % locals()
        try:
            return list(self.with_reconnects(self.db.__getitem__, doc_id)["views"])
        except (KeyError, ResourceNotFound):
            return []

    def get_records(self, record_type=None, create_view=False,
            design_doc=DEFAULT_DESIGN_DOCUMENT):
        """A convenience function to get records from a view named
        C{get_records_and_type}.  We optionally create a view in the design
        document.  C{create_view} may be True or False, and a special value,
        None, is analogous to  O_EXCL|O_CREAT .

        Set record_type to a string to retrieve records of only that
        specified type. Otherwise, usse the view to return *all* records.
        If there is no view to use or we insist on creating a new view
        and cannot, raise KeyError .

        You can use index notation on the result to get rows with a
        particular record type.
        =>> results = get_records()
        =>> for foo_document in results["foo"]:
        ...    print foo_document

        Use slice notation to apply start-key and end-key options to the view.
        =>> results = get_records()
        =>> people = results[['Person']:['Person','ZZZZ']]
        """
        view_name = "get_records_and_type"
        view_map_js = """
            function(doc) {
                try {
                    if (! doc['application_annotations']['Ubuntu One']
                            ['private_application_annotations']['deleted']) {
                        emit(doc.record_type, doc);
                    }
                } catch (e) {
                    emit(doc.record_type, doc);
                }
            }"""

        if design_doc is None:
            design_doc = view_name

        exists = self.view_exists(view_name, design_doc)

        if exists:
            if create_view is None:
                raise KeyError("Exclusive creation failed.")
        else:
            if create_view == False:
                raise KeyError("View doesn't already exist.")

        if not exists:
            self.add_view(view_name, view_map_js, None, design_doc)

        viewdata = self.execute_view(view_name, design_doc)
        if record_type is None:
            return viewdata
        else:
            return viewdata[record_type]

    def get_changes(self, niceness=10):
        """Get a list of database changes.  This is the sister function of
        report_changes that returns a list instead of calling a function for
        each."""
        l = list()
        self.report_changes(lambda **k: l.append(k), niceness=niceness)
        return l

    def report_changes(self, function_to_send_changes_to, niceness=10):
        """Check to see if there are any changes on this database since last
        call (or since this object was instantiated), call a function for each,
        and return the number of changes reported.

        The callback function is called for every single change, with the
        keyword parameters of the dictionary of values returned from couchdb.

        >>> def f(seq=None, id=None, changes=None):
        ...    pass

        >>> db_foo.report_changes(f)
        >>> time.sleep(30)
        >>> db_foo.report_changes(f)

        or

        >>> # Make function that returns true, to signal never to remove.
        >>> report_forever = lambda **kw: db_foo.report_changes(**kw) or True
        >>> cb_id = glib.mainloop.idle_add(report_forever, f)

        or

        >>> task_id = twisted.task.looping_call(db_foo.report_changes, f)
        """
        now = time()
        call_count = 0
        if not niceness or now > self._changes_last_used + niceness:

            # Can't use self._server.resource.get() directly because
            # it encodes "/".
            uri = couchdburi(
                self._server.resource.uri, self.db.name, "_changes",
                since=self._changes_since)
            ## Assume server has not crashed and URI is the same.  FIXME
            resp, data = self._server.resource.http.request(uri, "GET", "", {})
            if resp["status"] != '200':
                raise IOError(
                    "HTTP response code %s.\n%s" % (resp["status"], data))
            structure = json.loads(data)
            for change in structure.get("results"):
                # kw-args can't have unicode keys
                change_encoded_keys = dict(
                    (k.encode("utf8"), v) for k, v in change.iteritems())
                function_to_send_changes_to(**change_encoded_keys)
                # Not s.last_seq.  Exceptions!
                self._changes_since = change["seq"]
                call_count += 1
            # If exception in cb, we never update governor.
            self._changes_last_used = now
        return call_count

    def ensure_full_commit(self):
        """
        Make sure that CouchDb flushes all writes to the database,
        flushing all delayed commits, before going on.
        """
        self.db.resource.post(
            path='_ensure_full_commit',
            headers={'Content-Type': 'application/json'})
