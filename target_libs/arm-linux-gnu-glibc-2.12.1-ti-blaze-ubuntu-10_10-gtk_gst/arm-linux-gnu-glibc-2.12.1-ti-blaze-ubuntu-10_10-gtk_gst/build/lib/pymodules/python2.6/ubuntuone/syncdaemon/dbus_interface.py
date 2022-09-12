# ubuntuone.syncdaemon.dbus_interface - DBus Interface
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
""" DBUS interface module """

import os
import dbus.service
import logging
import uuid

from dbus import DBusException
from itertools import groupby, chain
from xml.etree import ElementTree

from twisted.internet import defer
from twisted.python.failure import Failure
from ubuntu_sso import DBUS_BUS_NAME, DBUS_IFACE_CRED_NAME, DBUS_CRED_PATH

from ubuntuone.syncdaemon.event_queue import EVENTS
from ubuntuone.syncdaemon.interfaces import IMarker
from ubuntuone.syncdaemon import config
from ubuntuone.syncdaemon.volume_manager import Share, UDF, VolumeDoesNotExist

# Disable the "Invalid Name" check here, as we have lots of DBus style names
# pylint: disable-msg=C0103

DBUS_IFACE_NAME = 'com.ubuntuone.SyncDaemon'
DBUS_IFACE_UNIQUE_NAME = DBUS_IFACE_NAME + '.UniqueInstance'
DBUS_IFACE_SYNC_NAME = DBUS_IFACE_NAME + '.SyncDaemon'
DBUS_IFACE_STATUS_NAME = DBUS_IFACE_NAME+'.Status'
DBUS_IFACE_EVENTS_NAME = DBUS_IFACE_NAME+'.Events'
DBUS_IFACE_FS_NAME = DBUS_IFACE_NAME+'.FileSystem'
DBUS_IFACE_SHARES_NAME = DBUS_IFACE_NAME+'.Shares'
DBUS_IFACE_CONFIG_NAME = DBUS_IFACE_NAME+'.Config'
DBUS_IFACE_FOLDERS_NAME = DBUS_IFACE_NAME+'.Folders'
DBUS_IFACE_PUBLIC_FILES_NAME = DBUS_IFACE_NAME + '.PublicFiles'

# NetworkManager State constants
NM_STATE_UNKNOWN = 0
NM_STATE_ASLEEP = 1
NM_STATE_CONNECTING = 2
NM_STATE_CONNECTED = 3
NM_STATE_DISCONNECTED = 4
# NM state -> events mapping
NM_STATE_EVENTS = {NM_STATE_CONNECTED: 'SYS_NET_CONNECTED',
                   NM_STATE_DISCONNECTED: 'SYS_NET_DISCONNECTED'}

logger = logging.getLogger("ubuntuone.SyncDaemon.DBus")


try:
    from ubuntuone import clientdefs
    APP_NAME = clientdefs.APP_NAME
    TC_URL = clientdefs.TC_URL
    DESCRIPTION = clientdefs.DESCRIPTION
except ImportError:
    APP_NAME = "Ubuntu One (ImportError mode)"
    TC_URL = "https://one.ubuntu.com/terms"
    DESCRIPTION = "Stub used since the real values can't be imported."
    logger.warning('Can not import APP_NAME, TC_URL, DESCRIPTION.'
                   'Using stub values (%s, %s, %s).',
                   APP_NAME, TC_URL, DESCRIPTION)


class NoAccessToken(Exception):
    """The access token could not be retrieved."""


def get_classname(thing):
    """
    Get the clasname of the thing.
    If we could forget 2.5, we could do attrgetter('__class__.__name__')
    Alas, we can't forget it yet.
    """
    return thing.__class__.__name__

def bool_str(value):
    """Return a string value that can be converted back to bool."""
    return 'True' if value else ''

def _get_share_dict(share):
    """Get a dict with all the attributes of: share."""
    share_dict = share.__dict__.copy()
    for k, v in share_dict.items():
        if v is None:
            share_dict[unicode(k)] = ''
        elif k == 'path':
            share_dict[unicode(k)] = v.decode('utf-8')
        elif k == 'accepted':
            share_dict[unicode(k)] = bool_str(v)
        else:
            share_dict[unicode(k)] = unicode(v)
    return share_dict

def _get_udf_dict(udf):
    """Get a dict with all the attributes of: udf."""
    udf_dict = udf.__dict__.copy()
    for k, v in udf_dict.items():
        if v is None:
            udf_dict[unicode(k)] = ''
        elif k == 'subscribed':
            udf_dict[unicode(k)] = bool_str(v)
        elif k == 'path':
            udf_dict[unicode(k)] = v.decode('utf-8')
        elif k == 'suggested_path' and isinstance(v, str):
            udf_dict[unicode(k)] = v.decode('utf-8')
        else:
            udf_dict[unicode(k)] = unicode(v)
    return udf_dict


class DBusExposedObject(dbus.service.Object):
    """ Base class that provides some helper methods to
    DBus exposed objects.
    """
    #__metaclass__ = InterfaceType

    def __init__(self, bus_name, path):
        """ creates the instance. """
        dbus.service.Object.__init__(self, bus_name=bus_name,
                                     object_path=self.path)

    @dbus.service.signal(DBUS_IFACE_SYNC_NAME, signature='sa{ss}')
    def SignalError(self, signal, extra_args):
        """ An error ocurred while trying to emit a signal. """
        pass

    def emit_signal_error(self, signal, extra_args):
        """ emit's a Error signal. """
        self.SignalError(signal, extra_args)

    @classmethod
    def _add_docstring(cls, func, reflection_data):
        """add <docstring> tag to reflection_data if func.__doc__ isn't None."""
        # add docstring element
        if getattr(func, '__doc__', None) is not None:

            element = ElementTree.fromstring(reflection_data)
            doc = element.makeelement('docstring', dict())
            data = '<![CDATA[' + func.__doc__ + ']]>'
            doc.text = '%s'
            element.insert(0, doc)
            return ElementTree.tostring(element) % data
        else:
            return reflection_data

    @classmethod
    def _reflect_on_method(cls, func):
        """override _reflect_on_method to provide an extra <docstring> element
        in the xml.
        """
        reflection_data = dbus.service.Object._reflect_on_method(func)
        reflection_data = cls._add_docstring(func, reflection_data)
        return reflection_data

    @classmethod
    def _reflect_on_signal(cls, func):
        reflection_data = dbus.service.Object._reflect_on_signal(func)
        reflection_data = cls._add_docstring(func, reflection_data)
        return reflection_data


