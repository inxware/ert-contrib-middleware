/*****************************************************************************
 * taglib.cpp: Taglib tag parser/writer
 *****************************************************************************
 * Copyright (C) 2003-2009 the VideoLAN team
 * $Id: 9ddb26e331c99bf9b96208e7d08cc0e94a6aa698 $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Rafaël Carré <funman@videolanorg>
 *          Rémi Duraffort <ivoire@videolan.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_playlist.h>
#include <vlc_meta.h>
#include <vlc_demux.h>
#include <vlc_strings.h>
#include <vlc_charset.h>
#include <vlc_url.h>

#ifdef WIN32
# include <io.h>
#else
# include <unistd.h>
#endif


// Taglib headers
#include <fileref.h>
#include <tag.h>
#include <tbytevector.h>

#include <apetag.h>
#include <id3v2tag.h>
#include <xiphcomment.h>

#include <flacfile.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <oggflacfile.h>

#ifdef TAGLIB_WITH_ASF
# include <aifffile.h>
# include <wavfile.h>
#endif

#ifdef TAGLIB_WITH_MP4
# include <mp4file.h>
#endif

#include <speexfile.h>
#include <trueaudiofile.h>
#include <vorbisfile.h>
#include <wavpackfile.h>

#include <attachedpictureframe.h>
#include <textidentificationframe.h>
#include <uniquefileidentifierframe.h>


// Local functions
static int ReadMeta    ( vlc_object_t * );
static int WriteMeta   ( vlc_object_t * );

vlc_module_begin ()
    set_capability( "meta reader", 1000 )
    set_callbacks( ReadMeta, NULL )
    add_submodule ()
        set_capability( "meta writer", 50 )
        set_callbacks( WriteMeta, NULL )
vlc_module_end ()

using namespace TagLib;


/**
 * Read meta informations from APE tags
 * @param tag: the APE tag
 * @param p_demux; the demux object
 * @param p_demux_meta: the demuxer meta
 * @param p_meta: the meta
 */
static void ReadMetaFromAPE( APE::Tag* tag, demux_t* p_demux, demux_meta_t* p_demux_meta, vlc_meta_t* p_meta )
{
    APE::Item item;
#define SET( keyName, metaName ) \
    item = tag->itemListMap()[keyName]; \
    vlc_meta_Set##metaName( p_meta, item.toString().toCString( true ) );\

    SET( "COPYRIGHT", Copyright );
    SET( "LANGUAGE", Language );
    SET( "PUBLISHER", Publisher );

#undef SET
}



/**
 * Read meta information from id3v2 tags
 * @param tag: the id3v2 tag
 * @param p_demux; the demux object
 * @param p_demux_meta: the demuxer meta
 * @param p_meta: the meta
 */
static void ReadMetaFromId3v2( ID3v2::Tag* tag, demux_t* p_demux, demux_meta_t* p_demux_meta, vlc_meta_t* p_meta )
{
    // Get the unique file identifier
    ID3v2::FrameList list = tag->frameListMap()["UFID"];
    ID3v2::FrameList::Iterator iter;
    for( iter = list.begin(); iter != list.end(); iter++ )
    {
        ID3v2::UniqueFileIdentifierFrame* p_ufid =
                dynamic_cast<ID3v2::UniqueFileIdentifierFrame*>(*iter);
        const char *owner = p_ufid->owner().toCString();
        if (!strcmp( owner, "http://musicbrainz.org" ))
        {
            /* ID3v2 UFID contains up to 64 bytes binary data
             * but in our case it will be a '\0'
             * terminated string */
            char psz_ufid[64];
            int max_size = __MIN( p_ufid->identifier().size(), 63);
            strncpy( psz_ufid, p_ufid->identifier().data(), max_size );
            psz_ufid[max_size] = '\0';
            vlc_meta_SetTrackID( p_meta, psz_ufid );
        }
    }

    // Get the use text
    list = tag->frameListMap()["TXXX"];
    for( iter = list.begin(); iter != list.end(); iter++ )
    {
        ID3v2::UserTextIdentificationFrame* p_txxx =
                dynamic_cast<ID3v2::UserTextIdentificationFrame*>(*iter);
        vlc_meta_AddExtra( p_meta, p_txxx->description().toCString( true ),
                           p_txxx->fieldList().toString().toCString( true ) );
    }

    // Get some more informations
#define SET( tagName, metaName )                                               \
    list = tag->frameListMap()[tagName];                                       \
    if( !list.isEmpty() )                                                      \
        vlc_meta_Set##metaName( p_meta,                                        \
                                (*list.begin())->toString().toCString( true ) );

    SET( "TCOP", Copyright );
    SET( "TENC", EncodedBy );
    SET( "TLAN", Language );
    SET( "TPUB", Publisher );

