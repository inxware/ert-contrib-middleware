/*****************************************************************************
 * video.c: video decoder using the ffmpeg library
 *****************************************************************************
 * Copyright (C) 1999-2001 the VideoLAN team
 * $Id: 234cd589f0c518c329f9249195f4cfdc51fc1f8a $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
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

#include <vlc_common.h>
#include <vlc_codec.h>
#include <vlc_codecs.h>                               /* BITMAPINFOHEADER */
#include <vlc_avcodec.h>
#include <assert.h>

/* ffmpeg header */
#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#   include <libavcodec/avcodec.h>
#   ifdef HAVE_AVCODEC_VAAPI
#       include <libavcodec/vaapi.h>
#   endif
#   ifdef HAVE_AVCODEC_DXVA2
#       include <libavcodec/dxva2.h>
#   endif
#elif defined(HAVE_FFMPEG_AVCODEC_H)
#   include <ffmpeg/avcodec.h>
#else
#   include <avcodec.h>
#endif

#include "avcodec.h"
#include "va.h"
#if defined(HAVE_AVCODEC_VAAPI) || defined(HAVE_AVCODEC_DXVA2)
#   define HAVE_AVCODEC_VA
#endif

/*****************************************************************************
 * decoder_sys_t : decoder descriptor
 *****************************************************************************/
struct decoder_sys_t
{
    FFMPEG_COMMON_MEMBERS

    /* Video decoder specific part */
    mtime_t input_pts;
    mtime_t input_dts;
    mtime_t i_pts;

    AVFrame          *p_ff_pic;

    /* for frame skipping algo */
    bool b_hurry_up;
    enum AVDiscard i_skip_frame;
    enum AVDiscard i_skip_idct;

    /* how many decoded frames are late */
    int     i_late_frames;
    mtime_t i_late_frames_start;

    /* for direct rendering */
    bool b_direct_rendering;
    int  i_direct_rendering_used;

    bool b_has_b_frames;

    /* Hack to force display of still pictures */
    bool b_first_frame;

    /* */
    AVPaletteControl palette;

    /* */
    bool b_flush;

    /* VA API */
    vlc_va_t *p_va;
};

/* FIXME (dummy palette for now) */
static const AVPaletteControl palette_control;

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static void ffmpeg_InitCodec      ( decoder_t * );
static int  ffmpeg_OpenCodec      ( decoder_t * );
static void ffmpeg_CopyPicture    ( decoder_t *, picture_t *, AVFrame * );
static int  ffmpeg_GetFrameBuf    ( struct AVCodecContext *, AVFrame * );
static int  ffmpeg_ReGetFrameBuf( struct AVCodecContext *, AVFrame * );
static void ffmpeg_ReleaseFrameBuf( struct AVCodecContext *, AVFrame * );
static void ffmpeg_NextPts( decoder_t * );

#ifdef HAVE_AVCODEC_VA
static enum PixelFormat ffmpeg_GetFormat( AVCodecContext *,
                                          const enum PixelFormat * );
#endif

static uint32_t ffmpeg_CodecTag( vlc_fourcc_t fcc )
{
    uint8_t *p = (uint8_t*)&fcc;
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/* Returns a new picture buffer */
static inline picture_t *ffmpeg_NewPictBuf( decoder_t *p_dec,
                                            AVCodecContext *p_context )
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    p_dec->fmt_out.video.i_width = p_context->width;
    p_dec->fmt_out.video.i_height = p_context->height;

    if( !p_context->width || !p_context->height )
    {
        return NULL; /* invalid display size */
    }

    if( !p_sys->p_va && GetVlcChroma( &p_dec->fmt_out.video, p_context->pix_fmt ) )
    {
        /* we are doomed, but not really, because most codecs set their pix_fmt
         * much later
         * FIXME does it make sense here ? */
        p_dec->fmt_out.video.i_chroma = VLC_CODEC_I420;
    }
    p_dec->fmt_out.i_codec = p_dec->fmt_out.video.i_chroma;

    /* If an aspect-ratio was specified in the input format then force it */
    if( p_dec->fmt_in.video.i_sar_num > 0 && p_dec->fmt_in.video.i_sar_den > 0 )
    {
        p_dec->fmt_out.video.i_sar_num = p_dec->fmt_in.video.i_sar_num;
        p_dec->fmt_out.video.i_sar_den = p_dec->fmt_in.video.i_sar_den;
    }
    else
    {
        p_dec->fmt_out.video.i_sar_num = p_context->sample_aspect_ratio.num;
        p_dec->fmt_out.video.i_sar_den = p_context->sample_aspect_ratio.den;

        if( !p_dec->fmt_out.video.i_sar_num || !p_dec->fmt_out.video.i_sar_den )
        {
            p_dec->fmt_out.video.i_sar_num = 1;
            p_dec->fmt_out.video.i_sar_den = 1;
        }
    }

    if( p_dec->fmt_in.video.i_frame_rate > 0 &&
        p_dec->fmt_in.video.i_frame_rate_base > 0 )
    {
        p_dec->fmt_out.video.i_frame_rate =
            p_dec->fmt_in.video.i_frame_rate;
        p_dec->fmt_out.video.i_frame_rate_base =
            p_dec->fmt_in.video.i_frame_rate_base;
    }
    else if( p_context->time_base.num > 0 && p_context->time_base.den > 0 )
    {
        p_dec->fmt_out.video.i_frame_rate = p_context->time_base.den;
        p_dec->fmt_out.video.i_frame_rate_base = p_context->time_base.num;
    }

    return decoder_NewPicture( p_dec );
}

