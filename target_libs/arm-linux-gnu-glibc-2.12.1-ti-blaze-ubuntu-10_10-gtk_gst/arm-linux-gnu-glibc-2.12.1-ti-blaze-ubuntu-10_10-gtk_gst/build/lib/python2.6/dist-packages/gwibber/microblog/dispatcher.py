#!/usr/bin/env python
# -*- coding: utf-8 -*-

import multiprocessing, threading, traceback, json
import gobject, dbus, dbus.service
import twitter, identica, statusnet, flickr, facebook
import qaiku, friendfeed, digg, buzz, pingfm
import sqlite3, mx.DateTime, re, uuid
import urlshorter, storage, network, util, config

from util import log
from util import resources
from util import exceptions
from util.const import *

# Try to import * from custom, install custom.py to include packaging 
# customizations like distro API keys, etc
try:
  from util.custom import *
except:
  pass

try:
  import indicate
except:
  indicate = None

gobject.threads_init()

log.logger.name = "Gwibber Dispatcher"

PROTOCOLS = {
  "twitter": twitter,
  "identica": identica,
  "flickr": flickr,
  "facebook": facebook,
  "friendfeed": friendfeed,
  "statusnet": statusnet,
  "twitter": twitter,
  "digg": digg,
  "qaiku": qaiku,
  "buzz": buzz,
  "pingfm": pingfm,
}

FEATURES = json.loads(GWIBBER_OPERATIONS)
SERVICES = dict([(k, v.PROTOCOL_INFO) for k, v in PROTOCOLS.items()])
SETTINGS = config.Preferences()

def perform_operation((account, opname, args, transient)):
  try:
    stream = FEATURES[opname]["stream"] or opname
    logtext = "<%s:%s>" % (account["service"], opname)

    """ Dead code from merge. Do we still need this?
    if not 'failed_id' in globals():
      global failed_id
      failed_id = []

    for key, val in account.items():
      if isinstance(val, str) and val.startswith(":KEYRING:"):
        value = util.keyring.get_account_password(account["_id"])

        if value is None:
          if account["_id"] not in failed_id:
            log.logger.debug("Adding %s to failed_id global", account["_id"])
            failed_id.append(account["_id"])
            log.logger.debug("Raising error to resolve failure for %s", account["_id"])
            raise exceptions.GwibberServiceError("keyring")
          return ("Failure", 0)

        account[key] = value
    """

    logtext = "<%s:%s>" % (account["service"], opname)
    log.logger.debug("%s Performing operation", logtext)

    args = dict((str(k), v) for k, v in args.items())
    message_data = PROTOCOLS[account["service"]].Client(account)(opname, **args)
    text_cleaner = re.compile(u"[: \n\t\r♻♺]+|@[^ ]+|![^ ]+|#[^ ]+") # signs, @nickname, !group, #tag
    new_messages = []

    if message_data is not None:
      for m in message_data:
        if isinstance(m, dict) and m.has_key("mid"):
          m["id"] = uuid.uuid1().hex
          m["operation"] = opname
          m["stream"] = stream
          m["transient"] = transient
          m["rtl"] = util.isRTL(re.sub(text_cleaner, "", m["text"].decode('utf-8')))

          log.logger.debug("%s Adding record", logtext)

          new_messages.insert(0, (
            m["id"],
            m["mid"],
            m["account"],
            account["service"],
            opname,
            transient,
            stream,
            m["time"],
            m["text"],
            m.get("sender", {}).get("is_me", None), 
            m.get("to_me", None),
            m.get("sender", {}).get("nick", None),
            m.get("reply", {}).get("nick", None),
            json.dumps(m)
          ))

    log.logger.debug("%s Finished operation", logtext)
    return ("Success", new_messages)
  except Exception as e:
    if not "logtext" in locals(): logtext = "<UNKNOWN>"
    log.logger.error("%s Operation failed", logtext)
    log.logger.debug("Traceback:\n%s", traceback.format_exc())
    return ("Failure", traceback.format_exc())