#undef SET

    /* Preferred type of image
     * The 21 types are defined in id3v2 standard:
     * http://www.id3.org/id3v2.4.0-frames */
    static const int pi_cover_score[] = {
        0,  /* Other */
        5,  /* 32x32 PNG image that should be used as the file icon */
        4,  /* File icon of a different size or format. */
        20, /* Front cover image of the album. */
        19, /* Back cover image of the album. */
        13, /* Inside leaflet page of the album. */
        18, /* Image from the album itself. */
        17, /* Picture of the lead artist or soloist. */
        16, /* Picture of the artist or performer. */
        14, /* Picture of the conductor. */
        15, /* Picture of the band or orchestra. */
        9,  /* Picture of the composer. */
        8,  /* Picture of the lyricist or text writer. */
        7,  /* Picture of the recording location or studio. */
        10, /* Picture of the artists during recording. */
        11, /* Picture of the artists during performance. */
        6,  /* Picture from a movie or video related to the track. */
        1,  /* Picture of a large, coloured fish. */
        12, /* Illustration related to the track. */
        3,  /* Logo of the band or performer. */
        2   /* Logo of the publisher (record company). */
    };
    int i_score = -1;

    // Try now to get embedded art
    list = tag->frameListMap()[ "APIC" ];
    if( list.isEmpty() )
        return;

    TAB_INIT( p_demux_meta->i_attachments, p_demux_meta->attachments );
    for( iter = list.begin(); iter != list.end(); iter++ )
    {
        ID3v2::AttachedPictureFrame* p_apic =
            dynamic_cast<ID3v2::AttachedPictureFrame*>(*iter);
        input_attachment_t *p_attachment;

        const char *psz_mime;
        char *psz_name, *psz_description;

        // Get the mime and description of the image.
        // If the description is empty, take the type as a description
        psz_mime = p_apic->mimeType().toCString( true );
        if( p_apic->description().size() > 0 )
            psz_description = strdup( p_apic->description().toCString( true ) );
        else
        {
            if( asprintf( &psz_description, "%i", p_apic->type() ) == -1 )
                psz_description = NULL;
        }

        if( !psz_description )
            continue;
        psz_name = psz_description;

        /* some old iTunes version not only sets incorrectly the mime type
         * or the description of the image,
         * but also embeds incorrectly the image.
         * Recent versions seem to behave correctly */
        if( !strncmp( psz_mime, "PNG", 3 ) ||
            !strncmp( psz_name, "\xC2\x89PNG", 5 ) )
        {
            msg_Warn( p_demux_meta, "Invalid picture embedded by broken iTunes version" );
            free( psz_description );
            continue;
        }

        const ByteVector picture = p_apic->picture();
        const char *p_data = picture.data();
        const unsigned i_data = picture.size();

        msg_Dbg( p_demux_meta, "Found embedded art: %s (%s) is %u bytes",
                 psz_name, psz_mime, i_data );

        p_attachment = vlc_input_attachment_New( psz_name, psz_mime,
                                psz_description, p_data, i_data );
        if( p_attachment )
            TAB_APPEND_CAST( (input_attachment_t**),
                             p_demux_meta->i_attachments, p_demux_meta->attachments,
                             p_attachment );
        free( psz_description );

        if( pi_cover_score[p_apic->type()] > i_score )
        {
            i_score = pi_cover_score[p_apic->type()];
            char *psz_url;
            if( asprintf( &psz_url, "attachment://%s",
                          p_attachment->psz_name ) == -1 )
                continue;
            vlc_meta_SetArtURL( p_meta, psz_url );
            free( psz_url );
        }
    }
}