/*****************************************************************************
 * InitVideo: initialize the video decoder
 *****************************************************************************
 * the ffmpeg codec will be opened, some memory allocated. The vout is not yet
 * opened (done after the first decoded frame).
 *****************************************************************************/
int InitVideoDec( decoder_t *p_dec, AVCodecContext *p_context,
                      AVCodec *p_codec, int i_codec_id, const char *psz_namecodec )
{
    decoder_sys_t *p_sys;
    int i_val;

    /* Allocate the memory needed to store the decoder's structure */
    if( ( p_dec->p_sys = p_sys = calloc( 1, sizeof(decoder_sys_t) ) ) == NULL )
        return VLC_ENOMEM;

    p_codec->type = CODEC_TYPE_VIDEO;
    p_context->codec_type = CODEC_TYPE_VIDEO;
    p_context->codec_id = i_codec_id;
    p_sys->p_context = p_context;
    p_sys->p_codec = p_codec;
    p_sys->i_codec_id = i_codec_id;
    p_sys->psz_namecodec = psz_namecodec;
    p_sys->p_ff_pic = avcodec_alloc_frame();
    p_sys->b_delayed_open = true;
    p_sys->p_va = NULL;

    /* ***** Fill p_context with init values ***** */
    p_sys->p_context->codec_tag = ffmpeg_CodecTag( p_dec->fmt_in.i_original_fourcc ?: p_dec->fmt_in.i_codec );

    /*  ***** Get configuration of ffmpeg plugin ***** */
    p_sys->p_context->workaround_bugs =
        var_InheritInteger( p_dec, "ffmpeg-workaround-bugs" );
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT( 52, 0, 0 )
    p_sys->p_context->error_resilience =
        var_InheritInteger( p_dec, "ffmpeg-error-resilience" );
#else
    p_sys->p_context->error_recognition =
        var_InheritInteger( p_dec, "ffmpeg-error-resilience" );
#endif

    if( var_CreateGetBool( p_dec, "grayscale" ) )
        p_sys->p_context->flags |= CODEC_FLAG_GRAY;

    i_val = var_CreateGetInteger( p_dec, "ffmpeg-vismv" );
    if( i_val ) p_sys->p_context->debug_mv = i_val;

    i_val = var_CreateGetInteger( p_dec, "ffmpeg-lowres" );
    if( i_val > 0 && i_val <= 2 ) p_sys->p_context->lowres = i_val;

    i_val = var_CreateGetInteger( p_dec, "ffmpeg-skiploopfilter" );
    if( i_val >= 4 ) p_sys->p_context->skip_loop_filter = AVDISCARD_ALL;
    else if( i_val == 3 ) p_sys->p_context->skip_loop_filter = AVDISCARD_NONKEY;
    else if( i_val == 2 ) p_sys->p_context->skip_loop_filter = AVDISCARD_BIDIR;
    else if( i_val == 1 ) p_sys->p_context->skip_loop_filter = AVDISCARD_NONREF;

    if( var_CreateGetBool( p_dec, "ffmpeg-fast" ) )
        p_sys->p_context->flags2 |= CODEC_FLAG2_FAST;

    /* ***** ffmpeg frame skipping ***** */
    p_sys->b_hurry_up = var_CreateGetBool( p_dec, "ffmpeg-hurry-up" );

    switch( var_CreateGetInteger( p_dec, "ffmpeg-skip-frame" ) )
    {
        case -1:
            p_sys->p_context->skip_frame = AVDISCARD_NONE;
            break;
        case 0:
            p_sys->p_context->skip_frame = AVDISCARD_DEFAULT;
            break;
        case 1:
            p_sys->p_context->skip_frame = AVDISCARD_BIDIR;
            break;
        case 2:
            p_sys->p_context->skip_frame = AVDISCARD_NONKEY;
            break;
        case 3:
            p_sys->p_context->skip_frame = AVDISCARD_ALL;
            break;
        default:
            p_sys->p_context->skip_frame = AVDISCARD_NONE;
            break;
    }
    p_sys->i_skip_frame = p_sys->p_context->skip_frame;

    switch( var_CreateGetInteger( p_dec, "ffmpeg-skip-idct" ) )
    {
        case -1:
            p_sys->p_context->skip_idct = AVDISCARD_NONE;
            break;
        case 0:
            p_sys->p_context->skip_idct = AVDISCARD_DEFAULT;
            break;
        case 1:
            p_sys->p_context->skip_idct = AVDISCARD_BIDIR;
            break;
        case 2:
            p_sys->p_context->skip_idct = AVDISCARD_NONKEY;
            break;
        case 3:
            p_sys->p_context->skip_idct = AVDISCARD_ALL;
            break;
        default:
            p_sys->p_context->skip_idct = AVDISCARD_NONE;
            break;
    }
    p_sys->i_skip_idct = p_sys->p_context->skip_idct;

    /* ***** ffmpeg direct rendering ***** */
    p_sys->b_direct_rendering = false;
    p_sys->i_direct_rendering_used = -1;
    if( var_CreateGetBool( p_dec, "ffmpeg-dr" ) &&
       (p_sys->p_codec->capabilities & CODEC_CAP_DR1) &&
        /* No idea why ... but this fixes flickering on some TSCC streams */
        p_sys->i_codec_id != CODEC_ID_TSCC &&
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT( 52, 68, 2 )
        /* avcodec native vp8 decode doesn't handle EMU_EDGE flag, and I
           don't have idea howto implement fallback to libvpx decoder */
        p_sys->i_codec_id != CODEC_ID_VP8 &&
#endif
        !p_sys->p_context->debug_mv )
    {
        /* Some codecs set pix_fmt only after the 1st frame has been decoded,
         * so we need to do another check in ffmpeg_GetFrameBuf() */
        p_sys->b_direct_rendering = true;
    }

    /* ffmpeg doesn't properly release old pictures when frames are skipped */
    //if( p_sys->b_hurry_up ) p_sys->b_direct_rendering = false;
    if( p_sys->b_direct_rendering )
    {
        msg_Dbg( p_dec, "trying to use direct rendering" );
        p_sys->p_context->flags |= CODEC_FLAG_EMU_EDGE;
    }
    else
    {
        msg_Dbg( p_dec, "direct rendering is disabled" );
    }

    /* Always use our get_buffer wrapper so we can calculate the
     * PTS correctly */
    p_sys->p_context->get_buffer = ffmpeg_GetFrameBuf;
    p_sys->p_context->reget_buffer = ffmpeg_ReGetFrameBuf;
    p_sys->p_context->release_buffer = ffmpeg_ReleaseFrameBuf;
    p_sys->p_context->opaque = p_dec;

#ifdef HAVE_AVCODEC_VA
    if( var_CreateGetBool( p_dec, "ffmpeg-hw" ) )
        p_sys->p_context->get_format = ffmpeg_GetFormat;
#endif

    /* ***** misc init ***** */
    p_sys->input_pts = p_sys->input_dts = 0;
    p_sys->i_pts = 0;
    p_sys->b_has_b_frames = false;
    p_sys->b_first_frame = true;
    p_sys->b_flush = false;
    p_sys->i_late_frames = 0;

    /* Set output properties */
    p_dec->fmt_out.i_cat = VIDEO_ES;
    if( GetVlcChroma( &p_dec->fmt_out.video, p_context->pix_fmt ) != VLC_SUCCESS )
    {
        /* we are doomed. but not really, because most codecs set their pix_fmt later on */
        p_dec->fmt_out.i_codec = VLC_CODEC_I420;
    }
    p_dec->fmt_out.i_codec = p_dec->fmt_out.video.i_chroma;

    /* Setup palette */
    memset( &p_sys->palette, 0, sizeof(p_sys->palette) );
    if( p_dec->fmt_in.video.p_palette )
    {
        p_sys->palette.palette_changed = 1;

        for( int i = 0; i < __MIN( AVPALETTE_COUNT, p_dec->fmt_in.video.p_palette->i_entries ); i++ )
        {
            union {
                uint32_t u;
                uint8_t a[4];
            } c;
            c.a[0] = p_dec->fmt_in.video.p_palette->palette[i][0];
            c.a[1] = p_dec->fmt_in.video.p_palette->palette[i][1];
            c.a[2] = p_dec->fmt_in.video.p_palette->palette[i][2];
            c.a[3] = p_dec->fmt_in.video.p_palette->palette[i][3];

            p_sys->palette.palette[i] = c.u;
        }
        p_sys->p_context->palctrl = &p_sys->palette;

        p_dec->fmt_out.video.p_palette = malloc( sizeof(video_palette_t) );
        if( p_dec->fmt_out.video.p_palette )
            *p_dec->fmt_out.video.p_palette = *p_dec->fmt_in.video.p_palette;
    }
    else if( p_sys->i_codec_id != CODEC_ID_MSVIDEO1 && p_sys->i_codec_id != CODEC_ID_CINEPAK )
    {
        p_sys->p_context->palctrl = &p_sys->palette;
    }

    /* ***** init this codec with special data ***** */
    ffmpeg_InitCodec( p_dec );

    /* ***** Open the codec ***** */
    if( ffmpeg_OpenCodec( p_dec ) < 0 )
    {
        msg_Err( p_dec, "cannot open codec (%s)", p_sys->psz_namecodec );
        av_free( p_sys->p_ff_pic );
        free( p_sys );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * DecodeVideo: Called to decode one or more frames
 *****************************************************************************/
picture_t *DecodeVideo( decoder_t *p_dec, block_t **pp_block )
{
    decoder_sys_t *p_sys = p_dec->p_sys;
    int b_drawpicture;
    int b_null_size = false;
    block_t *p_block;

    if( !pp_block || !*pp_block )
        return NULL;

    if( !p_sys->p_context->extradata_size && p_dec->fmt_in.i_extra )
    {
        ffmpeg_InitCodec( p_dec );
        if( p_sys->b_delayed_open )
        {
            if( ffmpeg_OpenCodec( p_dec ) )
                msg_Err( p_dec, "cannot open codec (%s)", p_sys->psz_namecodec );
        }
    }

    p_block = *pp_block;
    if( p_sys->b_delayed_open )
    {
        block_Release( p_block );
        return NULL;
    }

    if( p_block->i_flags & (BLOCK_FLAG_DISCONTINUITY|BLOCK_FLAG_CORRUPTED) )
    {
        p_sys->i_pts = 0; /* To make sure we recover properly */

        p_sys->input_pts = p_sys->input_dts = 0;
        p_sys->i_late_frames = 0;

        if( p_block->i_flags & BLOCK_FLAG_DISCONTINUITY )
            avcodec_flush_buffers( p_sys->p_context );

        block_Release( p_block );
        return NULL;
    }

    if( p_block->i_flags & BLOCK_FLAG_PREROLL )
    {
        /* Do not care about late frames when prerolling
         * TODO avoid decoding of non reference frame
         * (ie all B except for H264 where it depends only on nal_ref_idc) */
        p_sys->i_late_frames = 0;
    }

    if( !p_dec->b_pace_control && (p_sys->i_late_frames > 0) &&
        (mdate() - p_sys->i_late_frames_start > INT64_C(5000000)) )
    {
        if( p_sys->i_pts )
        {
            msg_Err( p_dec, "more than 5 seconds of late video -> "
                     "dropping frame (computer too slow ?)" );
            p_sys->i_pts = 0; /* To make sure we recover properly */
        }
        block_Release( p_block );
        p_sys->i_late_frames--;
        return NULL;
    }

    if( p_block->i_pts > 0 || p_block->i_dts > 0 )
    {
        p_sys->input_pts = p_block->i_pts;
        p_sys->input_dts = p_block->i_dts;

        /* Make sure we don't reuse the same timestamps twice */
        p_block->i_pts = p_block->i_dts = 0;
    }

    /* A good idea could be to decode all I pictures and see for the other */
    if( !p_dec->b_pace_control &&
        p_sys->b_hurry_up &&
        (p_sys->i_late_frames > 4) )
    {
        b_drawpicture = 0;
        if( p_sys->i_late_frames < 12 )
        {
            p_sys->p_context->skip_frame =
                    (p_sys->i_skip_frame <= AVDISCARD_BIDIR) ?
                    AVDISCARD_BIDIR : p_sys->i_skip_frame;
        }
        else
        {
            /* picture too late, won't decode
             * but break picture until a new I, and for mpeg4 ...*/
            p_sys->i_late_frames--; /* needed else it will never be decrease */
            block_Release( p_block );
            return NULL;
        }
    }
    else
    {
        if( p_sys->b_hurry_up )
            p_sys->p_context->skip_frame = p_sys->i_skip_frame;
        if( !(p_block->i_flags & BLOCK_FLAG_PREROLL) )
            b_drawpicture = 1;
        else
            b_drawpicture = 0;
    }

    if( p_sys->p_context->width <= 0 || p_sys->p_context->height <= 0 )
    {
        if( p_sys->b_hurry_up )
            p_sys->p_context->skip_frame = p_sys->i_skip_frame;
        b_null_size = true;
    }
    else if( !b_drawpicture )
    {
        /* It creates broken picture
         * FIXME either our parser or ffmpeg is broken */
#if 0
        if( p_sys->b_hurry_up )
            p_sys->p_context->skip_frame = __MAX( p_sys->p_context->skip_frame,
                                                  AVDISCARD_NONREF );
#endif
    }

    /*
     * Do the actual decoding now
     */

    /* Don't forget that ffmpeg requires a little more bytes
     * that the real frame size */
    if( p_block->i_buffer > 0 )
    {
        p_sys->b_flush = ( p_block->i_flags & BLOCK_FLAG_END_OF_SEQUENCE ) != 0;

        p_block = block_Realloc( p_block, 0,
                            p_block->i_buffer + FF_INPUT_BUFFER_PADDING_SIZE );
        if( !p_block )
            return NULL;
        p_block->i_buffer -= FF_INPUT_BUFFER_PADDING_SIZE;
        *pp_block = p_block;
        memset( p_block->p_buffer + p_block->i_buffer, 0,
                FF_INPUT_BUFFER_PADDING_SIZE );
    }

    while( p_block->i_buffer > 0 || p_sys->b_flush )
    {
        int i_used, b_gotpicture;
        picture_t *p_pic;

        p_sys->p_ff_pic->pts = p_sys->input_pts;
        i_used = avcodec_decode_video( p_sys->p_context, p_sys->p_ff_pic,
                                       &b_gotpicture,
                                       p_block->i_buffer <= 0 && p_sys->b_flush ? NULL : p_block->p_buffer, p_block->i_buffer );

        if( b_null_size && p_sys->p_context->width > 0 &&
            p_sys->p_context->height > 0 &&
            !p_sys->b_flush )
        {
            /* Reparse it to not drop the I frame */
            b_null_size = false;
            if( p_sys->b_hurry_up )
                p_sys->p_context->skip_frame = p_sys->i_skip_frame;
            i_used = avcodec_decode_video( p_sys->p_context, p_sys->p_ff_pic,
                                           &b_gotpicture, p_block->p_buffer,
                                           p_block->i_buffer );
        }

        if( p_sys->b_flush )
            p_sys->b_first_frame = true;

        if( p_block->i_buffer <= 0 )
            p_sys->b_flush = false;

        if( i_used < 0 )
        {
            if( b_drawpicture )
                msg_Warn( p_dec, "cannot decode one frame (%zu bytes)",
                          p_block->i_buffer );
            block_Release( p_block );
            return NULL;
        }
        else if( i_used > p_block->i_buffer )
        {
            i_used = p_block->i_buffer;
        }

        /* Consumed bytes */
        p_block->i_buffer -= i_used;
        p_block->p_buffer += i_used;

        /* Nothing to display */
        if( !b_gotpicture )
        {
            if( i_used == 0 ) break;
            continue;
        }

        /* Set the PTS */
        if( p_sys->p_ff_pic->pts )
            p_sys->i_pts = p_sys->p_ff_pic->pts;

        /* Update frame late count (except when doing preroll) */
        mtime_t i_display_date = 0;
        if( !(p_block->i_flags & BLOCK_FLAG_PREROLL) )
            i_display_date = decoder_GetDisplayDate( p_dec, p_sys->i_pts );

        if( i_display_date > 0 && i_display_date <= mdate() )
        {
            p_sys->i_late_frames++;
            if( p_sys->i_late_frames == 1 )
                p_sys->i_late_frames_start = mdate();
        }
        else
        {
            p_sys->i_late_frames = 0;
        }

        if( !b_drawpicture || ( !p_sys->p_va && !p_sys->p_ff_pic->linesize[0] ) )
        {
            /* Do not display the picture */
            p_pic = (picture_t *)p_sys->p_ff_pic->opaque;
            if( !b_drawpicture && p_pic )
                decoder_DeletePicture( p_dec, p_pic );

            ffmpeg_NextPts( p_dec );
            continue;
        }

        if( !p_sys->p_ff_pic->opaque )
        {
            /* Get a new picture */
            p_pic = ffmpeg_NewPictBuf( p_dec, p_sys->p_context );
            if( !p_pic )
            {
                block_Release( p_block );
                return NULL;
            }

            /* Fill p_picture_t from AVVideoFrame and do chroma conversion
             * if needed */
            ffmpeg_CopyPicture( p_dec, p_pic, p_sys->p_ff_pic );
        }
        else
        {
            p_pic = (picture_t *)p_sys->p_ff_pic->opaque;
        }

        /* Sanity check (seems to be needed for some streams) */
        if( p_sys->p_ff_pic->pict_type == FF_B_TYPE )
        {
            p_sys->b_has_b_frames = true;
        }

        if( !p_dec->fmt_in.video.i_sar_num || !p_dec->fmt_in.video.i_sar_den )
        {
            /* Fetch again the aspect ratio in case it changed */
            p_dec->fmt_out.video.i_sar_num
                = p_sys->p_context->sample_aspect_ratio.num;
            p_dec->fmt_out.video.i_sar_den
                = p_sys->p_context->sample_aspect_ratio.den;

            if( !p_dec->fmt_out.video.i_sar_num || !p_dec->fmt_out.video.i_sar_den )
            {
                p_dec->fmt_out.video.i_sar_num = 1;
                p_dec->fmt_out.video.i_sar_den = 1;
            }
        }

        /* Send decoded frame to vout */
        if( p_sys->i_pts )
        {
            p_pic->date = p_sys->i_pts;

            ffmpeg_NextPts( p_dec );

            if( p_sys->b_first_frame )
            {
                /* Hack to force display of still pictures */
                p_sys->b_first_frame = false;
                p_pic->b_force = true;
            }

            p_pic->i_nb_fields = 2 + p_sys->p_ff_pic->repeat_pict;
            p_pic->b_progressive = !p_sys->p_ff_pic->interlaced_frame;
            p_pic->b_top_field_first = p_sys->p_ff_pic->top_field_first;

            p_pic->i_qstride = p_sys->p_ff_pic->qstride;
            int i_mb_h = ( p_pic->format.i_height + 15 ) / 16;
            p_pic->p_q = malloc( p_pic->i_qstride * i_mb_h );
            memcpy( p_pic->p_q, p_sys->p_ff_pic->qscale_table,
                    p_pic->i_qstride * i_mb_h );
            switch( p_sys->p_ff_pic->qscale_type )
            {
                case FF_QSCALE_TYPE_MPEG1:
                    p_pic->i_qtype = QTYPE_MPEG1;
                    break;
                case FF_QSCALE_TYPE_MPEG2:
                    p_pic->i_qtype = QTYPE_MPEG2;
                    break;
                case FF_QSCALE_TYPE_H264:
                    p_pic->i_qtype = QTYPE_H264;
                    break;
            }

            return p_pic;
        }
        else
        {
            decoder_DeletePicture( p_dec, p_pic );
        }
    }

    block_Release( p_block );
    return NULL;
}

/*****************************************************************************
 * EndVideo: decoder destruction
 *****************************************************************************
 * This function is called when the thread ends after a successful
 * initialization.
 *****************************************************************************/
void EndVideoDec( decoder_t *p_dec )
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    /* do not flush buffers if codec hasn't been opened (theora/vorbis/VC1) */
    if( p_sys->p_context->codec )
        avcodec_flush_buffers( p_sys->p_context );

    if( p_sys->p_ff_pic ) av_free( p_sys->p_ff_pic );

    if( p_sys->p_va )
    {
        vlc_va_Delete( p_sys->p_va );
        p_sys->p_va = NULL;
    }
}

/*****************************************************************************
 * ffmpeg_InitCodec: setup codec extra initialization data for ffmpeg
 *****************************************************************************/
static void ffmpeg_InitCodec( decoder_t *p_dec )
{
    decoder_sys_t *p_sys = p_dec->p_sys;
    int i_size = p_dec->fmt_in.i_extra;

    if( !i_size ) return;

    if( p_sys->i_codec_id == CODEC_ID_SVQ3 )
    {
        uint8_t *p;

        p_sys->p_context->extradata_size = i_size + 12;
        p = p_sys->p_context->extradata  =
            malloc( p_sys->p_context->extradata_size );
        if( !p )
            return;

        memcpy( &p[0],  "SVQ3", 4 );
        memset( &p[4], 0, 8 );
        memcpy( &p[12], p_dec->fmt_in.p_extra, i_size );

        /* Now remove all atoms before the SMI one */
        if( p_sys->p_context->extradata_size > 0x5a &&
            strncmp( (char*)&p[0x56], "SMI ", 4 ) )
        {
            uint8_t *psz = &p[0x52];

            while( psz < &p[p_sys->p_context->extradata_size - 8] )
            {
                int i_size = GetDWBE( psz );
                if( i_size <= 1 )
                {
                    /* FIXME handle 1 as long size */
                    break;
                }
                if( !strncmp( (char*)&psz[4], "SMI ", 4 ) )
                {
                    memmove( &p[0x52], psz,
                             &p[p_sys->p_context->extradata_size] - psz );
                    break;
                }

                psz += i_size;
            }
        }
    }
    else
    {
        p_sys->p_context->extradata_size = i_size;
        p_sys->p_context->extradata =
            malloc( i_size + FF_INPUT_BUFFER_PADDING_SIZE );
        if( p_sys->p_context->extradata )
        {
            memcpy( p_sys->p_context->extradata,
                    p_dec->fmt_in.p_extra, i_size );
            memset( &((uint8_t*)p_sys->p_context->extradata)[i_size],
                    0, FF_INPUT_BUFFER_PADDING_SIZE );
        }
    }
}

/*****************************************************************************
 * ffmpeg_OpenCodec:
 *****************************************************************************/
static int ffmpeg_OpenCodec( decoder_t *p_dec )
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    if( p_sys->p_context->extradata_size <= 0 )
    {
        if( p_sys->i_codec_id == CODEC_ID_VC1 ||
            p_sys->i_codec_id == CODEC_ID_VORBIS ||
            p_sys->i_codec_id == CODEC_ID_THEORA )
        {
            msg_Warn( p_dec, "waiting for extra data for codec %s",
                      p_sys->psz_namecodec );
            return 1;
        }
    }
    p_sys->p_context->width  = p_dec->fmt_in.video.i_width;
    p_sys->p_context->height = p_dec->fmt_in.video.i_height;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 0, 0)
    p_sys->p_context->bits_per_sample = p_dec->fmt_in.video.i_bits_per_pixel;