class OperationCollector:
  def __init__(self, dispatcher):
    self.dispatcher = dispatcher

  def get_passwords(self, acct):
    for key, val in acct.items():
      if hasattr(val, "startswith") and val.startswith(":KEYRING:"):
        id = "%s/%s" % (acct["id"], key)
        try:
          acct[key] = self.dispatcher.accounts.passwords[id]
        except:
          pass
    return acct 

  def get_accounts(self):
    data = json.loads(self.dispatcher.accounts.List())
    return [self.get_passwords(acct) for acct in data]

  def get_account(self, id):
    data = json.loads(self.dispatcher.accounts.Get(id))
    return self.get_passwords(data)

  def handle_max_id(self, acct, opname, id=None):
    if not id: id = acct["id"]

    features = SERVICES[acct["service"]]["features"]

    if "sincetime" in features: select = "time"
    elif "sinceid" in features: select = "cast(mid as integer)"
    else: return {}
    
    query = """
            SELECT max(%s) FROM messages
            WHERE (account = '%s' or transient = '%s') AND operation = '%s'
            """ % (select, id, id, opname)

    with self.dispatcher.messages.db:
      result = self.dispatcher.messages.db.execute(query).fetchall()[0][0]
      if result: return {"since": result}

    return {}

  def validate_operation(self, acct, opname, enabled="receive_enabled"):
    service = SERVICES[acct["service"]]
    return acct["service"] in PROTOCOLS and \
           opname in service["features"] and \
           opname in FEATURES and acct[enabled]

  def stream_to_operation(self, stream):
    try:
      account = self.get_account(stream["account"])
    except:
      self.dispatcher.streams.Delete(stream["id"])
      return None
    args = stream["parameters"]
    opname = stream["operation"]
    if self.validate_operation(account, opname):
      args.update(self.handle_max_id(account, opname, stream["id"]))
      return (account, stream["operation"], args, stream["id"])

  def search_to_operations(self, search):
    for account in self.get_accounts():
      args = {"query": search["query"]}
      if self.validate_operation(account, "search"):
        args.update(self.handle_max_id(account, "search", search["id"]))
        yield (account, "search", args, search["id"])

  def account_to_operations(self, acct):
    if isinstance(acct, basestring):
      acct = self.get_account(acct)
    
    if SERVICES.has_key(acct["service"]):
      for opname in SERVICES[acct["service"]]["default_streams"]:
        if self.validate_operation(acct, opname):
          args = self.handle_max_id(acct, opname)
          yield (acct, opname, args, False)

  def get_send_operations(self, message):
    for account in self.get_accounts():
      if self.validate_operation(account, "send", "send_enabled"):
        yield (account, "send", {"message": message}, False)

  def get_operation_by_id(self, id):
    stream = self.dispatcher.streams.Get(id)
    if stream: return [self.stream_to_operation(json.loads(stream))]
    
    search = self.dispatcher.searches.Get(id)
    if search: return list(self.search_to_operations(json.loads(search)))

  def get_operations(self):
    for acct in self.get_accounts():
      for o in self.account_to_operations(acct):
        yield o

    for stream in json.loads(self.dispatcher.streams.List()):
      # TODO: Make sure account for stream exists
      o = self.stream_to_operation(stream)
      if o: yield o

    for search in json.loads(self.dispatcher.searches.List()):
      for o in self.search_to_operations(search):
        yield o

class MapAsync(threading.Thread):
  def __init__(self, func, iterable, cbsuccess, cbfailure, pool):
    threading.Thread.__init__(self)
    self.iterable = iterable
    self.callback = cbsuccess
    self.failure = cbfailure
    self.daemon = True
    self.func = func
    self.pool = pool
    self.start()

  def run(self):
    try:
      self.pool.map_async(self.func, self.iterable, callback = self.callback)
    except Exception as e:
      self.failure(e, traceback.format_exc())

