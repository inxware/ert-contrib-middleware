/*****************************************************************************
 * common.h: common definitions
 * Collection of useful common types and macros definitions
 *****************************************************************************
 * Copyright (C) 1998-2005 the VideoLAN team
 * $Id: 7f488a5323ebd1dcc1ca4d91d72bdb5cb24dc8e8 $
 *
 * Authors: Samuel Hocevar <sam@via.ecp.fr>
 *          Vincent Seguin <seguin@via.ecp.fr>
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

/**
 * \file
 * This file is a collection of common definitions and types
 */

#ifndef VLC_COMMON_H
# define VLC_COMMON_H 1

/*****************************************************************************
 * Required vlc headers
 *****************************************************************************/
#if defined( _MSC_VER )
#   pragma warning( disable : 4244 )
#endif

#include "vlc_config.h"

/*****************************************************************************
 * Required system headers
 *****************************************************************************/
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>

#ifndef __cplusplus
# include <stdbool.h>
#endif

/* Try to fix format strings for all versions of mingw and mingw64 */
#if defined( _WIN32 ) && defined( __USE_MINGW_ANSI_STDIO )
 #undef PRId64
 #define PRId64 "lld"
 #undef PRIi64
 #define PRIi64 "lli"
 #undef PRIu64
 #define PRIu64 "llu"
 #undef PRIo64
 #define PRIo64 "llo"
 #undef PRIx64
 #define PRIx64 "llx"
 #define snprintf        __mingw_snprintf
 #define vsnprintf       __mingw_vsnprintf
#endif

/* Format string sanity checks */
#ifdef __GNUC__
#   if defined( _WIN32 ) && (__GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 4 ) )
#     define LIBVLC_FORMAT(x,y) __attribute__ ((format(gnu_printf,x,y)))
#   else
#     define LIBVLC_FORMAT(x,y) __attribute__ ((format(printf,x,y)))
#   endif
#   define LIBVLC_FORMAT_ARG(x) __attribute__ ((format_arg(x)))
#   if __GNUC__ > 3 || (__GNUC__ == 3 && (__GNUC_MINOR__ >= 4))
#     define LIBVLC_USED __attribute__ ((warn_unused_result))
#   else
#     define LIBVLC_USED
#   endif
#   define LIBVLC_MALLOC __attribute__ ((malloc))
#else
#   define LIBVLC_FORMAT(x,y)
#   define LIBVLC_FORMAT_ARG(x)
#   define LIBVLC_USED
#   define LIBVLC_MALLOC
#endif

/* Branch prediction */
#ifdef __GNUC__
#   define likely(p)   __builtin_expect(!!(p), 1)
#   define unlikely(p) __builtin_expect(!!(p), 0)
#else
#   define likely(p)   (!!(p))
#   define unlikely(p) (!!(p))
#endif

/*****************************************************************************
 * Basic types definitions
 *****************************************************************************/
#if defined( WIN32 ) || defined( UNDER_CE )
#   include <malloc.h>
#   ifndef PATH_MAX
#       define PATH_MAX MAX_PATH
#   endif
#endif

#ifdef __SYMBIAN32__
 #include <sys/syslimits.h>
#endif

/* Counter for statistics and profiling */
typedef unsigned long       count_t;

/* Audio volume */
typedef uint16_t            audio_volume_t;

/**
 * High precision date or time interval
 *
 * Store a high precision date or time interval. The maximum precision is the
 * microsecond, and a 64 bits integer is used to avoid overflows (maximum
 * time interval is then 292271 years, which should be long enough for any
 * video). Dates are stored as microseconds since a common date (usually the
 * epoch). Note that date and time intervals can be manipulated using regular
 * arithmetic operators, and that no special functions are required.
 */
typedef int64_t mtime_t;

/**
 * The vlc_fourcc_t type.
 *
 * See http://www.webartz.com/fourcc/ for a very detailed list.
 */
typedef uint32_t vlc_fourcc_t;

#ifdef WORDS_BIGENDIAN
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)d) | ( ((uint32_t)c) << 8 ) \
           | ( ((uint32_t)b) << 16 ) | ( ((uint32_t)a) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(b) | ( (uint16_t)(a) << 8 ) )

#else
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
           | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(a) | ( (uint16_t)(b) << 8 ) )

#endif

