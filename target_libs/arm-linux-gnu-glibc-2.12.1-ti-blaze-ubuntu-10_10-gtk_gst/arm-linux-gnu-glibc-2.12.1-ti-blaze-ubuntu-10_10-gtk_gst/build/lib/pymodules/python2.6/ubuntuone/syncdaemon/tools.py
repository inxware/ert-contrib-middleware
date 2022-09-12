# ubuntuone.syncdaemon.tools - tools for SyncDaemon
#
# Author: Guillermo Gonzalez <guillermo.gonzalez@canonical.com>
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
""" SyncDaemon Tools """
import dbus
import logging
import time
import sys

from ubuntuone.syncdaemon.dbus_interface import (
    DBUS_IFACE_NAME,
    DBUS_IFACE_STATUS_NAME,
    DBUS_IFACE_SHARES_NAME,
    DBUS_IFACE_FOLDERS_NAME,
    DBUS_IFACE_SYNC_NAME,
    DBUS_IFACE_FS_NAME,
    DBUS_IFACE_PUBLIC_FILES_NAME,
    DBUS_IFACE_CONFIG_NAME,
)
from ubuntuone.syncdaemon.config import get_user_config
from dbus.lowlevel import SignalMessage, MethodCallMessage, ErrorMessage
from dbus.exceptions import DBusException
from twisted.internet import defer, reactor
from twisted.python.failure import Failure


def is_running(bus=None):
    """Check if there is a syncdaemon instance running or if
    the name is registered in the bus.
    """
    if bus is None:
        bus = dbus.SessionBus()
    return DBUS_IFACE_NAME in bus.list_names()

class ErrorSignal(Exception):
    pass


class DBusClient(object):
    """ Low level dbus client. To help testing the DBus interface. """

    def __init__(self, bus, path, interface, destination=DBUS_IFACE_NAME):
        """ create the instance """
        self.bus = bus
        self.path = path
        self.interface = interface
        self.destination = destination

    def send_signal(self, signal, *args):
        """ Send method with *args """
        msg = SignalMessage(self.path, self.interface,
                            signal)
        msg.set_no_reply(True)
        msg.append(*args)
        self.bus.send_message(msg)

    def call_method(self, method, *args, **kwargs):
        """ Call method with *args and **kwargs over dbus"""
        msg = MethodCallMessage(self.destination, self.path, self.interface,
                                method)
        msg.set_no_reply(True)
        # get the signature
        signature = kwargs.get('signature', None)
        if signature is not None:
            msg.append(signature=signature, *args)
        else:
            msg.append(*args)
        #gbet the reply/error handlers
        reply_handler = kwargs.get('reply_handler', None)
        error_handler = kwargs.get('error_handler', None)
        assert error_handler != None

        def parse_reply(message):
            """ handle the reply message"""
            if isinstance(message, ErrorMessage):
                return error_handler(DBusException(
                                    name=message.get_error_name(),
                                    *message.get_args_list()))
            args_list = message.get_args_list(utf8_strings=False,
                                                  byte_arrays=False)
            if reply_handler:
                if len(args_list) == 0:
                    reply_handler(None)
                elif len(args_list) == 1:
                    return reply_handler(args_list[0])
                else:
                    return reply_handler(tuple(args_list))
        return self.bus.send_message_with_reply(msg,
                                                reply_handler=parse_reply)