class Dispatcher(dbus.service.Object):
  """
  The Gwibber Dispatcher handles all the backend operations.
  """
  __dbus_object_path__ = "/com/gwibber/Service"

  def __init__(self, loop, autorefresh=True):
    self.bus = dbus.SessionBus()
    bus_name = dbus.service.BusName("com.Gwibber.Service", bus=self.bus)
    dbus.service.Object.__init__(self, bus_name, self.__dbus_object_path__)

    self.db = sqlite3.connect(SQLITE_DB_FILENAME)

    self.accounts = storage.AccountManager(self.db)
    self.searches = storage.SearchManager(self.db)
    self.streams = storage.StreamManager(self.db)
    self.messages = storage.MessageManager(self.db)
    self.collector = OperationCollector(self)

    if indicate and util.resources.get_desktop_file():
      self.indicate = indicate.indicate_server_ref_default()
      self.indicate.set_type("message.gwibber")
      self.indicate.set_desktop_file(util.resources.get_desktop_file())
      self.indicate.connect("server-display", self.on_indicator_activate)
      self.indicate.connect("interest-added", self.on_indicator_interest_added)
      self.indicate.connect("interest-removed", self.on_indicator_interest_removed)
      self.indicate.show()
    self.indicator_items = {}
    self.notified_items = []

    self.refresh_count = 0
    self.mainloop = loop
    self.workerpool = multiprocessing.Pool()

    self.refresh_timer_id = None

    if autorefresh:
      self.refresh()

    self.accounts_service = util.getbus("Accounts")
    self.accounts_service.connect_to_signal("Updated", self.on_account_updated)
    self.accounts_service.connect_to_signal("Deleted", self.on_account_deleted)
    self.accounts_service.connect_to_signal("Created", self.on_account_created)

  def on_account_updated(self, account):
    pass

  def on_account_created(self, account):
    self.refresh()

  def on_account_deleted(self, account):
    # Delete streams associated with the user that was deleted
    try:
      acct = json.loads(account)
      for stream in json.loads(self.streams.List()):
        if stream["account"] == acct["id"]:
          self.streams.Delete(stream["id"])
    except:
      pass

  @dbus.service.signal("com.Gwibber.Service")
  def LoadingComplete(self): pass

  @dbus.service.signal("com.Gwibber.Service")
  def LoadingStarted(self): pass

  @dbus.service.method("com.Gwibber.Service")
  def Refresh(self):
    """
    Calls the Gwibber Service to trigger a refresh operation
    example:
            import dbus
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            service.Refresh()

    """
    self.refresh()

  @dbus.service.method("com.Gwibber.Service", in_signature="s")
  def PerformOp(self, opdata):
    try: o = json.loads(opdata)
    except: return
    
    log.logger.debug("** Starting Single Operation **")
    self.LoadingStarted()
    
    params = ["account", "operation", "args", "transient"]
    operation = None
    
    if "account" in o and self.collector.get_account(o["account"]):
      account = self.collector.get_account(o["account"])
    
    if "id" in o:
      operation = self.collector.get_operation_by_id(o["id"])
    elif "operation" in o and self.collector.validate_operation(account, o["operation"]):
        operation = util.compact([(account, o["operation"], o["args"], None)])

    if operation:
      self.perform_async_operation(operation)

  @dbus.service.method("com.Gwibber.Service", in_signature="s")
  def SendMessage(self, message):
    """
    Posts a message/status update to all accounts with send_enabled = True.  It 
    takes one argument, which is a message formated as a string.
    example:
            import dbus
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            service.SendMessage("Your message")
    """
    self.send(list(self.collector.get_send_operations(message)))

  @dbus.service.method("com.Gwibber.Service", in_signature="s")
  def Send(self, opdata):
    try:
      o = json.loads(opdata)
      if "target" in o:
        args = {"message": o["message"], "target": o["target"]}
        operations = [(self.collector.get_account(o["target"]["account"]), "send_thread", args, None)]
      elif "private" in o:
        args = {"message": o["message"], "private": o["private"]}
        operations = [(self.collector.get_account(o["private"]["account"]), "send_private", args, None)]
      elif "accounts" in o:
        operations = [(self.collector.get_account(a), "send", {"message": o["message"]}, None) for a in o["accounts"]]
      self.send(operations)
    except:
      log.logger.error("Sending failed:\n%s", traceback.format_exc())

  @dbus.service.method("com.Gwibber.Service", out_signature="s")
  def GetServices(self):
    """
    Returns a list of services available as json string
    example:
            import dbus, json
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            services = json.loads(service.GetServices())

    """
    return json.dumps(SERVICES)

  @dbus.service.method("com.Gwibber.Service", out_signature="s")
  def GetFeatures(self):
    """
    Returns a list of features as json string
    example:
            import dbus, json
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            features = json.loads(service.GetFeatures())
    """
    return json.dumps(FEATURES)

  @dbus.service.method("com.Gwibber.Service", out_signature="s")
  def GetVersion(self): 
    """
    Returns a the gwibber-service version as a string
    example:
            import dbus
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            version = service.GetVersion()
    """
    return VERSION_NUMBER

  @dbus.service.method("com.Gwibber.Service")
  def Start(self):
    """
    Start the service
    example:
            import dbus
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            service.Start()
    """
    log.logger.info("Gwibber Service is starting")

  @dbus.service.method("com.Gwibber.Service")
  def Quit(self): 
    """
    Shutdown the service
    example:
            import dbus
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            service.Quit()
    """
    log.logger.info("Gwibber Service is being shutdown")
    self.mainloop.quit()

  @dbus.service.method("com.Gwibber.Service", out_signature="b")
  def IndicatorInterestCheck(self):
    """
    Check for interest from the messaging menu indicator
    Returns a boolean
    example:
            import dbus
            obj = dbus.SessionBus().get_object("com.Gwibber.Service", "/com/gwibber/Service")
            service = dbus.Interface(obj, "com.Gwibber.Service")
            res = service.IndicatorInterestCheck()
    """
    if indicate:
      return self.indicate.check_interest(indicate.INTEREST_SERVER_DISPLAY)
    else:
      return False

  @dbus.service.signal("com.Gwibber.Service")
  def IndicatorInterestAdded(self): pass
  
  @dbus.service.signal("com.Gwibber.Service")
  def IndicatorInterestRemoved(self): pass

  def perform_async_operation(self, iterable):
    t = MapAsync(perform_operation, iterable, self.loading_complete, self.loading_failed, self.workerpool)
    t.join()
  
  def loading_complete(self, output):
    self.refresh_count += 1
    
    items = []
    for o in output:
      for o2 in o[1]:
        if len(o2) > 1:
          items.append(o2)

    with sqlite3.connect(SQLITE_DB_FILENAME) as db:
      oldid = db.execute("select max(ROWID) from messages").fetchone()[0] or 0
      
      output = db.executemany("INSERT OR REPLACE INTO messages (%s) VALUES (%s)" % (
            ",".join(self.messages.columns),
            ",".join("?" * len(self.messages.columns))), items)

      new_items = db.execute("""
        select * from (select * from messages where operation == "receive" and ROWID > %s and to_me = 0 ORDER BY time DESC LIMIT 10) as a union
        select * from (select * from messages where operation IN ("receive", "private") and ROWID > %s and to_me != 0 ORDER BY time DESC LIMIT 10) as b 
        ORDER BY time ASC""" % (oldid, oldid)).fetchall()


      for i in new_items:
          self.new_message(i)
    
    self.LoadingComplete()
    log.logger.info("Loading complete: %s - %s", self.refresh_count, [o[0] for o in output])

  def on_indicator_interest_added(self, server, interest):
    self.IndicatorInterestAdded()

  def on_indicator_interest_removed(self, server, interest):
    self.IndicatorInterestRemoved()

  def on_indicator_activate(self, indicator, timestamp):
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    client_bus = dbus.SessionBus()
    log.logger.debug("Raising gwibber client")
    try:
      client_obj = client_bus.get_object("com.GwibberClient",
        "/com/GwibberClient", follow_name_owner_changes = True,
        introspect = False)
      gw = dbus.Interface(client_obj, "com.GwibberClient")
      gw.focus_client(reply_handler=self.handle_focus_reply,
                      error_handler=self.handle_focus_error)
    except dbus.DBusException:
      log.logger.error("Indicator activate failed:\n%s", traceback.format_exc())


  def on_indicator_reply_activate(self, indicator, timestamp):
    log.logger.debug("Raising gwibber client, focusing replies stream")
    client_bus = dbus.SessionBus()
    try:
      client_obj = client_bus.get_object("com.GwibberClient", "/com/GwibberClient")
      gw = dbus.Interface(client_obj, "com.GwibberClient")
      gw.show_replies(reply_handler=self.handle_focus_reply,
                      error_handler=self.handle_focus_error)
      for i in self.indicator_items.values():
        try:
          i.hide()
        except:
          pass
    except dbus.DBusException:
      log.logger.error("Indicator reply activate failed:\n%s", traceback.format_exc())

  def handle_focus_reply(self, *args):
    log.logger.debug("Gwibber Client raised")

  def handle_focus_error(self, *args):
    log.logger.error("Failed to raise client %s", args)

  def new_message(self, data):
    message = json.loads(data[-1])
    if message["transient"]:
      log.logger.debug("Message %s is transient, not notifying", message["id"])
      return 

    if indicate and message["to_me"]:
      if not self.indicator_items.has_key(str(message["id"])):
        gobject.idle_add(self.handle_indicator_item, message)

    if util.can_notify and  str(message["mid"]) not in self.notified_items:
      self.notified_items.append(message["mid"])
      if SETTINGS["notify_mentions_only"] and message["to_me"]: 
        log.logger.debug("%s is a mention and notify_mentions_only is true", message["mid"])
        gobject.idle_add(self.handle_notify_item, message)
      elif SETTINGS["show_notifications"] and not SETTINGS["notify_mentions_only"]:
        log.logger.debug("%s - show_notifications is true and notify_mentions_only is false", message["mid"])
        gobject.idle_add(self.handle_notify_item, message)

  def handle_notify_item(self, message):
    if SETTINGS["show_fullname"]:
      sender_name = message["sender"].get("name", message["sender"].get("nick", ""))
    else:
      sender_name = message["sender"].get("nick", message["sender"].get("name", ""))

    #image = util.resources.get_ui_asset("icons/breakdance/scalable/%s.svg" % message["service"])
    image = util.resources.get_avatar_path(message["sender"]["image"])
    if not image:
      image = util.resources.get_ui_asset("icons/breakdance/scalable/%s.svg" % message["service"])
    util.notify(sender_name, message["text"], image, 2000)

    return False

  def handle_indicator_item(self, message):
    if not self.indicator_items.has_key(str(message["mid"])):
      indicator = indicate.Indicator() if hasattr(indicate, "Indicator") else indicate.IndicatorMessage()
      indicator.connect("user-display", self.on_indicator_reply_activate)
      indicator.set_property("subtype", "im.gwibber")
      indicator.set_property("sender", message["sender"].get("name", ""))
      indicator.set_property("body", message["text"])
      indicator.set_property_time("time",
          mx.DateTime.DateTimeFromTicks(message["time"]).localtime().ticks())
      indicator.show()
      self.indicator_items[message["mid"]] = indicator
      log.logger.debug("Message from %s added to indicator", message["sender"].get("name", ""))
    return False

  def loading_failed(self, exception, tb):
    self.LoadingComplete()
    log.logger.error("Loading failed: %s - %s", exception, tb)

  def send(self, operations):
    operations = util.compact(operations)
    if operations:
      self.LoadingStarted()
      log.logger.debug("*** Sending Message ***")
      self.perform_async_operation(operations)

  def refresh(self):
    if self.refresh_timer_id:
      gobject.source_remove(self.refresh_timer_id)
    log.logger.debug("Refresh interval is set to %s", SETTINGS["interval"])
    operations = []
    
    for o in self.collector.get_operations():
      interval = FEATURES[o[1]].get("interval", 1)
      if self.refresh_count % interval == 0:
        operations.append(o)
    
    if operations:
      log.logger.debug("** Starting Refresh - %s **", mx.DateTime.now())
      self.LoadingStarted()
      self.perform_async_operation(operations)

    self.refresh_timer_id = gobject.timeout_add_seconds(int(60 * SETTINGS["interval"]), self.refresh)
    return False

