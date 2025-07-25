/*****************************************************************************
 * libvlc-module.c: Options for the main (libvlc itself) module
 *****************************************************************************
 * Copyright (C) 1998-2009 the VideoLAN team
 * $Id: 445a77ed8234e5070968e56b13d5762e069e9605 $
 *
 * Authors: Vincent Seguin <seguin@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Jean-Paul Saman <jpsaman #_at_# m2x.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

// Pretend we are a builtin module
#define MODULE_NAME main
#define MODULE_PATH main
#define __BUILTIN__


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_cpu.h>
#include "libvlc.h"

//#define Nothing here, this is just to prevent update-po from being stupid
#include "vlc_keys.h"
#include "vlc_meta.h"

#if defined (WIN32) || defined (__APPLE__)
static const char *const ppsz_language[] =
{
    "auto",
    "en",
    "ar",
    "bn",
    "pt_BR",
    "en_GB",
    "bg",
    "ca",
    "zh_TW",
    "cs",
    "da",
    "nl",
    "fi",
    "fr",
    "gl",
    "ka",
    "de",
    "he",
    "hr",
    "hu",
    "id",
    "it",
    "ja",
    "ko",
    "lt",
    "mn",
    "ms",
    "nn",
    "kk",
    "km",
    "oc",
    "fa",
    "pl",
    "pt_PT",
    "pa",
    "ro",
    "ru",
    "zh_CN",
    "si",
    "sr",
    "sk",
    "sl",
    "ckb",
    "es",
    "sv",
    "tr",
    "uk",
    "vi",
    "wa",
};

static const char *const ppsz_language_text[] =
{
    N_("Auto"),
    "American English",
    "ﻉﺮﺒﻳ",
    "বাংলা",
    "Português Brasileiro",
    "British English",
    "български език",
    "Català",
    "正體中文",
    "Čeština",
    "Dansk",
    "Nederlands",
    "Suomi",
    "Français",
    "Galego",
    "ქართული",
    "Deutsch",
    "עברית",
    "hrvatski",
    "Magyar",
    "Bahasa Indonesia",
    "Italiano",
    "日本語",
    "한국어",
    "lietuvių",
    "Монгол хэл",
    "Melayu",
    "Nynorsk",
    "Қазақ тілі",
    "ភាសាខ្មែរ",
    "Occitan",
    "ﻑﺍﺮﺳی",
    "Polski",
    "Português",
    "ਪੰਜਾਬੀ",
    "Română",
    "Русский",
    "简体中文",
    "සිංහල",
    "српски",
    "Slovensky",
    "slovenščina",
    "کوردیی سۆرانی",
    "Español",
    "Svenska",
    "Türkçe",
    "украї́нська мо́ва",
    "tiếng Việt",
    "Walon",
};
#endif

static const char *const ppsz_snap_formats[] =
{ "png", "jpg" };

/*****************************************************************************
 * Configuration options for the main program. Each module will also separatly
 * define its own configuration options.
 * Look into configuration.h if you need to know more about the following
 * macros.
 *****************************************************************************/

/*****************************************************************************
 * Intf
 ****************************************************************************/

// DEPRECATED
#define INTF_CAT_LONGTEXT N_( \
    "These options allow you to configure the interfaces used by VLC. " \
    "You can select the main interface, additional " \
    "interface modules, and define various related options." )

#define INTF_TEXT N_("Interface module")
#define INTF_LONGTEXT N_( \
    "This is the main interface used by VLC. " \
    "The default behavior is to automatically select the best module " \
    "available.")

#define EXTRAINTF_TEXT N_("Extra interface modules")
#define EXTRAINTF_LONGTEXT N_( \
    "You can select \"additional interfaces\" for VLC. " \
    "They will be launched in the background in addition to the default " \
    "interface. Use a colon separated list of interface modules. (common " \
    "values are \"rc\" (remote control), \"http\", \"gestures\" ...)")

#define CONTROL_TEXT N_("Control interfaces")
#define CONTROL_LONGTEXT N_( \
    "You can select control interfaces for VLC.")

#define VERBOSE_TEXT N_("Verbosity (0,1,2)")
#define VERBOSE_LONGTEXT N_( \
    "This is the verbosity level (0=only errors and " \
    "standard messages, 1=warnings, 2=debug).")

#define VERBOSE_OBJECTS_TEXT N_("Choose which objects should print debug " \
    "message")
#define VERBOSE_OBJECTS_LONGTEXT N_( \
    "This is a ',' separated string, each objects should be prefixed by " \
    "a '+' or a '-' to respectively enable or disable it. The keyword " \
    "'all' refers to all objects. Objects can be refered to by their " \
    "type or module name. Rules applying to named objects take precedence " \
    "over rules applying to object types. Note that you still need to " \
    "use -vvv to actually display debug message.")

#define QUIET_TEXT N_("Be quiet")
#define QUIET_LONGTEXT N_( \
    "Turn off all warning and information messages.")

#define OPEN_TEXT N_("Default stream")
#define OPEN_LONGTEXT N_( \
    "This stream will always be opened at VLC startup." )

#define LANGUAGE_TEXT N_("Language")
#define LANGUAGE_LONGTEXT N_( "You can manually select a language for the " \
    "interface. The system language is auto-detected if \"auto\" is " \
    "specified here." )

#define COLOR_TEXT N_("Color messages")
#define COLOR_LONGTEXT N_( \
    "This enables colorization of the messages sent to the console " \
    "Your terminal needs Linux color support for this to work.")

#define ADVANCED_TEXT N_("Show advanced options")
#define ADVANCED_LONGTEXT N_( \
    "When this is enabled, the preferences and/or interfaces will " \
    "show all available options, including those that most users should " \
    "never touch.")

#define INTERACTION_TEXT N_("Interface interaction")
#define INTERACTION_LONGTEXT N_( \
    "When this is enabled, the interface will show a dialog box each time " \
    "some user input is required." )


/*****************************************************************************
 * Audio
 ****************************************************************************/

// DEPRECATED
#define AOUT_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the audio " \
    "subsystem, and to add audio filters which can be used for " \
    "post processing or visual effects (spectrum analyzer, etc.). " \
    "Enable these filters here, and configure them in the \"audio filters\" " \
    "modules section.")

#define AOUT_TEXT N_("Audio output module")
#define AOUT_LONGTEXT N_( \
    "This is the audio output method used by VLC. " \
    "The default behavior is to automatically select the best method " \
    "available.")

#define AUDIO_TEXT N_("Enable audio")
#define AUDIO_LONGTEXT N_( \
    "You can completely disable the audio output. The audio " \
    "decoding stage will not take place, thus saving some processing power.")

#if 0
#define MONO_TEXT N_("Force mono audio")
#define MONO_LONGTEXT N_("This will force a mono audio output.")
#endif

#define VOLUME_TEXT N_("Default audio volume")
#define VOLUME_LONGTEXT N_( \
    "You can set the default audio output volume here, in a range from 0 to " \
    "1024.")

#define VOLUME_SAVE_TEXT N_("Audio output saved volume")
#define VOLUME_SAVE_LONGTEXT N_( \
    "This saves the audio output volume when you use the mute function. " \
    "You should not change this option manually.")

#define VOLUME_STEP_TEXT N_("Audio output volume step")
#define VOLUME_STEP_LONGTEXT N_( \
    "The step size of the volume is adjustable using this option, " \
    "in a range from 0 to 1024." )

#define AOUT_RATE_TEXT N_("Audio output frequency (Hz)")
#define AOUT_RATE_LONGTEXT N_( \
    "You can force the audio output frequency here. Common values are " \
    "-1 (default), 48000, 44100, 32000, 22050, 16000, 11025, 8000.")

#if !defined( __APPLE__ )
#define AOUT_RESAMP_TEXT N_("High quality audio resampling")
#define AOUT_RESAMP_LONGTEXT N_( \
    "This uses a high quality audio resampling algorithm. High quality " \
    "audio resampling can be processor intensive so you can " \
    "disable it and a cheaper resampling algorithm will be used instead.")
#endif

#define DESYNC_TEXT N_("Audio desynchronization compensation")
#define DESYNC_LONGTEXT N_( \
    "This delays the audio output. The delay must be given in milliseconds. " \
    "This can be handy if you notice a lag between the video and the audio.")

#define MULTICHA_TEXT N_("Audio output channels mode")
#define MULTICHA_LONGTEXT N_( \
    "This sets the audio output channels mode that will " \
    "be used by default when possible (ie. if your hardware supports it as " \
    "well as the audio stream being played).")

#define SPDIF_TEXT N_("Use S/PDIF when available")
#define SPDIF_LONGTEXT N_( \
    "S/PDIF can be used by default when " \
    "your hardware supports it as well as the audio stream being played.")

#define FORCE_DOLBY_TEXT N_("Force detection of Dolby Surround")
#define FORCE_DOLBY_LONGTEXT N_( \
    "Use this when you know your stream is (or is not) encoded with Dolby "\
    "Surround but fails to be detected as such. Even if the stream is "\
    "not actually encoded with Dolby Surround, turning on this option might "\
    "enhance your experience, especially when combined with the Headphone "\
    "Channel Mixer." )
static const int pi_force_dolby_values[] = { 0, 1, 2 };
static const char *const ppsz_force_dolby_descriptions[] = {
    N_("Auto"), N_("On"), N_("Off") };


#define AUDIO_FILTER_TEXT N_("Audio filters")
#define AUDIO_FILTER_LONGTEXT N_( \
    "This adds audio post processing filters, to modify " \
    "the sound rendering." )

#define AUDIO_VISUAL_TEXT N_("Audio visualizations ")
#define AUDIO_VISUAL_LONGTEXT N_( \
    "This adds visualization modules (spectrum analyzer, etc.).")


#define AUDIO_REPLAY_GAIN_MODE_TEXT N_( \
    "Replay gain mode" )
#define AUDIO_REPLAY_GAIN_MODE_LONGTEXT N_( \
    "Select the replay gain mode" )
#define AUDIO_REPLAY_GAIN_PREAMP_TEXT N_( \
    "Replay preamp" )
#define AUDIO_REPLAY_GAIN_PREAMP_LONGTEXT N_( \
    "This allows you to change the default target level (89 dB) " \
    "for stream with replay gain information" )
#define AUDIO_REPLAY_GAIN_DEFAULT_TEXT N_( \
    "Default replay gain" )
#define AUDIO_REPLAY_GAIN_DEFAULT_LONGTEXT N_( \
    "This is the gain used for stream without replay gain information" )
#define AUDIO_REPLAY_GAIN_PEAK_PROTECTION_TEXT N_( \
    "Peak protection" )
#define AUDIO_REPLAY_GAIN_PEAK_PROTECTION_LONGTEXT N_( \
    "Protect against sound clipping" )

#define AUDIO_TIME_STRETCH_TEXT N_( \
    "Enable time streching audio" )
#define AUDIO_TIME_STRETCH_LONGTEXT N_( \
    "This allows to play audio at lower or higher speed without " \
    "affecting the audio pitch" )


static const char *const ppsz_replay_gain_mode[] = {
    "none", "track", "album" };
static const char *const ppsz_replay_gain_mode_text[] = {
    N_("None"), N_("Track"), N_("Album") };

/*****************************************************************************
 * Video
 ****************************************************************************/

// DEPRECATED
#define VOUT_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the video output " \
    "subsystem. You can for example enable video filters (deinterlacing, " \
    "image adjusting, etc.). Enable these filters here and configure " \
    "them in the \"video filters\" modules section. You can also set many " \
    "miscellaneous video options." )

#define VOUT_TEXT N_("Video output module")
#define VOUT_LONGTEXT N_( \
    "This is the the video output method used by VLC. " \
    "The default behavior is to automatically select the best method available.")

#define VIDEO_TEXT N_("Enable video")
#define VIDEO_LONGTEXT N_( \
    "You can completely disable the video output. The video " \
    "decoding stage will not take place, thus saving some processing power.")

#define WIDTH_TEXT N_("Video width")
#define WIDTH_LONGTEXT N_( \
    "You can enforce the video width. By default (-1) VLC will " \
    "adapt to the video characteristics.")

#define HEIGHT_TEXT N_("Video height")
#define HEIGHT_LONGTEXT N_( \
    "You can enforce the video height. By default (-1) VLC will " \
    "adapt to the video characteristics.")

#define VIDEOX_TEXT N_("Video X coordinate")
#define VIDEOX_LONGTEXT N_( \
    "You can enforce the position of the top left corner of the video window "\
    "(X coordinate).")

#define VIDEOY_TEXT N_("Video Y coordinate")
#define VIDEOY_LONGTEXT N_( \
    "You can enforce the position of the top left corner of the video window "\
    "(Y coordinate).")

#define VIDEO_TITLE_TEXT N_("Video title")
#define VIDEO_TITLE_LONGTEXT N_( \
    "Custom title for the video window (in case the video is not embedded in "\
    "the interface).")

#define ALIGN_TEXT N_("Video alignment")
#define ALIGN_LONGTEXT N_( \
    "Enforce the alignment of the video in its window. By default (0) it " \
    "will be centered (0=center, 1=left, 2=right, 4=top, 8=bottom, you can " \
    "also use combinations of these values, like 6=4+2 meaning top-right).")
static const int pi_align_values[] = { 0, 1, 2, 4, 8, 5, 6, 9, 10 };
static const char *const ppsz_align_descriptions[] =
{ N_("Center"), N_("Left"), N_("Right"), N_("Top"), N_("Bottom"),
  N_("Top-Left"), N_("Top-Right"), N_("Bottom-Left"), N_("Bottom-Right") };

#define ZOOM_TEXT N_("Zoom video")
#define ZOOM_LONGTEXT N_( \
    "You can zoom the video by the specified factor.")

#define GRAYSCALE_TEXT N_("Grayscale video output")
#define GRAYSCALE_LONGTEXT N_( \
    "Output video in grayscale. As the color information aren't decoded, " \
    "this can save some processing power." )

#define EMBEDDED_TEXT N_("Embedded video")
#define EMBEDDED_LONGTEXT N_( \
    "Embed the video output in the main interface." )

#define DISPLAY_TEXT N_("X11 display")
#define DISPLAY_LONGTEXT N_( \
    "X11 hardware display to use. By default VLC will " \
    "use the value of the DISPLAY environment variable.")

#define FULLSCREEN_TEXT N_("Fullscreen video output")
#define FULLSCREEN_LONGTEXT N_( \
    "Start video in fullscreen mode" )

#define OVERLAY_TEXT N_("Overlay video output")
#define OVERLAY_LONGTEXT N_( \
    "Overlay is the hardware acceleration capability of your video card " \
    "(ability to render video directly). VLC will try to use it by default." )

#define VIDEO_ON_TOP_TEXT N_("Always on top")
#define VIDEO_ON_TOP_LONGTEXT N_( \
    "Always place the video window on top of other windows." )

#define WALLPAPER_TEXT N_("Enable wallpaper mode ")
#define WALLPAPER_LONGTEXT N_( \
    "The wallpaper mode allows you to display the video as the desktop " \
    "background." )

#define VIDEO_TITLE_SHOW_TEXT N_("Show media title on video")
#define VIDEO_TITLE_SHOW_LONGTEXT N_( \
    "Display the title of the video on top of the movie.")

#define VIDEO_TITLE_TIMEOUT_TEXT N_("Show video title for x milliseconds")
#define VIDEO_TITLE_TIMEOUT_LONGTEXT N_( \
    "Show the video title for n milliseconds, default is 5000 ms (5 sec.)")

#define VIDEO_TITLE_POSITION_TEXT N_("Position of video title")
#define VIDEO_TITLE_POSITION_LONGTEXT N_( \
    "Place on video where to display the title (default bottom center).")

#define MOUSE_HIDE_TIMEOUT_TEXT N_("Hide cursor and fullscreen " \
                                   "controller after x milliseconds")
#define MOUSE_HIDE_TIMEOUT_LONGTEXT N_( \
    "Hide mouse cursor and fullscreen controller after " \
    "n milliseconds, default is 3000 ms (3 sec.)")

#define DEINTERLACE_TEXT N_("Deinterlace")
#define DEINTERLACE_LONGTEXT N_(\
    "Deinterlace")
static const int pi_deinterlace[] = {
    0, -1, 1
};
static const char * const  ppsz_deinterlace_text[] = {
    "Off", "Automatic", "On"
};

#define DEINTERLACE_MODE_TEXT N_("Deinterlace mode")
#define DEINTERLACE_MODE_LONGTEXT N_( \
    "Deinterlace method to use for video processing.")
static const char * const ppsz_deinterlace_mode[] = {
    "discard", "blend", "mean", "bob",
    "linear", "x", "yadif", "yadif2x"
};
static const char * const ppsz_deinterlace_mode_text[] = {
    N_("Discard"), N_("Blend"), N_("Mean"), N_("Bob"),
    N_("Linear"), "X", "Yadif", "Yadif (2x)"
};

static const int pi_pos_values[] = { 0, 1, 2, 4, 8, 5, 6, 9, 10 };
static const char *const ppsz_pos_descriptions[] =
{ N_("Center"), N_("Left"), N_("Right"), N_("Top"), N_("Bottom"),
  N_("Top-Left"), N_("Top-Right"), N_("Bottom-Left"), N_("Bottom-Right") };

#define SS_TEXT N_("Disable screensaver")
#define SS_LONGTEXT N_("Disable the screensaver during video playback." )

#define INHIBIT_TEXT N_("Inhibit the power management daemon during playback")
#define INHIBIT_LONGTEXT N_("Inhibits the power management daemon during any " \
    "playback, to avoid the computer being suspended because of inactivity.")

#define VIDEO_DECO_TEXT N_("Window decorations")
#define VIDEO_DECO_LONGTEXT N_( \
    "VLC can avoid creating window caption, frames, etc... around the video" \
    ", giving a \"minimal\" window.")

#define VOUT_FILTER_TEXT N_("Video output filter module")
#define VOUT_FILTER_LONGTEXT N_( \
    "This adds video output filters like clone or wall" )

#define VIDEO_FILTER_TEXT N_("Video filter module")
#define VIDEO_FILTER_LONGTEXT N_( \
    "This adds post-processing filters to enhance the " \
    "picture quality, for instance deinterlacing, or distort " \
    "the video.")

#define SNAP_PATH_TEXT N_("Video snapshot directory (or filename)")
#define SNAP_PATH_LONGTEXT N_( \
    "Directory where the video snapshots will be stored.")

#define SNAP_PREFIX_TEXT N_("Video snapshot file prefix")
#define SNAP_PREFIX_LONGTEXT N_( \
    "Video snapshot file prefix" )

#define SNAP_FORMAT_TEXT N_("Video snapshot format")
#define SNAP_FORMAT_LONGTEXT N_( \
    "Image format which will be used to store the video snapshots" )

#define SNAP_PREVIEW_TEXT N_("Display video snapshot preview")
#define SNAP_PREVIEW_LONGTEXT N_( \
    "Display the snapshot preview in the screen's top-left corner.")

#define SNAP_SEQUENTIAL_TEXT N_("Use sequential numbers instead of timestamps")
#define SNAP_SEQUENTIAL_LONGTEXT N_( \
    "Use sequential numbers instead of timestamps for snapshot numbering")

#define SNAP_WIDTH_TEXT N_("Video snapshot width")
#define SNAP_WIDTH_LONGTEXT N_( \
    "You can enforce the width of the video snapshot. By default " \
    "it will keep the original width (-1). Using 0 will scale the width " \
    "to keep the aspect ratio." )