/**
 * Translate a vlc_fourcc into its string representation. This function
 * assumes there is enough room in psz_fourcc to store 4 characters in.
 *
 * \param fcc a vlc_fourcc_t
 * \param psz_fourcc string to store string representation of vlc_fourcc in
 */
static inline void vlc_fourcc_to_char( vlc_fourcc_t fcc, char *psz_fourcc )
{
    memcpy( psz_fourcc, &fcc, 4 );
}

#define vlc_fourcc_to_char( a, b ) \
        vlc_fourcc_to_char( (vlc_fourcc_t)(a), (char *)(b) )

/*****************************************************************************
 * Classes declaration
 *****************************************************************************/

/* Internal types */
typedef struct vlc_list_t vlc_list_t;
typedef struct vlc_object_t vlc_object_t;
typedef struct libvlc_int_t libvlc_int_t;
typedef struct date_t date_t;

/* Playlist */

/* FIXME */
/**
 * Playlist commands
 */
typedef enum {
    PLAYLIST_PLAY,      /**< No arg.                            res=can fail*/
    PLAYLIST_VIEWPLAY,  /**< arg1= playlist_item_t*,*/
                        /**  arg2 = playlist_item_t*          , res=can fail */
    PLAYLIST_PAUSE,     /**< No arg                             res=can fail*/
    PLAYLIST_STOP,      /**< No arg                             res=can fail*/
    PLAYLIST_SKIP,      /**< arg1=int,                          res=can fail*/
} playlist_command_t;


typedef struct playlist_t playlist_t;
typedef struct playlist_item_t playlist_item_t;
typedef struct playlist_view_t playlist_view_t;
typedef struct services_discovery_t services_discovery_t;
typedef struct services_discovery_sys_t services_discovery_sys_t;
typedef struct playlist_add_t playlist_add_t;

/* Modules */
typedef struct module_t module_t;
typedef struct module_config_t module_config_t;

typedef struct config_category_t config_category_t;

/* Input */
typedef struct input_thread_t input_thread_t;
typedef struct input_item_t input_item_t;
typedef struct input_item_node_t input_item_node_t;
typedef struct access_t access_t;
typedef struct access_sys_t access_sys_t;
typedef struct stream_t     stream_t;
typedef struct stream_sys_t stream_sys_t;
typedef struct demux_t  demux_t;
typedef struct demux_sys_t demux_sys_t;
typedef struct es_out_t     es_out_t;
typedef struct es_out_id_t  es_out_id_t;
typedef struct es_out_sys_t es_out_sys_t;
typedef struct seekpoint_t seekpoint_t;
typedef struct info_t info_t;
typedef struct info_category_t info_category_t;
typedef struct input_attachment_t input_attachment_t;

/* Format */
typedef struct audio_format_t audio_format_t;
typedef struct video_format_t video_format_t;
typedef struct subs_format_t subs_format_t;
typedef struct es_format_t es_format_t;
typedef struct video_palette_t video_palette_t;

/* Audio */
typedef struct aout_instance_t aout_instance_t;
typedef struct aout_sys_t aout_sys_t;
typedef struct aout_fifo_t aout_fifo_t;
typedef struct aout_input_t aout_input_t;
typedef struct block_t aout_buffer_t;
typedef audio_format_t audio_sample_format_t;
typedef struct aout_filter_t aout_filter_t;

/* Video */
typedef struct vout_thread_t vout_thread_t;

typedef video_format_t video_frame_format_t;
typedef struct picture_t picture_t;
typedef struct picture_sys_t picture_sys_t;

/* Subpictures */
typedef struct spu_t spu_t;
typedef struct subpicture_t subpicture_t;
typedef struct subpicture_sys_t subpicture_sys_t;
typedef struct subpicture_region_t subpicture_region_t;

typedef struct image_handler_t image_handler_t;

/* Stream output */
typedef struct sout_instance_t sout_instance_t;
typedef struct sout_instance_sys_t sout_instance_sys_t;

typedef struct sout_input_t sout_input_t;
typedef struct sout_packetizer_input_t sout_packetizer_input_t;

typedef struct sout_access_out_t sout_access_out_t;
typedef struct sout_access_out_sys_t   sout_access_out_sys_t;

typedef struct sout_mux_t sout_mux_t;
typedef struct sout_mux_sys_t sout_mux_sys_t;

typedef struct sout_stream_t    sout_stream_t;
typedef struct sout_stream_sys_t sout_stream_sys_t;

