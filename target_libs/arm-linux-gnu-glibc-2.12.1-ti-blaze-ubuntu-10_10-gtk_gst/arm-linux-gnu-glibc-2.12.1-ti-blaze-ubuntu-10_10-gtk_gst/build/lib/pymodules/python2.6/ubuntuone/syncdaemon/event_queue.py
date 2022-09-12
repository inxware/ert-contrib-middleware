# ubuntuone.syncdaemon.event_queue - Event queuing
#
# Author: Facundo Batista <facundo@canonical.com>
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
"""Module that implements the Event Queue machinery."""

import functools
import logging
import os
import re

import pyinotify
from twisted.internet import abstract, reactor, error, defer

from Queue import Queue, Empty

class InvalidEventError(Exception):
    """Received an Event that is not in the allowed list."""

# these are our internal events, what is inserted into the whole system
EVENTS = {
    'FS_FILE_OPEN': ('path',),
    'FS_FILE_CLOSE_NOWRITE': ('path',),
    'FS_FILE_CLOSE_WRITE': ('path',),
    'FS_FILE_CREATE': ('path',),
    'FS_DIR_CREATE': ('path',),
    'FS_FILE_DELETE': ('path',),
    'FS_DIR_DELETE': ('path',),
    'FS_FILE_MOVE': ('path_from', 'path_to',),
    'FS_DIR_MOVE': ('path_from', 'path_to',),
    'FS_INVALID_NAME': ('dirname', 'filename',),

    'AQ_FILE_NEW_OK': ('volume_id', 'marker', 'new_id', 'new_generation'),
    'AQ_FILE_NEW_ERROR': ('marker', 'failure'),
    'AQ_DIR_NEW_OK': ('volume_id', 'marker', 'new_id', 'new_generation'),
    'AQ_DIR_NEW_ERROR': ('marker', 'failure'),
    'AQ_MOVE_OK': ('share_id', 'node_id', 'new_generation'),
    'AQ_MOVE_ERROR': ('share_id', 'node_id',
                      'old_parent_id', 'new_parent_id', 'new_name', 'error'),
    'AQ_UNLINK_OK': ('share_id', 'parent_id', 'node_id', 'new_generation'),
    'AQ_UNLINK_ERROR': ('share_id', 'parent_id', 'node_id', 'error'),
    'AQ_DOWNLOAD_STARTED': ('share_id', 'node_id', 'server_hash'),
    'AQ_DOWNLOAD_FILE_PROGRESS': ('share_id', 'node_id',
                                  'n_bytes_read', 'deflated_size'),
    'AQ_DOWNLOAD_COMMIT': ('share_id', 'node_id', 'server_hash'),
    'AQ_DOWNLOAD_FINISHED': ('share_id', 'node_id', 'server_hash'),
    'AQ_DOWNLOAD_ERROR': ('share_id', 'node_id', 'server_hash', 'error'),
    'AQ_DOWNLOAD_CANCELLED': ('share_id', 'node_id', 'server_hash'),
    'AQ_DOWNLOAD_DOES_NOT_EXIST': ('share_id', 'node_id'),
    'AQ_UPLOAD_STARTED' : ('share_id', 'node_id', 'hash'),
    'AQ_UPLOAD_FILE_PROGRESS': ('share_id', 'node_id',
                                'n_bytes_written', 'deflated_size'),
    'AQ_UPLOAD_FINISHED': ('share_id', 'node_id', 'hash', 'new_generation'),
    'AQ_UPLOAD_ERROR': ('share_id', 'node_id', 'error', 'hash'),
    'AQ_SHARES_LIST': ('shares_list',),
    'AQ_LIST_SHARES_ERROR': ('error',),
    'AQ_CREATE_SHARE_OK': ('share_id', 'marker'),
    'AQ_CREATE_SHARE_ERROR': ('marker', 'error'),
    'AQ_DELETE_SHARE_OK': ('share_id',),
    'AQ_DELETE_SHARE_ERROR': ('share_id', 'error'),
    'AQ_QUERY_ERROR': ('item', 'error'),
    'AQ_ANSWER_SHARE_OK': ('share_id', 'answer'),
    'AQ_ANSWER_SHARE_ERROR': ('share_id', 'answer', 'error'),
    'AQ_FREE_SPACE_ERROR': ('error',),
    'AQ_ACCOUNT_ERROR': ('error',),
    'AQ_CREATE_UDF_OK': ('volume_id', 'node_id', 'marker'),
    'AQ_CREATE_UDF_ERROR': ('error', 'marker'),
    'AQ_LIST_VOLUMES': ('volumes',),
    'AQ_LIST_VOLUMES_ERROR': ('error',),
    'AQ_DELETE_VOLUME_OK': ('volume_id',),
    'AQ_DELETE_VOLUME_ERROR': ('volume_id', 'error'),
    'AQ_CHANGE_PUBLIC_ACCESS_OK': ('share_id', 'node_id', 'is_public',
                                   'public_url'),
    'AQ_CHANGE_PUBLIC_ACCESS_ERROR': ('share_id', 'node_id', 'error'),
    'AQ_PUBLIC_FILES_LIST_OK': ('public_files',),
    'AQ_PUBLIC_FILES_LIST_ERROR': ('error',),
    'AQ_DELTA_OK': ('volume_id', 'delta_content', 'end_generation',
                    'full', 'free_bytes'),
    'AQ_DELTA_ERROR': ('volume_id', 'error'),
    'AQ_DELTA_NOT_POSSIBLE': ('volume_id',),
    'AQ_RESCAN_FROM_SCRATCH_OK': ('volume_id', 'delta_content', 'end_generation',
                    'free_bytes'), # must always be full
    'AQ_RESCAN_FROM_SCRATCH_ERROR': ('volume_id', 'error'),

    'SV_SHARE_CHANGED': ('info',),
    'SV_SHARE_DELETED': ('share_id',),
    'SV_SHARE_ANSWERED': ('share_id', 'answer'),
    'SV_FREE_SPACE': ('share_id', 'free_bytes'),
    'SV_ACCOUNT_CHANGED': ('account_info',),
    'SV_VOLUME_CREATED': ('volume',),
    'SV_VOLUME_DELETED': ('volume_id',),
    'SV_VOLUME_NEW_GENERATION': ('volume_id', 'generation'),

    'HQ_HASH_NEW': ('path', 'hash', 'crc32', 'size', 'stat'),
    'HQ_HASH_ERROR': ('mdid',),

    'LR_SCAN_ERROR': ('mdid', 'udfmode'),

    'SYS_USER_CONNECT': ('access_token',),
    'SYS_USER_DISCONNECT': (),
    'SYS_STATE_CHANGED': ('state',),
    'SYS_NET_CONNECTED': (),
    'SYS_NET_DISCONNECTED': (),
    'SYS_INIT_DONE': (),
    'SYS_LOCAL_RESCAN_DONE': (),
    'SYS_CONNECTION_MADE': (),
    'SYS_CONNECTION_FAILED': (),
    'SYS_CONNECTION_LOST': (),
    'SYS_CONNECTION_RETRY': (),
    'SYS_PROTOCOL_VERSION_ERROR': ('error',),
    'SYS_PROTOCOL_VERSION_OK': (),
    'SYS_SET_CAPABILITIES_ERROR': ('error',),
    'SYS_SET_CAPABILITIES_OK': (),
    'SYS_AUTH_ERROR': ('error',),
    'SYS_AUTH_OK': (),
    'SYS_SERVER_RESCAN_DONE': (),
    'SYS_SERVER_RESCAN_ERROR': ('error',),
    'SYS_META_QUEUE_WAITING': (),
    'SYS_META_QUEUE_DONE': (),
    'SYS_CONTENT_QUEUE_WAITING': (),
    'SYS_CONTENT_QUEUE_DONE': (),
    'SYS_UNKNOWN_ERROR': (),
    'SYS_HANDSHAKE_TIMEOUT': (),
    'SYS_ROOT_RECEIVED': ('root_id', 'mdid'),
    'SYS_ROOT_MISMATCH': ('root_id', 'new_root_id'),
    'SYS_SERVER_ERROR': ('error',),
    'SYS_QUOTA_EXCEEDED': ('volume_id', 'free_bytes'),
    'SYS_BROKEN_NODE': ('volume_id', 'node_id', 'path', 'mdid'),

    'VM_UDF_SUBSCRIBED': ('udf',),
    'VM_UDF_SUBSCRIBE_ERROR': ('udf_id', 'error'),
    'VM_UDF_UNSUBSCRIBED': ('udf',),
    'VM_UDF_UNSUBSCRIBE_ERROR': ('udf_id', 'error'),
    'VM_UDF_CREATED': ('udf',),
    'VM_UDF_CREATE_ERROR': ('path', 'error'),
    'VM_SHARE_CREATED': ('share_id',),
    'VM_SHARE_DELETED': ('udf',),
    'VM_SHARE_DELETE_ERROR': ('path', 'error'),
    'VM_VOLUME_DELETED': ('volume',),
    'VM_VOLUME_DELETE_ERROR': ('volume_id', 'error'),
    'VM_SHARE_CHANGED': ('share_id',),

}