/**
 * Read the meta informations from XiphComments
 * @param tag: the Xiph Comment
 * @param p_demux; the demux object
 * @param p_demux_meta: the demuxer meta
 * @param p_meta: the meta
 */
static void ReadMetaFromXiph( Ogg::XiphComment* tag, demux_t* p_demux, demux_meta_t* p_demux_meta, vlc_meta_t* p_meta )
{
#define SET( keyName, metaName )                                               \
    StringList list = tag->fieldListMap()[keyName];                            \
    if( !list.isEmpty() )                                                      \
        vlc_meta_Set##metaName( p_meta, (*list.begin()).toCString( true ) );

    SET( "COPYRIGHT", Copyright );
#undef SET

    // Try now to get embedded art
    StringList mime_list = tag->fieldListMap()[ "COVERARTMIME" ];
    StringList art_list = tag->fieldListMap()[ "COVERART" ];

    // We get only the first covert art
    if( mime_list.size() > 1 || art_list.size() > 1 )
        msg_Warn( p_demux_meta, "Found %i embedded arts, so using only the first one",
                  art_list.size() );
    else if( mime_list.size() == 0 || art_list.size() == 0 )
        return;

    input_attachment_t *p_attachment;

    const char* psz_name = "cover";
    const char* psz_mime = mime_list[0].toCString(true);
    const char* psz_description = "cover";

    uint8_t *p_data;
    int i_data = vlc_b64_decode_binary( &p_data, art_list[0].toCString(true) );

    msg_Dbg( p_demux_meta, "Found embedded art: %s (%s) is %i bytes",
             psz_name, psz_mime, i_data );

    TAB_INIT( p_demux_meta->i_attachments, p_demux_meta->attachments );
              p_attachment = vlc_input_attachment_New( psz_name, psz_mime,
              psz_description, p_data, i_data );
    free( p_data );

    TAB_APPEND_CAST( (input_attachment_t**),
                     p_demux_meta->i_attachments, p_demux_meta->attachments,
                     p_attachment );

    vlc_meta_SetArtURL( p_meta, "attachment://cover" );
}

#if defined(TAGLIB_WITH_MP4) && defined(HAVE_TAGLIB_MP4COVERART_H)
static void ReadMetaFromMP4( MP4::Tag* tag, demux_t *p_demux, demux_meta_t *p_demux_meta, vlc_meta_t* p_meta )
{
    if( tag->itemListMap().contains("covr") )
    {
        MP4::CoverArtList list = tag->itemListMap()["covr"].toCoverArtList();
        const char *psz_format = list[0].format() == MP4::CoverArt::PNG ? "image/png" : "image/jpeg";

        TAB_INIT( p_demux_meta->i_attachments, p_demux_meta->attachments );
        input_attachment_t *p_attachment =
                vlc_input_attachment_New( "cover", psz_format, "cover",
                                          list[0].data().data(), list[0].data().size() );
        TAB_APPEND_CAST( (input_attachment_t**),
                         p_demux_meta->i_attachments, p_demux_meta->attachments,
                         p_attachment );
        vlc_meta_SetArtURL( p_meta, "attachment://cover" );
    }
}
#endif

/**
 * Get the tags from the file using TagLib
 * @param p_this: the demux object
 * @return VLC_SUCCESS if the operation success
 */