class Status(DBusExposedObject):
    """ Represent the status of the syncdaemon """

    def __init__(self, bus_name, dbus_iface):
        """ Creates the instance.

        @param bus: the BusName of this DBusExposedObject.
        """
        self.dbus_iface = dbus_iface
        self.action_queue = dbus_iface.action_queue
        self.fs_manager = dbus_iface.fs_manager
        self.path = '/status'
        DBusExposedObject.__init__(self, bus_name=bus_name,
                                   path=self.path)

    def _get_current_state(self):
        """Get the current status of the system."""
        state = self.dbus_iface.main.state_manager.state
        connection = self.dbus_iface.main.state_manager.connection.state
        queues = self.dbus_iface.main.state_manager.queues.state.name
        state_dict = {
            'name': state.name,
            'description': state.description,
            'is_error': bool_str(state.is_error),
            'is_connected': bool_str(state.is_connected),
            'is_online': bool_str(state.is_online),
            'queues': queues,
            'connection': connection,
        }
        return state_dict

    @dbus.service.method(DBUS_IFACE_STATUS_NAME,
                         in_signature='', out_signature='a{ss}')
    def current_status(self):
        """ return the current status of the system, one of: local_rescan,
        offline, trying_to_connect, server_rescan or online.
        """
        logger.debug('called current_status')
        return self._get_current_state()

    @dbus.service.method(DBUS_IFACE_STATUS_NAME, out_signature='aa{ss}')
    def current_downloads(self):
        """ return list of files with a download in progress. """
        logger.debug('called current_downloads')
        current_downloads = []
        for download in self.action_queue.downloading:
            try:
                relpath = self.fs_manager.get_by_node_id(*download).path
            except KeyError:
                # the path has gone away! ignore this download
                continue
            path = self.fs_manager.get_abspath(download[0], relpath)
            info = self.action_queue.downloading[download]
            entry = {'path':path,
                     'share_id':download[0],
                     'node_id':download[1],
                     'n_bytes_read':str(info.get('n_bytes_read', 0))}
            try:
                entry['deflated_size'] = str(info['deflated_size'])
                # the idea is to do nothing, pylint: disable-msg=W0704
            except KeyError:
                # ignore the deflated_size key
                pass
            current_downloads.append(entry)
        return current_downloads

    def _get_command_path(self, cmd):
        """Return the path on which the command applies."""
        try:
            if IMarker.providedBy(cmd.node_id):
                # it's a marker! so it's the mdid :)
                relpath = self.fs_manager.get_by_mdid(cmd.node_id).path
            else:
                relpath = self.fs_manager.get_by_node_id(cmd.share_id,
                                                         cmd.node_id).path
            path = self.fs_manager.get_abspath(cmd.share_id, relpath)
        except KeyError:
            # probably in the trash (normal case for Unlink)
            path = ''
            key = cmd.share_id, cmd.node_id
            if key in self.fs_manager.trash:
                node_data = self.fs_manager.trash[key]
                if len(node_data) == 3:
                    # new trash!
                    path = node_data[2]

        return path

    @dbus.service.method(DBUS_IFACE_STATUS_NAME, out_signature='a(sa{ss})')
    def waiting_metadata(self):
        """Return a list of the operations in the meta-queue."""
        logger.debug('called waiting_metadata')
        waiting_metadata = []
        for cmd in self.action_queue.meta_queue.full_queue():
            if cmd is not None:
                operation = cmd.__class__.__name__
                data = cmd.to_dict()
                if 'path' not in data and hasattr(cmd, 'share_id') \
                   and hasattr(cmd, 'node_id'):
                    data['path'] = self._get_command_path(cmd)
                waiting_metadata.append((operation, data))
        return waiting_metadata

    @dbus.service.method(DBUS_IFACE_STATUS_NAME, out_signature='aa{ss}')
    def waiting_content(self):
        """Return a list of files that are waiting to be up- or downloaded."""
        logger.debug('called waiting_content')
        waiting_content = []
        for cmd in self.action_queue.content_queue.full_queue():
            if cmd is not None:
                if hasattr(cmd, 'share_id') and hasattr(cmd, 'node_id'):
                    path = self._get_command_path(cmd)
                    data = dict(path=path, share=cmd.share_id,
                                node=cmd.node_id,
                                operation=cmd.__class__.__name__)
                else:
                    data = dict(operation=cmd.__class__.__name__)
                waiting_content.append(data)
        return waiting_content

    @dbus.service.method(DBUS_IFACE_STATUS_NAME, in_signature='ss')
    def schedule_next(self, share_id, node_id):
        """
        Make the command on the given share and node be next in the
        queue of waiting commands.
        """
        logger.debug('called schedule_next')
        self.action_queue.content_queue.schedule_next(share_id, node_id)

    @dbus.service.method(DBUS_IFACE_STATUS_NAME, out_signature='aa{ss}')
    def current_uploads(self):
        """ return a list of files with a upload in progress """
        logger.debug('called current_uploads')
        current_uploads = []
        for upload in self.action_queue.uploading:
            share_id, node_id = upload
            if IMarker.providedBy(node_id):
                continue
            try:
                relpath = self.fs_manager.get_by_node_id(share_id, node_id).path
            except KeyError:
                # the path has gone away! ignore this upload
                continue
            path = self.fs_manager.get_abspath(share_id, relpath)
            info = self.action_queue.uploading[upload]
            entry = {'path':path,
                     'share_id':upload[0],
                     'node_id':upload[1],
                     'n_bytes_written':str(info.get('n_bytes_written', 0))}
            try:
                entry['deflated_size'] = str(info['deflated_size'])
                # the idea is to do nothing, pylint: disable-msg=W0704
            except KeyError:
                # ignore the deflated_size key
                pass
            current_uploads.append(entry)
        return current_uploads

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME)
    def DownloadStarted(self, path):
        """Fire a signal to notify that a download has started."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='sa{ss}')
    def DownloadFileProgress(self, path, info):
        """Fire a signal to notify about a download progress."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='sa{ss}')
    def DownloadFinished(self, path, info):
        """Fire a signal to notify that a download has finished."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME)
    def UploadStarted(self, path):
        """Fire a signal to notify that an upload has started."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='sa{ss}')
    def UploadFileProgress(self, path, info):
        """Fire a signal to notify about an upload progress."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='sa{ss}')
    def UploadFinished(self, path, info):
        """Fire a signal to notify that an upload has finished."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='say')
    def InvalidName(self, dirname, filename):
        """Fire a signal to notify an invalid file or dir name."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='ssss')
    def BrokenNode(self, volume_id, node_id, mdid, path):
        """Fire a signal to notify a broken node."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME)
    def StatusChanged(self, status):
        """Fire a signal to notify that the status of the system changed."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='a{ss}')
    def AccountChanged(self, account_info):
        """Fire a signal to notify that account information has changed."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME, signature='a{sa{ss}}')
    def ContentQueueChanged(self, message):
        """Fire a signal to notify that the content queue has changed."""

    @dbus.service.signal(DBUS_IFACE_STATUS_NAME)
    def MetaQueueChanged(self):
        """Fire a signal to notify that the meta queue has changed."""

    def emit_content_queue_changed(self, head, waiting):
        """Emit ContentQueueChanged."""
        if head is None or getattr(head, 'node_id', None) is None:
            head_path = ''
            head_cmd = ''
        else:
            waiting = chain((head,), waiting)
            head_cmd = get_classname(head)
            try:
                if IMarker.providedBy(head.node_id):
                    # it's a marker! so it's the mdid :)
                    relpath = self.fs_manager.get_by_mdid(head.node_id).path
                else:
                    relpath = self.fs_manager.get_by_node_id(head.share_id,
                                                             head.node_id).path
            except KeyError:
                head_path = ''
            else:
                head_path = self.fs_manager.get_abspath(head.share_id, relpath)
        message = dict(
            (k, {'size': '0', 'count': str(len(tuple(v)))})
            for (k, v) in groupby(sorted(map(get_classname, waiting))))
        message['head'] = {'path': head_path,
                           'command': head_cmd,
                           'size': '0'}
        self.ContentQueueChanged(message)

    def emit_invalid_name(self, dirname, filename):
        """Emit InvalidName."""
        self.InvalidName(unicode(dirname), str(filename))

    def emit_broken_node(self, volume_id, node_id, mdid, path):
        """Emit BrokenNode."""
        if mdid is None:
            mdid = ''
        if path is None:
            path = ''
        self.BrokenNode(volume_id, node_id, mdid, path.decode('utf8'))

    def emit_status_changed(self, state):
        """Emit StatusChanged."""
        self.StatusChanged(self._get_current_state())

    def emit_download_started(self, download):
        """Emit DownloadStarted."""
        self.DownloadStarted(download)

    def emit_download_file_progress(self, download, **info):
        """Emit DownloadFileProgress."""
        for k, v in info.copy().items():
            info[str(k)] = str(v)
        self.DownloadFileProgress(download, info)

    def emit_download_finished(self, download, **info):
        """Emit DownloadFinished."""
        for k, v in info.copy().items():
            info[str(k)] = str(v)
        self.DownloadFinished(download, info)

    def emit_upload_started(self, upload):
        """Emit UploadStarted."""
        self.UploadStarted(upload)

    def emit_upload_file_progress(self, upload, **info):
        """Emit UploadFileProgress."""
        for k, v in info.copy().items():
            info[str(k)] = str(v)
        self.UploadFileProgress(upload, info)

    def emit_upload_finished(self, upload, **info):
        """Emit UploadFinished."""
        for k, v in info.copy().items():
            info[str(k)] = str(v)
        self.UploadFinished(upload, info)

    def emit_account_changed(self, account_info):
        """Emit AccountChanged."""
        info_dict = {'purchased_bytes': unicode(account_info.purchased_bytes)}
        self.AccountChanged(info_dict)

    def emit_metaqueue_changed(self):
        """Emit MetaQueueChanged."""
        self.MetaQueueChanged()


class Events(DBusExposedObject):
    """ The events of the system translated to D-BUS signals """

    def __init__(self, bus_name, event_queue):
        """ Creates the instance.

        @param bus: the BusName of this DBusExposedObject.
        """
        self.event_queue = event_queue
        self.path = '/events'
        DBusExposedObject.__init__(self, bus_name=bus_name,
                                   path=self.path)

    @dbus.service.signal(DBUS_IFACE_EVENTS_NAME,
                         signature='a{ss}')
    def Event(self, event_dict):
        """ Fire a D-BUS signal, notifying an event. """
        pass

    def emit_event(self, event):
        """ Emits the signal """
        event_dict = {}
        for key, value in event.iteritems():
            event_dict[str(key)] = str(value)
        self.Event(event_dict)

    @dbus.service.method(DBUS_IFACE_EVENTS_NAME, in_signature='sas')
    def push_event(self, event_name, args):
        """ Push a event to the event queue """
        logger.debug('push_event: %r with %r', event_name, args)
        str_args = []
        for arg in args:
            str_args.append(str(arg))
        self.event_queue.push(str(event_name), *str_args)


