# ubuntuone.syncdaemon.action_queue - Action queue
#
# Author: John Lenton <john.lenton@canonical.com>
# Author: Natalia B. Bidart <natalia.bidart@canonical.com>
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
"""The ActionQueue is where actions to be performed on the server are
queued up and then executed.

The idea is that there are two queues,
one for metadata and another for content; the metadata queue has
priority over the content queue.

"""
import base64
import itertools
import simplejson
import logging
import os
import random
import re
import tempfile
import traceback
import uuid
import zlib

from collections import deque, defaultdict
from functools import wraps, partial
from urllib import urlencode
from urllib2 import urlopen, Request, HTTPError
from urlparse import urljoin

from zope.interface import implements
from twisted.internet import reactor, defer, threads, task
from twisted.internet import error as twisted_errors
from twisted.names import client as dns_client
from twisted.python.failure import Failure, DefaultException

from oauth import oauth
from ubuntuone.storageprotocol import protocol_pb2
from ubuntuone.storageprotocol import errors as protocol_errors
from ubuntuone.storageprotocol.client import (
    ThrottlingStorageClient, ThrottlingStorageClientFactory
)
from ubuntuone.storageprotocol.context import get_ssl_context
from ubuntuone.syncdaemon.interfaces import IActionQueue, IMarker
from ubuntuone.syncdaemon.logger import mklog, TRACE

logger = logging.getLogger("ubuntuone.SyncDaemon.ActionQueue")

# I want something which repr() is "---" *without* the quotes :)
UNKNOWN = type('', (), {'__repr__': lambda _: '---'})()

# Regular expression to validate an e-mail address
EREGEX = "^.+\\@(\\[?)[a-zA-Z0-9\\-\\.]+\\.([a-zA-Z]{2,3}|[0-9]{1,3})(\\]?)$"

# progress threshold to emit a download/upload progress event: 64Kb
TRANSFER_PROGRESS_THRESHOLD = 64*1024*1024

def passit(func):
    """Pass the value on for the next deferred, while calling func with it."""

    @wraps(func)
    def wrapper(a):
        """Do it."""
        func(a)
        return a

    return wrapper


class UploadCompressionCancelled(Exception):
    """Compression of a file for upload cancelled."""


class RequestCleanedUp(Exception):
    """The request was cancelled by ActionQueue.cleanup()."""


class NamedTemporaryFile(object):
    """Like tempfile.NamedTemporaryFile, but working in 2.5.

    Also WRT the delete argument. Actually, one of these
    NamedTemporaryFile()s is the same as a
    tempfile.NamedTemporaryFile(delete=False) from 2.6.

    Or so the theory goes.

    """

    def __init__(self):
        fileno, self.name = tempfile.mkstemp()
        self._fd = os.fdopen(fileno, 'r+w')

    def __getattr__(self, attr):
        """Proxy everything else (other than .name) on to self._fd."""
        return getattr(self._fd, attr)


class LoggingStorageClient(ThrottlingStorageClient):
    """A subclass of StorageClient that logs.

    Specifically, it adds logging to processMessage and sendMessage.
    """

    def __init__(self):
        ThrottlingStorageClient.__init__(self)
        self.log = logging.getLogger('ubuntuone.SyncDaemon.StorageClient')
        # configure the handler level to be < than DEBUG
        self.log.setLevel(TRACE)
        self.log_trace = partial(self.log.log, TRACE)

    def processMessage(self, message):
        """Wrapper that logs the message and result."""
        # don't log the full message if it's of type BYTES
        if message.type == protocol_pb2.Message.BYTES:
            self.log_trace('start - processMessage: id: %s, type: %s',
                           message.id, message.type)
        else:
            self.log_trace('start - processMessage: %s',
                          str(message).replace("\n", " "))
        if message.id in self.requests:
            req = self.requests[message.id]
            req.deferred.addCallbacks(self.log_success, self.log_error)
        result = ThrottlingStorageClient.processMessage(self, message)
        self.log_trace('end - processMessage: id: %s - result: %s',
                       message.id, result)
        return result

    def log_error(self, failure):
        """Logging errback for requests."""
        self.log_trace('request error: %s', failure)
        return failure

    def log_success(self, result):
        """Logging callback for requests."""
        self.log_trace('request finished: %s', result)
        if getattr(result, '__dict__', None):
            self.log_trace('result.__dict__: %s', result.__dict__)
        return result

    def sendMessage(self, message):
        """Wrapper that logs the message and result."""
        # don't log the full message if it's of type BYTES
        if message.type == protocol_pb2.Message.BYTES:
            self.log_trace('start - sendMessage: id: %s, type: %s',
                           message.id, message.type)
        else:
            self.log_trace('start - sendMessage: %s',
                          str(message).replace("\n", " "))
        result = ThrottlingStorageClient.sendMessage(self, message)
        self.log_trace('end - sendMessage: id: %s', message.id)
        return result


class ActionQueueProtocol(LoggingStorageClient):
    """This is the Action Queue version of the StorageClient protocol."""

    factory = None
    _looping_ping = None
    _ping_delay = 600  # 10 minutes

    def connectionMade(self):
        """A new connection was made."""
        self.log.info('Connection made.')
        self.factory.event_queue.push('SYS_CONNECTION_MADE')
        if self._looping_ping is None:
            self._looping_ping = task.LoopingCall(self._do_ping)
            self._looping_ping.start(self._ping_delay, now=False)

    def connectionLost(self, reason):
        """The connection was lost."""
        self.log.info('Connection lost, reason: %s.', reason)
        if self._looping_ping is not None:
            self._looping_ping.stop()
            self._looping_ping = None

    @defer.inlineCallbacks
    def _do_ping(self):
        """Ping the server just to use the network."""
        req = yield self.ping()
        self.log.debug("Ping! rtt: %.3f segs", req.rtt)


class Marker(str):
    """A uuid4-based marker class."""

    implements(IMarker)

    def __new__(cls):
        return super(Marker, cls).__new__(cls, uuid.uuid4())

    def __repr__(self):
        return "marker:%s" % self


class ZipQueue(object):
    """A queue of files to be compressed for upload.

    Parts of this were shamelessly copied from
    twisted.internet.defer.DeferredSemaphore.

    See bug #373984

    """

    def __init__(self):
        self.waiting = deque()
        self.tokens = self.limit = 10

    def acquire(self):
        """Return a deferred which fires on token acquisition."""
        assert self.tokens >= 0, "Tokens should never be negative"
        d = defer.Deferred()
        if not self.tokens:
            self.waiting.append(d)
        else:
            self.tokens = self.tokens - 1
            d.callback(self)
        return d

    def release(self):
        """Release the token.

        Should be called by whoever did the acquire() when the shared
        resource is free.
        """
        assert self.tokens < self.limit, "Too many tokens!"
        self.tokens = self.tokens + 1
        if self.waiting:
            # someone is waiting to acquire token
            self.tokens = self.tokens - 1
            d = self.waiting.popleft()
            d.callback(self)

    def _compress(self, deferred, upload):
        """Compression background task."""
        try:
            fileobj = upload.fileobj_factory()
        except StandardError:
            # presumably the user deleted the file before we got to
            # upload it. Logging a warning just in case.
            upload.log.warn('unable to build fileobj'
                            ' (user deleted the file, maybe?)'
                            ' so cancelling the upload.')
            upload.cancel()
            fileobj = None

        filename = getattr(fileobj, 'name', '<?>')

        try:
            if upload.cancelled:
                raise UploadCompressionCancelled("Cancelled")
            upload.log.debug('compressing: %r', filename)
            # we need to compress the file completely to figure out its
            # compressed size. So streaming is out :(
            if upload.tempfile_factory is None:
                f = NamedTemporaryFile()
            else:
                f = upload.tempfile_factory()
            zipper = zlib.compressobj()
            while not upload.cancelled:
                data = fileobj.read(4096)
                if not data:
                    f.write(zipper.flush())
                    # no flush/sync because we don't need this to persist
                    # on disk; if the machine goes down, we'll lose it
                    # anyway (being in /tmp and all)
                    break
                f.write(zipper.compress(data))
            if upload.cancelled:
                raise UploadCompressionCancelled("Cancelled")
            upload.deflated_size = f.tell()
            # close the compressed file (thus, if you actually want to stream
            # it out, it must have a name so it can be reopnened)
            f.close()
            upload.tempfile = f
        except Exception, e: # pylint: disable-msg=W0703
            reactor.callFromThread(deferred.errback, e)
        else:
            reactor.callFromThread(deferred.callback, True)

    @defer.inlineCallbacks
    def zip(self, upload):
        """Acquire, do the compression in a thread, release."""
        deferred = defer.Deferred()

        yield self.acquire()
        try:
            reactor.callInThread(self._compress, deferred, upload)
        finally:
            self.release()

        # let's wait _compress to finish
        yield deferred


