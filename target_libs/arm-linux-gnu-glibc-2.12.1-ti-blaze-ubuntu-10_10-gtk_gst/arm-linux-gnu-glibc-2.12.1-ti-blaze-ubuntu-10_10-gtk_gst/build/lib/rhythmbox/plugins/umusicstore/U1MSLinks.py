"""The Ubuntu One Music Store Links plugin."""
# Copyright (C) 2010 Canonical, Ltd.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License
# version 3.0 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License version 3.0 for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library. If not, see
# <http://www.gnu.org/licenses/>.
#
# Authored by Stuart Langridge <stuart.langridge@canonical.com>

import urllib, xml.parsers, cgi, os, locale, operator, re, unicodedata
import gtk, gio, glib
import rb, rhythmdb
from xml.dom import minidom
try:
    import gwibber.lib.gtk.widgets
    gwibber_available = True
except ImportError:
    gwibber_available = False

import gettext
from gettext import lgettext as _
gettext.bindtextdomain("U1MSURL", "/usr/share/locale")
gettext.textdomain("U1MSURL")

ui_str = \
"""<ui>
  <menubar name="MenuBar">
    <menu name="ViewMenu" action="View">
      <placeholder name="ViewMenuModePlaceholder">
        <menuitem name="ViewMenuGetU1MSLink" action="GetU1MSLink"/>
      </placeholder>
    </menu>
  </menubar>
  <toolbar name="ToolBar">
    <placeholder name="ToolBarPluginPlaceholder">
      <toolitem name="U1MSLink" action="GetU1MSLink"/>
    </placeholder>
  </toolbar>
</ui>"""

ERRORS = {
    "could-not-detect": _("Unable to automatically detect which "
                "song is currently playing. Please try searching for "
                "the song in the Ubuntu One Music Store."),
    "could-not-find": _("Unable to automatically find the song that is "
                        "currently playing in the Ubuntu One Music Store. "
                        "Please try searching manually."
                        ),
    "could-not-connect": _("Unable to look up the song (couldn't connect "
                "to Ubuntu One). Please check your internet "
                "connection, or try again shortly.")
}

BASE_URL = os.environ.get("U1MSLINK_BASE_URL", "https://one.ubuntu.com")
# getpreferredencoding isn't threadsafe, but we should be OK
PREFERRED_ENCODING = locale.getpreferredencoding()
SCRUBBING_RE = re.compile(r"[\W\s]+", re.UNICODE)
CLEANING_RE = re.compile(r"\s+", re.UNICODE)

def normalize(a):
    """Normalize the given string, to increase the chances of a match"""
    # rule #0 of unicode mangling: first, normalize
    na = unicodedata.normalize('NFKD', a)
    # then, strip everything that's not alphanumeric
    na = re.sub(SCRUBBING_RE, "", na)
    # \w leaves _s
    na = na.replace("_", " ")
    # then, unify spaces
    na = re.sub(CLEANING_RE, " ", na).strip()
    # finally, lowercase
    na = na.lower()
    # if there's anything left, go with that, otherwise rollback
    return na if na else a

def modified_levenshtein(a,b):
    """Calculates the Levenshtein distance between a and b, but counts added
       characters as worth 0 (thus meaning that 'Foo' will match 
       'Foo (Radio Edit)' with a high degree of accuracy.)"""
    a = normalize(a)
    b = normalize(b)
    n, m = len(a), len(b)
    if n > m:
        # Make sure n <= m, to use O(min(n,m)) space
        a,b = b,a
        n,m = m,n
        
    current = range(n+1)
    for i in range(1,m+1):
        previous, current = current, [i]+[0]*n
        for j in range(1,n+1):
            add, delete = previous[j]+1, current[j-1]+1
            change = previous[j-1]
            if a[j-1] != b[i-1]:
                change = change + 1
            # replace add with 10000 to not count adds
            current[j] = min(10000, delete, change)
            
    return current[n]

