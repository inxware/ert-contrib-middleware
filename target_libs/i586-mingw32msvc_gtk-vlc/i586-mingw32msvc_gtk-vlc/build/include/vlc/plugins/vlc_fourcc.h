/*****************************************************************************
 * vlc_fourcc.h: Definition of various FOURCC and helpers
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * $Id: e6edd88fbf1c1f83e0adc2ede8bc0f99de45a23d $
 *
 * Authors: Laurent Aimar <fenrir _AT_ videolan _DOT_ com>
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

#ifndef VLC_FOURCC_H
#define VLC_FOURCC_H 1

#include <vlc_common.h>
#include <vlc_es.h>

/* Video codec */
#define VLC_CODEC_MPGV      VLC_FOURCC('m','p','g','v')
#define VLC_CODEC_MP4V      VLC_FOURCC('m','p','4','v')
#define VLC_CODEC_DIV1      VLC_FOURCC('D','I','V','1')
#define VLC_CODEC_DIV2      VLC_FOURCC('D','I','V','2')
#define VLC_CODEC_DIV3      VLC_FOURCC('D','I','V','3')
#define VLC_CODEC_SVQ1      VLC_FOURCC('S','V','Q','1')
#define VLC_CODEC_SVQ3      VLC_FOURCC('S','V','Q','3')
#define VLC_CODEC_H264      VLC_FOURCC('h','2','6','4')
#define VLC_CODEC_H263      VLC_FOURCC('h','2','6','3')
#define VLC_CODEC_H263I     VLC_FOURCC('I','2','6','3')
#define VLC_CODEC_H263P     VLC_FOURCC('I','L','V','R')
#define VLC_CODEC_FLV1      VLC_FOURCC('F','L','V','1')
#define VLC_CODEC_H261      VLC_FOURCC('h','2','6','1')
#define VLC_CODEC_MJPG      VLC_FOURCC('M','J','P','G')
#define VLC_CODEC_MJPGB     VLC_FOURCC('m','j','p','b')
#define VLC_CODEC_LJPG      VLC_FOURCC('L','J','P','G')
#define VLC_CODEC_WMV1      VLC_FOURCC('W','M','V','1')
#define VLC_CODEC_WMV2      VLC_FOURCC('W','M','V','2')
#define VLC_CODEC_WMV3      VLC_FOURCC('W','M','V','3')
#define VLC_CODEC_WMVA      VLC_FOURCC('W','M','V','A')
#define VLC_CODEC_WMVP      VLC_FOURCC('W','M','V','P')
#define VLC_CODEC_VC1       VLC_FOURCC('V','C','-','1')
#define VLC_CODEC_THEORA    VLC_FOURCC('t','h','e','o')
#define VLC_CODEC_TARKIN    VLC_FOURCC('t','a','r','k')
#define VLC_CODEC_SNOW      VLC_FOURCC('S','N','O','W')
#define VLC_CODEC_DIRAC     VLC_FOURCC('d','r','a','c')
#define VLC_CODEC_CAVS      VLC_FOURCC('C','A','V','S')
#define VLC_CODEC_NUV       VLC_FOURCC('N','J','P','G')
#define VLC_CODEC_RV10      VLC_FOURCC('R','V','1','0')
#define VLC_CODEC_RV13      VLC_FOURCC('R','V','1','3')
#define VLC_CODEC_RV20      VLC_FOURCC('R','V','2','0')
#define VLC_CODEC_RV30      VLC_FOURCC('R','V','3','0')
#define VLC_CODEC_RV40      VLC_FOURCC('R','V','4','0')
#define VLC_CODEC_VP3       VLC_FOURCC('V','P','3',' ')
#define VLC_CODEC_VP5       VLC_FOURCC('V','P','5',' ')
#define VLC_CODEC_VP6       VLC_FOURCC('V','P','6','2')
#define VLC_CODEC_VP6F      VLC_FOURCC('V','P','6','F')
#define VLC_CODEC_VP6A      VLC_FOURCC('V','P','6','A')
#define VLC_CODEC_MSVIDEO1  VLC_FOURCC('M','S','V','C')
#define VLC_CODEC_FLIC      VLC_FOURCC('F','L','I','C')
#define VLC_CODEC_SP5X      VLC_FOURCC('S','P','5','X')
#define VLC_CODEC_DV        VLC_FOURCC('d','v',' ',' ')
#define VLC_CODEC_MSRLE     VLC_FOURCC('m','r','l','e')
#define VLC_CODEC_INDEO3    VLC_FOURCC('I','V','3','1')
#define VLC_CODEC_HUFFYUV   VLC_FOURCC('H','F','Y','U')
#define VLC_CODEC_FFVHUFF   VLC_FOURCC('F','F','V','H')
#define VLC_CODEC_ASV1      VLC_FOURCC('A','S','V','1')
#define VLC_CODEC_ASV2      VLC_FOURCC('A','S','V','2')
#define VLC_CODEC_FFV1      VLC_FOURCC('F','F','V','1')
#define VLC_CODEC_VCR1      VLC_FOURCC('V','C','R','1')
#define VLC_CODEC_CLJR      VLC_FOURCC('C','L','J','R')
#define VLC_CODEC_RPZA      VLC_FOURCC('r','p','z','a')
#define VLC_CODEC_SMC       VLC_FOURCC('s','m','c',' ')
#define VLC_CODEC_CINEPAK   VLC_FOURCC('C','V','I','D')
#define VLC_CODEC_TSCC      VLC_FOURCC('T','S','C','C')
#define VLC_CODEC_CSCD      VLC_FOURCC('C','S','C','D')
#define VLC_CODEC_ZMBV      VLC_FOURCC('Z','M','B','V')
#define VLC_CODEC_VMNC      VLC_FOURCC('V','M','n','c')
#define VLC_CODEC_FRAPS     VLC_FOURCC('F','P','S','1')
#define VLC_CODEC_TRUEMOTION1 VLC_FOURCC('D','U','C','K')
#define VLC_CODEC_TRUEMOTION2 VLC_FOURCC('T','M','2','0')
#define VLC_CODEC_QTRLE     VLC_FOURCC('r','l','e',' ')
#define VLC_CODEC_QDRAW     VLC_FOURCC('q','d','r','w')
#define VLC_CODEC_QPEG      VLC_FOURCC('Q','P','E','G')
#define VLC_CODEC_ULTI      VLC_FOURCC('U','L','T','I')
#define VLC_CODEC_VIXL      VLC_FOURCC('V','I','X','L')
#define VLC_CODEC_LOCO      VLC_FOURCC('L','O','C','O')
#define VLC_CODEC_WNV1      VLC_FOURCC('W','N','V','1')
#define VLC_CODEC_AASC      VLC_FOURCC('A','A','S','C')
#define VLC_CODEC_INDEO2    VLC_FOURCC('I','V','2','0')
#define VLC_CODEC_FLASHSV   VLC_FOURCC('F','S','V','1')
#define VLC_CODEC_KMVC      VLC_FOURCC('K','M','V','C')
#define VLC_CODEC_SMACKVIDEO VLC_FOURCC('S','M','K','2')
#define VLC_CODEC_DNXHD     VLC_FOURCC('A','V','d','n')
#define VLC_CODEC_8BPS      VLC_FOURCC('8','B','P','S')
#define VLC_CODEC_MIMIC     VLC_FOURCC('M','L','2','O')
#define VLC_CODEC_INTERPLAY VLC_FOURCC('i','m','v','e')
#define VLC_CODEC_IDCIN     VLC_FOURCC('I','D','C','I')
#define VLC_CODEC_4XM       VLC_FOURCC('4','X','M','V')
#define VLC_CODEC_ROQ       VLC_FOURCC('R','o','Q','v')
#define VLC_CODEC_MDEC      VLC_FOURCC('M','D','E','C')
#define VLC_CODEC_VMDVIDEO  VLC_FOURCC('V','M','D','V')
#define VLC_CODEC_CDG       VLC_FOURCC('C','D','G',' ')
#define VLC_CODEC_FRWU      VLC_FOURCC('F','R','W','U')
#define VLC_CODEC_AMV       VLC_FOURCC('A','M','V',' ')
#define VLC_CODEC_INDEO5    VLC_FOURCC('I','V','5','0')
#define VLC_CODEC_VP8       VLC_FOURCC('V','P','8','0')
#define VLC_CODEC_JPEG2000  VLC_FOURCC('J','P','2','K')