#define SNAP_HEIGHT_TEXT N_("Video snapshot height")
#define SNAP_HEIGHT_LONGTEXT N_( \
    "You can enforce the height of the video snapshot. By default " \
    "it will keep the original height (-1). Using 0 will scale the height " \
    "to keep the aspect ratio." )

#define CROP_TEXT N_("Video cropping")
#define CROP_LONGTEXT N_( \
    "This forces the cropping of the source video. " \
    "Accepted formats are x:y (4:3, 16:9, etc.) expressing the global image " \
    "aspect.")

#define ASPECT_RATIO_TEXT N_("Source aspect ratio")
#define ASPECT_RATIO_LONGTEXT N_( \
    "This forces the source aspect ratio. For instance, some DVDs claim " \
    "to be 16:9 while they are actually 4:3. This can also be used as a " \
    "hint for VLC when a movie does not have aspect ratio information. " \
    "Accepted formats are x:y (4:3, 16:9, etc.) expressing the global image " \
    "aspect, or a float value (1.25, 1.3333, etc.) expressing pixel " \
    "squareness.")

#define AUTOSCALE_TEXT N_("Video Auto Scaling")
#define AUTOSCALE_LONGTEXT N_( \
    "Let the video scale to fit a given window or fullscreen.")

#define SCALEFACTOR_TEXT N_("Video scaling factor")
#define SCALEFACTOR_LONGTEXT N_( \
    "Scaling factor used when Auto Scaling is disabled.\n" \
    "Default value is 1.0 (original video size).")

#define CUSTOM_CROP_RATIOS_TEXT N_("Custom crop ratios list")
#define CUSTOM_CROP_RATIOS_LONGTEXT N_( \
    "Comma separated list of crop ratios which will be added in the " \
    "interface's crop ratios list.")

#define CUSTOM_ASPECT_RATIOS_TEXT N_("Custom aspect ratios list")
#define CUSTOM_ASPECT_RATIOS_LONGTEXT N_( \
    "Comma separated list of aspect ratios which will be added in the " \
    "interface's aspect ratio list.")

#define HDTV_FIX_TEXT N_("Fix HDTV height")
#define HDTV_FIX_LONGTEXT N_( \
    "This allows proper handling of HDTV-1080 video format " \
    "even if broken encoder incorrectly sets height to 1088 lines. " \
    "You should only disable this option if your video has a " \
    "non-standard format requiring all 1088 lines.")

#define MASPECT_RATIO_TEXT N_("Monitor pixel aspect ratio")
#define MASPECT_RATIO_LONGTEXT N_( \
    "This forces the monitor aspect ratio. Most monitors have square " \
    "pixels (1:1). If you have a 16:9 screen, you might need to change this " \
    "to 4:3 in order to keep proportions.")

#define SKIP_FRAMES_TEXT N_("Skip frames")
#define SKIP_FRAMES_LONGTEXT N_( \
    "Enables framedropping on MPEG2 stream. Framedropping " \
    "occurs when your computer is not powerful enough" )

#define DROP_LATE_FRAMES_TEXT N_("Drop late frames")
#define DROP_LATE_FRAMES_LONGTEXT N_( \
    "This drops frames that are late (arrive to the video output after " \
    "their intended display date)." )

#define QUIET_SYNCHRO_TEXT N_("Quiet synchro")
#define QUIET_SYNCHRO_LONGTEXT N_( \
    "This avoids flooding the message log with debug output from the " \
    "video output synchronization mechanism.")

#define KEYBOARD_EVENTS_TEXT N_("Key press events")
#define KEYBOARD_EVENTS_LONGTEXT N_( \
    "This enables VLC hotkeys from the (non-embedded) video window." )

#define MOUSE_EVENTS_TEXT N_("Mouse events")
#define MOUSE_EVENTS_LONGTEXT N_( \
    "This enables handling of mouse clicks on the video." )

/*****************************************************************************
 * Input
 ****************************************************************************/

// Deprecated
#define INPUT_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the input " \
    "subsystem, such as the DVD or VCD device, the network interface " \
    "settings or the subtitle channel.")

#define CR_AVERAGE_TEXT N_("Clock reference average counter")
#define CR_AVERAGE_LONGTEXT N_( \
    "When using the PVR input (or a very irregular source), you should " \
    "set this to 10000.")

#define CLOCK_SYNCHRO_TEXT N_("Clock synchronisation")
#define CLOCK_SYNCHRO_LONGTEXT N_( \
    "It is possible to disable the input clock synchronisation for " \
    "real-time sources. Use this if you experience jerky playback of " \
    "network streams.")

#define CLOCK_JITTER_TEXT N_("Clock jitter")
#define CLOCK_JITTER_LONGTEXT N_( \
    "It tells the clock algorithms what is the maximal input jitter that " \
    "is considered valid and can be compensated (in milliseconds)" )

#define NETSYNC_TEXT N_("Network synchronisation" )
#define NETSYNC_LONGTEXT N_( "This allows you to remotely " \
        "synchronise clocks for server and client. The detailed settings " \
        "are available in Advanced / Network Sync." )

static const int pi_clock_values[] = { -1, 0, 1 };
static const char *const ppsz_clock_descriptions[] =
{ N_("Default"), N_("Disable"), N_("Enable") };

#define SERVER_PORT_TEXT N_("UDP port")
#define SERVER_PORT_LONGTEXT N_( \
    "This is the default port used for UDP streams. Default is 1234." )

#define MTU_TEXT N_("MTU of the network interface")
#define MTU_LONGTEXT N_( \
    "This is the maximum application-layer packet size that can be " \
    "transmitted over the network (in bytes).")
/* Should be less than 1500 - 8[ppp] - 40[ip6] - 8[udp] in any case. */
#define MTU_DEFAULT 1400

#define TTL_TEXT N_("Hop limit (TTL)")
#define TTL_LONGTEXT N_( \
    "This is the hop limit (also known as \"Time-To-Live\" or TTL) of " \
    "the multicast packets sent by the stream output (-1 = use operating " \
    "system built-in default).")

#define MIFACE_TEXT N_("Multicast output interface")
#define MIFACE_LONGTEXT N_( \
    "Default multicast interface. This overrides the routing table.")

#define MIFACE_ADDR_TEXT N_("IPv4 multicast output interface address")
#define MIFACE_ADDR_LONGTEXT N_( \
    "IPv4 address for the default multicast interface. This overrides " \
    "the routing table.")

#define DSCP_TEXT N_("DiffServ Code Point")
#define DSCP_LONGTEXT N_("Differentiated Services Code Point " \
    "for outgoing UDP streams (or IPv4 Type Of Service, " \
    "or IPv6 Traffic Class). This is used for network Quality of Service.")

#define INPUT_PROGRAM_TEXT N_("Program")
#define INPUT_PROGRAM_LONGTEXT N_( \
    "Choose the program to select by giving its Service ID. " \
    "Only use this option if you want to read a multi-program stream " \
    "(like DVB streams for example)." )

#define INPUT_PROGRAMS_TEXT N_("Programs")
#define INPUT_PROGRAMS_LONGTEXT N_( \
    "Choose the programs to select by giving a comma-separated list of " \
    "Service IDs (SIDs). " \
    "Only use this option if you want to read a multi-program stream " \
    "(like DVB streams for example)." )

/// \todo Document how to find it
#define INPUT_AUDIOTRACK_TEXT N_("Audio track")
#define INPUT_AUDIOTRACK_LONGTEXT N_( \
    "Stream number of the audio track to use " \
    "(from 0 to n).")

#define INPUT_SUBTRACK_TEXT N_("Subtitles track")
#define INPUT_SUBTRACK_LONGTEXT N_( \
    "Stream number of the subtitle track to use " \
    "(from 0 to n).")

#define INPUT_AUDIOTRACK_LANG_TEXT N_("Audio language")
#define INPUT_AUDIOTRACK_LANG_LONGTEXT N_( \
    "Language of the audio track you want to use " \
    "(comma separated, two or three letter country code, you may use 'none' to avoid a fallback to another language).")

#define INPUT_SUBTRACK_LANG_TEXT N_("Subtitle language")
#define INPUT_SUBTRACK_LANG_LONGTEXT N_( \
    "Language of the subtitle track you want to use " \
    "(comma separated, two or three letters country code, you may use 'any' as a fallback).")

/// \todo Document how to find it
#define INPUT_AUDIOTRACK_ID_TEXT N_("Audio track ID")
#define INPUT_AUDIOTRACK_ID_LONGTEXT N_( \
    "Stream ID of the audio track to use.")

#define INPUT_SUBTRACK_ID_TEXT N_("Subtitles track ID")
#define INPUT_SUBTRACK_ID_LONGTEXT N_( \
    "Stream ID of the subtitle track to use.")

#define INPUT_REPEAT_TEXT N_("Input repetitions")
#define INPUT_REPEAT_LONGTEXT N_( \
    "Number of time the same input will be repeated")

#define START_TIME_TEXT N_("Start time")
#define START_TIME_LONGTEXT N_( \
    "The stream will start at this position (in seconds)." )

#define STOP_TIME_TEXT N_("Stop time")
#define STOP_TIME_LONGTEXT N_( \
    "The stream will stop at this position (in seconds)." )

#define RUN_TIME_TEXT N_("Run time")
#define RUN_TIME_LONGTEXT N_( \
    "The stream will run this duration (in seconds)." )

#define INPUT_FAST_SEEK_TEXT N_("Fast seek")
#define INPUT_FAST_SEEK_LONGTEXT N_( \
    "Favor speed over precision while seeking" )

#define INPUT_RATE_TEXT N_("Playback speed")
#define INPUT_RATE_LONGTEXT N_( \
    "This defines the playback speed (nominal speed is 1.0)." )

#define INPUT_LIST_TEXT N_("Input list")
#define INPUT_LIST_LONGTEXT N_( \
    "You can give a comma-separated list " \
    "of inputs that will be concatenated together after the normal one.")

#define INPUT_SLAVE_TEXT N_("Input slave (experimental)")
#define INPUT_SLAVE_LONGTEXT N_( \
    "This allows you to play from several inputs at " \
    "the same time. This feature is experimental, not all formats " \
    "are supported. Use a '#' separated list of inputs.")

#define BOOKMARKS_TEXT N_("Bookmarks list for a stream")
#define BOOKMARKS_LONGTEXT N_( \
    "You can manually give a list of bookmarks for a stream in " \
    "the form \"{name=bookmark-name,time=optional-time-offset," \
    "bytes=optional-byte-offset},{...}\"")

#define INPUT_RECORD_PATH_TEXT N_("Record directory or filename")
#define INPUT_RECORD_PATH_LONGTEXT N_( \
    "Directory or filename where the records will be stored" )

#define INPUT_RECORD_NATIVE_TEXT N_("Prefer native stream recording")
#define INPUT_RECORD_NATIVE_LONGTEXT N_( \
    "When possible, the input stream will be recorded instead of using " \
    "the stream output module" )

#define INPUT_TIMESHIFT_PATH_TEXT N_("Timeshift directory")
#define INPUT_TIMESHIFT_PATH_LONGTEXT N_( \
    "Directory used to store the timeshift temporary files." )

#define INPUT_TIMESHIFT_GRANULARITY_TEXT N_("Timeshift granularity")
#define INPUT_TIMESHIFT_GRANULARITY_LONGTEXT N_( \
    "This is the maximum size in bytes of the temporary files " \
    "that will be used to store the timeshifted streams." )

// DEPRECATED
#define SUB_CAT_LONGTEXT N_( \
    "These options allow you to modify the behavior of the subpictures " \
    "subsystem. You can for example enable subpictures filters (logo, etc.). " \
    "Enable these filters here and configure them in the " \
    "\"subpictures filters\" modules section. You can also set many " \
    "miscellaneous subpictures options." )

#define SUB_MARGIN_TEXT N_("Force subtitle position")
#define SUB_MARGIN_LONGTEXT N_( \
    "You can use this option to place the subtitles under the movie, " \
    "instead of over the movie. Try several positions.")

#define SPU_TEXT N_("Enable sub-pictures")
#define SPU_LONGTEXT N_( \
    "You can completely disable the sub-picture processing.")

#define OSD_TEXT N_("On Screen Display")
#define OSD_LONGTEXT N_( \
    "VLC can display messages on the video. This is called OSD (On Screen " \
    "Display).")

#define TEXTRENDERER_TEXT N_("Text rendering module")
#define TEXTRENDERER_LONGTEXT N_( \
    "VLC normally uses Freetype for rendering, but this allows you to use svg for instance.")

#define SUB_FILTER_TEXT N_("Subpictures filter module")
#define SUB_FILTER_LONGTEXT N_( \
    "This adds so-called \"subpicture filters\". These filters overlay " \
    "some images or text over the video (like a logo, arbitrary text, ...)." )

#define SUB_AUTO_TEXT N_("Autodetect subtitle files")
#define SUB_AUTO_LONGTEXT N_( \
    "Automatically detect a subtitle file, if no subtitle filename is " \
    "specified (based on the filename of the movie).")

#define SUB_FUZZY_TEXT N_("Subtitle autodetection fuzziness")
#define SUB_FUZZY_LONGTEXT N_( \
    "This determines how fuzzy subtitle and movie filename matching " \
    "will be. Options are:\n" \
    "0 = no subtitles autodetected\n" \
    "1 = any subtitle file\n" \
    "2 = any subtitle file containing the movie name\n" \
    "3 = subtitle file matching the movie name with additional chars\n" \
    "4 = subtitle file matching the movie name exactly")

#define SUB_PATH_TEXT N_("Subtitle autodetection paths")
#define SUB_PATH_LONGTEXT N_( \
    "Look for a subtitle file in those paths too, if your subtitle " \
    "file was not found in the current directory.")

#define SUB_FILE_TEXT N_("Use subtitle file")
#define SUB_FILE_LONGTEXT N_( \
    "Load this subtitle file. To be used when autodetect cannot detect " \
    "your subtitle file.")

#define DVD_DEV_TEXT N_("DVD device")
#ifdef WIN32
#define DVD_DEV_LONGTEXT N_( \
    "This is the default DVD drive (or file) to use. Don't forget the colon " \
    "after the drive letter (eg. D:)")
#else
#define DVD_DEV_LONGTEXT N_( \
    "This is the default DVD device to use.")
#endif

#define VCD_DEV_TEXT N_("VCD device")
#define VCD_DEV_LONGTEXT N_( \
    "This is the default VCD device to use." )

#define CDAUDIO_DEV_TEXT N_("Audio CD device")
#define CDAUDIO_DEV_LONGTEXT N_( \
    "This is the default Audio CD device to use." )

#define IPV6_TEXT N_("Force IPv6")
#define IPV6_LONGTEXT N_( \
    "IPv6 will be used by default for all connections.")

#define IPV4_TEXT N_("Force IPv4")
#define IPV4_LONGTEXT N_( \
    "IPv4 will be used by default for all connections.")

#define TIMEOUT_TEXT N_("TCP connection timeout")
#define TIMEOUT_LONGTEXT N_( \
    "Default TCP connection timeout (in milliseconds). " )

#define SOCKS_SERVER_TEXT N_("SOCKS server")
#define SOCKS_SERVER_LONGTEXT N_( \
    "SOCKS proxy server to use. This must be of the form " \
    "address:port. It will be used for all TCP connections" )

#define SOCKS_USER_TEXT N_("SOCKS user name")
#define SOCKS_USER_LONGTEXT N_( \
    "User name to be used for connection to the SOCKS proxy." )

#define SOCKS_PASS_TEXT N_("SOCKS password")
#define SOCKS_PASS_LONGTEXT N_( \
    "Password to be used for connection to the SOCKS proxy." )

#define META_TITLE_TEXT N_("Title metadata")
#define META_TITLE_LONGTEXT N_( \
     "Allows you to specify a \"title\" metadata for an input.")

#define META_AUTHOR_TEXT N_("Author metadata")
#define META_AUTHOR_LONGTEXT N_( \
     "Allows you to specify an \"author\" metadata for an input.")

#define META_ARTIST_TEXT N_("Artist metadata")
#define META_ARTIST_LONGTEXT N_( \
     "Allows you to specify an \"artist\" metadata for an input.")

#define META_GENRE_TEXT N_("Genre metadata")
#define META_GENRE_LONGTEXT N_( \
     "Allows you to specify a \"genre\" metadata for an input.")

#define META_CPYR_TEXT N_("Copyright metadata")
#define META_CPYR_LONGTEXT N_( \
     "Allows you to specify a \"copyright\" metadata for an input.")

#define META_DESCR_TEXT N_("Description metadata")
#define META_DESCR_LONGTEXT N_( \
     "Allows you to specify a \"description\" metadata for an input.")

#define META_DATE_TEXT N_("Date metadata")
#define META_DATE_LONGTEXT N_( \
     "Allows you to specify a \"date\" metadata for an input.")

#define META_URL_TEXT N_("URL metadata")
#define META_URL_LONGTEXT N_( \
     "Allows you to specify a \"url\" metadata for an input.")

// DEPRECATED
#define CODEC_CAT_LONGTEXT N_( \
    "This option can be used to alter the way VLC selects " \
    "its codecs (decompression methods). Only advanced users should " \
    "alter this option as it can break playback of all your streams." )

#define CODEC_TEXT N_("Preferred decoders list")
#define CODEC_LONGTEXT N_( \
    "List of codecs that VLC will use in " \
    "priority. For instance, 'dummy,a52' will try the dummy and a52 codecs " \
    "before trying the other ones. Only advanced users should " \
    "alter this option as it can break playback of all your streams." )

#define ENCODER_TEXT N_("Preferred encoders list")
#define ENCODER_LONGTEXT N_( \
    "This allows you to select a list of encoders that VLC will use in " \
    "priority.")

#define SYSTEM_CODEC_TEXT N_("Prefer system plugins over VLC")
#define SYSTEM_CODEC_LONGTEXT N_( \
    "Indicates whether VLC will prefer native plugins installed " \
    "on system over VLC owns plugins whenever a choice is available." )

/*****************************************************************************
 * Sout
 ****************************************************************************/

// DEPRECATED
#define SOUT_CAT_LONGTEXT N_( \
    "These options allow you to set default global options for the " \
    "stream output subsystem." )

#define SOUT_TEXT N_("Default stream output chain")
#define SOUT_LONGTEXT N_( \
    "You can enter here a default stream output chain. Refer to "\
    "the documentation to learn how to build such chains. " \
    "Warning: this chain will be enabled for all streams." )

#define SOUT_ALL_TEXT N_("Enable streaming of all ES")
#define SOUT_ALL_LONGTEXT N_( \
    "Stream all elementary streams (video, audio and subtitles)")

#define SOUT_DISPLAY_TEXT N_("Display while streaming")
#define SOUT_DISPLAY_LONGTEXT N_( \
    "Play locally the stream while streaming it.")

#define SOUT_VIDEO_TEXT N_("Enable video stream output")
#define SOUT_VIDEO_LONGTEXT N_( \
    "Choose whether the video stream should be redirected to " \
    "the stream output facility when this last one is enabled.")

#define SOUT_AUDIO_TEXT N_("Enable audio stream output")
#define SOUT_AUDIO_LONGTEXT N_( \
    "Choose whether the audio stream should be redirected to " \
    "the stream output facility when this last one is enabled.")

#define SOUT_SPU_TEXT N_("Enable SPU stream output")
#define SOUT_SPU_LONGTEXT N_( \
    "Choose whether the SPU streams should be redirected to " \
    "the stream output facility when this last one is enabled.")

#define SOUT_KEEP_TEXT N_("Keep stream output open" )
#define SOUT_KEEP_LONGTEXT N_( \
    "This allows you to keep an unique stream output instance across " \
    "multiple playlist item (automatically insert the gather stream output " \
    "if not specified)" )