class U1MSLinkProvider(object):
    """Get a public link for any playing song to buy it in 
       the Ubuntu One Music Store."""

    def __init__(self, find_file):
        self.find_file = find_file

    def activate(self, shell):
        """Plugin startup."""
        # Add button and menu item
        action = gtk.Action("GetU1MSLink", _("Get link to Music Store"),
            _("Get a link for the current song to the Ubuntu One Music Store"),
            "view-u1mslink")
        action.set_icon_name("ubuntuone")
        action.connect("activate", self.get_link, shell)
        manager = shell.get_player().get_property("ui-manager")
        self.action_group = gtk.ActionGroup('U1MSLinkPluginActions')
        self.action_group.add_action(action)
        manager.insert_action_group(self.action_group, 0)
        self.ui_id = manager.add_ui_from_string(ui_str)
        manager.ensure_update()

    def deactivate(self, shell):
        """Plugin shutdown."""
        manager = shell.get_player().get_property('ui-manager')
        manager.remove_ui(self.ui_id)
        manager.remove_action_group(self.action_group)
        manager.ensure_update()

    def dialog_response_cb(self, dialog, response_id):
        self.win.destroy()
        self.win = None

    def get_link(self, action, shell):
        """Get the details of the currently playing song from RB."""
        self.builder = gtk.Builder()
        self.builder.add_from_file(self.find_file("u1msurl.glade"))
        self.win = self.builder.get_object("winMain")
        self.win.connect("response", self.dialog_response_cb)
        self.win.set_transient_for(shell.props.window)
        self.image = self.builder.get_object("imgMain")
        self.image.set_from_icon_name("ubuntuone", gtk.ICON_SIZE_DIALOG)
        self.win.show_all()

        player = shell.get_player()
        entry = player.get_playing_entry()
        if not entry:
            # This shouldn't happen; the button and menu item should
            # be disabled when nothing is playing
            self.display_error('could-not-detect')
            return
        artist = shell.props.db.entry_get(entry, rhythmdb.PROP_ARTIST)
        if not artist:
            # If you're playing something odd, like a radio station, then
            # there is a playing entry but it doesn't have an artist.
            self.display_error('could-not-detect')
            return
        title = shell.props.db.entry_get(entry, rhythmdb.PROP_TITLE)
        album = shell.props.db.entry_get(entry, rhythmdb.PROP_ALBUM)
        # promote them from strings to Unicode
        artist = artist.decode(PREFERRED_ENCODING, 'replace')
        title = title.decode(PREFERRED_ENCODING, 'replace')
        album = album.decode(PREFERRED_ENCODING, 'replace')
        self.builder.get_object("lblProgress").set_markup(
            _("Looking up <i>%s</i> by <i>%s</i> from <i>%s</i>") % (
            cgi.escape(title), cgi.escape(artist), cgi.escape(album)))
        self.lookup_artist(title, artist, album)
    
    def display_error(self, messagename):
        self.builder.get_object("lblProgress").set_markup(ERRORS[messagename])
        self.image.set_from_icon_name("dialog-warning",
                                      gtk.ICON_SIZE_DIALOG)

    def lookup_artist(self, title, artist, album):
        """Use the 7digital public API to look up the artist."""
        base_url = "http://api.7digital.com/1.2/artist/search"
        params = {"q": artist.encode("utf-8", "replace"),
                  "oauth_consumer_key": "canonical",
                  "country":"GB"}
        url = "%s?%s" % (base_url, urllib.urlencode(params))
        self.artist_fetch = gio.File(url)
        self.artist_fetch.read_async(self.artist_fetched, 
            user_data=(title, artist, album))
    
    def artist_fetched(self, gdaemonfile, result, user_data):
        """Artist query is complete"""
        title, artist, album = user_data
        try:
            data = self.artist_fetch.read_finish(result).read()
        except glib.GError:
            self.display_error('could-not-connect')
            return
        try:
            dom = minidom.parseString(data)
        except xml.parsers.expat.ExpatError:
            self.display_error('could-not-connect')
            print "Could not parse returned XML:", data
            return
        artistnodes = dom.getElementsByTagName("artist")
        possibles = []
        for anode in artistnodes:
            names = anode.getElementsByTagName("name")
            if not names:
                continue
            namenode = names[0]
            if not namenode.hasChildNodes():
                continue
            name = namenode.firstChild.nodeValue
            possibles.append((
                anode.getAttribute("id"),
                modified_levenshtein(name, artist),
                name
            ))
        possibles.sort(key=operator.itemgetter(1))
        if not possibles:
            self.display_error('could-not-find')
            print "Couldn't find artist '%s'" % artist
            return
        artist_id = possibles[0][0]
        print "got artist %s" % possibles[0][2]
        self.lookup_track(title, artist, album, artist_id)
    
    def lookup_track(self, title, artist, album, artist_id):
        """Use the 7digital public API to look up the song."""
        # There is no way of searching the 7digital API and saying
        # I *know* the artist ID, please only return me tracks by that
        # artist. So instead we search for the artist name + track name
        # combined, and then later we'll walk through the results ourselves.
        base_url = "http://api.7digital.com/1.2/track/search"
        params = {"q": "%s %s" % (artist.encode("utf-8", "replace"),
                                  title.encode("utf-8", "replace")),
                  "oauth_consumer_key": "canonical",
                  "country":"GB", "pagesize": 50}
        url = "%s?%s" % (base_url, urllib.urlencode(params))
        self.track_fetch = gio.File(url)
        self.track_fetch.read_async(self.track_fetched,
            user_data=(title, artist, album, artist_id))

    def track_fetched(self, gdaemonfile, result, user_data):
        """Track query is complete"""
        title, artist, album, artist_id = user_data
        try:
            data = self.artist_fetch.read_finish(result).read()
        except glib.GError:
            self.display_error('could-not-connect')
            return
        try:
            dom = minidom.parseString(data)
        except xml.parsers.expat.ExpatError:
            self.display_error('could-not-connect')
            print "Could not parse returned XML:", data
            return
        print data
        tracknodes = dom.getElementsByTagName("track")
        possibles = []
        for tnode in tracknodes:
            anodes = tnode.getElementsByTagName("artist")
            if not anodes: continue
            aid = anodes[0].getAttribute("id")
            if aid == artist_id:
                artist_score = 0
                artist_name = artist
            else:
                atnodes = anodes[0].getElementsByTagName("name")
                if not atnodes: continue
                if not atnodes[0].hasChildNodes(): continue
                # a +100 penalty for not being the right artist
                artist_score = 100 + modified_levenshtein(
                    atnodes[0].firstChild.nodeValue, artist)
                artist_name = atnodes[0].firstChild.nodeValue
            nnodes = tnode.getElementsByTagName("title")
            if not nnodes: continue
            if not nnodes[0].hasChildNodes(): continue
            rnodes = tnode.getElementsByTagName("release")
            if not rnodes: continue
            rtnodes = rnodes[0].getElementsByTagName("title")
            if not rtnodes: continue
            if not rtnodes[0].hasChildNodes(): continue
            track_score = modified_levenshtein(
                nnodes[0].firstChild.nodeValue, title)
            album_score = modified_levenshtein(
                rtnodes[0].firstChild.nodeValue, album)
            total_score = track_score + album_score + artist_score
            result = {"track": nnodes[0].firstChild.nodeValue,
                      "album": rtnodes[0].firstChild.nodeValue,
                      "artist": artist_name,
                      "track_score": track_score,
                      "album_score": album_score,
                      "artist_score": album_score,
                      "total_score": total_score,
                      "trackid": tnode.getAttribute("id")}
            if total_score < 150:
                possibles.append(result)
            else:
                print "discarding", result
        possibles.sort(key=operator.itemgetter('total_score'))
        if not possibles:
            print "Didn't find any matches at all"
            self.display_error('could-not-find')
            return
        print possibles
        match = possibles[0]
        print "got a match", match
        dest_url = "%s/music/l/%s/%s" % (
            BASE_URL, match["trackid"], "0") # this 0 should be userid
        self.builder.get_object("lblProgress").set_markup(_("Found <i>%s</i> by <i>%s</i> from <i>%s</i>") % (
            cgi.escape(match["track"]), cgi.escape(match["artist"]), 
            cgi.escape(match["album"])))
        self.builder.get_object("lblURL").set_markup("<big><b>%s</b></big>" % dest_url)
        clipboard = gtk.Clipboard(
            gtk.gdk.display_manager_get().get_default_display(), "CLIPBOARD")
        clipboard.set_text(dest_url)

        if gwibber_available:
            self.tweet_button = gtk.Button("Tweet this link")
            self.tweet_button.connect("clicked", self.gwibber, dest_url, 
                artist, album, title)
            self.win.get_content_area().pack_end(self.tweet_button)
            self.tweet_button.show()
    
    def gwibber(self, button, url, artist, album, title):
        self.pvb = gwibber.lib.gtk.widgets.GwibberPosterVBox()
        self.message = "Currently listening to %s by %s %s" % (title, artist, url)
        self.pvb.input.connect("expose-event", self.gwibber_input_text) 
        self.pvb.button_send.connect("clicked", 
            lambda x: self.builder.get_object("winMain").destroy())
        self.pvb.show_all()
        self.builder.get_object("vbMain").pack_end(self.pvb)
        self.tweet_button.hide()

    def gwibber_input_text(self, *args):
        self.pvb.input.set_text(self.message)

