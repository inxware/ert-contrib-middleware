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
# Author: Stuart Langridge <stuart.langridge@canonical.com>
#         Eric Casteleijn <eric.casteleijn@canonical.com>

"""Specify location of files relevant to desktop CouchDB.

Checks to see whether we're running out of the source tree or not.
"""
from __future__ import with_statement
import os
import errno
import xdg.BaseDirectory as xdg_base_dirs
import subprocess
import logging
import tempfile
import random
import string
import re
import gnomekeyring
try:
    import ConfigParser as configparser
except ImportError:
    import configparser


COUCH_EXE = os.environ.get('COUCHDB')
if not COUCH_EXE:
    for x in os.environ['PATH'].split(':'):
        if os.path.exists(os.path.join(x, 'couchdb')):
            COUCH_EXE = os.path.join(x, 'couchdb')
if not COUCH_EXE:
    raise ImportError("Could not find couchdb")


class _Configuration(object):
    def __init__(self, ctx):
        super(_Configuration, self).__init__()
        self.file_name_used = ctx.file_ini
        self._c = configparser.SafeConfigParser()
        # monkeypatch ConfigParser to stop it lower-casing option names
        self._c.optionxform = lambda s: s

        bookmark_file = os.path.join(ctx.db_dir, "couchdb.html")
        try:
            bookmark_file_contents = open(bookmark_file).read()
            if "-hashed-" in bookmark_file_contents:
                raise ValueError("Basic-auth cred lost.")
            u, p = re.findall("<!-- !!([^!]+)!!([^!]+)!! -->",
                    bookmark_file_contents)[-1]  # trial run, check sanity.
            self._fill_from_file(self.file_name_used)
            return
        except (IOError, ValueError, IndexError):
            pass  # Loading failed.  Let's fill it with sensible defaults.

        local = {
            'couchdb': {
                'database_dir': ctx.db_dir,
                'view_index_dir': ctx.db_dir,
            },
            'httpd': {
                'bind_address': '127.0.0.1',
                'port': '0',
                'WWW-Authenticate': 'Basic realm="bookmarkable-user-auth"'
            },
            'log': {
                'file': ctx.file_log,
                'level': ctx.couchdb_log_level,
            },
        }

        if ctx.with_auth:
            admin_username = self._make_random_string(10)
            admin_password = self._make_random_string(10)
            if ctx.keyring is not None:
                try:
                    data = ctx.keyring.find_items_sync(gnomekeyring.ITEM_GENERIC_SECRET,
                            {'desktopcouch': 'basic'})
                    admin_username, admin_password = data[0].secret.split(":", 1)
                except gnomekeyring.NoMatchError:
                    try:
                        # save admin account details in keyring
                        ctx.keyring.item_create_sync(None,
                                gnomekeyring.ITEM_GENERIC_SECRET,
                                'Desktop Couch user authentication',
                                {'desktopcouch': 'basic'},
                                ":".join([admin_username, admin_password]), True)
                    except gnomekeyring.NoKeyringDaemonError:
                        logging.warn("There is no keyring to store our admin credentials.")
                except gnomekeyring.CancelledError:
                    logging.warn("There is no keyring to store our admin credentials.")
                    ctx.keyring = None;

            consumer_key = self._make_random_string(10)
            consumer_secret = self._make_random_string(10)
            token = self._make_random_string(10)
            token_secret = self._make_random_string(10)

            if ctx.keyring is not None:
                # Save the new OAuth creds so that 3rd-party apps can authenticate by
                # accessing the keyring first.  This is one-way.  We don't read from keyring.
                try:
                    ctx.keyring.item_create_sync(None,
                            gnomekeyring.ITEM_GENERIC_SECRET,
                            'Desktop Couch user authentication', {'desktopcouch': 'oauth'},
                            ":".join([consumer_key, consumer_secret, token, token_secret]),
                            True)
                except gnomekeyring.NoKeyringDaemonError:
                    logging.warn("There is no keyring to store our oauth credentials.")
                except gnomekeyring.CancelledError:
                    logging.warn("There is no keyring to store our admin credentials.")

            local.update({'admins': {
                    admin_username: admin_password
                },
                'oauth_consumer_secrets': {
                    consumer_key: consumer_secret
                },
                'oauth_token_secrets': {
                    token: token_secret
                },
                'oauth_token_users': {
                    token: admin_username
                },
                'couch_httpd_auth': {
                    'require_valid_user': 'true'
                }
            })

        self._fill_from_structure(local)
        self.save_to_file(self.file_name_used)

        fp = open(bookmark_file, "w")
        try:
            fp.write("Desktopcouch is not yet started.\n")
            fp.write("<!-- Couch clobbers useful creds in INI file. -->\n")
            fp.write("<!-- So, store creds in the place we need it. -->\n")
            fp.write("<!-- The '!' fmt is perpetual but port varies. -->\n")
            fp.write("<!-- !!%(admin_username)s!!%(admin_password)s!! -->\n" \
                    % locals())
            fp.write("<!-- ^^  Seeded only creds into bookmark file. -->\n")
            fp.write("<!-- Will flesh out with port when we know it. -->\n")
            fp.write("<!-- Replacement text also uses creds format. -->\n")
        finally:
            fp.close()

    # randomly generate tokens and usernames
    @staticmethod
    def _make_random_string(count):
        entropy = random.SystemRandom()
        return ''.join([entropy.choice(string.letters) for x in range(count)])

    def _fill_from_structure(self, structure):
        for section_name in structure:
            for key in structure[section_name]:
                self.set_item(section_name, key, structure[section_name][key])

    def _fill_from_file(self, file_name):
        with file(file_name) as f:
            self._c.readfp(f)

    def save_to_file(self, file_name):
        container = os.path.split(file_name)[0]
        fd, temp_file_name = tempfile.mkstemp(dir=container)
        f = os.fdopen(fd, "w")
        try:
            self._c.write(f)
        finally:
            f.close()
        os.rename(temp_file_name, file_name)
    
    def items_in_section(self, section_name):
        try:
            return self._c.items(section_name)
        except configparser.NoSectionError:
            raise ValueError("Section %r not present." % (section_name,))

    def set_item(self, section_name, key, value):
        if not self._c.has_section(section_name):
            self._c.add_section(section_name)
        self._c.set(section_name, key, value)

    def sync(self):
        """Write back to the file named when we tried to read in init."""
        self.save_to_file(self.file_name_used)

    def __str__(self):
        return self.file_name_used


