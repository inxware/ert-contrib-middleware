# ubuntuone.syncdaemon.hash_queue - hash queues
#
# Author: Facundo Batista <facundo@canonical.com>
#         Guillermo Gonzalez <guillermo.gonzalez@canonical.com>
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
"""Module that implements the Hash Queue machinery."""

from __future__ import with_statement

import logging
import threading
import Queue
import os

from twisted.internet import reactor

from ubuntuone.storageprotocol.content_hash import \
    content_hash_factory, crc32


class StopHashing(Exception):
    """The current hash was cancelled."""


class _Hasher(threading.Thread):
    """Class that lives in another thread, hashing all night long."""

    def __init__(self, queue, end_mark, event_queue):
        self.logger = logging.getLogger('ubuntuone.SyncDaemon.HQ.hasher')
        self.end_mark = end_mark
        self.queue = queue
        self.eq = event_queue
        # mutex to access _should_cancel and _hashing attributes
        self.mutex = threading.Lock()
        self._should_cancel = None
        self._stopped = True # start stopped
        self.chunk_size = 2**16
        self.hashing = None
        threading.Thread.__init__(self)

    def run(self):
        """Run the thread."""
        self._stopped = False
        while True:
            if self._stopped:
                break
            info = self.queue.get()
            if info is self.end_mark:
                self._stopped = True
                self.queue.task_done()
                break

            path, mdid = info
            with self.mutex:
                self.hashing = path
            m = "Hasher: got file to hash: path %r  mdid %s"
            self.logger.debug(m, path, mdid)
            try:
                result = self._hash(path)
            except (IOError, OSError), e:
                m = "Hasher: hash error %s  (path %r  mdid %s)"
                self.logger.debug(m, e, path, mdid)
                reactor.callFromThread(self.eq.push, "HQ_HASH_ERROR", mdid)
            except StopHashing, e:
                self.logger.debug(str(e))
            else:
                hashdata, crc, size, stat = result
                self.logger.debug("Hasher: path hash pushed:  path=%r  hash=%s"
                                  "  crc=%s  size=%d  st_ino=%d  st_size=%d"
                                  "  st_mtime=%r", path, hashdata,crc, size,
                                  stat.st_ino, stat.st_size, stat.st_mtime)
                reactor.callFromThread(self.eq.push,
                                       "HQ_HASH_NEW", path, *result)
            finally:
                with self.mutex:
                    self.hashing = None

            self.queue.task_done()

    def stop(self):
        """Stop the hasher.

        Will be effective in the next loop if a hash is in progress.

        """
        # clear the queue to push a end_mark, just to unblok if we are waiting
        # for a new item
        self.queue.clear()
        # set the end_mark in case we are waiting a path
        self.queue.put(self.end_mark)
        self._stopped = True

    def _hash(self, path):
        """Actually hashes a file."""
        hasher = content_hash_factory()
        crc = 0
        size = 0
        try:
            initial_stat = os.stat(path)
            with open(path) as fh:
                while True:
                    # stop hashing if path_to_cancel == path or _stopped is True
                    with self.mutex:
                        path_to_cancel = self._should_cancel
                    if path_to_cancel == path or self._stopped:
                        raise StopHashing('hashing of %r was cancelled' % path)
                    cont = fh.read(self.chunk_size)
                    if not cont:
                        break
                    hasher.update(cont)
                    crc = crc32(cont, crc)
                    size += len(cont)
        finally:
            with self.mutex:
                self._should_cancel = None

        return hasher.content_hash(), crc, size, initial_stat

    def busy(self):
        """Return whether we are busy."""
        with self.mutex:
            return self.hashing

    def cancel_if_running(self, path):
        """Request a cancel/stop of the current hash, if it's == path."""
        with self.mutex:
            if self.hashing == path:
                self._should_cancel = path


class HashQueue(object):
    """Interface between the real Hasher and the rest of the world."""

    def __init__(self, event_queue):
        self.logger = logging.getLogger('ubuntuone.SyncDaemon.HQ')
        self._stopped = False
        self._queue = UniqueQueue()
        self._end_mark = object()
        self.hasher = _Hasher(self._queue, self._end_mark, event_queue)
        self.hasher.setDaemon(True)
        self.hasher.start()
        self.logger.info("HashQueue: _hasher started")

    def insert(self, path, mdid):
        """Insert the path of a file to be hashed."""
        if self._stopped:
            self.logger.warning("HashQueue: already stopped when received "
                                "path %r  mdid %s", path, mdid)
            return
        self.logger.debug("HashQueue: inserting path %r  mdid %s", path, mdid)
        self.hasher.cancel_if_running(path)
        self._queue.put((path, mdid))

    def shutdown(self):
        """Shutdown all resources and clear the queue"""
        # clear the queue
        self._queue.clear()
        # stop the hasher
        self.hasher.stop()
        self._stopped = True
        self.logger.info("HashQueue: _hasher stopped")

    def empty(self):
        """Return whether we are empty or not"""
        return self._queue.empty() and not self.hasher.busy()

    def __len__(self):
        """Return the length of the queue (not reliable!)"""
        return self._queue.qsize()

    def is_hashing(self, path, mdid):
        """Return if the path is being hashed or in the queue."""
        if self.hasher.hashing == path:
            return True
        if (path, mdid) in self._queue:
            return True
        return False


class UniqueQueue(Queue.Queue):
    """Variant of Queue that only inserts unique items in the Queue
    """

    def __init__(self, *args, **kwargs):
        """create the instance"""
        Queue.Queue.__init__(self, *args, **kwargs)
        self._set = set() # to quickly check if a item is in the queue
        self.logger = logging.getLogger('ubuntuone.SyncDaemon.HQ.Queue')

    def _put(self, item):
        """Custom _put that avoid duplicate items in the queue."""
        if item not in self._set:
            Queue.Queue._put(self, item)
            self._set.add(item)
        else:
            self.logger.debug('Item already in the queue: %r', item)

    def _get(self):
        """Custom _get that removes the item from self._set."""
        item = Queue.Queue._get(self)
        self._set.discard(item)
        return item

    def clear(self):
        """clear the internal queue and notify all blocked threads"""
        self.queue.clear()
        with self.all_tasks_done:
            self.unfinished_tasks = 0
            self.all_tasks_done.notifyAll()

    def __contains__(self, item):
        """Tell if item in the queue."""
        return item in self._set