#define SOUT_MUX_CACHING_TEXT N_("Stream output muxer caching (ms)")
#define SOUT_MUX_CACHING_LONGTEXT N_( \
    "This allow you to configure the initial caching amount for stream output " \
    " muxer. This value should be set in milliseconds." )

#define PACKETIZER_TEXT N_("Preferred packetizer list")
#define PACKETIZER_LONGTEXT N_( \
    "This allows you to select the order in which VLC will choose its " \
    "packetizers."  )

#define MUX_TEXT N_("Mux module")
#define MUX_LONGTEXT N_( \
    "This is a legacy entry to let you configure mux modules")

#define ACCESS_OUTPUT_TEXT N_("Access output module")
#define ACCESS_OUTPUT_LONGTEXT N_( \
    "This is a legacy entry to let you configure access output modules")

#define ANN_SAPCTRL_TEXT N_("Control SAP flow")
#define ANN_SAPCTRL_LONGTEXT N_( \
    "If this option is enabled, the flow on " \
    "the SAP multicast address will be controlled. This is needed if you " \
    "want to make announcements on the MBone." )

#define ANN_SAPINTV_TEXT N_("SAP announcement interval")
#define ANN_SAPINTV_LONGTEXT N_( \
    "When the SAP flow control is disabled, " \
    "this lets you set the fixed interval between SAP announcements." )

/*****************************************************************************
 * Advanced
 ****************************************************************************/

// DEPRECATED
#define CPU_CAT_LONGTEXT N_( \
    "These options allow you to enable special CPU optimizations. " \
    "You should always leave all these enabled." )

#define MMX_TEXT N_("Enable CPU MMX support")
#define MMX_LONGTEXT N_( \
    "If your processor supports the MMX instructions set, VLC can take " \
    "advantage of them.")

#define THREE_DN_TEXT N_("Enable CPU 3D Now! support")
#define THREE_DN_LONGTEXT N_( \
    "If your processor supports the 3D Now! instructions set, VLC can take " \
    "advantage of them.")

#define MMXEXT_TEXT N_("Enable CPU MMX EXT support")
#define MMXEXT_LONGTEXT N_( \
    "If your processor supports the MMX EXT instructions set, VLC can take " \
    "advantage of them.")

#define SSE_TEXT N_("Enable CPU SSE support")
#define SSE_LONGTEXT N_( \
    "If your processor supports the SSE instructions set, VLC can take " \
    "advantage of them.")

#define SSE2_TEXT N_("Enable CPU SSE2 support")
#define SSE2_LONGTEXT N_( \
    "If your processor supports the SSE2 instructions set, VLC can take " \
    "advantage of them.")

#define SSE3_TEXT N_("Enable CPU SSE3 support")
#define SSE3_LONGTEXT N_( \
    "If your processor supports the SSE3 instructions set, VLC can take " \
    "advantage of them.")

#define SSSE3_TEXT N_("Enable CPU SSSE3 support")
#define SSSE3_LONGTEXT N_( \
    "If your processor supports the SSSE3 instructions set, VLC can take " \
    "advantage of them.")

#define SSE4_1_TEXT N_("Enable CPU SSE4.1 support")
#define SSE4_1_LONGTEXT N_( \
    "If your processor supports the SSE4.1 instructions set, VLC can take " \
    "advantage of them.")

#define SSE4_2_TEXT N_("Enable CPU SSE4.2 support")
#define SSE4_2_LONGTEXT N_( \
    "If your processor supports the SSE4.2 instructions set, VLC can take " \
    "advantage of them.")

#define ALTIVEC_TEXT N_("Enable CPU AltiVec support")
#define ALTIVEC_LONGTEXT N_( \
    "If your processor supports the AltiVec instructions set, VLC can take " \
    "advantage of them.")

// DEPRECATED
#define MISC_CAT_LONGTEXT N_( \
    "These options allow you to select default modules. Leave these " \
    "alone unless you really know what you are doing." )

#define MEMCPY_TEXT N_("Memory copy module")
#define MEMCPY_LONGTEXT N_( \
    "You can select which memory copy module you want to use. By default " \
    "VLC will select the fastest one supported by your hardware.")

#define ACCESS_TEXT N_("Access module")
#define ACCESS_LONGTEXT N_( \
    "This allows you to force an access module. You can use it if " \
    "the correct access is not automatically detected. You should not "\
    "set this as a global option unless you really know what you are doing." )

#define STREAM_FILTER_TEXT N_("Stream filter module")
#define STREAM_FILTER_LONGTEXT N_( \
    "Stream filters are used to modify the stream that is being read. " )

#define DEMUX_TEXT N_("Demux module")
#define DEMUX_LONGTEXT N_( \
    "Demultiplexers are used to separate the \"elementary\" streams " \
    "(like audio and video streams). You can use it if " \
    "the correct demuxer is not automatically detected. You should not "\
    "set this as a global option unless you really know what you are doing." )

#define RT_PRIORITY_TEXT N_("Allow real-time priority")
#define RT_PRIORITY_LONGTEXT N_( \
    "Running VLC in real-time priority will allow for much more precise " \
    "scheduling and yield better, especially when streaming content. " \
    "It can however lock up your whole machine, or make it very very " \
    "slow. You should only activate this if you know what you're " \
    "doing.")

#define RT_OFFSET_TEXT N_("Adjust VLC priority")
#define RT_OFFSET_LONGTEXT N_( \
    "This option adds an offset (positive or negative) to VLC default " \
    "priorities. You can use it to tune VLC priority against other " \
    "programs, or against other VLC instances.")

#define USE_STREAM_IMMEDIATE N_("(Experimental) Don't do caching at the access level.")
#define USE_STREAM_IMMEDIATE_LONGTEXT N_( \
     "This option is useful if you want to lower the latency when " \
     "reading a stream")

#define PLUGIN_PATH_TEXT N_("Modules search path")
#define PLUGIN_PATH_LONGTEXT N_( \
    "Additional path for VLC to look for its modules. You can add " \
    "several paths by concatenating them using \" PATH_SEP \" as separator")

#define DATA_PATH_TEXT N_("Data search path")
#define DATA_PATH_LONGTEXT N_( \
    "Override the default data/share search path.")

#define VLM_CONF_TEXT N_("VLM configuration file")
#define VLM_CONF_LONGTEXT N_( \
    "Read a VLM configuration file as soon as VLM is started." )

#define PLUGINS_CACHE_TEXT N_("Use a plugins cache")
#define PLUGINS_CACHE_LONGTEXT N_( \
    "Use a plugins cache which will greatly improve the startup time of VLC.")

#define STATS_TEXT N_("Locally collect statistics")
#define STATS_LONGTEXT N_( \
     "Collect miscellaneous local statistics about the playing media.")

#define DAEMON_TEXT N_("Run as daemon process")
#define DAEMON_LONGTEXT N_( \
     "Runs VLC as a background daemon process.")

#define PIDFILE_TEXT N_("Write process id to file")
#define PIDFILE_LONGTEXT N_( \
       "Writes process id into specified file.")

#define FILE_LOG_TEXT N_( "Log to file" )
#define FILE_LOG_LONGTEXT N_( \
    "Log all VLC messages to a text file." )

#define SYSLOG_TEXT N_( "Log to syslog" )
#define SYSLOG_LONGTEXT N_( \
    "Log all VLC messages to syslog (UNIX systems)." )

#define ONEINSTANCE_TEXT N_("Allow only one running instance")
#if defined( WIN32 )
#define ONEINSTANCE_LONGTEXT N_( \
    "Allowing only one running instance of VLC can sometimes be useful, " \
    "for example if you associated VLC with some media types and you " \
    "don't want a new instance of VLC to be opened each time you " \
    "double-click on a file in the explorer. This option will allow you " \
    "to play the file with the already running instance or enqueue it.")
#elif defined( HAVE_DBUS )
#define ONEINSTANCE_LONGTEXT N_( \
    "Allowing only one running instance of VLC can sometimes be useful, " \
    "for example if you associated VLC with some media types and you " \
    "don't want a new instance of VLC to be opened each time you " \
    "open a file in your file manager. This option will allow you " \
    "to play the file with the already running instance or enqueue it. " \
    "This option requires the D-Bus session daemon to be active " \
    "and the running instance of VLC to use D-Bus control interface.")
#endif

#define STARTEDFROMFILE_TEXT N_("VLC is started from file association")
#define STARTEDFROMFILE_LONGTEXT N_( \
    "Tell VLC that it is being launched due to a file association in the OS" )

#define ONEINSTANCEWHENSTARTEDFROMFILE_TEXT N_( \
    "One instance when started from file")
#define ONEINSTANCEWHENSTARTEDFROMFILE_LONGTEXT N_( \
    "Allow only one running instance when started from file.")

#define HPRIORITY_TEXT N_("Increase the priority of the process")
#define HPRIORITY_LONGTEXT N_( \
    "Increasing the priority of the process will very likely improve your " \
    "playing experience as it allows VLC not to be disturbed by other " \
    "applications that could otherwise take too much processor time. " \
    "However be advised that in certain circumstances (bugs) VLC could take " \
    "all the processor time and render the whole system unresponsive which " \
    "might require a reboot of your machine.")

#define PLAYLISTENQUEUE_TEXT N_( \
    "Enqueue items to playlist when in one instance mode")
#define PLAYLISTENQUEUE_LONGTEXT N_( \
    "When using the one instance only option, enqueue items to playlist " \
    "and keep playing current item.")

/*****************************************************************************
 * Playlist
 ****************************************************************************/

// DEPRECATED
#define PLAYLIST_CAT_LONGTEXT N_( \
     "These options define the behavior of the playlist. Some " \
     "of them can be overridden in the playlist dialog box." )

#define PREPARSE_TEXT N_( "Automatically preparse files")
#define PREPARSE_LONGTEXT N_( \
    "Automatically preparse files added to the playlist " \
    "(to retrieve some metadata)." )

#define ALBUM_ART_TEXT N_( "Album art policy" )
#define ALBUM_ART_LONGTEXT N_( \
    "Choose how album art will be downloaded." )

static const int pi_albumart_values[] = { ALBUM_ART_WHEN_ASKED,
                                          ALBUM_ART_WHEN_PLAYED,
                                          ALBUM_ART_ALL };
static const char *const ppsz_albumart_descriptions[] =
    { N_("Manual download only"),
      N_("When track starts playing"),
      N_("As soon as track is added") };

#define SD_TEXT N_( "Services discovery modules")
#define SD_LONGTEXT N_( \
     "Specifies the services discovery modules to load, separated by " \
     "colons. Typical values are sap, hal, ..." )

#define RANDOM_TEXT N_("Play files randomly forever")
#define RANDOM_LONGTEXT N_( \
    "VLC will randomly play files in the playlist until interrupted.")

#define LOOP_TEXT N_("Repeat all")
#define LOOP_LONGTEXT N_( \
    "VLC will keep playing the playlist indefinitely." )

#define REPEAT_TEXT N_("Repeat current item")
#define REPEAT_LONGTEXT N_( \
    "VLC will keep playing the current playlist item." )

#define PAS_TEXT N_("Play and stop")
#define PAS_LONGTEXT N_( \
    "Stop the playlist after each played playlist item." )

#define PAE_TEXT N_("Play and exit")
#define PAE_LONGTEXT N_( \
    "Exit if there are no more items in the playlist." )

#define PAP_TEXT N_("Play and pause")
#define PAP_LONGTEXT N_( \
    "Pause each item in the playlist on the last frame." )

#define ML_TEXT N_("Use media library")
#define ML_LONGTEXT N_( \
    "The media library is automatically saved and reloaded each time you " \
    "start VLC." )

#define PLTREE_TEXT N_("Display playlist tree")
#define PLTREE_LONGTEXT N_( \
    "The playlist can use a tree to categorize some items, like the " \
    "contents of a directory." )


/*****************************************************************************
 * Hotkeys
 ****************************************************************************/

// DEPRECATED
#define HOTKEY_CAT_LONGTEXT N_( "These settings are the global VLC key " \
    "bindings, known as \"hotkeys\"." )

#define TOGGLE_FULLSCREEN_KEY_TEXT N_("Fullscreen")
#define TOGGLE_FULLSCREEN_KEY_LONGTEXT N_("Select the hotkey to use to swap fullscreen state.")
#define LEAVE_FULLSCREEN_KEY_TEXT N_("Leave fullscreen")
#define LEAVE_FULLSCREEN_KEY_LONGTEXT N_("Select the hotkey to use to leave fullscreen state.")
#define PLAY_PAUSE_KEY_TEXT N_("Play/Pause")
#define PLAY_PAUSE_KEY_LONGTEXT N_("Select the hotkey to use to swap paused state.")
#define PAUSE_KEY_TEXT N_("Pause only")
#define PAUSE_KEY_LONGTEXT N_("Select the hotkey to use to pause.")
#define PLAY_KEY_TEXT N_("Play only")
#define PLAY_KEY_LONGTEXT N_("Select the hotkey to use to play.")
#define FASTER_KEY_TEXT N_("Faster")
#define FASTER_KEY_LONGTEXT N_("Select the hotkey to use for fast forward playback.")
#define SLOWER_KEY_TEXT N_("Slower")
#define SLOWER_KEY_LONGTEXT N_("Select the hotkey to use for slow motion playback.")
#define RATE_NORMAL_KEY_TEXT N_("Normal rate")
#define RATE_NORMAL_KEY_LONGTEXT N_("Select the hotkey to set the playback rate back to normal.")
#define RATE_FASTER_FINE_KEY_TEXT N_("Faster (fine)")
#define RATE_FASTER_FINE_KEY_LONGTEXT N_("Select the hotkey to use for fast forward playback.")
#define RATE_SLOWER_FINE_KEY_TEXT N_("Slower (fine)")
#define RATE_SLOWER_FINE_KEY_LONGTEXT N_("Select the hotkey to use for slow motion playback.")
#define NEXT_KEY_TEXT N_("Next")
#define NEXT_KEY_LONGTEXT N_("Select the hotkey to use to skip to the next item in the playlist.")
#define PREV_KEY_TEXT N_("Previous")
#define PREV_KEY_LONGTEXT N_("Select the hotkey to use to skip to the previous item in the playlist.")
#define STOP_KEY_TEXT N_("Stop")
#define STOP_KEY_LONGTEXT N_("Select the hotkey to stop playback.")
#define POSITION_KEY_TEXT N_("Position")
#define POSITION_KEY_LONGTEXT N_("Select the hotkey to display the position.")

#define JBEXTRASHORT_KEY_TEXT N_("Very short backwards jump")
#define JBEXTRASHORT_KEY_LONGTEXT \
    N_("Select the hotkey to make a very short backwards jump.")
#define JBSHORT_KEY_TEXT N_("Short backwards jump")
#define JBSHORT_KEY_LONGTEXT \
    N_("Select the hotkey to make a short backwards jump.")
#define JBMEDIUM_KEY_TEXT N_("Medium backwards jump")
#define JBMEDIUM_KEY_LONGTEXT \
    N_("Select the hotkey to make a medium backwards jump.")
#define JBLONG_KEY_TEXT N_("Long backwards jump")
#define JBLONG_KEY_LONGTEXT \
    N_("Select the hotkey to make a long backwards jump.")

#define JFEXTRASHORT_KEY_TEXT N_("Very short forward jump")
#define JFEXTRASHORT_KEY_LONGTEXT \
    N_("Select the hotkey to make a very short forward jump.")
#define JFSHORT_KEY_TEXT N_("Short forward jump")
#define JFSHORT_KEY_LONGTEXT \
    N_("Select the hotkey to make a short forward jump.")
#define JFMEDIUM_KEY_TEXT N_("Medium forward jump")
#define JFMEDIUM_KEY_LONGTEXT \
    N_("Select the hotkey to make a medium forward jump.")
#define JFLONG_KEY_TEXT N_("Long forward jump")
#define JFLONG_KEY_LONGTEXT \
    N_("Select the hotkey to make a long forward jump.")
#define FRAME_NEXT_KEY_TEXT N_("Next frame")
#define FRAME_NEXT_KEY_LONGTEXT \
    N_("Select the hotkey to got to the next video frame.")

#define JIEXTRASHORT_TEXT N_("Very short jump length")
#define JIEXTRASHORT_LONGTEXT N_("Very short jump length, in seconds.")
#define JISHORT_TEXT N_("Short jump length")
#define JISHORT_LONGTEXT N_("Short jump length, in seconds.")
#define JIMEDIUM_TEXT N_("Medium jump length")
#define JIMEDIUM_LONGTEXT N_("Medium jump length, in seconds.")
#define JILONG_TEXT N_("Long jump length")
#define JILONG_LONGTEXT N_("Long jump length, in seconds.")

#define QUIT_KEY_TEXT N_("Quit")
#define QUIT_KEY_LONGTEXT N_("Select the hotkey to quit the application.")
#define NAV_UP_KEY_TEXT N_("Navigate up")
#define NAV_UP_KEY_LONGTEXT N_("Select the key to move the selector up in DVD menus.")
#define NAV_DOWN_KEY_TEXT N_("Navigate down")
#define NAV_DOWN_KEY_LONGTEXT N_("Select the key to move the selector down in DVD menus.")
#define NAV_LEFT_KEY_TEXT N_("Navigate left")
#define NAV_LEFT_KEY_LONGTEXT N_("Select the key to move the selector left in DVD menus.")
#define NAV_RIGHT_KEY_TEXT N_("Navigate right")
#define NAV_RIGHT_KEY_LONGTEXT N_("Select the key to move the selector right in DVD menus.")
#define NAV_ACTIVATE_KEY_TEXT N_("Activate")
#define NAV_ACTIVATE_KEY_LONGTEXT N_("Select the key to activate selected item in DVD menus.")
#define DISC_MENU_TEXT N_("Go to the DVD menu")
#define DISC_MENU_LONGTEXT N_("Select the key to take you to the DVD menu")
#define TITLE_PREV_TEXT N_("Select previous DVD title")
#define TITLE_PREV_LONGTEXT N_("Select the key to choose the previous title from the DVD")
#define TITLE_NEXT_TEXT N_("Select next DVD title")
#define TITLE_NEXT_LONGTEXT N_("Select the key to choose the next title from the DVD")
#define CHAPTER_PREV_TEXT N_("Select prev DVD chapter")
#define CHAPTER_PREV_LONGTEXT N_("Select the key to choose the previous chapter from the DVD")
#define CHAPTER_NEXT_TEXT N_("Select next DVD chapter")
#define CHAPTER_NEXT_LONGTEXT N_("Select the key to choose the next chapter from the DVD")
#define VOL_UP_KEY_TEXT N_("Volume up")
#define VOL_UP_KEY_LONGTEXT N_("Select the key to increase audio volume.")
#define VOL_DOWN_KEY_TEXT N_("Volume down")
#define VOL_DOWN_KEY_LONGTEXT N_("Select the key to decrease audio volume.")
#define VOL_MUTE_KEY_TEXT N_("Mute")
#define VOL_MUTE_KEY_LONGTEXT N_("Select the key to mute audio.")
#define SUBDELAY_UP_KEY_TEXT N_("Subtitle delay up")
#define SUBDELAY_UP_KEY_LONGTEXT N_("Select the key to increase the subtitle delay.")
#define SUBDELAY_DOWN_KEY_TEXT N_("Subtitle delay down")
#define SUBDELAY_DOWN_KEY_LONGTEXT N_("Select the key to decrease the subtitle delay.")
#define SUBPOS_UP_KEY_TEXT N_("Subtitle position up")
#define SUBPOS_UP_KEY_LONGTEXT N_("Select the key to move subtitles higher.")
#define SUBPOS_DOWN_KEY_TEXT N_("Subtitle position down")
#define SUBPOS_DOWN_KEY_LONGTEXT N_("Select the key to move subtitles lower.")
#define AUDIODELAY_UP_KEY_TEXT N_("Audio delay up")
#define AUDIODELAY_UP_KEY_LONGTEXT N_("Select the key to increase the audio delay.")
#define AUDIODELAY_DOWN_KEY_TEXT N_("Audio delay down")
#define AUDIODELAY_DOWN_KEY_LONGTEXT N_("Select the key to decrease the audio delay.")