class RequestQueue(object):
    """
    RequestQueue is a queue that ensures that there is at most one
    request at a time 'on the wire', and that uses the action queue's
    states for its syncrhonization.
    """
    commutative = False

    def __init__(self, name, action_queue):
        super(RequestQueue, self).__init__()
        self.name = name
        self.action_queue = action_queue
        self.waiting = deque()
        self._head = None

    def __len__(self):
        """return the length of the queue"""
        return len(self.waiting)

    def cleanup_and_retry(self):
        """call cleanup_and_retry on the head"""
        if self._head is not None:
            cmd = self._head
            self._head = None
            cmd.cleanup_and_retry()

    def schedule_next(self, share_id, node_id):
        """
        Promote the newest command on the given share and node to
        the front of the queue, if it is possible to do so.
        """
        if not self.commutative:
            return
        for cmd in reversed(self.waiting):
            try:
                if cmd.share_id == share_id and cmd.node_id == node_id:
                    break
            except AttributeError:
                # not a command we can reschedule
                pass
        else:
            raise KeyError("No waiting command for that share and node")
        self.waiting.remove(cmd)
        self.waiting.appendleft(cmd)

    def queue(self, command, on_top=False):
        """Add a command to the queue."""
        # puts the command where it was asked for
        if on_top:
            self.waiting.appendleft(command)
        else:
            self.waiting.append(command)

        # if nothing running, and this command is the first in the
        # queue, send the signal
        if len(self.waiting) == 1 and not self._head:
            self.action_queue.event_queue.push('SYS_' + self.name + '_WAITING')

        # check conditions if the command is runnable
        if command.is_runnable:
            self.check_conditions()

    def done(self):
        """A command must call done() on the queue when it has finished."""
        self._head = None
        if len(self.waiting):
            event = 'SYS_' + self.name + '_WAITING'
        else:
            event = 'SYS_' + self.name + '_DONE'
        self.action_queue.event_queue.push(event)

    def _is_empty_or_runnable_waiting(self):
        """Returns True if there is an immediately runnable command in the
        queue, or if the queue is empty.
        """
        if not self.waiting:
            return True
        if self.commutative:
            for command in self.waiting:
                if command.is_runnable:
                    return True
            return False
        else:
            return self.waiting[0].is_runnable

    def _get_next_runnable_command(self):
        """Returns the next runnable command, if there is one."""
        if self.commutative:
            n_skipped = 0
            for command in self.waiting:
                if command.is_runnable:
                    break
                n_skipped += 1
            if n_skipped < len(self.waiting):
                # we found a runnable command
                if n_skipped > 0:
                    # move the skipped commands to the end
                    self.waiting.rotate(-n_skipped)
                command = self.waiting.popleft()
            else:
                command = None
        else:
            if self.waiting and self.waiting[0].is_runnable:
                command = self.waiting.popleft()
            else:
                command = None
        if command is None:
            flag = self._is_empty_or_runnable_waiting
            command = WaitForCondition(self, flag)
            command.pre_queue_setup()
        return command

    def check_conditions(self):
        """Check conditions on which the head command may be waiting."""
        if self._head is not None:
            self._head.check_conditions()

    def run(self):
        """Run the next available command in the queue."""
        if not self.waiting:
            logger.warning("run() called with %s queue empty!", self.name)
            return

        if self._head is not None:
            # already executing something, not start another command
            return

        self._head = command = self._get_next_runnable_command()
        d = command.run()
        d.addBoth(passit(lambda _: self.done()))
        return d

    def node_is_queued(self, command, share_id, node_id):
        """True if a command is queued for that node."""
        for cmd in itertools.chain((self._head,), self.waiting):
            if isinstance(cmd, command):
                if getattr(cmd, "node_id", None) == node_id and \
                   getattr(cmd, "share_id", None) == share_id:
                    return True

    def full_queue(self):
        """Get the full queue (head + waiting)."""
        return [self._head] + list(self.waiting)

    def remove(self, command):
        """Removes a command from 'waiting', if there."""
        if command in self.waiting:
            self.waiting.remove(command)


class UniqueRequestQueue(RequestQueue):
    """A unique RequestQueue.

    It only ever queues one command for each (share_id, node_id) pair.
    """

    def queue(self, command, on_top=False):
        """Add a command to the queue.

        If there is a command in the queue for the same node, it (the old
        one) will be removed, leaving queued only the new one.
        """
        for wc in self.waiting:
            if wc.share_id == command.share_id and \
               wc.node_id == command.node_id:
                self.waiting.remove(wc)
                m = "Request removed (%s), queing other (%s) for same node."
                logger.debug(m, wc, command)
                break
        return super(UniqueRequestQueue, self).queue(command, on_top)


class NoisyRequestQueue(RequestQueue):
    """A RequestQueue that notifies when it's changed."""

    def __init__(self, name, action_queue, change_notification_cb=None):
        self.set_change_notification_cb(change_notification_cb)
        super(NoisyRequestQueue, self).__init__(name, action_queue)

    def set_change_notification_cb(self, change_notification_cb):
        """Set the change notification callback."""
        if change_notification_cb is None:
            self.change_notification_cb = lambda *_: None
        else:
            self.change_notification_cb = change_notification_cb

    def queue(self, command, on_top=False):
        """Add a command to the queue.

        Calls the change notification callback
        """
        super(NoisyRequestQueue, self).queue(command, on_top)
        self.change_notification_cb(self._head, self.waiting)

    def get_head(self):
        """Getter for the queue's head."""
        return self.__head

    def set_head(self, head):
        """Setter for the queue's head.

        Calls he change notification callback.
        """
        self.change_notification_cb(head, self.waiting)
        self.__head = head

    _head = property(get_head, set_head)


class ContentQueue(NoisyRequestQueue, UniqueRequestQueue):
    """The content queue is a commutative (uniquified), noisy request queue."""
    commutative = True


class DeferredMap(object):
    """A mapping of deferred values.

    Return deferreds for a key that are fired (succesfully or not) later.
    """

    def __init__(self):
        self.waiting = defaultdict(list)

    def get(self, key):
        """Return a deferred for the given key."""
        d = defer.Deferred()
        self.waiting[key].append(d)
        return d

    def set(self, key, value):
        """We've got the value for a key!

        If it was waited, fire the waiting deferreds and remove the key.
        """
        if key in self.waiting:
            deferreds = self.waiting.pop(key)
            for d in deferreds:
                d.callback(value)

    def err(self, key, failure):
        """Something went terribly wrong in the process of getting a value.

        Break the news to the waiting deferreds and remove the key.
        """
        if key in self.waiting:
            deferreds = self.waiting.pop(key)
            for d in deferreds:
                d.errback(failure)


class UploadProgressWrapper(object):
    """A wrapper around the file-like object used for Uploads.

    It can be used to keep track of the number of bytes that have been
    written to the store and invokes a hook on progress.

    """

    __slots__ = ('fd', 'data_dict', 'n_bytes_read', 'progress_hook')

    def __init__(self, fd, data_dict, progress_hook):
        """
        fd is the file-like object used for uploads. data_dict is the
        entry in the uploading dictionary.
        """
        self.fd = fd
        self.data_dict = data_dict
        self.n_bytes_read = 0
        self.progress_hook = progress_hook

    def read(self, size=None):
        """
        read at most size bytes from the file-like object.

        Keep track of the number of bytes that have been read, and the
        number of bytes that have been written (assumed to be equal to
        the number of bytes read on the previews call to read). The
        latter is done directly in the data_dict.
        """
        self.data_dict['n_bytes_written'] = self.n_bytes_read
        self.progress_hook(self.n_bytes_read)

        data = self.fd.read(size)
        self.n_bytes_read += len(data)
        return data

    def __getattr__(self, attr):
        """
        Proxy all the rest.
        """
        return getattr(self.fd, attr)