class EventListener(object):
    """ An Event Queue Listener """

    def __init__(self, dbus_iface):
        """ created the instance """
        self.dbus_iface = dbus_iface

    def _fire_event(self, event_dict):
        """ Calls emit_event in a reactor thread. """
        self.dbus_iface.events.emit_event(event_dict)

    def handle_default(self, event_name, *args, **kwargs):
        """ handle all events """
        if not self.dbus_iface.send_events:
            return
        event_dict = {'event_name' : event_name, }
        event_args = list(EVENTS[event_name])
        event_dict.update(kwargs)
        for key in set(event_args).intersection(kwargs.keys()):
            event_args.pop(event_args.index(key))
        for i in xrange(0, len(event_args)):
            event_dict[event_args[i]] = args[i]
        event_dict.update(kwargs)
        self._fire_event(event_dict)

    def handle_AQ_DOWNLOAD_STARTED(self, share_id, node_id, server_hash):
        """ handle AQ_DOWNLOAD_STARTED """
        self.handle_default('AQ_DOWNLOAD_STARTED', share_id, node_id,
                            server_hash)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id, node_id)
            if mdobj.is_dir:
                return
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_download_started(path)
        except KeyError, e:
            args = dict(message='The md is gone before sending '
                        'DownloadStarted signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id))
            self.dbus_iface.status.emit_signal_error('DownloadStarted', args)

    def handle_AQ_DOWNLOAD_FILE_PROGRESS(self, share_id, node_id,
                                         n_bytes_read, deflated_size):
        """Handle AQ_DOWNLOAD_FILE_PROGRESS."""
        self.handle_default('AQ_DOWNLOAD_FILE_PROGRESS', share_id, node_id,
                            n_bytes_read, deflated_size)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id, node_id)
        except KeyError, e:
            args = dict(message='The md is gone before sending '
                        'DownloadFileProgress signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id))
            self.dbus_iface.status.emit_signal_error('DownloadFileProgress',
                                                     args)
        else:
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_download_file_progress(path,
                                                 n_bytes_read=n_bytes_read,
                                                 deflated_size=deflated_size
                                                               )

    def handle_AQ_DOWNLOAD_FINISHED(self, share_id, node_id, server_hash):
        """ handle AQ_DOWNLOAD_FINISHED """
        self.handle_default('AQ_DOWNLOAD_FINISHED', share_id,
                            node_id, server_hash)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id, node_id)
            if mdobj.is_dir:
                return
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_download_finished(path)
        except KeyError, e:
            # file is gone before we got this
            args = dict(message='The md is gone before sending '
                        'DownloadFinished signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id))
            self.dbus_iface.status.emit_signal_error('DownloadFinished', args)

    def handle_AQ_DOWNLOAD_CANCELLED(self, share_id, node_id, server_hash):
        """ handle AQ_DOWNLOAD_CANCELLED """
        self.handle_AQ_DOWNLOAD_ERROR(share_id, node_id, server_hash,
                                      'CANCELLED', 'AQ_DOWNLOAD_CANCELLED')

    def handle_AQ_DOWNLOAD_ERROR(self, share_id, node_id, server_hash, error,
                                 event='AQ_DOWNLOAD_ERROR'):
        """ handle AQ_DOWNLOAD_ERROR """
        self.handle_default(event, share_id, node_id,
                            server_hash, error)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id, node_id)
            if mdobj.is_dir:
                return
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_download_finished(path, error=error)
        except KeyError, e:
            # file is gone before we got this
            args = dict(message='The md is gone before sending '
                        'DownloadFinished signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id),
                        download_error=str(error))
            self.dbus_iface.status.emit_signal_error('DownloadFinished', args)

    def handle_AQ_UPLOAD_STARTED(self, share_id, node_id, hash):
        """ handle AQ_UPLOAD_STARTED """
        self.handle_default('AQ_UPLOAD_STARTED', share_id, node_id, hash)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id, node_id)
            if mdobj.is_dir:
                return
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_upload_started(path)
        except KeyError, e:
            args = dict(message='The md is gone before sending '
                        'UploadStarted signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id))
            self.dbus_iface.status.emit_signal_error('UploadStarted', args)

    def handle_AQ_UPLOAD_FILE_PROGRESS(self, share_id, node_id,
                                         n_bytes_written, deflated_size):
        """Handle AQ_UPLOAD_FILE_PROGRESS."""
        self.handle_default('AQ_UPLOAD_FILE_PROGRESS', share_id, node_id,
                            n_bytes_written, deflated_size)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id, node_id)
        except KeyError, e:
            args = dict(message='The md is gone before sending '
                        'UploadFileProgress signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id))
            self.dbus_iface.status.emit_signal_error('UploadFileProgress',
                                                     args)
        else:
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_upload_file_progress(path,
                                                n_bytes_written=n_bytes_written,
                                                deflated_size=deflated_size
                                                             )

    def handle_AQ_UPLOAD_FINISHED(self, share_id, node_id, hash,
                                  new_generation):
        """Handle AQ_UPLOAD_FINISHED."""
        self.handle_default('AQ_UPLOAD_FINISHED', share_id, node_id, hash,
                            new_generation)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id,
                                                              node_id)
            if mdobj.is_dir:
                return
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_upload_finished(path)
        except KeyError, e:
            # file is gone before we got this
            args = dict(message='The metadata is gone before sending '
                        'UploadFinished signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id))
            self.dbus_iface.status.emit_signal_error('UploadFinished', args)

    def handle_SV_ACCOUNT_CHANGED(self, account_info):
        """ handle SV_ACCOUNT_CHANGED """
        self.handle_default('SV_ACCOUNT_CHANGED', account_info)
        self.dbus_iface.status.emit_account_changed(account_info)

    def handle_AQ_UPLOAD_ERROR(self, share_id, node_id, error, hash):
        """ handle AQ_UPLOAD_ERROR """
        self.handle_default('AQ_UPLOAD_ERROR', share_id, node_id, error, hash)
        try:
            mdobj = self.dbus_iface.fs_manager.get_by_node_id(share_id, node_id)
            if mdobj.is_dir:
                return
            path = self.dbus_iface.fs_manager.get_abspath(share_id, mdobj.path)
            self.dbus_iface.status.emit_upload_finished(path, error=error)
        except KeyError, e:
            # file is gone before we got this
            args = dict(message='The metadata is gone before sending '
                        'UploadFinished signal',
                        error=str(e),
                        share_id=str(share_id),
                        node_id=str(node_id),
                        upload_error=str(error))
            self.dbus_iface.status.emit_signal_error('UploadFinished', args)

    def handle_FS_INVALID_NAME(self, dirname, filename):
        """Handle FS_INVALID_NAME."""
        self.handle_default('FS_INVALID_NAME', dirname, filename)
        self.dbus_iface.status.emit_invalid_name(dirname, filename)

    def handle_SYS_BROKEN_NODE(self, volume_id, node_id, mdid, path):
        """Handle SYS_BROKEN_NODE."""
        self.handle_default('SYS_BROKEN_NODE', volume_id, node_id, mdid, path)
        self.dbus_iface.status.emit_broken_node(volume_id, node_id, mdid, path)

    def handle_SYS_STATE_CHANGED(self, state):
        """ handle SYS_STATE_CHANGED """
        self.handle_default('SYS_STATE_CHANGED', state)
        self.dbus_iface.status.emit_status_changed(state)

    def handle_SV_FREE_SPACE(self, share_id, free_bytes):
        """ handle SV_FREE_SPACE event, emit ShareChanged signal. """
        self.handle_default('SV_FREE_SPACE', share_id, free_bytes)
        self.dbus_iface.shares.emit_free_space(share_id, free_bytes)

    def handle_AQ_CREATE_SHARE_OK(self, share_id, marker):
        """ handle AQ_CREATE_SHARE_OK event, emit's ShareCreated signal. """
        self.handle_default('AQ_CREATE_SHARE_OK', share_id, marker)
        share = self.dbus_iface.volume_manager.shared.get(str(share_id))
        share_dict = {}
        if share:
            # pylint: disable-msg=W0212
            share_dict.update(_get_share_dict(share))
        else:
            share_dict.update(dict(volume_id=str(share_id)))
        self.dbus_iface.shares.emit_share_created(share_dict)

    def handle_AQ_CREATE_SHARE_ERROR(self, marker, error):
        """ handle AQ_CREATE_SHARE_ERROR event, emit's ShareCreateError signal.
        """
        self.handle_default('AQ_CREATE_SHARE_ERROR', marker, error)
        self.dbus_iface.shares.emit_share_create_error(dict(marker=marker),
                                                       error)

    def handle_AQ_ANSWER_SHARE_OK(self, share_id, answer):
        """ handle AQ_ANSWER_SHARE_OK event, emit's ShareAnswerOk signal. """
        self.handle_default('AQ_ANSWER_SHARE_OK', str(share_id), answer)
        self.dbus_iface.shares.emit_share_answer_response(str(share_id), answer)

    def handle_AQ_ANSWER_SHARE_ERROR(self, share_id, answer, error):
        """ handle AQ_ANSWER_SHARE_ERROR event, emit's ShareAnswerError signal.
        """
        self.handle_default('AQ_ANSWER_SHARE_ERROR', str(share_id), answer, error)
        self.dbus_iface.shares.emit_share_answer_response(str(share_id), answer,
                                                          error)
    def handle_VM_UDF_SUBSCRIBED(self, udf):
        """Handle VM_UDF_SUBSCRIBED event, emit FolderSubscribed signal."""
        self.handle_default('VM_UDF_SUBSCRIBED', udf)
        self.dbus_iface.folders.emit_folder_subscribed(udf)

    def handle_VM_UDF_SUBSCRIBE_ERROR(self, udf_id, error):
        """
        Handle VM_UDF_SUBSCRIBE_ERROR event, emit
        FolderSubscribeError signal.

        """
        self.handle_default('VM_UDF_SUBSCRIBE_ERROR', udf_id, error)
        self.dbus_iface.folders.emit_folder_subscribe_error(udf_id, error)

    def handle_VM_UDF_UNSUBSCRIBED(self, udf):
        """Handle VM_UDF_UNSUBSCRIBED event, emit FolderUnSubscribed signal."""
        self.handle_default('VM_UDF_UNSUBSCRIBED', udf)
        self.dbus_iface.folders.emit_folder_unsubscribed(udf)

    def handle_VM_UDF_UNSUBSCRIBE_ERROR(self, udf_id, error):
        """Handle VM_UDF_UNSUBSCRIBE_ERROR event.

        Emit FolderUnSubscribeError signal.

        """
        self.handle_default('VM_UDF_UNSUBSCRIBE_ERROR', udf_id, error)
        self.dbus_iface.folders.emit_folder_unsubscribe_error(udf_id, error)

    def handle_VM_UDF_CREATED(self, udf):
        """Handle VM_UDF_CREATED event, emit FolderCreated signal."""
        self.handle_default('VM_UDF_CREATED', udf)
        self.dbus_iface.folders.emit_folder_created(udf)

    def handle_VM_UDF_CREATE_ERROR(self, path, error):
        """Handle VM_UDF_CREATE_ERROR event, emit FolderCreateError signal."""
        self.handle_default('VM_UDF_CREATE_ERROR', path, error)
        self.dbus_iface.folders.emit_folder_create_error(path, error)

    def handle_VM_SHARE_CREATED(self, share_id):
        """Handle VM_SHARE_CREATED event, emit NewShare event."""
        self.handle_default('VM_SHARE_CREATED', share_id)
        self.dbus_iface.shares.emit_new_share(share_id)

    def handle_VM_SHARE_DELETED(self, share):
        """Handle VM_SHARE_DELETED event, emit NewShare event."""
        self.handle_default('VM_SHARE_DELETED', getattr(share, 'id', share))
        self.dbus_iface.shares.emit_share_changed('deleted', share)

    def handle_VM_SHARE_DELETE_ERROR(self, share_id, error):
        """Handle VM_DELETE_SHARE_ERROR event, emit ShareCreateError signal."""
        self.handle_default('VM_SHARE_DELETE_ERROR', share_id, error)
        self.dbus_iface.shares.ShareDeleteError(dict(volume_id=share_id), error)

    def handle_VM_VOLUME_DELETED(self, volume):
        """Handle VM_VOLUME_DELETED event.

        Emits FolderDeleted or ShareChanged signal.

        """
        self.handle_default('VM_VOLUME_DELETED', getattr(volume, 'id', volume))
        if isinstance(volume, Share):
            self.dbus_iface.shares.emit_share_changed('deleted', volume)
        elif isinstance(volume, UDF):
            self.dbus_iface.folders.emit_folder_deleted(volume)
        else:
            logger.error("Unable to handle VM_VOLUME_DELETE for "
                     "volume_id=%r as it's not a share or UDF", volume.id)

    def handle_VM_VOLUME_DELETE_ERROR(self, volume_id, error):
        """Handle VM_VOLUME_DELETE_ERROR event, emit ShareDeleted event."""
        self.handle_default('VM_VOLUME_DELETE_ERROR', volume_id, error)
        try:
            volume = self.dbus_iface.volume_manager.get_volume(volume_id)
        except VolumeDoesNotExist:
            logger.error("Unable to handle VM_VOLUME_DELETE_ERROR for "
                         "volume_id=%r, no such volume.", volume_id)
        else:
            if isinstance(volume, Share):
                self.dbus_iface.shares.emit_share_delete_error(volume, error)
            elif isinstance(volume, UDF):
                self.dbus_iface.folders.emit_folder_delete_error(volume, error)
            else:
                logger.error("Unable to handle VM_VOLUME_DELETE_ERROR for "
                         "volume_id=%r as it's not a share or UDF", volume_id)

    def handle_VM_SHARE_CHANGED(self, share_id):
        """ handle VM_SHARE_CHANGED event, emit's ShareChanged signal. """
        self.handle_default('VM_SHARE_CHANGED', share_id)
        share = self.dbus_iface.volume_manager.shares.get(share_id)
        self.dbus_iface.shares.emit_share_changed('changed', share)

    def handle_AQ_CHANGE_PUBLIC_ACCESS_OK(self, share_id, node_id,
                                          is_public, public_url):
        """Handle the AQ_CHANGE_PUBLIC_ACCESS_OK event."""
        self.handle_default('AQ_CHANGE_PUBLIC_ACCESS_OK', share_id, node_id,
                            is_public, public_url)
        self.dbus_iface.public_files.emit_public_access_changed(
            share_id, node_id, is_public, public_url)

    def handle_AQ_CHANGE_PUBLIC_ACCESS_ERROR(self, share_id, node_id, error):
        """Handle the AQ_CHANGE_PUBLIC_ACCESS_ERROR event."""
        self.handle_default('AQ_CHANGE_PUBLIC_ACCESS_ERROR',
                            share_id, node_id, error)
        self.dbus_iface.public_files.emit_public_access_change_error(
            share_id, node_id, error)

    def handle_SYS_ROOT_MISMATCH(self, root_id, new_root_id):
        """Handle the SYS_ROOT_MISMATCH event."""
        self.handle_default('SYS_ROOT_MISMATCH', root_id, new_root_id)
        self.dbus_iface.sync.emit_root_mismatch(root_id, new_root_id)

    def handle_AQ_PUBLIC_FILES_LIST_OK(self, public_files):
        """Handle the AQ_PUBLIC_FILES_LIST_OK event."""
        self.handle_default('AQ_PUBLIC_FILES_LIST_OK', public_files)
        self.dbus_iface.public_files.emit_public_files_list(public_files)

    def handle_AQ_PUBLIC_FILES_LIST_ERROR(self, error):
        """Handle the AQ_PUBLIC_FILES_LIST_ERROR event."""
        self.handle_default('AQ_PUBLIC_FILES_LIST_ERROR', error)
        self.dbus_iface.public_files.emit_public_files_list_error(error)

    def handle_SYS_QUOTA_EXCEEDED(self, volume_id, free_bytes):
        """Handle the SYS_QUOTA_EXCEEDED event."""
        self.handle_default('SYS_QUOTA_EXCEEDED', volume_id, free_bytes)

        volume = self.dbus_iface.volume_manager.get_volume(str(volume_id))

        volume_dict = {}
        if isinstance(volume, UDF):
            volume_dict = _get_udf_dict(volume)
        else:
            # either a Share or Root
            volume_dict = _get_share_dict(volume)

        # be sure that the volume has the most updated free bytes info
        volume_dict['free_bytes'] = str(free_bytes)

        self.dbus_iface.sync.emit_quota_exceeded(volume_dict)

    def handle_SYS_META_QUEUE_WAITING(self):
        """Handle SYS_META_QUEUE_WAITING."""
        self.handle_default('SYS_META_QUEUE_WAITING')
        self.dbus_iface.status.emit_metaqueue_changed()

    def handle_SYS_META_QUEUE_DONE(self):
        """Handle SYS_META_QUEUE_DONE."""
        self.handle_default('SYS_META_QUEUE_DONE')
        self.dbus_iface.status.emit_metaqueue_changed()


class SyncDaemon(DBusExposedObject):
    """ The Daemon dbus interface. """

    def __init__(self, bus_name, dbus_iface):
        """ Creates the instance.

        @param bus: the BusName of this DBusExposedObject.
        """
        self.dbus_iface = dbus_iface
        self.path = '/'
        DBusExposedObject.__init__(self, bus_name=bus_name,
                                   path=self.path)

    @dbus.service.method(DBUS_IFACE_SYNC_NAME,
                         in_signature='', out_signature='')
    def connect(self):
        """ Connect to the server. """
        logger.debug('connect requested')
        self.dbus_iface.connect()

    @dbus.service.method(DBUS_IFACE_SYNC_NAME,
                         in_signature='', out_signature='')
    def disconnect(self):
        """ Disconnect from the server. """
        logger.debug('disconnect requested')
        self.dbus_iface.disconnect()

    @dbus.service.method(DBUS_IFACE_SYNC_NAME,
                         in_signature='', out_signature='s')
    def get_rootdir(self):
        """ Returns the root dir/mount point. """
        logger.debug('called get_rootdir')
        return self.dbus_iface.main.get_rootdir()

    @dbus.service.method(DBUS_IFACE_SYNC_NAME,
                         in_signature='d', out_signature='b',
                         async_callbacks=('reply_handler', 'error_handler'))
    def wait_for_nirvana(self, last_event_interval,
                         reply_handler=None, error_handler=None):
        """ call the reply handler when there are no more
        events or transfers.
        """
        logger.debug('called wait_for_nirvana')
        d = self.dbus_iface.main.wait_for_nirvana(last_event_interval)
        d.addCallbacks(reply_handler, error_handler)
        return d

    @dbus.service.method(DBUS_IFACE_SYNC_NAME,
                         in_signature='', out_signature='',
                         async_callbacks=('reply_handler', 'error_handler'))
    def quit(self, reply_handler=None, error_handler=None):
        """ shutdown the syncdaemon. """
        logger.debug('Quit requested')
        if reply_handler:
            reply_handler()
        self.dbus_iface.quit()

    @dbus.service.method(DBUS_IFACE_SYNC_NAME,
                         in_signature='s', out_signature='')
    def rescan_from_scratch(self, volume_id):
        """Request a rescan from scratch of the volume with volume_id."""
        # check that the volume exists
        volume = self.dbus_iface.volume_manager.get_volume(volume_id)
        self.dbus_iface.action_queue.rescan_from_scratch(volume.volume_id)

    @dbus.service.signal(DBUS_IFACE_SYNC_NAME,
                         signature='ss')
    def RootMismatch(self, root_id, new_root_id):
        """RootMismatch signal, the user connected with a different account."""
        pass

    def emit_root_mismatch(self, root_id, new_root_id):
        """Emit RootMismatch signal."""
        self.RootMismatch(root_id, new_root_id)

    @dbus.service.signal(DBUS_IFACE_SYNC_NAME,
                         signature='a{ss}')
    def QuotaExceeded(self, volume_dict):
        """QuotaExceeded signal, the user ran out of space."""
        pass

    def emit_quota_exceeded(self, volume_dict):
        """Emit QuotaExceeded signal."""
        self.QuotaExceeded(volume_dict)


class FileSystem(DBusExposedObject):
    """ A dbus interface to the FileSystem Manager. """

    def __init__(self, bus_name, fs_manager, action_queue):
        """ Creates the instance. """
        self.fs_manager = fs_manager
        self.action_queue = action_queue
        self.path = '/filesystem'
        DBusExposedObject.__init__(self, bus_name=bus_name,
                                   path=self.path)

    @dbus.service.method(DBUS_IFACE_FS_NAME,
                         in_signature='s', out_signature='a{ss}')
    def get_metadata(self, path):
        """Return the metadata (as a dict) for the specified path."""
        logger.debug('get_metadata by path: %r', path)
        real_path = os.path.realpath(path.encode('utf-8'))
        mdobj = self.fs_manager.get_by_path(real_path)
        md_dict = self._mdobj_dict(mdobj)
        md_dict['path'] = path
        return md_dict

    @dbus.service.method(DBUS_IFACE_FS_NAME,
                         in_signature='ss', out_signature='a{ss}')
    def get_metadata_by_node(self, share_id, node_id):
        """Return the metadata (as a dict) for the specified share/node."""
        logger.debug('get_metadata by share: %r  node: %r', share_id, node_id)
        mdobj = self.fs_manager.get_by_node_id(share_id, node_id)
        md_dict = self._mdobj_dict(mdobj)
        path = self.fs_manager.get_abspath(mdobj.share_id, mdobj.path)
        md_dict['path'] = path
        return md_dict

    @dbus.service.method(DBUS_IFACE_FS_NAME,
                         in_signature='s', out_signature='a{ss}')
    def get_metadata_and_quick_tree_synced(self, path):
        """ returns the dict with the attributes of the metadata for
        the specified path, including the quick subtree status.
        """
        logger.debug('get_metadata_and_quick_tree_synced: %r', path)
        real_path = os.path.realpath(path.encode('utf-8'))
        mdobj = self.fs_manager.get_by_path(real_path)
        md_dict = self._mdobj_dict(mdobj)
        md_dict['path'] = path
        if (self._path_in_queue(path, self.action_queue.meta_queue)
            or self._path_in_queue(path, self.action_queue.content_queue)):
            md_dict['quick_tree_synced'] = ''
        else:
            md_dict['quick_tree_synced'] = 'synced'
        return md_dict

    def _path_in_queue(self, path, queue):
        """A helper that returns whether there are commands in the queue
        pertaining to the path"""
        for cmd in queue.full_queue():
            share_id = getattr(cmd, 'share_id', None)
            node_id = getattr(cmd, 'node_id', None)
            if share_id is not None and node_id is not None:
                # XXX: nested try/excepts in a loop are probably a
                # sign that I'm doing something wrong - or that
                # somebody is :)
                this_path = ''
                try:
                    node_md = self.fs_manager.get_by_node_id(share_id, node_id)
                except KeyError:
                    # maybe it's actually the mdid?
                    try:
                        node_md = self.fs_manager.get_by_mdid(node_id)
                    except KeyError:
                        # hm, nope. Dunno what to do then
                        pass
                    else:
                        this_path = self.fs_manager.get_abspath(share_id,
                                                                node_md.path)
                else:
                    this_path = self.fs_manager.get_abspath(share_id,
                                                            node_md.path)
                if this_path.startswith(path):
                    return True
        return False

    def _mdobj_dict(self, mdobj):
        """Returns a dict from a MDObject."""
        md_dict = {}
        for k, v in mdobj.__dict__.items():
            if k == 'info':
                continue
            elif k == 'path':
                md_dict[str(k)] = v.decode('utf-8')
            else:
                md_dict[str(k)] = str(v)
        if mdobj.__dict__.get('info', None):
            for k, v in mdobj.info.__dict__.items():
                md_dict['info_'+str(k)] = str(v)
        return md_dict

    @dbus.service.method(DBUS_IFACE_FS_NAME,
                         in_signature='', out_signature='aa{ss}')
    def get_dirty_nodes(self):
        """Rerturn a list of dirty nodes."""
        mdobjs = self.fs_manager.get_dirty_nodes()
        dirty_nodes = []
        for mdobj in mdobjs:
            dirty_nodes.append(self._mdobj_dict(mdobj))
        return dirty_nodes


class Shares(DBusExposedObject):
    """ A dbus interface to interact wiht shares """

    def __init__(self, bus_name, fs_manager, volume_manager):
        """ Creates the instance. """
        self.fs_manager = fs_manager
        self.vm = volume_manager
        self.path = '/shares'
        DBusExposedObject.__init__(self, bus_name=bus_name,
                                   path=self.path)

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='', out_signature='aa{ss}')
    def get_shares(self):
        """ returns a list of dicts, each dict represents a share """
        logger.debug('called get_shares')
        shares = []
        for share_id, share in self.vm.shares.items():
            if share_id == '':
                continue
            share_dict = _get_share_dict(share)
            shares.append(share_dict)
        return shares

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='s', out_signature='',
                         async_callbacks=('reply_handler', 'error_handler'))
    def accept_share(self, share_id, reply_handler=None, error_handler=None):
        """ Accepts a share, a ShareAnswerOk|Error signal will be fired in the
        future as a success/failure indicator.
        """
        logger.debug('accept_share: %r', share_id)
        if str(share_id) in self.vm.shares:
            self.vm.accept_share(str(share_id), True)
            reply_handler()
        else:
            error_handler(ValueError("The share with id: %s don't exists" % \
                                     str(share_id)))

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='s', out_signature='',
                         async_callbacks=('reply_handler', 'error_handler'))
    def reject_share(self, share_id, reply_handler=None, error_handler=None):
        """ Rejects a share. """
        logger.debug('reject_share: %r', share_id)
        if str(share_id) in self.vm.shares:
            self.vm.accept_share(str(share_id), False)
            reply_handler()
        else:
            error_handler(ValueError("The share with id: %s don't exists" % \
                                     str(share_id)))

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='s', out_signature='')
    def delete_share(self, share_id):
        """Delete a Share, both kinds: "to me" and "from me"."""
        logger.debug('delete_share: %r', share_id)
        try:
            try:
                self.vm.delete_volume(str(share_id))
            except VolumeDoesNotExist:
                # isn't a volume! it might be a "share from me (a.k.a shared)"
                self.vm.delete_share(str(share_id))
        except Exception, e:
            logger.exception('Error while deleting share: %r', share_id)
            self.ShareDeleteError({'volume_id':share_id}, str(e))
            # propagate the error
            raise

    @dbus.service.signal(DBUS_IFACE_SHARES_NAME,
                         signature='a{ss}')
    def ShareChanged(self, share_dict):
        """ A share changed, share_dict contains all the share attributes. """
        pass

    @dbus.service.signal(DBUS_IFACE_SHARES_NAME,
                         signature='a{ss}')
    def ShareDeleted(self, share_dict):
        """ A share was deleted, share_dict contains all available
        share attributes. """
        pass

    @dbus.service.signal(DBUS_IFACE_SHARES_NAME,
                         signature='a{ss}s')
    def ShareDeleteError(self, share_dict, error):
        """ A share was deleted, share_dict contains all available
        share attributes. """
        pass

    def emit_share_changed(self, message, share):
        """ emits ShareChanged or ShareDeleted signal for the share
        notification.
        """
        if message == 'deleted':
            self.ShareDeleted(_get_share_dict(share))
        elif message == 'changed':
            self.ShareChanged(_get_share_dict(share))

    def emit_share_delete_error(self, share, error):
        """Emits ShareDeleteError signal."""
        self.ShareDeleteError(_get_share_dict(share), error)

    def emit_free_space(self, share_id, free_bytes):
        """ emits ShareChanged when free space changes """
        if share_id in self.vm.shares:
            share = self.vm.shares[share_id]
            share_dict = _get_share_dict(share)
            share_dict['free_bytes'] = unicode(free_bytes)
            self.ShareChanged(share_dict)

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='ssss', out_signature='')
    def create_share(self, path, username, name, access_level):
        """ Share a subtree to the user identified by username.

        @param path: that path to share (the root of the subtree)
        @param username: the username to offer the share to
        @param name: the name of the share
        @param access_level: 'View' or 'Modify'
        """
        logger.debug('create share: %r, %r, %r, %r',
                     path, username, name, access_level)
        path = path.encode("utf8")
        username = unicode(username)
        name = unicode(name)
        access_level = str(access_level)
        try:
            self.fs_manager.get_by_path(path)
        except KeyError:
            raise ValueError("path '%r' does not exist" % path)
        self.vm.create_share(path, username, name, access_level)

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='sasss', out_signature='')
    def create_shares(self, path, usernames, name, access_level):
        """Share a subtree with several users at once.

        @param path: that path to share (the root of the subtree)
        @param usernames: the user names to offer the share to
        @param name: the name of the share
        @param access_level: 'View' or 'Modify'
        """
        logger.debug('create shares: %r, %r, %r, %r',
                     path, usernames, name, access_level)
        for user in usernames:
            self.create_share(path, user, name, access_level)

    @dbus.service.signal(DBUS_IFACE_SHARES_NAME,
                         signature='a{ss}')
    def ShareCreated(self, share_info):
        """ The requested share was succesfully created. """
        pass

    @dbus.service.signal(DBUS_IFACE_SHARES_NAME,
                         signature='a{ss}s')
    def ShareCreateError(self, share_info, error):
        """ An error ocurred while creating the share. """
        pass

    def emit_share_created(self, share_info):
        """ emits ShareCreated signal """
        self.ShareCreated(share_info)

    def emit_share_create_error(self, share_info, error):
        """Emit ShareCreateError signal."""
        path = self.fs_manager.get_by_mdid(str(share_info['marker'])).path
        share_info.update(dict(path=path))
        self.ShareCreateError(share_info, error)

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='', out_signature='')
    def refresh_shares(self):
        """ Refresh the share list, requesting it to the server. """
        self.vm.refresh_shares()

    @dbus.service.method(DBUS_IFACE_SHARES_NAME,
                         in_signature='', out_signature='aa{ss}')
    def get_shared(self):
        """ returns a list of dicts, each dict represents a shared share.
        A share might not have the path set, as we might be still fetching the
        nodes from the server. In this cases the path is ''
        """
        logger.debug('called get_shared')
        shares = []
        for share_id, share in self.vm.shared.items():
            if share_id == '':
                continue
            share_dict = _get_share_dict(share)
            shares.append(share_dict)
        return shares

    @dbus.service.signal(DBUS_IFACE_SHARES_NAME,
                         signature='a{ss}')
    def ShareAnswerResponse(self, answer_info):
        """The answer to share was succesfull"""
        pass

    def emit_share_answer_response(self, share_id, answer, error=None):
        """Emits ShareAnswerResponse signal."""
        answer_info = dict(volume_id=share_id, answer=answer)
        if error:
            answer_info['error'] = error
        self.ShareAnswerResponse(answer_info)

    @dbus.service.signal(DBUS_IFACE_SHARES_NAME,
                         signature='a{ss}')
    def NewShare(self, share_info):
        """A new share notification."""
        pass

    def emit_new_share(self, share_id):
        """Emits NewShare signal."""
        share = self.vm.get_volume(share_id)
        self.NewShare(_get_share_dict(share))