class ConnectionMonitor(dbus.service.Object):
  __dbus_object_path__ = "/com/gwibber/Connection"

  def __init__(self):
    self.bus = dbus.SessionBus()
    bus_name = dbus.service.BusName("com.Gwibber.Connection", bus=self.bus)
    dbus.service.Object.__init__(self, bus_name, self.__dbus_object_path__)

    self.sysbus = dbus.SystemBus()
    try:
      self.nm = self.sysbus.get_object(NM_DBUS_SERVICE, NM_DBUS_OBJECT_PATH)
      self.nm.connect_to_signal("StateChanged", self.on_connection_changed)
    except:
      pass

  def on_connection_changed(self, state):
    if state == NM_STATE_CONNECTED:
      log.logger.info("Network state changed to Online")
      self.ConnectionOnline()
    if state == NM_STATE_DISCONNECTED:
      log.logger.info("Network state changed to Offline")
      self.ConnectionOffline()

  @dbus.service.signal("com.Gwibber.Connection")
  def ConnectionOnline(self): pass

  @dbus.service.signal("com.Gwibber.Connection")
  def ConnectionOffline(self): pass

  @dbus.service.method("com.Gwibber.Connection")
  def isConnected(self):
    try:
      if self.nm.state() == NM_STATE_CONNECTED:
        return True
      return False
    except:
      return True