if 'IN_CREATE' in vars(pyinotify.EventsCodes):
    # < 0.8; event codes in EventsCodes; events have is_dir attribute
    evtcodes = pyinotify.EventsCodes
    event_is_dir = lambda event: event.is_dir
else:
    # >= 0.8; event codes in pyinotify itself; events have dir attribute
    evtcodes = pyinotify
    event_is_dir = lambda event: event.dir

# translates quickly the event and it's is_dir state to our standard events
NAME_TRANSLATIONS = {
    evtcodes.IN_OPEN: 'FS_FILE_OPEN',
    evtcodes.IN_CLOSE_NOWRITE: 'FS_FILE_CLOSE_NOWRITE',
    evtcodes.IN_CLOSE_WRITE: 'FS_FILE_CLOSE_WRITE',
    evtcodes.IN_CREATE: 'FS_FILE_CREATE',
    evtcodes.IN_CREATE | evtcodes.IN_ISDIR: 'FS_DIR_CREATE',
    evtcodes.IN_DELETE: 'FS_FILE_DELETE',
    evtcodes.IN_DELETE | evtcodes.IN_ISDIR: 'FS_DIR_DELETE',
    evtcodes.IN_MOVED_FROM: 'FS_FILE_DELETE',
    evtcodes.IN_MOVED_FROM | evtcodes.IN_ISDIR: 'FS_DIR_DELETE',
    evtcodes.IN_MOVED_TO: 'FS_FILE_CREATE',
    evtcodes.IN_MOVED_TO | evtcodes.IN_ISDIR: 'FS_DIR_CREATE',
}

