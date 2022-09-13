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
 
from couchdb import Server
from couchdb.client import Resource
import desktopcouch
from desktopcouch.records import server_base
import urlparse

class OAuthCapableServer(Server):
    def __init__(self, uri, oauth_tokens=None, ctx=None):
        """Subclass of couchdb.client.Server which creates a custom
           httplib2.Http subclass which understands OAuth"""
        http = server_base.OAuthCapableHttp(scheme=urlparse.urlparse(uri)[0])
        http.force_exception_to_status_code = False
        if ctx is None:
            ctx = desktopcouch.local_files.DEFAULT_CONTEXT
        if oauth_tokens is None:
            oauth_tokens = desktopcouch.local_files.get_oauth_tokens(ctx)
        (consumer_key, consumer_secret, token, token_secret) = (
            oauth_tokens["consumer_key"], oauth_tokens["consumer_secret"], 
            oauth_tokens["token"], oauth_tokens["token_secret"])
        http.add_oauth_tokens(consumer_key, consumer_secret, token, token_secret)
        self.resource = Resource(http, uri)
 
class CouchDatabase(server_base.CouchDatabaseBase):
    """An small records specific abstraction over a couch db database."""
 
    def __init__(self, database, uri=None, record_factory=None, create=False,
                 server_class=OAuthCapableServer, oauth_tokens=None,
                 ctx=desktopcouch.local_files.DEFAULT_CONTEXT):
        self.ctx = ctx
        self.server_uri = uri
        super(CouchDatabase, self).__init__(
                database, uri, record_factory=record_factory, create=create,
                server_class=server_class, oauth_tokens=oauth_tokens, ctx=ctx)

    def _reconnect(self):
        if not self.server_uri:
            port = desktopcouch.find_port(ctx=self.ctx)
            uri = "http://localhost:%s" % port
        else:
            uri = self.server_uri
        super(CouchDatabase, self)._reconnect(uri=uri)