/* Planar YUV 4:1:0 Y:V:U */
#define VLC_CODEC_YV9       VLC_FOURCC('Y','V','U','9')
/* Planar YUV 4:2:0 Y:V:U */
#define VLC_CODEC_YV12      VLC_FOURCC('Y','V','1','2')
/* Planar YUV 4:1:0 Y:U:V */
#define VLC_CODEC_I410      VLC_FOURCC('I','4','1','0')
/* Planar YUV 4:1:1 Y:U:V */
#define VLC_CODEC_I411      VLC_FOURCC('I','4','1','1')
/* Planar YUV 4:2:0 Y:U:V */
#define VLC_CODEC_I420      VLC_FOURCC('I','4','2','0')
/* Planar YUV 4:2:2 Y:U:V */
#define VLC_CODEC_I422      VLC_FOURCC('I','4','2','2')
/* Planar YUV 4:4:0 Y:U:V */
#define VLC_CODEC_I440      VLC_FOURCC('I','4','4','0')
/* Planar YUV 4:4:4 Y:U:V */
#define VLC_CODEC_I444      VLC_FOURCC('I','4','4','4')
/* Planar YUV 4:2:0 Y:U:V full scale */
#define VLC_CODEC_J420      VLC_FOURCC('J','4','2','0')
/* Planar YUV 4:2:2 Y:U:V full scale */
#define VLC_CODEC_J422      VLC_FOURCC('J','4','2','2')
/* Planar YUV 4:4:0 Y:U:V full scale */
#define VLC_CODEC_J440      VLC_FOURCC('J','4','4','0')
/* Planar YUV 4:4:4 Y:U:V full scale */
#define VLC_CODEC_J444      VLC_FOURCC('J','4','4','4')
/* Palettized YUV with palette element Y:U:V:A */
#define VLC_CODEC_YUVP      VLC_FOURCC('Y','U','V','P')
/* Planar YUV 4:4:4 Y:U:V:A */
#define VLC_CODEC_YUVA      VLC_FOURCC('Y','U','V','A')
/* Palettized RGB with palette element R:G:B */
#define VLC_CODEC_RGBP      VLC_FOURCC('R','G','B','P')
/* 8 bits RGB */
#define VLC_CODEC_RGB8      VLC_FOURCC('R','G','B','8')
/* 15 bits RGB stored on 16 bits */
#define VLC_CODEC_RGB15     VLC_FOURCC('R','V','1','5')
/* 16 bits RGB store on a 16 bits */
#define VLC_CODEC_RGB16     VLC_FOURCC('R','V','1','6')
/* 24 bits RGB */
#define VLC_CODEC_RGB24     VLC_FOURCC('R','V','2','4')
/* 32 bits RGB */
#define VLC_CODEC_RGB32     VLC_FOURCC('R','V','3','2')
/* 32 bits VLC RGBA */
#define VLC_CODEC_RGBA      VLC_FOURCC('R','G','B','A')
/* 8 bits grey */
#define VLC_CODEC_GREY      VLC_FOURCC('G','R','E','Y')
/* Packed YUV 4:2:2, U:Y:V:Y */
#define VLC_CODEC_UYVY      VLC_FOURCC('U','Y','V','Y')
/* Packed YUV 4:2:2, V:Y:U:Y */
#define VLC_CODEC_VYUY      VLC_FOURCC('V','Y','U','Y')
/* Packed YUV 4:2:2, Y:U:Y:V */
#define VLC_CODEC_YUYV      VLC_FOURCC('Y','U','Y','2')
/* Packed YUV 4:2:2, Y:V:Y:U */
#define VLC_CODEC_YVYU      VLC_FOURCC('Y','V','Y','U')
/* Packed YUV 2:1:1, Y:U:Y:V */
#define VLC_CODEC_Y211      VLC_FOURCC('Y','2','1','1')
/* Packed YUV 4:2:2, U:Y:V:Y, reverted */
#define VLC_CODEC_CYUV      VLC_FOURCC('c','y','u','v')
/* 10-bit 4:2:2 Component YCbCr */
#define VLC_CODEC_V210      VLC_FOURCC('v','2','1','0')
/* Planar Y Packet UV (420) */
#define VLC_CODEC_NV12      VLC_FOURCC('N','V','1','2')

