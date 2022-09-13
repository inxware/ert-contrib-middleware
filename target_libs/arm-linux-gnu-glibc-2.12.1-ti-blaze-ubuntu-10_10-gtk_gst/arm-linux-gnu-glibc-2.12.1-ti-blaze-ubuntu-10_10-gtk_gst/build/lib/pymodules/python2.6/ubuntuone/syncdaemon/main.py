# ubuntuone.syncdaemon.main - main SyncDaemon innards
#
# Author: Guillermo Gonzalez <guillermo.gonzalez@canonical.com>
#         John Lenton <john.lenton@canonical.com>
#
# Copyright 2009 Canonical Ltd.
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
"""SyncDaemon Main."""

import logging
import os
import sys

import dbus

from dbus.mainloop.glib import DBusGMainLoop
from twisted.internet import defer, reactor, task

from ubuntuone.syncdaemon import (
    action_queue,
    config,
    dbus_interface,
    event_queue,
    filesystem_manager,
    hash_queue,
    events_nanny,
    local_rescan,
    sync,
    volume_manager,
)
from ubuntuone import syncdaemon

from ubuntuone.syncdaemon.states import StateManager, QueueManager


class WaitingHelpingHandler(object):
    """
    An auxiliary class that helps wait for events
    """
    def __init__(self, event_queue, waiting_events, waiting_kwargs,
                 result=None):
        self.deferred = defer.Deferred()
        self.event_queue = event_queue
        self.result = result
        self.waiting_events = waiting_events
        self.waiting_kwargs = waiting_kwargs
        event_queue.subscribe(self)

    def handle_default(self, event, *args, **kwargs):
        """Got an event: fire if it's one we want"""
        if event in self.waiting_events:
            for wk, wv in self.waiting_kwargs.items():
                if not (wk in kwargs and kwargs[wk] == wv):
                    return
            self.fire()

    def fire(self):
        """start fire the callback"""
        self.event_queue.unsubscribe(self)
        reactor.callLater(0, lambda: self.deferred.callback(self.result))