# these are the events that will listen from inotify
INOTIFY_EVENTS_GENERAL = (
    evtcodes.IN_OPEN |
    evtcodes.IN_CLOSE_NOWRITE |
    evtcodes.IN_CLOSE_WRITE |
    evtcodes.IN_CREATE |
    evtcodes.IN_DELETE |
    evtcodes.IN_MOVED_FROM |
    evtcodes.IN_MOVED_TO |
    evtcodes.IN_MOVE_SELF
)
INOTIFY_EVENTS_ANCESTORS = (
    evtcodes.IN_DELETE |
    evtcodes.IN_MOVED_FROM |
    evtcodes.IN_MOVED_TO |
    evtcodes.IN_MOVE_SELF
)

DEFAULT_HANDLER = "handle_default" # receives (event_name, *args, **kwargs)


def validate_filename(real_func):
    """Decorator that validates the filename."""
    def func(self, event):
        """If valid, executes original function."""
        try:
            # validate UTF-8
            event.name.decode("utf8")
        except UnicodeDecodeError:
            dirname = event.path.decode("utf8")
            self.invnames_log.info("%s in %r: path %r", event.maskname,
                                   dirname, event.name)
            self.eq.push('FS_INVALID_NAME', dirname, event.name)
        else:
            real_func(self, event)
    return func


class MuteFilter(object):
    """Stores what needs to be muted."""
    def __init__(self):
        self._cnt = {}
        self.log = logging.getLogger('ubuntuone.SyncDaemon.MuteFilter')

    def add(self, element):
        """Add and element to the filter."""
        self.log.debug("Adding: %s", element)
        self._cnt[element] = self._cnt.get(element, 0) + 1

    def rm(self, element):
        """Remove an element from the filter."""
        self.log.debug("Removing: %s", element)
        new_val = self._cnt[element] - 1
        if new_val == 0:
            del self._cnt[element]
        else:
            self._cnt[element] = new_val

    def pop(self, element):
        """Pops an element from the filter, if there, and returns if it was."""
        if element not in self._cnt:
            return False

        self._cnt[element] = self._cnt.get(element, 0) - 1
        if not self._cnt[element]:
            # reached zero
            del self._cnt[element]

        # log what happened and how many items we have left
        q = sum(self._cnt.itervalues())
        self.log.debug("Blocking %s (%d left)", element, q)

        return True