/* Image codec (video) */
#define VLC_CODEC_PNG       VLC_FOURCC('p','n','g',' ')
#define VLC_CODEC_PPM       VLC_FOURCC('p','p','m',' ')
#define VLC_CODEC_PGM       VLC_FOURCC('p','g','m',' ')
#define VLC_CODEC_PGMYUV    VLC_FOURCC('p','g','m','y')
#define VLC_CODEC_PAM       VLC_FOURCC('p','a','m',' ')
#define VLC_CODEC_JPEG      VLC_FOURCC('j','p','e','g')
#define VLC_CODEC_JPEGLS    VLC_FOURCC('M','J','L','S')
#define VLC_CODEC_BMP       VLC_FOURCC('b','m','p',' ')
#define VLC_CODEC_TIFF      VLC_FOURCC('t','i','f','f')
#define VLC_CODEC_GIF       VLC_FOURCC('g','i','f',' ')
#define VLC_CODEC_TARGA     VLC_FOURCC('t','g','a',' ')
#define VLC_CODEC_SGI       VLC_FOURCC('s','g','i',' ')
#define VLC_CODEC_PNM       VLC_FOURCC('p','n','m',' ')
#define VLC_CODEC_PCX       VLC_FOURCC('p','c','x',' ')


/* Audio codec */
#define VLC_CODEC_MPGA      VLC_FOURCC('m','p','g','a')
#define VLC_CODEC_MP4A      VLC_FOURCC('m','p','4','a')
#define VLC_CODEC_ALS       VLC_FOURCC('a','l','s',' ')
#define VLC_CODEC_A52       VLC_FOURCC('a','5','2',' ')
#define VLC_CODEC_EAC3      VLC_FOURCC('e','a','c','3')
#define VLC_CODEC_DTS       VLC_FOURCC('d','t','s',' ')
#define VLC_CODEC_WMA1      VLC_FOURCC('W','M','A','1')
#define VLC_CODEC_WMA2      VLC_FOURCC('W','M','A','2')
#define VLC_CODEC_WMAP      VLC_FOURCC('W','M','A','P')
#define VLC_CODEC_WMAL      VLC_FOURCC('W','M','A','L')
#define VLC_CODEC_WMAS      VLC_FOURCC('W','M','A','S')
#define VLC_CODEC_FLAC      VLC_FOURCC('f','l','a','c')
#define VLC_CODEC_MLP       VLC_FOURCC('m','l','p',' ')
#define VLC_CODEC_TRUEHD    VLC_FOURCC('t','r','h','d')
#define VLC_CODEC_DVAUDIO   VLC_FOURCC('d','v','a','u')
#define VLC_CODEC_SPEEX     VLC_FOURCC('s','p','x',' ')
#define VLC_CODEC_VORBIS    VLC_FOURCC('v','o','r','b')
#define VLC_CODEC_MACE3     VLC_FOURCC('M','A','C','3')
#define VLC_CODEC_MACE6     VLC_FOURCC('M','A','C','6')
#define VLC_CODEC_MUSEPACK7 VLC_FOURCC('M','P','C',' ')
#define VLC_CODEC_MUSEPACK8 VLC_FOURCC('M','P','C','K')
#define VLC_CODEC_RA_144    VLC_FOURCC('1','4','_','4')
#define VLC_CODEC_RA_288    VLC_FOURCC('2','8','_','8')
#define VLC_CODEC_ADPCM_4XM VLC_FOURCC('4','x','m','a')
#define VLC_CODEC_ADPCM_EA  VLC_FOURCC('A','D','E','A')
#define VLC_CODEC_INTERPLAY_DPCM VLC_FOURCC('i','d','p','c')
#define VLC_CODEC_ROQ_DPCM  VLC_FOURCC('R','o','Q','a')
#define VLC_CODEC_DSICINAUDIO VLC_FOURCC('D','C','I','A')
#define VLC_CODEC_ADPCM_XA  VLC_FOURCC('x','a',' ',' ')
#define VLC_CODEC_ADPCM_ADX VLC_FOURCC('a','d','x',' ')
#define VLC_CODEC_ADPCM_IMA_WS VLC_FOURCC('A','I','W','S')
#define VLC_CODEC_ADPCM_G722 VLC_FOURCC('g','7','2','2')
#define VLC_CODEC_ADPCM_G726 VLC_FOURCC('g','7','2','6')
#define VLC_CODEC_ADPCM_SWF VLC_FOURCC('S','W','F','a')
#define VLC_CODEC_ADPCM_MS  VLC_FOURCC('m','s',0x00,0x02)
#define VLC_CODEC_ADPCM_IMA_WAV VLC_FOURCC('m','s',0x00,0x11)
#define VLC_CODEC_VMDAUDIO  VLC_FOURCC('v','m','d','a')
#define VLC_CODEC_AMR_NB    VLC_FOURCC('s','a','m','r')
#define VLC_CODEC_AMR_WB    VLC_FOURCC('s','a','w','b')
#define VLC_CODEC_ALAC      VLC_FOURCC('a','l','a','c')
#define VLC_CODEC_QDM2      VLC_FOURCC('Q','D','M','2')
#define VLC_CODEC_COOK      VLC_FOURCC('c','o','o','k')
#define VLC_CODEC_SIPR      VLC_FOURCC('s','i','p','r')
#define VLC_CODEC_TTA       VLC_FOURCC('T','T','A','1')
#define VLC_CODEC_SHORTEN   VLC_FOURCC('s','h','n',' ')
#define VLC_CODEC_WAVPACK   VLC_FOURCC('W','V','P','K')
#define VLC_CODEC_GSM       VLC_FOURCC('g','s','m',' ')
#define VLC_CODEC_GSM_MS    VLC_FOURCC('a','g','s','m')
#define VLC_CODEC_ATRAC1    VLC_FOURCC('a','t','r','1')
#define VLC_CODEC_ATRAC3    VLC_FOURCC('a','t','r','c')
#define VLC_CODEC_SONIC     VLC_FOURCC('S','O','N','C')
#define VLC_CODEC_IMC       VLC_FOURCC(0x1,0x4,0x0,0x0)
#define VLC_CODEC_TRUESPEECH VLC_FOURCC(0x22,0x0,0x0,0x0)
#define VLC_CODEC_NELLYMOSER VLC_FOURCC('N','E','L','L')
#define VLC_CODEC_APE       VLC_FOURCC('A','P','E',' ')
#define VLC_CODEC_QCELP     VLC_FOURCC('Q','c','l','p')
#define VLC_CODEC_302M      VLC_FOURCC('3','0','2','m')
#define VLC_CODEC_DVD_LPCM  VLC_FOURCC('l','p','c','m')
#define VLC_CODEC_DVDA_LPCM VLC_FOURCC('a','p','c','m')
#define VLC_CODEC_BD_LPCM   VLC_FOURCC('b','p','c','m')
#define VLC_CODEC_SDDS      VLC_FOURCC('s','d','d','s')
#define VLC_CODEC_MIDI      VLC_FOURCC('M','I','D','I')
#define VLC_CODEC_S8        VLC_FOURCC('s','8',' ',' ')
#define VLC_CODEC_U8        VLC_FOURCC('u','8',' ',' ')
#define VLC_CODEC_S16L      VLC_FOURCC('s','1','6','l')
#define VLC_CODEC_S16B      VLC_FOURCC('s','1','6','b')
#define VLC_CODEC_U16L      VLC_FOURCC('u','1','6','l')
#define VLC_CODEC_U16B      VLC_FOURCC('u','1','6','b')
#define VLC_CODEC_S20B      VLC_FOURCC('s','2','0','b')
#define VLC_CODEC_S24L      VLC_FOURCC('s','2','4','l')
#define VLC_CODEC_S24B      VLC_FOURCC('s','2','4','b')
#define VLC_CODEC_U24L      VLC_FOURCC('u','2','4','l')
#define VLC_CODEC_U24B      VLC_FOURCC('u','2','4','b')
#define VLC_CODEC_S32L      VLC_FOURCC('s','3','2','l')
#define VLC_CODEC_S32B      VLC_FOURCC('s','3','2','b')
#define VLC_CODEC_U32L      VLC_FOURCC('u','3','2','l')
#define VLC_CODEC_U32B      VLC_FOURCC('u','3','2','b')
#define VLC_CODEC_F32L      VLC_FOURCC('f','3','2','l')
#define VLC_CODEC_F32B      VLC_FOURCC('f','3','2','b')
#define VLC_CODEC_F64L      VLC_FOURCC('f','6','4','l')
#define VLC_CODEC_F64B      VLC_FOURCC('f','6','4','b')