class Main(object):
    """The one who executes the syncdaemon."""

    def __init__(self, root_dir, shares_dir, data_dir, partials_dir,
                 host='fs-1.one.ubuntu.com', port=443, dns_srv=None, ssl=True,
                 disable_ssl_verify=False, glib_loop=False,
                 mark_interval=120, dbus_events=False, handshake_timeout=30,
                 shares_symlink_name='Shared With Me',
                 read_limit=None, write_limit=None, throttling_enabled=False,
                 ignore_files=None):
        self.root_dir = root_dir
        self.shares_dir = shares_dir
        self.shares_dir_link = os.path.join(self.root_dir, shares_symlink_name)
        self.data_dir = data_dir
        self.partials_dir = partials_dir
        self.logger = logging.getLogger('ubuntuone.SyncDaemon.Main')
        user_config = config.get_user_config()
        if read_limit is None:
            read_limit = user_config.get_throttling_read_limit()
        if write_limit is None:
            write_limit = user_config.get_throttling_write_limit()
        if not throttling_enabled:
            throttling_enabled = user_config.get_throttling()

        self.vm = volume_manager.VolumeManager(self)
        self.fs = filesystem_manager.FileSystemManager(data_dir,
                                                       partials_dir, self.vm)
        self.event_q = event_queue.EventQueue(self.fs, ignore_files)
        self.fs.register_eq(self.event_q)

        # subscribe VM to EQ
        self.event_q.subscribe(self.vm)
        self.vm.init_root()

        # we don't have the oauth tokens yet, we 'll get them later
        self.action_q = action_queue.ActionQueue(self.event_q, self,
                                                 host, port,
                                                 dns_srv, ssl,
                                                 disable_ssl_verify,
                                                 read_limit, write_limit,
                                                 throttling_enabled)
        self.hash_q = hash_queue.HashQueue(self.event_q)
        events_nanny.DownloadFinishedNanny(self.fs, self.event_q, self.hash_q)

        # call StateManager after having AQ
        self.state_manager = StateManager(self, handshake_timeout)

        self.sync = sync.Sync(self)
        self.lr = local_rescan.LocalRescan(self.vm, self.fs,
                                           self.event_q, self.action_q)

        if not glib_loop:
            self.bus = dbus.SessionBus()
        else:
            loop = DBusGMainLoop(set_as_default=True)
            self.bus = dbus.SessionBus(loop)
        self.dbus_iface = dbus_interface.DBusInterface(self.bus, self,
                                                       send_events=dbus_events)
        self.action_q.content_queue.set_change_notification_cb(
            self.dbus_iface.status.emit_content_queue_changed)
        self.logger.info("Using %r as root dir", self.root_dir)
        self.logger.info("Using %r as data dir", self.data_dir)
        self.logger.info("Using %r as shares root dir", self.shares_dir)
        self.mark = task.LoopingCall(self.log_mark)
        self.mark.start(mark_interval)

    def log_mark(self):
        """Log a "mark" that includes the current AQ state and queue size."""
        self.logger.note("---- MARK (state: %s; queues: metadata: %d; content:"
                         " %d; hash: %d, fsm-cache: hit=%d miss=%d) ----",
                         self.state_manager, len(self.action_q.meta_queue),
                         len(self.action_q.content_queue), len(self.hash_q),
                         self.fs.fs.cache_hits, self.fs.fs.cache_misses)

    def wait_for_nirvana(self, last_event_interval=0.5):
        """Get a deferred that will fire on Nirvana.

        That is, when there are no more events or transfers.
        """
        self.logger.debug('wait_for_nirvana(%s)' % last_event_interval)
        d = defer.Deferred()
        def start():
            """Request the event empty notification."""
            self.logger.debug('starting wait_for_nirvana')
            self.event_q.add_empty_event_queue_callback(callback)
        def callback():
            """Event queue is empty."""
            if not (self.state_manager.state == StateManager.QUEUE_MANAGER
                    and self.state_manager.queues.state == QueueManager.IDLE
                    and not self.action_q.meta_queue
                    and not self.action_q.content_queue
                    and self.hash_q.empty()):
                self.logger.debug("I can't attain Nirvana yet. [state: %s; "
                                  "metadata: %d; content: %d; hash: %d]",
                                  self.state_manager,
                                  len(self.action_q.meta_queue),
                                  len(self.action_q.content_queue),
                                  len(self.hash_q))
                return
            self.logger.debug("Nirvana reached!! I'm a Buddha")
            self.event_q.remove_empty_event_queue_callback(callback)
            d.callback(True)
        reactor.callLater(last_event_interval, start)
        return d

    def wait_for(self, *waiting_events, **waiting_kwargs):
        """defer until event appears"""
        return WaitingHelpingHandler(self.event_q,
                                     waiting_events,
                                     waiting_kwargs).deferred

    def start(self):
        """Setup the daemon to be ready to run."""
        # hook the event queue to the root dir
        self.event_q.push('SYS_INIT_DONE')
        self.event_q.inotify_add_watch(self.root_dir)

    def shutdown(self, with_restart=False):
        """Shutdown the daemon; optionally restart it."""
        self.event_q.push('SYS_USER_DISCONNECT')
        self.event_q.shutdown()
        self.hash_q.shutdown()
        self.state_manager.shutdown()
        self.dbus_iface.shutdown(with_restart)
        self.mark.stop()

    def restart(self):
        """Restart the daemon.

        This ultimately shuts down the daemon and asks dbus to start one again.
        """
        self.quit(exit_value=42, with_restart=True)

    def check_version(self):
        """Check the client protocol version matches that of the server."""
        self.action_q.check_version()

    def authenticate(self):
        """Do the authentication dance."""
        self.action_q.authenticate()

    def set_capabilities(self):
        """Set the capabilities with the server"""
        self.logger.debug("capabilities query: %r", syncdaemon.REQUIRED_CAPS)
        self.action_q.set_capabilities(syncdaemon.REQUIRED_CAPS)

    def server_rescan(self):
        """Do the server rescan."""
        return self.vm.server_rescan()

    def get_rootdir(self):
        """Return the base dir/mount point."""
        return self.root_dir

    def quit(self, exit_value=0, with_restart=False):
        """Shutdown and stop the reactor."""
        self.shutdown(with_restart)
        if reactor.running:
            reactor.stop()
        else:
            sys.exit(exit_value)

    def local_rescan(self):
        """Do the local rescan."""
        self.logger.note("Local rescan starting...")
        d = self.lr.start()

        def _wait_for_hashq():
            """Keep on calling this until the hash_q finishes."""
            if len(self.hash_q):
                self.logger.info("hash queue pending. Waiting for it...")
                reactor.callLater(.1, _wait_for_hashq)
            else:
                self.logger.info("hash queue empty. We are ready!")
                # nudge the action queue into action
                self.event_q.push('SYS_LOCAL_RESCAN_DONE')

        def local_rescan_done(_):
            """After local rescan finished."""
            self.logger.note("Local rescan finished!")
            _wait_for_hashq()

        def stop_the_press(failure):
            """Something went wrong in LR, can't continue."""
            self.logger.error("Local rescan finished with error: %s",
                                                failure.getBriefTraceback())
            self.event_q.push('SYS_UNKNOWN_ERROR')

        d.addCallbacks(local_rescan_done, stop_the_press)