typedef struct config_chain_t       config_chain_t;
typedef struct session_descriptor_t session_descriptor_t;

/* Decoders */
typedef struct decoder_t         decoder_t;
typedef struct decoder_sys_t     decoder_sys_t;
typedef struct decoder_synchro_t decoder_synchro_t;

/* Encoders */
typedef struct encoder_t      encoder_t;
typedef struct encoder_sys_t  encoder_sys_t;

/* Filters */
typedef struct filter_t filter_t;
typedef struct filter_sys_t filter_sys_t;

/* Network */
typedef struct virtual_socket_t v_socket_t;
typedef struct vlc_acl_t vlc_acl_t;
typedef struct vlc_url_t vlc_url_t;

/* Misc */
typedef struct iso639_lang_t iso639_lang_t;

/* block */
typedef struct block_t      block_t;
typedef struct block_fifo_t block_fifo_t;

/* httpd */
typedef struct httpd_t          httpd_t;
typedef struct httpd_host_t     httpd_host_t;
typedef struct httpd_url_t      httpd_url_t;
typedef struct httpd_client_t   httpd_client_t;
typedef struct httpd_callback_sys_t httpd_callback_sys_t;
typedef struct httpd_message_t  httpd_message_t;
typedef int    (*httpd_callback_t)( httpd_callback_sys_t *, httpd_client_t *, httpd_message_t *answer, const httpd_message_t *query );
typedef struct httpd_file_t     httpd_file_t;
typedef struct httpd_file_sys_t httpd_file_sys_t;
typedef int (*httpd_file_callback_t)( httpd_file_sys_t *, httpd_file_t *, uint8_t *psz_request, uint8_t **pp_data, int *pi_data );
typedef struct httpd_handler_t  httpd_handler_t;
typedef struct httpd_handler_sys_t httpd_handler_sys_t;
typedef int (*httpd_handler_callback_t)( httpd_handler_sys_t *, httpd_handler_t *, char *psz_url, uint8_t *psz_request, int i_type, uint8_t *p_in, int i_in, char *psz_remote_addr, char *psz_remote_host, uint8_t **pp_data, int *pi_data );
typedef struct httpd_redirect_t httpd_redirect_t;
typedef struct httpd_stream_t httpd_stream_t;

/* TLS support */
typedef struct tls_server_t tls_server_t;
typedef struct tls_session_t tls_session_t;

/* Hashing */
typedef struct md5_s md5_t;

/* XML */
typedef struct xml_t xml_t;
typedef struct xml_sys_t xml_sys_t;
typedef struct xml_reader_t xml_reader_t;
typedef struct xml_reader_sys_t xml_reader_sys_t;

/* vod server */
typedef struct vod_t     vod_t;
typedef struct vod_sys_t vod_sys_t;
typedef struct vod_media_t vod_media_t;

/* osdmenu */
typedef struct osd_menu_t   osd_menu_t;
typedef struct osd_state_t  osd_state_t;
typedef struct osd_event_t  osd_event_t;
typedef struct osd_button_t osd_button_t;
typedef struct osd_menu_state_t osd_menu_state_t;

/* VLM */
typedef struct vlm_t         vlm_t;
typedef struct vlm_message_t vlm_message_t;

/* misc */
typedef struct vlc_meta_t    vlc_meta_t;

/* Stats */
typedef struct counter_t     counter_t;
typedef struct counter_sample_t counter_sample_t;
typedef struct stats_handler_t stats_handler_t;
typedef struct input_stats_t input_stats_t;

/* Update */
typedef struct update_t update_t;
typedef struct update_iterator_t update_iterator_t;

/* Meta engine */
typedef struct meta_engine_t meta_engine_t;

/* stat/lstat/fstat */
#ifdef WIN32
#include <sys/stat.h>

# ifndef UNDER_CE
struct _stati64;
#define stat _stati64
#define fstat _fstati64
#endif

/* You should otherwise use vlc_stat and vlc_lstat. */
#else
struct stat;
#endif

/**
 * VLC value structure
 */
typedef union
{
    int64_t         i_int;
    bool            b_bool;
    float           f_float;
    char *          psz_string;
    void *          p_address;
    vlc_object_t *  p_object;
    vlc_list_t *    p_list;
    mtime_t         i_time;
    struct { int32_t x; int32_t y; } coords;

} vlc_value_t;

/**
 * VLC list structure
 */