#define ZOOM_QUARTER_KEY_TEXT N_("1:4 Quarter")
#define ZOOM_HALF_KEY_TEXT N_("1:2 Half")
#define ZOOM_ORIGINAL_KEY_TEXT N_("1:1 Original")
#define ZOOM_DOUBLE_KEY_TEXT N_("2:1 Double")

#define PLAY_BOOKMARK1_KEY_TEXT N_("Play playlist bookmark 1")
#define PLAY_BOOKMARK2_KEY_TEXT N_("Play playlist bookmark 2")
#define PLAY_BOOKMARK3_KEY_TEXT N_("Play playlist bookmark 3")
#define PLAY_BOOKMARK4_KEY_TEXT N_("Play playlist bookmark 4")
#define PLAY_BOOKMARK5_KEY_TEXT N_("Play playlist bookmark 5")
#define PLAY_BOOKMARK6_KEY_TEXT N_("Play playlist bookmark 6")
#define PLAY_BOOKMARK7_KEY_TEXT N_("Play playlist bookmark 7")
#define PLAY_BOOKMARK8_KEY_TEXT N_("Play playlist bookmark 8")
#define PLAY_BOOKMARK9_KEY_TEXT N_("Play playlist bookmark 9")
#define PLAY_BOOKMARK10_KEY_TEXT N_("Play playlist bookmark 10")
#define PLAY_BOOKMARK_KEY_LONGTEXT N_("Select the key to play this bookmark.")
#define SET_BOOKMARK1_KEY_TEXT N_("Set playlist bookmark 1")
#define SET_BOOKMARK2_KEY_TEXT N_("Set playlist bookmark 2")
#define SET_BOOKMARK3_KEY_TEXT N_("Set playlist bookmark 3")
#define SET_BOOKMARK4_KEY_TEXT N_("Set playlist bookmark 4")
#define SET_BOOKMARK5_KEY_TEXT N_("Set playlist bookmark 5")
#define SET_BOOKMARK6_KEY_TEXT N_("Set playlist bookmark 6")
#define SET_BOOKMARK7_KEY_TEXT N_("Set playlist bookmark 7")
#define SET_BOOKMARK8_KEY_TEXT N_("Set playlist bookmark 8")
#define SET_BOOKMARK9_KEY_TEXT N_("Set playlist bookmark 9")
#define SET_BOOKMARK10_KEY_TEXT N_("Set playlist bookmark 10")
#define SET_BOOKMARK_KEY_LONGTEXT N_("Select the key to set this playlist bookmark.")

#define BOOKMARK1_TEXT N_("Playlist bookmark 1")
#define BOOKMARK2_TEXT N_("Playlist bookmark 2")
#define BOOKMARK3_TEXT N_("Playlist bookmark 3")
#define BOOKMARK4_TEXT N_("Playlist bookmark 4")
#define BOOKMARK5_TEXT N_("Playlist bookmark 5")
#define BOOKMARK6_TEXT N_("Playlist bookmark 6")
#define BOOKMARK7_TEXT N_("Playlist bookmark 7")
#define BOOKMARK8_TEXT N_("Playlist bookmark 8")
#define BOOKMARK9_TEXT N_("Playlist bookmark 9")
#define BOOKMARK10_TEXT N_("Playlist bookmark 10")
#define BOOKMARK_LONGTEXT N_( \
      "This allows you to define playlist bookmarks.")

#define HISTORY_BACK_TEXT N_("Go back in browsing history")
#define HISTORY_BACK_LONGTEXT N_("Select the key to go back (to the previous media item) in the browsing history.")
#define HISTORY_FORWARD_TEXT N_("Go forward in browsing history")
#define HISTORY_FORWARD_LONGTEXT N_("Select the key to go forward (to the next media item) in the browsing history.")

#define AUDIO_TRACK_KEY_TEXT N_("Cycle audio track")
#define AUDIO_TRACK_KEY_LONGTEXT N_("Cycle through the available audio tracks(languages).")
#define SUBTITLE_TRACK_KEY_TEXT N_("Cycle subtitle track")
#define SUBTITLE_TRACK_KEY_LONGTEXT N_("Cycle through the available subtitle tracks.")
#define ASPECT_RATIO_KEY_TEXT N_("Cycle source aspect ratio")
#define ASPECT_RATIO_KEY_LONGTEXT N_("Cycle through a predefined list of source aspect ratios.")
#define CROP_KEY_TEXT N_("Cycle video crop")
#define CROP_KEY_LONGTEXT N_("Cycle through a predefined list of crop formats.")
#define TOGGLE_AUTOSCALE_KEY_TEXT N_("Toggle autoscaling")
#define TOGGLE_AUTOSCALE_KEY_LONGTEXT N_("Activate or deactivate autoscaling.")
#define SCALE_UP_KEY_TEXT N_("Increase scale factor")
#define SCALE_UP_KEY_LONGTEXT N_("Increase scale factor.")
#define SCALE_DOWN_KEY_TEXT N_("Decrease scale factor")
#define SCALE_DOWN_KEY_LONGTEXT N_("Decrease scale factor.")
#define DEINTERLACE_KEY_TEXT N_("Cycle deinterlace modes")
#define DEINTERLACE_KEY_LONGTEXT N_("Cycle through deinterlace modes.")
#define INTF_SHOW_KEY_TEXT N_("Show interface")
#define INTF_SHOW_KEY_LONGTEXT N_("Raise the interface above all other windows.")
#define INTF_HIDE_KEY_TEXT N_("Hide interface")
#define INTF_HIDE_KEY_LONGTEXT N_("Lower the interface below all other windows.")
#define SNAP_KEY_TEXT N_("Take video snapshot")
#define SNAP_KEY_LONGTEXT N_("Takes a video snapshot and writes it to disk.")

#define RECORD_KEY_TEXT N_("Record")
#define RECORD_KEY_LONGTEXT N_("Record access filter start/stop.")
#define DUMP_KEY_TEXT N_("Dump")
#define DUMP_KEY_LONGTEXT N_("Media dump access filter trigger.")

#define LOOP_KEY_TEXT N_("Normal/Repeat/Loop")
#define LOOP_KEY_LONGTEXT N_("Toggle Normal/Repeat/Loop playlist modes")

#define RANDOM_KEY_TEXT N_("Random")
#define RANDOM_KEY_LONGTEXT N_("Toggle random playlist playback")

#define ZOOM_KEY_TEXT N_("Zoom")
#define ZOOM_KEY_LONGTEXT N_("Zoom")

#define UNZOOM_KEY_TEXT N_("Un-Zoom")
#define UNZOOM_KEY_LONGTEXT N_("Un-Zoom")

#define CROP_TOP_KEY_TEXT N_("Crop one pixel from the top of the video")
#define CROP_TOP_KEY_LONGTEXT N_("Crop one pixel from the top of the video")
#define UNCROP_TOP_KEY_TEXT N_("Uncrop one pixel from the top of the video")
#define UNCROP_TOP_KEY_LONGTEXT N_("Uncrop one pixel from the top of the video")

#define CROP_LEFT_KEY_TEXT N_("Crop one pixel from the left of the video")
#define CROP_LEFT_KEY_LONGTEXT N_("Crop one pixel from the left of the video")
#define UNCROP_LEFT_KEY_TEXT N_("Uncrop one pixel from the left of the video")
#define UNCROP_LEFT_KEY_LONGTEXT N_("Uncrop one pixel from the left of the video")

#define CROP_BOTTOM_KEY_TEXT N_("Crop one pixel from the bottom of the video")
#define CROP_BOTTOM_KEY_LONGTEXT N_("Crop one pixel from the bottom of the video")
#define UNCROP_BOTTOM_KEY_TEXT N_("Uncrop one pixel from the bottom of the video")
#define UNCROP_BOTTOM_KEY_LONGTEXT N_("Uncrop one pixel from the bottom of the video")

#define CROP_RIGHT_KEY_TEXT N_("Crop one pixel from the right of the video")
#define CROP_RIGHT_KEY_LONGTEXT N_("Crop one pixel from the right of the video")
#define UNCROP_RIGHT_KEY_TEXT N_("Uncrop one pixel from the right of the video")
#define UNCROP_RIGHT_KEY_LONGTEXT N_("Uncrop one pixel from the right of the video")

#define WALLPAPER_KEY_TEXT N_("Toggle wallpaper mode in video output")
#define WALLPAPER_KEY_LONGTEXT N_( \
    "Toggle wallpaper mode in video output." )

#define MENU_ON_KEY_TEXT N_("Display OSD menu on top of video output")
#define MENU_ON_KEY_LONGTEXT N_("Display OSD menu on top of video output")
#define MENU_OFF_KEY_TEXT N_("Do not display OSD menu on video output")
#define MENU_OFF_KEY_LONGTEXT N_("Do not display OSD menu on top of video output")
#define MENU_RIGHT_KEY_TEXT N_("Highlight widget on the right")
#define MENU_RIGHT_KEY_LONGTEXT N_( \
        "Move OSD menu highlight to the widget on the right")
#define MENU_LEFT_KEY_TEXT N_("Highlight widget on the left")
#define MENU_LEFT_KEY_LONGTEXT N_( \
        "Move OSD menu highlight to the widget on the left")
#define MENU_UP_KEY_TEXT N_("Highlight widget on top")
#define MENU_UP_KEY_LONGTEXT N_( \
        "Move OSD menu highlight to the widget on top")
#define MENU_DOWN_KEY_TEXT N_("Highlight widget below")
#define MENU_DOWN_KEY_LONGTEXT N_( \
        "Move OSD menu highlight to the widget below")
#define MENU_SELECT_KEY_TEXT N_("Select current widget")
#define MENU_SELECT_KEY_LONGTEXT N_( \
        "Selecting current widget performs the associated action.")

#define AUDI_DEVICE_CYCLE_KEY_TEXT N_("Cycle through audio devices")
#define AUDI_DEVICE_CYCLE_KEY_LONGTEXT N_("Cycle through available audio devices")
const char vlc_usage[] = N_(
    "Usage: %s [options] [stream] ..."
    "\nYou can specify multiple streams on the commandline. They will be enqueued in the playlist."
    "\nThe first item specified will be played first."
    "\n"
    "\nOptions-styles:"
    "\n  --option  A global option that is set for the duration of the program."
    "\n   -option  A single letter version of a global --option."
    "\n   :option  An option that only applies to the stream directly before it"
    "\n            and that overrides previous settings."
    "\n"
    "\nStream MRL syntax:"
    "\n  [[access][/demux]://]URL[@[title][:chapter][-[title][:chapter]]] [:option=value ...]"
    "\n"
    "\n  Many of the global --options can also be used as MRL specific :options."
    "\n  Multiple :option=value pairs can be specified."
    "\n"
    "\nURL syntax:"
    "\n  [file://]filename              Plain media file"
    "\n  http://ip:port/file            HTTP URL"
    "\n  ftp://ip:port/file             FTP URL"
    "\n  mms://ip:port/file             MMS URL"
    "\n  screen://                      Screen capture"
    "\n  [dvd://][device][@raw_device]  DVD device"
    "\n  [vcd://][device]               VCD device"
    "\n  [cdda://][device]              Audio CD device"
    "\n  udp://[[<source address>]@[<bind address>][:<bind port>]]"
    "\n                                 UDP stream sent by a streaming server"
    "\n  vlc://pause:<seconds>          Special item to pause the playlist for a certain time"
    "\n  vlc://quit                     Special item to quit VLC"
    "\n");

/*
 * Quick usage guide for the configuration options:
 *
 * add_category_hint( N_(text), N_(longtext), b_advanced_option )
 * add_subcategory_hint( N_(text), N_(longtext), b_advanced_option )
 * add_usage_hint( N_(text), b_advanced_option )
 * add_string( option_name, value, p_callback, N_(text), N_(longtext),
               b_advanced_option )
 * add_file( option_name, psz_value, p_callback, N_(text), N_(longtext) )
 * add_module( option_name, psz_value, i_capability, p_callback,
 *             N_(text), N_(longtext) )
 * add_integer( option_name, i_value, p_callback, N_(text), N_(longtext),
                b_advanced_option )
 * add_bool( option_name, b_value, p_callback, N_(text), N_(longtext),
             b_advanced_option )
 */

vlc_module_begin ()
/* Audio options */
    set_category( CAT_AUDIO )
    set_subcategory( SUBCAT_AUDIO_GENERAL )
    add_category_hint( N_("Audio"), AOUT_CAT_LONGTEXT , false )

    add_bool( "audio", 1, NULL, AUDIO_TEXT, AUDIO_LONGTEXT, false )
        change_safe ()
    add_integer_with_range( "volume", AOUT_VOLUME_DEFAULT, AOUT_VOLUME_MIN,
                            AOUT_VOLUME_MAX, NULL, VOLUME_TEXT,
                            VOLUME_LONGTEXT, false )
    add_integer_with_range( "volume-step", AOUT_VOLUME_STEP, AOUT_VOLUME_MIN,
                            AOUT_VOLUME_MAX, NULL, VOLUME_STEP_TEXT,
                            VOLUME_STEP_LONGTEXT, true )
    add_integer( "aout-rate", -1, NULL, AOUT_RATE_TEXT,
                 AOUT_RATE_LONGTEXT, true )
#if HAVE_FPU && !defined( __APPLE__ )
    add_bool( "hq-resampling", 1, NULL, AOUT_RESAMP_TEXT,
              AOUT_RESAMP_LONGTEXT, true )
#endif
    add_bool( "spdif", 0, NULL, SPDIF_TEXT, SPDIF_LONGTEXT, false )
    add_integer( "force-dolby-surround", 0, NULL, FORCE_DOLBY_TEXT,
                 FORCE_DOLBY_LONGTEXT, false )
        change_integer_list( pi_force_dolby_values, ppsz_force_dolby_descriptions, NULL )
    add_integer( "audio-desync", 0, NULL, DESYNC_TEXT,
                 DESYNC_LONGTEXT, true )
        change_safe ()

    /* FIXME TODO create a subcat replay gain ? */
    add_string( "audio-replay-gain-mode", ppsz_replay_gain_mode[0], NULL, AUDIO_REPLAY_GAIN_MODE_TEXT,
                AUDIO_REPLAY_GAIN_MODE_LONGTEXT, false )
        change_string_list( ppsz_replay_gain_mode, ppsz_replay_gain_mode_text, 0 )
    add_float( "audio-replay-gain-preamp", 0.0, NULL,
               AUDIO_REPLAY_GAIN_PREAMP_TEXT, AUDIO_REPLAY_GAIN_PREAMP_LONGTEXT, false )
    add_float( "audio-replay-gain-default", -7.0, NULL,
               AUDIO_REPLAY_GAIN_DEFAULT_TEXT, AUDIO_REPLAY_GAIN_DEFAULT_LONGTEXT, false )
    add_bool( "audio-replay-gain-peak-protection", true, NULL,
              AUDIO_REPLAY_GAIN_PEAK_PROTECTION_TEXT, AUDIO_REPLAY_GAIN_PEAK_PROTECTION_LONGTEXT, true )

    add_bool( "audio-time-stretch", HAVE_FPU, NULL,
              AUDIO_TIME_STRETCH_TEXT, AUDIO_TIME_STRETCH_LONGTEXT, false )

    set_subcategory( SUBCAT_AUDIO_AOUT )
    add_module( "aout", "audio output", NULL, NULL, AOUT_TEXT, AOUT_LONGTEXT,
                true )
        change_short('A')
    set_subcategory( SUBCAT_AUDIO_AFILTER )
    add_module_list_cat( "audio-filter", SUBCAT_AUDIO_AFILTER, 0,
                         NULL, AUDIO_FILTER_TEXT,
                         AUDIO_FILTER_LONGTEXT, false )
    set_subcategory( SUBCAT_AUDIO_VISUAL )
    add_module( "audio-visual", "visualization2",NULL, NULL,AUDIO_VISUAL_TEXT,
                AUDIO_VISUAL_LONGTEXT, false )

/* Video options */
    set_category( CAT_VIDEO )
    set_subcategory( SUBCAT_VIDEO_GENERAL )
    add_category_hint( N_("Video"), VOUT_CAT_LONGTEXT , false )

    add_bool( "video", 1, NULL, VIDEO_TEXT, VIDEO_LONGTEXT, true )
        change_safe ()
    add_bool( "grayscale", 0, NULL, GRAYSCALE_TEXT,
              GRAYSCALE_LONGTEXT, true )
#if defined (HAVE_MAEMO)
# define FULLSCREEN_DEFAULT true
#else
# define FULLSCREEN_DEFAULT false
#endif
    add_bool( "fullscreen", FULLSCREEN_DEFAULT, NULL, FULLSCREEN_TEXT,
              FULLSCREEN_LONGTEXT, false )
        change_short('f')
        change_safe ()
    add_bool( "embedded-video", 1, NULL, EMBEDDED_TEXT, EMBEDDED_LONGTEXT,
              true )
#ifdef __APPLE__
       add_deprecated_alias( "macosx-embedded" ) /*deprecated since 0.9.0 */
#endif
    add_string( "x11-display", NULL, NULL,
                DISPLAY_TEXT, DISPLAY_LONGTEXT, true )
        add_deprecated_alias( "xvideo-display" ) /* deprecated since 1.1.0 */
        add_deprecated_alias( "glx-display" )
    add_bool( "xlib", true, NULL, "", "", true )
        change_private ()
    add_bool( "drop-late-frames", 1, NULL, DROP_LATE_FRAMES_TEXT,
              DROP_LATE_FRAMES_LONGTEXT, true )
    /* Used in vout_synchro */
    add_bool( "skip-frames", 1, NULL, SKIP_FRAMES_TEXT,
              SKIP_FRAMES_LONGTEXT, true )
    add_bool( "quiet-synchro", 0, NULL, QUIET_SYNCHRO_TEXT,
              QUIET_SYNCHRO_LONGTEXT, true )
    add_bool( "keyboard-events", true, NULL, KEYBOARD_EVENTS_TEXT,
              KEYBOARD_EVENTS_LONGTEXT, true )
    add_bool( "mouse-events", true, NULL, MOUSE_EVENTS_TEXT,
              MOUSE_EVENTS_LONGTEXT, true )
    add_obsolete_integer( "vout-event" ) /* deprecated since 1.1.0 */
    add_obsolete_integer( "x11-event" ) /* renamed since 1.0.0 */
#ifndef __APPLE__
    add_bool( "overlay", 1, NULL, OVERLAY_TEXT, OVERLAY_LONGTEXT, false )
        change_safe()
#endif
    add_bool( "video-on-top", 0, NULL, VIDEO_ON_TOP_TEXT,
              VIDEO_ON_TOP_LONGTEXT, false )
    add_bool( "video-wallpaper", false, NULL, WALLPAPER_TEXT,
              WALLPAPER_LONGTEXT, false )
#ifdef WIN32
        add_deprecated_alias( "directx-wallpaper" )