static int ReadMeta( vlc_object_t* p_this)
{
    demux_meta_t*   p_demux_meta = (demux_meta_t *)p_this;
    demux_t*        p_demux = p_demux_meta->p_demux;
    vlc_meta_t*     p_meta;
    FileRef f;

    p_demux_meta->p_meta = NULL;


#if defined(WIN32) || defined (UNDER_CE)
    wchar_t wpath[MAX_PATH + 1];
    if( !MultiByteToWideChar( CP_UTF8, 0, p_demux->psz_path, -1, wpath, MAX_PATH) )
        return VLC_EGENERIC;
    wpath[MAX_PATH] = L'\0';
    f = FileRef( wpath );
#else
    const char* local_name = ToLocale( p_demux->psz_path );
    if( !local_name )
        return VLC_EGENERIC;
    f = FileRef( local_name );
    LocaleFree( local_name );
#endif

    if( f.isNull() )
        return VLC_EGENERIC;
    if( !f.tag() || f.tag()->isEmpty() )
        return VLC_EGENERIC;

    p_demux_meta->p_meta = p_meta = vlc_meta_New();
    if( !p_meta )
        return VLC_ENOMEM;


    // Read the tags from the file
    Tag* p_tag = f.tag();

#define SET( tag, meta )                                                       \
    if( !p_tag->tag().isNull() && !p_tag->tag().isEmpty() )                    \
        vlc_meta_Set##meta( p_meta, p_tag->tag().toCString(true) )
#define SETINT( tag, meta )                                                    \
    if( p_tag->tag() )                                                         \
    {                                                                          \
        char psz_tmp[10];                                                      \
        snprintf( psz_tmp, 10, "%d", p_tag->tag() );                           \
        vlc_meta_Set##meta( p_meta, psz_tmp );                                 \
    }

    SET( title, Title );
    SET( artist, Artist );
    SET( album, Album );
    SET( comment, Description );
    SET( genre, Genre );
    SETINT( year, Date );
    SETINT( track, TrackNum );

#undef SETINT
#undef SET


    // Try now to read special tags
    if( FLAC::File* flac = dynamic_cast<FLAC::File*>(f.file()) )
    {
        if( flac->ID3v2Tag() )
            ReadMetaFromId3v2( flac->ID3v2Tag(), p_demux, p_demux_meta, p_meta );
        else if( flac->xiphComment() )
            ReadMetaFromXiph( flac->xiphComment(), p_demux, p_demux_meta, p_meta );
    }
#if defined(TAGLIB_WITH_MP4) && defined(HAVE_TAGLIB_MP4COVERART_H)
    else if( MP4::File *mp4 = dynamic_cast<MP4::File*>(f.file()) )
    {
        if( mp4->tag() )
            ReadMetaFromMP4( mp4->tag(), p_demux, p_demux_meta, p_meta );
    }
#endif
    else if( MPC::File* mpc = dynamic_cast<MPC::File*>(f.file()) )
    {
        if( mpc->APETag() )
            ReadMetaFromAPE( mpc->APETag(), p_demux, p_demux_meta, p_meta );
    }
    else if( MPEG::File* mpeg = dynamic_cast<MPEG::File*>(f.file()) )
    {
        if( mpeg->ID3v2Tag() )
            ReadMetaFromId3v2( mpeg->ID3v2Tag(), p_demux, p_demux_meta, p_meta );
        else if( mpeg->APETag() )
            ReadMetaFromAPE( mpeg->APETag(), p_demux, p_demux_meta, p_meta );
    }
    else if( Ogg::File* ogg = dynamic_cast<Ogg::File*>(f.file()) )
    {
        if( Ogg::FLAC::File* ogg_flac = dynamic_cast<Ogg::FLAC::File*>(f.file()))
            ReadMetaFromXiph( ogg_flac->tag(), p_demux, p_demux_meta, p_meta );
        else if( Ogg::Speex::File* ogg_speex = dynamic_cast<Ogg::Speex::File*>(f.file()) )
            ReadMetaFromXiph( ogg_speex->tag(), p_demux, p_demux_meta, p_meta );
        else if( Ogg::Vorbis::File* ogg_vorbis = dynamic_cast<Ogg::Vorbis::File*>(f.file()) )
            ReadMetaFromXiph( ogg_vorbis->tag(), p_demux, p_demux_meta, p_meta );
    }
#ifdef TAGLIB_WITH_ASF
    else if( RIFF::File* riff = dynamic_cast<RIFF::File*>(f.file()) )
    {
        if( RIFF::AIFF::File* riff_aiff = dynamic_cast<RIFF::AIFF::File*>(f.file()) )
            ReadMetaFromId3v2( riff_aiff->tag(), p_demux, p_demux_meta, p_meta );
        else if( RIFF::WAV::File* riff_wav = dynamic_cast<RIFF::WAV::File*>(f.file()) )
            ReadMetaFromId3v2( riff_wav->tag(), p_demux, p_demux_meta, p_meta );
    }
#endif
    else if( TrueAudio::File* trueaudio = dynamic_cast<TrueAudio::File*>(f.file()) )
    {
        if( trueaudio->ID3v2Tag() )
            ReadMetaFromId3v2( trueaudio->ID3v2Tag(), p_demux, p_demux_meta, p_meta );
    }
    else if( WavPack::File* wavpack = dynamic_cast<WavPack::File*>(f.file()) )
    {
        if( wavpack->APETag() )
            ReadMetaFromAPE( wavpack->APETag(), p_demux, p_demux_meta, p_meta );
    }

    return VLC_SUCCESS;
}