#else
    p_sys->p_context->bits_per_coded_sample = p_dec->fmt_in.video.i_bits_per_pixel;
#endif

    int ret;
    vlc_avcodec_lock();
    ret = avcodec_open( p_sys->p_context, p_sys->p_codec );
    vlc_avcodec_unlock();
    if( ret < 0 )
        return VLC_EGENERIC;
    msg_Dbg( p_dec, "ffmpeg codec (%s) started", p_sys->psz_namecodec );

    p_sys->b_delayed_open = false;

    return VLC_SUCCESS;
}
/*****************************************************************************
 * ffmpeg_CopyPicture: copy a picture from ffmpeg internal buffers to a
 *                     picture_t structure (when not in direct rendering mode).
 *****************************************************************************/
static void ffmpeg_CopyPicture( decoder_t *p_dec,
                                picture_t *p_pic, AVFrame *p_ff_pic )
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    if( p_sys->p_va )
    {
        vlc_va_Extract( p_sys->p_va, p_pic, p_ff_pic );
    }
    else if( TestFfmpegChroma( p_sys->p_context->pix_fmt, -1 ) == VLC_SUCCESS )
    {
        int i_plane, i_size, i_line;
        uint8_t *p_dst, *p_src;
        int i_src_stride, i_dst_stride;

        for( i_plane = 0; i_plane < p_pic->i_planes; i_plane++ )
        {
            p_src  = p_ff_pic->data[i_plane];
            p_dst = p_pic->p[i_plane].p_pixels;
            i_src_stride = p_ff_pic->linesize[i_plane];
            i_dst_stride = p_pic->p[i_plane].i_pitch;

            i_size = __MIN( i_src_stride, i_dst_stride );
            for( i_line = 0; i_line < p_pic->p[i_plane].i_visible_lines;
                 i_line++ )
            {
                vlc_memcpy( p_dst, p_src, i_size );
                p_src += i_src_stride;
                p_dst += i_dst_stride;
            }
        }
    }
    else
    {
        msg_Err( p_dec, "don't know how to convert chroma %i",
                 p_sys->p_context->pix_fmt );
        p_dec->b_error = 1;
    }
}