struct vlc_list_t
{
    int             i_count;
    vlc_value_t *   p_values;
    int *           pi_types;

};

/**
 * \defgroup var_type Variable types
 * These are the different types a vlc variable can have.
 * @{
 */
#define VLC_VAR_VOID      0x0010
#define VLC_VAR_BOOL      0x0020
#define VLC_VAR_INTEGER   0x0030
#define VLC_VAR_HOTKEY    0x0031
#define VLC_VAR_STRING    0x0040
#define VLC_VAR_VARIABLE  0x0044
#define VLC_VAR_FLOAT     0x0050
#define VLC_VAR_TIME      0x0060
#define VLC_VAR_ADDRESS   0x0070
#define VLC_VAR_MUTEX     0x0080
#define VLC_VAR_COORDS    0x00A0
/**@}*/

/*****************************************************************************
 * Error values (shouldn't be exposed)
 *****************************************************************************/
#define VLC_SUCCESS         -0                                   /* No error */
#define VLC_ENOMEM          -1                          /* Not enough memory */
#define VLC_ETIMEOUT        -3                                    /* Timeout */

#define VLC_ENOMOD         -10                           /* Module not found */

#define VLC_ENOOBJ         -20                           /* Object not found */

#define VLC_ENOVAR         -30                         /* Variable not found */
#define VLC_EBADVAR        -31                         /* Bad variable value */

#define VLC_ENOITEM        -40                           /**< Item not found */

#define VLC_EEXIT         -255                             /* Program exited */
#define VLC_EEXITSUCCESS  -999                /* Program exited successfully */
#define VLC_EGENERIC      -666                              /* Generic error */

/*****************************************************************************
 * Variable callbacks
 *****************************************************************************/
typedef int ( * vlc_callback_t ) ( vlc_object_t *,      /* variable's object */
                                   char const *,            /* variable name */
                                   vlc_value_t,                 /* old value */
                                   vlc_value_t,                 /* new value */
                                   void * );                /* callback data */

/*****************************************************************************
 * Plug-in stuff
 *****************************************************************************/

#ifdef __cplusplus
# define LIBVLC_EXTERN extern "C"
#else
# define LIBVLC_EXTERN extern
#endif
#if defined (WIN32) && defined (DLL_EXPORT)
#if defined (UNDER_CE)
# include <windef.h>
#endif
# define LIBVLC_EXPORT __declspec(dllexport)
#else
# define LIBVLC_EXPORT
#endif
#define VLC_EXPORT( type, name, args ) \
                        LIBVLC_EXTERN LIBVLC_EXPORT type name args

/*****************************************************************************
 * OS-specific headers and thread types
 *****************************************************************************/
#if defined( WIN32 ) || defined( UNDER_CE )
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

#include "vlc_mtime.h"
#include "vlc_threads.h"

/**
 * Memory storage space for an atom. Never access it directly.
 */
typedef union
{
    volatile uintptr_t u;
    volatile intptr_t  s;
} vlc_atomic_t;

/*****************************************************************************
 * Common structure members
 *****************************************************************************/

/* VLC_COMMON_MEMBERS : members common to all basic vlc objects */
#define VLC_COMMON_MEMBERS                                                  \
/** \name VLC_COMMON_MEMBERS                                                \
 * these members are common for all vlc objects                             \
 */                                                                         \
/**@{*/                                                                     \
    const char *psz_object_type;                                            \
                                                                            \
    /* Messages header */                                                   \
    char *psz_header;                                                       \
    int  i_flags;                                                           \
                                                                            \
    /* Object properties */                                                 \
    volatile bool b_die;                   /**< set by the outside */ \
    bool b_force;      /**< set by the outside (eg. module_need()) */ \
                                                                            \
    /* Stuff related to the libvlc structure */                             \
    libvlc_int_t *p_libvlc;                  /**< (root of all evil) - 1 */ \
                                                                            \
    vlc_object_t *  p_parent;                            /**< our parent */ \
                                                                            \
/**@}*/                                                                     \

/* VLC_OBJECT: attempt at doing a clever cast */
#if defined( __GNUC__ ) && __GNUC__ > 3
# ifndef __cplusplus
#  define VLC_OBJECT( x ) \
    __builtin_choose_expr(__builtin_offsetof(__typeof__(*x), psz_object_type), \
                          (void)0 /* screw you */, (vlc_object_t *)(x))