class ActionQueue(ThrottlingStorageClientFactory, object):
    """The ActionQueue itself."""

    implements(IActionQueue)
    protocol = ActionQueueProtocol

    def __init__(self, event_queue, main, host, port, dns_srv,
                 use_ssl=False, disable_ssl_verify=False,
                 read_limit=None, write_limit=None, throttling_enabled=False,
                 connection_timeout=30):
        ThrottlingStorageClientFactory.__init__(self, read_limit=read_limit,
                                      write_limit=write_limit,
                                      throttling_enabled=throttling_enabled)
        self.event_queue = event_queue
        self.main = main
        self.host = host
        self.port = port
        self.dns_srv = dns_srv
        self.use_ssl = use_ssl
        self.disable_ssl_verify = disable_ssl_verify
        self.connection_timeout = connection_timeout

        # credentials
        self.token = None
        self.consumer = None

        self.client = None # an instance of self.protocol

        # is a twisted.internet.tcp/ssl.Connector instance
        self.connector = None # created on reactor.connectTCP/SSL
        # we need to track down if a connection is in progress
        # to avoid double connections
        self.connect_in_progress = False

        self.content_queue = ContentQueue('CONTENT_QUEUE', self)
        self.meta_queue = RequestQueue('META_QUEUE', self)
        self.uuid_map = DeferredMap()
        self.zip_queue = ZipQueue()

        self.estimated_free_space = {}

        self.uploading = {}
        self.downloading = {}

        event_queue.subscribe(self)

    def check_conditions(self):
        """Poll conditions on which running actions may be waiting."""
        self.content_queue.check_conditions()
        self.meta_queue.check_conditions()

    def have_sufficient_space_for_upload(self, share_id, upload_size):
        """Returns True if we have sufficient space for the given upload."""
        free = self.main.vm.get_free_space(share_id)
        enough = free is None or free >= upload_size
        if not enough:
            logger.info("Not enough space for upload %s bytes (available: %s)",
                        upload_size, free)
            self.event_queue.push('SYS_QUOTA_EXCEEDED', volume_id=share_id,
                                  free_bytes=free)

        return enough

    def handle_SYS_USER_CONNECT(self, access_token):
        """Stow the access token away for later use."""
        self.token = oauth.OAuthToken(access_token['token'],
                                      access_token['token_secret'])
        self.consumer = oauth.OAuthConsumer(access_token['consumer_key'],
                                            access_token['consumer_secret'])

    def cleanup(self):
        """Cancel, clean up, and reschedule things that were in progress."""
        self.meta_queue.cleanup_and_retry()
        self.content_queue.cleanup_and_retry()

    def _cleanup_connection_state(self, *args):
        """Reset connection state."""
        self.client = None
        self.connector = None
        self.connect_in_progress = False

    def _share_change_callback(self, info):
        """Called by the client when notified that a share changed."""
        self.event_queue.push('SV_SHARE_CHANGED', info=info)

    def _share_delete_callback(self, share_id):
        """Called by the client when notified that a share was deleted."""
        self.event_queue.push('SV_SHARE_DELETED', share_id=share_id)

    def _share_answer_callback(self, share_id, answer):
        """Called by the client when it gets a share answer notification."""
        self.event_queue.push('SV_SHARE_ANSWERED',
                              share_id=str(share_id), answer=answer)

    def _free_space_callback(self, share_id, free_bytes):
        """Called by the client when it gets a free space notification."""
        self.event_queue.push('SV_FREE_SPACE',
                              share_id=str(share_id), free_bytes=free_bytes)

    def _account_info_callback(self, account_info):
        """Called by the client when it gets an account info notification."""
        self.event_queue.push('SV_ACCOUNT_CHANGED',
                              account_info=account_info)

    def _volume_created_callback(self, volume):
        """Process new volumes."""
        self.event_queue.push('SV_VOLUME_CREATED', volume=volume)

    def _volume_deleted_callback(self, volume_id):
        """Process volume deletion."""
        self.event_queue.push('SV_VOLUME_DELETED', volume_id=volume_id)

    def _volume_new_generation_callback(self, volume_id, generation):
        """Process new volumes."""
        self.event_queue.push('SV_VOLUME_NEW_GENERATION',
                              volume_id=volume_id, generation=generation)

    def _lookup_srv(self):
        """Do the SRV lookup.

        Return a deferred whose callback is going to be called with
        (host, port). If we can't do the lookup, the default host, port
        is used.

        """

        def on_lookup_ok(results):
            """Get a random host from the SRV result."""
            logger.debug('SRV lookup done, choosing a server.')
            # pylint: disable-msg=W0612
            records, auth, add = results
            if not records:
                raise ValueError('No available records.')
            # pick a random server
            record = random.choice(records)
            logger.debug('Using record: %r', record)
            if record.payload:
                return record.payload.target.name, record.payload.port
            else:
                logger.info('Empty SRV record, fallback to %r:%r',
                            self.host, self.port)
                return self.host, self.port

        def on_lookup_error(failure):
            """Return the default host/post on a DNS SRV lookup failure."""
            logger.info("SRV lookup error, fallback to %r:%r \n%s",
                        self.host, self.port, failure.getTraceback())
            return self.host, self.port

        if self.dns_srv:
            # lookup the DNS SRV records
            d = dns_client.lookupService(self.dns_srv, timeout=[3, 2])
            d.addCallback(on_lookup_ok)
            d.addErrback(on_lookup_error)
            return d
        else:
            return defer.succeed((self.host, self.port))

    def _make_connection(self, result):
        """Do the real connect call."""
        host, port = result
        ssl_context = get_ssl_context(self.disable_ssl_verify)
        if self.use_ssl:
            self.connector = reactor.connectSSL(host, port, factory=self,
                                                contextFactory=ssl_context,
                                                timeout=self.connection_timeout)
        else:
            self.connector = reactor.connectTCP(host, port, self,
                                                timeout=self.connection_timeout)

    def connect(self):
        """Start the circus going."""
        # avoid multiple connections
        if self.connect_in_progress:
            msg = "Discarding new connection attempt, there is a connector!"
            logger.warning(msg)
            return

        self.connect_in_progress = True
        d = self._lookup_srv()
        # DNS lookup always succeeds, proceed to actually connect
        d.addCallback(self._make_connection)

    def buildProtocol(self, addr):
        """Build the client and store it. Connect callbacks."""
        # XXX: Very Important Note: within the storageprotocol project,
        # ThrottlingStorageClient.connectionMade sets self.factory.client
        # to self *if* self.factory.client is not None.
        # Since buildProcotol is called before connectionMade, the latter
        # does nothing (safely).
        self.client = ThrottlingStorageClientFactory.buildProtocol(self, addr)

        self.client.set_share_change_callback(self._share_change_callback)
        self.client.set_share_answer_callback(self._share_answer_callback)
        self.client.set_free_space_callback(self._free_space_callback)
        self.client.set_account_info_callback(self._account_info_callback)
        # volumes
        self.client.set_volume_created_callback(self._volume_created_callback)
        self.client.set_volume_deleted_callback(self._volume_deleted_callback)
        self.client.set_volume_new_generation_callback(
                                        self._volume_new_generation_callback)

        logger.info('Connection made.')
        return self.client

    def startedConnecting(self, connector):
        """Called when a connection has been started."""
        logger.info('Connection started to host %s, port %s.',
                    self.host, self.port)

    def disconnect(self):
        """Disconnect the client.

        This shouldn't be called if the client is already disconnected.

        """
        if self.connector is not None:
            self.connector.disconnect()
            self._cleanup_connection_state()
        else:
            msg = 'disconnect() was called when the connector was None.'
            logger.warning(msg)

        logger.debug("Disconnected.")

    def clientConnectionFailed(self, connector, reason):
        """Called when the connect() call fails."""
        self._cleanup_connection_state()
        self.event_queue.push('SYS_CONNECTION_FAILED')
        logger.info('Connection failed: %s', reason.getErrorMessage())

    def clientConnectionLost(self, connector, reason):
        """The client connection went down."""
        self._cleanup_connection_state()
        self.event_queue.push('SYS_CONNECTION_LOST')
        logger.warning('Connection lost: %s', reason.getErrorMessage())

    @defer.inlineCallbacks
    def _send_request_and_handle_errors(self, request, request_error,
                                        event_error, event_ok,
                                        handle_exception=True,
                                        args=(), kwargs={}):
        """Send 'request' to the server, using params 'args' and 'kwargs'.

        Expect 'request_error' as valid error, and push 'event_error' in that
        case. Do generic error handling for the rest of the protocol errors.

        """
        # if the client changes while we're waiting, this message is
        # old news and should be discarded (the message would
        # typically be a failure: timeout or disconnect). So keep the
        # original client around for comparison.
        client = self.client
        req_name = request.__name__
        failure = None
        event = None
        result = None
        try:
            try:
                result = yield request(*args, **kwargs)
            finally:
                # common handling for all cases
                if client is not self.client:
                    msg = "Client mismatch while processing the request '%s'" \
                          ", client (%r) is not self.client (%r)."
                    logger.warning(msg, req_name, client, self.client)
                    return
        except request_error, failure:
            event = event_error
            self.event_queue.push(event_error, error=str(failure))
        except protocol_errors.AuthenticationRequiredError, failure:
            # we need to separate this case from the rest because an
            # AuthenticationRequiredError is an StorageRequestError,
            # and we treat it differently.
            event = 'SYS_UNKNOWN_ERROR'
            self.event_queue.push(event)
        except protocol_errors.StorageRequestError, failure:
            event = 'SYS_SERVER_ERROR'
            self.event_queue.push(event, error=str(failure))
        except Exception, failure:
            if handle_exception:
                event = 'SYS_UNKNOWN_ERROR'
                self.event_queue.push(event)
            else:
                raise
        else:
            logger.info("The request '%s' finished OK.", req_name)
            if event_ok is not None:
                self.event_queue.push(event_ok)

        if failure is not None:
            logger.info("The request '%s' failed with the error: %s "
                        "and was handled with the event: %s",
                         req_name, failure, event)
        else:
            defer.returnValue(result)

    def check_version(self):
        """Check if the client protocol version matches that of the server."""
        check_version_d = self._send_request_and_handle_errors(
            request=self.client.protocol_version,
            request_error=protocol_errors.UnsupportedVersionError,
            event_error='SYS_PROTOCOL_VERSION_ERROR',
            event_ok='SYS_PROTOCOL_VERSION_OK'
        )
        return check_version_d

    @defer.inlineCallbacks
    def set_capabilities(self, caps):
        """Set the capabilities with the server."""

        @defer.inlineCallbacks
        def caps_raising_if_not_accepted(capability_method, caps, msg):
            """Discuss capabilities with the server."""
            client_caps = getattr(self.client, capability_method)
            req = yield client_caps(caps)
            if not req.accepted:
                raise StandardError(msg)
            defer.returnValue(req)

        error_msg = "The server doesn't have the requested capabilities"
        query_caps_d = self._send_request_and_handle_errors(
            request=caps_raising_if_not_accepted,
            request_error=StandardError,
            event_error='SYS_SET_CAPABILITIES_ERROR',
            event_ok=None,
            args=('query_caps', caps, error_msg)
        )
        req = yield query_caps_d

        # req can be None if set capabilities failed, error is handled by
        # _send_request_and_handle_errors
        if not req:
            return

        error_msg = "The server denied setting '%s' capabilities" % caps
        set_caps_d = self._send_request_and_handle_errors(
            request=caps_raising_if_not_accepted,
            request_error=StandardError,
            event_error='SYS_SET_CAPABILITIES_ERROR',
            event_ok='SYS_SET_CAPABILITIES_OK',
            args=('set_caps', caps, error_msg)
        )
        yield set_caps_d

    @defer.inlineCallbacks
    def authenticate(self):
        """Authenticate against the server using stored credentials."""
        authenticate_d = self._send_request_and_handle_errors(
            request=self.client.oauth_authenticate,
            request_error=protocol_errors.AuthenticationFailedError,
            event_error='SYS_AUTH_ERROR', event_ok='SYS_AUTH_OK',
            # XXX: handle self.token is None or self.consumer is None?
            args=(self.consumer, self.token)
        )
        req = yield authenticate_d

        # req can be None if the auth failed, but it's handled by
        # _send_request_and_handle_errors
        if req:
            # log the session_id
            logger.note('Session ID: %r', str(req.session_id))

    @defer.inlineCallbacks
    def query_volumes(self):
        """Get the list of volumes.

        This method will *not* queue a command, the request will be
        executed right away.
        """
        result = yield self._send_request_and_handle_errors(
            request=self.client.list_volumes,
            request_error=None, event_error=None,
            event_ok=None, handle_exception=False)
        defer.returnValue(result.volumes)

    def make_file(self, share_id, parent_id, name, marker):
        """See .interfaces.IMetaQueue."""
        return MakeFile(self.meta_queue, share_id, parent_id,
                        name, marker).queue()

    def make_dir(self, share_id, parent_id, name, marker):
        """See .interfaces.IMetaQueue."""
        return MakeDir(self.meta_queue, share_id, parent_id,
                       name, marker).queue()

    def move(self, share_id, node_id, old_parent_id, new_parent_id, new_name):
        """See .interfaces.IMetaQueue."""
        return Move(self.meta_queue, share_id, node_id, old_parent_id,
                    new_parent_id, new_name).queue()

    def unlink(self, share_id, parent_id, node_id):
        """See .interfaces.IMetaQueue."""
        return Unlink(self.meta_queue, share_id, parent_id, node_id).queue()

    def inquire_free_space(self, share_id):
        """See .interfaces.IMetaQueue."""
        return FreeSpaceInquiry(self.meta_queue, share_id).queue()

    def inquire_account_info(self):
        """See .interfaces.IMetaQueue."""
        return AccountInquiry(self.meta_queue).queue()

    def list_shares(self):
        """List the shares; put the result on the event queue."""
        return ListShares(self.meta_queue).queue()

    def answer_share(self, share_id, answer):
        """Answer the offer of a share."""
        return AnswerShare(self.meta_queue, share_id, answer).queue()

    def create_share(self, node_id, share_to, name, access_level, marker):
        """Share a node with somebody."""
        return CreateShare(self.meta_queue, node_id, share_to, name,
                           access_level, marker).queue()

    def delete_share(self, share_id):
        """Delete a offered share."""
        return DeleteShare(self.meta_queue, share_id).queue()

    def create_udf(self, path, name, marker):
        """Create a User Defined Folder.

        @param path: the path in disk to the UDF.
        @param name: the name of the UDF.
        @param marker: a marker identifying this UDF request.

        Result will be signaled using events:
            - AQ_CREATE_UDF_OK on succeess.
            - AQ_CREATE_UDF_ERROR on failure.

        """
        return CreateUDF(self.meta_queue, path, name, marker).queue()

    def list_volumes(self):
        """List all the volumes the user has.

        This includes the volumes:
            - all the user's UDFs.
            - all the shares the user has accepted.
            - the root-root volume.

        Result will be signaled using events.
            - AQ_LIST_VOLUMES for the volume list.

        """
        return ListVolumes(self.meta_queue).queue()

    def delete_volume(self, volume_id):
        """Delete a volume on the server, removing the associated tree.

        @param volume_id: the id of the volume to delete.

        Result will be signaled using events:
            - AQ_DELETE_VOLUME_OK on succeess.
            - AQ_DELETE_VOLUME_ERROR on failure.

        """
        return DeleteVolume(self.meta_queue, volume_id).queue()

    def change_public_access(self, share_id, node_id, is_public):
        """Change the public access of a file.

        @param share_id: the id of the share holding the file.
        @param node_id: the id of the file.
        @param is_public: whether to make the file public.

        Result will be signaled using events:
            - AQ_CHANGE_PUBLIC_ACCESS_OK on success.
            - AQ_CHANGE_PUBLIC_ACCESS_ERROR on failure.
        """
        return ChangePublicAccess(
            self.meta_queue, share_id, node_id, is_public).queue()

    def get_public_files(self):
        """Get the list of public files.

        Result will be signaled using events:
            - AQ_PUBLIC_FILES_LIST_OK on success.
            - AQ_PUBLIC_FILES_LIST_ERROR on failure.
        """
        return GetPublicFiles(self.meta_queue).queue()

    def download(self, share_id, node_id, server_hash, fileobj_factory):
        """See .interfaces.IContentQueue.download."""
        return Download(self.content_queue, share_id, node_id, server_hash,
                        fileobj_factory).queue()

    def upload(self, share_id, node_id, previous_hash, hash, crc32,
               size, fileobj_factory, tempfile_factory=None):
        """See .interfaces.IContentQueue."""
        return Upload(self.content_queue, share_id, node_id, previous_hash,
                      hash, crc32, size,
                      fileobj_factory, tempfile_factory).queue()

    def _cancel_op(self, share_id, node_id, cmdclass, logstr):
        """Generalized form of cancel_upload and cancel_download."""
        log = mklog(logger, logstr, share_id, node_id,
                    share=share_id, node=node_id)
        log.debug('starting')
        for queue in self.meta_queue, self.content_queue:
            for cmd in queue.full_queue():
                if isinstance(cmd, cmdclass) \
                        and share_id == cmd.share_id \
                        and node_id in (cmd.marker_maybe, cmd.node_id):
                    log.debug('cancelling')
                    cmd.cancel()
        log.debug('finished')

    def cancel_upload(self, share_id, node_id):
        """See .interfaces.IContentQueue."""
        self._cancel_op(share_id, node_id, Upload, 'cancel_upload')

    def cancel_download(self, share_id, node_id):
        """See .interfaces.IContentQueue."""
        self._cancel_op(share_id, node_id, Download, 'cancel_download')

    def node_is_with_queued_move(self, share_id, node_id):
        """True if a Move is queued for that node."""
        return self.meta_queue.node_is_queued(Move, share_id, node_id)

    def get_delta(self, volume_id, generation):
        """Get a delta from generation for the volume.

        @param volume_id: the id of the volume to get the delta.
        @param generation: the generation to get the delta from.

        Result will be signaled using events:
            - AQ_DELTA_OK on succeess.
            - AQ_DELTA_ERROR on generic failure.
            - AQ_DELTA_NOT_POSSIBLE if the server told so

        """
        return GetDelta(self.meta_queue, volume_id, generation).queue()

    def rescan_from_scratch(self, volume_id):
        """Get a delta from scratch for the volume.

        @param volume_id: the id of the volume to get the delta.

        Result will be signaled using events:
            - AQ_RESCAN_FROM_SCRATCH_OK on succeess.
            - AQ_RESCAN_FROM_SCRATCH_ERROR on generic failure.
        """
        return GetDeltaFromScratch(self.meta_queue, volume_id).queue()

    def handle_SYS_ROOT_RECEIVED(self, root_id, mdid):
        """Demark the root node_id."""
        self.uuid_map.set(mdid, root_id)