/*****************************************************************************
 * ffmpeg_GetFrameBuf: callback used by ffmpeg to get a frame buffer.
 *****************************************************************************
 * It is used for direct rendering as well as to get the right PTS for each
 * decoded picture (even in indirect rendering mode).
 *****************************************************************************/
static void ffmpeg_SetFrameBufferPts( decoder_t *p_dec, AVFrame *p_ff_pic );

static int ffmpeg_GetFrameBuf( struct AVCodecContext *p_context,
                               AVFrame *p_ff_pic )
{
    decoder_t *p_dec = (decoder_t *)p_context->opaque;
    decoder_sys_t *p_sys = p_dec->p_sys;
    picture_t *p_pic;

    /* Set picture PTS */
    ffmpeg_SetFrameBufferPts( p_dec, p_ff_pic );

    /* */
    p_ff_pic->opaque = NULL;

    if( p_sys->p_va )
    {
#ifdef HAVE_AVCODEC_VA
        /* hwaccel_context is not present in old fffmpeg version */
        if( vlc_va_Setup( p_sys->p_va,
                          &p_sys->p_context->hwaccel_context, &p_dec->fmt_out.video.i_chroma,
                          p_sys->p_context->width, p_sys->p_context->height ) )
        {
            msg_Err( p_dec, "vlc_va_Setup failed" );
            return -1;
        }
#else
        assert(0);
#endif

        /* */
        p_ff_pic->type = FF_BUFFER_TYPE_USER;
        /* FIXME what is that, should give good value */
        p_ff_pic->age = 256*256*256*64; // FIXME FIXME from ffmpeg

        if( vlc_va_Get( p_sys->p_va, p_ff_pic ) )
        {
            msg_Err( p_dec, "VaGrabSurface failed" );
            return -1;
        }
        return 0;
    }
    else if( !p_sys->b_direct_rendering )
    {
        /* Not much to do in indirect rendering mode. */
        return avcodec_default_get_buffer( p_context, p_ff_pic );
    }

    /* Some codecs set pix_fmt only after the 1st frame has been decoded,
     * so we need to check for direct rendering again. */

    int i_width = p_sys->p_context->width;
    int i_height = p_sys->p_context->height;
    avcodec_align_dimensions( p_sys->p_context, &i_width, &i_height );

    if( GetVlcChroma( &p_dec->fmt_out.video, p_context->pix_fmt ) != VLC_SUCCESS ||
        p_context->pix_fmt == PIX_FMT_PAL8 )
        goto no_dr;

    p_dec->fmt_out.i_codec = p_dec->fmt_out.video.i_chroma;

    /* Get a new picture */
    p_pic = ffmpeg_NewPictBuf( p_dec, p_sys->p_context );
    if( !p_pic )
        goto no_dr;
    bool b_compatible = true;
    if( p_pic->p[0].i_pitch / p_pic->p[0].i_pixel_pitch < i_width ||
        p_pic->p[0].i_lines < i_height )
        b_compatible = false;
    for( int i = 0; i < p_pic->i_planes && b_compatible; i++ )
    {
        unsigned i_align;
        switch( p_sys->i_codec_id )
        {
        case CODEC_ID_SVQ1:
        case CODEC_ID_VP5:
        case CODEC_ID_VP6:
        case CODEC_ID_VP6F:
        case CODEC_ID_VP6A:
            i_align = 16;
            break;
        default:
            i_align = i == 0 ? 16 : 8;
            break;
        }
        if( p_pic->p[i].i_pitch % i_align )
            b_compatible = false;
        if( (intptr_t)p_pic->p[i].p_pixels % i_align )
            b_compatible = false;
    }
    if( p_context->pix_fmt == PIX_FMT_YUV422P && b_compatible )
    {
        if( 2 * p_pic->p[1].i_pitch != p_pic->p[0].i_pitch ||
            2 * p_pic->p[2].i_pitch != p_pic->p[0].i_pitch )
            b_compatible = false;
    }
    if( !b_compatible )
    {
        decoder_DeletePicture( p_dec, p_pic );
        goto no_dr;
    }

    if( p_sys->i_direct_rendering_used != 1 )
    {
        msg_Dbg( p_dec, "using direct rendering" );
        p_sys->i_direct_rendering_used = 1;
    }

    p_sys->p_context->draw_horiz_band = NULL;

    p_ff_pic->opaque = (void*)p_pic;
    p_ff_pic->type = FF_BUFFER_TYPE_USER;
    p_ff_pic->data[0] = p_pic->p[0].p_pixels;
    p_ff_pic->data[1] = p_pic->p[1].p_pixels;
    p_ff_pic->data[2] = p_pic->p[2].p_pixels;
    p_ff_pic->data[3] = NULL; /* alpha channel but I'm not sure */

    p_ff_pic->linesize[0] = p_pic->p[0].i_pitch;
    p_ff_pic->linesize[1] = p_pic->p[1].i_pitch;
    p_ff_pic->linesize[2] = p_pic->p[2].i_pitch;
    p_ff_pic->linesize[3] = 0;

    decoder_LinkPicture( p_dec, p_pic );

    /* FIXME what is that, should give good value */
    p_ff_pic->age = 256*256*256*64; // FIXME FIXME from ffmpeg

    return 0;

no_dr:
    if( p_sys->i_direct_rendering_used != 0 )
    {
        msg_Warn( p_dec, "disabling direct rendering" );
        p_sys->i_direct_rendering_used = 0;
    }
    return avcodec_default_get_buffer( p_context, p_ff_pic );
}
static int  ffmpeg_ReGetFrameBuf( struct AVCodecContext *p_context, AVFrame *p_ff_pic )
{
    decoder_t *p_dec = (decoder_t *)p_context->opaque;
    int i_ret;

    /* */
    p_ff_pic->pts = AV_NOPTS_VALUE;

    /* We always use default reget function, it works perfectly fine */
    i_ret = avcodec_default_reget_buffer( p_context, p_ff_pic );

    /* Set picture PTS if avcodec_default_reget_buffer didn't set it (through a
     * ffmpeg_GetFrameBuf call) */
    if( !i_ret && p_ff_pic->pts == AV_NOPTS_VALUE )
        ffmpeg_SetFrameBufferPts( p_dec, p_ff_pic );

    return i_ret;
}

