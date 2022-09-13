# Copyright 2009-2010 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranties of
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

from oauth import oauth
import urllib2
import libproxy
import gconf
import simplejson
import urlparse

GCONF_HTTP_USE_AUTH = "/system/http_proxy/use_authentication"
GCONF_HTTP_PROXY_USER = "/system/http_proxy/authentication_user"
GCONF_HTTP_PROXY_PASSWORD =  "/system/http_proxy/authentication_password"

class RestClient:
    """Class to perform REST API calls to one.ubuntu.com, with proxy support."""

    def __init__(self, url):
        self.conf_client = gconf.client_get_default()

        # Get the proxy to be used
        pf = libproxy.ProxyFactory()
        proxies = pf.getProxies(url)
        for proxy in proxies:
            if proxy == "direct://":
                break
            elif proxy.startswith("http://") or proxy.startswith("https://"):
                real_proxy = proxy
                if self.conf_client.get_bool(GCONF_HTTP_USE_AUTH):
                    username = self.conf_client.get_string(GCONF_HTTP_PROXY_USER)
                    password = self.conf_client.get_string(GCONF_HTTP_PROXY_PASSWORD)
                    if username is not None and password is not None:
                        scheme, netloc, path, query, fragment = urlparse.urlsplit(proxy)
                        real_proxy = "%s://%s:%s@%s" % (scheme, username, password, netloc)

                self.proxy_support = urllib2.ProxyHandler({"http": real_proxy, "https": real_proxy})
                self.opener = urllib2.build_opener(self.proxy_support)
                urllib2.install_opener(self.opener)
                break

    def call(self, url, method, oauth_consumer, oauth_token):
	"""Call a REST method on the given URL."""
        oauth_request = oauth.OAuthRequest.from_consumer_and_token(
            http_url=url,
            http_method=method,
            oauth_consumer=oauth_consumer,
            token=oauth_token,
            parameters='')
        oauth_request.sign_request(oauth.OAuthSignatureMethod_HMAC_SHA1(),
                                   oauth_consumer, oauth_token)

        try:
            headers = oauth_request.to_header()
            request = urllib2.Request(url, data=None, headers=headers)
            response = urllib2.urlopen(request)
            data = response.read()
            return simplejson.loads(data)

        except urllib2.HTTPError, e:
            # the error is not pickle-able, so str() it
            return {'error' : str(e)}

