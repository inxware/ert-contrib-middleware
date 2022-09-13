#  software-properties PPA support
#
#  Copyright (c) 2004-2009 Canonical Ltd.
#
#  Author: Michael Vogt <mvo@debian.org>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
#  USA

import apt_pkg
import re
import subprocess
from threading import Thread
import urllib
from urllib2 import urlopen, Request, URLError
from urlparse import urlparse

def encode(s):
    return re.sub("[^a-zA-Z0-9_-]","_", s)

def expand_ppa_line(abrev, distro_codename):
    """ Convert an abbreviated ppa name of the form 'ppa:$name' to a
        proper sources.list line of the form 'deb ...' """
    # leave non-ppa: lines unchanged
    if not abrev.startswith("ppa:"):
        return (abrev, None)
    # FIXME: add support for dependency PPAs too (once we can get them
    #        via some sort of API, see LP #385129)
    abrev = abrev.split(":")[1]
    ppa_owner = abrev.split("/")[0]
    try:
        ppa_name = abrev.split("/")[1]
    except IndexError, e:
        ppa_name = "ppa"
    sourceslistd = apt_pkg.Config.find_dir("Dir::Etc::sourceparts")
    line = "deb http://ppa.launchpad.net/%s/%s/ubuntu %s main" % (
        ppa_owner, ppa_name, distro_codename)
    filename = "%s/%s-%s-%s.list" % (
        sourceslistd, encode(ppa_owner), encode(ppa_name), distro_codename)
    return (line, filename)


class AddPPASigningKeyThread(Thread):
    " thread class for adding the signing key in the background "

    def __init__(self, ppa_path):
        Thread.__init__(self)
        self.ppa_path = ppa_path
        
    def run(self):
        self.add_ppa_signing_key(self.ppa_path)
        
    def add_ppa_signing_key(self, ppa_path):
        """Query and add the corresponding PPA signing key.
        
        The signing key fingerprint is obtained from the Launchpad PPA page,
        via a secure channel, so it can be trusted.
        """
        owner_name, ppa_name, distro = ppa_path[1:].split('/')
        lp_url = ('https://launchpad.net/api/1.0/~%s/+archive/%s' % (
            owner_name, ppa_name))
        try:
            # we ask for a JSON structure from lp_page, we could use
            # simplejson, but the format is simple enough for the regexp
            req =  Request(lp_url)
            req.add_header("Accept","application/json")
            lp_page = urlopen(req).read()
            #print lp_page
            signing_key_fingerprint = re.findall(
                '\"signing_key_fingerprint\": \"(\w*)\"', lp_page)
            if not signing_key_fingerprint:
              raise IOError()
            # FIXME: this needs to go - elmo says the keyserver will not handle
            #        the load
            res = subprocess.call(
                ["apt-key", "adv", "--keyserver", "keyserver.ubuntu.com",
                 "--recv", signing_key_fingerprint[0]])
            return (res == 0)
        except URLError, e:
            print "Error reading %s: %s" % (lp_url, e)
            return False
        except IOError, e:
            print "Error: can't find signing_key_fingerprint at %s" % lp_url
            return False
