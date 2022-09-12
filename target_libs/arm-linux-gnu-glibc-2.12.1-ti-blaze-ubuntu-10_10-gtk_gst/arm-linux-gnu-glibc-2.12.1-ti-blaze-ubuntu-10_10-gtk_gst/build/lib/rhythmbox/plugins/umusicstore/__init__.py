"""The Ubuntu One Music Store Rhythmbox plugin."""
# Copyright (C) 2009 Canonical, Ltd.
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

import gconf
from MusicStoreWidget import U1MusicStoreWidget
from U1MSLinks import U1MSLinkProvider

U1_FIRST_TIME_FLAG_ENTRY = "/apps/rhythmbox/plugins/umusicstore/first_time_flag"

import rb

class U1MusicStorePlugin (rb.Plugin):
    """The Ubuntu One Music Store."""

    def __init__(self):
        rb.Plugin.__init__(self)
        
        # The Music Store itself
        self.music_store_widget = U1MusicStoreWidget(
            plugin=self, find_file=self.find_file)
        
        # The Links button
        self.music_store_links_provider = U1MSLinkProvider(
            find_file=self.find_file)

    def activate(self, shell):
        """Plugin startup."""
        self.music_store_widget.activate(shell)
        self.music_store_links_provider.activate(shell)

	# Select the source if it's the first time
        conf_client = gconf.client_get_default ()
        if not conf_client.get_bool(U1_FIRST_TIME_FLAG_ENTRY):
            shell.props.sourcelist.select(self.music_store_widget.source)
            conf_client.set_bool(U1_FIRST_TIME_FLAG_ENTRY, True)

    def deactivate(self, shell):
        """Plugin shutdown."""
        self.music_store_widget.deactivate(shell)
        self.music_store_links_provider.deactivate(shell)