class Context():
    """A mimic of xdg BaseDirectory, with overridable values that do not
    depend on environment variables."""

    def __init__(self, run_dir, db_dir, config_dir, with_auth=True,
            keyring=gnomekeyring):  # (cache, data, config)

        self.couchdb_log_level = 'notice'

        for d in (run_dir, db_dir, config_dir):
            if not os.path.isdir(d):
                os.makedirs(d, 0700)
            else:
                os.chmod(d, 0700)

        self.run_dir = os.path.realpath(run_dir)
        self.config_dir = os.path.realpath(config_dir)
        self.db_dir = os.path.realpath(db_dir)
        self.with_auth = with_auth
        self.keyring = keyring

        self.file_ini = os.path.join(config_dir, "desktop-couchdb.ini")
        self.file_pid = os.path.join(run_dir, "desktop-couchdb.pid")
        self.file_log = os.path.join(run_dir, "desktop-couchdb.log")
        self.file_stdout = os.path.join(run_dir, "desktop-couchdb.stdout")
        self.file_stderr = os.path.join(run_dir, "desktop-couchdb.stderr")

        # You will need to add -b or -k on the end of this
        self.couch_exec_command = [COUCH_EXE, self.couch_chain_ini_files(), 
                '-p', self.file_pid, 
                '-o', self.file_stdout,
                '-e', self.file_stderr]

        self.configuration = _Configuration(self)

    def sanitize_log_files(self):
        for descr in ("ini", "pid", "log", "stdout", "stderr",):
            f = getattr(self, "file_" + descr)
            if os.path.isfile(f):
                os.chmod(f, 0600)
        for descr in ("log", "stdout", "stderr",):
            f = getattr(self, "file_" + descr)
            for src_ext, dst_ext in ("23", "12", (None, "1")):
                if src_ext is None:
                    srcname = f
                else:
                    srcname = "%s.%s" % (f, src_ext)
                dstname = "%s.%s" % (f, dst_ext)

                try:
                    os.rename(srcname, dstname)
                except OSError, e:
                    if e.errno != errno.ENOENT:
                        logging.exception("failed to back-up %s" % (srcname,))

    def load_config_paths(self):
        """This is xdg/BaseDirectory.py load_config_paths() with hard-code to
        use desktop-couch resource and read from this context."""
        yield self.config_dir
        for config_dir in \
                os.environ.get('XDG_CONFIG_DIRS', '/etc/xdg').split(':'):
            path = os.path.join(config_dir, "desktop-couch")
            if os.path.exists(path):
                yield path

    def couch_chain_ini_files(self):
        process = subprocess.Popen([COUCH_EXE, '-V'], shell=False,
            stdout=subprocess.PIPE)
        line = process.stdout.read().split('\n')[0]
        couchversion = line.split()[-1]

        # Explicitly add default ini file
        ini_files = ["/etc/couchdb/default.ini"]

        # find all ini files in the desktopcouch XDG_CONFIG_DIRS and add them to
        # the chain
        config_dirs = self.load_config_paths()
        # Reverse the list because it's in most-important-first order
        for folder in reversed(list(config_dirs)):
            ini_files.extend([os.path.join(folder, x)
                          for x in sorted(os.listdir(folder))
                          if x.endswith(".ini")])

        if self.file_ini not in ini_files:
            ini_files.append(self.file_ini)

        chain = "-n -a %s " % " -a ".join(ini_files)

        return chain