class ActionQueueCommand(object):
    """Base of all the action queue commands."""

    # the info used in the protocol errors is hidden, but very useful!
    # pylint: disable-msg=W0212
    suppressed_error_messages = (
        [x for x in protocol_errors._error_mapping.values()
         if x is not protocol_errors.InternalError] +
        [protocol_errors.RequestCancelledError,
         twisted_errors.ConnectionDone, RequestCleanedUp]
    )

    retryable_errors = (
        protocol_errors.TryAgainError,
        twisted_errors.ConnectionDone,
        twisted_errors.ConnectionLost,
    )

    logged_attrs = ()
    possible_markers = ()
    is_runnable = True

    __slots__ = ('_queue', 'start_done', 'running', 'log',
                 'markers_resolved_deferred')

    def __init__(self, request_queue):
        """Initialize a command instance."""
        self._queue = request_queue
        self.start_done = False
        self.running = False
        self.log = None
        self.markers_resolved_deferred = defer.Deferred()

    def to_dict(self):
        """Dump logged attributes to a dict."""
        return dict((n, getattr(self, n, None)) for n in self.logged_attrs)

    def make_logger(self):
        """Create a logger for this object."""
        share_id = getattr(self, "share_id", UNKNOWN)
        node_id = getattr(self, "node_id", None) or \
                      getattr(self, "marker", UNKNOWN)
        return mklog(logger, self.__class__.__name__,
                     share_id, node_id, **self.to_dict())

    def check_conditions(self):
        """Check conditions on which the command may be waiting."""

    @defer.inlineCallbacks
    def demark(self):
        """Arrange to have maybe_markers realized."""
        # we need to issue all the DeferredMap.get's right now, to be
        # dereferenced later
        waiting_structure = []
        for name in self.possible_markers:
            marker = getattr(self, name)

            # if a marker, get the real value; if not, it's already there, so
            # no action needed
            if IMarker.providedBy(marker):
                self.log.debug("waiting for the real value of %r", marker)
                d = self.action_queue.uuid_map.get(marker)
                waiting_structure.append((name, marker, d))

        # now, we wait for all the dereferencings... if any
        for (name, marker, deferred) in waiting_structure:
            try:
                value = yield deferred
            except Exception, e:
                # on first failure, errback the marker resolved flag, and
                # quit waiting for other deferreds
                self.log.error("failed %r", marker)
                self.markers_resolved_deferred.errback(e)
                break
            else:
                self.log.debug("for %r got value %r", marker, value)
                setattr(self, name, value)
        else:
            # fire the deferred only if all markers finished ok
            self.markers_resolved_deferred.callback(True)

    def end_callback(self, arg):
        """It worked!"""
        if self.running:
            self.log.debug('success')
            return self.handle_success(arg)
        else:
            self.log.debug('not running, so no success')

    def end_errback(self, failure):
        """It failed!"""
        error_message = failure.getErrorMessage()
        if failure.check(*self.suppressed_error_messages):
            self.log.warn('failure: %s', error_message)
        else:
            self.log.error('failure: %s', error_message)
            self.log.debug('traceback follows:\n\n' + failure.getTraceback())
        was_running = self.running
        self.cleanup()
        if failure.check(*self.retryable_errors):
            if was_running:
                reactor.callLater(0.1, self.retry)
        else:
            return self.handle_failure(failure)

    def pre_queue_setup(self):
        """Set up before the command gets really queued."""
        self.log = self.make_logger()
        self.demark()

    def queue(self):
        """Queue the command."""
        self.pre_queue_setup()
        self.log.debug('queueing in the %s', self._queue.name)
        self._queue.queue(self)

    def cleanup(self):
        """Do whatever is needed to clean up from a failure.

        For example, stop producers and others that aren't cleaned up
        appropriately on their own.
        """
        self.running = False
        self.log.debug('cleanup')

    def _start(self):
        """Do the specialized pre-run setup."""
        return defer.succeed(None)

    @defer.inlineCallbacks
    def run(self):
        """Do the deed."""
        self.running = True
        try:
            if self.start_done:
                self.log.debug('retrying')
            else:
                self.log.debug('starting')
                yield self._start()
                self.start_done = True
                yield self.markers_resolved_deferred
        except Exception, e:
            yield self.end_errback(Failure(e))
        else:
            yield self._ready_to_run()
        finally:
            self.running = False

    def _ready_to_run(self):
        self.log.debug('running')
        if self.running:
            d = self._run()
        else:
            d = defer.succeed(None)
        d.addCallbacks(self.end_callback, self.end_errback)
        return d

    def handle_success(self, success):
        """Do anthing that's needed to handle success of the operation."""
        return success

    def handle_failure(self, failure):
        """Do anthing that's needed to handle failure of the operation.

        Note that cancellation and TRY_AGAIN are already handled.
        """
        return failure

    def cleanup_and_retry(self):
        """First, cleanup; then, retry :)"""
        self.log.debug('cleanup and retry')
        self.cleanup()
        return self.retry()

    def retry(self):
        """Request cancelled or TRY_AGAIN. Well then, try again!"""
        self.running = False
        self.log.debug('will retry')
        return self._queue.queue(self, on_top=True)

    @property
    def action_queue(self):
        """Return the action queue."""
        return self._queue.action_queue

    def __str__(self, str_attrs=None):
        """Return a str representation of the instance."""
        if str_attrs is None:
            str_attrs = self.logged_attrs
        name = self.__class__.__name__
        if len(str_attrs) == 0:
            return name
        attrs = [str(attr) + '=' + str(getattr(self, attr, None) or 'None') \
                 for attr in str_attrs]
        return ''.join([name, '(', ', '.join([attr for attr in attrs]), ')'])