/**
 * Write meta informations to APE tags
 * @param tag: the APE tag
 * @param p_item: the input item
 */
static void WriteMetaToAPE( APE::Tag* tag, input_item_t* p_item )
{
    char* psz_meta;
#define WRITE( metaName, keyName )                      \
    psz_meta = input_item_Get##metaName( p_item );      \
    if( psz_meta )                                      \
    {                                                   \
        String key( keyName, String::UTF8 );            \
        String value( psz_meta, String::UTF8 );         \
        tag->addValue( key, value, true );              \
    }                                                   \
    free( psz_meta );

    WRITE( Copyright, "COPYRIGHT" );
    WRITE( Language, "LANGUAGE" );
    WRITE( Publisher, "PUBLISHER" );

#undef WRITE
}



/**
 * Write meta information to id3v2 tags
 * @param tag: the id3v2 tag
 * @param p_input: the input item
 */
static void WriteMetaToId3v2( ID3v2::Tag* tag, input_item_t* p_item )
{
    char* psz_meta;
#define WRITE( metaName, tagName )                                            \
    psz_meta = input_item_Get##metaName( p_item );                            \
    if( psz_meta )                                                            \
    {                                                                         \
        ByteVector p_byte( tagName, 4 );                                      \
        tag->removeFrames( p_byte );                                         \
        ID3v2::TextIdentificationFrame* p_frame =                             \
            new ID3v2::TextIdentificationFrame( p_byte, String::UTF8 );       \
        p_frame->setText( psz_meta );                                         \
        tag->addFrame( p_frame );                                             \
    }                                                                         \
    free( psz_meta );

    WRITE( Copyright, "TCOP" );
    WRITE( EncodedBy, "TENC" );
    WRITE( Language,  "TLAN" );
    WRITE( Publisher, "TPUB" );

#undef WRITE
}



/**
 * Write the meta informations to XiphComments
 * @param tag: the Xiph Comment
 * @param p_input: the input item
 */
static void WriteMetaToXiph( Ogg::XiphComment* tag, input_item_t* p_item )
{
    char* psz_meta;
#define WRITE( metaName, keyName )                      \
    psz_meta = input_item_Get##metaName( p_item );      \
    if( psz_meta )                                      \
    {                                                   \
        String key( keyName, String::UTF8 );            \
        String value( psz_meta, String::UTF8 );         \
        tag->addField( key, value, true );              \
    }                                                   \
    free( psz_meta );

    WRITE( Copyright, "COPYRIGHT" );

#undef WRITE
}



/**
 * Set the tags to the file using TagLib
 * @param p_this: the demux object
 * @return VLC_SUCCESS if the operation success
 */