DEFAULT_CONTEXT = Context(
        os.path.join(xdg_base_dirs.xdg_cache_home, "desktop-couch"),
        xdg_base_dirs.save_data_path("desktop-couch"),
        xdg_base_dirs.save_config_path("desktop-couch"))

class NoOAuthTokenException(Exception):
    def __init__(self, file_name):
        super(Exception, self).__init__()
        self.file_name = file_name
    def __str__(self):
        return "OAuth details were not found in the ini file (%s)" % (
            self.file_name)

def get_admin_credentials(ctx=DEFAULT_CONTEXT):
    return ctx.configuration.items_in_section("admins")[0]  # return first tuple

def get_oauth_tokens(ctx=DEFAULT_CONTEXT):
    """Return the OAuth tokens from the desktop Couch ini file.
       CouchDB OAuth is two-legged OAuth (not three-legged like most OAuth).
       We have one "consumer", defined by a consumer_key and a secret,
       and an "access token", defined by a token and a secret.
       The OAuth signature is created with reference to these and the requested
       URL.
       (More traditional 3-legged OAuth starts with a "request token" which is
       then used to procure an "access token". We do not require this.)
    """

    cf = ctx.configuration
    try:
        oauth_token_secrets = cf.items_in_section("oauth_token_secrets")[0]
        oauth_consumer_secrets = cf.items_in_section("oauth_consumer_secrets")[0]
    except configparser.NoSectionError:
        raise NoOAuthTokenException(cf)
    except IndexError:
        raise NoOAuthTokenException(cf)
    if not oauth_token_secrets or not oauth_consumer_secrets:
        raise NoOAuthTokenException(cf)
    try:
        out = {
            "token": oauth_token_secrets[0],
            "token_secret": oauth_token_secrets[1],
            "consumer_key": oauth_consumer_secrets[0],
            "consumer_secret": oauth_consumer_secrets[1]
        }
    except IndexError:
        raise NoOAuthTokenException(cf)
    return out

def get_bind_address(ctx=DEFAULT_CONTEXT):
    """Retreive a string if it exists, or None if it doesn't."""
    for k, v in ctx.configuration.items_in_section("httpd"):
        if k == "bind_address":
            return v

def set_bind_address(address, ctx=DEFAULT_CONTEXT):
    ctx.configuration.set_item("httpd", "bind_address", address)
    ctx.configuration.sync()