class Config(DBusExposedObject):
    """ The Syncdaemon config/settings dbus interface. """

    def __init__(self, bus_name, dbus_iface):
        """ Creates the instance.

        @param bus: the BusName of this DBusExposedObject.
        """
        self.dbus_iface = dbus_iface
        self.path = '/config'
        DBusExposedObject.__init__(self, bus_name=bus_name,
                                   path=self.path)

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='a{si}',
                         async_callbacks=('reply_handler', 'error_handler'))
    def get_throttling_limits(self, reply_handler=None, error_handler=None):
        """Get the read/write limit from AQ and return a dict.
        Returns a dict(download=int, upload=int), if int is -1 the value isn't
        configured.
        The values are bytes/second
        """
        logger.debug("called get_throttling_limits")
        try:
            aq = self.dbus_iface.action_queue
            download = -1
            upload = -1
            if aq.readLimit is not None:
                download = aq.readLimit
            if aq.writeLimit is not None:
                upload = aq.writeLimit
            info = dict(download=download,
                        upload=upload)
            if reply_handler:
                reply_handler(info)
            else:
                return info
            # pylint: disable-msg=W0703
        except Exception, e:
            if error_handler:
                error_handler(e)
            else:
                raise

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='ii', out_signature='',
                         async_callbacks=('reply_handler', 'error_handler'))
    def set_throttling_limits(self, download, upload,
                         reply_handler=None, error_handler=None):
        """Set the read and write limits. The expected values are bytes/sec."""
        logger.debug("called set_throttling_limits")
        try:
            # modify and save the config file
            user_config = config.get_user_config()
            user_config.set_throttling_read_limit(download)
            user_config.set_throttling_write_limit(upload)
            user_config.save()
            # modify AQ settings
            aq = self.dbus_iface.action_queue
            if download == -1:
                download = None
            if upload == -1:
                upload = None
            aq.readLimit = download
            aq.writeLimit = upload
            if reply_handler:
                reply_handler()
            # pylint: disable-msg=W0703
        except Exception, e:
            if error_handler:
                error_handler(e)
            else:
                raise

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='',
                         async_callbacks=('reply_handler', 'error_handler'))
    def enable_bandwidth_throttling(self, reply_handler=None,
                                    error_handler=None):
        """Enable bandwidth throttling."""
        try:
            self._set_throttling_enabled(True)
            if reply_handler:
                reply_handler()
            # pylint: disable-msg=W0703
        except Exception, e:
            if error_handler:
                error_handler(e)
            else:
                raise

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='',
                         async_callbacks=('reply_handler', 'error_handler'))
    def disable_bandwidth_throttling(self, reply_handler=None,
                                     error_handler=None):
        """Disable bandwidth throttling."""
        try:
            self._set_throttling_enabled(False)
            if reply_handler:
                reply_handler()
            # pylint: disable-msg=W0703
        except Exception, e:
            if error_handler:
                error_handler(e)
            else:
                raise

    def _set_throttling_enabled(self, enabled):
        """set throttling enabled value and save the config"""
        # modify and save the config file
        user_config = config.get_user_config()
        user_config.set_throttling(enabled)
        user_config.save()
        # modify AQ settings
        if enabled:
            self.dbus_iface.action_queue.enable_throttling()
        else:
            self.dbus_iface.action_queue.disable_throttling()

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='b',
                         async_callbacks=('reply_handler', 'error_handler'))
    def bandwidth_throttling_enabled(self, reply_handler=None,
                                     error_handler=None):
        """Returns True (actually 1) if bandwidth throttling is enabled and
        False (0) otherwise.
        """
        enabled = self.dbus_iface.action_queue.throttling_enabled
        if reply_handler:
            reply_handler(enabled)
        else:
            return enabled

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='b')
    def udf_autosubscribe_enabled(self):
        """Return the udf_autosubscribe config value."""
        return config.get_user_config().get_udf_autosubscribe()

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='')
    def enable_udf_autosubscribe(self):
        """Enable UDF autosubscribe."""
        user_config = config.get_user_config()
        user_config.set_udf_autosubscribe(True)
        user_config.save()

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='')
    def disable_udf_autosubscribe(self):
        """Enable UDF autosubscribe."""
        user_config = config.get_user_config()
        user_config.set_udf_autosubscribe(False)
        user_config.save()

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='b', out_signature='')
    def set_files_sync_enabled(self, enabled):
        """Enable UDF autosubscribe."""
        logger.debug('called set_files_sync_enabled %d', enabled)
        user_config = config.get_user_config()
        user_config.set_files_sync_enabled(bool(int(enabled)))
        user_config.save()

    @dbus.service.method(DBUS_IFACE_CONFIG_NAME,
                         in_signature='', out_signature='b')
    def files_sync_enabled(self):
        logger.debug('called files_sync_enabled')
        """Return the udf_autosubscribe config value."""
        return config.get_user_config().get_files_sync_enabled()