static int WriteMeta( vlc_object_t *p_this )
{
    meta_export_t *p_export = (meta_export_t *)p_this;
    input_item_t *p_item = p_export->p_item;
    FileRef f;

    if( !p_item )
    {
        msg_Err( p_this, "Can't save meta data of an empty input" );
        return VLC_EGENERIC;
    }

#if defined(WIN32) || defined (UNDER_CE)
    wchar_t wpath[MAX_PATH + 1];
    if( !MultiByteToWideChar( CP_UTF8, 0, p_export->psz_file, -1, wpath, MAX_PATH) )
        return VLC_EGENERIC;
    wpath[MAX_PATH] = L'\0';
    f = FileRef( wpath );
#else
    const char* local_name = ToLocale( p_export->psz_file );
    if( !local_name )
        return VLC_EGENERIC;
    f = FileRef( local_name );
    LocaleFree( local_name );
#endif

    if( f.isNull() || !f.tag() || f.file()->readOnly() )
    {
        msg_Err( p_this, "File %s can't be opened for tag writing",
                 p_export->psz_file );
        return VLC_EGENERIC;
    }

    msg_Dbg( p_this, "Writing metadata for %s", p_export->psz_file );

    Tag *p_tag = f.tag();

    char *psz_meta;

#define SET( a, b )                                             \
    psz_meta = input_item_Get ## a( p_item );                   \
    if( psz_meta )                                              \
    {                                                           \
        String tmp( psz_meta, String::UTF8 );                   \
        p_tag->set##b( tmp );                                   \
    }                                                           \
    free( psz_meta );

    // Saving all common fields
    // If the title is empty, use the name
    SET( TitleFbName, Title );
    SET( Artist, Artist );
    SET( Album, Album );
    SET( Description, Comment );
    SET( Genre, Genre );

#undef SET

    psz_meta = input_item_GetDate( p_item );
    if( psz_meta ) p_tag->setYear( atoi( psz_meta ) );
    free( psz_meta );

    psz_meta = input_item_GetTrackNum( p_item );
    if( psz_meta ) p_tag->setTrack( atoi( psz_meta ) );
    free( psz_meta );


    // Try now to write special tags
    if( FLAC::File* flac = dynamic_cast<FLAC::File*>(f.file()) )
    {
        if( flac->ID3v2Tag() )
            WriteMetaToId3v2( flac->ID3v2Tag(), p_item );
        else if( flac->xiphComment() )
            WriteMetaToXiph( flac->xiphComment(), p_item );
    }
    else if( MPC::File* mpc = dynamic_cast<MPC::File*>(f.file()) )
    {
        if( mpc->APETag() )
            WriteMetaToAPE( mpc->APETag(), p_item );
    }
    else if( MPEG::File* mpeg = dynamic_cast<MPEG::File*>(f.file()) )
    {
        if( mpeg->ID3v2Tag() )
            WriteMetaToId3v2( mpeg->ID3v2Tag(), p_item );
        else if( mpeg->APETag() )
            WriteMetaToAPE( mpeg->APETag(), p_item );
    }
    else if( Ogg::File* ogg = dynamic_cast<Ogg::File*>(f.file()) )
    {
        if( Ogg::FLAC::File* ogg_flac = dynamic_cast<Ogg::FLAC::File*>(f.file()))
            WriteMetaToXiph( ogg_flac->tag(), p_item );
        else if( Ogg::Speex::File* ogg_speex = dynamic_cast<Ogg::Speex::File*>(f.file()) )
            WriteMetaToXiph( ogg_speex->tag(), p_item );
        else if( Ogg::Vorbis::File* ogg_vorbis = dynamic_cast<Ogg::Vorbis::File*>(f.file()) )
            WriteMetaToXiph( ogg_vorbis->tag(), p_item );
    }
#ifdef TAGLIB_WITH_ASF
    else if( RIFF::File* riff = dynamic_cast<RIFF::File*>(f.file()) )
    {
        if( RIFF::AIFF::File* riff_aiff = dynamic_cast<RIFF::AIFF::File*>(f.file()) )
            WriteMetaToId3v2( riff_aiff->tag(), p_item );
        else if( RIFF::WAV::File* riff_wav = dynamic_cast<RIFF::WAV::File*>(f.file()) )
            WriteMetaToId3v2( riff_wav->tag(), p_item );
    }
#endif
    else if( TrueAudio::File* trueaudio = dynamic_cast<TrueAudio::File*>(f.file()) )
    {
        if( trueaudio->ID3v2Tag() )
            WriteMetaToId3v2( trueaudio->ID3v2Tag(), p_item );
    }
    else if( WavPack::File* wavpack = dynamic_cast<WavPack::File*>(f.file()) )
    {
        if( wavpack->APETag() )
            WriteMetaToAPE( wavpack->APETag(), p_item );
    }

    // Save the meta data
    f.save();

    return VLC_SUCCESS;
}