class WaitForCondition(ActionQueueCommand):
    """A command which waits for some condition to be satisfied."""

    __slots__ = ('_condition', '_deferred')

    def __init__(self, request_queue, condition):
        """Initializes the command instance."""
        ActionQueueCommand.__init__(self, request_queue)
        self._condition = condition
        self._deferred = None

    def _run(self):
        """Returns a deferred which blocks until the condition is satisfied."""
        if self._condition():
            d = defer.succeed(None)
            self._deferred = None
        else:
            d = defer.Deferred()
            self._deferred = d
        return d

    def check_conditions(self):
        """Poll the action's condition."""
        if self._deferred is not None and self._condition():
            try:
                self._deferred.callback(None)
                self._deferred = None
            except defer.AlreadyCalledError:
                return

    def cleanup(self):
        """Does cleanup."""
        self._deferred = None
        ActionQueueCommand.cleanup(self)


class MakeThing(ActionQueueCommand):
    """Base of MakeFile and MakeDir."""

    __slots__ = ('share_id', 'parent_id', 'name', 'marker')
    logged_attrs = __slots__
    possible_markers = 'parent_id',

    def __init__(self, request_queue, share_id, parent_id, name, marker):
        super(MakeThing, self).__init__(request_queue)
        self.share_id = share_id
        self.parent_id = parent_id
        # Unicode boundary! the name is Unicode in protocol and server, but
        # here we use bytes for paths
        self.name = name.decode("utf8")
        self.marker = marker

    def _run(self):
        """Do the actual running."""
        maker = getattr(self.action_queue.client, self.client_method)
        return maker(self.share_id, self.parent_id, self.name)

    def handle_success(self, request):
        """It worked! Push the event."""
        # note that we're not getting the new name from the answer
        # message, if we would get it, we would have another Unicode
        # boundary with it
        d = dict(marker=self.marker, new_id=request.new_id,
                 new_generation=request.new_generation,
                 volume_id=self.share_id)
        self.action_queue.event_queue.push(self.ok_event_name, **d)
        return request

    def handle_failure(self, failure):
        """It didn't work! Push the event."""
        self.action_queue.event_queue.push(self.error_event_name,
                                           marker=self.marker,
                                           failure=failure)


class MakeFile(MakeThing):
    """Make a file."""
    __slots__ = ()
    ok_event_name = 'AQ_FILE_NEW_OK'
    error_event_name = 'AQ_FILE_NEW_ERROR'
    client_method = 'make_file'


class MakeDir(MakeThing):
    """Make a directory."""
    __slots__ = ()
    ok_event_name = 'AQ_DIR_NEW_OK'
    error_event_name = 'AQ_DIR_NEW_ERROR'
    client_method = 'make_dir'


class Move(ActionQueueCommand):
    """Move a file or directory."""
    __slots__ = ('share_id', 'node_id', 'old_parent_id',
                 'new_parent_id', 'new_name')
    logged_attrs = __slots__
    possible_markers = 'node_id', 'old_parent_id', 'new_parent_id'

    def __init__(self, request_queue, share_id, node_id, old_parent_id,
                 new_parent_id, new_name):
        super(Move, self).__init__(request_queue)
        self.share_id = share_id
        self.node_id = node_id
        self.old_parent_id = old_parent_id
        self.new_parent_id = new_parent_id
        # Unicode boundary! the name is Unicode in protocol and server, but
        # here we use bytes for paths
        self.new_name = new_name.decode("utf8")

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.move(self.share_id,
                                             self.node_id,
                                             self.new_parent_id,
                                             self.new_name)

    def handle_success(self, request):
        """It worked! Push the event."""
        d = dict(share_id=self.share_id, node_id=self.node_id,
                 new_generation=request.new_generation)
        self.action_queue.event_queue.push('AQ_MOVE_OK', **d)
        return request

    def handle_failure(self, failure):
        """It didn't work! Push the event."""
        self.action_queue.event_queue.push('AQ_MOVE_ERROR',
                                           error=failure.getErrorMessage(),
                                           share_id=self.share_id,
                                           node_id=self.node_id,
                                           old_parent_id=self.old_parent_id,
                                           new_parent_id=self.new_parent_id,
                                           new_name=self.new_name)


class Unlink(ActionQueueCommand):
    """
    Unlink a file or dir
    """
    __slots__ = ('share_id', 'node_id', 'parent_id')
    logged_attrs = __slots__
    possible_markers = 'node_id', 'parent_id'

    def __init__(self, request_queue, share_id, parent_id, node_id):
        super(Unlink, self).__init__(request_queue)
        self.share_id = share_id
        self.node_id = node_id
        self.parent_id = parent_id

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.unlink(self.share_id, self.node_id)

    def handle_success(self, request):
        """It worked! Push the event."""
        d = dict(share_id=self.share_id, parent_id=self.parent_id,
                 node_id=self.node_id, new_generation=request.new_generation)
        self.action_queue.event_queue.push('AQ_UNLINK_OK', **d)
        return request

    def handle_failure(self, failure):
        """It didn't work! Push the event."""
        self.action_queue.event_queue.push('AQ_UNLINK_ERROR',
                                           error=failure.getErrorMessage(),
                                           share_id=self.share_id,
                                           parent_id=self.parent_id,
                                           node_id=self.node_id)


class ListShares(ActionQueueCommand):
    """
    List shares shared to me
    """
    __slots__ = ()

    def _run(self):
        """
        Do the actual running
        """
        return self.action_queue.client.list_shares()

    def handle_success(self, success):
        """
        It worked! Push the event.
        """
        self.action_queue.event_queue.push('AQ_SHARES_LIST',
                                           shares_list=success)

    def handle_failure(self, failure):
        """
        It didn't work! Push the event.
        """
        self.action_queue.event_queue.push('AQ_LIST_SHARES_ERROR',
                                           error=failure.getErrorMessage())