class _AncestorsINotifyProcessor(pyinotify.ProcessEvent):
    """inotify's processor when an event happens on an UDFs ancestor."""
    def __init__(self, eq):
        self.log = logging.getLogger('ubuntuone.SyncDaemon.AncestorsINotProc')
        self.eq = eq

    def _get_udfs(self, path):
        """Yield all the subscribed udfs under a specific path."""
        pathsep = path + os.path.sep
        for udf in self.eq.fs.vm.udfs.itervalues():
            udfpath = udf.path + os.path.sep
            if udfpath.startswith(pathsep) and udf.subscribed:
                yield udf

    def process_IN_MOVE_SELF(self, event):
        """Don't do anything here.

        We just turned this event on because pyinotify does some
        path-fixing in its internal processing when this happens.
        """
    process_IN_MOVED_TO = process_IN_MOVE_SELF

    def process_IN_MOVED_FROM(self, event):
        """Getting it out or renaming means unsuscribe."""
        if event.mask & evtcodes.IN_ISDIR:
            unsubscribed_udfs = set()
            for udf in self._get_udfs(event.pathname):
                self.log.info("Got MOVED_FROM on path %r, unsubscribing "
                               "udf %s", event.pathname, udf)
                self.eq.fs.vm.unsubscribe_udf(udf.volume_id)
                unsubscribed_udfs.add(udf)
            self._unwatch_ancestors(unsubscribed_udfs)

    def process_IN_DELETE(self, event):
        """Check to see if the UDF was deleted."""
        if event.mask & evtcodes.IN_ISDIR:
            deleted_udfs = set()
            for udf in self._get_udfs(event.pathname):
                self.log.info("Got DELETE on path %r, deleting udf %s",
                               event.pathname, udf)
                self.eq.fs.vm.delete_volume(udf.volume_id)
                deleted_udfs.add(udf)
            self._unwatch_ancestors(deleted_udfs)

    def _unwatch_ancestors(self, udfs):
        """Unwatch the ancestors of the recevied udfs only."""
        # collect all the ancestors of the received udfs
        ancestors_to_unwatch = set()
        for udf in udfs:
            ancestors_to_unwatch.update(set(udf.ancestors))

        # collect the ancestors of all the still subscribed UDFs except
        # the received ones
        sub_udfs = (u for u in self.eq.fs.vm.udfs.itervalues() if u.subscribed)
        udf_remain = set(sub_udfs) - udfs
        ancestors_to_keep = set()
        for udf in udf_remain:
            ancestors_to_keep.update(set(udf.ancestors))

        # unwatch only the ancestors of the received udfs
        only_these = ancestors_to_unwatch - ancestors_to_keep
        for ancestor in only_these:
            self.eq.inotify_rm_watch(ancestor)