#endif
    add_bool( "disable-screensaver", true, NULL, SS_TEXT, SS_LONGTEXT,
              true )

    add_bool( "video-title-show", 1, NULL, VIDEO_TITLE_SHOW_TEXT,
              VIDEO_TITLE_SHOW_LONGTEXT, false )
        change_safe()
    add_integer( "video-title-timeout", 5000, NULL, VIDEO_TITLE_TIMEOUT_TEXT,
                 VIDEO_TITLE_TIMEOUT_LONGTEXT, false )
        change_safe()
    add_integer( "video-title-position", 8, NULL, VIDEO_TITLE_POSITION_TEXT,
                 VIDEO_TITLE_POSITION_LONGTEXT, false )
        change_safe()
        change_integer_list( pi_pos_values, ppsz_pos_descriptions, NULL )
    // autohide after 1.5s
    add_integer( "mouse-hide-timeout", 1500, NULL, MOUSE_HIDE_TIMEOUT_TEXT,
                 MOUSE_HIDE_TIMEOUT_LONGTEXT, false )
    set_section( N_("Snapshot") , NULL )
    add_directory( "snapshot-path", NULL, NULL, SNAP_PATH_TEXT,
                   SNAP_PATH_LONGTEXT, false )
    add_string( "snapshot-prefix", "vlcsnap-", NULL, SNAP_PREFIX_TEXT,
                   SNAP_PREFIX_LONGTEXT, false )
    add_string( "snapshot-format", "png", NULL, SNAP_FORMAT_TEXT,
                   SNAP_FORMAT_LONGTEXT, false )
        change_string_list( ppsz_snap_formats, NULL, 0 )
    add_bool( "snapshot-preview", true, NULL, SNAP_PREVIEW_TEXT,
              SNAP_PREVIEW_LONGTEXT, false )
    add_bool( "snapshot-sequential", false, NULL, SNAP_SEQUENTIAL_TEXT,
              SNAP_SEQUENTIAL_LONGTEXT, false )
    add_integer( "snapshot-width", -1, NULL, SNAP_WIDTH_TEXT,
                 SNAP_WIDTH_LONGTEXT, true )
    add_integer( "snapshot-height", -1, NULL, SNAP_HEIGHT_TEXT,
                 SNAP_HEIGHT_LONGTEXT, true )

    set_section( N_("Window properties" ), NULL )
    add_integer( "width", -1, NULL, WIDTH_TEXT, WIDTH_LONGTEXT, true )
        change_safe ()
    add_integer( "height", -1, NULL, HEIGHT_TEXT, HEIGHT_LONGTEXT, true )
        change_safe ()
    add_integer( "video-x", 0, NULL, VIDEOX_TEXT, VIDEOX_LONGTEXT, true )
        change_safe ()
    add_integer( "video-y", 0, NULL, VIDEOY_TEXT, VIDEOY_LONGTEXT, true )
        change_safe ()
    add_string( "crop", NULL, NULL, CROP_TEXT, CROP_LONGTEXT, false )
        change_safe ()
    add_string( "custom-crop-ratios", NULL, NULL, CUSTOM_CROP_RATIOS_TEXT,
                CUSTOM_CROP_RATIOS_LONGTEXT, false )
    add_string( "aspect-ratio", NULL, NULL,
                ASPECT_RATIO_TEXT, ASPECT_RATIO_LONGTEXT, false )
        change_safe ()
    add_bool( "autoscale", true, NULL, AUTOSCALE_TEXT, AUTOSCALE_LONGTEXT, false )
        change_safe ()
    add_float( "scale", 1.0, NULL, SCALEFACTOR_TEXT, SCALEFACTOR_LONGTEXT, false )
        change_safe ()
    add_string( "monitor-par", NULL, NULL,
                MASPECT_RATIO_TEXT, MASPECT_RATIO_LONGTEXT, true )
    add_string( "custom-aspect-ratios", NULL, NULL, CUSTOM_ASPECT_RATIOS_TEXT,
                CUSTOM_ASPECT_RATIOS_LONGTEXT, false )
    add_bool( "hdtv-fix", 1, NULL, HDTV_FIX_TEXT, HDTV_FIX_LONGTEXT, true )
    add_bool( "video-deco", 1, NULL, VIDEO_DECO_TEXT,
              VIDEO_DECO_LONGTEXT, true )
    add_string( "video-title", NULL, NULL, VIDEO_TITLE_TEXT,
                 VIDEO_TITLE_LONGTEXT, true )
    add_integer( "align", 0, NULL, ALIGN_TEXT, ALIGN_LONGTEXT, true )
        change_integer_list( pi_align_values, ppsz_align_descriptions, NULL )
    add_float( "zoom", 1, NULL, ZOOM_TEXT, ZOOM_LONGTEXT, true )
    add_integer( "deinterlace", 0, NULL,
                 DEINTERLACE_TEXT, DEINTERLACE_LONGTEXT, false )
        change_integer_list( pi_deinterlace, ppsz_deinterlace_text, 0 )
        change_safe()
    add_string( "deinterlace-mode", "blend", NULL,
                DEINTERLACE_MODE_TEXT, DEINTERLACE_MODE_LONGTEXT, false )
        change_string_list( ppsz_deinterlace_mode, ppsz_deinterlace_mode_text, 0 )
        change_safe()

    set_subcategory( SUBCAT_VIDEO_VOUT )
    add_module( "vout", "vout display", NULL, NULL, VOUT_TEXT, VOUT_LONGTEXT,
                true )
        change_short('V')

    set_subcategory( SUBCAT_VIDEO_VFILTER )
    add_module_list_cat( "video-filter", SUBCAT_VIDEO_VFILTER, NULL, NULL,
                VIDEO_FILTER_TEXT, VIDEO_FILTER_LONGTEXT, false )
        add_deprecated_alias( "filter" ) /*deprecated since 0.8.2 */
    add_module_list_cat( "vout-filter", SUBCAT_VIDEO_VFILTER, NULL, NULL,
                        VOUT_FILTER_TEXT, VOUT_FILTER_LONGTEXT, false )
#if 0
    add_string( "pixel-ratio", "1", NULL, PIXEL_RATIO_TEXT, PIXEL_RATIO_TEXT )
#endif

/* Subpictures options */
    set_subcategory( SUBCAT_VIDEO_SUBPIC )
    set_section( N_("On Screen Display") , NULL )
    add_category_hint( N_("Subpictures"), SUB_CAT_LONGTEXT , false )

    add_bool( "spu", 1, NULL, SPU_TEXT, SPU_LONGTEXT, true )
        change_safe ()
    add_bool( "osd", 1, NULL, OSD_TEXT, OSD_LONGTEXT, false )
    add_module( "text-renderer", "text renderer", NULL, NULL, TEXTRENDERER_TEXT,
                TEXTRENDERER_LONGTEXT, true )

    set_section( N_("Subtitles") , NULL )
    add_file( "sub-file", NULL, NULL, SUB_FILE_TEXT,
              SUB_FILE_LONGTEXT, false )
        change_safe()
    add_bool( "sub-autodetect-file", true, NULL,
                 SUB_AUTO_TEXT, SUB_AUTO_LONGTEXT, false )
    add_integer( "sub-autodetect-fuzzy", 3, NULL,
                 SUB_FUZZY_TEXT, SUB_FUZZY_LONGTEXT, true )
#ifdef WIN32
#   define SUB_PATH ".\\subtitles"
#else
#   define SUB_PATH "./Subtitles, ./subtitles"
#endif
    add_string( "sub-autodetect-path", SUB_PATH, NULL,
                 SUB_PATH_TEXT, SUB_PATH_LONGTEXT, true )
    add_integer( "sub-margin", 0, NULL, SUB_MARGIN_TEXT,
                 SUB_MARGIN_LONGTEXT, true )
        add_deprecated_alias( "spu-margin" ) /*Deprecated since 0.8.2 */
    set_section( N_( "Overlays" ) , NULL )
    add_module_list_cat( "sub-filter", SUBCAT_VIDEO_SUBPIC, NULL, NULL,
                SUB_FILTER_TEXT, SUB_FILTER_LONGTEXT, false )

/* Input options */
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_GENERAL )

    set_section( N_( "Track settings" ), NULL )
    add_integer( "program", 0, NULL,
                 INPUT_PROGRAM_TEXT, INPUT_PROGRAM_LONGTEXT, true )
        change_safe ()
    add_string( "programs", "", NULL,
                INPUT_PROGRAMS_TEXT, INPUT_PROGRAMS_LONGTEXT, true )
        change_safe ()
    add_integer( "audio-track", -1, NULL,
                 INPUT_AUDIOTRACK_TEXT, INPUT_AUDIOTRACK_LONGTEXT, true )
        change_safe ()
        add_deprecated_alias( "audio-channel" ) /*deprecated since 0.8.2 */
    add_integer( "sub-track", -1, NULL,
                 INPUT_SUBTRACK_TEXT, INPUT_SUBTRACK_LONGTEXT, true )
        change_safe ()
        add_deprecated_alias("spu-channel" ) /*deprecated since 0.8.2*/
    add_string( "audio-language", "", NULL,
                 INPUT_AUDIOTRACK_LANG_TEXT, INPUT_AUDIOTRACK_LANG_LONGTEXT,
                  false )
        change_safe ()
    add_string( "sub-language", "", NULL,
                 INPUT_SUBTRACK_LANG_TEXT, INPUT_SUBTRACK_LANG_LONGTEXT,
                  false )
        change_safe ()
    add_integer( "audio-track-id", -1, NULL, INPUT_AUDIOTRACK_ID_TEXT,
                 INPUT_AUDIOTRACK_ID_LONGTEXT, true )
        change_safe ()
    add_integer( "sub-track-id", -1, NULL,
                 INPUT_SUBTRACK_ID_TEXT, INPUT_SUBTRACK_ID_LONGTEXT, true )
        change_safe ()

    set_section( N_( "Playback control" ) , NULL)
    add_integer( "input-repeat", 0, NULL,
                 INPUT_REPEAT_TEXT, INPUT_REPEAT_LONGTEXT, false )
        change_safe ()
    add_float( "start-time", 0, NULL,
               START_TIME_TEXT, START_TIME_LONGTEXT, true )
        change_safe ()
    add_float( "stop-time", 0, NULL,
               STOP_TIME_TEXT, STOP_TIME_LONGTEXT, true )
        change_safe ()
    add_float( "run-time", 0, NULL,
               RUN_TIME_TEXT, RUN_TIME_LONGTEXT, true )
        change_safe ()
    add_bool( "input-fast-seek", false, NULL,
              INPUT_FAST_SEEK_TEXT, INPUT_FAST_SEEK_LONGTEXT, false )
        change_safe ()
    add_float( "rate", 1., NULL,
               INPUT_RATE_TEXT, INPUT_RATE_LONGTEXT, false )

    add_string( "input-list", NULL, NULL,
                 INPUT_LIST_TEXT, INPUT_LIST_LONGTEXT, true )
    add_string( "input-slave", NULL, NULL,
                 INPUT_SLAVE_TEXT, INPUT_SLAVE_LONGTEXT, true )

    add_string( "bookmarks", NULL, NULL,
                 BOOKMARKS_TEXT, BOOKMARKS_LONGTEXT, true )

    set_section( N_( "Default devices") , NULL )

    add_file( "dvd", DVD_DEVICE, NULL, DVD_DEV_TEXT, DVD_DEV_LONGTEXT,
              false )
    add_file( "vcd", VCD_DEVICE, NULL, VCD_DEV_TEXT, VCD_DEV_LONGTEXT,
              false )
    add_file( "cd-audio", CDAUDIO_DEVICE, NULL, CDAUDIO_DEV_TEXT,
              CDAUDIO_DEV_LONGTEXT, false )

    set_section( N_( "Network settings" ), NULL )

    add_integer( "server-port", 1234, NULL,
                 SERVER_PORT_TEXT, SERVER_PORT_LONGTEXT, false )
    add_integer( "mtu", MTU_DEFAULT, NULL, MTU_TEXT, MTU_LONGTEXT, true )
    add_bool( "ipv6", 0, NULL, IPV6_TEXT, IPV6_LONGTEXT, false )
        change_short('6')
    add_bool( "ipv4", 0, NULL, IPV4_TEXT, IPV4_LONGTEXT, false )
        change_short('4')
    add_integer( "ipv4-timeout", 5 * 1000, NULL, TIMEOUT_TEXT,
                 TIMEOUT_LONGTEXT, true )

    set_section( N_( "Socks proxy") , NULL )
    add_string( "socks", NULL, NULL,
                 SOCKS_SERVER_TEXT, SOCKS_SERVER_LONGTEXT, true )
    add_string( "socks-user", NULL, NULL,
                 SOCKS_USER_TEXT, SOCKS_USER_LONGTEXT, true )
    add_string( "socks-pwd", NULL, NULL,
                 SOCKS_PASS_TEXT, SOCKS_PASS_LONGTEXT, true )


    set_section( N_("Metadata" ) , NULL )
    add_string( "meta-title", NULL, NULL, META_TITLE_TEXT,
                META_TITLE_LONGTEXT, true )
        change_safe()
    add_string( "meta-author", NULL, NULL, META_AUTHOR_TEXT,
                META_AUTHOR_LONGTEXT, true )
        change_safe()
    add_string( "meta-artist", NULL, NULL, META_ARTIST_TEXT,
                META_ARTIST_LONGTEXT, true )
        change_safe()
    add_string( "meta-genre", NULL, NULL, META_GENRE_TEXT,
                META_GENRE_LONGTEXT, true )
        change_safe()
    add_string( "meta-copyright", NULL, NULL, META_CPYR_TEXT,
                META_CPYR_LONGTEXT, true )
        change_safe()
    add_string( "meta-description", NULL, NULL, META_DESCR_TEXT,
                META_DESCR_LONGTEXT, true )
        change_safe()
    add_string( "meta-date", NULL, NULL, META_DATE_TEXT,
                META_DATE_LONGTEXT, true )
        change_safe()
    add_string( "meta-url", NULL, NULL, META_URL_TEXT,
                META_URL_LONGTEXT, true )
        change_safe()

    set_section( N_( "Advanced" ), NULL )

    add_integer( "cr-average", 40, NULL, CR_AVERAGE_TEXT,
                 CR_AVERAGE_LONGTEXT, true )
    add_integer( "clock-synchro", -1, NULL, CLOCK_SYNCHRO_TEXT,
                 CLOCK_SYNCHRO_LONGTEXT, true )
        change_integer_list( pi_clock_values, ppsz_clock_descriptions, NULL )
    add_integer( "clock-jitter", 5 * CLOCK_FREQ/1000, NULL, CLOCK_JITTER_TEXT,
              CLOCK_JITTER_LONGTEXT, true )
        change_safe()

    add_bool( "network-synchronisation", false, NULL, NETSYNC_TEXT,
              NETSYNC_LONGTEXT, true )

    add_string( "input-record-path", NULL, NULL, INPUT_RECORD_PATH_TEXT,
                INPUT_RECORD_PATH_LONGTEXT, true )
    add_bool( "input-record-native", true, NULL, INPUT_RECORD_NATIVE_TEXT,
              INPUT_RECORD_NATIVE_LONGTEXT, true )

    add_string( "input-timeshift-path", NULL, NULL, INPUT_TIMESHIFT_PATH_TEXT,
                INPUT_TIMESHIFT_PATH_LONGTEXT, true )
    add_integer( "input-timeshift-granularity", -1, NULL, INPUT_TIMESHIFT_GRANULARITY_TEXT,
                 INPUT_TIMESHIFT_GRANULARITY_LONGTEXT, true )

/* Decoder options */
    add_category_hint( N_("Decoders"), CODEC_CAT_LONGTEXT , true )
    add_string( "codec", NULL, NULL, CODEC_TEXT,
                CODEC_LONGTEXT, true )
    add_string( "encoder",  NULL, NULL, ENCODER_TEXT,
                ENCODER_LONGTEXT, true )

    set_subcategory( SUBCAT_INPUT_ACCESS )
    add_category_hint( N_("Input"), INPUT_CAT_LONGTEXT , false )
    add_module( "access", "access", NULL, NULL, ACCESS_TEXT,
                ACCESS_LONGTEXT, true )

    set_subcategory( SUBCAT_INPUT_DEMUX )
    add_module( "demux", "demux", NULL, NULL, DEMUX_TEXT,
                DEMUX_LONGTEXT, true )
    set_subcategory( SUBCAT_INPUT_VCODEC )
    set_subcategory( SUBCAT_INPUT_ACODEC )
    set_subcategory( SUBCAT_INPUT_SCODEC )
    add_bool( "prefer-system-codecs", false, NULL, SYSTEM_CODEC_TEXT,
                                SYSTEM_CODEC_LONGTEXT, false )

    set_subcategory( SUBCAT_INPUT_STREAM_FILTER )
    add_module_list_cat( "stream-filter", SUBCAT_INPUT_STREAM_FILTER, NULL, NULL,
                STREAM_FILTER_TEXT, STREAM_FILTER_LONGTEXT, false )


/* Stream output options */
    set_category( CAT_SOUT )
    set_subcategory( SUBCAT_SOUT_GENERAL )
    add_category_hint( N_("Stream output"), SOUT_CAT_LONGTEXT , true )

    add_string( "sout", NULL, NULL, SOUT_TEXT, SOUT_LONGTEXT, true )
    add_bool( "sout-display", false, NULL, SOUT_DISPLAY_TEXT,
                                SOUT_DISPLAY_LONGTEXT, true )
    add_bool( "sout-keep", false, NULL, SOUT_KEEP_TEXT,
                                SOUT_KEEP_LONGTEXT, true )
    add_bool( "sout-all", 0, NULL, SOUT_ALL_TEXT,
                                SOUT_ALL_LONGTEXT, true )
    add_bool( "sout-audio", 1, NULL, SOUT_AUDIO_TEXT,
                                SOUT_AUDIO_LONGTEXT, true )
    add_bool( "sout-video", 1, NULL, SOUT_VIDEO_TEXT,
                                SOUT_VIDEO_LONGTEXT, true )
    add_bool( "sout-spu", 1, NULL, SOUT_SPU_TEXT,
                                SOUT_SPU_LONGTEXT, true )
    add_integer( "sout-mux-caching", 1500, NULL, SOUT_MUX_CACHING_TEXT,
                                SOUT_MUX_CACHING_LONGTEXT, true )

    set_section( N_("VLM"), NULL )
    add_string( "vlm-conf", NULL, NULL, VLM_CONF_TEXT,
                    VLM_CONF_LONGTEXT, true )



    set_subcategory( SUBCAT_SOUT_STREAM )
    set_subcategory( SUBCAT_SOUT_MUX )
    add_module( "mux", "sout mux", NULL, NULL, MUX_TEXT,
                                MUX_LONGTEXT, true )
    set_subcategory( SUBCAT_SOUT_ACO )
    add_module( "access_output", "sout access", NULL, NULL,
                ACCESS_OUTPUT_TEXT, ACCESS_OUTPUT_LONGTEXT, true )
    add_integer( "ttl", -1, NULL, TTL_TEXT, TTL_LONGTEXT, true )
    add_string( "miface", NULL, NULL, MIFACE_TEXT, MIFACE_LONGTEXT, true )
    add_string( "miface-addr", NULL, NULL, MIFACE_ADDR_TEXT, MIFACE_ADDR_LONGTEXT, true )
    add_integer( "dscp", 0, NULL, DSCP_TEXT, DSCP_LONGTEXT, true )

    set_subcategory( SUBCAT_SOUT_PACKETIZER )
    add_module( "packetizer","packetizer", NULL, NULL,
                PACKETIZER_TEXT, PACKETIZER_LONGTEXT, true )

    set_subcategory( SUBCAT_SOUT_SAP )
    add_obsolete_bool( "sap-flow-control" )
    add_integer( "sap-interval", 5, NULL, ANN_SAPINTV_TEXT,
                               ANN_SAPINTV_LONGTEXT, true )

    set_subcategory( SUBCAT_SOUT_VOD )