class FreeSpaceInquiry(ActionQueueCommand):
    """Inquire about free space."""

    def __init__(self, request_queue, share_id):
        """Initialize the instance."""
        super(FreeSpaceInquiry, self).__init__(request_queue)
        self.share_id = share_id

    def _run(self):
        """Do the query."""
        return self.action_queue.client.get_free_space(self.share_id)

    def handle_success(self, success):
        """Publish the free space information."""
        self.action_queue.event_queue.push('SV_FREE_SPACE',
                                           share_id=success.share_id,
                                           free_bytes=success.free_bytes)

    def handle_failure(self, failure):
        """Publish the error."""
        self.action_queue.event_queue.push('AQ_FREE_SPACE_ERROR',
                                           error=failure.getErrorMessage())


class AccountInquiry(ActionQueueCommand):
    """Query user account information."""

    def _run(self):
        """Make the actual request."""
        return self.action_queue.client.get_account_info()

    def handle_success(self, success):
        """Publish the account information to the event queue."""
        self.action_queue.event_queue.push('SV_ACCOUNT_CHANGED',
                                           account_info=success)

    def handle_failure(self, failure):
        """Publish the error."""
        self.action_queue.event_queue.push('AQ_ACCOUNT_ERROR',
                                           error=failure.getErrorMessage())


class AnswerShare(ActionQueueCommand):
    """Answer a share offer."""

    __slots__ = ('share_id', 'answer')

    def __init__(self, request_queue, share_id, answer):
        super(AnswerShare, self).__init__(request_queue)
        self.share_id = share_id
        self.answer = answer

    def _run(self):
        """
        Do the actual running
        """
        return self.action_queue.client.accept_share(self.share_id,
                                                     self.answer)

    def handle_success(self, success):
        """
        It worked! Push the event.
        """
        self.action_queue.event_queue.push('AQ_ANSWER_SHARE_OK',
                                           share_id=self.share_id,
                                           answer=self.answer)
        return success

    def handle_failure(self, failure):
        """
        It didn't work. Push the event.
        """
        self.action_queue.event_queue.push('AQ_ANSWER_SHARE_ERROR',
                                           share_id=self.share_id,
                                           answer=self.answer,
                                           error=failure.getErrorMessage())
        return failure


class CreateShare(ActionQueueCommand):
    """Offer a share to somebody."""

    __slots__ = ('node_id', 'share_to', 'name', 'access_level',
                 'marker', 'use_http')
    possible_markers = 'node_id',

    def __init__(self, request_queue, node_id, share_to, name, access_level,
                 marker):
        super(CreateShare, self).__init__(request_queue)
        self.node_id = node_id
        self.share_to = share_to
        self.name = name
        self.access_level = access_level
        self.marker = marker
        self.use_http = False

        if share_to and re.match(EREGEX, share_to):
            self.use_http = True

    def _create_share_http(self, node_id, user, name, read_only, deferred):
        """Create a share using the HTTP Web API method."""

        url = "https://one.ubuntu.com/files/api/offer_share/"
        method = oauth.OAuthSignatureMethod_PLAINTEXT()
        request = oauth.OAuthRequest.from_consumer_and_token(
            http_url=url,
            http_method="POST",
            oauth_consumer=self.action_queue.consumer,
            token=self.action_queue.token)
        request.sign_request(method, self.action_queue.consumer,
                             self.action_queue.token)
        data = dict(offer_to_email=user,
                    read_only=read_only,
                    node_id=node_id,
                    share_name=name)
        pdata = urlencode(data)
        headers = request.to_header()
        req = Request(url, pdata, headers)
        try:
            urlopen(req)
        except HTTPError, e:
            deferred.errback(Failure(e))

        deferred.callback(None)

    def _run(self):
        """
        Do the actual running
        """
        if self.use_http:
            # External user, do the HTTP REST method
            deferred = defer.Deferred()
            d = threads.deferToThread(self._create_share_http,
                                      self.node_id, self.share_to,
                                      self.name, self.access_level != 'Modify',
                                      deferred)
            d.addErrback(deferred.errback)
            return deferred
        else:
            return self.action_queue.client.create_share(self.node_id,
                                                         self.share_to,
                                                         self.name,
                                                         self.access_level)

    def handle_success(self, success):
        """
        It worked! Push the event.
        """
        # We don't get a share_id back from the HTTP REST method
        if not self.use_http:
            self.action_queue.event_queue.push('AQ_CREATE_SHARE_OK',
                                               share_id=success.share_id,
                                               marker=self.marker)
        return success

    def handle_failure(self, failure):
        """
        It didn't work! Push the event.
        """
        self.action_queue.event_queue.push('AQ_CREATE_SHARE_ERROR',
                                           marker=self.marker,
                                           error=failure.getErrorMessage())


class DeleteShare(ActionQueueCommand):
    """Delete a offered Share."""

    __slots__ = ('share_id',)

    def __init__(self, request_queue, share_id):
        super(DeleteShare, self).__init__(request_queue)
        self.share_id = share_id

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.delete_share(self.share_id)

    def handle_success(self, success):
        """It worked! Push the event."""
        self.action_queue.event_queue.push('AQ_DELETE_SHARE_OK',
                                           share_id=self.share_id)
        return success

    def handle_failure(self, failure):
        """It didn't work. Push the event."""
        self.action_queue.event_queue.push('AQ_DELETE_SHARE_ERROR',
                                           share_id=self.share_id,
                                           error=failure.getErrorMessage())


class CreateUDF(ActionQueueCommand):
    """Create a new User Defined Folder."""

    __slots__ = ('path', 'name', 'marker')

    def __init__(self, request_queue, path, name, marker):
        super(CreateUDF, self).__init__(request_queue)
        self.path = path
        # XXX Unicode boundary?
        self.name = name
        self.marker = marker

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.create_udf(self.path, self.name)

    def handle_success(self, success):
        """It worked! Push the success event."""
        kwargs = dict(marker=self.marker,
                      volume_id=success.volume_id,
                      node_id=success.node_id)
        self.action_queue.event_queue.push('AQ_CREATE_UDF_OK', **kwargs)
        return success

    def handle_failure(self, failure):
        """It didn't work! Push the failure event."""
        self.action_queue.event_queue.push('AQ_CREATE_UDF_ERROR',
                                           marker=self.marker,
                                           error=failure.getErrorMessage())


class ListVolumes(ActionQueueCommand):
    """List all the volumes for a given user."""

    __slots__ = ()

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.list_volumes()

    def handle_success(self, success):
        """It worked! Push the success event."""
        self.action_queue.event_queue.push('AQ_LIST_VOLUMES',
                                           volumes=success.volumes)
        return success

    def handle_failure(self, failure):
        """It didn't work! Push the failure event."""
        self.action_queue.event_queue.push('AQ_LIST_VOLUMES_ERROR',
                                           error=failure.getErrorMessage())


class DeleteVolume(ActionQueueCommand):
    """Delete an exsistent volume."""

    __slots__ = ('volume_id', 'marker')

    def __init__(self, request_queue, volume_id):
        super(DeleteVolume, self).__init__(request_queue)
        self.volume_id = volume_id

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.delete_volume(self.volume_id)

    def handle_success(self, success):
        """It worked! Push the success event."""
        self.action_queue.event_queue.push('AQ_DELETE_VOLUME_OK',
                                           volume_id=self.volume_id)
        return success

    def handle_failure(self, failure):
        """It didn't work! Push the failure event."""
        self.action_queue.event_queue.push('AQ_DELETE_VOLUME_ERROR',
                                           volume_id=self.volume_id,
                                           error=failure.getErrorMessage())

class DeltaList(list):
    """A list with a smal and fixed representation.

    We use delta lists instead of regular lists when we push deltas into
    the event queue so when we log the arguments of the event that was pushed
    we dont flood the logs.
    """

    def __init__(self, source):
        super(DeltaList, self).__init__()
        self[:] = source

    def __repr__(self):
        """A short representation for the list."""
        return "<DeltaList(len=%s)>" % (len(self),)

    __str__ = __repr__


class GetDelta(ActionQueueCommand):
    """Gets a delta from a generation for a volume."""

    def __init__(self, request_queue, volume_id, generation):
        super(GetDelta, self).__init__(request_queue)
        self.volume_id = volume_id
        self.generation = generation

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.get_delta(self.volume_id,
                                                  self.generation)

    def queue(self):
        """Queue the command only if it should."""
        # first start part to get the logger
        self.pre_queue_setup()

        for cmd in self._queue.waiting:
            if isinstance(cmd, GetDelta) and cmd.volume_id == self.volume_id:
                # other GetDelta for same volume! leave the smaller one
                if cmd.generation > self.generation:
                    m = "removing previous command because bigger gen num: %s"
                    self.log.debug(m, cmd)
                    self._queue.waiting.remove(cmd)
                    break
                else:
                    self.log.debug("not queueing self because there's other "
                                   "command with less or same gen num")
                    return

        # no similar command
        self.log.debug('queueing in the %s', self._queue.name)
        self._queue.queue(self)

    def handle_success(self, request):
        """It worked! Push the success event."""
        data = dict(
            volume_id=self.volume_id,
            delta_content=DeltaList(request.response),
            end_generation=request.end_generation,
            full=request.full,
            free_bytes=request.free_bytes,
        )
        self.action_queue.event_queue.push('AQ_DELTA_OK', **data)
        return request

    def handle_failure(self, failure):
        """It didn't work! Push the failure event."""
        if failure.check(protocol_errors.CannotProduceDelta):
            self.action_queue.event_queue.push('AQ_DELTA_NOT_POSSIBLE',
                                               volume_id=self.volume_id)
        else:
            self.action_queue.event_queue.push('AQ_DELTA_ERROR',
                                               volume_id=self.volume_id,
                                               error=failure.getErrorMessage())

    def make_logger(self):
        """Create a logger for this object."""
        return mklog(logger, 'GetDelta', self.volume_id,
                     None, generation=self.generation)


