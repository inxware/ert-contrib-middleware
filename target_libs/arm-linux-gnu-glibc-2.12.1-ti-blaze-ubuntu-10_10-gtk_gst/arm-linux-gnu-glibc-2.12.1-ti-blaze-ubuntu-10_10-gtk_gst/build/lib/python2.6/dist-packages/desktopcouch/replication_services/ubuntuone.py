from oauth import oauth
import logging
import httplib2

import dbus
import ubuntu_sso

try:
    # Python 2.5
    import simplejson as json
except ImportError:
    # Python 2.6+
    import json


name = "Ubuntu One"
description = "The Ubuntu One cloud service"


def in_main_thread(function, *args, **kwargs):
    from twisted.python.threadable import isInIOThread
    if isInIOThread():  # We may not be in main thread right now.
        from twisted.internet.threads import blockingCallFromThread
        from twisted.internet import reactor
        return blockingCallFromThread(reactor, function, *args, **kwargs)
    else:  # If in main, just do it.
        return function(*args, **kwargs)

def is_active():
    """Can we deliver information?"""
    return get_oauth_data() is not None

oauth_data = None
def get_oauth_data():
    """Information needed to replicate to a server."""
    global oauth_data
    if oauth_data is not None:
        return oauth_data

    bus = dbus.SessionBus()
    proxy = bus.get_object(ubuntu_sso.DBUS_BUS_NAME,
                           ubuntu_sso.DBUS_CRED_PATH,
                           follow_name_owner_changes=True)
    logging.info('get_oauth_data: asking for credentials to Ubuntu SSO. '
                 'App name: %s', name)
    oauth_data = proxy.find_credentials(name)
    logging.info('Got credentials from Ubuntu SSO. Non emtpy credentials? %s',
                 len(oauth_data) > 0)
    return oauth_data


def get_oauth_request_header(consumer, access_token, http_url):
    """Get an oauth request header given the token and the url"""
    signature_method = oauth.OAuthSignatureMethod_PLAINTEXT()
    assert http_url.startswith("https")
    oauth_request = oauth.OAuthRequest.from_consumer_and_token(
        http_url=http_url,
        http_method="GET",
        oauth_consumer=consumer,
        token=access_token)
    oauth_request.sign_request(signature_method, consumer, access_token)
    return oauth_request.to_header()


class PrefixGetter():
    def __init__(self):
        self.str = None
        self.oauth_header = None
        self.user_id = None

    def __str__(self):
        if self.str is not None:
            return self.str

        url = "https://one.ubuntu.com/api/account/"
        if self.oauth_header is None:
            try:
                get_oauth_data()
                consumer = oauth.OAuthConsumer(oauth_data['consumer_key'],
                                               oauth_data['consumer_secret'])
                access_token = oauth.OAuthToken(oauth_data['token'],
                                                oauth_data['token_secret'])
            except:
                logging.exception("Could not get access token from sso.")
                raise ValueError("No access token.")

        self.oauth_header = get_oauth_request_header(
                consumer, access_token, url)

        client = httplib2.Http()
        resp, content = client.request(url, "GET", headers=self.oauth_header)
        if resp['status'] == "200":
            document = json.loads(content)
            if "couchdb_root" not in document:
                raise ValueError("couchdb_root not found in %s" % (document,))
            self.str = document["couchdb_root"]
            self.user_id = document["id"]
        else:
            logging.error(
                "Couldn't talk to %r.  Got HTTP %s", url, resp['status'])
            raise ValueError("HTTP %s for %r" % (resp['status'], url))

        return self.str

# Access to this as a string fires off functions.
db_name_prefix = PrefixGetter()


from desktopcouch.records.server import CouchDatabase
from desktopcouch.records.record import Record
from desktopcouch.pair.couchdb_pairing import couchdb_io
from desktopcouch import local_files

#
#  One day, if we decide this is a good idea, we can generalize this to
#  any pairing.
#
class ReplicationExclusion(object):
    """Set some databases to be replicatable or non-replicatable to
    Ubuntu One cloud couchdb servers."""
    def __init__(self, url=None, ctx=local_files.DEFAULT_CONTEXT):
        super(ReplicationExclusion, self).__init__()
        self.management_db = CouchDatabase("management", uri=url, create=True,
                ctx=ctx)
        all_pairing_records = self.management_db.get_records(
                record_type=couchdb_io.PAIRED_SERVER_RECORD_TYPE,
                create_view=True)
        for row in all_pairing_records:
            if row["value"].get("service_name", None) == "ubuntuone":
                self.pairing_record = Record(data=row["value"],
                        record_type=row["key"], record_id=row["id"])
                return
        # else
        raise ValueError("No pairing record for ubuntuone.")

    def all_exclusions(self):
        """Gets all exclusions that can be removed, which omits 'management'
        and 'users', as those are private to this machine always."""

        return set(self.pairing_record.get("excluded_names", []))

    def _update(self, verb, db_name):
        current = self.all_exclusions()
        try:
            getattr(current, verb)(db_name)
        except KeyError:
            pass  # Not an error to remove something already gone.
        if len(current) > 0:
            self.pairing_record["excluded_names"] = list(current)
        else:
            if "excluded_names" in self.pairing_record:
                del self.pairing_record["excluded_names"]
        self.management_db.put_record(self.pairing_record)

    def exclude(self, db_name):
        """Sets a database never to be replicatable to Ubuntu One."""
        self._update("add", db_name)

    def replicate(self, db_name):
        """Sets a database to be replicatable to Ubuntu One."""
        self._update("remove", db_name)

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG, format="%(message)s")
    print str(db_name_prefix)