class Folders(DBusExposedObject):
    """A dbus interface to interact with User Defined Folders"""

    def __init__(self, bus_name, volume_manager, fs_manager):
        """ Creates the instance. """
        self.vm = volume_manager
        self.fs = fs_manager
        self.path = '/folders'
        DBusExposedObject.__init__(self, bus_name=bus_name,
                                   path=self.path)

    @dbus.service.method(DBUS_IFACE_FOLDERS_NAME, in_signature='s')
    def create(self, path):
        """Create a user defined folder in the specified path."""
        logger.debug('Folders.create: %r', path)
        try:
            path = os.path.normpath(path)
            self.vm.create_udf(path.encode('utf-8'))
        except Exception, e:
            logger.exception('Error while creating udf: %r', path)
            self.emit_folder_create_error(path, str(e))

    @dbus.service.method(DBUS_IFACE_FOLDERS_NAME, in_signature='s')
    def delete(self, folder_id):
        """Delete the folder specified by folder_id"""
        logger.debug('Folders.delete: %r', folder_id)
        try:
            self.vm.delete_volume(folder_id)
        except VolumeDoesNotExist, e:
            self.FolderDeleteError({'volume_id':folder_id}, str(e))
        except Exception, e:
            logger.exception('Error while deleting volume: %r', folder_id)
            self.FolderDeleteError({'volume_id':folder_id}, str(e))

    @dbus.service.method(DBUS_IFACE_FOLDERS_NAME, out_signature='aa{ss}')
    def get_folders(self):
        """Return the list of folders (a list of dicts)"""
        logger.debug('Folders.get_folders')
        return [_get_udf_dict(udf) for udf in self.vm.udfs.values()]

    @dbus.service.method(DBUS_IFACE_FOLDERS_NAME,
                         in_signature='s', out_signature='')
    def subscribe(self, folder_id):
        """Subscribe to the specified folder"""
        logger.debug('Folders.subscribe: %r', folder_id)
        try:
            self.vm.subscribe_udf(folder_id)
        except Exception:
            logger.exception('Error while subscribing udf: %r', folder_id)
            raise

    @dbus.service.method(DBUS_IFACE_FOLDERS_NAME,
                         in_signature='s', out_signature='')
    def unsubscribe(self, folder_id):
        """Unsubscribe from the specified folder"""
        logger.debug('Folders.unsubscribe: %r', folder_id)
        try:
            self.vm.unsubscribe_udf(folder_id)
        except Exception:
            logger.exception('Error while unsubscribing udf: %r', folder_id)
            raise

    @dbus.service.method(DBUS_IFACE_FOLDERS_NAME,
                         in_signature='s', out_signature='a{ss}')
    def get_info(self, path):
        """Returns a dict containing the folder information."""
        logger.debug('Folders.get_info: %r', path)
        mdobj = self.fs.get_by_path(path.encode('utf-8'))
        udf = self.vm.udfs.get(mdobj.share_id, None)
        if udf is None:
            return dict()
        else:
            return _get_udf_dict(udf)

    @dbus.service.method(DBUS_IFACE_FOLDERS_NAME,
                         in_signature='', out_signature='')
    def refresh_volumes(self):
        """Refresh the volumes list, requesting it to the server."""
        self.vm.refresh_volumes()

    def emit_folder_created(self, folder):
        """Emit the FolderCreated signal"""
        udf_dict = _get_udf_dict(folder)
        self.FolderCreated(udf_dict)

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}')
    def FolderCreated(self, folder_info):
        """Notify the creation of a user defined folder."""
        pass

    def emit_folder_create_error(self, path, error):
        """Emit the FolderCreateError signal"""
        info = dict(path=path.decode('utf-8'))
        self.FolderCreateError(info, str(error))

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}s')
    def FolderCreateError(self, folder_info, error):
        """Notify an error during the creation of a user defined folder."""
        pass

    def emit_folder_deleted(self, folder):
        """Emit the FolderCreated signal"""
        udf_dict = _get_udf_dict(folder)
        self.FolderDeleted(udf_dict)

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}')
    def FolderDeleted(self, folder_info):
        """Notify the deletion of a user defined folder."""
        pass

    def emit_folder_delete_error(self, folder, error):
        """Emit the FolderCreateError signal"""
        udf_dict = _get_udf_dict(folder)
        self.FolderDeleteError(udf_dict, str(error))

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}s')
    def FolderDeleteError(self, folder_info, error):
        """Notify an error during the deletion of a user defined folder."""
        pass

    def emit_folder_subscribed(self, folder):
        """Emit the FolderSubscribed signal"""
        udf_dict = _get_udf_dict(folder)
        self.FolderSubscribed(udf_dict)

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}')
    def FolderSubscribed(self, folder_info):
        """Notify the subscription to a user defined folder."""
        pass

    def emit_folder_subscribe_error(self, folder_id, error):
        """Emit the FolderSubscribeError signal"""
        self.FolderSubscribeError({'id':folder_id}, str(error))

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}s')
    def FolderSubscribeError(self, folder_info, error):
        """Notify an error while subscribing to a user defined folder."""
        pass

    def emit_folder_unsubscribed(self, folder):
        """Emit the FolderUnSubscribed signal"""
        udf_dict = _get_udf_dict(folder)
        self.FolderUnSubscribed(udf_dict)

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}')
    def FolderUnSubscribed(self, folder_info):
        """Notify the unsubscription to a user defined folder."""
        pass

    def emit_folder_unsubscribe_error(self, folder_id, error):
        """Emit the FolderUnSubscribeError signal"""
        self.FolderUnSubscribeError({'id':folder_id}, str(error))

    @dbus.service.signal(DBUS_IFACE_FOLDERS_NAME,
                         signature='a{ss}s')
    def FolderUnSubscribeError(self, folder_info, error):
        """Notify an error while unsubscribing from a user defined folder."""
        pass