#define VLC_CODEC_ALAW      VLC_FOURCC('a','l','a','w')
#define VLC_CODEC_MULAW     VLC_FOURCC('m','l','a','w')
#define VLC_CODEC_DAT12     VLC_FOURCC('L','P','1','2')
#define VLC_CODEC_S24DAUD   VLC_FOURCC('d','a','u','d')
#define VLC_CODEC_FI32      VLC_FOURCC('f','i','3','2')
#define VLC_CODEC_TWINVQ    VLC_FOURCC('T','W','I','N')
#define VLC_CODEC_ADPCM_IMA_AMV VLC_FOURCC('i','m','a','v')

/* Subtitle */
#define VLC_CODEC_SPU       VLC_FOURCC('s','p','u',' ')
#define VLC_CODEC_DVBS      VLC_FOURCC('d','v','b','s')
#define VLC_CODEC_SUBT      VLC_FOURCC('s','u','b','t')
#define VLC_CODEC_XSUB      VLC_FOURCC('X','S','U','B')
#define VLC_CODEC_SSA       VLC_FOURCC('s','s','a',' ')
#define VLC_CODEC_TEXT      VLC_FOURCC('T','E','X','T')
#define VLC_CODEC_TELETEXT  VLC_FOURCC('t','e','l','x')
#define VLC_CODEC_KATE      VLC_FOURCC('k','a','t','e')
#define VLC_CODEC_CMML      VLC_FOURCC('c','m','m','l')
#define VLC_CODEC_ITU_T140  VLC_FOURCC('t','1','4','0')
#define VLC_CODEC_USF       VLC_FOURCC('u','s','f',' ')
#define VLC_CODEC_OGT       VLC_FOURCC('o','g','t',' ')
#define VLC_CODEC_CVD       VLC_FOURCC('c','v','d',' ')
/* Blu-ray Presentation Graphics */
#define VLC_CODEC_BD_PG     VLC_FOURCC('b','d','p','g')


