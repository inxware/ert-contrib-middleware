Instructions to code your own VLC Lua scripts.
$Id$

1 - About Lua
=============

Lua documentation is available on http://www.lua.org . The reference manual
is very useful: http://www.lua.org/manual/5.1/ .
VLC uses Lua 5.1
All the Lua standard libraries are available.

2 - Lua in VLC
==============

Several types of VLC Lua scripts can currently be coded:
 * Playlist (see playlist/README.txt)
 * Art fetcher (see meta/README.txt)
 * Interface (see intf/README.txt)
 * Extensions (see extensions/README.txt)
 * Services Discovery (see sd/README.txt)

Lua scripts are tried in alphabetical order in the user's VLC config
directory lua/{playlist,meta,intf}/ subdirectory on Windows and Mac OS X or
in the user's local share directory (~/.local/share/vlc/lua/... on linux),
then in the global VLC lua/{playlist,meta,intf}/ directory.

3 - VLC specific Lua modules
============================

All VLC specifics modules are in the "vlc" object. For example, if you want
to use the "info" function of the "msg" VLC specific Lua module:
vlc.msg.info( "This is an info message and will be displayed in the console" )

Note: availability of the different VLC specific Lua modules depends on
the type of VLC Lua script your are in.

Access lists
------------
local a = vlc.acl(true) -> new ACL with default set to allow
a:check("10.0.0.1") -> 0 == allow, 1 == deny, -1 == error
a("10.0.0.1") -> same as a:check("10.0.0.1")
a:duplicate() -> duplicate ACL object
a:add_host("10.0.0.1",true) -> allow 10.0.0.1
a:add_net("10.0.0.0",24,true) -> allow 10.0.0.0/24 (not sure)
a:load_file("/path/to/acl") -> load ACL from file

Configuration
-------------
config.get( name ): Get the VLC configuration option "name"'s value.
config.set( name, value ): Set the VLC configuration option "name"'s value.