class URLShorten(dbus.service.Object):
  __dbus_object_path__ = "/com/gwibber/URLShorten"

  def __init__(self):
    self.bus = dbus.SessionBus()
    bus_name = dbus.service.BusName("com.Gwibber.URLShorten", bus=self.bus)
    dbus.service.Object.__init__(self, bus_name, self.__dbus_object_path__)

  @dbus.service.method("com.Gwibber.URLShorten", in_signature="s", out_signature="s")
  def Shorten(self, url):
    """
    Takes a url as a string and returns a shortened url as a string.
    example:
            import dbus
            url = "http://www.example.com/this/is/a/long/url"
            obj = dbus.SessionBus().get_object("com.Gwibber.URLShorten", "/com/gwibber/URLShorten")
            shortener = dbus.Interface(obj, "com.Gwibber.URLShorten")
            short_url = shortener.Shorten(url)
    """
    
    service = SETTINGS["urlshorter"] or "is.gd"
    log.logger.info("Shortening URL %s with %s", url, service)
    if self.IsShort(url): return url
    try:
      s = urlshorter.PROTOCOLS[service].URLShorter()
      return s.short(url)
    except: return url

  def IsShort(self, url):
    for us in urlshorter.PROTOCOLS.values():
      if url.startswith(us.PROTOCOL_INFO["fqdn"]):
        return True
    return False

class Translate(dbus.service.Object):
  __dbus_object_path__ = "/com/gwibber/Translate"

  def __init__(self):
    self.bus = dbus.SessionBus()
    bus_name = dbus.service.BusName("com.Gwibber.Translate", bus=self.bus)
    dbus.service.Object.__init__(self, bus_name, self.__dbus_object_path__)

  @dbus.service.method("com.Gwibber.Translate", in_signature="sss", out_signature="s")
  def Translate(self, text, srclang, dstlang):
    url = "http://ajax.googleapis.com/ajax/services/language/translate"
    params = {"v": "1.0", "q": text, "langpair": "|".join((srclang, dstlang))}
    data = network.Download(url, params).get_json()
    
    if data["responseStatus"] == 200:
      return data.get("responseData", {}).get("translatedText", "")
    else: return ""