# else
#  define VLC_OBJECT( x ) \
    ((vlc_object_t *)(x) \
      + 0 * __builtin_offsetof(__typeof__(*x), psz_object_type))
# endif
#else
# define VLC_OBJECT( x ) ((vlc_object_t *)(x))
#endif

typedef struct gc_object_t
{
    vlc_atomic_t    refs;
    void          (*pf_destructor) (struct gc_object_t *);
} gc_object_t;

/**
 * These members are common to all objects that wish to be garbage-collected.
 */
#define VLC_GC_MEMBERS gc_object_t vlc_gc_data;

VLC_EXPORT(void *, vlc_gc_init, (gc_object_t *, void (*)(gc_object_t *)));
VLC_EXPORT(void *, vlc_hold, (gc_object_t *));
VLC_EXPORT(void, vlc_release, (gc_object_t *));

#define vlc_gc_init( a,b ) vlc_gc_init( &(a)->vlc_gc_data, (b) )
#define vlc_gc_incref( a ) vlc_hold( &(a)->vlc_gc_data )
#define vlc_gc_decref( a ) vlc_release( &(a)->vlc_gc_data )
#define vlc_priv( gc, t ) ((t *)(((char *)(gc)) - offsetof(t, vlc_gc_data)))

/*****************************************************************************
 * Macros and inline functions
 *****************************************************************************/

/* CEIL: division with round to nearest greater integer */
#define CEIL(n, d)  ( ((n) / (d)) + ( ((n) % (d)) ? 1 : 0) )

/* PAD: PAD(n, d) = CEIL(n ,d) * d */
#define PAD(n, d)   ( ((n) % (d)) ? ((((n) / (d)) + 1) * (d)) : (n) )

/* __MAX and __MIN: self explanatory */
#ifndef __MAX
#   define __MAX(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __MIN
#   define __MIN(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

LIBVLC_USED
static inline int64_t GCD ( int64_t a, int64_t b )
{
    while( b )
    {
        int64_t c = a % b;
        a = b;
        b = c;
    }
    return a;
}

/* function imported from libavutil/common.h */
LIBVLC_USED
static inline uint8_t clip_uint8_vlc( int32_t a )
{
    if( a&(~255) ) return (-a)>>31;
    else           return a;
}

/* Count leading zeroes */
LIBVLC_USED
static inline unsigned clz (unsigned x)
{
#ifdef __GNUC_
    return __builtin_clz (x);
#else
    unsigned i = sizeof (x) * 8;

    while (x)
    {
        x = x >> 1;
        i--;
    }
    return i;
#endif
}

#define clz8( x ) (clz(x) - ((sizeof(unsigned) - sizeof (uint8_t)) * 8))
#define clz16( x ) (clz(x) - ((sizeof(unsigned) - sizeof (uint16_t)) * 8))
/* XXX: this assumes that int is 32-bits or more */
#define clz32( x ) (clz(x) - ((sizeof(unsigned) - sizeof (uint32_t)) * 8))

/* Free and set set the variable to NULL */
#define FREENULL(a) do { free( a ); a = NULL; } while(0)

#define EMPTY_STR(str) (!str || !*str)

VLC_EXPORT( char const *, vlc_error, ( int ) LIBVLC_USED );

#include <vlc_arrays.h>

/* MSB (big endian)/LSB (little endian) conversions - network order is always
 * MSB, and should be used for both network communications and files. */
LIBVLC_USED
static inline uint16_t U16_AT( const void * _p )
{
    const uint8_t * p = (const uint8_t *)_p;
    return ( ((uint16_t)p[0] << 8) | p[1] );
}

LIBVLC_USED
static inline uint32_t U32_AT( const void * _p )
{
    const uint8_t * p = (const uint8_t *)_p;
    return ( ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
              | ((uint32_t)p[2] << 8) | p[3] );
}

LIBVLC_USED
static inline uint64_t U64_AT( const void * _p )
{
    const uint8_t * p = (const uint8_t *)_p;
    return ( ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48)
              | ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32)
              | ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16)
              | ((uint64_t)p[6] << 8) | p[7] );
}

LIBVLC_USED
static inline uint16_t GetWLE( const void * _p )
{
    const uint8_t * p = (const uint8_t *)_p;
    return ( ((uint16_t)p[1] << 8) | p[0] );
}