class PublicFiles(DBusExposedObject):
    """A DBus interface for handling public files."""

    def __init__(self, bus_name, fs_manager, action_queue):
        self.fs = fs_manager
        self.aq = action_queue
        self.path = '/publicfiles'
        DBusExposedObject.__init__(self, bus_name=bus_name, path=self.path)

    @dbus.service.method(DBUS_IFACE_PUBLIC_FILES_NAME,
                         in_signature='ssb', out_signature='')
    def change_public_access(self, share_id, node_id, is_public):
        """Change the public access of a file."""
        logger.debug('PublicFiles.change_public_access: %r, %r, %r',
                     share_id, node_id, is_public)
        if share_id:
            share_id = uuid.UUID(share_id)
        else:
            share_id = None
        node_id = uuid.UUID(node_id)
        self.aq.change_public_access(share_id, node_id, is_public)

    @dbus.service.method(DBUS_IFACE_PUBLIC_FILES_NAME)
    def get_public_files(self):
        """Request the list of public files to the server.

        The result will be send in a PublicFilesList signal.
        """
        self.aq.get_public_files()

    def emit_public_access_changed(self, share_id, node_id, is_public,
                                   public_url):
        """Emit the PublicAccessChanged signal."""
        share_id = str(share_id) if share_id else ''
        node_id = str(node_id)
        try:
            relpath = self.fs.get_by_node_id(share_id,
                                             node_id).path
        except KeyError:
            path=''
        else:
            path=self.fs.get_abspath(share_id, relpath)
        self.PublicAccessChanged(dict(
                share_id=str(share_id) if share_id else '',
                node_id=str(node_id),
                is_public=bool_str(is_public),
                public_url=public_url if public_url else '',
                path=path))

    @dbus.service.signal(DBUS_IFACE_PUBLIC_FILES_NAME,
                         signature='a{ss}')
    def PublicAccessChanged(self, file_info):
        """Notify the new public access state of a file."""

    def emit_public_access_change_error(self, share_id, node_id, error):
        """Emit the PublicAccessChangeError signal."""
        try:
            relpath = self.fs.get_by_node_id(share_id,
                                             node_id).path
        except KeyError:
            path=''
        else:
            path=self.fs.get_abspath(share_id, relpath)
        self.PublicAccessChangeError(dict(
                share_id=str(share_id) if share_id else '',
                node_id=str(node_id),
                path=path), str(error))

    @dbus.service.signal(DBUS_IFACE_PUBLIC_FILES_NAME,
                         signature='a{ss}s')
    def PublicAccessChangeError(self, file_info, error):
        """Report an error in changing the public access of a file."""

    @dbus.service.signal(DBUS_IFACE_PUBLIC_FILES_NAME,
                        signature='aa{ss}')
    def PublicFilesList(self, files):
        """Notify the list of public files."""

    @dbus.service.signal(DBUS_IFACE_PUBLIC_FILES_NAME,
                         signature='s')
    def PublicFilesListError(self, error):
        """Report an error in geting the public files list."""

    def emit_public_files_list(self, public_files):
        """Emit the PublicFilesList signal."""
        files = []
        for pf in public_files:
            try:
                volume_id = str(pf['volume_id'])
                node_id = str(pf['node_id'])
                public_url = str(pf['public_url'])
                relpath = self.fs.get_by_node_id(volume_id,
                                                 pf['node_id']).path
            except KeyError:
                pass
            else:
                path = self.fs.get_abspath(volume_id, relpath).decode('utf-8')
                files.append(dict(volume_id=volume_id, node_id=node_id,
                                  public_url=public_url, path=path))
        self.PublicFilesList(files)

    def emit_public_files_list_error(self, error):
        """Emit the PublicFilesListError signal."""
        self.PublicFilesListError(error)