class _GeneralINotifyProcessor(pyinotify.ProcessEvent):
    """inotify's processor when a general event happens.

    This class also catchs the MOVEs events, and synthetises a new
    FS_(DIR|FILE)_MOVE event when possible.
    """
    def __init__(self, eq, ignore_config=None):
        self.log = logging.getLogger('ubuntuone.SyncDaemon.GeneralINotProc')
        self.invnames_log = logging.getLogger(
                                        'ubuntuone.SyncDaemon.InvalidNames')
        self.eq = eq
        self.held_event = None
        self.timer = None
        self.frozen_path = None
        self.frozen_evts = False
        self._to_mute = MuteFilter()
        self.conflict_RE = re.compile(r"\.u1conflict(?:\.\d+)?$")

        if ignore_config is not None:
            self.log.info("Ignoring files: %s", ignore_config)
            # thanks Chipaca for the following "regex composing"
            complex = '|'.join('(?:' + r + ')' for r in ignore_config)
            self.ignore_RE = re.compile(complex)
        else:
            self.ignore_RE = None

    def _mute_filter(self, action, event, *paths):
        """Really touches the mute filter."""
        # all events have one path except the MOVEs
        if event in ("FS_FILE_MOVE", "FS_DIR_MOVE"):
            f_path, t_path = paths
            is_from_forreal = not self.is_ignored(f_path)
            is_to_forreal = not self.is_ignored(t_path)
            if is_from_forreal and is_to_forreal:
                action((event, f_path, t_path))
            elif is_to_forreal:
                action(('FS_FILE_CREATE', t_path))
                action(('FS_FILE_CLOSE_WRITE', t_path))
        else:
            path = paths[0]
            if not self.is_ignored(path):
                action((event, path))

    def rm_from_mute_filter(self, event, *paths):
        """Remove an event and path(s) from the mute filter."""
        self._mute_filter(self._to_mute.rm, event, *paths)

    def add_to_mute_filter(self, event, *paths):
        """Add an event and path(s) to the mute filter."""
        self._mute_filter(self._to_mute.add, event, *paths)

    def on_timeout(self):
        """Called on timeout."""
        if self.held_event is not None:
            self.release_held_event(True)

    def release_held_event(self, timed_out=False):
        """Release the event on hold to fulfill its destiny."""
        if not timed_out:
            try:
                self.timer.cancel()
            except error.AlreadyCalled:
                # self.timeout() was *just* called, do nothing here
                return
        self.push_event(self.held_event)
        self.held_event = None

    @validate_filename
    def process_IN_OPEN(self, event):
        """Filter IN_OPEN to make it happen only in files."""
        if not (event.mask & evtcodes.IN_ISDIR):
            self.push_event(event)

    @validate_filename
    def process_IN_CLOSE_NOWRITE(self, event):
        """Filter IN_CLOSE_NOWRITE to make it happen only in files."""
        if not (event.mask & evtcodes.IN_ISDIR):
            self.push_event(event)

    def process_IN_MOVE_SELF(self, event):
        """Don't do anything here.

        We just turned this event on because pyinotify does some
        path-fixing in its internal processing when this happens.

        """

    @validate_filename
    def process_IN_MOVED_FROM(self, event):
        """Capture the MOVED_FROM to maybe syntethize FILE_MOVED."""
        if self.held_event is not None:
            self.release_held_event()

        self.held_event = event
        self.timer = reactor.callLater(1, self.on_timeout)

    def is_ignored(self, path):
        """should we ignore this path?"""
        # don't support symlinks yet
        if os.path.islink(path):
            return True

        # check if we are can read
        if os.path.exists(path) and not os.access(path, os.R_OK):
            self.log.warning("Ignoring path as we don't have enough "
                             "permissions to track it: %r", path)
            return True

        is_conflict = self.conflict_RE.search
        dirname, filename = os.path.split(path)
        # ignore conflicts
        if is_conflict(filename):
            return True
        # ignore partial downloads
        if filename == '.u1partial' or filename.startswith('.u1partial.'):
            return True

        # and ignore paths that are inside conflicts (why are we even
        # getting the event?)
        if any(part.endswith('.u1partial') or is_conflict(part)
               for part in dirname.split(os.path.sep)):
            return True

        if self.ignore_RE is not None and self.ignore_RE.match(filename):
            return True

        return False

    @validate_filename
    def process_IN_MOVED_TO(self, event):
        """Capture the MOVED_TO to maybe syntethize FILE_MOVED."""
        if self.held_event is not None:
            if event.cookie == self.held_event.cookie:
                try:
                    self.timer.cancel()
                except error.AlreadyCalled:
                    # self.timeout() was *just* called, do nothing here
                    pass
                else:
                    f_path_dir = self.held_event.path
                    f_path = os.path.join(f_path_dir, self.held_event.name)
                    t_path_dir = event.path
                    t_path = os.path.join(t_path_dir, event.name)

                    is_from_forreal = not self.is_ignored(f_path)
                    is_to_forreal = not self.is_ignored(t_path)
                    if is_from_forreal and is_to_forreal:
                        f_share_id = self.eq.fs.get_by_path(f_path_dir).share_id
                        t_share_id = self.eq.fs.get_by_path(t_path_dir).share_id
                        this_is_a_dir = event_is_dir(event)
                        if this_is_a_dir:
                            evtname = "FS_DIR_"
                        else:
                            evtname = "FS_FILE_"
                        if f_share_id != t_share_id:
                            # if the share_id are != push a delete/create
                            m = "Delete because of different shares: %r"
                            self.log.info(m, f_path)
                            self.eq_push(evtname+"DELETE", f_path)
                            self.eq_push(evtname+"CREATE", t_path)
                            if not this_is_a_dir:
                                self.eq_push('FS_FILE_CLOSE_WRITE', t_path)
                        else:
                            self.eq.inotify_watch_fix(f_path, t_path)
                            self.eq_push(evtname+"MOVE", f_path, t_path)
                    elif is_to_forreal:
                        # this is the case of a MOVE from something ignored
                        # to a valid filename
                        this_is_a_dir = event_is_dir(event)
                        if this_is_a_dir:
                            evtname = "FS_DIR_"
                        else:
                            evtname = "FS_FILE_"
                        self.eq_push(evtname+"CREATE", t_path)
                        if not this_is_a_dir:
                            self.eq_push('FS_FILE_CLOSE_WRITE', t_path)

                    self.held_event = None
                return
            else:
                self.release_held_event()
                self.push_event(event)
        else:
            # we don't have a held_event so this is a move from outside.
            # if it's a file move it's atomic on POSIX, so we aren't going to
            # receive a IN_CLOSE_WRITE, so let's fake it for files
            self.push_event(event)
            is_dir = event_is_dir(event)
            if not is_dir:
                t_path = os.path.join(event.path, event.name)
                self.eq_push('FS_FILE_CLOSE_WRITE', t_path)

    def eq_push(self, *event_data):
        """Sends to EQ the event data, maybe filtering it."""
        if not self._to_mute.pop(event_data):
            self.eq.push(*event_data)

    @validate_filename
    def process_default(self, event):
        """Push the event into the EventQueue."""
        if self.held_event is not None:
            self.release_held_event()
        self.push_event(event)

    def push_event(self, event):
        """Push the event to the EQ."""
        # ignore this trash
        if event.mask == evtcodes.IN_IGNORED:
            return

        # change the pattern IN_CREATE to FS_FILE_CREATE or FS_DIR_CREATE
        try:
            evt_name = NAME_TRANSLATIONS[event.mask]
        except:
            raise KeyError("Unhandled Event in INotify: %s" % event)

        # push the event
        fullpath = os.path.join(event.path, event.name)

        # check if the path is not frozen
        if self.frozen_path is not None:
            if event.path == self.frozen_path:
                # this will at least store the last one, for debug
                # purposses
                self.frozen_evts = (evt_name, fullpath)
                return

        if not self.is_ignored(fullpath):
            if evt_name == 'FS_DIR_DELETE':
                self.handle_dir_delete(fullpath)
            self.eq_push(evt_name, fullpath)

    def freeze_begin(self, path):
        """Puts in hold all the events for this path."""
        self.log.debug("Freeze begin: %r", path)
        self.frozen_path = path
        self.frozen_evts = False

    def freeze_rollback(self):
        """Unfreezes the frozen path, reseting to idle state."""
        self.log.debug("Freeze rollback: %r", self.frozen_path)
        self.frozen_path = None
        self.frozen_evts = False

    def freeze_commit(self, events):
        """Unfreezes the frozen path, sending received events if not dirty.

        If events for that path happened:
            - return True
        else:
            - push the here received events, return False
        """
        self.log.debug("Freeze commit: %r (%d events)",
                                                self.frozen_path, len(events))
        if self.frozen_evts:
            # ouch! we're dirty!
            self.log.debug("Dirty by %s", self.frozen_evts)
            self.frozen_evts = False
            return True

        # push the received events
        for evt_name, path in events:
            if not self.is_ignored(path):
                self.eq_push(evt_name, path)

        self.frozen_path = None
        self.frozen_evts = False
        return False

    def handle_dir_delete(self, fullpath):
        """Some special work when a directory is deleted."""
        # remove the watch on that dir from our structures
        self.eq.inotify_rm_watch(fullpath)

        # handle the case of move a dir to a non-watched directory
        paths = self.eq.fs.get_paths_starting_with(fullpath,
                                                   include_base=False)
        paths.sort(reverse=True)
        for path, is_dir in paths:
            m = "Pushing deletion because of parent dir move: (is_dir=%s) %r"
            self.log.info(m, is_dir, path)
            if is_dir:
                self.eq.inotify_rm_watch(path)
                self.eq_push('FS_DIR_DELETE', path)
            else:
                self.eq_push('FS_FILE_DELETE', path)