/* Special endian dependant values
 * The suffic N means Native
 * The suffix I means Inverted (ie non native) */
#ifdef WORDS_BIGENDIAN
#   define VLC_CODEC_S16N VLC_CODEC_S16B
#   define VLC_CODEC_U16N VLC_CODEC_U16B
#   define VLC_CODEC_S24N VLC_CODEC_S24B
#   define VLC_CODEC_S32N VLC_CODEC_S32B
#   define VLC_CODEC_FL32 VLC_CODEC_F32B
#   define VLC_CODEC_FL64 VLC_CODEC_F64B

#   define VLC_CODEC_S16I VLC_CODEC_S16L
#   define VLC_CODEC_U16I VLC_CODEC_U16L
#   define VLC_CODEC_S24I VLC_CODEC_S24L
#   define VLC_CODEC_S32I VLC_CODEC_S32L
#else
#   define VLC_CODEC_S16N VLC_CODEC_S16L
#   define VLC_CODEC_U16N VLC_CODEC_U16L
#   define VLC_CODEC_S24N VLC_CODEC_S24L
#   define VLC_CODEC_S32N VLC_CODEC_S32L
#   define VLC_CODEC_FL32 VLC_CODEC_F32L
#   define VLC_CODEC_FL64 VLC_CODEC_F64L