/* CPU options */
    set_category( CAT_ADVANCED )
    set_subcategory( SUBCAT_ADVANCED_CPU )
    add_category_hint( N_("CPU"), CPU_CAT_LONGTEXT, true )
    add_obsolete_bool( "fpu" )
#if defined( __i386__ ) || defined( __x86_64__ )
    add_bool( "mmx", 1, NULL, MMX_TEXT, MMX_LONGTEXT, true )
        change_need_restart ()
    add_bool( "3dn", 1, NULL, THREE_DN_TEXT, THREE_DN_LONGTEXT, true )
        change_need_restart ()
    add_bool( "mmxext", 1, NULL, MMXEXT_TEXT, MMXEXT_LONGTEXT, true )
        change_need_restart ()
    add_bool( "sse", 1, NULL, SSE_TEXT, SSE_LONGTEXT, true )
        change_need_restart ()
    add_bool( "sse2", 1, NULL, SSE2_TEXT, SSE2_LONGTEXT, true )
        change_need_restart ()
    add_bool( "sse3", 1, NULL, SSE3_TEXT, SSE3_LONGTEXT, true )
        change_need_restart ()
    add_bool( "ssse3", 1, NULL, SSSE3_TEXT, SSSE3_LONGTEXT, true )
        change_need_restart ()
    add_bool( "sse41", 1, NULL, SSE4_1_TEXT, SSE4_1_LONGTEXT, true )
        change_need_restart ()
    add_bool( "sse42", 1, NULL, SSE4_2_TEXT, SSE4_2_LONGTEXT, true )
        change_need_restart ()
#endif
#if defined( __powerpc__ ) || defined( __ppc__ ) || defined( __ppc64__ )
    add_bool( "altivec", 1, NULL, ALTIVEC_TEXT, ALTIVEC_LONGTEXT, true )
        change_need_restart ()
#endif

/* Misc options */
    set_subcategory( SUBCAT_ADVANCED_MISC )
    set_section( N_("Special modules"), NULL )
    add_category_hint( N_("Miscellaneous"), MISC_CAT_LONGTEXT, true )
    add_module( "memcpy", "memcpy", NULL, NULL, MEMCPY_TEXT,
                MEMCPY_LONGTEXT, true )
        change_need_restart ()

    set_section( N_("Plugins" ), NULL )
    add_bool( "plugins-cache", true, NULL, PLUGINS_CACHE_TEXT,
              PLUGINS_CACHE_LONGTEXT, true )
        change_need_restart ()
    add_directory( "plugin-path", NULL, NULL, PLUGIN_PATH_TEXT,
                   PLUGIN_PATH_LONGTEXT, true )
        change_need_restart ()
    add_directory( "data-path", NULL, NULL, DATA_PATH_TEXT,
                   DATA_PATH_LONGTEXT, true )
        change_need_restart ()

    set_section( N_("Performance options"), NULL )
    add_obsolete_bool( "minimize-threads" )

    add_obsolete_bool( "use-stream-immediate" )

    add_obsolete_bool( "auto-adjust-pts-delay" )

#if !defined(__APPLE__) && !defined(SYS_BEOS) && defined(LIBVLC_USE_PTHREAD)
    add_bool( "rt-priority", false, NULL, RT_PRIORITY_TEXT,
              RT_PRIORITY_LONGTEXT, true )
        change_need_restart ()
#endif

#if !defined(SYS_BEOS) && defined(LIBVLC_USE_PTHREAD)
    add_integer( "rt-offset", 0, NULL, RT_OFFSET_TEXT,
                 RT_OFFSET_LONGTEXT, true )
        change_need_restart ()
#endif

#if defined(HAVE_DBUS)
    add_bool( "inhibit", 1, NULL, INHIBIT_TEXT,
              INHIBIT_LONGTEXT, true )
#endif

#if defined(WIN32) || defined(HAVE_DBUS)
    add_bool( "one-instance", 0, NULL, ONEINSTANCE_TEXT,
              ONEINSTANCE_LONGTEXT, true )
    add_bool( "started-from-file", 0, NULL, STARTEDFROMFILE_TEXT,
              STARTEDFROMFILE_LONGTEXT, true )
        change_volatile ()
    add_bool( "one-instance-when-started-from-file", 1, NULL,
              ONEINSTANCEWHENSTARTEDFROMFILE_TEXT,
              ONEINSTANCEWHENSTARTEDFROMFILE_LONGTEXT, true )
    add_bool( "playlist-enqueue", 0, NULL, PLAYLISTENQUEUE_TEXT,
              PLAYLISTENQUEUE_LONGTEXT, true )
#endif

#if defined(WIN32)
    add_bool( "high-priority", 0, NULL, HPRIORITY_TEXT,
              HPRIORITY_LONGTEXT, false )
        change_need_restart ()
#endif

/* Playlist options */
    set_category( CAT_PLAYLIST )
    set_subcategory( SUBCAT_PLAYLIST_GENERAL )
    add_category_hint( N_("Playlist"), PLAYLIST_CAT_LONGTEXT , false )
    add_bool( "random", 0, NULL, RANDOM_TEXT, RANDOM_LONGTEXT, false )
        change_short('Z')
        change_safe()
    add_bool( "loop", 0, NULL, LOOP_TEXT, LOOP_LONGTEXT, false )
        change_short('L')
        change_safe()
    add_bool( "repeat", 0, NULL, REPEAT_TEXT, REPEAT_LONGTEXT, false )
        change_short('R')
        change_safe()
    add_bool( "play-and-exit", 0, NULL, PAE_TEXT, PAE_LONGTEXT, false )
    add_bool( "play-and-stop", 0, NULL, PAS_TEXT, PAS_LONGTEXT, false )
        change_safe()
    add_bool( "play-and-pause", 0, NULL, PAP_TEXT, PAP_LONGTEXT, true )
        change_safe()
    add_bool( "media-library", 1, NULL, ML_TEXT, ML_LONGTEXT, false )
    add_bool( "playlist-tree", 0, NULL, PLTREE_TEXT, PLTREE_LONGTEXT, false )

    add_string( "open", "", NULL, OPEN_TEXT, OPEN_LONGTEXT, false )
        change_need_restart ()

    add_bool( "auto-preparse", true, NULL, PREPARSE_TEXT,
              PREPARSE_LONGTEXT, false )

    add_integer( "album-art", ALBUM_ART_WHEN_ASKED, NULL, ALBUM_ART_TEXT,
                 ALBUM_ART_LONGTEXT, false )
        change_integer_list( pi_albumart_values,
                             ppsz_albumart_descriptions, 0 )

    set_subcategory( SUBCAT_PLAYLIST_SD )
    add_module_list_cat( "services-discovery", SUBCAT_PLAYLIST_SD, NULL,
                          NULL, SD_TEXT, SD_LONGTEXT, false )
        change_short('S')
        change_need_restart ()

/* Interface options */
    set_category( CAT_INTERFACE )
    set_subcategory( SUBCAT_INTERFACE_GENERAL )
    add_integer( "verbose", 0, NULL, VERBOSE_TEXT, VERBOSE_LONGTEXT,
                 false )
        change_short('v')
    add_string( "verbose-objects", 0, NULL, VERBOSE_OBJECTS_TEXT, VERBOSE_OBJECTS_LONGTEXT,
                 false )
    add_bool( "quiet", 0, NULL, QUIET_TEXT, QUIET_LONGTEXT, true )
        change_short('q')

#if !defined(WIN32)
    add_bool( "daemon", 0, NULL, DAEMON_TEXT, DAEMON_LONGTEXT, true )
        change_short('d')
        change_need_restart ()

    add_string( "pidfile", NULL, NULL, PIDFILE_TEXT, PIDFILE_LONGTEXT,
                                       false )
        change_need_restart ()
#endif

    add_bool( "file-logging", false, NULL, FILE_LOG_TEXT, FILE_LOG_LONGTEXT,
              true )
        change_need_restart ()
#ifdef HAVE_SYSLOG_H
    add_bool ( "syslog", false, NULL, SYSLOG_TEXT, SYSLOG_LONGTEXT,
               true )
        change_need_restart ()
#endif

#if defined (WIN32) || defined (__APPLE__)
    add_string( "language", "auto", NULL, LANGUAGE_TEXT, LANGUAGE_LONGTEXT,
                false )
        change_string_list( ppsz_language, ppsz_language_text, 0 )
        change_need_restart ()
#endif

    add_bool( "color", true, NULL, COLOR_TEXT, COLOR_LONGTEXT, true )
    add_bool( "advanced", false, NULL, ADVANCED_TEXT, ADVANCED_LONGTEXT,
                    false )
        change_need_restart ()
    add_bool( "interact", true, NULL, INTERACTION_TEXT,
              INTERACTION_LONGTEXT, false )

    add_obsolete_bool( "show-intf" );

    add_bool ( "stats", true, NULL, STATS_TEXT, STATS_LONGTEXT, true )
        change_need_restart ()

    set_subcategory( SUBCAT_INTERFACE_MAIN )
    add_module_cat( "intf", SUBCAT_INTERFACE_MAIN, NULL, NULL, INTF_TEXT,
                INTF_LONGTEXT, false )
        change_short('I')
        change_need_restart ()
    add_module_list_cat( "extraintf", SUBCAT_INTERFACE_MAIN,
                         NULL, NULL, EXTRAINTF_TEXT,
                         EXTRAINTF_LONGTEXT, false )
        change_need_restart ()


    set_subcategory( SUBCAT_INTERFACE_CONTROL )
    add_module_list_cat( "control", SUBCAT_INTERFACE_CONTROL, NULL, NULL,
                         CONTROL_TEXT, CONTROL_LONGTEXT, false )
        change_need_restart ()

/* Hotkey options*/
    set_subcategory( SUBCAT_INTERFACE_HOTKEYS )
    add_category_hint( N_("Hot keys"), HOTKEY_CAT_LONGTEXT , false )

#if defined(__APPLE__)
/* Don't use the following combo's */

/*  copy                          KEY_MODIFIER_COMMAND|'c'
 *  cut                           KEY_MODIFIER_COMMAND|'x'
 *  paste                         KEY_MODIFIER_COMMAND|'v'
 *  select all                    KEY_MODIFIER_COMMAND|'a'
 *  preferences                   KEY_MODIFIER_COMMAND|','
 *  hide vlc                      KEY_MODIFIER_COMMAND|'h'
 *  hide other                    KEY_MODIFIER_COMMAND|KEY_MODIFIER_ALT|'h'
 *  open file                     KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'o'
 *  open                          KEY_MODIFIER_COMMAND|'o'
 *  open disk                     KEY_MODIFIER_COMMAND|'d'
 *  open network                  KEY_MODIFIER_COMMAND|'n'
 *  open capture                  KEY_MODIFIER_COMMAND|'r'
 *  save playlist                 KEY_MODIFIER_COMMAND|'s'
 *  playlist repeat all           KEY_MODIFIER_COMMAND|'l'
 *  playlist repeat               KEY_MODIFIER_COMMAND|'r'
 *  video fit to screen           KEY_MODIFIER_COMMAND|'3'
 *  minimize window               KEY_MODIFIER_COMMAND|'m'
 *  close window                  KEY_MODIFIER_COMMAND|'w'
 *  streaming wizard              KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'w'
 *  show controller               KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'c'
 *  show playlist                 KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'p'
 *  show info                     KEY_MODIFIER_COMMAND|'i'
 *  show extended controls        KEY_MODIFIER_COMMAND|'e'
 *  show equaliser                KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'e'
 *  show bookmarks                KEY_MODIFIER_COMMAND|'b'
 *  show messages                 KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'m'
 *  show errors and warnings      KEY_MODIFIER_COMMAND|KEY_MODIFIER_CTRL|'m'
 *  help                          KEY_MODIFIER_COMMAND|'?'
 *  readme / FAQ                  KEY_MODIFIER_COMMAND|KEY_MODIFIER_ALT|'?'
 */
#   define KEY_TOGGLE_FULLSCREEN  KEY_MODIFIER_COMMAND|'f'
#   define KEY_LEAVE_FULLSCREEN   KEY_ESC
#   define KEY_PLAY_PAUSE         KEY_MODIFIER_COMMAND|'p'
#   define KEY_PAUSE              KEY_UNSET
#   define KEY_PLAY               KEY_UNSET
#   define KEY_FASTER             KEY_MODIFIER_COMMAND|'='
#   define KEY_SLOWER             KEY_MODIFIER_COMMAND|'-'
#   define KEY_RATE_NORMAL        KEY_UNSET
#   define KEY_RATE_FASTER_FINE   KEY_UNSET
#   define KEY_RATE_SLOWER_FINE   KEY_UNSET
#   define KEY_NEXT               KEY_MODIFIER_COMMAND|KEY_RIGHT
#   define KEY_PREV               KEY_MODIFIER_COMMAND|KEY_LEFT
#   define KEY_STOP               KEY_MODIFIER_COMMAND|'.'
#   define KEY_POSITION           't'
#   define KEY_JUMP_MEXTRASHORT   KEY_MODIFIER_COMMAND|KEY_MODIFIER_CTRL|KEY_LEFT
#   define KEY_JUMP_PEXTRASHORT   KEY_MODIFIER_COMMAND|KEY_MODIFIER_CTRL|KEY_RIGHT
#   define KEY_JUMP_MSHORT        KEY_MODIFIER_COMMAND|KEY_MODIFIER_ALT|KEY_LEFT
#   define KEY_JUMP_PSHORT        KEY_MODIFIER_COMMAND|KEY_MODIFIER_ALT|KEY_RIGHT
#   define KEY_JUMP_MMEDIUM       KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|KEY_LEFT
#   define KEY_JUMP_PMEDIUM       KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|KEY_RIGHT
#   define KEY_JUMP_MLONG         KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|KEY_MODIFIER_ALT|KEY_LEFT
#   define KEY_JUMP_PLONG         KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|KEY_MODIFIER_ALT|KEY_RIGHT
#   define KEY_FRAME_NEXT         'e'
#   define KEY_NAV_ACTIVATE       KEY_ENTER
#   define KEY_NAV_UP             KEY_UP
#   define KEY_NAV_DOWN           KEY_DOWN
#   define KEY_NAV_LEFT           KEY_LEFT
#   define KEY_NAV_RIGHT          KEY_RIGHT
#   define KEY_QUIT               KEY_MODIFIER_COMMAND|'q'
#   define KEY_VOL_UP             KEY_MODIFIER_COMMAND|KEY_UP
#   define KEY_VOL_DOWN           KEY_MODIFIER_COMMAND|KEY_DOWN
#   define KEY_VOL_MUTE           KEY_MODIFIER_COMMAND|KEY_MODIFIER_ALT|KEY_DOWN
#   define KEY_SUBDELAY_UP        'j'
#   define KEY_SUBDELAY_DOWN      'h'
#   define KEY_SUBPOS_DOWN        KEY_UNSET
#   define KEY_SUBPOS_UP          KEY_UNSET
#   define KEY_AUDIODELAY_UP      'g'
#   define KEY_AUDIODELAY_DOWN    'f'
#   define KEY_AUDIO_TRACK        'l'
#   define KEY_SUBTITLE_TRACK     's'
#   define KEY_ASPECT_RATIO       'a'
#   define KEY_CROP               'c'
#   define KEY_TOGGLE_AUTOSCALE   'o'
#   define KEY_SCALE_UP           KEY_MODIFIER_ALT|'o'
#   define KEY_SCALE_DOWN         KEY_MODIFIER_SHIFT|KEY_MODIFIER_ALT|'o'
#   define KEY_DEINTERLACE        'd'
#   define KEY_INTF_SHOW          'i'
#   define KEY_INTF_HIDE          KEY_MODIFIER_SHIFT|'i'
#   define KEY_DISC_MENU          KEY_MODIFIER_CTRL|'m'
#   define KEY_TITLE_PREV         KEY_MODIFIER_CTRL|'p'
#   define KEY_TITLE_NEXT         KEY_MODIFIER_CTRL|'n'
#   define KEY_CHAPTER_PREV       KEY_MODIFIER_CTRL|'u'
#   define KEY_CHAPTER_NEXT       KEY_MODIFIER_CTRL|'d'
#   define KEY_SNAPSHOT           KEY_MODIFIER_COMMAND|KEY_MODIFIER_ALT|'s'
#   define KEY_ZOOM               'z'
#   define KEY_UNZOOM             KEY_MODIFIER_SHIFT|'z'
#   define KEY_RANDOM             KEY_MODIFIER_COMMAND|'z'
#   define KEY_LOOP               KEY_MODIFIER_SHIFT|'l'

#   define KEY_CROP_TOP           KEY_MODIFIER_ALT|'i'
#   define KEY_UNCROP_TOP         KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'i'
#   define KEY_CROP_LEFT          KEY_MODIFIER_ALT|'j'
#   define KEY_UNCROP_LEFT        KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'j'
#   define KEY_CROP_BOTTOM        KEY_MODIFIER_ALT|'k'
#   define KEY_UNCROP_BOTTOM      KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'k'
#   define KEY_CROP_RIGHT         KEY_MODIFIER_ALT|'l'
#   define KEY_UNCROP_RIGHT       KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'l'

/* the macosx-interface already has bindings */
#   define KEY_ZOOM_QUARTER       KEY_UNSET
#   define KEY_ZOOM_HALF          KEY_MODIFIER_COMMAND|'0'
#   define KEY_ZOOM_ORIGINAL      KEY_MODIFIER_COMMAND|'1'
#   define KEY_ZOOM_DOUBLE        KEY_MODIFIER_COMMAND|'2'

#   define KEY_SET_BOOKMARK1      KEY_MODIFIER_COMMAND|KEY_F1
#   define KEY_SET_BOOKMARK2      KEY_MODIFIER_COMMAND|KEY_F2
#   define KEY_SET_BOOKMARK3      KEY_MODIFIER_COMMAND|KEY_F3
#   define KEY_SET_BOOKMARK4      KEY_MODIFIER_COMMAND|KEY_F4
#   define KEY_SET_BOOKMARK5      KEY_MODIFIER_COMMAND|KEY_F5
#   define KEY_SET_BOOKMARK6      KEY_MODIFIER_COMMAND|KEY_F6
#   define KEY_SET_BOOKMARK7      KEY_MODIFIER_COMMAND|KEY_F7
#   define KEY_SET_BOOKMARK8      KEY_MODIFIER_COMMAND|KEY_F8
#   define KEY_SET_BOOKMARK9      KEY_UNSET
#   define KEY_SET_BOOKMARK10     KEY_UNSET
#   define KEY_PLAY_BOOKMARK1     KEY_F1
#   define KEY_PLAY_BOOKMARK2     KEY_F2
#   define KEY_PLAY_BOOKMARK3     KEY_F3
#   define KEY_PLAY_BOOKMARK4     KEY_F4
#   define KEY_PLAY_BOOKMARK5     KEY_F5
#   define KEY_PLAY_BOOKMARK6     KEY_F6
#   define KEY_PLAY_BOOKMARK7     KEY_F7
#   define KEY_PLAY_BOOKMARK8     KEY_F8
#   define KEY_PLAY_BOOKMARK9     KEY_UNSET
#   define KEY_PLAY_BOOKMARK10    KEY_UNSET
#   define KEY_HISTORY_BACK       KEY_MODIFIER_COMMAND|'['
#   define KEY_HISTORY_FORWARD    KEY_MODIFIER_COMMAND|']'
#   define KEY_RECORD             KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'r'
#   define KEY_DUMP               KEY_MODIFIER_COMMAND|KEY_MODIFIER_SHIFT|'d'
#   define KEY_WALLPAPER          KEY_MODIFIER_COMMAND|'w'

