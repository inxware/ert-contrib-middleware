/*****************************************************************************
 * dshow.cpp : DirectShow access module for vlc
 *****************************************************************************
 * Copyright (C) 2002, 2003 the VideoLAN team
 * $Id: 9f5210ac658fb1c444f0ff563cb1756f0aa1db18 $
 *
 * Author: Gildas Bazin <gbazin@videolan.org>
 *         Damien Fouilleul <damienf@videolan.org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define __STDC_CONSTANT_MACROS 1
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_input.h>
#include <vlc_access.h>
#include <vlc_demux.h>
#include <vlc_dialog.h>
#include <vlc_charset.h>

#include "common.h"
#include "filter.h"

/*****************************************************************************
 * Access: local prototypes
 *****************************************************************************/
static block_t *ReadCompressed( access_t * );
static int AccessControl ( access_t *, int, va_list );

static int Demux       ( demux_t * );
static int DemuxControl( demux_t *, int, va_list );

static int OpenDevice( vlc_object_t *, access_sys_t *, string, bool );
static IBaseFilter *FindCaptureDevice( vlc_object_t *, string *,
                                       list<string> *, bool );
static size_t EnumDeviceCaps( vlc_object_t *, IBaseFilter *,
                              int, int, int, int, int, int,
                              AM_MEDIA_TYPE *mt, size_t );
static bool ConnectFilters( vlc_object_t *, access_sys_t *,
                            IBaseFilter *, CaptureFilter * );
static int FindDevicesCallback( vlc_object_t *, char const *,
                                vlc_value_t, vlc_value_t, void * );
static int ConfigDevicesCallback( vlc_object_t *, char const *,
                                  vlc_value_t, vlc_value_t, void * );

static void ShowPropertyPage( IUnknown * );
static void ShowDeviceProperties( vlc_object_t *, ICaptureGraphBuilder2 *,
                                  IBaseFilter *, bool );
static void ShowTunerProperties( vlc_object_t *, ICaptureGraphBuilder2 *,
                                 IBaseFilter *, bool );
static void ConfigTuner( vlc_object_t *, ICaptureGraphBuilder2 *,
                         IBaseFilter * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static const char *const ppsz_vdev[] = { "", "none" };
static const char *const ppsz_vdev_text[] = { N_("Default"), N_("None") };
static const char *const ppsz_adev[] = { "", "none" };
static const char *const ppsz_adev_text[] = { N_("Default"), N_("None") };
static const int pi_tuner_input[] = { 0, 1, 2 };
static const char *const ppsz_tuner_input_text[] =
    {N_("Default"), N_("Cable"), N_("Antenna")};
static const int pi_amtuner_mode[]  = { AMTUNER_MODE_DEFAULT,
                                 AMTUNER_MODE_TV,
                                 AMTUNER_MODE_FM_RADIO,
                                 AMTUNER_MODE_AM_RADIO,
                                 AMTUNER_MODE_DSS };
static const char *const ppsz_amtuner_mode_text[] = { N_("Default"),
                                          N_("TV"),
                                          N_("FM radio"),
                                          N_("AM radio"),
                                          N_("DSS") };

#define CACHING_TEXT N_("Caching value in ms")
#define CACHING_LONGTEXT N_( \
    "Caching value for DirectShow streams. " \
    "This value should be set in milliseconds." )
#define VDEV_TEXT N_("Video device name")
#define VDEV_LONGTEXT N_( \
    "Name of the video device that will be used by the " \
    "DirectShow plugin. If you don't specify anything, the default device " \
    "will be used.")
#define ADEV_TEXT N_("Audio device name")
#define ADEV_LONGTEXT N_( \
    "Name of the audio device that will be used by the " \
    "DirectShow plugin. If you don't specify anything, the default device " \
    "will be used. ")
#define SIZE_TEXT N_("Video size")
#define SIZE_LONGTEXT N_( \
    "Size of the video that will be displayed by the " \
    "DirectShow plugin. If you don't specify anything the default size for " \
    "your device will be used. You can specify a standard size (cif, d1, ...) or <width>x<height>.")
#define ASPECT_TEXT N_("Picture aspect-ratio n:m")
#define ASPECT_LONGTEXT N_("Define input picture aspect-ratio to use. Default is 4:3" )
#define CHROMA_TEXT N_("Video input chroma format")
#define CHROMA_LONGTEXT N_( \
    "Force the DirectShow video input to use a specific chroma format " \
    "(eg. I420 (default), RV24, etc.)")
#define FPS_TEXT N_("Video input frame rate")
#define FPS_LONGTEXT N_( \
    "Force the DirectShow video input to use a specific frame rate" \
    "(eg. 0 means default, 25, 29.97, 50, 59.94, etc.)")
#define CONFIG_TEXT N_("Device properties")
#define CONFIG_LONGTEXT N_( \
    "Show the properties dialog of the selected device before starting the " \
    "stream.")
#define TUNER_TEXT N_("Tuner properties")
#define TUNER_LONGTEXT N_( \
    "Show the tuner properties [channel selection] page." )
#define CHANNEL_TEXT N_("Tuner TV Channel")
#define CHANNEL_LONGTEXT N_( \
    "Set the TV channel the tuner will set to " \
    "(0 means default)." )
#define COUNTRY_TEXT N_("Tuner country code")
#define COUNTRY_LONGTEXT N_( \
    "Set the tuner country code that establishes the current " \
    "channel-to-frequency mapping (0 means default)." )
#define TUNER_INPUT_TEXT N_("Tuner input type")
#define TUNER_INPUT_LONGTEXT N_( \
    "Select the tuner input type (Cable/Antenna)." )
#define VIDEO_IN_TEXT N_("Video input pin")
#define VIDEO_IN_LONGTEXT N_( \
  "Select the video input source, such as composite, s-video, " \
  "or tuner. Since these settings are hardware-specific, you should find good " \
  "settings in the \"Device config\" area, and use those numbers here. -1 " \
  "means that settings will not be changed.")
#define AUDIO_IN_TEXT N_("Audio input pin")
#define AUDIO_IN_LONGTEXT N_( \
  "Select the audio input source. See the \"video input\" option." )
#define VIDEO_OUT_TEXT N_("Video output pin")
#define VIDEO_OUT_LONGTEXT N_( \
  "Select the video output type. See the \"video input\" option." )
#define AUDIO_OUT_TEXT N_("Audio output pin")
#define AUDIO_OUT_LONGTEXT N_( \
  "Select the audio output type. See the \"video input\" option." )

#define AMTUNER_MODE_TEXT N_("AM Tuner mode")
#define AMTUNER_MODE_LONGTEXT N_( \
    "AM Tuner mode. Can be one of Default (0), TV (1)," \
     "AM Radio (2), FM Radio (3) or DSS (4).")

#define AUDIO_CHANNELS_TEXT N_("Number of audio channels")
#define AUDIO_CHANNELS_LONGTEXT N_( \
    "Select audio input format with the given number of audio channels (if non 0)" )

#define AUDIO_SAMPLERATE_TEXT N_("Audio sample rate")
#define AUDIO_SAMPLERATE_LONGTEXT N_( \
    "Select audio input format with the given sample rate (if non 0)" )

#define AUDIO_BITSPERSAMPLE_TEXT N_("Audio bits per sample")
#define AUDIO_BITSPERSAMPLE_LONGTEXT N_( \
    "Select audio input format with the given bits/sample (if non 0)" )

static int  CommonOpen ( vlc_object_t *, access_sys_t *, bool );
static void CommonClose( vlc_object_t *, access_sys_t * );

static int  AccessOpen ( vlc_object_t * );
static void AccessClose( vlc_object_t * );

static int  DemuxOpen  ( vlc_object_t * );
static void DemuxClose ( vlc_object_t * );

vlc_module_begin ()
    set_shortname( N_("DirectShow") )
    set_description( N_("DirectShow input") )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_ACCESS )
    add_integer( "dshow-caching", (mtime_t)(0.2*CLOCK_FREQ) / 1000, NULL,
                 CACHING_TEXT, CACHING_LONGTEXT, true )

    add_string( "dshow-vdev", NULL, NULL, VDEV_TEXT, VDEV_LONGTEXT, false)
        change_string_list( ppsz_vdev, ppsz_vdev_text, FindDevicesCallback )
        change_action_add( FindDevicesCallback, N_("Refresh list") )
        change_action_add( ConfigDevicesCallback, N_("Configure") )

    add_string( "dshow-adev", NULL, NULL, ADEV_TEXT, ADEV_LONGTEXT, false)
        change_string_list( ppsz_adev, ppsz_adev_text, FindDevicesCallback )
        change_action_add( FindDevicesCallback, N_("Refresh list") )
        change_action_add( ConfigDevicesCallback, N_("Configure") )

    add_string( "dshow-size", NULL, NULL, SIZE_TEXT, SIZE_LONGTEXT, false)

    add_string( "dshow-aspect-ratio", "4:3", NULL, ASPECT_TEXT, ASPECT_LONGTEXT, false)

    add_string( "dshow-chroma", NULL, NULL, CHROMA_TEXT, CHROMA_LONGTEXT, true )

    add_float( "dshow-fps", 0.0f, NULL, FPS_TEXT, FPS_LONGTEXT, true )

    add_bool( "dshow-config", false, NULL, CONFIG_TEXT, CONFIG_LONGTEXT, true )

    add_bool( "dshow-tuner", false, NULL, TUNER_TEXT, TUNER_LONGTEXT, true )

    add_integer( "dshow-tuner-channel", 0, NULL, CHANNEL_TEXT, CHANNEL_LONGTEXT,
                true )

    add_integer( "dshow-tuner-country", 0, NULL, COUNTRY_TEXT, COUNTRY_LONGTEXT,
                true )

    add_integer( "dshow-tuner-input", 0, NULL, TUNER_INPUT_TEXT,
                 TUNER_INPUT_LONGTEXT, true )
        change_integer_list( pi_tuner_input, ppsz_tuner_input_text, NULL )

    add_integer( "dshow-video-input",  -1, NULL, VIDEO_IN_TEXT,
                 VIDEO_IN_LONGTEXT, true )

    add_integer( "dshow-video-output", -1, NULL, VIDEO_OUT_TEXT,
                 VIDEO_OUT_LONGTEXT, true )

    add_integer( "dshow-audio-input",  -1, NULL, AUDIO_IN_TEXT,
                 AUDIO_IN_LONGTEXT, true )

    add_integer( "dshow-audio-output", -1, NULL, AUDIO_OUT_TEXT,
                 AUDIO_OUT_LONGTEXT, true )

    add_integer( "dshow-amtuner-mode", AMTUNER_MODE_TV, NULL,
                AMTUNER_MODE_TEXT, AMTUNER_MODE_LONGTEXT, false)
        change_integer_list( pi_amtuner_mode, ppsz_amtuner_mode_text, NULL )

    add_integer( "dshow-audio-channels", 0, NULL, AUDIO_CHANNELS_TEXT,
                 AUDIO_CHANNELS_LONGTEXT, true )
    add_integer( "dshow-audio-samplerate", 0, NULL, AUDIO_SAMPLERATE_TEXT,
                 AUDIO_SAMPLERATE_LONGTEXT, true )
    add_integer( "dshow-audio-bitspersample", 0, NULL, AUDIO_BITSPERSAMPLE_TEXT,
                 AUDIO_BITSPERSAMPLE_LONGTEXT, true )

    add_shortcut( "dshow" )
    set_capability( "access_demux", 0 )
    set_callbacks( DemuxOpen, DemuxClose )

    add_submodule ()
    set_description( N_("DirectShow input") )
    add_shortcut( "dshow" )
    set_capability( "access", 0 )
    set_callbacks( AccessOpen, AccessClose )