static void ffmpeg_SetFrameBufferPts( decoder_t *p_dec, AVFrame *p_ff_pic )
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    /* Set picture PTS */
    if( p_sys->input_pts )
    {
        p_ff_pic->pts = p_sys->input_pts;
    }
    else if( p_sys->input_dts )
    {
        /* Some demuxers only set the dts so let's try to find a useful
         * timestamp from this */
        if( !p_sys->p_context->has_b_frames || !p_sys->b_has_b_frames ||
            !p_ff_pic->reference || !p_sys->i_pts )
        {
            p_ff_pic->pts = p_sys->input_dts;
        }
        else
        {
            p_ff_pic->pts = 0;
        }
    }
    else
    {
        p_ff_pic->pts = 0;
    }

    if( p_sys->i_pts ) /* make sure 1st frame has a pts > 0 */
    {
        p_sys->input_pts = p_sys->input_dts = 0;
    }
}

static void ffmpeg_ReleaseFrameBuf( struct AVCodecContext *p_context,
                                    AVFrame *p_ff_pic )
{
    decoder_t *p_dec = (decoder_t *)p_context->opaque;
    decoder_sys_t *p_sys = p_dec->p_sys;

    if( p_sys->p_va )
    {
        vlc_va_Release( p_sys->p_va, p_ff_pic );
    }
    else if( !p_ff_pic->opaque )
    {
        /* We can end up here without the AVFrame being allocated by
         * avcodec_default_get_buffer() if VA is used and the frame is
         * released when the decoder is closed
         */
        if( p_ff_pic->type == FF_BUFFER_TYPE_INTERNAL )
            avcodec_default_release_buffer( p_context, p_ff_pic );
    }
    else
    {
        picture_t *p_pic = (picture_t*)p_ff_pic->opaque;

        decoder_UnlinkPicture( p_dec, p_pic );
    }
    for( int i = 0; i < 4; i++ )
        p_ff_pic->data[i] = NULL;
}