class DBusInterface(object):
    """ Holder of all DBus exposed objects """
    test = False

    def __init__(self, bus, main, system_bus=None, send_events=False):
        """ Create the instance and add the exposed object to the
        specified bus.
        """
        self.bus = bus
        self.main = main
        self.event_queue = main.event_q
        self.action_queue = main.action_q
        self.fs_manager = main.fs
        self.volume_manager = main.vm
        self.send_events = send_events
        self.busName = dbus.service.BusName(DBUS_IFACE_NAME, bus=self.bus)
        self.status = Status(self.busName, self)
        self.events = Events(self.busName, self.event_queue)
        self.event_listener = EventListener(self)
        self.sync = SyncDaemon(self.busName, self)
        self.fs = FileSystem(self.busName, self.fs_manager,
                             self.action_queue)
        self.shares = Shares(self.busName, self.fs_manager,
                             self.volume_manager)
        self.folders = Folders(self.busName, self.volume_manager,
                               self.fs_manager)
        self.public_files = PublicFiles(
            self.busName, self.fs_manager, self.action_queue)
        self.config = Config(self.busName, self)
        if system_bus is None and not DBusInterface.test:
            logger.debug('using the real system bus')
            self.system_bus = self.bus.get_system()
        elif system_bus is None and DBusInterface.test:
            # this is just for the case when test_sync instatiate Main for
            # running it's tests as pqm don't have a system bus running
            logger.debug('using the session bus as system bus')
            self.system_bus = self.bus
        else:
            self.system_bus = system_bus
        if self.event_queue:
            self.event_queue.subscribe(self.event_listener)
            # on initialization, fake a SYS_NET_CONNECTED if appropriate
            if DBusInterface.test:
                # testing under sync; just do it
                logger.debug('using the fake NetworkManager')
                self.connection_state_changed(NM_STATE_CONNECTED)
            else:
                def error_handler(error):
                    """Handle errors from NM."""
                    logger.error(
                        "Error while getting the NetworkManager state %s",
                        error)
                    # If we get an error back from NetworkManager, we should
                    # just try to connect anyway; it probably means that
                    # NetworkManager is down or broken or something.
                    self.connection_state_changed(NM_STATE_CONNECTED)
                try:
                    nm = self.system_bus.get_object(
                        'org.freedesktop.NetworkManager',
                        '/org/freedesktop/NetworkManager',
                        follow_name_owner_changes=True)
                    iface = dbus.Interface(nm, 'org.freedesktop.NetworkManager')
                except dbus.DBusException, e:
                    if e.get_dbus_name() == \
                        'org.freedesktop.DBus.Error.ServiceUnknown':
                        # NetworkManager isn't running.
                        logger.warn("Unable to connect to NetworkManager. "
                                      "Assuming we have network.")
                        self.connection_state_changed(NM_STATE_CONNECTED)
                    else:
                        raise
                else:
                    iface.state(reply_handler=self.connection_state_changed,
                                error_handler=error_handler)

        # register a handler to NM StateChanged signal
        self.system_bus.add_signal_receiver(self.connection_state_changed,
                               signal_name='StateChanged',
                               dbus_interface='org.freedesktop.NetworkManager',
                               path='/org/freedesktop/NetworkManager')

        self.oauth_credentials = None
        self._deferred = None # for firing login/registration

        logger.info('DBusInterface initialized.')

    def shutdown(self, with_restart=False):
        """ remove the registered object from the bus and unsubscribe from the
        event queue.
        """
        logger.info('Shuttingdown DBusInterface!')
        self.status.remove_from_connection()
        self.events.remove_from_connection()
        self.sync.remove_from_connection()
        self.fs.remove_from_connection()
        self.shares.remove_from_connection()
        self.config.remove_from_connection()
        self.event_queue.unsubscribe(self.event_listener)
        self.folders.remove_from_connection()
        # remove the NM's StateChanged signal receiver
        self.system_bus.remove_signal_receiver(self.connection_state_changed,
                               signal_name='StateChanged',
                               dbus_interface='org.freedesktop.NetworkManager',
                               path='/org/freedesktop/NetworkManager')
        self.bus.release_name(self.busName.get_name())
        if with_restart:
            # this is what activate_name_owner boils down to, except that
            # activate_name_owner blocks, which is a luxury we can't allow
            # ourselves.
            self.bus.call_async(dbus.bus.BUS_DAEMON_NAME,
                                dbus.bus.BUS_DAEMON_PATH,
                                dbus.bus.BUS_DAEMON_IFACE,
                                'StartServiceByName', 'su',
                                (DBUS_IFACE_NAME, 0),
                                self._restart_reply_handler,
                                self._restart_error_handler)

    def _restart_reply_handler(self, *args):
        """
        This is called by the restart async call.

        It's here to be stepped on from tests; in production we are
        going away and don't really care if the async call works or
        not: there is nothing we can do about it.
        """
    _restart_error_handler = _restart_reply_handler

    def connection_state_changed(self, state):
        """ Push a connection state changed event to the Event Queue. """
        event = NM_STATE_EVENTS.get(state, None)
        if event is not None:
            self.event_queue.push(event)

    @defer.inlineCallbacks
    def connect(self):
        """Push the SYS_USER_CONNECT event with the token.

        The token is requested via ubuntu-sso-client.

        """
        if self.oauth_credentials is not None:
            logger.debug('connect: oauth credentials were given by parameter.')
            ckey = csecret = key = secret = None
            if len(self.oauth_credentials) == 4:
                ckey, csecret, key, secret = self.oauth_credentials
            elif len(self.oauth_credentials) == 2:
                ckey, csecret = ('ubuntuone', 'hammertime')
                key, secret = self.oauth_credentials
            else:
                msg = 'connect: oauth_credentials (%s) was set but is useless!'
                logger.warning(msg, self.oauth_credentials)
                return
            token = {'consumer_key': ckey, 'consumer_secret': csecret,
                     'token': key, 'token_secret': secret}
        else:
            token = yield self._request_token()

        logger.info('connect: credential request was successful, '
                    'pushing SYS_USER_CONNECT.')
        self.event_queue.push('SYS_USER_CONNECT', access_token=token)

    def _signal_handler(self, *args, **kwargs):
        """Generic signal handler."""
        member = kwargs.get('member', None)
        app_name = args[0] if len(args) > 0 else None
        d = self._deferred
        logger.debug('Handling DBus signal for member: %r, app_name: %r.',
                     member, app_name)

        if app_name != APP_NAME:
            logger.info('Received %s but app_name %s does not match %s, ' \
                        'exiting.', member, app_name, APP_NAME)
            return

        if member in ('CredentialsError', 'AuthorizationDenied'):
            logger.warning('%r: %r %r', member, args, kwargs)
            if not args:
                d.errback(Failure(NoAccessToken(member)))
            else:
                d.errback(Failure(NoAccessToken("%s: %s %s" %
                                                (member, args, kwargs))))
        elif member == 'CredentialsFound' and not d.called:
            credentials = args[1]
            logger.info('%r: callbacking with credentials (token_name: %r).',
                        member, None) # fill token_name later, see bug #623604
            d.callback(credentials)
        else:
            logger.debug('_signal_handler: member %r not used or deferred '
                         'already called? %r.', member, d.called)

    def _request_token(self):
        """Request to SSO auth service to fetch the token."""
        self._deferred = d = defer.Deferred()

        def error_handler(error):
            """Default dbus error handler."""
            logger.error('Handling DBus error on _request_token: %r.', error)
            if not d.called:
                d.errback(Failure(error))

        # register signal handlers for each kind of error
        match = self.bus.add_signal_receiver(self._signal_handler,
                    member_keyword='member',
                    dbus_interface=DBUS_IFACE_CRED_NAME)
        # call ubuntu sso
        try:
            client = self.bus.get_object(DBUS_BUS_NAME, DBUS_CRED_PATH,
                                         follow_name_owner_changes=True)
            iface = dbus.Interface(client, DBUS_IFACE_CRED_NAME)
            iface.login_or_register_to_get_credentials(
                        APP_NAME, TC_URL, DESCRIPTION, 0, # window_id is 0
                        # ignore the reply, we get the result via signals
                        reply_handler=lambda: None,
                        error_handler=error_handler)
        except DBusException, e:
            error_handler(e)
        except:
            logger.exception('connect failed while getting the token')
            raise

        def remove_signal_receiver(r):
            """Cleanup the signal receivers."""
            self.bus.remove_signal_receiver(
                match, dbus_interface=DBUS_IFACE_CRED_NAME)
            return r

        d.addBoth(remove_signal_receiver)
        return d

    def disconnect(self):
        """ Push the SYS_USER_DISCONNECT event. """
        self.event_queue.push('SYS_USER_DISCONNECT')

    def quit(self):
        """ calls Main.quit. """
        logger.debug('Calling Main.quit')
        self.main.quit()