vlc_module_end ()

/*****************************************************************************
 * DirectShow elementary stream descriptor
 *****************************************************************************/
typedef struct dshow_stream_t
{
    string          devicename;
    IBaseFilter     *p_device_filter;
    CaptureFilter   *p_capture_filter;
    AM_MEDIA_TYPE   mt;

    union
    {
      VIDEOINFOHEADER video;
      WAVEFORMATEX    audio;

    } header;

    int             i_fourcc;
    es_out_id_t     *p_es;

    bool      b_pts;

    deque<VLCMediaSample> samples_queue;
} dshow_stream_t;

/*****************************************************************************
 * DirectShow utility functions
 *****************************************************************************/
static void CreateDirectShowGraph( access_sys_t *p_sys )
{
    p_sys->i_crossbar_route_depth = 0;

    /* Create directshow filter graph */
    if( SUCCEEDED( CoCreateInstance( CLSID_FilterGraph, 0, CLSCTX_INPROC,
                       (REFIID)IID_IFilterGraph, (void **)&p_sys->p_graph) ) )
    {
        /* Create directshow capture graph builder if available */
        if( SUCCEEDED( CoCreateInstance( CLSID_CaptureGraphBuilder2, 0,
                         CLSCTX_INPROC, (REFIID)IID_ICaptureGraphBuilder2,
                         (void **)&p_sys->p_capture_graph_builder2 ) ) )
        {
            p_sys->p_capture_graph_builder2->
                SetFiltergraph((IGraphBuilder *)p_sys->p_graph);
        }

        p_sys->p_graph->QueryInterface( IID_IMediaControl,
                                        (void **)&p_sys->p_control );
    }
}

static void DeleteDirectShowGraph( access_sys_t *p_sys )
{
    DeleteCrossbarRoutes( p_sys );

    /* Remove filters from graph */
    for( int i = 0; i < p_sys->i_streams; i++ )
    {
        p_sys->p_graph->RemoveFilter( p_sys->pp_streams[i]->p_capture_filter );
        p_sys->p_graph->RemoveFilter( p_sys->pp_streams[i]->p_device_filter );
        p_sys->pp_streams[i]->p_capture_filter->Release();
        p_sys->pp_streams[i]->p_device_filter->Release();
    }

    /* Release directshow objects */
    if( p_sys->p_control )
    {
        p_sys->p_control->Release();
        p_sys->p_control = NULL;
    }
    if( p_sys->p_capture_graph_builder2 )
    {
        p_sys->p_capture_graph_builder2->Release();
        p_sys->p_capture_graph_builder2 = NULL;
    }

    if( p_sys->p_graph )
    {
        p_sys->p_graph->Release();
        p_sys->p_graph = NULL;
    }
}

/*****************************************************************************
 * CommonOpen: open direct show device
 *****************************************************************************/