class SyncDaemonTool(object):
    """ Various utility methods to test/play with the SyncDaemon. """

    def __init__(self, bus):
        self.bus = bus
        self.last_event = 0
        self.delayed_call = None
        self.log = logging.getLogger('ubuntuone.SyncDaemon.SDTool')

    def _get_dict(self, a_dict):
        """ Converts a dict returned by dbus to a dict of strings. """
        str_dict = {}
        for key in a_dict:
            str_dict[key] = unicode(a_dict[key])
        return str_dict

    def wait_connected(self):
        """ Wait until syncdaemon is connected to the server. """
        self.log.debug('wait_connected')
        d = defer.Deferred()
        def check_connection_status():
            """ Check if the daemon is up and running. """
            # check if the syncdaemon is running
            # catch all errors, pylint: disable-msg=W0703
            try:
                self.bus.get_object(DBUS_IFACE_NAME, '/',
                                    follow_name_owner_changes=True)
                self.log.debug('wait_connected: Done!')
                d.callback(True)
            except Exception, e:
                self.log.debug('Not connected: %s', e)
                d.errback()

        reactor.callLater(.5, check_connection_status)
        return d

    def get_current_downloads(self):
        """ Return a deferred that 'll be called with the list
        of current downloads
        """
        d = defer.Deferred()
        def current_downloads():
            """ Call Status.current_downloads """
            status_client = DBusClient(self.bus, '/status',
                                       DBUS_IFACE_STATUS_NAME)
            status_client.call_method('current_downloads',
                                      reply_handler=reply_handler,
                                      error_handler=d.errback)

        def reply_handler(downloads):
            """ current downloads callback """
            downloads_str = []
            for download in downloads:
                downloads_str.append(self._get_dict(download))
            d.callback(downloads_str)

        reactor.callLater(0, current_downloads)
        return d

    def wait_all_downloads(self, verbose=False):
        """ Wait until there is no more pending downloads """
        self.log.debug('wait_all_downloads')
        d = self.get_current_downloads()
        def reply_handler(downloads):
            """ Check if the are downloads in progress, and reschelude a
            new check if there is at least one.
            """
            if verbose:
                sys.stdout.write(', %s' % str(len(downloads)))
                sys.stdout.flush()
            if len(downloads) > 0:
                self.log.debug('wait_all_downloads: %d', len(downloads))
                return self.get_current_downloads()
            else:
                self.log.debug('wait_all_downloads: No more downloads')
                return True

        if verbose:
            sys.stdout.write('\nchecking current downloads')
            sys.stdout.flush()
        d.addCallback(reply_handler)
        return d

    def get_current_uploads(self):
        """ Return a deferred that 'll be called with the list
        of current uploads
        """
        d = defer.Deferred()
        def current_uploads():
            """ Call Status.current_uploads """
            status_client = DBusClient(self.bus, '/status',
                                       DBUS_IFACE_STATUS_NAME)
            status_client.call_method('current_uploads',
                                      reply_handler=reply_handler,
                                      error_handler=d.errback)

        def reply_handler(uploads):
            """ reply handler """
            uploads_str = []
            for upload in uploads:
                uploads_str.append(self._get_dict(upload))
            d.callback(uploads_str)

        reactor.callLater(0, current_uploads)
        return d

    def wait_all_uploads(self, verbose=False):
        """ Wait until there is no more pending uploads """
        self.log.debug('wait_all_uploads')
        d = self.get_current_uploads()

        def reply_handler(uploads):
            """ Check if the are downloads in progress, and reschelude a
            new check if there is at least one.
            """
            if verbose:
                sys.stdout.write(', %s' % str(len(uploads)))
                sys.stdout.flush()
            if len(uploads) > 0:
                self.log.debug('wait_all_uploads: %d', len(uploads))
                return self.get_current_uploads()
            else:
                self.log.debug('wait_all_uploads: No more uploads')
                return True

        if verbose:
            sys.stdout.write('\nchecking current uploads')
            sys.stdout.flush()

        d.addCallback(reply_handler)
        return d

    def wait_no_more_events(self, last_event_interval, verbose=False):
        """ Wait until no more events are fired by the syncdaemon. """
        self.log.debug('wait_no_more_events')
        d = defer.Deferred()
        def check_last_event():
            """ Check if the daemon is connected and we didn't received event
            in the last_event_interval
            """
            current_time = time.time()
            if self.last_event and \
               current_time - self.last_event < last_event_interval:
                # keep it running in case this is the last event
                self.log.debug('rescheduling wait_no_more_events')
                if not self.delayed_call.active():
                    self.delayed_call = reactor.callLater(last_event_interval,
                                                          check_last_event)
                else:
                    self.delayed_call.reset(last_event_interval)
            else:
                self.log.debug('wait_no_more_events: No more events!')
                d.callback(True)

        if verbose:
            sys.stdout.write("Listening events")
            sys.stdout.flush()
        def event_handler(event_dict):
            """ update last_event and run checks """
            self.last_event = time.time()
            self.log.debug('wait_no_more_events - new event: %s - %s',
                           event_dict['event_name'], str(self.last_event))
            if verbose:
                sys.stdout.write('.')
                sys.stdout.flush()
            if self.delayed_call.active():
                self.delayed_call.reset(last_event_interval)

        self.bus.add_signal_receiver(event_handler, signal_name='Event')
        def cleanup(result):
            """ remove the signal handler """
            self.bus.remove_signal_receiver(event_handler,
                                             signal_name='Event')
            return result
        d.addBoth(cleanup)
        # in case the daemon already reached nirvana
        self.delayed_call = reactor.callLater(last_event_interval,
                                              check_last_event)
        return d

    def wait_for_nirvana(self, last_event_interval=5, verbose=False):
        """ Wait until the syncdaemon reachs nirvana. This is when there are:
            - the syncdaemon is connected
            - 0 transfers inprogress
            - no more events are fired in the event queue
        @param last_event_interval: the seconds to wait to determine that there
        is no more events in the queue and the daemon reached nirvana
        """
        self.log.debug('wait_for_nirvana')
        sd_client = DBusClient(self.bus, '/', DBUS_IFACE_SYNC_NAME)
        d = defer.Deferred()
        sd_client.call_method('wait_for_nirvana', last_event_interval,
                              reply_handler=d.callback,
                              error_handler=d.errback)
        return d

    def accept_share(self, share_id):
        """ Accept the share with id: share_id. """
        self.log.debug('accept_share(%s)', share_id)
        shares_client = DBusClient(self.bus, '/shares', DBUS_IFACE_SHARES_NAME)
        d = self.wait_for_signal('ShareAnswerResponse',
                                 lambda info: info['volume_id']==share_id)
        shares_client.call_method('accept_share', share_id,
                                  reply_handler=lambda _: None,
                                  error_handler=d.errback)
        return d

    def reject_share(self, share_id):
        """ Reject the share with id: share_id. """
        self.log.debug('reject_share(%s)', share_id)
        shares_client = DBusClient(self.bus, '/shares', DBUS_IFACE_SHARES_NAME)
        d = self.wait_for_signal('ShareAnswerResponse',
                                    lambda info: info['volume_id']==share_id)
        shares_client.call_method('reject_share', share_id,
                                  reply_handler=lambda _: None,
                                  error_handler=d.errback)
        return d

    def get_shares(self):
        """ Get the list of shares (accepted or not) """
        self.log.debug('get_shares')
        shares_client = DBusClient(self.bus, '/shares', DBUS_IFACE_SHARES_NAME)
        d = defer.Deferred()
        def reply_handler(results):
            """ get_shares reply handler. """
            shares = []
            for result in results:
                shares.append(self._get_dict(result))
            self.log.debug('shares: %r', shares)
            d.callback(shares)

        shares_client.call_method('get_shares',
                                  reply_handler=reply_handler,
                                  error_handler=d.errback)
        return d

    def refresh_shares(self):
        """ Call refresh_shares method via DBus.
        Request a refresh of share list to the server.
        """
        self.log.debug('refresh_shares')
        shares_client = DBusClient(self.bus, '/shares', DBUS_IFACE_SHARES_NAME)
        d = defer.Deferred()
        shares_client.call_method('refresh_shares',
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def offer_share(self, path, username, name, access_level):
        """ Offer a share at the specified path to user with id: username. """
        self.log.debug('offer_share(%s, %s, %s, %s)',
                   path, username, name, access_level)
        shares_client = DBusClient(self.bus, '/shares', DBUS_IFACE_SHARES_NAME)
        d = defer.Deferred()
        shares_client.call_method('create_share', path, username,
                                  name, access_level,
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def list_shared(self):
        """ get the list of the shares "shared"/created/offered. """
        self.log.debug('list_shared')
        shares_client = DBusClient(self.bus, '/shares', DBUS_IFACE_SHARES_NAME)
        d = defer.Deferred()
        def reply_handler(results):
            """ get_shares reply handler. """
            shares = []
            for result in results:
                shares.append(self._get_dict(result))
            self.log.debug('shared: %r', shares)
            d.callback(shares)
        shares_client.call_method('get_shared',
                                  reply_handler=reply_handler,
                                  error_handler=d.errback)
        return d

    def wait_for_signals(self, signal_ok, signal_error,
                         dbus_iface=DBUS_IFACE_FOLDERS_NAME):
        """wait for one of the specified DBus signals
        this returns a deferred

        @param signal_ok: this will fire the deferred's callback
        @param signal_error: the will fire the deferred's errback
        @param dbus_iface: the interface the signal belongs to
        """

        d = defer.Deferred()
        def signal_handler(*args, **kwargs):
            """Signal handler"""
            member = kwargs.get('member', None)
            if member == signal_ok:
                d.callback(args)
            elif member == signal_error:
                d.errback(ErrorSignal(signal_error, args))

        # register signal handlers for each kind of error
        match = self.bus.add_signal_receiver(
            signal_handler, member_keyword='member', dbus_interface=dbus_iface)
        def remove_signal_receiver(r):
            # cleanup the signal receivers
            self.bus.remove_signal_receiver(match, dbus_interface=dbus_iface)
            return r

        d.addBoth(remove_signal_receiver)
        return d

    def create_folder(self, path):
        """Create a user defined folder in the specified path."""
        self.log.debug('create_folder')
        folders_client = DBusClient(self.bus, '/folders', DBUS_IFACE_FOLDERS_NAME)

        d = self.wait_for_signals('FolderCreated', 'FolderCreateError')
        folders_client.call_method('create', path,
                                  reply_handler=lambda _: None,
                                  error_handler=d.errback)
        return d

    def delete_folder(self, folder_id):
        """Delete a user defined folder given its id."""
        self.log.debug('delete_folder')
        folders_client = DBusClient(self.bus, '/folders', DBUS_IFACE_FOLDERS_NAME)

        d = self.wait_for_signals('FolderDeleted', 'FolderDeleteError')
        folders_client.call_method('delete', folder_id,
                                  reply_handler=lambda _: None,
                                  error_handler=d.errback)
        return d

    def subscribe_folder(self, folder_id):
        """Subscribe to a user defined folder given its id."""
        self.log.debug('subscribe_folder')
        folders_client = DBusClient(self.bus, '/folders', DBUS_IFACE_FOLDERS_NAME)

        d = self.wait_for_signals('FolderSubscribed', 'FolderSubscribeError')
        folders_client.call_method('subscribe', folder_id,
                                  reply_handler=lambda _: None,
                                  error_handler=d.errback)
        return d

    def unsubscribe_folder(self, folder_id):
        """Unsubscribe from a user defined folder given its id."""
        self.log.debug('unsubscribe_folder')
        folders_client = DBusClient(self.bus, '/folders', DBUS_IFACE_FOLDERS_NAME)

        d = self.wait_for_signals('FolderUnSubscribed', 'FolderUnSubscribeError')
        folders_client.call_method('unsubscribe', folder_id,
                                  reply_handler=lambda _: None,
                                  error_handler=d.errback)
        return d

    def get_folders(self):
        """Return the list of folders (a list of dicts)."""
        self.log.debug('get_folders')
        folders_client = DBusClient(self.bus, '/folders', DBUS_IFACE_FOLDERS_NAME)
        d = defer.Deferred()
        def reply_handler(results):
            """ get_folders reply handler. """
            folders = []
            for result in results:
                folders.append(self._get_dict(result))
            self.log.debug('folders: %r', folders)
            d.callback(folders)
        folders_client.call_method('get_folders',
                                  reply_handler=reply_handler,
                                  error_handler=d.errback)
        return d

    def get_folder_info(self, path):
        """Call the get_info method for a UDF path."""
        self.log.debug('get_info')
        client = DBusClient(self.bus, '/folders', DBUS_IFACE_FOLDERS_NAME)
        d = defer.Deferred()
        client.call_method('get_info', path,
                           reply_handler=d.callback,
                           error_handler=d.errback)
        return d

    def get_metadata(self, path):
        """ calls the exposed mtehod FileSystem.get_metadata using DBus. """
        self.log.debug('get_metadata(%s)', path)
        fs_client = DBusClient(self.bus, '/filesystem', DBUS_IFACE_FS_NAME)
        d = defer.Deferred()
        fs_client.call_method('get_metadata', path,
                              reply_handler=d.callback,
                              error_handler=d.errback)
        return d

    @defer.inlineCallbacks
    def change_public_access(self, path, is_public):
        """Change the public access for a given path."""
        self.log.debug('change_public_access(%s)', path)
        fs_client = DBusClient(self.bus, '/filesystem', DBUS_IFACE_FS_NAME)
        d = defer.Deferred()
        fs_client.call_method('get_metadata', path,
                              reply_handler=d.callback,
                              error_handler=d.errback)
        metadata = yield d

        pf_client = DBusClient(
            self.bus, '/publicfiles', DBUS_IFACE_PUBLIC_FILES_NAME)
        d = self.wait_for_signals(
            'PublicAccessChanged', 'PublicAccessChangeError',
            dbus_iface=DBUS_IFACE_PUBLIC_FILES_NAME)
        pf_client.call_method('change_public_access', metadata['share_id'],
                              metadata['node_id'], is_public,
                              reply_handler=lambda _: None,
                              error_handler=d.errback)
        (file_info,) = yield d
        defer.returnValue(file_info)

    def quit(self):
        """quit the syncdaemon"""
        self.log.debug('quit')
        # avoid triggering dbus activation while calling quit
        if not is_running(self.bus):
            return defer.succeed(None)
        sd_client = DBusClient(self.bus, '/', DBUS_IFACE_SYNC_NAME)
        d = defer.Deferred()
        sd_client.call_method('quit',
                           reply_handler=d.callback,
                           error_handler=d.errback)
        def check(r):
            """wait 0.5 sec to return, to allow syncdaemon to shutdown"""
            d1 = defer.Deferred()
            reactor.callLater(0.5, d1.callback, r)
            return d1
        d.addCallback(check)
        return d

    def wait_for_signal(self, signal_name, filter):
        """wait for the specified DBus signal (the first received).
        'filter' is used to fire the deferred callback.

        @param signal_name: the signal name
        @param filter: a callable to filter signal, must return True
        """
        d = defer.Deferred()
        def signal_handler(result):
            """handle the signals and fires the call/errback"""
            try:
                if filter(result) and not d.called:
                    d.callback(result)
                # catch all exceptions, pylint: disable-msg=W0703
            except Exception, e:
                d.errback(Failure(e))

        match = self.bus.add_signal_receiver(signal_handler,
                                             signal_name=signal_name)
        def cleanup(result):
            """remove the signal receiver from the bus."""
            self.bus.remove_signal_receiver(match)
            return result
        d.addCallback(cleanup)
        return d

    def connect(self):
        """Connect syncdaemon"""
        sd_client = DBusClient(self.bus, '/', DBUS_IFACE_SYNC_NAME)
        d = defer.Deferred()
        sd_client.call_method('connect',
                              reply_handler=d.callback,
                              error_handler=d.errback)
        return d

    def disconnect(self):
        """Disconnect syncdaemon"""
        sd_client = DBusClient(self.bus, '/', DBUS_IFACE_SYNC_NAME)
        d = defer.Deferred()
        sd_client.call_method('disconnect',
                              reply_handler=d.callback,
                              error_handler=d.errback)
        return d

    def get_status(self):
        """Get the current_status dict"""
        d = defer.Deferred()
        status_client = DBusClient(self.bus, '/status',
                                   DBUS_IFACE_STATUS_NAME)
        def reply_handler(status):
            """The reply handler"""
            state_dict = self._get_dict(status)
            state_dict['is_connected'] = bool(state_dict['is_connected'])
            state_dict['is_online'] = bool(state_dict['is_online'])
            state_dict['is_error'] = bool(state_dict['is_error'])
            d.callback(state_dict)
        status_client.call_method('current_status',
                                  reply_handler=reply_handler,
                                  error_handler=d.errback)
        return d

    def waiting_metadata(self):
        """Return a description of the waiting metadata queue elements."""
        d = defer.Deferred()
        status_client = DBusClient(self.bus, '/status',
                                   DBUS_IFACE_STATUS_NAME)
        status_client.call_method('waiting_metadata',
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def waiting_content(self):
        """Returns the waiting content queue elements."""
        d = defer.Deferred()
        status_client = DBusClient(self.bus, '/status',
                                   DBUS_IFACE_STATUS_NAME)
        status_client.call_method('waiting_content',
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def schedule_next(self, share_id, node_id):
        """Make the command on the given share_id and node_id be next in the
        queue of waiting commands.
        """
        d = defer.Deferred()
        status_client = DBusClient(self.bus, '/status',
                                   DBUS_IFACE_STATUS_NAME)
        status_client.call_method('schedule_next', share_id, node_id,
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def start(self):
        """Start syncdaemon using the StartServiceByName method
        if it's not running.
        """
        if not is_running(self.bus):
            wait_d = self.wait_for_signal('StatusChanged', lambda x: x)
            d = defer.Deferred()
            bus_client = DBusClient(self.bus, dbus.bus.BUS_DAEMON_PATH,
                                    dbus.bus.BUS_DAEMON_IFACE,
                                    dbus.bus.BUS_DAEMON_NAME)
            bus_client.call_method('StartServiceByName',
                                   DBUS_IFACE_NAME, 0,
                                   signature='su',
                                   reply_handler=d.callback,
                                   error_handler=d.errback)
            d.addCallback(lambda _: wait_d)
            return d
        else:
            return defer.succeed(None)

    def get_throttling_limits(self):
        """Return a dict with the read and write limits."""
        d = defer.Deferred()
        config_client = DBusClient(self.bus, '/config',
                                   DBUS_IFACE_CONFIG_NAME)
        config_client.call_method('get_throttling_limits',
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def set_throttling_limits(self, read_limit, write_limit):
        """Set the read and write limits."""
        d = defer.Deferred()
        config_client = DBusClient(self.bus, '/config',
                                   DBUS_IFACE_CONFIG_NAME)
        config_client.call_method('set_throttling_limits',
                                  read_limit, write_limit,
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def is_throttling_enabled(self):
        """Check if throttling is enabled."""
        d = defer.Deferred()
        config_client = DBusClient(self.bus, '/config',
                                   DBUS_IFACE_CONFIG_NAME)
        config_client.call_method('bandwidth_throttling_enabled',
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d


    def enable_throttling(self, enabled):
        """Enable/disable throttling."""
        d = defer.Deferred()
        config_client = DBusClient(self.bus, '/config',
                                   DBUS_IFACE_CONFIG_NAME)
        if enabled:
            meth = 'enable_bandwidth_throttling'
        else:
            meth = 'disable_bandwidth_throttling'
        config_client.call_method(meth,
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def is_files_sync_enabled(self):
        """Check if files sync is enabled."""
        self.log.debug('is_files_sync_enabled')
        return get_user_config().get_files_sync_enabled()

    def enable_files_sync(self, enabled):
        """Enable/disable files sync."""
        self.log.debug('enable_files_sync %d', enabled)
        config = get_user_config()
        if config.get_files_sync_enabled():
            d = defer.Deferred()
            config_client = DBusClient(self.bus, '/config',
                                       DBUS_IFACE_CONFIG_NAME)
            config_client.call_method('set_files_sync_enabled',
                                      enabled,
                                      reply_handler=d.callback,
                                      error_handler=d.errback)
            config.set_files_sync_enabled(False)
            return d
        else:
            if enabled:
                config.set_files_sync_enabled(True)
                config.save()
                self.start()
            return defer.succeed(None)

    def refresh_volumes(self):
        """Call refresh_volumes method via DBus.

        Request the volumes list to the server.
        """
        self.log.debug('refresh_volumes')
        shares_client = DBusClient(self.bus, '/folders', DBUS_IFACE_FOLDERS_NAME)
        d = defer.Deferred()
        shares_client.call_method('refresh_volumes',
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def rescan_from_scratch(self, volume_id):
        """Call rescan_from_scratch via DBus.

        Request a rescan from scratch for volume_id.
        """
        self.log.debug('rescan_from_scratch %r', volume_id)
        shares_client = DBusClient(self.bus, '/', DBUS_IFACE_SYNC_NAME)
        d = defer.Deferred()
        shares_client.call_method('rescan_from_scratch', volume_id,
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d

    def get_dirty_nodes(self):
        """Call get_dirty_nodes via DBus.

        Return the list of dirty nodes.
        """
        self.log.debug('get_dirty_nodes')
        fs_client = DBusClient(self.bus, '/filesystem', DBUS_IFACE_FS_NAME)
        d = defer.Deferred()
        fs_client.call_method('get_dirty_nodes',
                                  reply_handler=d.callback,
                                  error_handler=d.errback)
        return d


# callbacks used by u1sdtool script

def show_shared(shares, out):
    """ Callback that prints the list of shared shares"""
    if len(shares) == 0:
        out.write("No shared\n")
    else:
        out.write("Shared list:\n")
    for share in shares:
        msg_template = '  id=%s name=%s accepted=%s ' + \
                'access_level=%s to=%s path=%s\n'
        out.write(msg_template % (share['volume_id'], share['name'],
                                  bool(share['accepted']), share['access_level'],
                                  share['other_username'],
                                  share['path']))


def show_folders(folders, out):
    """ Callback that prints the list of user defined folders"""
    if len(folders) == 0:
        out.write("No folders\n")
    else:
        out.write("Folder list:\n")
    for folder in folders:
        msg_template = '  id=%s subscribed=%s path=%s\n'
        out.write(msg_template % (folder['volume_id'],
                                  bool(folder['subscribed']),
                                  folder['path']))


def show_error(error, out):
    """Format an error when things go wrong"""
    try:
        raise error.value
    except ErrorSignal:
        signal, (args, retval) = error.value.args
        msg_template = u"%s: %s (%s)\n"
        fmtd_args = u", ".join("%s=%s"%(k, v) for k, v in args.items())
        out.write( msg_template % (signal, retval, fmtd_args) )


def show_shares(shares, out):
    """ Callback that print the list of shares"""
    if len(shares) == 0:
        out.write("No shares\n")
    else:
        out.write("Shares list:\n")
    for share in shares:
        out.write(' id=%s name=%s accepted=%s access_level=%s from=%s\n' % \
                  (share['volume_id'], share['name'], bool(share['accepted']),
                   share['access_level'], share['other_username']))


def show_path_info(result, path, out):
    """ print the path info to stdout"""
    out_encoding = out.encoding
    if out_encoding is None:
        out_encoding = 'utf-8'
    out.write(" File: %s\n" % path.decode(out_encoding, 'replace'))
    keys = list(result.keys())
    keys.sort()
    for key in keys:
        out.write("  %s: %s\n" % (key, result[key]))


def show_uploads(uploads, out):
    """ print the uploads to stdout"""
    if uploads:
        out.write("Current uploads:\n")
    else:
        out.write("Current uploads: 0\n")
    for upload in uploads:
        out.write("  path: %s\n" % upload['path'])
        out.write("    deflated size: %s\n" % \
                  upload.get('deflated_size', 'N/A'))
        out.write("    bytes written: %s\n" % upload['n_bytes_written'])


def show_downloads(downloads, out):
    """ print the downloas to stdout"""
    if downloads:
        out.write("Current downloads:\n")
    else:
        out.write("Current downloads: 0\n")
    for download in downloads:
        out.write("  path: %s\n" % download['path'])
        out.write("    deflated size: %s\n" % \
                  download.get('deflated_size', 'N/A'))
        out.write("    bytes read: %s\n" % download['n_bytes_read'])


def show_state(state_dict, out):
    """Print the state to out."""
    out.write("State: %s\n" % state_dict.pop('name'))
    for k, v in sorted(state_dict.items()):
        out.write("    %s: %s\n" % (k, v))
    out.write("\n")


def show_waiting_metadata(waiting_ops, out):
    """Print the waiting_metadata result.

    We receive an unordered dict, but always try to show first the
    share_id, then the node_id, then the path, and the rest in
    alphabetical order.
    """
    for op_name, op_data in waiting_ops:
        out.write(' ' + op_name)
        if not op_data:
            out.write('\n')
            continue

        # custom
        attributes = []
        for attr in ('share_id', 'node_id', 'path'):
            if attr in op_data:
                attributes.append(u"%s='%s'" % (attr, op_data.pop(attr)))

        # the rest, ordered
        for attr in sorted(op_data):
            attributes.append(u"%s='%s'" % (attr, op_data[attr]))

        out.write("(%s)\n" % (u', '.join(attributes),))


def show_waiting_content(waiting_ops, out):
    """Print the waiting_content result"""
    value_tpl = "operation='%(operation)s' node_id='%(node)s' " + \
            "share_id='%(share)s' path='%(path)s'"
    for value in waiting_ops:
        str_value = value_tpl % value
        out.write("%s\n" % str_value)


def show_public_file_info(file_info, out):
    """Print the public access information for a file."""
    if file_info['is_public']:
        out.write("File is published at %s\n" % file_info['public_url'])
    else:
        out.write("File is not published\n")


def show_dirty_nodes(nodes, out):
    """Print the list of dirty nodes."""
    if not nodes:
        out.write(" No dirty nodes.\n")
        return
    out_encoding = out.encoding
    if out_encoding is None:
        out_encoding = 'utf-8'
    node_line_tpl = "mdid: %(mdid)s volume_id: %(share_id)s " + \
            "node_id: %(node_id)s is_dir: %(is_dir)s path: %(path)s\n"
    out.write(" Dirty nodes:\n")
    for node in nodes:
        node['path'] = node['path'].decode(out_encoding, 'replace')
        out.write(node_line_tpl % node)