Dialog
------
local d = vlc.dialog( "My VLC Extension" ): Create a new UI dialog, with a human-readable title: "My VLC Extension"
d:show(): Show this dialog.
d:hide(): Hide (but not close) this dialog.
d:delete(): Close and delete this dialog.
d:del_widget( widget ): Delete 'widget'. It disappears from the dialog and repositioning may occur.
d:update(): Update the dialog immediately (don't wait for the current function to return)

In the following functions, you can always add some optional parameters: col, row, col_span, row_span, width, height.
They define the position of a widget in the dialog:
- row, col are the absolute positions on a grid of widgets. First row, col are 1.
- row_span, col_span represent the relative size of a widget on the grid. A widget with col_span = 4 will be displayed as wide as 4 widgets of col_span = 1.
- width and height are size hints (in pixels) but may be discarded by the GUI module
Example: w = d:add_label( "My Label", 2, 3, 4, 5 ) will create a label at row 3, col 2, with a relative width of 4, height of 5.

d:add_button( text, func, ... ): Create a button with caption "text" (string). When clicked, call function "func". func is a function reference.
d:add_label( text, ... ): Create a text label with caption "text" (string).
d:add_html( text, ... ): Create a rich text label with caption "text" (string), that supports basic HTML formatting (such as <i> or <h1> for instance).
d:add_text_input( text, ... ): Create an editable text field, in order to read user input.
d:add_password( text, ... ): Create an editable text field, in order to read user input. Text entered in this box will not be readable (replaced by asterisks).
d:add_check_box( text, ... ): Create a check box with a text. They have a boolean state (true/false).
d:add_dropdown( ... ): Create a drop-down widget. Only 1 element can be selected the same time.
d:add_list( ... ): Create a list widget. Allows multiple or empty selections.
d:add_image( path, ... ): Create an image label. path is a relative or absolute path to the image on the local computer.

Some functions can also be applied on widgets:
w:set_text( text ): Change text displayed by the widget. Applies to: button, label, html, text_input, password, check_box.
w:get_text(): Read text displayed by the widget. Returns a string. Applies to: button, label, html, text_input, password, check_box.
w:set_checked( bool ): Set check state of a check box. Applies to: check_box.
w:get_checked(): Read check state of a check box. Returns a boolean. Applies to: check_box.
w:add_value( text, id ): Add a value with identifier 'id' (integer) and text 'text'. It's always best to have unique identifiers. Applies to: drop_down.
w:get_value(): Return identifier of the selected item. Corresponds to the text value chosen by the user. Applies to: drop_down.
w:clear(): Clear a list or drop_down widget. After that, all values previously added are lost.
w:get_selection(): Retrieve a table representing the current selection. Keys are the ids, values are the texts associated. Applies to: list.


Extension
---------
deactivate(): Deactivate current extension (after the end of the current function).

HTTPd
-----
http( host, port, [cert, key, ca, crl]): create a new HTTP (SSL) daemon.

local h = vlc.httpd( "localhost", 8080 )
h:handler( url, user, password, acl, callback, data ) -- add a handler for given url. If user and password are non nil, they will be used to authenticate connecting clients. If acl is non nil, it will be used to restrict access. callback will be called to handle connections. The callback function takes 7 arguments: data, url, request, type, in, addr, host. It returns the reply as a string.
h:file( url, mime, user, password, acl, callback, data ) -- add a file for given url with given mime type. If user and password are non nil, they will be used to authenticate connecting clients. If acl is non nil, it will be used to restrict access. callback will be called to handle connections. The callback function takes 2 arguments: data and request. It returns the reply as a string.
h:redirect( url_dst, url_src ): Redirect all connections from url_src to url_dst.

Input
-----
input.is_playing(): Return true if input exists.
input.add_subtitle(url): Add a subtitle to the current input
input.item(): Get the current input item. Input item methods are:
  :uri(): Get item's URI.
  :name(): Get item's name.
  :duration(): Get item's duration in seconds or negative value if unavailable.
  :is_preparsed(): Return true if meta data has been preparsed
  :metas(): Get meta data as a table.
  :set_meta(key, value): Set meta data.
  :info(): Get the current input's info. Return value is a table of tables. Keys of the top level table are info category labels.
  :stats(): Get statistics about the input. This is a table with the following fields:
    .read_packets
    .read_bytes
    .input_bitrate
    .average_input_bitrate
    .demux_read_packets
    .demux_read_bytes
    .demux_bitrate
    .average_demux_bitrate
    .demux_corrupted
    .demux_discontinuity
    .decoded_audio
    .decoded_video
    .displayed_pictures
    .lost_pictures
    .sent_packets
    .sent_bytes
    .send_bitrate
    .played_abuffers
    .lost_abuffers

MD5
---
md5( str ): return the string's hash
md5(): create an md5 object with the following methods:
  :add( str ): add a string to the hash
  :end_(): finish hashing
  :hash(): return the hash

Messages
--------
msg.dbg( [str1, [str2, [...]]] ): Output debug messages (-vv).
msg.warn( [str1, [str2, [...]]] ): Output warning messages (-v).
msg.err( [str1, [str2, [...]]] ): Output error messages.
msg.info( [str1, [str2, [...]]] ): Output info messages.

Misc
----
misc.version(): Get the VLC version string.
misc.copyright(): Get the VLC copyright statement.
misc.license(): Get the VLC license.

misc.datadir(): Get the VLC data directory.
misc.userdatadir(): Get the user's VLC data directory.
misc.homedir(): Get the user's home directory.
misc.configdir(): Get the user's VLC config directory.
misc.cachedir(): Get the user's VLC cache directory.

misc.datadir_list( name ): FIXME: write description ... or ditch function
  if it isn't useful anymore, we have datadir and userdatadir :)

misc.mdate(): Get the current date (in milliseconds).
misc.mwait(): Wait for the given date (in milliseconds).

misc.lock_and_wait(): Lock our object thread and wait for a wake up signal.

misc.should_die(): Returns true if the interface should quit.
misc.quit(): Quit VLC.

Net
---
net.url_parse( url, [option delimiter] ): Parse URL. Returns a table with
  fields "protocol", "username", "password", "host", "port", path" and
  "option".
net.listen_tcp( host, port ): Listen to TCP connections. This returns an
  object with an accept and an fds method. accept is blocking (Poll on the
  fds with the net.POLLIN flag if you want to be non blocking).
  The fds method returns a list of fds you can call poll on before using
  the accept method. For example:
local l = vlc.net.listen_tcp( "localhost", 1234 )
while true do
  local fd = l:accept()
  if fd >= 0 do
    net.send( fd, "blabla" )
    net.close( fd )
  end
end
net.close( fd ): Close file descriptor.
net.send( fd, string, [length] ): Send data on fd.
net.recv( fd, [max length] ): Receive data from fd.
net.poll( { fd = events }, [timeout in seconds] ): Implement poll function.
  Returns the numbers of file descriptors with a non 0 revent. The function
  modifies the input table to { fd = revents }. See "man poll".
net.POLLIN/POLLPRI/POLLOUT/POLLRDHUP/POLLERR/POLLHUP/POLLNVAL: poll event flags
net.fd_read( fd, [max length] ): Read data from fd.
net.fd_write( fd, string, [length] ): Write data to fd.
net.stat( path ): Stat a file. Returns a table with the following fields:
    .type
    .mode
    .uid
    .gid
    .size
    .access_time
    .modification_time
    .creation_time
net.opendir( path ): List a directory's contents.

Objects
-------
object.input(): Get the current input object.
object.playlist(): Get the playlist object.
object.libvlc(): Get the libvlc object.

object.find( object, type, mode ): Find an object of given type. mode can
  be any of "parent", "child" and "anywhere". If set to "parent", it will
  look in "object"'s parent objects. If set to "child" it will look in
  "object"'s children. If set to "anywhere", it will look in all the
  objects. If object is unset, the current module's object will be used.
  Type can be: "libvlc", "playlist", "input", "decoder",
  "vout", "aout", "packetizer", "generic".
  This function is deprecated and slow and should be avoided.
object.find_name( object, name, mode ): Same as above except that it matches
  on the object's name and not type. This function is also slow and should
  be avoided if possible.

OSD
---
osd.icon( type, [id] ): Display an icon on the given OSD channel. Uses the
  default channel is none is given. Icon types are: "pause", "play",
  "speaker" and "mute".
osd.message( string, [id] ): Display text message on the given OSD channel.
osd.slider( position, type, [id] ): Display slider. Position is an integer
  from 0 to 100. Type can be "horizontal" or "vertical".
osd.channel_register(): Register a new OSD channel. Returns the channel id.
osd.channel_clear( id ): Clear OSD channel.
osd.menu.show(): Show the OSD menu.
osd.menu.hide(): Hide the OSD menu.
osd.menu.prev(): Select previous/left item.
osd.menu.next(): Select next/right item.
osd.menu.up(): Move selection up.
osd.menu.down(): Move selection down.
osd.menu.activate(): Activate/validate current selection.

Playlist
--------
playlist.prev(): Play previous track.
playlist.next(): Play next track.
playlist.skip( n ): Skip n tracks.
playlist.play(): Play.
playlist.pause(): Pause.
playlist.stop(): Stop.
playlist.clear(): Clear the playlist.
playlist.repeat_( [status] ): Toggle item repeat or set to specified value.
playlist.loop( [status] ): Toggle playlist loop or set to specified value.
playlist.random( [status] ): Toggle playlist random or set to specified value.
playlist.goto( id ): Go to specified track.
playlist.add( ... ): Add a bunch of items to the playlist.
  The playlist is a table of playlist objects.
  A playlist object has the following members:
      .path: the item's full path / URL
      .name: the item's name in playlist (OPTIONAL)
      .title: the item's Title (OPTIONAL, meta data)
      .artist: the item's Artist (OPTIONAL, meta data)
      .genre: the item's Genre (OPTIONAL, meta data)
      .copyright: the item's Copyright (OPTIONAL, meta data)
      .album: the item's Album (OPTIONAL, meta data)
      .tracknum: the item's Tracknum (OPTIONAL, meta data)
      .description: the item's Description (OPTIONAL, meta data)
      .rating: the item's Rating (OPTIONAL, meta data)
      .date: the item's Date (OPTIONAL, meta data)
      .setting: the item's Setting (OPTIONAL, meta data)
      .url: the item's URL (OPTIONAL, meta data)
      .language: the item's Language (OPTIONAL, meta data)
      .nowplaying: the item's NowPlaying (OPTIONAL, meta data)
      .publisher: the item's Publisher (OPTIONAL, meta data)
      .encodedby: the item's EncodedBy (OPTIONAL, meta data)
      .arturl: the item's ArtURL (OPTIONAL, meta data)
      .trackid: the item's TrackID (OPTIONAL, meta data)
      .options: a list of VLC options (OPTIONAL)
                example: .options = { "fullscreen" }
      .duration: stream duration in seconds (OPTIONAL)
      .meta: custom meta data (OPTIONAL, meta data)
             A .meta field is a table of custom meta categories which
             each have custom meta properties.
             example: .meta = { ["Google video"] = { ["docid"] = "-5784010886294950089"; ["GVP version"] = "1.1" }; ["misc"] = { "Hello" = "World!" } }
  Invalid playlist items will be discarded by VLC.
playlist.enqueue( ... ): like playlist.add() except that track isn't played.
playlist.get( [what, [tree]] ): Get the playlist.
  If "what" is a number, then this will return the corresponding playlist
  item's playlist hierarchy. If it is "normal" or "playlist", it will
  return the normal playlist. If it is "ml" or "media library", it will
  return the media library. If it is "root" it will return the full playlist.
  If it is a service discovery module's name, it will return that service
  discovery's playlist. If it is any other string, it won't return anything.
  Else it will return the full playlist.
  The second argument, "tree", is optional. If set to true or unset, the
  playlist will be returned in a tree layout. If set to false, the playlist
  will be returned using the flat layout.
  Each playlist item returned will have the following members:
      .id: The item's id.
      .flags: a table with the following members if the corresponding flag is
              set:
          .save
          .skip
          .disabled
          .ro
          .remove
          .expanded
      .name:
      .path:
      .duration: (-1 if unknown)
      .nb_played:
      .children: A table of children playlist items.

FIXME: add methods to get an item's meta, options, es ...

SD
--
sd.get_services_names(): Get a table of all available service discovery
  modules. The module name is used as key, the long name is used as value.
sd.add( name ): Add service discovery.
sd.remove( name ): Remove service discovery.
sd.is_loaded( name ): Check if service discovery is loaded.
sd.add_item( ... ): Add an item to the service discovery.
  The item object has the same members as the one in playlist.add().
  Returns the input item.
sd.add_node( ... ): Add a node to the service discovery.
  The node object has the following members:
      .title: the node's name
      .arturl: the node's ArtURL (OPTIONAL)

n = vlc.sd.add_node( {title="Node"} )
n:add_subitem( ... ): Same as sd.add_item(), but as a subitem of n.
n:add_node( ... ): Same as sd.add_node(), but as a subnode of n.

Stream
------
stream( url ): Instantiate a stream object for specific url.
memory_stream( string ): Instantiate a stream object containing a specific string.
  Those two functions return the stream object upon success and nil if an
  error occurred, in that case, the second return value will be an error message.

s = vlc.stream( "http://www.videolan.org/" )
s:read( 128 ) -- read up to 128 characters. Return 0 if no more data is available (FIXME?).
s:readline() -- read a line. Return nil if EOF was reached.
s:addfilter() -- add a stream filter. If no argument was specified, try to add all automatic stream filters.

Strings
-------
strings.decode_uri( [uri1, [uri2, [...]]] ): Decode a list of URIs. This
  function returns as many variables as it had arguments.
strings.encode_uri_component( [uri1, [uri2, [...]]] ): Encode a list of URI
  components. This function returns as many variables as it had arguments.
strings.resolve_xml_special_chars( [str1, [str2, [...]]] ): Resolve XML
  special characters in a list of strings. This function returns as many
  variables as it had arguments.
strings.convert_xml_special_chars( [str1, [str2, [...]]] ): Do the inverse
  operation.

Variables
---------
var.get( object, name ): Get the object's variable "name"'s value.
var.set( object, name, value ): Set the object's variable "name" to "value".
var.get_list( object, name ): Get the object's variable "name"'s value list.
  1st return value is the value list, 2nd return value is the text list.
var.create( object, name, value ): Create and set the object's variable "name"
  to "value". Created vars can be of type float, string or bool.

var.add_callback( object, name, function, data ): Add a callback to the
  object's "name" variable. Callback functions take 4 arguments: the
  variable name, the old value, the new value and data.
var.del_callback( object, name, function, data ): Delete a callback to
  the object's "name" variable. "function" and "data" must be the same as
  when add_callback() was called.
var.trigger_callback( object, name ): Trigger the callbacks associated with the
  object's "name" variable.

var.command( object name, name, argument ): Issue "object name"'s "name"
  command with argument "argument".
var.libvlc_command( name, argument ): Issue libvlc's "name" command with
  argument "argument".

Video
-----
video.fullscreen( [status] ):
 * toggle fullscreen if no arguments are given
 * switch to fullscreen 1st argument is true
 * disable fullscreen if 1st argument is false

VLM
---
vlm(): Instanciate a VLM object.

v = vlc.vlm()
v:execute_command( "new test broadcast" ) -- execute given VLM command

Note: if the VLM object is deleted and you were the last person to hold
a reference to it, all VLM items will be deleted.

Volume
------
volume.set( level ): Set volume to an absolute level between 0 and 1024.
  256 is 100%.
volume.get(): Get volume.
volume.up( [n] ): Increment volume by n steps of 32. n defaults to 1.
volume.down( [n] ): Decrement volume by n steps of 32. n defaults to 1.