#   define KEY_MENU_ON            KEY_MODIFIER_ALT|'m'
#   define KEY_MENU_OFF           KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'m'
#   define KEY_MENU_RIGHT         KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_RIGHT
#   define KEY_MENU_LEFT          KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_LEFT
#   define KEY_MENU_UP            KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_UP
#   define KEY_MENU_DOWN          KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_DOWN
#   define KEY_MENU_SELECT        KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_ENTER
#   define KEY_AUDIODEVICE_CYCLE  KEY_MODIFIER_SHIFT|'a'

#else /* Non Mac OS X */
    /*
       You should try to avoid Ctrl + letter key, because they are usually for
       dialogs showing and interface related stuffs.
       It would be nice (less important than previous rule) to try to avoid
       alt + letter key, because they are usually for menu accelerators and you
       don't know how the translator is going to do it.
     */
#   define KEY_TOGGLE_FULLSCREEN  'f'
#   define KEY_LEAVE_FULLSCREEN   KEY_ESC
#   define KEY_PLAY_PAUSE         ' '
#   define KEY_PAUSE              KEY_UNSET
#   define KEY_PLAY               KEY_UNSET
#   define KEY_FASTER             '+'
#   define KEY_SLOWER             '-'
#   define KEY_RATE_NORMAL        '='
#   define KEY_RATE_FASTER_FINE   ']'
#   define KEY_RATE_SLOWER_FINE   '['
#   define KEY_NEXT               'n'
#   define KEY_PREV               'p'
#   define KEY_STOP               's'
#   define KEY_POSITION           't'
#   define KEY_JUMP_MEXTRASHORT   KEY_MODIFIER_SHIFT|KEY_LEFT
#   define KEY_JUMP_PEXTRASHORT   KEY_MODIFIER_SHIFT|KEY_RIGHT
#   define KEY_JUMP_MSHORT        KEY_MODIFIER_ALT|KEY_LEFT
#   define KEY_JUMP_PSHORT        KEY_MODIFIER_ALT|KEY_RIGHT
#   define KEY_JUMP_MMEDIUM       KEY_MODIFIER_CTRL|KEY_LEFT
#   define KEY_JUMP_PMEDIUM       KEY_MODIFIER_CTRL|KEY_RIGHT
#   define KEY_JUMP_MLONG         KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_LEFT
#   define KEY_JUMP_PLONG         KEY_MODIFIER_CTRL|KEY_MODIFIER_ALT|KEY_RIGHT
#   define KEY_FRAME_NEXT         'e'
#   define KEY_NAV_ACTIVATE       KEY_ENTER
#   define KEY_NAV_UP             KEY_UP
#   define KEY_NAV_DOWN           KEY_DOWN
#   define KEY_NAV_LEFT           KEY_LEFT
#   define KEY_NAV_RIGHT          KEY_RIGHT
#   define KEY_QUIT               KEY_MODIFIER_CTRL|'q'
#   define KEY_VOL_UP             KEY_MODIFIER_CTRL|KEY_UP
#   define KEY_VOL_DOWN           KEY_MODIFIER_CTRL|KEY_DOWN
#   define KEY_VOL_MUTE           'm'
#   define KEY_SUBDELAY_UP        'h'
#   define KEY_SUBDELAY_DOWN      'g'
#   define KEY_SUBPOS_DOWN        KEY_UNSET
#   define KEY_SUBPOS_UP          KEY_UNSET
#   define KEY_AUDIODELAY_UP      'k'
#   define KEY_AUDIODELAY_DOWN    'j'
#   define KEY_RANDOM             'r'
#   define KEY_LOOP               'l'

#   define KEY_AUDIO_TRACK        'b'
#   define KEY_SUBTITLE_TRACK     'v'
#   define KEY_ASPECT_RATIO       'a'
#   define KEY_CROP               'c'
#   define KEY_TOGGLE_AUTOSCALE   'o'
#   define KEY_SCALE_UP           KEY_MODIFIER_ALT|'o'
#   define KEY_SCALE_DOWN         KEY_MODIFIER_SHIFT|KEY_MODIFIER_ALT|'o'
#   define KEY_DEINTERLACE        'd'
#   define KEY_INTF_SHOW          'i'
#   define KEY_INTF_HIDE          KEY_MODIFIER_SHIFT|'i'
#   define KEY_DISC_MENU          KEY_MODIFIER_SHIFT|'m'
#   define KEY_TITLE_PREV         KEY_MODIFIER_SHIFT|'o'
#   define KEY_TITLE_NEXT         KEY_MODIFIER_SHIFT|'b'
#   define KEY_CHAPTER_PREV       KEY_MODIFIER_SHIFT|'p'
#   define KEY_CHAPTER_NEXT       KEY_MODIFIER_SHIFT|'n'
#   define KEY_SNAPSHOT           KEY_MODIFIER_SHIFT|'s'

#   define KEY_ZOOM               'z'
#   define KEY_UNZOOM             KEY_MODIFIER_SHIFT|'z'

#   define KEY_AUDIODEVICE_CYCLE  KEY_MODIFIER_SHIFT|'a'

#   define KEY_HISTORY_BACK       KEY_MODIFIER_SHIFT|'g'
#   define KEY_HISTORY_FORWARD    KEY_MODIFIER_SHIFT|'h'
#   define KEY_RECORD             KEY_MODIFIER_SHIFT|'r'
#   define KEY_DUMP               KEY_MODIFIER_SHIFT|'d'
#   define KEY_WALLPAPER          'w'

/* Cropping */
#   define KEY_CROP_TOP           KEY_MODIFIER_ALT|'r'
#   define KEY_UNCROP_TOP         KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'r'
#   define KEY_CROP_LEFT          KEY_MODIFIER_ALT|'d'
#   define KEY_UNCROP_LEFT        KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'d'
#   define KEY_CROP_BOTTOM        KEY_MODIFIER_ALT|'c'
#   define KEY_UNCROP_BOTTOM      KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'c'
#   define KEY_CROP_RIGHT         KEY_MODIFIER_ALT|'f'
#   define KEY_UNCROP_RIGHT       KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'f'

/* Zooming */
#   define KEY_ZOOM_QUARTER       KEY_MODIFIER_ALT|'1'
#   define KEY_ZOOM_HALF          KEY_MODIFIER_ALT|'2'
#   define KEY_ZOOM_ORIGINAL      KEY_MODIFIER_ALT|'3'
#   define KEY_ZOOM_DOUBLE        KEY_MODIFIER_ALT|'4'

/* Bookmarks */
#   define KEY_SET_BOOKMARK1      KEY_MODIFIER_CTRL|KEY_F1
#   define KEY_SET_BOOKMARK2      KEY_MODIFIER_CTRL|KEY_F2
#   define KEY_SET_BOOKMARK3      KEY_MODIFIER_CTRL|KEY_F3
#   define KEY_SET_BOOKMARK4      KEY_MODIFIER_CTRL|KEY_F4
#   define KEY_SET_BOOKMARK5      KEY_MODIFIER_CTRL|KEY_F5
#   define KEY_SET_BOOKMARK6      KEY_MODIFIER_CTRL|KEY_F6
#   define KEY_SET_BOOKMARK7      KEY_MODIFIER_CTRL|KEY_F7
#   define KEY_SET_BOOKMARK8      KEY_MODIFIER_CTRL|KEY_F8
#   define KEY_SET_BOOKMARK9      KEY_MODIFIER_CTRL|KEY_F9
#   define KEY_SET_BOOKMARK10     KEY_MODIFIER_CTRL|KEY_F10
#   define KEY_PLAY_BOOKMARK1     KEY_F1
#   define KEY_PLAY_BOOKMARK2     KEY_F2
#   define KEY_PLAY_BOOKMARK3     KEY_F3
#   define KEY_PLAY_BOOKMARK4     KEY_F4
#   define KEY_PLAY_BOOKMARK5     KEY_F5
#   define KEY_PLAY_BOOKMARK6     KEY_F6
#   define KEY_PLAY_BOOKMARK7     KEY_F7
#   define KEY_PLAY_BOOKMARK8     KEY_F8
#   define KEY_PLAY_BOOKMARK9     KEY_F9
#   define KEY_PLAY_BOOKMARK10    KEY_F10

/* OSD menu */
#   define KEY_MENU_ON            KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|'m'
#   define KEY_MENU_OFF           KEY_MODIFIER_ALT|KEY_MODIFIER_CTRL|'m'
#   define KEY_MENU_RIGHT         KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_RIGHT
#   define KEY_MENU_LEFT          KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_LEFT
#   define KEY_MENU_UP            KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_UP
#   define KEY_MENU_DOWN          KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_DOWN
#   define KEY_MENU_SELECT        KEY_MODIFIER_ALT|KEY_MODIFIER_SHIFT|KEY_ENTER
#endif

    add_key( "key-toggle-fullscreen", KEY_TOGGLE_FULLSCREEN, NULL, TOGGLE_FULLSCREEN_KEY_TEXT,
             TOGGLE_FULLSCREEN_KEY_LONGTEXT, false )
       add_deprecated_alias( "key-fullscreen" ) /*deprecated since 0.9.0 */
    add_key( "key-leave-fullscreen", KEY_LEAVE_FULLSCREEN, NULL, LEAVE_FULLSCREEN_KEY_TEXT,
             LEAVE_FULLSCREEN_KEY_LONGTEXT, false )
    add_key( "key-play-pause", KEY_PLAY_PAUSE, NULL, PLAY_PAUSE_KEY_TEXT,
             PLAY_PAUSE_KEY_LONGTEXT, false )
    add_key( "key-pause", KEY_PAUSE, NULL, PAUSE_KEY_TEXT,
             PAUSE_KEY_LONGTEXT, true )
    add_key( "key-play", KEY_PLAY, NULL, PLAY_KEY_TEXT,
             PLAY_KEY_LONGTEXT, true )
    add_key( "key-faster", KEY_FASTER, NULL, FASTER_KEY_TEXT,
             FASTER_KEY_LONGTEXT, false )
    add_key( "key-slower", KEY_SLOWER, NULL, SLOWER_KEY_TEXT,
             SLOWER_KEY_LONGTEXT, false )
    add_key( "key-rate-normal", KEY_RATE_NORMAL, NULL, RATE_NORMAL_KEY_TEXT,
             RATE_NORMAL_KEY_LONGTEXT, false )
    add_key( "key-rate-faster-fine", KEY_RATE_FASTER_FINE, NULL, RATE_FASTER_FINE_KEY_TEXT,
             RATE_FASTER_FINE_KEY_LONGTEXT, false )
    add_key( "key-rate-slower-fine", KEY_RATE_SLOWER_FINE, NULL, RATE_SLOWER_FINE_KEY_TEXT,
             RATE_SLOWER_FINE_KEY_LONGTEXT, false )
    add_key( "key-next", KEY_NEXT, NULL, NEXT_KEY_TEXT,
             NEXT_KEY_LONGTEXT, false )
    add_key( "key-prev", KEY_PREV, NULL, PREV_KEY_TEXT,
             PREV_KEY_LONGTEXT, false )
    add_key( "key-stop", KEY_STOP, NULL, STOP_KEY_TEXT,
             STOP_KEY_LONGTEXT, false )
    add_key( "key-position", KEY_POSITION, NULL, POSITION_KEY_TEXT,
             POSITION_KEY_LONGTEXT, true )
    add_key( "key-jump-extrashort", KEY_JUMP_MEXTRASHORT, NULL,
             JBEXTRASHORT_KEY_TEXT, JBEXTRASHORT_KEY_LONGTEXT, false )
    add_key( "key-jump+extrashort", KEY_JUMP_PEXTRASHORT, NULL,
             JFEXTRASHORT_KEY_TEXT, JFEXTRASHORT_KEY_LONGTEXT, false )
    add_key( "key-jump-short", KEY_JUMP_MSHORT, NULL, JBSHORT_KEY_TEXT,
             JBSHORT_KEY_LONGTEXT, false )
    add_key( "key-jump+short", KEY_JUMP_PSHORT, NULL, JFSHORT_KEY_TEXT,
             JFSHORT_KEY_LONGTEXT, false )
    add_key( "key-jump-medium", KEY_JUMP_MMEDIUM, NULL, JBMEDIUM_KEY_TEXT,
             JBMEDIUM_KEY_LONGTEXT, false )
    add_key( "key-jump+medium", KEY_JUMP_PMEDIUM, NULL, JFMEDIUM_KEY_TEXT,
             JFMEDIUM_KEY_LONGTEXT, false )
    add_key( "key-jump-long", KEY_JUMP_MLONG, NULL, JBLONG_KEY_TEXT,
             JBLONG_KEY_LONGTEXT, false )
    add_key( "key-jump+long", KEY_JUMP_PLONG, NULL, JFLONG_KEY_TEXT,
             JFLONG_KEY_LONGTEXT, false )
    add_key( "key-frame-next", KEY_FRAME_NEXT, NULL, FRAME_NEXT_KEY_TEXT,
             FRAME_NEXT_KEY_LONGTEXT, false )
    add_key( "key-nav-activate", KEY_NAV_ACTIVATE, NULL, NAV_ACTIVATE_KEY_TEXT,
             NAV_ACTIVATE_KEY_LONGTEXT, true )
    add_key( "key-nav-up", KEY_NAV_UP, NULL, NAV_UP_KEY_TEXT,
             NAV_UP_KEY_LONGTEXT, true )
    add_key( "key-nav-down", KEY_NAV_DOWN, NULL, NAV_DOWN_KEY_TEXT,
             NAV_DOWN_KEY_LONGTEXT, true )
    add_key( "key-nav-left", KEY_NAV_LEFT, NULL, NAV_LEFT_KEY_TEXT,
             NAV_LEFT_KEY_LONGTEXT, true )
    add_key( "key-nav-right", KEY_NAV_RIGHT, NULL, NAV_RIGHT_KEY_TEXT,
             NAV_RIGHT_KEY_LONGTEXT, true )

    add_key( "key-disc-menu", KEY_DISC_MENU, NULL, DISC_MENU_TEXT,
             DISC_MENU_LONGTEXT, true )
    add_key( "key-title-prev", KEY_TITLE_PREV, NULL, TITLE_PREV_TEXT,
             TITLE_PREV_LONGTEXT, true )
    add_key( "key-title-next", KEY_TITLE_NEXT, NULL, TITLE_NEXT_TEXT,
             TITLE_NEXT_LONGTEXT, true )
    add_key( "key-chapter-prev", KEY_CHAPTER_PREV, NULL, CHAPTER_PREV_TEXT,
             CHAPTER_PREV_LONGTEXT, true )
    add_key( "key-chapter-next", KEY_CHAPTER_NEXT, NULL, CHAPTER_NEXT_TEXT,
             CHAPTER_NEXT_LONGTEXT, true )
    add_key( "key-quit", KEY_QUIT, NULL, QUIT_KEY_TEXT,
             QUIT_KEY_LONGTEXT, false )
    add_key( "key-vol-up", KEY_VOL_UP, NULL, VOL_UP_KEY_TEXT,
             VOL_UP_KEY_LONGTEXT, false )
    add_key( "key-vol-down", KEY_VOL_DOWN, NULL, VOL_DOWN_KEY_TEXT,
             VOL_DOWN_KEY_LONGTEXT, false )
    add_key( "key-vol-mute", KEY_VOL_MUTE, NULL, VOL_MUTE_KEY_TEXT,
             VOL_MUTE_KEY_LONGTEXT, false )
    add_key( "key-subdelay-up", KEY_SUBDELAY_UP, NULL,
             SUBDELAY_UP_KEY_TEXT, SUBDELAY_UP_KEY_LONGTEXT, true )
    add_key( "key-subdelay-down", KEY_SUBDELAY_DOWN, NULL,
             SUBDELAY_DOWN_KEY_TEXT, SUBDELAY_DOWN_KEY_LONGTEXT, true )
    add_key( "key-subpos-up", KEY_SUBPOS_UP, NULL,
             SUBPOS_UP_KEY_TEXT, SUBPOS_UP_KEY_LONGTEXT, true )
    add_key( "key-subpos-down", KEY_SUBPOS_DOWN, NULL,
             SUBPOS_DOWN_KEY_TEXT, SUBPOS_DOWN_KEY_LONGTEXT, true )
    add_key( "key-audiodelay-up", KEY_AUDIODELAY_UP, NULL,
             AUDIODELAY_UP_KEY_TEXT, AUDIODELAY_UP_KEY_LONGTEXT, true )
    add_key( "key-audiodelay-down", KEY_AUDIODELAY_DOWN, NULL,
             AUDIODELAY_DOWN_KEY_TEXT, AUDIODELAY_DOWN_KEY_LONGTEXT, true )
    add_key( "key-audio-track", KEY_AUDIO_TRACK, NULL, AUDIO_TRACK_KEY_TEXT,
             AUDIO_TRACK_KEY_LONGTEXT, false )
    add_key( "key-audiodevice-cycle", KEY_AUDIODEVICE_CYCLE, NULL, AUDI_DEVICE_CYCLE_KEY_TEXT,
             AUDI_DEVICE_CYCLE_KEY_LONGTEXT, false )
    add_key( "key-subtitle-track", KEY_SUBTITLE_TRACK, NULL,
             SUBTITLE_TRACK_KEY_TEXT, SUBTITLE_TRACK_KEY_LONGTEXT, false )
    add_key( "key-aspect-ratio", KEY_ASPECT_RATIO, NULL,
             ASPECT_RATIO_KEY_TEXT, ASPECT_RATIO_KEY_LONGTEXT, false )
    add_key( "key-crop", KEY_CROP, NULL,
             CROP_KEY_TEXT, CROP_KEY_LONGTEXT, false )
    add_key( "key-toggle-autoscale", KEY_TOGGLE_AUTOSCALE, NULL,
             TOGGLE_AUTOSCALE_KEY_TEXT, TOGGLE_AUTOSCALE_KEY_LONGTEXT, false )
    add_key( "key-incr-scalefactor", KEY_SCALE_UP, NULL,
             SCALE_UP_KEY_TEXT, SCALE_UP_KEY_LONGTEXT, false )
    add_key( "key-decr-scalefactor", KEY_SCALE_DOWN, NULL,
             SCALE_DOWN_KEY_TEXT, SCALE_DOWN_KEY_LONGTEXT, false )
    add_key( "key-deinterlace", KEY_DEINTERLACE, NULL,
             DEINTERLACE_KEY_TEXT, DEINTERLACE_KEY_LONGTEXT, false )
    add_key( "key-intf-show", KEY_INTF_SHOW, NULL,
             INTF_SHOW_KEY_TEXT, INTF_SHOW_KEY_LONGTEXT, true )
    add_key( "key-intf-hide", KEY_INTF_HIDE, NULL,
             INTF_HIDE_KEY_TEXT, INTF_HIDE_KEY_LONGTEXT, true )
    add_key( "key-snapshot", KEY_SNAPSHOT, NULL,
        SNAP_KEY_TEXT, SNAP_KEY_LONGTEXT, true )
    add_key( "key-history-back", KEY_HISTORY_BACK, NULL, HISTORY_BACK_TEXT,
             HISTORY_BACK_LONGTEXT, true )
    add_key( "key-history-forward", KEY_HISTORY_FORWARD, NULL,
             HISTORY_FORWARD_TEXT, HISTORY_FORWARD_LONGTEXT, true )
    add_key( "key-record", KEY_RECORD, NULL,
             RECORD_KEY_TEXT, RECORD_KEY_LONGTEXT, true )
    add_key( "key-dump", KEY_DUMP, NULL,
             DUMP_KEY_TEXT, DUMP_KEY_LONGTEXT, true )
    add_key( "key-zoom", KEY_ZOOM, NULL,
             ZOOM_KEY_TEXT, ZOOM_KEY_LONGTEXT, true )
    add_key( "key-unzoom", KEY_UNZOOM, NULL,
             UNZOOM_KEY_TEXT, UNZOOM_KEY_LONGTEXT, true )
    add_key( "key-wallpaper", KEY_WALLPAPER, NULL, WALLPAPER_KEY_TEXT,
             WALLPAPER_KEY_LONGTEXT, false )

    add_key( "key-menu-on", KEY_MENU_ON, NULL,
             MENU_ON_KEY_TEXT, MENU_ON_KEY_LONGTEXT, true )
    add_key( "key-menu-off", KEY_MENU_OFF, NULL,
             MENU_OFF_KEY_TEXT, MENU_OFF_KEY_LONGTEXT, true )
    add_key( "key-menu-right", KEY_MENU_RIGHT, NULL,
             MENU_RIGHT_KEY_TEXT, MENU_RIGHT_KEY_LONGTEXT, true )
    add_key( "key-menu-left", KEY_MENU_LEFT, NULL,
             MENU_LEFT_KEY_TEXT, MENU_LEFT_KEY_LONGTEXT, true )
    add_key( "key-menu-up", KEY_MENU_UP, NULL,
             MENU_UP_KEY_TEXT, MENU_UP_KEY_LONGTEXT, true )
    add_key( "key-menu-down", KEY_MENU_DOWN, NULL,
             MENU_DOWN_KEY_TEXT, MENU_DOWN_KEY_LONGTEXT, true )
    add_key( "key-menu-select", KEY_MENU_SELECT, NULL,
             MENU_SELECT_KEY_TEXT, MENU_SELECT_KEY_LONGTEXT, true )

    add_key( "key-crop-top", KEY_CROP_TOP, NULL,
             CROP_TOP_KEY_TEXT, CROP_TOP_KEY_LONGTEXT, true )
    add_key( "key-uncrop-top", KEY_UNCROP_TOP, NULL,
             UNCROP_TOP_KEY_TEXT, UNCROP_TOP_KEY_LONGTEXT, true )
    add_key( "key-crop-left", KEY_CROP_LEFT, NULL,
             CROP_LEFT_KEY_TEXT, CROP_LEFT_KEY_LONGTEXT, true )
    add_key( "key-uncrop-left", KEY_UNCROP_LEFT, NULL,
             UNCROP_LEFT_KEY_TEXT, UNCROP_LEFT_KEY_LONGTEXT, true )
    add_key( "key-crop-bottom", KEY_CROP_BOTTOM, NULL,
             CROP_BOTTOM_KEY_TEXT, CROP_BOTTOM_KEY_LONGTEXT, true )
    add_key( "key-uncrop-bottom", KEY_UNCROP_BOTTOM, NULL,
             UNCROP_BOTTOM_KEY_TEXT, UNCROP_BOTTOM_KEY_LONGTEXT, true )
    add_key( "key-crop-right", KEY_CROP_RIGHT, NULL,
             CROP_RIGHT_KEY_TEXT, CROP_RIGHT_KEY_LONGTEXT, true )
    add_key( "key-uncrop-right", KEY_UNCROP_RIGHT, NULL,
             UNCROP_RIGHT_KEY_TEXT, UNCROP_RIGHT_KEY_LONGTEXT, true )
    add_key( "key-random", KEY_RANDOM, NULL,
             RANDOM_KEY_TEXT, RANDOM_KEY_LONGTEXT, false )
    add_key( "key-loop", KEY_LOOP, NULL,
             LOOP_KEY_TEXT, LOOP_KEY_LONGTEXT, false )

    set_section ( N_("Zoom" ), NULL )
    add_key( "key-zoom-quarter",  KEY_ZOOM_QUARTER, NULL,
        ZOOM_QUARTER_KEY_TEXT,  NULL, false )
    add_key( "key-zoom-half",     KEY_ZOOM_HALF, NULL,
        ZOOM_HALF_KEY_TEXT,     NULL, false )
    add_key( "key-zoom-original", KEY_ZOOM_ORIGINAL, NULL,
        ZOOM_ORIGINAL_KEY_TEXT, NULL, false )
    add_key( "key-zoom-double",   KEY_ZOOM_DOUBLE, NULL,
        ZOOM_DOUBLE_KEY_TEXT,   NULL, false )

    set_section ( N_("Jump sizes" ), NULL )
    add_integer( "extrashort-jump-size", 3, NULL, JIEXTRASHORT_TEXT,
                                    JIEXTRASHORT_LONGTEXT, false )
    add_integer( "short-jump-size", 10, NULL, JISHORT_TEXT,
                                    JISHORT_LONGTEXT, false )
    add_integer( "medium-jump-size", 60, NULL, JIMEDIUM_TEXT,
                                    JIMEDIUM_LONGTEXT, false )
    add_integer( "long-jump-size", 300, NULL, JILONG_TEXT,
                                    JILONG_LONGTEXT, false )

    /* HACK so these don't get displayed */
    set_category( -1 )
    set_subcategory( -1 )
    add_key( "key-set-bookmark1", KEY_SET_BOOKMARK1, NULL,
             SET_BOOKMARK1_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark2", KEY_SET_BOOKMARK2, NULL,
             SET_BOOKMARK2_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark3", KEY_SET_BOOKMARK3, NULL,
             SET_BOOKMARK3_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark4", KEY_SET_BOOKMARK4, NULL,
             SET_BOOKMARK4_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark5", KEY_SET_BOOKMARK5, NULL,
             SET_BOOKMARK5_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark6", KEY_SET_BOOKMARK6, NULL,
             SET_BOOKMARK6_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark7", KEY_SET_BOOKMARK7, NULL,
             SET_BOOKMARK7_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark8", KEY_SET_BOOKMARK8, NULL,
             SET_BOOKMARK8_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark9", KEY_SET_BOOKMARK9, NULL,
             SET_BOOKMARK9_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-set-bookmark10", KEY_SET_BOOKMARK10, NULL,
             SET_BOOKMARK10_KEY_TEXT, SET_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark1", KEY_PLAY_BOOKMARK1, NULL,
             PLAY_BOOKMARK1_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark2", KEY_PLAY_BOOKMARK2, NULL,
             PLAY_BOOKMARK2_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark3", KEY_PLAY_BOOKMARK3, NULL,
             PLAY_BOOKMARK3_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark4", KEY_PLAY_BOOKMARK4, NULL,
             PLAY_BOOKMARK4_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark5", KEY_PLAY_BOOKMARK5, NULL,
             PLAY_BOOKMARK5_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark6", KEY_PLAY_BOOKMARK6, NULL,
             PLAY_BOOKMARK6_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark7", KEY_PLAY_BOOKMARK7, NULL,
             PLAY_BOOKMARK7_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark8", KEY_PLAY_BOOKMARK8, NULL,
             PLAY_BOOKMARK8_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark9", KEY_PLAY_BOOKMARK9, NULL,
             PLAY_BOOKMARK9_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )
    add_key( "key-play-bookmark10", KEY_PLAY_BOOKMARK10, NULL,
             PLAY_BOOKMARK10_KEY_TEXT, PLAY_BOOKMARK_KEY_LONGTEXT, true )


    add_string( "bookmark1", NULL, NULL,
             BOOKMARK1_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark2", NULL, NULL,
             BOOKMARK2_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark3", NULL, NULL,
             BOOKMARK3_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark4", NULL, NULL,
             BOOKMARK4_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark5", NULL, NULL,
             BOOKMARK5_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark6", NULL, NULL,
             BOOKMARK6_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark7", NULL, NULL,
             BOOKMARK7_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark8", NULL, NULL,
             BOOKMARK8_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark9", NULL, NULL,
             BOOKMARK9_TEXT, BOOKMARK_LONGTEXT, false )
    add_string( "bookmark10", NULL, NULL,
              BOOKMARK10_TEXT, BOOKMARK_LONGTEXT, false )