LIBVLC_USED
static inline uint32_t GetDWLE( const void * _p )
{
    const uint8_t * p = (const uint8_t *)_p;
    return ( ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16)
              | ((uint32_t)p[1] << 8) | p[0] );
}

LIBVLC_USED
static inline uint64_t GetQWLE( const void * _p )
{
    const uint8_t * p = (const uint8_t *)_p;
    return ( ((uint64_t)p[7] << 56) | ((uint64_t)p[6] << 48)
              | ((uint64_t)p[5] << 40) | ((uint64_t)p[4] << 32)
              | ((uint64_t)p[3] << 24) | ((uint64_t)p[2] << 16)
              | ((uint64_t)p[1] << 8) | p[0] );
}

#define GetWBE( p )     U16_AT( p )
#define GetDWBE( p )    U32_AT( p )
#define GetQWBE( p )    U64_AT( p )

/* Helper writer functions */
#define SetWLE( p, v ) _SetWLE( (uint8_t*)(p), v)
static inline void _SetWLE( uint8_t *p, uint16_t i_dw )
{
    p[1] = ( i_dw >>  8 )&0xff;
    p[0] = ( i_dw       )&0xff;
}

#define SetDWLE( p, v ) _SetDWLE( (uint8_t*)(p), v)
static inline void _SetDWLE( uint8_t *p, uint32_t i_dw )
{
    p[3] = ( i_dw >> 24 )&0xff;
    p[2] = ( i_dw >> 16 )&0xff;
    p[1] = ( i_dw >>  8 )&0xff;
    p[0] = ( i_dw       )&0xff;
}
#define SetQWLE( p, v ) _SetQWLE( (uint8_t*)(p), v)
static inline void _SetQWLE( uint8_t *p, uint64_t i_qw )
{
    SetDWLE( p,   i_qw&0xffffffff );
    SetDWLE( p+4, ( i_qw >> 32)&0xffffffff );
}
#define SetWBE( p, v ) _SetWBE( (uint8_t*)(p), v)
static inline void _SetWBE( uint8_t *p, uint16_t i_dw )
{
    p[0] = ( i_dw >>  8 )&0xff;
    p[1] = ( i_dw       )&0xff;
}

#define SetDWBE( p, v ) _SetDWBE( (uint8_t*)(p), v)
static inline void _SetDWBE( uint8_t *p, uint32_t i_dw )
{
    p[0] = ( i_dw >> 24 )&0xff;
    p[1] = ( i_dw >> 16 )&0xff;
    p[2] = ( i_dw >>  8 )&0xff;
    p[3] = ( i_dw       )&0xff;
}
#define SetQWBE( p, v ) _SetQWBE( (uint8_t*)(p), v)
static inline void _SetQWBE( uint8_t *p, uint64_t i_qw )
{
    SetDWBE( p+4,   i_qw&0xffffffff );
    SetDWBE( p, ( i_qw >> 32)&0xffffffff );
}

#define hton16(i) htons(i)
#define hton32(i) htonl(i)
#define ntoh16(i) ntohs(i)
#define ntoh32(i) ntohl(i)

LIBVLC_USED
static inline uint64_t ntoh64 (uint64_t ll)
{
    union { uint64_t qw; uint8_t b[16]; } v = { ll };
    return ((uint64_t)v.b[0] << 56)
         | ((uint64_t)v.b[1] << 48)
         | ((uint64_t)v.b[2] << 40)
         | ((uint64_t)v.b[3] << 32)
         | ((uint64_t)v.b[4] << 24)
         | ((uint64_t)v.b[5] << 16)
         | ((uint64_t)v.b[6] <<  8)
         | ((uint64_t)v.b[7] <<  0);
}
#define hton64(i) ntoh64(i)

/* */
#define VLC_UNUSED(x) (void)(x)

/* Stuff defined in src/extras/libc.c */

#if defined(WIN32) || defined(UNDER_CE)
/* win32, cl and icl support */
#   if defined( _MSC_VER ) || !defined( __MINGW32__ )
#       define __attribute__(x)
#       define __inline__      __inline
#       define S_IFBLK         0x3000  /* Block */
#       define S_ISBLK(m)      (0)
#       define S_ISCHR(m)      (0)
#       define S_ISFIFO(m)     (((m)&_S_IFMT) == _S_IFIFO)
#       define S_ISREG(m)      (((m)&_S_IFMT) == _S_IFREG)
#   endif