class EventQueue(object):
    """Manages the events from different sources and distributes them."""

    def __init__(self, fs, ignore_config=None):
        self._listeners = []

        self.log = logging.getLogger('ubuntuone.SyncDaemon.EQ')
        self.fs = fs

        # general inotify
        self._inotify_general_wm = wm = pyinotify.WatchManager()
        self._processor = _GeneralINotifyProcessor(self, ignore_config)
        self._inotify_notifier_gral = pyinotify.Notifier(wm, self._processor)
        self._inotify_reader_gral = self._hook_inotify_to_twisted(
                                            wm, self._inotify_notifier_gral)
        self._general_watchs = {}

        # ancestors inotify
        self._inotify_ancestors_wm = wm = pyinotify.WatchManager()
        antr_processor = _AncestorsINotifyProcessor(self)
        self._inotify_notifier_antr = pyinotify.Notifier(wm, antr_processor)
        self._inotify_reader_antr = self._hook_inotify_to_twisted(
                                            wm, self._inotify_notifier_antr)
        self._ancestors_watchs = {}

        self.dispatching = False
        self.dispatch_queue = Queue()
        self.empty_event_queue_callbacks = set()

    def add_to_mute_filter(self, *info):
        """Add info to mute filter in the processor."""
        self._processor.add_to_mute_filter(*info)

    def rm_from_mute_filter(self, *info):
        """Remove info to mute filter in the processor."""
        self._processor.rm_from_mute_filter(*info)

    def add_empty_event_queue_callback(self, callback):
        """add a callback for when the even queue has no more events."""
        self.empty_event_queue_callbacks.add(callback)
        if not self.dispatching and self.dispatch_queue.empty():
            if callable(callback):
                callback()

    def remove_empty_event_queue_callback(self, callback):
        """remove the callback"""
        self.empty_event_queue_callbacks.remove(callback)

    def _hook_inotify_to_twisted(self, wm, notifier):
        """This will hook inotify to twisted."""

        class MyReader(abstract.FileDescriptor):
            """Chain between inotify and twisted."""
            # will never pass a fd to write, pylint: disable-msg=W0223

            def fileno(self):
                """Returns the fileno to select()."""
                # pylint: disable-msg=W0212
                return wm._fd

            def doRead(self):
                """Called when twisted says there's something to read."""
                notifier.read_events()
                notifier.process_events()

        reader = MyReader()
        reactor.addReader(reader)
        return reader

    def shutdown(self):
        """Prepares the EQ to be closed."""
        self._inotify_notifier_gral.stop()
        self._inotify_notifier_antr.stop()
        reactor.removeReader(self._inotify_reader_gral)
        reactor.removeReader(self._inotify_reader_antr)

    def inotify_rm_watch(self, dirpath):
        """Remove watch from a dir."""
        if dirpath in self._general_watchs:
            w_dict = self._general_watchs
            w_manager = self._inotify_general_wm
        elif dirpath in self._ancestors_watchs:
            w_dict = self._ancestors_watchs
            w_manager = self._inotify_ancestors_wm
        else:
            self.log.warning("Tried to remove a nonexistent watch on %r",
                             dirpath)
            return

        wd = w_dict.pop(dirpath)
        w_manager.rm_watch(wd)

    def inotify_add_watch(self, dirpath):
        """Add watch to a dir."""
        # see where to add it
        if self._is_udf_ancestor(dirpath):
            w_type = "ancestors"
            w_manager = self._inotify_ancestors_wm
            w_dict = self._ancestors_watchs
            events = INOTIFY_EVENTS_ANCESTORS
        else:
            w_type = "general"
            w_manager = self._inotify_general_wm
            w_dict = self._general_watchs
            events = INOTIFY_EVENTS_GENERAL

        # add the watch!
        self.log.debug("Adding %s inotify watch to %r", w_type, dirpath)
        result = w_manager.add_watch(dirpath, events)
        w_dict[dirpath] = result[dirpath]

    def inotify_has_watch(self, dirpath):
        """Check if a dirpath is watched."""
        return (dirpath in self._general_watchs or
                dirpath in self._ancestors_watchs)

    def inotify_watch_fix(self, pathfrom, pathto):
        """Fix the path in inotify structures."""
        if pathfrom in self._general_watchs:
            wdict = self._general_watchs
        elif pathfrom in self._ancestors_watchs:
            wdict = self._ancestors_watchs
        else:
            m = "Tried to fix nonexistent path %r in watches (to %r)"
            self.log.warning(m, pathfrom, pathto)
            return

        # fix
        wdict[pathto] = wdict.pop(pathfrom)

    def _is_udf_ancestor(self, path):
        """Decide if path is an UDF ancestor or not."""
        for udf in self.fs.vm.udfs.itervalues():
            parent = os.path.dirname(udf.path) + os.path.sep
            if parent.startswith(path + os.path.sep):
                return True
        return False

    def unsubscribe(self, obj):
        """Remove the callback object from the listener queue.

        @param obj: the callback object to remove from the queue.
        """
        self._listeners.remove(obj)

    def subscribe(self, obj):
        """Store the callback object to whom push the events when received.

        @param obj: the callback object to add to the listener queue.

        These objects should provide a 'handle_FOO' to receive the FOO
        events (replace FOO with the desired event).
        """
        if obj not in self._listeners:
            self._listeners.append(obj)

    def push(self, event_name, *args, **kwargs):
        """Receives a push for all events.

        The signature for each event is forced on each method, not in this
        'push' arguments.
        """
        log_msg = "push_event: %s, args:%s, kw:%s"
        if event_name.endswith('DELETE'):
            # log every DELETE in INFO level
            self.log.info(log_msg, event_name, args, kwargs)
        elif event_name == 'SYS_USER_CONNECT':
            self.log.debug(log_msg, event_name, '*', '*')
        else:
            self.log.debug(log_msg, event_name, args, kwargs)

        # get the event parameters
        try:
            event_params = EVENTS[event_name]
        except KeyError:
            msg = "The received event_name (%r) is not valid!" % event_name
            self.log.error(msg)
            raise InvalidEventError(msg)

        # validate that the received arguments are ok
        if args:
            if len(args) > len(event_params):
                msg = "Too many arguments! (should receive %s)" % event_params
                self.log.error(msg)
                raise TypeError(msg)
            event_params = event_params[len(args):]

        s_eventparms = set(event_params)
        s_kwargs = set(kwargs.keys())
        if s_eventparms != s_kwargs:
            msg = "Wrong arguments for event %s (should receive %s, got %s)" \
                  % (event_name, event_params, kwargs.keys())
            self.log.error(msg)
            raise TypeError(msg)

        # check if we are currently dispatching an event
        self.dispatch_queue.put((event_name, args, kwargs))
        if not self.dispatching:
            self.dispatching = True
            while True:
                try:
                    event_name, args, kwargs = \
                            self.dispatch_queue.get(block=False)
                    self._dispatch(event_name, *args, **kwargs)
                except Empty:
                    self.dispatching = False
                    for callable in self.empty_event_queue_callbacks.copy():
                        callable()
                    break

    def _dispatch(self, event_name, *args, **kwargs):
        """ push the event to all listeners. """
        # check listeners to see if have the proper method, and call it
        meth_name = "handle_" + event_name
        for listener in self._listeners:
            # don't use hasattr because is expensive and
            # catch too many errors
            # we need to catch all here, pylint: disable-msg=W0703
            method = self._get_listener_method(listener, meth_name, event_name)
            if method is not None:
                try:
                    method(*args, **kwargs)
                except Exception:
                    self.log.exception("Error encountered while handling: %s"
                                     " in %s", event_name, listener)

    def _get_listener_method(self, listener, method_name, event_name):
        """ returns the method named method_name or hanlde_default from the
        listener. Or None if the methods are not defined in the listener.
        """
        method = getattr(listener, method_name, None)
        if method is None:
            method = getattr(listener, DEFAULT_HANDLER, None)
            if method is not None:
                method = functools.partial(method, event_name)
        return method

    def is_frozen(self):
        """Checks if there's something frozen."""
        return self._processor.frozen_path is not None

    def freeze_begin(self, path):
        """Puts in hold all the events for this path."""
        if self._processor.frozen_path is not None:
            raise ValueError("There's something already frozen!")
        self._processor.freeze_begin(path)

    def freeze_rollback(self):
        """Unfreezes the frozen path, reseting to idle state."""
        if self._processor.frozen_path is None:
            raise ValueError("Rolling back with nothing frozen!")
        self._processor.freeze_rollback()

    def freeze_commit(self, events):
        """Unfreezes the frozen path, sending received events if not dirty.

        If events for that path happened:
            - return True
        else:
            - push the here received events, return False
        """
        if self._processor.frozen_path is None:
            raise ValueError("Commiting with nothing frozen!")

        d = defer.execute(self._processor.freeze_commit, events)
        return d