#   define VLC_CODEC_S16I VLC_CODEC_S16B
#   define VLC_CODEC_U16I VLC_CODEC_U16B
#   define VLC_CODEC_S24I VLC_CODEC_S24B
#   define VLC_CODEC_S32I VLC_CODEC_S32B
#endif

/* Non official codecs, used to force a profile in an encoder */
/* MPEG-1 video */
#define VLC_CODEC_MP1V      VLC_FOURCC('m','p','1','v')
/* MPEG-2 video */
#define VLC_CODEC_MP2V      VLC_FOURCC('m','p','2','v')
/* MPEG-I/II layer 3 audio */
#define VLC_CODEC_MP3       VLC_FOURCC('m','p','3',' ')

/**
 * It returns the codec associated to a fourcc within a ES category.
 *
 * If not found, it will return the given fourcc.
 * If found, it will allways be one of the VLC_CODEC_ defined above.
 *
 * You may use UNKNOWN_ES for the ES category if you don't have the information.
 */
VLC_EXPORT( vlc_fourcc_t, vlc_fourcc_GetCodec, ( int i_cat, vlc_fourcc_t i_fourcc ) );

/**
 * It returns the codec associated to a fourcc store in a zero terminated
 * string.
 *
 * If the string is NULL or does not have exactly 4 charateres, it will
 * return 0, otherwise it behaves like vlc_fourcc_GetCodec.
 *
 * Provided for convenience.
 */
