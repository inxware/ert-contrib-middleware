# Copyright 2009 Canonical Ltd.
#
# This file is part of desktopcouch.
#
# desktopcouch is free software: you can redistribute it and/or modify
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
# Authors: Chad Miller <chad.miller@canonical.com>

import logging
log = logging.getLogger("replication")

import dbus.exceptions

from desktopcouch.pair.couchdb_pairing import couchdb_io
from desktopcouch.pair.couchdb_pairing import dbus_io
from desktopcouch import replication_services

try:
    import urlparse
except ImportError:
    import urllib.parse as urlparse

from twisted.internet import task, reactor


known_bad_service_names = set()
is_running = True


def db_targetprefix_for_service(service_name):
    """Use the service name to look up what the prefix should be on the
    databases.  This gives an egalitarian way for non-UbuntuOne servers to have
    their own remote-db-name scheme."""
    try:
        container = "desktopcouch.replication_services"
        log.debug("Looking up prefix for service %r", service_name)
        mod = __import__(container, fromlist=[service_name])
        return getattr(mod, service_name).db_name_prefix
    except ImportError, e:
        log.error("The service %r is unknown.  It is not a "
                "module in the %s package ." % (service_name, container))
        return ""
    except Exception, e:
        log.exception("Not changing remote db name.")
        return ""

def oauth_info_for_service(service_name):
    """Use the service name to look up what oauth information we should use
    when talking to that service."""
    try:
        container = "desktopcouch.replication_services"
        log.debug("Looking up prefix for service %r", service_name)
        mod = __import__(container, fromlist=[service_name])
        return getattr(mod, service_name).get_oauth_data()
    except ImportError, e:
        log.error("The service %r is unknown.  It is not a "
                "module in the %s package ." % (service_name, container))
        return None

def do_all_replication(local_port):
    log.debug("started replicating")
    local_uri = couchdb_io.mkuri("localhost", local_port)
    try:
        try:
            # All machines running desktopcouch must advertise themselves with
            # zeroconf.  We collect those elsewhere and filter out the ones
            # that we have paired with.  Now, it's time to send our changes to
            # all those.

            for remote_hostid, addr, port, is_unpaired, remote_oauth in \
                    dbus_io.get_seen_paired_hosts(uri=local_uri):

                if is_unpaired:
                    # The far end doesn't know want to break up.
                    count = 0
                    for local_identifier in couchdb_io.get_my_host_unique_id():
                        last_exception = None
                        try:
                            # Tell her gently, using each pseudonym.
                            couchdb_io.expunge_pairing(local_identifier,
                                    couchdb_io.mkuri(addr, port), remote_oauth)
                            count += 1
                        except Exception, e:
                            last_exception = e
                    if count == 0:
                        if last_exception is not None:
                            # If she didn't recognize us, something's wrong.
                            try:
                                raise last_exception
                                # push caught exception back...
                            except:
                                # ... so that we log it here.
                                log.exception(
                                        "failed to unpair from other end.")
                                continue
                    else:
                        # Finally, find your inner peace...
                        couchdb_io.expunge_pairing(remote_hostid)
                    # ...and move on.
                    continue

                # Ah, good, this is an active relationship.  Be a giver.
                log.debug("want to replipush to discovered host %r @ %s",
                        remote_hostid, addr)
                for db_name in couchdb_io.get_database_names_replicatable(local_uri):
                    if not is_running: return
                    couchdb_io.replicate(db_name, db_name,
                            target_host=addr, target_port=port,
                            source_port=local_port, target_oauth=remote_oauth,
                            local_uri=local_uri)
            log.debug("replication of discovered hosts finished")
        except Exception, e:
            log.exception("replication of discovered hosts aborted")
            pass

        try:
            # There may be services we send data to.  Use the service name (sn)
            # to look up what the service needs from us.

            for remote_hostid, sn, to_pull, to_push in \
                        couchdb_io.get_static_paired_hosts(port=local_port):

                if not sn in dir(replication_services):
                    if not is_running:
                        return
                    if sn in known_bad_service_names:
                        continue  # Don't nag.
                    known_bad_service_names.add(sn)

                remote_oauth_data = oauth_info_for_service(sn)

                # TODO: push all this into service module.
                try:
                    prefix_getter = db_targetprefix_for_service(sn)
                    remote_location = str(prefix_getter)
                    if hasattr(prefix_getter, 'user_id'):
                        user_id = prefix_getter.user_id
                    else:
                        user_id = None
                    urlinfo = urlparse.urlsplit(str(remote_location))
                except ValueError, e:
                    log.warn("Can't reach service %s.  %s", sn, e)
                    continue
                if ":" in urlinfo.netloc:
                    addr, port = urlinfo.netloc.rsplit(":", 1)
                else:
                    addr = urlinfo.netloc
                    port = 443 if urlinfo.scheme == "https" else 80
                remote_db_name_prefix = urlinfo.path.strip("/")
                # ^
                if to_push:
                    for db_name in couchdb_io.get_database_names_replicatable(local_uri):
                        if not is_running: return

                        remote_db_name = remote_db_name_prefix + "/" + db_name

                        log.debug("want to replipush %r to static host %r @ %s",
                                remote_db_name, remote_hostid, addr)

                        couchdb_io.replicate(db_name, remote_db_name,
                                target_host=addr, target_port=port,
                                source_port=local_port, target_ssl=True,
                                target_oauth=remote_oauth_data,
                                local_uri=local_uri)
                if to_pull:
                    for remote_db_name in \
                            couchdb_io.get_database_names_replicatable(
                    couchdb_io.mkuri(
                            addr, int(port), has_ssl=(urlinfo.scheme=='https')),
                            oauth_tokens=remote_oauth_data, service=True,
                            user_id=user_id):
                        if not is_running:
                            return
                        try:
                            if not remote_db_name.startswith(
                                    str(remote_db_name_prefix + "/")):
                                continue
                        except ValueError, e:
                            log.error("skipping %r on %s.  %s", db_name, sn, e)
                            continue
                        prefix_len = len(str(remote_db_name_prefix))
                        db_name = remote_db_name[prefix_len + 1:]
                        if db_name.strip("/") == "management":
                            continue  # be paranoid about what we accept.
                        log.debug(
                                "want to replipull %r from static host %r @ %s",
                                db_name, remote_hostid, addr)
                        couchdb_io.replicate(remote_db_name, db_name,
                                source_host=addr, source_port=port,
                                target_port=local_port, source_ssl=True,
                                source_oauth=remote_oauth_data,
                                local_uri=local_uri)

        except Exception, e:
            log.exception("replication of services aborted")
            pass
    finally:
        log.debug("finished replicating")


def set_up(port_getter):
    port = port_getter()
    unique_identifiers = couchdb_io.get_my_host_unique_id(
            couchdb_io.mkuri("localhost", int(port)), create=True)

    beacons = [dbus_io.LocationAdvertisement(port, "desktopcouch " + i)
            for i in unique_identifiers]
    for b in beacons:
        try:
            b.publish()
        except dbus.exceptions.DBusException, e:
            log.error("We seem to be running already, or can't publish "
                    "our zeroconf advert.  %s", e)
            return None

    dbus_io.maintain_discovered_servers()

    t = task.LoopingCall(do_all_replication, int(port))
    t.start(600)

    # TODO:  port may change, so every so often, check it and
    # perhaps refresh the beacons.  We return an array of beacons, so we could
    # keep a reference to that array and mutate it when the port-beacons
    # change.

    return beacons, t


def tear_down(beacons, looping_task):
    for b in beacons:
        b.unpublish()
    try:
        is_running = False
        looping_task.stop()
    except:
        pass