#define HELP_TEXT \
    N_("print help for VLC (can be combined with --advanced and " \
       "--help-verbose)")
#define FULL_HELP_TEXT \
    N_("Exhaustive help for VLC and its modules")
#define LONGHELP_TEXT \
    N_("print help for VLC and all its modules (can be combined with " \
       "--advanced and --help-verbose)")
#define HELP_VERBOSE_TEXT \
    N_("ask for extra verbosity when displaying help")
#define LIST_TEXT \
    N_("print a list of available modules")
#define LIST_VERBOSE_TEXT \
    N_("print a list of available modules with extra detail")
#define MODULE_TEXT \
    N_("print help on a specific module (can be combined with --advanced " \
       "and --help-verbose). Prefix the module name with = for strict " \
       "matches.")
#define IGNORE_CONFIG_TEXT \
    N_("no configuration option will be loaded nor saved to config file")
#define RESET_CONFIG_TEXT \
    N_("reset the current config to the default values")
#define CONFIG_TEXT \
    N_("use alternate config file")
#define RESET_PLUGINS_CACHE_TEXT \
    N_("resets the current plugins cache")
#define VERSION_TEXT \
    N_("print version information")

    add_bool( "help", false, NULL, HELP_TEXT, "", false )
        change_short( 'h' )
        change_volatile ()
    add_bool( "full-help", false, NULL, FULL_HELP_TEXT, "", false )
        change_short( 'H' )
        change_volatile ()
    add_bool( "longhelp", false, NULL, LONGHELP_TEXT, "", false )
        change_volatile ()
    add_bool( "help-verbose", false, NULL, HELP_VERBOSE_TEXT, "",
              false )
        change_volatile ()
    add_bool( "list", false, NULL, LIST_TEXT, "", false )
        change_short( 'l' )
        change_volatile ()
    add_bool( "list-verbose", false, NULL, LIST_VERBOSE_TEXT, "",
              false )
        change_volatile ()
    add_string( "module", NULL, NULL, MODULE_TEXT, "", false )
        change_short( 'p' )
        change_volatile ()
    add_bool( "ignore-config", true, NULL, IGNORE_CONFIG_TEXT, "", false )
        change_volatile ()
    add_obsolete_bool( "save-config" )
    add_bool( "reset-config", false, NULL, RESET_CONFIG_TEXT, "", false )
        change_volatile ()
    add_bool( "reset-plugins-cache", false, NULL,
              RESET_PLUGINS_CACHE_TEXT, "", false )
        change_volatile ()
    add_bool( "version", false, NULL, VERSION_TEXT, "", false )
        change_volatile ()
    add_string( "config", NULL, NULL, CONFIG_TEXT, "", false )
        change_volatile ()

   /* Usage (mainly useful for cmd line stuff) */
    /* add_usage_hint( PLAYLIST_USAGE ) */

    set_description( N_("main program") )
    set_capability( "main", 100 )
vlc_module_end ()

/*****************************************************************************
 * End configuration.
 *****************************************************************************/

/*****************************************************************************
 * Initializer for the libvlc instance structure
 * storing the action / key associations
 *****************************************************************************/
const struct action libvlc_actions[] =
{
    /* *MUST* be sorted (ASCII order) */
    { "key-aspect-ratio", ACTIONID_ASPECT_RATIO, },
    { "key-audio-track", ACTIONID_AUDIO_TRACK, },
    { "key-audiodelay-down", ACTIONID_AUDIODELAY_DOWN, },
    { "key-audiodelay-up", ACTIONID_AUDIODELAY_UP, },
    { "key-audiodevice-cycle", ACTIONID_AUDIODEVICE_CYCLE, },
    { "key-chapter-next", ACTIONID_CHAPTER_NEXT, },
    { "key-chapter-prev", ACTIONID_CHAPTER_PREV, },
    { "key-crop", ACTIONID_CROP, },
    { "key-crop-bottom", ACTIONID_CROP_BOTTOM, },
    { "key-crop-left", ACTIONID_CROP_LEFT, },
    { "key-crop-right", ACTIONID_CROP_RIGHT, },
    { "key-crop-top", ACTIONID_CROP_TOP, },
    { "key-decr-scalefactor", ACTIONID_SCALE_DOWN, },
    { "key-deinterlace", ACTIONID_DEINTERLACE, },
    { "key-disc-menu", ACTIONID_DISC_MENU, },
    { "key-dump", ACTIONID_DUMP, },
    { "key-faster", ACTIONID_FASTER, },
    { "key-frame-next", ACTIONID_FRAME_NEXT, },
    { "key-history-back", ACTIONID_HISTORY_BACK, },
    { "key-history-forward", ACTIONID_HISTORY_FORWARD, },
    { "key-incr-scalefactor", ACTIONID_SCALE_UP, },
    { "key-intf-hide", ACTIONID_INTF_HIDE, },
    { "key-intf-show", ACTIONID_INTF_SHOW, },
    { "key-jump+extrashort", ACTIONID_JUMP_FORWARD_EXTRASHORT, },
    { "key-jump+long", ACTIONID_JUMP_FORWARD_LONG, },
    { "key-jump+medium", ACTIONID_JUMP_FORWARD_MEDIUM, },
    { "key-jump+short", ACTIONID_JUMP_FORWARD_SHORT, },
    { "key-jump-extrashort", ACTIONID_JUMP_BACKWARD_EXTRASHORT, },
    { "key-jump-long", ACTIONID_JUMP_BACKWARD_LONG, },
    { "key-jump-medium", ACTIONID_JUMP_BACKWARD_MEDIUM, },
    { "key-jump-short", ACTIONID_JUMP_BACKWARD_SHORT, },
    { "key-leave-fullscreen", ACTIONID_LEAVE_FULLSCREEN, },
    { "key-loop", ACTIONID_LOOP, },
    { "key-menu-down", ACTIONID_MENU_DOWN, },
    { "key-menu-left", ACTIONID_MENU_LEFT, },
    { "key-menu-off", ACTIONID_MENU_OFF, },
    { "key-menu-on", ACTIONID_MENU_ON, },
    { "key-menu-right", ACTIONID_MENU_RIGHT, },
    { "key-menu-select", ACTIONID_MENU_SELECT, },
    { "key-menu-up", ACTIONID_MENU_UP, },
    { "key-nav-activate", ACTIONID_NAV_ACTIVATE, },
    { "key-nav-down", ACTIONID_NAV_DOWN, },
    { "key-nav-left", ACTIONID_NAV_LEFT, },
    { "key-nav-right", ACTIONID_NAV_RIGHT, },
    { "key-nav-up", ACTIONID_NAV_UP, },
    { "key-next", ACTIONID_NEXT, },
    { "key-pause", ACTIONID_PAUSE, },
    { "key-play", ACTIONID_PLAY, },
    { "key-play-bookmark1", ACTIONID_PLAY_BOOKMARK1, },
    { "key-play-bookmark10", ACTIONID_PLAY_BOOKMARK10, },
    { "key-play-bookmark2", ACTIONID_PLAY_BOOKMARK2, },
    { "key-play-bookmark3", ACTIONID_PLAY_BOOKMARK3, },
    { "key-play-bookmark4", ACTIONID_PLAY_BOOKMARK4, },
    { "key-play-bookmark5", ACTIONID_PLAY_BOOKMARK5, },
    { "key-play-bookmark6", ACTIONID_PLAY_BOOKMARK6, },
    { "key-play-bookmark7", ACTIONID_PLAY_BOOKMARK7, },
    { "key-play-bookmark8", ACTIONID_PLAY_BOOKMARK8, },
    { "key-play-bookmark9", ACTIONID_PLAY_BOOKMARK9, },
    { "key-play-pause", ACTIONID_PLAY_PAUSE, },
    { "key-position", ACTIONID_POSITION, },
    { "key-prev", ACTIONID_PREV, },
    { "key-quit", ACTIONID_QUIT, },
    { "key-random", ACTIONID_RANDOM, },
    { "key-rate-faster-fine", ACTIONID_RATE_FASTER_FINE, },
    { "key-rate-normal", ACTIONID_RATE_NORMAL, },
    { "key-rate-slower-fine", ACTIONID_RATE_SLOWER_FINE, },
    { "key-record", ACTIONID_RECORD, },
    { "key-set-bookmark1", ACTIONID_SET_BOOKMARK1, },
    { "key-set-bookmark10", ACTIONID_SET_BOOKMARK10, },
    { "key-set-bookmark2", ACTIONID_SET_BOOKMARK2, },
    { "key-set-bookmark3", ACTIONID_SET_BOOKMARK3, },
    { "key-set-bookmark4", ACTIONID_SET_BOOKMARK4, },
    { "key-set-bookmark5", ACTIONID_SET_BOOKMARK5, },
    { "key-set-bookmark6", ACTIONID_SET_BOOKMARK6, },
    { "key-set-bookmark7", ACTIONID_SET_BOOKMARK7, },
    { "key-set-bookmark8", ACTIONID_SET_BOOKMARK8, },
    { "key-set-bookmark9", ACTIONID_SET_BOOKMARK9, },
    { "key-slower", ACTIONID_SLOWER, },
    { "key-snapshot", ACTIONID_SNAPSHOT, },
    { "key-stop", ACTIONID_STOP, },
    { "key-subdelay-down", ACTIONID_SUBDELAY_DOWN, },
    { "key-subdelay-up", ACTIONID_SUBDELAY_UP, },
    { "key-subpos-down", ACTIONID_SUBPOS_DOWN, },
    { "key-subpos-up", ACTIONID_SUBPOS_UP, },
    { "key-subtitle-track", ACTIONID_SUBTITLE_TRACK, },
    { "key-title-next", ACTIONID_TITLE_NEXT, },
    { "key-title-prev", ACTIONID_TITLE_PREV, },
    { "key-toggle-autoscale", ACTIONID_TOGGLE_AUTOSCALE, },
    { "key-toggle-fullscreen", ACTIONID_TOGGLE_FULLSCREEN, },
    { "key-uncrop-bottom", ACTIONID_UNCROP_BOTTOM, },
    { "key-uncrop-left", ACTIONID_UNCROP_LEFT, },
    { "key-uncrop-right", ACTIONID_UNCROP_RIGHT, },
    { "key-uncrop-top", ACTIONID_UNCROP_TOP, },
    { "key-unzoom", ACTIONID_UNZOOM, },
    { "key-vol-down", ACTIONID_VOL_DOWN, },
    { "key-vol-mute", ACTIONID_VOL_MUTE, },
    { "key-vol-up", ACTIONID_VOL_UP, },
    { "key-wallpaper", ACTIONID_WALLPAPER, },
    { "key-zoom", ACTIONID_ZOOM, },
    { "key-zoom-double", ACTIONID_ZOOM_DOUBLE, },
    { "key-zoom-half", ACTIONID_ZOOM_HALF, },
    { "key-zoom-original", ACTIONID_ZOOM_ORIGINAL, },
    { "key-zoom-quarter", ACTIONID_ZOOM_QUARTER, },
};

const size_t libvlc_actions_count =
    sizeof (libvlc_actions) / sizeof (libvlc_actions[0]);