VLC_EXPORT( vlc_fourcc_t, vlc_fourcc_GetCodecFromString, ( int i_cat, const char * ) );

/**
 * It convert the gives fourcc to an audio codec when possible.
 *
 * The fourcc converted are aflt, araw/pcm , twos, sowt. When an incompatible i_bits
 * is detected, 0 is returned.
 * The other fourcc goes through vlc_fourcc_GetCodec and i_bits is not checked.
 */
VLC_EXPORT( vlc_fourcc_t, vlc_fourcc_GetCodecAudio, ( vlc_fourcc_t i_fourcc, int i_bits ) );

/**
 * It returns the description of the given fourcc or NULL if not found.
 *
 * You may use UNKNOWN_ES for the ES category if you don't have the information.
 */
VLC_EXPORT( const char *, vlc_fourcc_GetDescription, ( int i_cat, vlc_fourcc_t i_fourcc ) );

/**
 * It returns a list (terminated with the value 0) of YUV fourccs in
 * decreasing priority order for the given chroma.
 *
 * It will always return a non NULL pointer that must not be freed.
 */
VLC_EXPORT( const vlc_fourcc_t *, vlc_fourcc_GetYUVFallback, ( vlc_fourcc_t ) );

/**
 * It returns a list (terminated with the value 0) of RGB fourccs in
 * decreasing priority order for the given chroma.
 *
 * It will always return a non NULL pointer that must not be freed.
 */
VLC_EXPORT( const vlc_fourcc_t *, vlc_fourcc_GetRGBFallback, ( vlc_fourcc_t ) );

/**
 * It returns true if the given fourcc is YUV and false otherwise.
 */
VLC_EXPORT( bool, vlc_fourcc_IsYUV, ( vlc_fourcc_t ) );

/**
 * It returns true if the two fourccs are equivalent if their U&V planes are
 * swapped.
 */
VLC_EXPORT( bool, vlc_fourcc_AreUVPlanesSwapped, (vlc_fourcc_t , vlc_fourcc_t ) );

/**
 * Chroma related information.
 */
typedef struct {
    unsigned plane_count;
    struct {
        struct {
            unsigned num;
            unsigned den;
        } w;
        struct {
            unsigned num;
            unsigned den;
        } h;
    } p[4];
    unsigned pixel_size;
} vlc_chroma_description_t;

/**
 * It returns a vlc_chroma_description_t describing the request fourcc or NULL
 * if not found.
 */
VLC_EXPORT( const vlc_chroma_description_t *, vlc_fourcc_GetChromaDescription, ( vlc_fourcc_t fourcc ) );

#endif /* _VLC_FOURCC_H */