static int CommonOpen( vlc_object_t *p_this, access_sys_t *p_sys,
                       bool b_access_demux )
{
    int i;
    char *psz_val;

    /* Get/parse options and open device(s) */
    string vdevname, adevname;
    int i_width = 0, i_height = 0;
    vlc_fourcc_t i_chroma = 0;
    bool b_use_audio = true;
    bool b_use_video = true;

    /* Initialize OLE/COM */
    CoInitialize( 0 );

    var_Create( p_this, "dshow-config", VLC_VAR_BOOL | VLC_VAR_DOINHERIT );
    var_Create( p_this, "dshow-tuner", VLC_VAR_BOOL | VLC_VAR_DOINHERIT );
    psz_val = var_CreateGetString( p_this, "dshow-vdev" );
    if( psz_val )
    {
        msg_Dbg( p_this, "dshow-vdev: %s", psz_val ) ;
        /* skip none device */
        if ( strncasecmp( psz_val, "none", 4 ) != 0 )
            vdevname = string( psz_val );
        else
            b_use_video = false ;
    }
    free( psz_val );

    psz_val = var_CreateGetString( p_this, "dshow-adev" );
    if( psz_val )
    {
        msg_Dbg( p_this, "dshow-adev: %s", psz_val ) ;
        /* skip none device */
        if ( strncasecmp( psz_val, "none", 4 ) != 0 )
            adevname = string( psz_val );
        else
            b_use_audio = false ;
    }
    free( psz_val );

    static struct {const char *psz_size; int  i_width; int  i_height;} size_table[] =
    { { "subqcif", 128, 96 }, { "qsif", 160, 120 }, { "qcif", 176, 144 },
      { "sif", 320, 240 }, { "cif", 352, 288 }, { "d1", 640, 480 },
      { 0, 0, 0 },
    };

    psz_val = var_CreateGetString( p_this, "dshow-size" );
    if( !EMPTY_STR(psz_val) )
    {
        for( i = 0; size_table[i].psz_size; i++ )
        {
            if( !strcmp( psz_val, size_table[i].psz_size ) )
            {
                i_width = size_table[i].i_width;
                i_height = size_table[i].i_height;
                break;
            }
        }
        if( !size_table[i].psz_size ) /* Try to parse "WidthxHeight" */
        {
            char *psz_parser;
            i_width = strtol( psz_val, &psz_parser, 0 );
            if( *psz_parser == 'x' || *psz_parser == 'X')
            {
                i_height = strtol( psz_parser + 1, &psz_parser, 0 );
            }
            msg_Dbg( p_this, "width x height %dx%d", i_width, i_height );
        }
    }
    free( psz_val );

    psz_val = var_CreateGetString( p_this, "dshow-chroma" );
    i_chroma = vlc_fourcc_GetCodecFromString( UNKNOWN_ES, psz_val );
    p_sys->b_chroma = i_chroma != 0;
    free( psz_val );

    var_Create( p_this, "dshow-fps", VLC_VAR_FLOAT | VLC_VAR_DOINHERIT );
    var_Create( p_this, "dshow-tuner-channel",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Create( p_this, "dshow-tuner-country",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Create( p_this, "dshow-tuner-input",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );

    var_Create( p_this, "dshow-amtuner-mode",
                VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );

    var_Create( p_this, "dshow-caching", VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );

    var_Create( p_this, "dshow-video-input", VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Create( p_this, "dshow-audio-input", VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Create( p_this, "dshow-video-output", VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );
    var_Create( p_this, "dshow-audio-output", VLC_VAR_INTEGER | VLC_VAR_DOINHERIT );


    /* Initialize some data */
    p_sys->i_streams = 0;
    p_sys->pp_streams = NULL;
    p_sys->i_width = i_width;
    p_sys->i_height = i_height;
    p_sys->i_chroma = i_chroma;

    p_sys->p_graph = NULL;
    p_sys->p_capture_graph_builder2 = NULL;
    p_sys->p_control = NULL;

    /* Build directshow graph */
    CreateDirectShowGraph( p_sys );

    vlc_mutex_init( &p_sys->lock );
    vlc_cond_init( &p_sys->wait );

    if( !b_use_video && !b_use_audio )
    {
        dialog_Fatal( p_this, _("Capture failed"),
                        _("No video or audio device selected.") );
        return VLC_EGENERIC ;
    }

    if( !b_use_video )
        msg_Dbg( p_this, "skipping video device" ) ;
    bool b_err_video = false ;

    if( b_use_video && OpenDevice( p_this, p_sys, vdevname, 0 ) != VLC_SUCCESS )
    {
        msg_Err( p_this, "can't open video device");
        b_err_video = true ;
    }

    if ( b_use_video && !b_err_video )
    {
        /* Check if we can handle the demuxing ourselves or need to spawn
         * a demuxer module */
        dshow_stream_t *p_stream = p_sys->pp_streams[p_sys->i_streams-1];

        if( p_stream->mt.majortype == MEDIATYPE_Video )
        {
            if( /* Raw DV stream */
                p_stream->i_fourcc == VLC_CODEC_DV ||
                /* Raw MPEG video stream */
                p_stream->i_fourcc == VLC_CODEC_MPGV )
            {
                b_use_audio = false;

                if( b_access_demux )
                {
                    /* Let the access (only) take care of that */
                    return VLC_EGENERIC;
                }
            }
        }

        if( p_stream->mt.majortype == MEDIATYPE_Stream )
        {
            b_use_audio = false;

            if( b_access_demux )
            {
                /* Let the access (only) take care of that */
                return VLC_EGENERIC;
            }

            if( var_GetBool( p_this, "dshow-tuner" ) )
            {
                /* FIXME: we do MEDIATYPE_Stream here so we don't do
                 * it twice. */
                ShowTunerProperties( p_this, p_sys->p_capture_graph_builder2,
                                     p_stream->p_device_filter, 0 );
            }
        }
    }

    if( !b_use_audio )
        msg_Dbg( p_this, "skipping audio device") ;

    bool b_err_audio = false ;

    if( b_use_audio && OpenDevice( p_this, p_sys, adevname, 1 ) != VLC_SUCCESS )
    {
        msg_Err( p_this, "can't open audio device");
        b_err_audio = true ;
    }

    if( ( b_use_video && b_err_video && b_use_audio && b_err_audio ) ||
        ( !b_use_video && b_use_audio && b_err_audio ) ||
        ( b_use_video && !b_use_audio && b_err_video ) )
    {
        msg_Err( p_this, "FATAL: could not open ANY device" ) ;
        dialog_Fatal( p_this,  _("Capture failed"),
                        _("VLC cannot open ANY capture device."
                          "Check the error log for details.") );
        return VLC_EGENERIC ;
    }

    for( i = p_sys->i_crossbar_route_depth-1; i >= 0 ; --i )
    {
        int i_val = var_GetInteger( p_this, "dshow-video-input" );
        if( i_val >= 0 )
            p_sys->crossbar_routes[i].VideoInputIndex = i_val;
        i_val = var_GetInteger( p_this, "dshow-video-output" );
        if( i_val >= 0 )
            p_sys->crossbar_routes[i].VideoOutputIndex = i_val;
        i_val = var_GetInteger( p_this, "dshow-audio-input" );
        if( i_val >= 0 )
            p_sys->crossbar_routes[i].AudioInputIndex = i_val;
        i_val = var_GetInteger( p_this, "dshow-audio-output" );
        if( i_val >= 0 )
            p_sys->crossbar_routes[i].AudioOutputIndex = i_val;

        IAMCrossbar *pXbar = p_sys->crossbar_routes[i].pXbar;
        LONG VideoInputIndex = p_sys->crossbar_routes[i].VideoInputIndex;
        LONG VideoOutputIndex = p_sys->crossbar_routes[i].VideoOutputIndex;
        LONG AudioInputIndex = p_sys->crossbar_routes[i].AudioInputIndex;
        LONG AudioOutputIndex = p_sys->crossbar_routes[i].AudioOutputIndex;

        if( SUCCEEDED(pXbar->Route(VideoOutputIndex, VideoInputIndex)) )
        {
            msg_Dbg( p_this, "crossbar at depth %d, routed video "
                     "output %ld to video input %ld", i, VideoOutputIndex,
                     VideoInputIndex );

            if( AudioOutputIndex != -1 && AudioInputIndex != -1 )
            {
                if( SUCCEEDED( pXbar->Route(AudioOutputIndex,
                                            AudioInputIndex)) )
                {
                    msg_Dbg(p_this, "crossbar at depth %d, routed audio "
                            "output %ld to audio input %ld", i,
                            AudioOutputIndex, AudioInputIndex );
                }
            }
        }
        else
            msg_Err( p_this, "crossbar at depth %d could not route video "
                     "output %ld to input %ld", i, VideoOutputIndex, VideoInputIndex );
    }

    /*
    ** Show properties pages from other filters in graph
    */
    if( var_GetBool( p_this, "dshow-config" ) )
    {
        for( i = p_sys->i_crossbar_route_depth-1; i >= 0 ; --i )
        {
            IAMCrossbar *pXbar = p_sys->crossbar_routes[i].pXbar;
            IBaseFilter *p_XF;

            if( SUCCEEDED( pXbar->QueryInterface( IID_IBaseFilter,
                                                  (void **)&p_XF ) ) )
            {
                ShowPropertyPage( p_XF );
                p_XF->Release();
            }
        }
    }

    /* Initialize some data */
    p_sys->i_current_stream = 0;

    if( !p_sys->i_streams ) return VLC_EGENERIC;

    return VLC_SUCCESS;
}

/*****************************************************************************
 * DemuxOpen: open direct show device as an access_demux module
 *****************************************************************************/
static int DemuxOpen( vlc_object_t *p_this )
{
    demux_t      *p_demux = (demux_t *)p_this;
    access_sys_t *p_sys;
    int i;

    p_sys = (access_sys_t*)calloc( 1, sizeof( access_sys_t ) );
    if( !p_sys )
        return VLC_ENOMEM;
    p_demux->p_sys = (demux_sys_t *)p_sys;

    if( CommonOpen( p_this, p_sys, true ) != VLC_SUCCESS )
    {
        CommonClose( p_this, p_sys );
        return VLC_EGENERIC;
    }

    /* Everything is ready. Let's rock baby */
    msg_Dbg( p_this, "Playing...");
    p_sys->p_control->Run();

    p_demux->pf_demux   = Demux;
    p_demux->pf_control = DemuxControl;
    p_demux->info.i_update = 0;
    p_demux->info.i_title = 0;
    p_demux->info.i_seekpoint = 0;

    for( i = 0; i < p_sys->i_streams; i++ )
    {
        dshow_stream_t *p_stream = p_sys->pp_streams[i];
        es_format_t fmt;

        if( p_stream->mt.majortype == MEDIATYPE_Video )
        {
            char *psz_aspect = var_CreateGetString( p_this, "dshow-aspect-ratio" );
            char *psz_delim = !EMPTY_STR( psz_aspect ) ? strchr( psz_aspect, ':' ) : NULL;

            es_format_Init( &fmt, VIDEO_ES, p_stream->i_fourcc );

            fmt.video.i_width  = p_stream->header.video.bmiHeader.biWidth;
            fmt.video.i_height = p_stream->header.video.bmiHeader.biHeight;

            if( psz_delim )
            {
                fmt.video.i_sar_num = atoi( psz_aspect ) * fmt.video.i_height;
                fmt.video.i_sar_den = atoi( psz_delim + 1 ) * fmt.video.i_width;
            }
            else
            {
                fmt.video.i_sar_num = 4 * fmt.video.i_height;
                fmt.video.i_sar_den = 3 * fmt.video.i_width;
            }
            free( psz_aspect );

            if( !p_stream->header.video.bmiHeader.biCompression )
            {
                /* RGB DIB are coded from bottom to top */
                fmt.video.i_height = (unsigned int)(-(int)fmt.video.i_height);
            }

            /* Setup rgb mask for RGB formats */
            if( p_stream->i_fourcc == VLC_CODEC_RGB24 )
            {
                /* This is in RGB format
            http://msdn.microsoft.com/en-us/library/dd407253%28VS.85%29.aspx?ppud=4
                 */
                fmt.video.i_rmask = 0x00ff0000;
                fmt.video.i_gmask = 0x0000ff00;
                fmt.video.i_bmask = 0x000000ff;
            }

            if( p_stream->header.video.AvgTimePerFrame )
            {
                fmt.video.i_frame_rate = 10000000;
                fmt.video.i_frame_rate_base =
                    p_stream->header.video.AvgTimePerFrame;
            }
        }
        else if( p_stream->mt.majortype == MEDIATYPE_Audio )
        {
            es_format_Init( &fmt, AUDIO_ES, p_stream->i_fourcc );

            fmt.audio.i_channels = p_stream->header.audio.nChannels;
            fmt.audio.i_rate = p_stream->header.audio.nSamplesPerSec;
            fmt.audio.i_bitspersample = p_stream->header.audio.wBitsPerSample;
            fmt.audio.i_blockalign = fmt.audio.i_channels *
                fmt.audio.i_bitspersample / 8;
            fmt.i_bitrate = fmt.audio.i_channels * fmt.audio.i_rate *
                fmt.audio.i_bitspersample;
        }

        p_stream->p_es = es_out_Add( p_demux->out, &fmt );
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * AccessOpen: open direct show device as an access module
 *****************************************************************************/
static int AccessOpen( vlc_object_t *p_this )
{
    access_t     *p_access = (access_t*)p_this;
    access_sys_t *p_sys;

    p_access->p_sys = p_sys = (access_sys_t*)calloc( 1, sizeof( access_sys_t ) );
    if( !p_sys )
        return VLC_ENOMEM;

    if( CommonOpen( p_this, p_sys, false ) != VLC_SUCCESS )
    {
        CommonClose( p_this, p_sys );
        return VLC_EGENERIC;
    }

    dshow_stream_t *p_stream = p_sys->pp_streams[0];

    /* Check if we need to force demuxers */
    if( !p_access->psz_demux || !*p_access->psz_demux )
    {
        if( p_stream->i_fourcc == VLC_CODEC_DV )
        {
            free( p_access->psz_demux );
            p_access->psz_demux = strdup( "rawdv" );
        }
        else if( p_stream->i_fourcc == VLC_CODEC_MPGV )
        {
            free( p_access->psz_demux );
            p_access->psz_demux = strdup( "mpgv" );
        }
    }

    /* Setup Access */
    p_access->pf_read = NULL;
    p_access->pf_block = ReadCompressed;
    p_access->pf_control = AccessControl;
    p_access->pf_seek = NULL;
    p_access->info.i_update = 0;
    p_access->info.i_size = 0;
    p_access->info.i_pos = 0;
    p_access->info.b_eof = false;
    p_access->info.i_title = 0;
    p_access->info.i_seekpoint = 0;
    p_access->p_sys = p_sys;

    /* Everything is ready. Let's rock baby */
    msg_Dbg( p_this, "Playing...");
    p_sys->p_control->Run();

    return VLC_SUCCESS;
}

/*****************************************************************************
 * CommonClose: close device
 *****************************************************************************/
static void CommonClose( vlc_object_t *p_this, access_sys_t *p_sys )
{
    msg_Dbg( p_this, "releasing DirectShow");

    DeleteDirectShowGraph( p_sys );

    /* Uninitialize OLE/COM */
    CoUninitialize();

    for( int i = 0; i < p_sys->i_streams; i++ ) delete p_sys->pp_streams[i];
    if( p_sys->i_streams ) free( p_sys->pp_streams );

    vlc_mutex_destroy( &p_sys->lock );
    vlc_cond_destroy( &p_sys->wait );

    free( p_sys );
}

/*****************************************************************************
 * AccessClose: close device
 *****************************************************************************/
static void AccessClose( vlc_object_t *p_this )
{
    access_t     *p_access = (access_t *)p_this;
    access_sys_t *p_sys    = p_access->p_sys;

    /* Stop capturing stuff */
    p_sys->p_control->Stop();

    CommonClose( p_this, p_sys );
}

/*****************************************************************************
 * DemuxClose: close device
 *****************************************************************************/
static void DemuxClose( vlc_object_t *p_this )
{
    demux_t      *p_demux = (demux_t *)p_this;
    access_sys_t *p_sys   = (access_sys_t *)p_demux->p_sys;

    /* Stop capturing stuff */
    p_sys->p_control->Stop();

    CommonClose( p_this, p_sys );
}

/****************************************************************************
 * ConnectFilters
 ****************************************************************************/
static bool ConnectFilters( vlc_object_t *p_this, access_sys_t *p_sys,
                            IBaseFilter *p_filter,
                            CaptureFilter *p_capture_filter )
{
    CapturePin *p_input_pin = p_capture_filter->CustomGetPin();

    AM_MEDIA_TYPE mediaType = p_input_pin->CustomGetMediaType();

    if( p_sys->p_capture_graph_builder2 )
    {
        if( FAILED(p_sys->p_capture_graph_builder2->
                RenderStream( &PIN_CATEGORY_CAPTURE, &mediaType.majortype,
                              p_filter, 0, (IBaseFilter *)p_capture_filter )) )
        {
            return false;
        }

        // Sort out all the possible video inputs
        // The class needs to be given the capture filters ANALOGVIDEO input pin
        IEnumPins *pins = NULL;
        if( ( mediaType.majortype == MEDIATYPE_Video ||
              mediaType.majortype == MEDIATYPE_Stream ) &&
            SUCCEEDED(p_filter->EnumPins(&pins)) )
        {
            IPin        *pP = NULL;
            ULONG        n;
            PIN_INFO     pinInfo;
            BOOL         Found = FALSE;
            IKsPropertySet *pKs = NULL;
            GUID guid;
            DWORD dw;

            while( !Found && ( S_OK == pins->Next(1, &pP, &n) ) )
            {
                if( S_OK == pP->QueryPinInfo(&pinInfo) )
                {
                    // is this pin an ANALOGVIDEOIN input pin?
                    if( pinInfo.dir == PINDIR_INPUT &&
                        pP->QueryInterface( IID_IKsPropertySet,
                                            (void **)&pKs ) == S_OK )
                    {
                        if( pKs->Get( AMPROPSETID_Pin,
                                      AMPROPERTY_PIN_CATEGORY, NULL, 0,
                                      &guid, sizeof(GUID), &dw ) == S_OK )
                        {
                            if( guid == PIN_CATEGORY_ANALOGVIDEOIN )
                            {
                                // recursively search crossbar routes
                                FindCrossbarRoutes( p_this, p_sys, pP, 0 );
                                // found it
                                Found = TRUE;
                            }
                        }
                        pKs->Release();
                    }
                    pinInfo.pFilter->Release();
                }
                pP->Release();
            }
            pins->Release();
            msg_Dbg( p_this, "ConnectFilters: graph_builder2 available.") ;
            if ( !Found )
                msg_Warn( p_this, "ConnectFilters: No crossBar routes found (incobatible pin types)" ) ;
        }
        return true;
    }
    else
    {
        IEnumPins *p_enumpins;
        IPin *p_pin;

        if( S_OK != p_filter->EnumPins( &p_enumpins ) ) return false;

        while( S_OK == p_enumpins->Next( 1, &p_pin, NULL ) )
        {
            PIN_DIRECTION pin_dir;
            p_pin->QueryDirection( &pin_dir );

            if( pin_dir == PINDIR_OUTPUT &&
                p_sys->p_graph->ConnectDirect( p_pin, (IPin *)p_input_pin,
                                               0 ) == S_OK )
            {
                p_pin->Release();
                p_enumpins->Release();
                return true;
            }
            p_pin->Release();
        }

        p_enumpins->Release();
        return false;
    }
}

/*
 * get fourcc priority from arbritary preference, the higher the better
 */
static int GetFourCCPriority( int i_fourcc )
{
    switch( i_fourcc )
    {
    case VLC_CODEC_I420:
    case VLC_CODEC_FL32:
        return 9;
    case VLC_CODEC_YV12:
    case VLC_FOURCC('a','r','a','w'):
        return 8;
    case VLC_CODEC_RGB24:
        return 7;
    case VLC_CODEC_YUYV:
    case VLC_CODEC_RGB32:
    case VLC_CODEC_RGBA:
        return 6;
    }

    return 0;
}

#define MAX_MEDIA_TYPES 32

static int OpenDevice( vlc_object_t *p_this, access_sys_t *p_sys,
                       string devicename, bool b_audio )
{
    /* See if device is already opened */
    for( int i = 0; i < p_sys->i_streams; i++ )
    {
        if( devicename.size() &&
            p_sys->pp_streams[i]->devicename == devicename )
        {
            /* Already opened */
            return VLC_SUCCESS;
        }
    }

    list<string> list_devices;

    /* Enumerate devices and display their names */
    FindCaptureDevice( p_this, NULL, &list_devices, b_audio );
    if( !list_devices.size() )
        return VLC_EGENERIC;

    list<string>::iterator iter;
    for( iter = list_devices.begin(); iter != list_devices.end(); iter++ )
        msg_Dbg( p_this, "found device: %s", iter->c_str() );

    /* If no device name was specified, pick the 1st one */
    if( devicename.size() == 0 )
    {
        /* When none selected */
        devicename = *list_devices.begin();
        msg_Dbg( p_this, "asking for default device: %s", devicename.c_str() ) ;
    }
    else
        msg_Dbg( p_this, "asking for device: %s", devicename.c_str() ) ;
    // Use the system device enumerator and class enumerator to find
    // a capture/preview device, such as a desktop USB video camera.
    IBaseFilter *p_device_filter =
        FindCaptureDevice( p_this, &devicename, NULL, b_audio );

    if( p_device_filter )
        msg_Dbg( p_this, "using device: %s", devicename.c_str() );
    else
    {
        msg_Err( p_this, "can't use device: %s, unsupported device type",
                 devicename.c_str() );
        dialog_Fatal( p_this, _("Capture failed"),
                        _("VLC cannot use the device \"%s\", because its "
                          "type is not supported."), devicename.c_str() );
        return VLC_EGENERIC;
    }

    // Retreive acceptable media types supported by device
    AM_MEDIA_TYPE media_types[MAX_MEDIA_TYPES];
    size_t media_count =
        EnumDeviceCaps( p_this, p_device_filter, b_audio ? 0 : p_sys->i_chroma,
                        p_sys->i_width, p_sys->i_height,
      b_audio ? var_CreateGetInteger( p_this, "dshow-audio-channels" ) : 0,
      b_audio ? var_CreateGetInteger( p_this, "dshow-audio-samplerate" ) : 0,
      b_audio ? var_CreateGetInteger( p_this, "dshow-audio-bitspersample" ) : 0,
      media_types, MAX_MEDIA_TYPES );

    AM_MEDIA_TYPE *mt = NULL;

    if( media_count > 0 )
    {
        mt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * media_count);

        // Order and copy returned media types according to arbitrary
        // fourcc priority
        for( size_t c = 0; c < media_count; c++ )
        {
            int slot_priority =
                GetFourCCPriority(GetFourCCFromMediaType(media_types[c]));
            size_t slot_copy = c;
            for( size_t d = c+1; d < media_count; d++ )
            {
                int priority =
                    GetFourCCPriority(GetFourCCFromMediaType(media_types[d]));
                if( priority > slot_priority )
                {
                    slot_priority = priority;
                    slot_copy = d;
                }
            }
            if( slot_copy != c )
            {
                mt[c] = media_types[slot_copy];
                media_types[slot_copy] = media_types[c];
            }
            else
            {
                mt[c] = media_types[c];
            }
        }
    }
    else {
        /* capture device */
        msg_Err( p_this, "capture device '%s' does not support required parameters !", devicename.c_str() );
        dialog_Fatal( p_this, _("Capture failed"),
                        _("The capture device \"%s\" does not support the "
                          "required parameters."), devicename.c_str() );
        p_device_filter->Release();
        return VLC_EGENERIC;
    }

    /* Create and add our capture filter */
    CaptureFilter *p_capture_filter =
        new CaptureFilter( p_this, p_sys, mt, media_count );
    p_sys->p_graph->AddFilter( p_capture_filter, 0 );

    /* Add the device filter to the graph (seems necessary with VfW before
     * accessing pin attributes). */
    p_sys->p_graph->AddFilter( p_device_filter, 0 );

    /* Attempt to connect one of this device's capture output pins */
    msg_Dbg( p_this, "connecting filters" );
    if( ConnectFilters( p_this, p_sys, p_device_filter, p_capture_filter ) )
    {
        /* Success */
        msg_Dbg( p_this, "filters connected successfully !" );

        dshow_stream_t dshow_stream;
        dshow_stream.b_pts = false;
        dshow_stream.p_es = 0;
        dshow_stream.mt =
            p_capture_filter->CustomGetPin()->CustomGetMediaType();

        /* Show Device properties. Done here so the VLC stream is setup with
         * the proper parameters. */
        if( var_GetBool( p_this, "dshow-config" ) )
        {
            ShowDeviceProperties( p_this, p_sys->p_capture_graph_builder2,
                                  p_device_filter, b_audio );
        }

        ConfigTuner( p_this, p_sys->p_capture_graph_builder2,
                     p_device_filter );

        if( var_GetBool( p_this, "dshow-tuner" ) &&
            dshow_stream.mt.majortype != MEDIATYPE_Stream )
        {
            /* FIXME: we do MEDIATYPE_Stream later so we don't do it twice. */
            ShowTunerProperties( p_this, p_sys->p_capture_graph_builder2,
                                 p_device_filter, b_audio );
        }

        dshow_stream.mt =
            p_capture_filter->CustomGetPin()->CustomGetMediaType();

        dshow_stream.i_fourcc = GetFourCCFromMediaType( dshow_stream.mt );
        if( dshow_stream.i_fourcc )
        {
            if( dshow_stream.mt.majortype == MEDIATYPE_Video )
            {
                dshow_stream.header.video =
                    *(VIDEOINFOHEADER *)dshow_stream.mt.pbFormat;
                msg_Dbg( p_this, "MEDIATYPE_Video" );
                msg_Dbg( p_this, "selected video pin accepts format: %4.4s",
                         (char *)&dshow_stream.i_fourcc);
            }
            else if( dshow_stream.mt.majortype == MEDIATYPE_Audio )
            {
                dshow_stream.header.audio =
                    *(WAVEFORMATEX *)dshow_stream.mt.pbFormat;
                msg_Dbg( p_this, "MEDIATYPE_Audio" );
                msg_Dbg( p_this, "selected audio pin accepts format: %4.4s",
                         (char *)&dshow_stream.i_fourcc);
            }
            else if( dshow_stream.mt.majortype == MEDIATYPE_Stream )
            {
                msg_Dbg( p_this, "MEDIATYPE_Stream" );
                msg_Dbg( p_this, "selected stream pin accepts format: %4.4s",
                         (char *)&dshow_stream.i_fourcc);
            }
            else
            {
                msg_Dbg( p_this, "unknown stream majortype" );
                goto fail;
            }

            /* Add directshow elementary stream to our list */
            dshow_stream.p_device_filter = p_device_filter;
            dshow_stream.p_capture_filter = p_capture_filter;

            p_sys->pp_streams = (dshow_stream_t **)xrealloc( p_sys->pp_streams,
                          sizeof(dshow_stream_t *) * (p_sys->i_streams + 1) );
            p_sys->pp_streams[p_sys->i_streams] = new dshow_stream_t;
            *p_sys->pp_streams[p_sys->i_streams++] = dshow_stream;

            return VLC_SUCCESS;
        }
    }

 fail:
    /* Remove filters from graph */
    p_sys->p_graph->RemoveFilter( p_device_filter );
    p_sys->p_graph->RemoveFilter( p_capture_filter );

    /* Release objects */
    p_device_filter->Release();
    p_capture_filter->Release();

    return VLC_EGENERIC;
}

/* FindCaptureDevices:: This Function had two purposes :
    Returns the list of capture devices when p_listdevices != NULL
    Creates an IBaseFilter when p_devicename corresponds to an existing devname
   These actions *may* be requested whith a single call.
*/
static IBaseFilter *
FindCaptureDevice( vlc_object_t *p_this, string *p_devicename,
                   list<string> *p_listdevices, bool b_audio )
{
    IBaseFilter *p_base_filter = NULL;
    IMoniker *p_moniker = NULL;
    ULONG i_fetched;
    HRESULT hr;
    list<string> devicelist;

    /* Create the system device enumerator */
    ICreateDevEnum *p_dev_enum = NULL;

    hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           IID_ICreateDevEnum, (void **)&p_dev_enum );
    if( FAILED(hr) )
    {
        msg_Err( p_this, "failed to create the device enumerator (0x%lx)", hr);
        return NULL;
    }

    /* Create an enumerator for the video capture devices */
    IEnumMoniker *p_class_enum = NULL;
    if( !b_audio )
        hr = p_dev_enum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory,
                                                &p_class_enum, 0 );
    else
        hr = p_dev_enum->CreateClassEnumerator( CLSID_AudioInputDeviceCategory,
                                                &p_class_enum, 0 );
    p_dev_enum->Release();
    if( FAILED(hr) )
    {
        msg_Err( p_this, "failed to create the class enumerator (0x%lx)", hr );
        return NULL;
    }

    /* If there are no enumerators for the requested type, then
     * CreateClassEnumerator will succeed, but p_class_enum will be NULL */
    if( p_class_enum == NULL )
    {
        msg_Err( p_this, "no %s capture device was detected", ( b_audio ? "audio" : "video" ) );
        return NULL;
    }

    /* Enumerate the devices */

    /* Note that if the Next() call succeeds but there are no monikers,
     * it will return S_FALSE (which is not a failure). Therefore, we check
     * that the return code is S_OK instead of using SUCCEEDED() macro. */

    while( p_class_enum->Next( 1, &p_moniker, &i_fetched ) == S_OK )
    {
        /* Getting the property page to get the device name */
        IPropertyBag *p_bag;
        hr = p_moniker->BindToStorage( 0, 0, IID_IPropertyBag,
                                       (void **)&p_bag );
        if( SUCCEEDED(hr) )
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = p_bag->Read( L"FriendlyName", &var, NULL );
            p_bag->Release();
            if( SUCCEEDED(hr) )
            {
                char *p_buf = FromWide( var.bstrVal );
                string devname = string(p_buf);
                free( p_buf) ;

                int dup = 0;
                /* find out if this name is already used by a previously found device */
                list<string>::const_iterator iter = devicelist.begin();
                list<string>::const_iterator end = devicelist.end();
                string ordevname = devname ;
                while ( iter != end )
                {
                    if( 0 == (*iter).compare( devname ) )
                    { /* devname is on the list. Try another name with sequence
                         number apended and then rescan until a unique entry is found*/
                         char seq[16];
                         snprintf(seq, 16, " #%d", ++dup);
                         devname = ordevname + seq;
                         iter = devicelist.begin();
                    }
                    else
                         ++iter;
                }
                devicelist.push_back( devname );

                if( p_devicename && *p_devicename == devname )
                {
                    msg_Dbg( p_this, "asked for %s, binding to %s", p_devicename->c_str() , devname.c_str() ) ;
                    /* NULL possibly means we don't need BindMoniker BindCtx ?? */
                    hr = p_moniker->BindToObject( NULL, 0, IID_IBaseFilter,
                                                  (void **)&p_base_filter );
                    if( FAILED(hr) )
                    {
                        msg_Err( p_this, "couldn't bind moniker to filter "
                                 "object (0x%lx)", hr );
                        p_moniker->Release();
                        p_class_enum->Release();
                        return NULL;
                    }
                    p_moniker->Release();
                    p_class_enum->Release();
                    return p_base_filter;
                }
            }
        }

        p_moniker->Release();
    }

    p_class_enum->Release();

    if( p_listdevices ) {
    devicelist.sort();
    *p_listdevices = devicelist;
    }
    return NULL;
}

static size_t EnumDeviceCaps( vlc_object_t *p_this, IBaseFilter *p_filter,
                              int i_fourcc, int i_width, int i_height,
                              int i_channels, int i_samplespersec,
                              int i_bitspersample, AM_MEDIA_TYPE *mt,
                              size_t mt_max )
{
    IEnumPins *p_enumpins;
    IPin *p_output_pin;
    IEnumMediaTypes *p_enummt;
    size_t mt_count = 0;

    LONGLONG i_AvgTimePerFrame = 0;
    float r_fps = var_GetFloat( p_this, "dshow-fps" );
    if( r_fps )
        i_AvgTimePerFrame = 10000000000LL/(LONGLONG)(r_fps*1000.0f);

    if( FAILED(p_filter->EnumPins( &p_enumpins )) )
    {
        msg_Dbg( p_this, "EnumDeviceCaps failed: no pin enumeration !");
        return 0;
    }

    while( S_OK == p_enumpins->Next( 1, &p_output_pin, NULL ) )
    {
        PIN_INFO info;

        if( S_OK == p_output_pin->QueryPinInfo( &info ) )
        {
            msg_Dbg( p_this, "EnumDeviceCaps: %s pin: %S",
                     info.dir == PINDIR_INPUT ? "input" : "output",
                     info.achName );
            if( info.pFilter ) info.pFilter->Release();
        }

        p_output_pin->Release();
    }

    p_enumpins->Reset();

    while( !mt_count && p_enumpins->Next( 1, &p_output_pin, NULL ) == S_OK )
    {
        PIN_INFO info;

        if( S_OK == p_output_pin->QueryPinInfo( &info ) )
        {
            if( info.pFilter ) info.pFilter->Release();
            if( info.dir == PINDIR_INPUT )
            {
                p_output_pin->Release();
                continue;
            }
            msg_Dbg( p_this, "EnumDeviceCaps: trying pin %S", info.achName );
        }

        AM_MEDIA_TYPE *p_mt;

        /*
        ** Configure pin with a default compatible media if possible
        */

        IAMStreamConfig *pSC;
        if( SUCCEEDED(p_output_pin->QueryInterface( IID_IAMStreamConfig,
                                            (void **)&pSC )) )
        {
            int piCount, piSize;
            if( SUCCEEDED(pSC->GetNumberOfCapabilities(&piCount, &piSize)) )
            {
                BYTE *pSCC= (BYTE *)CoTaskMemAlloc(piSize);
                if( NULL != pSCC )
                {
                    int i_priority = -1;
                    for( int i=0; i<piCount; ++i )
                    {
                        if( SUCCEEDED(pSC->GetStreamCaps(i, &p_mt, pSCC)) )
                        {
                            int i_current_fourcc = GetFourCCFromMediaType( *p_mt );
                            int i_current_priority = GetFourCCPriority(i_current_fourcc);

                            if( (i_fourcc && (i_current_fourcc != i_fourcc))
                             || (i_priority > i_current_priority) )
                            {
                                // unwanted chroma, try next media type
                                FreeMediaType( *p_mt );
                                CoTaskMemFree( (PVOID)p_mt );
                                continue;
                            }

                            if( MEDIATYPE_Video == p_mt->majortype
                                    && FORMAT_VideoInfo == p_mt->formattype )
                            {
                                VIDEO_STREAM_CONFIG_CAPS *pVSCC = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(pSCC);
                                VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(p_mt->pbFormat);

                                if( i_AvgTimePerFrame )
                                {
                                    if( pVSCC->MinFrameInterval > i_AvgTimePerFrame
                                      || i_AvgTimePerFrame > pVSCC->MaxFrameInterval )
                                    {
                                        // required frame rate not compatible, try next media type
                                        FreeMediaType( *p_mt );
                                        CoTaskMemFree( (PVOID)p_mt );
                                        continue;
                                    }
                                    pVih->AvgTimePerFrame = i_AvgTimePerFrame;
                                }

                                if( i_width )
                                {
                                    if((   !pVSCC->OutputGranularityX
                                           && i_width != pVSCC->MinOutputSize.cx
                                           && i_width != pVSCC->MaxOutputSize.cx)
                                       ||
                                       (   pVSCC->OutputGranularityX
                                           && ((i_width % pVSCC->OutputGranularityX)
                                               || pVSCC->MinOutputSize.cx > i_width
                                               || i_width > pVSCC->MaxOutputSize.cx )))
                                    {
                                        // required width not compatible, try next media type
                                        FreeMediaType( *p_mt );
                                        CoTaskMemFree( (PVOID)p_mt );
                                        continue;
                                    }
                                    pVih->bmiHeader.biWidth = i_width;
                                }

                                if( i_height )
                                {
                                    if((   !pVSCC->OutputGranularityY
                                           && i_height != pVSCC->MinOutputSize.cy
                                           && i_height != pVSCC->MaxOutputSize.cy)
                                       ||
                                       (   pVSCC->OutputGranularityY
                                           && ((i_height % pVSCC->OutputGranularityY)
                                               || pVSCC->MinOutputSize.cy > i_height
                                               || i_height > pVSCC->MaxOutputSize.cy )))
                                    {
                                        // required height not compatible, try next media type
                                        FreeMediaType( *p_mt );
                                        CoTaskMemFree( (PVOID)p_mt );
                                        continue;
                                    }
                                    pVih->bmiHeader.biHeight = i_height;
                                }

                                // Set the sample size and image size.
                                // (Round the image width up to a DWORD boundary.)
                                p_mt->lSampleSize = pVih->bmiHeader.biSizeImage =
                                    ((pVih->bmiHeader.biWidth + 3) & ~3) *
                                    pVih->bmiHeader.biHeight * (pVih->bmiHeader.biBitCount>>3);

                                // no cropping, use full video input buffer
                                memset(&(pVih->rcSource), 0, sizeof(RECT));
                                memset(&(pVih->rcTarget), 0, sizeof(RECT));

                                // select this format as default
                                if( SUCCEEDED( pSC->SetFormat(p_mt) ) )
                                {
                                    i_priority = i_current_priority;
                                    if( i_fourcc )
                                        // no need to check any more media types
                                        i = piCount;
                                }
                            }
                            else if( p_mt->majortype == MEDIATYPE_Audio
                                    && p_mt->formattype == FORMAT_WaveFormatEx )
                            {
                                AUDIO_STREAM_CONFIG_CAPS *pASCC = reinterpret_cast<AUDIO_STREAM_CONFIG_CAPS*>(pSCC);
                                WAVEFORMATEX *pWfx = reinterpret_cast<WAVEFORMATEX*>(p_mt->pbFormat);

                                if( i_current_fourcc && (WAVE_FORMAT_PCM == pWfx->wFormatTag) )
                                {
                                    int val = i_channels;
                                    if( ! val )
                                        val = 2;

                                    if( (   !pASCC->ChannelsGranularity
                                            && (unsigned int)val != pASCC->MinimumChannels
                                            && (unsigned int)val != pASCC->MaximumChannels)
                                        ||
                                        (   pASCC->ChannelsGranularity
                                            && ((val % pASCC->ChannelsGranularity)
                                                || (unsigned int)val < pASCC->MinimumChannels
                                                || (unsigned int)val > pASCC->MaximumChannels)))
                                    {
                                        // required number channels not available, try next media type
                                        FreeMediaType( *p_mt );
                                        CoTaskMemFree( (PVOID)p_mt );
                                        continue;
                                    }
                                    pWfx->nChannels = val;

                                    val = i_samplespersec;
                                    if( ! val )
                                        val = 44100;

                                    if( (   !pASCC->SampleFrequencyGranularity
                                            && (unsigned int)val != pASCC->MinimumSampleFrequency
                                            && (unsigned int)val != pASCC->MaximumSampleFrequency)
                                        ||
                                        (   pASCC->SampleFrequencyGranularity
                                            && ((val % pASCC->SampleFrequencyGranularity)
                                                || (unsigned int)val < pASCC->MinimumSampleFrequency
                                                || (unsigned int)val > pASCC->MaximumSampleFrequency )))
                                    {
                                        // required sampling rate not available, try next media type
                                        FreeMediaType( *p_mt );
                                        CoTaskMemFree( (PVOID)p_mt );
                                        continue;
                                    }
                                    pWfx->nSamplesPerSec = val;
 
                                    val = i_bitspersample;
                                    if( ! val )
                                    {
                                        if( VLC_CODEC_FL32 == i_current_fourcc )
                                            val = 32;
                                        else
                                            val = 16;
                                    }

                                    if( (   !pASCC->BitsPerSampleGranularity
                                            && (unsigned int)val != pASCC->MinimumBitsPerSample
                                            &&	(unsigned int)val != pASCC->MaximumBitsPerSample )
                                        ||
                                        (   pASCC->BitsPerSampleGranularity
                                            && ((val % pASCC->BitsPerSampleGranularity)
                                                || (unsigned int)val < pASCC->MinimumBitsPerSample
                                                || (unsigned int)val > pASCC->MaximumBitsPerSample )))
                                    {
                                        // required sample size not available, try next media type
                                        FreeMediaType( *p_mt );
                                        CoTaskMemFree( (PVOID)p_mt );
                                        continue;
                                    }

                                    pWfx->wBitsPerSample = val;
                                    pWfx->nBlockAlign = (pWfx->wBitsPerSample * pWfx->nChannels)/8;
                                    pWfx->nAvgBytesPerSec = pWfx->nSamplesPerSec * pWfx->nBlockAlign;

                                    // select this format as default
                                    if( SUCCEEDED( pSC->SetFormat(p_mt) ) )
                                    {
                                        i_priority = i_current_priority;
                                    }
                                }
                            }
                            FreeMediaType( *p_mt );
                            CoTaskMemFree( (PVOID)p_mt );
                        }
                    }
                    CoTaskMemFree( (LPVOID)pSCC );
                    if( i_priority >= 0 )
                        msg_Dbg( p_this, "EnumDeviceCaps: input pin default format configured");
                }
            }
            pSC->Release();
        }

        /*
        ** Probe pin for available medias (may be a previously configured one)
        */

        if( FAILED( p_output_pin->EnumMediaTypes( &p_enummt ) ) )
        {
            p_output_pin->Release();
            continue;
        }

        while( p_enummt->Next( 1, &p_mt, NULL ) == S_OK )
        {
            int i_current_fourcc = GetFourCCFromMediaType( *p_mt );
            if( i_current_fourcc && p_mt->majortype == MEDIATYPE_Video
                && p_mt->formattype == FORMAT_VideoInfo )
            {
                int i_current_width = ((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biWidth;
                int i_current_height = ((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biHeight;
                LONGLONG i_current_atpf = ((VIDEOINFOHEADER *)p_mt->pbFormat)->AvgTimePerFrame;

                if( i_current_height < 0 )
                    i_current_height = -i_current_height;

                msg_Dbg( p_this, "EnumDeviceCaps: input pin "
                         "accepts chroma: %4.4s, width:%i, height:%i, fps:%f",
                         (char *)&i_current_fourcc, i_current_width,
                         i_current_height, (10000000.0f/((float)i_current_atpf)) );

                if( ( !i_fourcc || i_fourcc == i_current_fourcc ) &&
                    ( !i_width || i_width == i_current_width ) &&
                    ( !i_height || i_height == i_current_height ) &&
                    ( !i_AvgTimePerFrame || i_AvgTimePerFrame == i_current_atpf ) &&
                    mt_count < mt_max )
                {
                    /* Pick match */
                    mt[mt_count++] = *p_mt;
                }
                else FreeMediaType( *p_mt );
            }
            else if( i_current_fourcc && p_mt->majortype == MEDIATYPE_Audio
                    && p_mt->formattype == FORMAT_WaveFormatEx)
            {
                int i_current_channels =
                    ((WAVEFORMATEX *)p_mt->pbFormat)->nChannels;
                int i_current_samplespersec =
                    ((WAVEFORMATEX *)p_mt->pbFormat)->nSamplesPerSec;
                int i_current_bitspersample =
                    ((WAVEFORMATEX *)p_mt->pbFormat)->wBitsPerSample;

                msg_Dbg( p_this, "EnumDeviceCaps: input pin "
                         "accepts format: %4.4s, channels:%i, "
                         "samples/sec:%i bits/sample:%i",
                         (char *)&i_current_fourcc, i_current_channels,
                         i_current_samplespersec, i_current_bitspersample);

                if( (!i_channels || i_channels == i_current_channels) &&
                    (!i_samplespersec ||
                     i_samplespersec == i_current_samplespersec) &&
                    (!i_bitspersample ||
                     i_bitspersample == i_current_bitspersample) &&
                    mt_count < mt_max )
                {
                    /* Pick  match */
                    mt[mt_count++] = *p_mt;

                    /* Setup a few properties like the audio latency */
                    IAMBufferNegotiation *p_ambuf;
                    if( SUCCEEDED( p_output_pin->QueryInterface(
                          IID_IAMBufferNegotiation, (void **)&p_ambuf ) ) )
                    {
                        ALLOCATOR_PROPERTIES AllocProp;
                        AllocProp.cbAlign = -1;

                        /* 100 ms of latency */
                        AllocProp.cbBuffer = i_current_channels *
                          i_current_samplespersec *
                          i_current_bitspersample / 8 / 10;

                        AllocProp.cbPrefix = -1;
                        AllocProp.cBuffers = -1;
                        p_ambuf->SuggestAllocatorProperties( &AllocProp );
                        p_ambuf->Release();
                    }
                }
                else FreeMediaType( *p_mt );
            }
            else if( i_current_fourcc && p_mt->majortype == MEDIATYPE_Stream )
            {
                msg_Dbg( p_this, "EnumDeviceCaps: input pin "
                         "accepts stream format: %4.4s",
                         (char *)&i_current_fourcc );

                if( ( !i_fourcc || i_fourcc == i_current_fourcc ) &&
                    mt_count < mt_max )
                {
                    /* Pick match */
                    mt[mt_count++] = *p_mt;
                    i_fourcc = i_current_fourcc;
                }
                else FreeMediaType( *p_mt );
            }
            else
            {
                const char * psz_type = "unknown";
                if( p_mt->majortype == MEDIATYPE_Video ) psz_type = "video";
                if( p_mt->majortype == MEDIATYPE_Audio ) psz_type = "audio";
                if( p_mt->majortype == MEDIATYPE_Stream ) psz_type = "stream";
                msg_Dbg( p_this, "EnumDeviceCaps: input pin media: unsupported format "
                         "(%s %4.4s)", psz_type, (char *)&p_mt->subtype );

                FreeMediaType( *p_mt );
            }
            CoTaskMemFree( (PVOID)p_mt );
        }

        if( !mt_count && p_enummt->Reset() == S_OK )
        {
            // VLC did not find any supported MEDIATYPE for this output pin.
            // However the graph builder might insert converter filters in
            // the graph if we use a different codec in VLC filter input pin.
            // however, in order to avoid nasty surprises, make use of this
            // facility only for known unsupported codecs.

            while( !mt_count && p_enummt->Next( 1, &p_mt, NULL ) == S_OK )
            {
                // the first four bytes of subtype GUID contains the codec FOURCC
                const char *pfcc = (char *)&p_mt->subtype;
                int i_current_fourcc = VLC_FOURCC(pfcc[0], pfcc[1], pfcc[2], pfcc[3]);
                if( VLC_FOURCC('H','C','W','2') == i_current_fourcc
                 && p_mt->majortype == MEDIATYPE_Video )
                {
                    // output format for 'Hauppauge WinTV PVR PCI II Capture'
                    // try I420 as an input format
                    i_current_fourcc = VLC_CODEC_I420;
                    if( !i_fourcc || i_fourcc == i_current_fourcc )
                    {
                        // return alternative media type
                        AM_MEDIA_TYPE mtr;
                        VIDEOINFOHEADER vh;
 
                        mtr.majortype            = MEDIATYPE_Video;
                        mtr.subtype              = MEDIASUBTYPE_I420;
                        mtr.bFixedSizeSamples    = TRUE;
                        mtr.bTemporalCompression = FALSE;
                        mtr.pUnk                 = NULL;
                        mtr.formattype           = FORMAT_VideoInfo;
                        mtr.cbFormat             = sizeof(vh);
                        mtr.pbFormat             = (BYTE *)&vh;
 
                        memset(&vh, 0, sizeof(vh));
 
                        vh.bmiHeader.biSize   = sizeof(vh.bmiHeader);
                        vh.bmiHeader.biWidth  = i_width > 0 ? i_width :
                            ((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biWidth;
                        vh.bmiHeader.biHeight = i_height > 0 ? i_height :
                            ((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biHeight;
                        vh.bmiHeader.biPlanes      = 3;
                        vh.bmiHeader.biBitCount    = 12;
                        vh.bmiHeader.biCompression = VLC_CODEC_I420;
                        vh.bmiHeader.biSizeImage   = vh.bmiHeader.biWidth * 12 *
                            vh.bmiHeader.biHeight / 8;
                        mtr.lSampleSize            = vh.bmiHeader.biSizeImage;
 
                        msg_Dbg( p_this, "EnumDeviceCaps: input pin media: using 'I420' in place of unsupported format 'HCW2'");

                        if( SUCCEEDED(CopyMediaType(mt+mt_count, &mtr)) )
                            ++mt_count;
                    }
                }
                FreeMediaType( *p_mt );
            }
        }

        p_enummt->Release();
        p_output_pin->Release();
    }

    p_enumpins->Release();
    return mt_count;
}

/*****************************************************************************
 * ReadCompressed: reads compressed (MPEG/DV) data from the device.
 *****************************************************************************
 * Returns -1 in case of error, 0 in case of EOF, otherwise the number of
 * bytes.
 *****************************************************************************/
static block_t *ReadCompressed( access_t *p_access )
{
    access_sys_t   *p_sys = p_access->p_sys;
    dshow_stream_t *p_stream = NULL;
    VLCMediaSample sample;

    /* Read 1 DV/MPEG frame (they contain the video and audio data) */

    /* There must be only 1 elementary stream to produce a valid stream
     * of MPEG or DV data */
    p_stream = p_sys->pp_streams[0];

    while( 1 )
    {
        if( !vlc_object_alive (p_access) ) return NULL;

        /* Get new sample/frame from the elementary stream (blocking). */
        vlc_mutex_lock( &p_sys->lock );

        if( p_stream->p_capture_filter->CustomGetPin()
              ->CustomGetSample( &sample ) != S_OK )
        {
            /* No data available. Wait until some data has arrived */
            vlc_cond_wait( &p_sys->wait, &p_sys->lock );
            vlc_mutex_unlock( &p_sys->lock );
            continue;
        }

        vlc_mutex_unlock( &p_sys->lock );

        /*
         * We got our sample
         */
        block_t *p_block;
        uint8_t *p_data;
        int i_data_size = sample.p_sample->GetActualDataLength();

        if( !i_data_size || !(p_block = block_New( p_access, i_data_size )) )
        {
            sample.p_sample->Release();
            continue;
        }

        sample.p_sample->GetPointer( &p_data );
        vlc_memcpy( p_block->p_buffer, p_data, i_data_size );
        sample.p_sample->Release();

        /* The caller got what he wanted */
        return p_block;
    }

    return NULL; /* never reached */
}

/****************************************************************************
 * Demux:
 ****************************************************************************/
static int Demux( demux_t *p_demux )
{
    access_sys_t *p_sys = (access_sys_t *)p_demux->p_sys;
    int i_stream;
    int i_found_samples;

    i_found_samples = 0;
    vlc_mutex_lock( &p_sys->lock );

    while ( !i_found_samples )
    {
        /* Try to grab samples from all streams */
        for( i_stream = 0; i_stream < p_sys->i_streams; i_stream++ )
        {
            dshow_stream_t *p_stream = p_sys->pp_streams[i_stream];
            if( p_stream->p_capture_filter &&
                p_stream->p_capture_filter->CustomGetPin()
                ->CustomGetSamples( p_stream->samples_queue ) == S_OK )
            {
                i_found_samples = 1;
            }
        }

        if ( !i_found_samples)
        {
            /* Didn't find any audio nor video sample, just wait till the
             * dshow thread pushes some samples */
            vlc_cond_wait( &p_sys->wait, &p_sys->lock );
            /* Some DShow thread pushed data, or the OS broke the wait all
             * by itself. In all cases, it's *strongly* advised to test the
             * condition again, so let the loop do the test again */
        }
    }

    vlc_mutex_unlock( &p_sys->lock );

    for ( i_stream = 0; i_stream < p_sys->i_streams; i_stream++ )
    {
        int i_samples;
        dshow_stream_t *p_stream = p_sys->pp_streams[i_stream];

        i_samples = p_stream->samples_queue.size();
        while ( i_samples > 0 )
        {
            int i_data_size;
            uint8_t *p_data;
            block_t *p_block;
            VLCMediaSample sample;

            sample = p_stream->samples_queue.front();
            p_stream->samples_queue.pop_front();

            i_data_size = sample.p_sample->GetActualDataLength();
            sample.p_sample->GetPointer( &p_data );

            REFERENCE_TIME i_pts, i_end_date;
            HRESULT hr = sample.p_sample->GetTime( &i_pts, &i_end_date );
            if( hr != VFW_S_NO_STOP_TIME && hr != S_OK ) i_pts = 0;

            if( !i_pts )
            {
                if( p_stream->mt.majortype == MEDIATYPE_Video || !p_stream->b_pts )
                {
                    /* Use our data timestamp */
                    i_pts = sample.i_timestamp;
                    p_stream->b_pts = true;
                }
            }

            i_pts /= 10; /* Dshow works with 100 nano-seconds resolution */

#if 0
            msg_Dbg( p_demux, "Read() stream: %i, size: %i, PTS: %"PRId64,
                     i_stream, i_data_size, i_pts );
#endif

            p_block = block_New( p_demux, i_data_size );
            vlc_memcpy( p_block->p_buffer, p_data, i_data_size );
            p_block->i_pts = p_block->i_dts = i_pts;
            sample.p_sample->Release();

            es_out_Control( p_demux->out, ES_OUT_SET_PCR, i_pts > 0 ? i_pts : 0 );
            es_out_Send( p_demux->out, p_stream->p_es, p_block );

            i_samples--;
        }
    }

    return 1;
}

/*****************************************************************************
 * AccessControl:
 *****************************************************************************/
static int AccessControl( access_t *p_access, int i_query, va_list args )
{
    bool   *pb_bool;
    int          *pi_int;
    int64_t      *pi_64;

    switch( i_query )
    {
    /* */
    case ACCESS_CAN_SEEK:
    case ACCESS_CAN_FASTSEEK:
    case ACCESS_CAN_PAUSE:
    case ACCESS_CAN_CONTROL_PACE:
        pb_bool = (bool*)va_arg( args, bool* );
        *pb_bool = false;
        break;

    /* */
    case ACCESS_GET_PTS_DELAY:
        pi_64 = (int64_t*)va_arg( args, int64_t * );
        *pi_64 = (int64_t)var_GetInteger( p_access, "dshow-caching" ) * 1000;
        break;

    /* */
    case ACCESS_SET_PAUSE_STATE:
    case ACCESS_GET_TITLE_INFO:
    case ACCESS_SET_TITLE:
    case ACCESS_SET_SEEKPOINT:
    case ACCESS_SET_PRIVATE_ID_STATE:
        return VLC_EGENERIC;

    default:
        msg_Warn( p_access, "unimplemented query in control" );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

/****************************************************************************
 * DemuxControl:
 ****************************************************************************/
static int DemuxControl( demux_t *p_demux, int i_query, va_list args )
{
    bool *pb;
    int64_t    *pi64;

    switch( i_query )
    {
    /* Special for access_demux */
    case DEMUX_CAN_PAUSE:
    case DEMUX_CAN_SEEK:
    case DEMUX_SET_PAUSE_STATE:
    case DEMUX_CAN_CONTROL_PACE:
        pb = (bool*)va_arg( args, bool * );
        *pb = false;
        return VLC_SUCCESS;

    case DEMUX_GET_PTS_DELAY:
        pi64 = (int64_t*)va_arg( args, int64_t * );
        *pi64 = (int64_t)var_GetInteger( p_demux, "dshow-caching" ) * 1000;
        return VLC_SUCCESS;

    case DEMUX_GET_TIME:
        pi64 = (int64_t*)va_arg( args, int64_t * );
        *pi64 = mdate();
        return VLC_SUCCESS;

    /* TODO implement others */
    default:
        return VLC_EGENERIC;
    }

    return VLC_EGENERIC;
}

/*****************************************************************************
 * config variable callback
 *****************************************************************************/
static int FindDevicesCallback( vlc_object_t *p_this, char const *psz_name,
                               vlc_value_t newval, vlc_value_t oldval, void * )
{
    module_config_t *p_item;
    bool b_audio = false;
    int i;

    p_item = config_FindConfig( p_this, psz_name );
    if( !p_item ) return VLC_SUCCESS;

    if( !strcmp( psz_name, "dshow-adev" ) ) b_audio = true;

    /* Clear-up the current list */
    if( p_item->i_list )
    {
        /* Keep the 2 first entries */
        for( i = 2; i < p_item->i_list; i++ )
        {
            free( const_cast<char *>(p_item->ppsz_list[i]) );
            free( const_cast<char *>(p_item->ppsz_list_text[i]) );
        }
        /* TODO: Remove when no more needed */
        p_item->ppsz_list[i] = NULL;
        p_item->ppsz_list_text[i] = NULL;
    }
    p_item->i_list = 2;

    /* Find list of devices */
    list<string> list_devices;

    /* Initialize OLE/COM */
    CoInitialize( 0 );

    FindCaptureDevice( p_this, NULL, &list_devices, b_audio );

    /* Uninitialize OLE/COM */
    CoUninitialize();

    if( list_devices.empty() ) return VLC_SUCCESS;

    p_item->ppsz_list = (char**)xrealloc( p_item->ppsz_list,
                          (list_devices.size()+3) * sizeof(char *) );
    p_item->ppsz_list_text = (char**)xrealloc( p_item->ppsz_list_text,
                          (list_devices.size()+3) * sizeof(char *) );

    list<string>::iterator iter;
    for( iter = list_devices.begin(), i = 2; iter != list_devices.end();
         iter++, i++ )
    {
        p_item->ppsz_list[i] = strdup( iter->c_str() );
        p_item->ppsz_list_text[i] = NULL;
        p_item->i_list++;
    }
    p_item->ppsz_list[i] = NULL;
    p_item->ppsz_list_text[i] = NULL;

    /* Signal change to the interface */
    p_item->b_dirty = true;

    return VLC_SUCCESS;
}

static int ConfigDevicesCallback( vlc_object_t *p_this, char const *psz_name,
                               vlc_value_t newval, vlc_value_t oldval, void * )
{
    module_config_t *p_item;
    bool b_audio = false;
    char *psz_device = NULL;
    int i_ret = VLC_SUCCESS;

    if( !EMPTY_STR( newval.psz_string ) )
        psz_device = strdup( newval.psz_string );

    /* Initialize OLE/COM */
    CoInitialize( 0 );

    p_item = config_FindConfig( p_this, psz_name );

    if( !p_item )
    {
        free( psz_device );
        CoUninitialize();
        return VLC_SUCCESS;
    }

    if( !strcmp( psz_name, "dshow-adev" ) ) b_audio = true;

    string devicename;

    if( psz_device )
    {
        devicename = psz_device ;
    }
    else
    {
        /* If no device name was specified, pick the 1st one */
        list<string> list_devices;

        /* Enumerate devices */
        FindCaptureDevice( p_this, NULL, &list_devices, b_audio );
        if( list_devices.empty() )
        {
            CoUninitialize();
            return VLC_EGENERIC;
        }
        devicename = *list_devices.begin();
    }

    IBaseFilter *p_device_filter =
        FindCaptureDevice( p_this, &devicename, NULL, b_audio );
    if( p_device_filter )
    {
        ShowPropertyPage( p_device_filter );
        p_device_filter->Release();
    }
    else
    {
        msg_Err( p_this, "didn't find device: %s", devicename.c_str() );
        i_ret = VLC_EGENERIC;
    }

    /* Uninitialize OLE/COM */
    CoUninitialize();

    free( psz_device );
    return i_ret;
}

/*****************************************************************************
 * Properties
 *****************************************************************************/

static void ShowPropertyPage( IUnknown *obj )
{
    ISpecifyPropertyPages *p_spec;
    CAUUID cauuid;

    HRESULT hr = obj->QueryInterface( IID_ISpecifyPropertyPages,
                                      (void **)&p_spec );
    if( FAILED(hr) ) return;

    if( SUCCEEDED(p_spec->GetPages( &cauuid )) )
    {
        if( cauuid.cElems > 0 )
        {
            HWND hwnd_desktop = ::GetDesktopWindow();

            OleCreatePropertyFrame( hwnd_desktop, 30, 30, NULL, 1, &obj,
                                    cauuid.cElems, cauuid.pElems, 0, 0, NULL );

            CoTaskMemFree( cauuid.pElems );
        }
        p_spec->Release();
    }
}

static void ShowDeviceProperties( vlc_object_t *p_this,
                                  ICaptureGraphBuilder2 *p_graph,
                                  IBaseFilter *p_device_filter,
                                  bool b_audio )
{
    HRESULT hr;
    msg_Dbg( p_this, "configuring Device Properties" );

    /*
     * Video or audio capture filter page
     */
    ShowPropertyPage( p_device_filter );

    /*
     * Audio capture pin
     */
    if( p_graph && b_audio )
    {
        IAMStreamConfig *p_SC;

        msg_Dbg( p_this, "showing WDM Audio Configuration Pages" );

        hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                     &MEDIATYPE_Audio, p_device_filter,
                                     IID_IAMStreamConfig, (void **)&p_SC );
        if( SUCCEEDED(hr) )
        {
            ShowPropertyPage(p_SC);
            p_SC->Release();
        }

        /*
         * TV Audio filter
         */
        IAMTVAudio *p_TVA;
        HRESULT hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                             &MEDIATYPE_Audio, p_device_filter,
                                             IID_IAMTVAudio, (void **)&p_TVA );
        if( SUCCEEDED(hr) )
        {
            ShowPropertyPage(p_TVA);
            p_TVA->Release();
        }
    }

    /*
     * Video capture pin
     */
    if( p_graph && !b_audio )
    {
        IAMStreamConfig *p_SC;

        msg_Dbg( p_this, "showing WDM Video Configuration Pages" );

        hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                     &MEDIATYPE_Interleaved, p_device_filter,
                                     IID_IAMStreamConfig, (void **)&p_SC );
        if( FAILED(hr) )
        {
            hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                         &MEDIATYPE_Video, p_device_filter,
                                         IID_IAMStreamConfig, (void **)&p_SC );
        }

        if( FAILED(hr) )
        {
            hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                         &MEDIATYPE_Stream, p_device_filter,
                                         IID_IAMStreamConfig, (void **)&p_SC );
        }

        if( SUCCEEDED(hr) )
        {
            ShowPropertyPage(p_SC);
            p_SC->Release();
        }
    }
}

static void ShowTunerProperties( vlc_object_t *p_this,
                                 ICaptureGraphBuilder2 *p_graph,
                                 IBaseFilter *p_device_filter,
                                 bool b_audio )
{
    HRESULT hr;
    msg_Dbg( p_this, "configuring Tuner Properties" );

    if( !p_graph || b_audio ) return;

    IAMTVTuner *p_TV;
    hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                 &MEDIATYPE_Interleaved, p_device_filter,
                                 IID_IAMTVTuner, (void **)&p_TV );
    if( FAILED(hr) )
    {
        hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                     &MEDIATYPE_Video, p_device_filter,
                                     IID_IAMTVTuner, (void **)&p_TV );
    }

    if( FAILED(hr) )
    {
        hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
                                     &MEDIATYPE_Stream, p_device_filter,
                                     IID_IAMTVTuner, (void **)&p_TV );
    }

    if( SUCCEEDED(hr) )
    {
        ShowPropertyPage(p_TV);
        p_TV->Release();
    }
}

static void ConfigTuner( vlc_object_t *p_this, ICaptureGraphBuilder2 *p_graph,
                         IBaseFilter *p_device_filter )
{
    int i_channel, i_country, i_input, i_amtuner_mode;
    long l_modes = 0;
    IAMTVTuner *p_TV;
    HRESULT hr;

    if( !p_graph ) return;

    i_channel = var_GetInteger( p_this, "dshow-tuner-channel" );
    i_country = var_GetInteger( p_this, "dshow-tuner-country" );
    i_input = var_GetInteger( p_this, "dshow-tuner-input" );
    i_amtuner_mode = var_GetInteger( p_this, "dshow-amtuner-mode" );

    if( !i_channel && !i_country && !i_input ) return; /* Nothing to do */

    msg_Dbg( p_this, "tuner config: channel %i, country %i, input type %i",
             i_channel, i_country, i_input );

    hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved,
                                 p_device_filter, IID_IAMTVTuner,
                                 (void **)&p_TV );
    if( FAILED(hr) )
    {
        hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                     p_device_filter, IID_IAMTVTuner,
                                     (void **)&p_TV );
    }

    if( FAILED(hr) )
    {
        hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Stream,
                                     p_device_filter, IID_IAMTVTuner,
                                     (void **)&p_TV );
    }

    if( FAILED(hr) )
    {
        msg_Dbg( p_this, "couldn't find tuner interface" );
        return;
    }

    hr = p_TV->GetAvailableModes( &l_modes );
    if( SUCCEEDED(hr) && (l_modes & i_amtuner_mode) )
    {
        hr = p_TV->put_Mode( (AMTunerModeType)i_amtuner_mode );
    }

    if( i_input == 1 ) p_TV->put_InputType( 0, TunerInputCable );
    else if( i_input == 2 ) p_TV->put_InputType( 0, TunerInputAntenna );

    p_TV->put_CountryCode( i_country );
    p_TV->put_Channel( i_channel, AMTUNER_SUBCHAN_NO_TUNE,
                       AMTUNER_SUBCHAN_NO_TUNE );
    p_TV->Release();
}