class GetDeltaFromScratch(ActionQueueCommand):
    """Gets a delta from scratch."""

    def __init__(self, request_queue, volume_id):
        super(GetDeltaFromScratch, self).__init__(request_queue)
        self.volume_id = volume_id

    def _run(self):
        """Do the actual running."""
        return self.action_queue.client.get_delta(self.volume_id,
                                                  from_scratch=True)

    def queue(self):
        """Queue the command only if it should."""
        # first start part to get the logger
        self.pre_queue_setup()

        for cmd in self._queue.waiting:
            if isinstance(cmd, GetDeltaFromScratch) and \
                    cmd.volume_id == self.volume_id:
                # other GetDeltaFromScratch for same volume! skip self
                m = "GetDeltaFromScratch already queued, not queueing self"
                self.log.debug(m)
                return

        # no similar command
        self.log.debug('queueing in the %s', self._queue.name)
        self._queue.queue(self)

    def handle_success(self, request):
        """It worked! Push the success event."""
        data = dict(
            volume_id=self.volume_id,
            delta_content=DeltaList(request.response),
            end_generation=request.end_generation,
            free_bytes=request.free_bytes,
        )
        self.action_queue.event_queue.push('AQ_RESCAN_FROM_SCRATCH_OK', **data)
        return request

    def handle_failure(self, failure):
        """It didn't work! Push the failure event."""
        self.action_queue.event_queue.push('AQ_RESCAN_FROM_SCRATCH_ERROR',
                                           volume_id=self.volume_id,
                                           error=failure.getErrorMessage())

    def make_logger(self):
        """Create a logger for this object."""
        return mklog(logger, 'GetDeltaFromScratch', self.volume_id, None)



class ChangePublicAccess(ActionQueueCommand):
    """Change the public access of a file."""

    __slots__ = ('share_id', 'node_id', 'is_public')
    possible_markers = 'node_id',

    def __init__(self, request_queue, share_id, node_id, is_public):
        super(ChangePublicAccess, self).__init__(request_queue)
        self.share_id = share_id
        self.node_id = node_id
        self.is_public = is_public

    def _change_public_access_http(self):
        """Change public access using the HTTP Web API method."""

        # Construct the node key.
        node_key = base64.urlsafe_b64encode(self.node_id.bytes).strip("=")
        if self.share_id is not None:
            node_key = "%s:%s" % (
                base64.urlsafe_b64encode(self.share_id.bytes).strip("="),
                node_key)

        url = "https://one.ubuntu.com/files/api/set_public/%s" % (node_key,)
        method = oauth.OAuthSignatureMethod_PLAINTEXT()
        request = oauth.OAuthRequest.from_consumer_and_token(
            http_url=url,
            http_method="POST",
            oauth_consumer=self.action_queue.consumer,
            token=self.action_queue.token)
        request.sign_request(method, self.action_queue.consumer,
                             self.action_queue.token)
        data = dict(is_public=bool(self.is_public))
        pdata = urlencode(data)
        headers = request.to_header()
        req = Request(url, pdata, headers)
        response = urlopen(req)
        return simplejson.load(response)

    def _run(self):
        """See ActionQueueCommand."""
        return threads.deferToThread(self._change_public_access_http)

    def handle_success(self, success):
        """See ActionQueueCommand."""
        self.action_queue.event_queue.push('AQ_CHANGE_PUBLIC_ACCESS_OK',
                                           share_id=self.share_id,
                                           node_id=self.node_id,
                                           is_public=success['is_public'],
                                           public_url=success['public_url'])
        return success

    def handle_failure(self, failure):
        """
        It didn't work! Push the event.
        """
        if issubclass(failure.type, HTTPError):
            message = failure.value.read()
        else:
            message = failure.getErrorMessage()
        self.action_queue.event_queue.push('AQ_CHANGE_PUBLIC_ACCESS_ERROR',
                                           share_id=self.share_id,
                                           node_id=self.node_id,
                                           error=message)


class GetPublicFiles(ActionQueueCommand):
    """Get the list of public files."""

    __slots__ = ('_url',)

    def __init__(self, request_queue, base_url='https://one.ubuntu.com'):
        super(GetPublicFiles, self).__init__(request_queue)
        self._url = urljoin(base_url, 'files/api/public_files')

    def _get_public_files_http(self):
        """Get public files list using the HTTP Web API method."""

        method = oauth.OAuthSignatureMethod_PLAINTEXT()
        request = oauth.OAuthRequest.from_consumer_and_token(
            http_url=self._url,
            http_method="GET",
            oauth_consumer=self.action_queue.consumer,
            token=self.action_queue.token)
        request.sign_request(method, self.action_queue.consumer,
                             self.action_queue.token)
        headers = request.to_header()
        req = Request(self._url, headers=headers)
        response = urlopen(req)
        files = simplejson.load(response)
        # translate nodekeys to (volume_id, node_id)
        for pf in files:
            _, node_id = self.split_nodekey(pf.pop('nodekey'))
            volume_id = pf['volume_id']
            pf['volume_id'] = '' if volume_id is None else volume_id
            pf['node_id'] = node_id
        return files

    def _run(self):
        """See ActionQueueCommand."""
        return threads.deferToThread(self._get_public_files_http)

    def handle_success(self, success):
        """See ActionQueueCommand."""
        self.action_queue.event_queue.push('AQ_PUBLIC_FILES_LIST_OK',
                                           public_files=success)
        return success

    def handle_failure(self, failure):
        """
        It didn't work! Push the event.
        """
        if issubclass(failure.type, HTTPError):
            message = failure.value.read()
        else:
            message = failure.getErrorMessage()
        self.action_queue.event_queue.push('AQ_PUBLIC_FILES_LIST_ERROR',
                                           error=message)

    def split_nodekey(self, nodekey):
        """Split a node key into a share_id, node_id."""
        if nodekey is None:
            return None, None
        if ":" in nodekey:
            parts = nodekey.split(":")
            return self.decode_uuid(parts[0]), self.decode_uuid(parts[1])
        else:
            return '', self.decode_uuid(nodekey)

    def decode_uuid(self, encoded):
        """Return a uuid from the encoded value.

        If the value isn't UUID, just return the decoded value
        """
        if encoded:
            data = str(encoded) + '=' * (len(encoded) % 4)
            value = base64.urlsafe_b64decode(data)
        try:
            return str(uuid.UUID(bytes=value))
        except ValueError:
            return value