/* several type definitions */
#   if defined( __MINGW32__ )
#       if !defined( _OFF_T_ )
            typedef long long _off_t;
            typedef _off_t off_t;
#           define _OFF_T_
#       else
#           ifdef off_t
#               undef off_t
#           endif
#           define off_t long long
#       endif
#   endif

#   if defined( _MSC_VER ) && !defined( __WXMSW__ )
#       if !defined( _OFF_T_DEFINED )
            typedef __int64 off_t;
#           define _OFF_T_DEFINED
#       else
            /* for wx compatibility typedef long off_t; */
#           define off_t __int64
#       endif
#   endif

#   if defined( __BORLANDC__ )
#       undef off_t
#       define off_t unsigned __int64
#   endif

#   ifndef O_NONBLOCK
#       define O_NONBLOCK 0
#   endif

#   ifndef alloca
#       define alloca _alloca
#   endif

#   include <tchar.h>
#endif

VLC_EXPORT( bool, vlc_ureduce, ( unsigned *, unsigned *, uint64_t, uint64_t, uint64_t ) );

VLC_EXPORT( void *, vlc_memalign, ( void **base, size_t alignment, size_t size ) LIBVLC_USED );

/* execve wrapper (defined in src/extras/libc.c) */
VLC_EXPORT( int, vlc_execve, ( vlc_object_t *p_object, int i_argc, char *const *pp_argv, char *const *pp_env, const char *psz_cwd, const char *p_in, size_t i_in, char **pp_data, size_t *pi_data ) LIBVLC_USED );
#define vlc_execve(a,b,c,d,e,f,g,h,i) vlc_execve(VLC_OBJECT(a),b,c,d,e,f,g,h,i)

VLC_EXPORT( void, vlc_tdestroy, ( void *, void (*)(void *) ) );

/* Fast large memory copy and memory set */
VLC_EXPORT( void *, vlc_memcpy, ( void *, const void *, size_t ) );
VLC_EXPORT( void *, vlc_memset, ( void *, int, size_t ) );

/*****************************************************************************
 * I18n stuff
 *****************************************************************************/
VLC_EXPORT( char *, vlc_gettext, ( const char *msgid ) LIBVLC_FORMAT_ARG(1) );

#define vlc_pgettext( ctx, id ) \
        vlc_pgettext_aux( ctx "\004" id, id )

LIBVLC_FORMAT_ARG(2)
static inline const char *vlc_pgettext_aux( const char *ctx, const char *id )
{
    const char *tr = vlc_gettext( ctx );
    return (tr == ctx) ? id : tr;
}

/*****************************************************************************
 * Loosy memory allocation functions. Do not use in new code.
 *****************************************************************************/
static inline void *xmalloc (size_t len)
{
    void *ptr = malloc (len);
    if (unlikely (ptr == NULL))
        abort ();
    return ptr;
}

static inline void *xrealloc (void *ptr, size_t len)
{
    void *nptr = realloc (ptr, len);
    if (unlikely (nptr == NULL))
        abort ();
    return nptr;
}

/*****************************************************************************
 * libvlc features
 *****************************************************************************/
VLC_EXPORT( const char *, VLC_CompileBy, ( void ) LIBVLC_USED );
VLC_EXPORT( const char *, VLC_CompileHost, ( void ) LIBVLC_USED );
VLC_EXPORT( const char *, VLC_Compiler, ( void ) LIBVLC_USED );

/*****************************************************************************
 * Additional vlc stuff
 *****************************************************************************/
#include "vlc_messages.h"
#include "vlc_objects.h"
#include "vlc_variables.h"
#include "vlc_main.h"
#include "vlc_configuration.h"

#if defined( WIN32 ) || defined( UNDER_CE ) || defined( __SYMBIAN32__ )
#   define DIR_SEP_CHAR '\\'
#   define DIR_SEP "\\"
#   define PATH_SEP_CHAR ';'
#   define PATH_SEP ";"
#else
#   define DIR_SEP_CHAR '/'
#   define DIR_SEP "/"
#   define PATH_SEP_CHAR ':'
#   define PATH_SEP ":"
#endif

#define LICENSE_MSG \
  _("This program comes with NO WARRANTY, to the extent permitted by " \
    "law.\nYou may redistribute it under the terms of the GNU General " \
    "Public License;\nsee the file named COPYING for details.\n" \
    "Written by the VideoLAN team; see the AUTHORS file.\n")

#endif /* !VLC_COMMON_H */