static void ffmpeg_NextPts( decoder_t *p_dec )
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    if( p_sys->i_pts <= 0 )
        return;

    /* interpolate the next PTS */
    if( p_dec->fmt_in.video.i_frame_rate > 0 &&
        p_dec->fmt_in.video.i_frame_rate_base > 0 )
    {
        p_sys->i_pts += INT64_C(1000000) *
            (2 + p_sys->p_ff_pic->repeat_pict) *
            p_dec->fmt_in.video.i_frame_rate_base /
            (2 * p_dec->fmt_in.video.i_frame_rate);
    }
    else if( p_sys->p_context->time_base.den > 0 )
    {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,20,0)
        int i_tick = p_sys->p_context->ticks_per_frame;
        if( i_tick <= 0 )
            i_tick = 1;
#else
        int i_tick = 1;
#endif

        p_sys->i_pts += INT64_C(1000000) *
            (2 + p_sys->p_ff_pic->repeat_pict) *
            i_tick * p_sys->p_context->time_base.num /
            (2 * p_sys->p_context->time_base.den);
    }
}

#ifdef HAVE_AVCODEC_VA
static enum PixelFormat ffmpeg_GetFormat( AVCodecContext *p_codec,
                                          const enum PixelFormat *pi_fmt )
{
    decoder_t *p_dec = p_codec->opaque;
    decoder_sys_t *p_sys = p_dec->p_sys;

    if( p_sys->p_va )
    {
        vlc_va_Delete( p_sys->p_va );
        p_sys->p_va = NULL;
    }

    /* Try too look for a supported hw acceleration */
    for( int i = 0; pi_fmt[i] != PIX_FMT_NONE; i++ )
    {
        static const char *ppsz_name[PIX_FMT_NB] = {
            [PIX_FMT_VDPAU_H264] = "PIX_FMT_VDPAU_H264",
            [PIX_FMT_VAAPI_IDCT] = "PIX_FMT_VAAPI_IDCT",
            [PIX_FMT_VAAPI_VLD] = "PIX_FMT_VAAPI_VLD",
            [PIX_FMT_VAAPI_MOCO] = "PIX_FMT_VAAPI_MOCO",
#ifdef HAVE_AVCODEC_DXVA2
            [PIX_FMT_DXVA2_VLD] = "PIX_FMT_DXVA2_VLD",
#endif
            [PIX_FMT_YUYV422] = "PIX_FMT_YUYV422",
            [PIX_FMT_YUV420P] = "PIX_FMT_YUV420P",
        };
        msg_Dbg( p_dec, "Available decoder output format %d (%s)", pi_fmt[i], ppsz_name[pi_fmt[i]] ?: "Unknown" );

        /* Only VLD supported */
        if( pi_fmt[i] == PIX_FMT_VAAPI_VLD )
        {
#ifdef HAVE_AVCODEC_VAAPI
            msg_Dbg( p_dec, "Trying VA API" );
            p_sys->p_va = vlc_va_NewVaapi( p_sys->i_codec_id );
            if( !p_sys->p_va )
                msg_Warn( p_dec, "Failed to open VA API" );
#else
            continue;
#endif
        }
#ifdef HAVE_AVCODEC_DXVA2
        if( pi_fmt[i] == PIX_FMT_DXVA2_VLD )
        {
            msg_Dbg( p_dec, "Trying DXVA2" );
            p_sys->p_va = vlc_va_NewDxva2( VLC_OBJECT(p_dec), p_sys->i_codec_id );
            if( !p_sys->p_va )
                msg_Warn( p_dec, "Failed to open DXVA2" );
        }
#endif

        if( p_sys->p_va &&
            p_sys->p_context->width > 0 && p_sys->p_context->height > 0 )
        {
            /* We try to call vlc_va_Setup when possible to detect errors when
             * possible (later is too late) */
            if( vlc_va_Setup( p_sys->p_va,
                              &p_sys->p_context->hwaccel_context,
                              &p_dec->fmt_out.video.i_chroma,
                              p_sys->p_context->width, p_sys->p_context->height ) )
            {
                msg_Err( p_dec, "vlc_va_Setup failed" );
                vlc_va_Delete( p_sys->p_va );
                p_sys->p_va = NULL;
            }
        }

        if( p_sys->p_va )
        {
            if( p_sys->p_va->description )
                msg_Info( p_dec, "Using %s for hardware decoding.", p_sys->p_va->description );

            /* FIXME this will disabled direct rendering
             * even if a new pixel format is renegociated
             */
            p_sys->b_direct_rendering = false;
            p_sys->p_context->draw_horiz_band = NULL;
            return pi_fmt[i];
        }
    }

    /* Fallback to default behaviour */
    return avcodec_default_get_format( p_codec, pi_fmt );
}
#endif