class Download(ActionQueueCommand):
    """Get the contents of a file."""

    __slots__ = ('share_id', 'node_id', 'server_hash', 'fileobj_factory',
                 'fileobj', 'gunzip', 'marker_maybe', 'cancelled',
                 'download_req', 'deflated_size', 'n_bytes_read_last')
    logged_attrs = ('share_id', 'node_id', 'server_hash', 'fileobj_factory')
    possible_markers = 'node_id',

    def __init__(self, request_queue, share_id, node_id, server_hash,
                 fileobj_factory):
        super(Download, self).__init__(request_queue)
        self.share_id = share_id
        self.marker_maybe = self.node_id = node_id
        self.server_hash = server_hash
        self.fileobj_factory = fileobj_factory
        self.fileobj = None
        self.gunzip = zlib.decompressobj()
        self.cancelled = False
        self.download_req = None
        self.action_queue.cancel_download(self.share_id, self.node_id)

    def cancel(self):
        """Cancel the download."""
        self.cancelled = True
        if self.download_req is not None:
            self.download_req.cancel()
        self._queue.remove(self)
        self.cleanup()

    def _run(self):
        """Do the actual running."""
        if self.cancelled:
            return defer.fail(RequestCleanedUp('CANCELLED'))
        if self.fileobj is None:
            try:
                self.fileobj = self.fileobj_factory()
            except StandardError:
                self.log.debug(traceback.format_exc())
                msg = DefaultException('unable to build fileobj'
                                       ' (file went away?)'
                                       ' so aborting the download.')
                return defer.fail(Failure(msg))
        downloading = self.action_queue.downloading
        if (self.share_id, self.node_id) not in downloading:
            downloading[self.share_id, self.node_id] = {'n_bytes_read': 0,
                                                        'command': self}
        assert downloading[self.share_id, self.node_id]['command'] is self
        offset = downloading[self.share_id, self.node_id]['n_bytes_read']
        self.progress_start(offset)
        self.action_queue.event_queue.push('AQ_DOWNLOAD_STARTED',
                                           share_id=self.share_id,
                                           node_id=self.node_id,
                                           server_hash=self.server_hash)

        req = self.action_queue.client.get_content_request(
            self.share_id, self.node_id, self.server_hash,
            offset=offset,
            callback=self.cb, node_attr_callback=self.nacb)
        self.download_req = req
        downloading[self.share_id, self.node_id]['req'] = req
        d = req.deferred
        d.addBoth(lambda x: defer.fail(RequestCleanedUp('CANCELLED'))
                  if self.cancelled else x)
        d.addCallback(passit(
                lambda _: downloading.pop((self.share_id, self.node_id))))
        return d

    def handle_success(self, _):
        """It worked! Push the event."""
        self.sync()
        # send a COMMIT, the Nanny will issue the FINISHED if it's ok
        self.action_queue.event_queue.push('AQ_DOWNLOAD_COMMIT',
                                           share_id=self.share_id,
                                           node_id=self.node_id,
                                           server_hash=self.server_hash)

    def handle_failure(self, failure):
        """It didn't work! Push the event."""
        downloading = self.action_queue.downloading
        if (self.share_id, self.node_id) in downloading and \
            downloading[self.share_id, self.node_id]['command'] is self:
            del downloading[self.share_id, self.node_id]
        self.reset_fileobj()
        if failure.check(protocol_errors.RequestCancelledError,
                         RequestCleanedUp):
            self.action_queue.event_queue.push('AQ_DOWNLOAD_CANCELLED',
                                               share_id=self.share_id,
                                               node_id=self.node_id,
                                               server_hash=self.server_hash)
        elif failure.check(protocol_errors.DoesNotExistError):
            self.action_queue.event_queue.push('AQ_DOWNLOAD_DOES_NOT_EXIST',
                                               share_id=self.share_id,
                                               node_id=self.node_id)
        else:
            self.action_queue.event_queue.push('AQ_DOWNLOAD_ERROR',
                                               error=failure.getErrorMessage(),
                                               share_id=self.share_id,
                                               node_id=self.node_id,
                                               server_hash=self.server_hash)

    def reset_fileobj(self):
        """Rewind and empty the file.

        Usefult for get it ready to try again if necessary.
        """
        self.log.debug('reset fileobj')
        if self.fileobj is not None:
            self.fileobj.seek(0, 0)
            self.fileobj.truncate(0)

    def cb(self, bytes):
        """A streaming decompressor."""
        dloading = self.action_queue.downloading[self.share_id,
                                                 self.node_id]
        dloading['n_bytes_read'] += len(bytes)
        self.fileobj.write(self.gunzip.decompress(bytes))
        self.fileobj.flush()     # not strictly necessary but nice to
                                 # see the downloaded size
        self.progress_hook(dloading['n_bytes_read'])

    def progress_start(self, n_bytes_read_already):
        """Start tracking progress.

        Consider that n_bytes_read_already have been already read.
        """
        self.n_bytes_read_last = n_bytes_read_already

    def progress_hook(self, n_bytes_read):
        """Convert downloading progress into an event."""
        n_bytes_read_last = self.n_bytes_read_last
        self.n_bytes_read_last = n_bytes_read
        # produce an event only if there has been a threshold-sized progress
        if n_bytes_read - n_bytes_read_last < TRANSFER_PROGRESS_THRESHOLD:
            return
        self.action_queue.event_queue.push('AQ_DOWNLOAD_FILE_PROGRESS',
                                      share_id=self.share_id,
                                      node_id=self.node_id,
                                      n_bytes_read=n_bytes_read,
                                      deflated_size=self.deflated_size)

    def nacb(self, **kwargs):
        """Set the node attrs in the 'currently downloading' dict."""
        self.deflated_size = kwargs['deflated_size']
        self.action_queue.downloading[self.share_id,
                                      self.node_id].update(kwargs)

    def sync(self):
        """Flush the buffers and sync them to disk if possible."""
        remains = self.gunzip.flush()
        if remains:
            self.fileobj.write(remains)
        self.fileobj.flush()
        if getattr(self.fileobj, 'fileno', None) is not None:
            # it's a real file, with a fileno! Let's sync its data
            # out to disk
            os.fsync(self.fileobj.fileno())
        self.fileobj.close()


class Upload(ActionQueueCommand):
    """Upload stuff to a file."""

    __slots__ = ('share_id', 'node_id', 'previous_hash', 'hash', 'crc32',
                 'size', 'fileobj_factory', 'tempfile_factory',
                 'deflated_size', 'tempfile', 'cancelled', 'upload_req',
                 'marker_maybe', 'n_bytes_written_last')

    logged_attrs = ('share_id', 'node_id', 'previous_hash', 'hash', 'crc32',
                    'size', 'fileobj_factory')
    retryable_errors = ActionQueueCommand.retryable_errors + (
                                        protocol_errors.UploadInProgressError,)
    possible_markers = 'node_id',

    def __init__(self, request_queue, share_id, node_id, previous_hash, hash,
                 crc32, size, fileobj_factory, tempfile_factory):
        super(Upload, self).__init__(request_queue)
        self.share_id = share_id
        self.node_id = node_id
        self.previous_hash = previous_hash
        self.hash = hash
        self.crc32 = crc32
        self.size = size
        self.fileobj_factory = fileobj_factory
        self.tempfile_factory = tempfile_factory
        self.deflated_size = None
        self.tempfile = None
        self.cancelled = False
        self.upload_req = None
        self.marker_maybe = None
        if (self.share_id, self.node_id) in self.action_queue.uploading:
            self.action_queue.cancel_upload(self.share_id, self.node_id)
        self.n_bytes_written_last = None # set by _run

    @property
    def is_runnable(self):
        """Tell if the upload is ok to be carried on.

        Return True if there is sufficient space available to complete
        the upload, or if the upload is cancelled so it can pursue
        its fate.
        """
        if self.cancelled:
            return True
        else:
            return self.action_queue.have_sufficient_space_for_upload(
                                                    self.share_id, self.size)

    def cancel(self):
        """Cancel the upload."""
        self.cancelled = True
        if self.upload_req is not None:
            self.upload_req.cancel()
        self._queue.remove(self)
        self.cleanup()

    def cleanup(self):
        """Cleanup: stop the producer."""
        super(Upload, self).cleanup()
        if self.upload_req is not None and self.upload_req.producer is not None:
            self.log.debug('stopping the producer')
            self.upload_req.producer.stopProducing()

    def _start(self):
        """Do the specialized pre-run setup."""
        d = defer.Deferred()

        uploading = {"hash": self.hash, "req": self}
        self.action_queue.uploading[self.share_id, self.node_id] = uploading

        d = self.action_queue.zip_queue.zip(self)
        d.addBoth(lambda x: defer.fail(RequestCleanedUp('CANCELLED'))
                  if self.cancelled else x)
        return d

    def _run(self):
        """Do the actual running."""
        if self.cancelled:
            return defer.fail(RequestCleanedUp('CANCELLED'))
        uploading = {"hash": self.hash, "deflated_size": self.deflated_size,
                     "req": self}
        self.action_queue.uploading[self.share_id, self.node_id] = uploading

        self.action_queue.event_queue.push('AQ_UPLOAD_STARTED',
                                           share_id=self.share_id,
                                           node_id=self.node_id,
                                           hash=self.hash)

        if getattr(self.tempfile, 'name', None) is not None:
            self.tempfile = open(self.tempfile.name)
        self.n_bytes_written_last = 0
        f = UploadProgressWrapper(self.tempfile, uploading, self.progress_hook)
        req = self.action_queue.client.put_content_request(
            self.share_id, self.node_id, self.previous_hash, self.hash,
            self.crc32, self.size, self.deflated_size, f)
        self.upload_req = req
        d = req.deferred
        d.addBoth(lambda x: defer.fail(RequestCleanedUp('CANCELLED'))
                  if self.cancelled else x)
        d.addBoth(passit(lambda _:
                             self.action_queue.uploading.pop((self.share_id,
                                                              self.node_id))))
        d.addBoth(passit(lambda _: self.tempfile.close()))
        return d

    def progress_hook(self, n_bytes_written):
        """Convert uploading progress into an event."""
        n_bytes_written_last = self.n_bytes_written_last
        self.n_bytes_written_last = n_bytes_written
        # produce an event only if there has been a threshold-sized progress
        if n_bytes_written - n_bytes_written_last < TRANSFER_PROGRESS_THRESHOLD:
            return
        self.action_queue.event_queue.push('AQ_UPLOAD_FILE_PROGRESS',
                                      share_id=self.share_id,
                                      node_id=self.node_id,
                                      n_bytes_written=n_bytes_written,
                                      deflated_size=self.deflated_size)

    def handle_success(self, request):
        """It worked! Push the event."""
        # remove the temporary file if have one
        if getattr(self.tempfile, 'name', None) is not None:
            os.unlink(self.tempfile.name)

        # send the event
        d = dict(share_id=self.share_id, node_id=self.node_id, hash=self.hash,
                 new_generation=request.new_generation)
        self.action_queue.event_queue.push('AQ_UPLOAD_FINISHED', **d)

    def handle_failure(self, failure):
        """It didn't work! Push the event."""
        if getattr(self.tempfile, 'name', None) is not None:
            os.unlink(self.tempfile.name)

        if failure.check(protocol_errors.QuotaExceededError):
            error = failure.value
            self.action_queue.event_queue.push('SYS_QUOTA_EXCEEDED',
                                               volume_id=str(error.share_id),
                                               free_bytes=error.free_bytes)

        self.action_queue.event_queue.push('AQ_UPLOAD_ERROR',
                                           error=failure.getErrorMessage(),
                                           share_id=self.share_id,
                                           node_id=self.node_id,
                                           hash=self.hash)
