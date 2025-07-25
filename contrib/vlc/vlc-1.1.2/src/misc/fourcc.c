/*****************************************************************************
 * fourcc.c: fourcc helpers functions
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 * $Id: 2cecbd95d2e0738e321a8598977fa22808c6c618 $
 *
 * Authors: Laurent Aimar <fenrir@videolan.org>
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

#include <vlc_fourcc.h>
#include <assert.h>

typedef struct
{
    char p_class[4];
    char p_fourcc[4];
    const char *psz_description;
} entry_t;

#define NULL4 "\x00\x00\x00\x00"

/* XXX You don't want to see the preprocessor generated code ;) */
#ifdef WORDS_BIGENDIAN
#   define FCC2STR(f) { ((f)>>24)&0xff, ((f)>>16)&0xff, ((f)>>8)&0xff, ((f)>>0)&0xff }
#else
#   define FCC2STR(f) { ((f)>>0)&0xff, ((f)>>8)&0xff, ((f)>>16)&0xff, ((f)>>24)&0xff }
#endif
/* Begin a new class */
#define B(a, c) { .p_class = FCC2STR(a), .p_fourcc = FCC2STR(a), .psz_description = c }
/* Create a sub-class entry with description */
#define E(b, c) { .p_class = NULL4, .p_fourcc = b, .psz_description = c }
/* Create a sub-class entry without description (alias) */
#define A(b) E(b, NULL4)


/* */
static const entry_t p_list_video[] = {

    B(VLC_CODEC_MPGV, "MPEG-1/2 Video"),
        A("mpgv"),
        A("mp1v"),
        A("mpeg"),
        A("mpg1"),
        A("mp2v"),
        A("MPEG"),
        A("mpg2"),
        A("MPG2"),

        E("PIM1", "Pinnacle DC1000 (MPEG-1 Video)"),

        E("hdv1", "HDV 720p30 (MPEG-2 Video)"),
        E("hdv2", "Sony HDV (MPEG-2 Video)"),
        E("hdv3", "FCP HDV (MPEG-2 Video)"),
        E("hdv5", "HDV 720p25 (MPEG-2 Video)"),
        E("hdv6", "HDV 1080p24 (MPEG-2 Video)"),
        E("hdv7", "HDV 1080p25 (MPEG-2 Video)"),
        E("hdv8", "HDV 1080p30 (MPEG-2 Video)"),

        E("mx5n", "MPEG2 IMX NTSC 525/60 50mb/s (FCP)"),
        E("mx5p", "MPEG2 IMX PAL 625/60 50mb/s (FCP)"),
        E("mx4n", "MPEG2 IMX NTSC 525/60 40mb/s (FCP)"),
        E("mx4p", "MPEG2 IMX PAL 625/50 40mb/s (FCP)"),
        E("mx3n", "MPEG2 IMX NTSC 525/60 30mb/s (FCP)"),
        E("mx3p", "MPEG2 IMX NTSC 625/50 30mb/s (FCP)"),

        E("xdv1", "XDCAM HD"),
        E("xdv2", "XDCAM HD 1080i60 35mb/s"),
        E("xdv3", "XDCAM HD 1080i50 35mb/s"),
        E("xdv4", "XDCAM HD"),
        E("xdv5", "XDCAM HD"),
        E("xdv6", "XDCAM HD 1080p24 35mb/s"),
        E("xdv7", "XDCAM HD 1080p25 35mb/s"),
        E("xdv8", "XDCAM HD 1080p30 35mb/s"),
        E("xdv9", "XDCAM HD"),

        E("xdva", "XDCAM"),
        E("xdvb", "XDCAM"),
        E("xdvc", "XDCAM"),
        E("xdvd", "XDCAM"),
        E("xdve", "XDCAM"),
        E("xdvf", "XDCAM"),

        E("xd5a", "XDCAM"),
        E("xd5b", "XDCAM"),
        E("xd5c", "XDCAM"),
        E("xd5d", "XDCAM"),
        E("xd5e", "XDCAM"),
        E("xd5f", "XDCAM"),
        E("xd59", "XDCAM"),

        E("AVmp", "AVID IMX PAL"),
        E("MMES", "Matrox MPEG-2"),
        E("mmes", "Matrox MPEG-2"),
        E("PIM2", "Pinnacle MPEG-2"),
        E("LMP2", "Lead MPEG-2"),

        E("VCR2", "ATI VCR-2"),

    B(VLC_CODEC_MP4V, "MPEG-4 Video"),
        A("mp4v"),
        A("DIVX"),
        A("divx"),
        A("MP4S"),
        A("mp4s"),
        A("M4S2"),
        A("m4s2"),
        A("MP4V"),
        A("\x04\x00\x00\x00"),
        A("m4cc"),
        A("M4CC"),
        A("FMP4"),
        A("fmp4"),
        A("DCOD"),
        A("MVXM"),
        A("PM4V"),
        A("M4T3"),
        A("GEOX"),
        A("GEOV"),
        A("DMK2"),
        A("WV1F"),
        A("DIGI"),
        A("INMC"),
        A("SN40"),
        A("EPHV"),
        /* XVID flavours */
        E("xvid", "Xvid MPEG-4 Video"),
        E("XVID", "Xvid MPEG-4 Video"),
        E("XviD", "Xvid MPEG-4 Video"),
        E("XVIX", "Xvid MPEG-4 Video"),
        E("xvix", "Xvid MPEG-4 Video"),
        /* DX50 */
        E("DX50", "DivX MPEG-4 Video"),
        E("dx50", "DivX MPEG-4 Video"),
        E("BLZ0", "Blizzard MPEG-4 Video"),
        E("DXGM", "Electronic Arts Game MPEG-4 Video"),
        /* 3ivx delta 4 */
        E("3IV2", "3ivx MPEG-4 Video"),
        E("3iv2", "3ivx MPEG-4 Video"),
        /* Various */
        E("UMP4", "UB MPEG-4 Video"),
        E("SEDG", "Samsung MPEG-4 Video"),
        E("RMP4", "REALmagic MPEG-4 Video"),
        E("HDX4", "Jomigo HDX4 (MPEG-4 Video)"),
        E("hdx4", "Jomigo HDX4 (MPEG-4 Video)"),
        E("SMP4", "Samsung SMP4 (MPEG-4 Video)"),
        E("smp4", "Samsung SMP4 (MPEG-4 Video)"),
        E("fvfw", "FFmpeg MPEG-4"),
        E("FVFW", "FFmpeg MPEG-4"),
        E("FFDS", "FFDShow MPEG-4"),
        E("VIDM", "vidm 4.01 codec"),
        /* 3ivx delta 3.5 Unsupported
         * putting it here gives extreme distorted images */
        //E("3IV1", "3ivx delta 3.5 MPEG-4 Video"),
        //E("3iv1", "3ivx delta 3.5 MPEG-4 Video"),

    /* MSMPEG4 v1 */
    B(VLC_CODEC_DIV1, "MS MPEG-4 Video v1"),
        A("DIV1"),
        A("div1"),
        A("MPG4"),
        A("mpg4"),
        A("mp41"),

    /* MSMPEG4 v2 */
    B(VLC_CODEC_DIV2, "MS MPEG-4 Video v2"),
        A("DIV2"),
        A("div2"),
        A("MP42"),
        A("mp42"),

    /* MSMPEG4 v3 / M$ mpeg4 v3 */
    B(VLC_CODEC_DIV3, "MS MPEG-4 Video v3"),
        A("DIV3"),
        A("MPG3"),
        A("mpg3"),
        A("div3"),
        A("MP43"),
        A("mp43"),
        /* DivX 3.20 */
        A("DIV4"),
        A("div4"),
        A("DIV5"),
        A("div5"),
        A("DIV6"),
        A("div6"),
        E("divf", "DivX 4.12"),
        E("DIVF", "DivX 4.12"),
        /* Cool Codec */
        A("COL1"),
        A("col1"),
        A("COL0"),
        A("col0"),
        /* AngelPotion stuff */
        A("AP41"),
        /* 3ivx doctered divx files */
        A("3IVD"),
        A("3ivd"),
        /* who knows? */
        A("3VID"),
        A("3vid"),
        A("DVX3"),

    /* Sorenson v1 */
    B(VLC_CODEC_SVQ1, "SVQ-1 (Sorenson Video v1)"),
        A("SVQ1"),
        A("svq1"),
        A("svqi"),

    /* Sorenson v3 */
    B(VLC_CODEC_SVQ3, "SVQ-3 (Sorenson Video v3)"),
        A("SVQ3"),

    /* h264 */
    B(VLC_CODEC_H264, "H264 - MPEG-4 AVC (part 10)"),
        A("H264"),
        A("h264"),
        A("x264"),
        A("X264"),
        /* avc1: special case h264 */
        A("avc1"),
        A("AVC1"),
        E("VSSH", "Vanguard VSS H264"),
        E("VSSW", "Vanguard VSS H264"),
        E("vssh", "Vanguard VSS H264"),
        E("DAVC", "Dicas MPEGable H.264/MPEG-4 AVC"),
        E("davc", "Dicas MPEGable H.264/MPEG-4 AVC"),

    /* H263 and H263i */
    /* H263(+) is also known as Real Video 1.0 */

    /* H263 */
    B(VLC_CODEC_H263, "H263"),
        A("H263"),
        A("h263"),
        A("VX1K"),
        A("s263"),
        A("S263"),
        A("U263"),
        A("u263"),
        E("D263", "DEC H263"),
        E("L263", "LEAD H263"),
        E("M263", "Microsoft H263"),
        E("X263", "Xirlink H263"),
        /* Zygo (partial) */
        E("ZyGo", "ITU H263+"),

    /* H263i */
    B(VLC_CODEC_H263I, "I263.I"),
        A("I263"),
        A("i263"),

    /* H263P */
    B(VLC_CODEC_H263P, "ITU H263+"),
        E("ILVR", "ITU H263+"),
        E("viv1", "H263+"),
        E("vivO", "H263+"),
        E("viv2", "H263+"),
        E("U263", "UB H263+"),

    /* Flash (H263) variant */
    B(VLC_CODEC_FLV1, "Flash Video"),
        A("FLV1"),
        A("flv "),

    /* H261 */
    B(VLC_CODEC_H261, "H.261"),
        A("H261"),
        A("h261"),

    B(VLC_CODEC_FLIC, "Flic Video"),
        A("FLIC"),

    /* MJPEG */
    B(VLC_CODEC_MJPG, "Motion JPEG Video"),
        A("MJPG"),
        A("mjpg"),
        A("mjpa"),
        A("jpeg"),
        A("JPEG"),
        A("JFIF"),
        A("JPGL"),
        A("AVDJ"),
        A("MMJP"),
        A("QIVG"),
        /* AVID MJPEG */
        E("AVRn", "Avid Motion JPEG"),
        E("AVDJ", "Avid Motion JPEG"),
        E("ADJV", "Avid Motion JPEG"),
        E("dmb1", "Motion JPEG OpenDML Video"),
        E("ijpg", "Intergraph JPEG Video"),
        E("IJPG", "Intergraph JPEG Video"),
        E("ACDV", "ACD Systems Digital"),
        E("SLMJ", "SL M-JPEG"),

    B(VLC_CODEC_MJPGB, "Motion JPEG B Video"),
        A("mjpb"),

    B(VLC_CODEC_LJPG, "Lead Motion JPEG Video"),
        A("LJPG"),

    // ? from avcodec/fourcc.c but makes not sense.
    //{ VLC_FOURCC( 'L','J','P','G' ), CODEC_ID_MJPEG,       VIDEO_ES, "Lead Motion JPEG Video" },

    /* SP5x */
    B(VLC_CODEC_SP5X, "Sunplus Motion JPEG Video"),
        A("SP5X"),
        A("SP53"),
        A("SP54"),
        A("SP55"),
        A("SP56"),
        A("SP57"),
        A("SP58"),

    /* DV */
    B(VLC_CODEC_DV, "DV Video"),
        A("dv  "),
        A("dvsl"),
        A("DVSD"),
        A("dvsd"),
        A("DVCS"),
        A("dvcs"),
        A("dvhd"),
        A("dvhp"),
        A("dvhq"),
        A("dvh3"),
        A("dvh5"),
        A("dvh6"),
        A("dv1n"),
        A("dv1p"),
        A("dvc "),
        A("dv25"),
        A("dvh1"),
        A("dvs1"),
        E("dvcp", "DV Video PAL"),
        E("dvp ", "DV Video Pro"),
        E("dvpp", "DV Video Pro PAL"),
        E("dv50", "DV Video C Pro 50"),
        E("dv5p", "DV Video C Pro 50 PAL"),
        E("dv5n", "DV Video C Pro 50 NTSC"),
        E("AVdv", "AVID DV"),
        E("AVd1", "AVID DV"),
        E("CDVC", "Canopus DV Video"),
        E("cdvc", "Canopus DV Video"),
        E("CDVH", "Canopus DV Video"),
        E("cdvh", "Canopus DV Video"),

    /* Windows Media Video */
    B(VLC_CODEC_WMV1, "Windows Media Video 7"),
        A("WMV1"),
        A("wmv1"),

    B(VLC_CODEC_WMV2, "Windows Media Video 8"),
        A("WMV2"),
        A("wmv2"),

    B(VLC_CODEC_WMV3, "Windows Media Video 9"),
        A("WMV3"),
        A("wmv3"),

    /* WMVA is the VC-1 codec before the standardization proces,
     * it is not bitstream compatible and deprecated  */
    B(VLC_CODEC_WMVA, "Windows Media Video Advanced Profile"),
        A("WMVA"),
        A("wmva"),
        A("WVP2"),
        A("wvp2"),

    B(VLC_CODEC_VC1, "Windows Media Video VC1"),
        A("WVC1"),
        A("wvc1"),
        A("vc-1"),
        A("VC-1"),

    /* Microsoft Video 1 */
    B(VLC_CODEC_MSVIDEO1, "Microsoft Video 1"),
        A("MSVC"),
        A("msvc"),
        A("CRAM"),
        A("cram"),
        A("WHAM"),
        A("wham"),

    /* Microsoft RLE */
    B(VLC_CODEC_MSRLE, "Microsoft RLE Video"),
        A("mrle"),
        A("WRLE"),
        A("\x01\x00\x00\x00"),
        A("\x02\x00\x00\x00"),

    /* Indeo Video Codecs (Quality of this decoder on ppc is not good) */
    B(VLC_CODEC_INDEO3, "Indeo Video v3"),
        A("IV31"),
        A("iv31"),
        A("IV32"),
        A("iv32"),

    /* Huff YUV */
    B(VLC_CODEC_HUFFYUV, "Huff YUV Video"),
        A("HFYU"),

    B(VLC_CODEC_FFVHUFF, "Huff YUV Video"),
        A("FFVH"),

    /* On2 VP3 Video Codecs */
    B(VLC_CODEC_VP3, "On2's VP3 Video"),
        A("VP3 "),
        A("VP30"),
        A("vp30"),
        A("VP31"),
        A("vp31"),

    /* On2  VP5, VP6 codecs */
    B(VLC_CODEC_VP5, "On2's VP5 Video"),
        A("VP5 "),
        A("VP50"),

    B(VLC_CODEC_VP6, "On2's VP6.2 Video"),
        A("VP62"),
        A("vp62"),
        E("VP60", "On2's VP6.0 Video"),
        E("VP61", "On2's VP6.1 Video"),

    B(VLC_CODEC_VP6F, "On2's VP6.2 Video (Flash)"),
        A("VP6F"),
        A("FLV4"),

    B(VLC_CODEC_VP6A, "On2's VP6 A Video"),
        A("VP6A"),

    B(VLC_CODEC_VP8, "Google/On2's VP8 Video"),
        A("VP80"),


    /* Xiph.org theora */
    B(VLC_CODEC_THEORA, "Xiph.org's Theora Video"),
        A("theo"),
        A("Thra"),

    /* Xiph.org tarkin */
    B(VLC_CODEC_TARKIN, "Xiph.org's Tarkin Video"),
        A("tark"),

    /* Asus Video (Another thing that doesn't work on PPC) */
    B(VLC_CODEC_ASV1, "Asus V1 Video"),
        A("ASV1"),
    B(VLC_CODEC_ASV2, "Asus V2 Video"),
        A("ASV2"),

    /* FFMPEG Video 1 (lossless codec) */
    B(VLC_CODEC_FFV1, "FFMpeg Video 1"),
        A("FFV1"),

    /* ATI VCR1 */
    B(VLC_CODEC_VCR1, "ATI VCR1 Video"),
        A("VCR1"),

    /* Cirrus Logic AccuPak */
    B(VLC_CODEC_CLJR, "Creative Logic AccuPak"),
        A("CLJR"),

    /* Real Video */
    B(VLC_CODEC_RV10, "Real Video 1.0"),
        A("RV10"),
        A("rv10"),

    B(VLC_CODEC_RV13, "Real Video 1.3"),
        A("RV13"),
        A("rv13"),

    B(VLC_CODEC_RV20, "Real Video 2.0"),
        A("RV20"),
        A("rv20"),

    B(VLC_CODEC_RV30, "Real Video 3.0"),
        A("RV30"),
        A("rv30"),

    B(VLC_CODEC_RV40, "Real Video 4.0"),
        A("RV40"),
        A("rv40"),

    /* Apple Video */
    B(VLC_CODEC_RPZA, "Apple Video"),
        A("rpza"),
        A("azpr"),
        A("RPZA"),
        A("AZPR"),

    B(VLC_CODEC_SMC, "Apple graphics"),
        A("smc "),

    B(VLC_CODEC_CINEPAK, "Cinepak Video"),
        A("CVID"),
        A("cvid"),

    /* Screen Capture Video Codecs */
    B(VLC_CODEC_TSCC, "TechSmith Camtasia Screen Capture"),
        A("TSCC"),
        A("tscc"),

    B(VLC_CODEC_CSCD, "CamStudio Screen Codec"),
        A("CSCD"),
        A("cscd"),

    B(VLC_CODEC_ZMBV, "DosBox Capture Codec"),
        A("ZMBV"),

    B(VLC_CODEC_VMNC, "VMware Video"),
        A("VMnc"),
    B(VLC_CODEC_FRAPS, "FRAPS: Realtime Video Capture"),
        A("FPS1"),
        A("fps1"),

    /* Duck TrueMotion */
    B(VLC_CODEC_TRUEMOTION1, "Duck TrueMotion v1 Video"),
        A("DUCK"),
        A("PVEZ"),
    B(VLC_CODEC_TRUEMOTION2, "Duck TrueMotion v2.0 Video"),
        A("TM20"),

    /* FFMPEG's SNOW wavelet codec */
    B(VLC_CODEC_SNOW, "FFMpeg SNOW wavelet Video"),
        A("SNOW"),
        A("snow"),

    B(VLC_CODEC_QTRLE, "Apple QuickTime RLE Video"),
        A("rle "),

    B(VLC_CODEC_QDRAW, "Apple QuickDraw Video"),
        A("qdrw"),

    B(VLC_CODEC_QPEG, "QPEG Video"),
        A("QPEG"),
        A("Q1.0"),
        A("Q1.1"),

    B(VLC_CODEC_ULTI, "IBM Ultimotion Video"),
        A("ULTI"),

    B(VLC_CODEC_VIXL, "Miro/Pinnacle VideoXL Video"),
        A("VIXL"),
        A("XIXL"),
        E("PIXL", "Pinnacle VideoXL Video"),

    B(VLC_CODEC_LOCO, "LOCO Video"),
        A("LOCO"),

    B(VLC_CODEC_WNV1, "Winnov WNV1 Video"),
        A("WNV1"),

    B(VLC_CODEC_AASC, "Autodesc RLE Video"),
        A("AASC"),

    B(VLC_CODEC_INDEO2, "Indeo Video v2"),
        A("IV20"),
        A("RT21"),

        /* Flash Screen Video */
    B(VLC_CODEC_FLASHSV, "Flash Screen Video"),
        A("FSV1"),
    B(VLC_CODEC_KMVC, "Karl Morton's Video Codec (Worms)"),
        A("KMVC"),

    B(VLC_CODEC_NUV, "Nuppel Video"),
        A("RJPG"),
        A("NUV1"),

    /* CODEC_ID_SMACKVIDEO */
    B(VLC_CODEC_SMACKVIDEO, "Smacker Video"),
        A("SMK2"),
        A("SMK4"),

    /* Chinese AVS - Untested */
    B(VLC_CODEC_CAVS, "Chinese AVS"),
        A("CAVS"),
        A("AVs2"),
        A("avs2"),

    B(VLC_CODEC_AMV, "AMV"),

    /* */
    B(VLC_CODEC_DNXHD, "DNxHD"),
        A("AVdn"),
    B(VLC_CODEC_8BPS, "8BPS"),
        A("8BPS"),
    B(VLC_CODEC_MIMIC, "Mimic"),
        A("ML2O"),

    B(VLC_CODEC_CDG, "CD-G Video"),
        A("CDG "),

    B(VLC_CODEC_FRWU, "Forward Uncompressed" ),
        A("FRWU"),

    B(VLC_CODEC_INDEO5, "Indeo Video v5"),
        A("IV50"),
        A("iv50"),


    /* */
    B(VLC_CODEC_YV12, "Planar 4:2:0 YVU"),
        A("YV12"),
        A("yv12"),
    B(VLC_CODEC_YV9,  "Planar 4:1:0 YVU"),
        A("YVU9"),
    B(VLC_CODEC_I410, "Planar 4:1:0 YUV"),
        A("I410"),
    B(VLC_CODEC_I411, "Planar 4:1:1 YUV"),
        A("I411"),
    B(VLC_CODEC_I420, "Planar 4:2:0 YUV"),
        A("I420"),
        A("IYUV"),
    B(VLC_CODEC_I422, "Planar 4:2:2 YUV"),
        A("I422"),
    B(VLC_CODEC_I444, "Planar 4:4:0 YUV"),
        A("I440"),
    B(VLC_CODEC_I444, "Planar 4:4:4 YUV"),
        A("I444"),

    B(VLC_CODEC_J420, "Planar 4:2:0 YUV full scale"),
        A("J420"),
    B(VLC_CODEC_J422, "Planar 4:2:2 YUV full scale"),
        A("J422"),
    B(VLC_CODEC_J440, "Planar 4:4:0 YUV full scale"),
        A("J440"),
    B(VLC_CODEC_J444, "Planar 4:4:4 YUV full scale"),
        A("J444"),

    B(VLC_CODEC_YUVP, "Palettized YUV with palette element Y:U:V:A"),
        A("YUVP"),

    B(VLC_CODEC_YUVA, "Planar YUV 4:4:4 Y:U:V:A"),
        A("YUVA"),

    B(VLC_CODEC_RGBP, "Palettized RGB with palette element R:G:B"),
        A("RGBP"),

    B(VLC_CODEC_RGB8, "8 bits RGB"),
        A("RGB2"),
    B(VLC_CODEC_RGB15, "15 bits RGB"),
        A("RV15"),
    B(VLC_CODEC_RGB16, "16 bits RGB"),
        A("RV16"),
    B(VLC_CODEC_RGB24, "24 bits RGB"),
        A("RV24"),
    B(VLC_CODEC_RGB32, "32 bits RGB"),
        A("RV32"),
    B(VLC_CODEC_RGBA, "32 bits RGBA"),
        A("RGBA"),

    B(VLC_CODEC_GREY, "8 bits greyscale"),
        A("GREY"),
        A("Y800"),
        A("Y8  "),

    B(VLC_CODEC_UYVY, "Packed YUV 4:2:2, U:Y:V:Y"),
        A("UYVY"),
        A("UYNV"),
        A("UYNY"),
        A("Y422"),
        A("HDYC"),
        A("AVUI"),
        A("uyv1"),
        A("2vuy"),
        A("2Vuy"),
        A("2Vu1"),
    B(VLC_CODEC_VYUY, "Packed YUV 4:2:2, V:Y:U:Y"),
        A("VYUY"),
    B(VLC_CODEC_YUYV, "Packed YUV 4:2:2, Y:U:Y:V"),
        A("YUY2"),
        A("YUYV"),
        A("YUNV"),
        A("V422"),
    B(VLC_CODEC_YVYU, "Packed YUV 4:2:2, Y:V:Y:U"),
        A("YVYU"),

    B(VLC_CODEC_Y211, "Packed YUV 2:1:1, Y:U:Y:V "),
        A("Y211"),
    B(VLC_CODEC_CYUV, "Creative Packed YUV 4:2:2, U:Y:V:Y, reverted"),
        A("cyuv"),
        A("CYUV"),

    B(VLC_CODEC_V210, "10-bit 4:2:2 Component YCbCr"),
        A("v210"),

    B(VLC_CODEC_NV12, "Planar  Y, Packet UV (420)"),
        A("NV12"),

    /* Videogames Codecs */

    /* Interplay MVE */
    B(VLC_CODEC_INTERPLAY, "Interplay MVE Video"),
        A("imve"),
        A("INPV"),

    /* Id Quake II CIN */
    B(VLC_CODEC_IDCIN, "Id Quake II CIN Video"),
        A("IDCI"),

    /* 4X Technologies */
    B(VLC_CODEC_4XM, "4X Technologies Video"),
        A("4XMV"),
        A("4xmv"),

    /* Id RoQ */
    B(VLC_CODEC_ROQ, "Id RoQ Video"),
        A("RoQv"),

    /* Sony Playstation MDEC */
    B(VLC_CODEC_MDEC, "PSX MDEC Video"),
        A("MDEC"),

    /* Sierra VMD */
    B(VLC_CODEC_VMDVIDEO, "Sierra VMD Video"),
        A("VMDV"),
        A("vmdv"),

    B(VLC_CODEC_DIRAC, "Dirac" ),
        A("drac"),

    /* Image */
    B(VLC_CODEC_PNG, "PNG Image"),
        A("png "),

    B(VLC_CODEC_PPM, "PPM Image"),
        A("ppm "),

    B(VLC_CODEC_PGM, "PGM Image"),
        A("pgm "),

    B(VLC_CODEC_PGMYUV, "PGM YUV Image"),
        A("pgmy"),

    B(VLC_CODEC_PAM, "PAM Image"),
        A("pam "),

    B(VLC_CODEC_JPEGLS, "Lossless JPEG"),
        A("MJLS"),

    B(VLC_CODEC_JPEG, "JPEG"),
        A("jpeg"),
        A("JPEG"),

    B(VLC_CODEC_BMP, "BMP Image"),
        A("bmp "),

    B(VLC_CODEC_TIFF, "TIFF Image"),
        A("tiff"),

    B(VLC_CODEC_GIF, "GIF Image"),
        A("gif "),


    B(VLC_CODEC_TARGA, "Truevision Targa Image"),
        A("tga "),
        A("mtga"),
        A("MTGA"),

    B(VLC_CODEC_SGI, "SGI Image"),
        A("sgi "),

    B(VLC_CODEC_PNM, "Portable Anymap Image"),
        A("pnm "),

    B(VLC_CODEC_PCX, "Personal Computer Exchange Image"),
        A("pcx "),

    B(0, "")
};
static const entry_t p_list_audio[] = {

    /* Windows Media Audio 1 */
    B(VLC_CODEC_WMA1, "Windows Media Audio 1"),
        A("WMA1"),
        A("wma1"),

    /* Windows Media Audio 2 */
    B(VLC_CODEC_WMA2, "Windows Media Audio 2"),
        A("WMA2"),
        A("wma2"),
        A("wma "),

    /* Windows Media Audio Professional */
    B(VLC_CODEC_WMAP, "Windows Media Audio Professional"),
        A("WMAP"),
        A("wmap"),

    /* Windows Media Audio Lossless */
    B(VLC_CODEC_WMAL, "Windows Media Audio Lossless"),
        A("WMAL"),
        A("wmal"),

    /* Windows Media Audio Speech */
    B(VLC_CODEC_WMAS, "Windows Media Audio Voice (Speech)"),
        A("WMAS"),
        A("wmas"),

    /* DV Audio */
    B(VLC_CODEC_DVAUDIO, "DV Audio"),
        A("dvau"),
        A("vdva"),
        A("dvca"),
        A("RADV"),

    /* MACE-3 Audio */
    B(VLC_CODEC_MACE3, "MACE-3 Audio"),
        A("MAC3"),

    /* MACE-6 Audio */
    B(VLC_CODEC_MACE6, "MACE-6 Audio"),
        A("MAC6"),

    /* MUSEPACK7 Audio */
    B(VLC_CODEC_MUSEPACK7, "MUSEPACK7 Audio"),
        A("MPC "),

    /* MUSEPACK8 Audio */
    B(VLC_CODEC_MUSEPACK8, "MUSEPACK8 Audio"),
        A("MPCK"),
        A("MPC8"),

    /* RealAudio 1.0 */
    B(VLC_CODEC_RA_144, "RealAudio 1.0"),
        A("14_4"),
        A("lpcJ"),

    /* RealAudio 2.0 */
    B(VLC_CODEC_RA_288, "RealAudio 2.0"),
        A("28_8"),

    B(VLC_CODEC_SIPR, "RealAudio Sipr"),
        A("sipr"),

    /* MPEG Audio layer 1/2/3 */
    B(VLC_CODEC_MPGA, "MPEG Audio layer 1/2/3"),
        A("mpga"),
        A("mp3 "),
        A(".mp3"),
        A("MP3 "),
        A("LAME"),
        A("ms\x00\x50"),
        A("ms\x00\x55"),

    /* A52 Audio (aka AC3) */
    B(VLC_CODEC_A52, "A52 Audio (aka AC3)"),
        A("a52 "),
        A("a52b"),
        A("ac-3"),
        A("ms\x20\x00"),

    B(VLC_CODEC_EAC3, "A/52 B Audio (aka E-AC3)"),
        A("eac3"),

    /* DTS Audio */
    B(VLC_CODEC_DTS, "DTS Audio"),
        A("dts "),
        A("dtsb"),
        A("ms\x20\x01"),

    /* AAC audio */
    B(VLC_CODEC_MP4A, "MPEG AAC Audio"),
        A("mp4a"),
        A("aac "),

    /* ALS audio */
    B(VLC_CODEC_ALS, "MPEG-4 Audio Lossless (ALS)"),
        A("als "),

    /* 4X Technologies */
    B(VLC_CODEC_ADPCM_4XM, "4X Technologies Audio"),
        A("4xma"),

    /* EA ADPCM */
    B(VLC_CODEC_ADPCM_EA, "EA ADPCM Audio"),
        A("ADEA"),

    /* Interplay DPCM */
    B(VLC_CODEC_INTERPLAY_DPCM, "Interplay DPCM Audio"),
        A("idpc"),

    /* Id RoQ */
    B(VLC_CODEC_ROQ_DPCM, "Id RoQ DPCM Audio"),
        A("RoQa"),

    /* DCIN Audio */
    B(VLC_CODEC_DSICINAUDIO, "Delphine CIN Audio"),
        A("DCIA"),

    /* Sony Playstation XA ADPCM */
    B(VLC_CODEC_ADPCM_XA, "PSX XA ADPCM Audio"),
        A("xa  "),

    /* ADX ADPCM */
    B(VLC_CODEC_ADPCM_ADX, "ADX ADPCM Audio"),
        A("adx "),

    /* Westwood ADPCM */
    B(VLC_CODEC_ADPCM_IMA_WS, "Westwood IMA ADPCM audio"),
        A("AIWS"),

    /* MS ADPCM */
    B(VLC_CODEC_ADPCM_MS, "MS ADPCM audio"),
        A("ms\x00\x02"),

    /* Sierra VMD */
    B(VLC_CODEC_VMDAUDIO, "Sierra VMD Audio"),
        A("vmda"),

    /* G.726 ADPCM */
    B(VLC_CODEC_ADPCM_G726, "G.726 ADPCM Audio"),
        A("g726"),

    /* Flash ADPCM */
    B(VLC_CODEC_ADPCM_SWF, "Flash ADPCM Audio"),
        A("SWFa"),

    B(VLC_CODEC_ADPCM_IMA_WAV, "IMA WAV ADPCM Audio"),
        A("ms\x00\x11"),

    B(VLC_CODEC_ADPCM_IMA_AMV, "IMA AMV ADPCM Audio"),
        A("imav"),

    /* AMR */
    B(VLC_CODEC_AMR_NB, "AMR narrow band"),
        A("samr"),

    B(VLC_CODEC_AMR_WB, "AMR wide band"),
        A("sawb"),

    /* FLAC */
    B(VLC_CODEC_FLAC, "FLAC (Free Lossless Audio Codec)"),
        A("flac"),

    /* ALAC */
    B(VLC_CODEC_ALAC, "Apple Lossless Audio Codec"),
        A("alac"),

    /* QDM2 */
    B(VLC_CODEC_QDM2, "QDM2 Audio"),
        A("QDM2"),

    /* COOK */
    B(VLC_CODEC_COOK, "Cook Audio"),
        A("cook"),

    /* TTA: The Lossless True Audio */
    B(VLC_CODEC_TTA, "The Lossless True Audio"),
        A("TTA1"),

    /* Shorten */
    B(VLC_CODEC_SHORTEN, "Shorten Lossless Audio"),
        A("shn "),
        A("shrn"),

    B(VLC_CODEC_WAVPACK, "WavPack"),
        A("WVPK"),
        A("wvpk"),

    B(VLC_CODEC_GSM, "GSM Audio"),
        A("gsm "),

    B(VLC_CODEC_GSM_MS, "Microsoft GSM Audio"),
        A("agsm"),

    B(VLC_CODEC_ATRAC1, "atrac 1"),
        A("atr1"),

    B(VLC_CODEC_ATRAC3, "atrac 3"),
        A("atrc"),
        A("\x70\x02\x00\x00"),

    B(VLC_CODEC_SONIC, "Sonic"),
        A("SONC"),

    B(VLC_CODEC_IMC, "IMC" ),
        A("\x01\x04\x00\x00"),

    B(VLC_CODEC_TRUESPEECH,"TrueSpeech"),
        A("\x22\x00\x00\x00"),

    B(VLC_CODEC_NELLYMOSER, "NellyMoser ASAO"),
        A("NELL"),

    B(VLC_CODEC_APE, "Monkey's Audio"),
        A("APE "),

    B(VLC_CODEC_MLP, "MLP/TrueHD Audio"),
        A("mlp "),

    B(VLC_CODEC_TRUEHD, "TrueHD Audio"),
        A("trhd"),

    B(VLC_CODEC_QCELP, "QCELP Audio"),
        A("Qclp"),

    B(VLC_CODEC_SPEEX, "Speex Audio"),
        A("spx "),
        A("spxr"),

    B(VLC_CODEC_VORBIS, "Vorbis Audio"),
        A("vorb"),

    B(VLC_CODEC_302M, "302M Audio"),
        A("302m"),

    B(VLC_CODEC_DVD_LPCM, "DVD LPCM Audio"),
        A("lpcm"),

    B(VLC_CODEC_DVDA_LPCM, "DVD-Audio LPCM Audio"),
        A("apcm"),

    B(VLC_CODEC_BD_LPCM, "BD LPCM Audio"),
        A("bpcm"),

    B(VLC_CODEC_SDDS, "SDDS Audio"),
        A("sdds"),
        A("sddb"),

    B(VLC_CODEC_MIDI, "MIDI Audio"),
        A("MIDI"),

    /* PCM */
    B(VLC_CODEC_S8, "PCM S8"),
        A("s8  "),

    B(VLC_CODEC_U8, "PCM U8"),
        A("u8  "),

    B(VLC_CODEC_S16L, "PCM S16 LE"),
        A("s16l"),

    B(VLC_CODEC_S16B, "PCM S16 BE"),
        A("s16b"),

    B(VLC_CODEC_U16L, "PCM U16 LE"),
        A("u16l"),

    B(VLC_CODEC_U16B, "PCM U16 BE"),
        A("u16b"),

    B(VLC_CODEC_S24L, "PCM S24 LE"),
        A("s24l"),
        A("42ni"),  /* Quicktime */

    B(VLC_CODEC_S24B, "PCM S24 BE"),
        A("s24b"),
        A("in24"),  /* Quicktime */

    B(VLC_CODEC_U24L, "PCM U24 LE"),
        A("u24l"),

    B(VLC_CODEC_U24B, "PCM U24 BE"),
        A("u24b"),

    B(VLC_CODEC_S32L, "PCM S32 LE"),
        A("s32l"),
        A("23ni"),  /* Quicktime */

    B(VLC_CODEC_S32B, "PCM S32 BE"),
        A("s32b"),
        A("in32"),  /* Quicktime */

    B(VLC_CODEC_U32L, "PCM U32 LE"),
        A("u32l"),

    B(VLC_CODEC_U32B, "PCM U32 BE"),
        A("u32b"),

    B(VLC_CODEC_ALAW, "PCM ALAW"),
        A("alaw"),

    B(VLC_CODEC_MULAW, "PCM MU-LAW"),
        A("mlaw"),
        A("ulaw"),

    B(VLC_CODEC_S24DAUD, "PCM DAUD"),
        A("daud"),

    B(VLC_CODEC_FI32, "32 bits fixed float"),
        A("fi32"),

    B(VLC_CODEC_F32L, "32 bits float LE"),
        A("f32l"),
        A("fl32"),

    B(VLC_CODEC_F32B, "32 bits float BE"),
        A("f32b"),

    B(VLC_CODEC_F64L, "64 bits float LE"),
        A("f64l"),

    B(VLC_CODEC_F64L, "64 bits float BE"),
        A("f64b"),

    B(VLC_CODEC_TWINVQ, "TwinVQ"),
        A("TWIN"),

    B(0, "")
};
static const entry_t p_list_spu[] = {

    B(VLC_CODEC_SPU, "DVD Subtitles"),
        A("spu "),
        A("spub"),

    B(VLC_CODEC_DVBS, "DVB Subtitles"),
        A("dvbs"),

    B(VLC_CODEC_SUBT, "Text subtitles with various tags"),
        A("subt"),

    B(VLC_CODEC_XSUB, "DivX XSUB subtitles"),
        A("XSUB"),
        A("xsub"),
        A("DXSB"),

    B(VLC_CODEC_SSA, "SubStation Alpha subtitles"),
        A("ssa "),

    B(VLC_CODEC_TEXT, "Plain text subtitles"),
        A("TEXT"),

    B(VLC_CODEC_TELETEXT, "Teletext"),
        A("telx"),

    B(VLC_CODEC_KATE, "Kate subtitles"),
        A("kate"),

    B(VLC_CODEC_CMML, "CMML annotations/metadata"),
        A("cmml"),

    B(VLC_CODEC_ITU_T140, "ITU T.140 subtitles"),
        A("t140"),

    B(VLC_CODEC_USF, "USF subtitles"),
        A("usf "),

    B(VLC_CODEC_OGT, "OGT subtitles"),
        A("ogt "),

    B(VLC_CODEC_CVD, "CVD subtitles"),
        A("cvd "),

    B(VLC_CODEC_BD_PG, "BD subtitles"),
        A("bdpg"),

    B(0, "")
};

/* Create a fourcc from a string.
 * XXX it assumes that the string is at least four bytes */
static inline vlc_fourcc_t CreateFourcc( const char *psz_fourcc )
{
    return VLC_FOURCC( psz_fourcc[0], psz_fourcc[1],
                       psz_fourcc[2], psz_fourcc[3] );
}

/* */
static entry_t Lookup( const entry_t p_list[], vlc_fourcc_t i_fourcc )
{
    const char *p_class = NULL;
    const char *psz_description = NULL;

    entry_t e = B(0, "");

    for( int i = 0; ; i++ )
    {
        const entry_t *p = &p_list[i];
        const vlc_fourcc_t i_entry_fourcc = CreateFourcc( p->p_fourcc );
        const vlc_fourcc_t i_entry_class = CreateFourcc( p->p_class );

        if( i_entry_fourcc == 0 )
            break;

        if( i_entry_class != 0 )
        {
            p_class = p->p_class;
            psz_description = p->psz_description;
        }

        if( i_entry_fourcc == i_fourcc )
        {
            assert( p_class != NULL );

            memcpy( e.p_class, p_class, 4 );
            memcpy( e.p_fourcc, p->p_fourcc, 4 );
            e.psz_description = p->psz_description ?
                                p->psz_description : psz_description;
            break;
        }
    }
    return e;
}

/* */
static entry_t Find( int i_cat, vlc_fourcc_t i_fourcc )
{
    entry_t e;

    switch( i_cat )
    {
    case VIDEO_ES:
        return Lookup( p_list_video, i_fourcc );
    case AUDIO_ES:
        return Lookup( p_list_audio, i_fourcc );
    case SPU_ES:
        return Lookup( p_list_spu, i_fourcc );

    default:
        e = Find( VIDEO_ES, i_fourcc );
        if( CreateFourcc( e.p_class ) == 0 )
            e = Find( AUDIO_ES, i_fourcc );
        if( CreateFourcc( e.p_class ) == 0 )
            e = Find( SPU_ES, i_fourcc );
        return e;
    }
}

/* */
vlc_fourcc_t vlc_fourcc_GetCodec( int i_cat, vlc_fourcc_t i_fourcc )
{
    entry_t e = Find( i_cat, i_fourcc );

    if( CreateFourcc( e.p_class ) == 0 )
        return i_fourcc;
    return CreateFourcc( e.p_class );
}

vlc_fourcc_t vlc_fourcc_GetCodecFromString( int i_cat, const char *psz_fourcc )
{
    if( !psz_fourcc || strlen(psz_fourcc) != 4 )
        return 0;
    return vlc_fourcc_GetCodec( i_cat,
                                VLC_FOURCC( psz_fourcc[0], psz_fourcc[1],
                                            psz_fourcc[2], psz_fourcc[3] ) );
}

vlc_fourcc_t vlc_fourcc_GetCodecAudio( vlc_fourcc_t i_fourcc, int i_bits )
{
    const int i_bytes = ( i_bits + 7 ) / 8;

    if( i_fourcc == VLC_FOURCC( 'a', 'f', 'l', 't' ) )
    {
        switch( i_bytes )
        {
        case 4:
            return VLC_CODEC_FL32;
        case 8:
            return VLC_CODEC_FL64;
        default:
            return 0;
        }
    }
    else if( i_fourcc == VLC_FOURCC( 'a', 'r', 'a', 'w' ) ||
             i_fourcc == VLC_FOURCC( 'p', 'c', 'm', ' ' ) )
    {
        switch( i_bytes )
        {
        case 1:
            return VLC_CODEC_U8;
        case 2:
            return VLC_CODEC_S16L;
        case 3:
            return VLC_CODEC_S24L;
            break;
        case 4:
            return VLC_CODEC_S32L;
        default:
            return 0;
        }
    }
    else if( i_fourcc == VLC_FOURCC( 't', 'w', 'o', 's' ) )
    {
        switch( i_bytes )
        {
        case 1:
            return VLC_CODEC_S8;
        case 2:
            return VLC_CODEC_S16B;
        case 3:
            return VLC_CODEC_S24B;
        case 4:
            return VLC_CODEC_S32B;
        default:
            return 0;
        }
    }
    else if( i_fourcc == VLC_FOURCC( 's', 'o', 'w', 't' ) )
    {
        switch( i_bytes )
        {
        case 1:
            return VLC_CODEC_S8;
        case 2:
            return VLC_CODEC_S16L;
        case 3:
            return VLC_CODEC_S24L;
        case 4:
            return VLC_CODEC_S32L;
        default:
            return 0;
        }
    }
    else
    {
        return vlc_fourcc_GetCodec( AUDIO_ES, i_fourcc );
    }
}

/* */
const char *vlc_fourcc_GetDescription( int i_cat, vlc_fourcc_t i_fourcc )
{
    entry_t e = Find( i_cat, i_fourcc );

    return e.psz_description;
}


/* */
#define VLC_CODEC_YUV_PLANAR_410 \
    VLC_CODEC_I410, VLC_CODEC_YV9

#define VLC_CODEC_YUV_PLANAR_420 \
    VLC_CODEC_I420, VLC_CODEC_YV12, VLC_CODEC_J420

#define VLC_CODEC_YUV_PLANAR_422 \
    VLC_CODEC_I422, VLC_CODEC_J422

#define VLC_CODEC_YUV_PLANAR_440 \
    VLC_CODEC_I440, VLC_CODEC_J440

#define VLC_CODEC_YUV_PLANAR_444 \
    VLC_CODEC_I444, VLC_CODEC_J444

#define VLC_CODEC_YUV_PACKED \
    VLC_CODEC_YUYV, VLC_CODEC_YVYU, \
    VLC_CODEC_UYVY, VLC_CODEC_VYUY

#define VLC_CODEC_FALLBACK_420 \
    VLC_CODEC_YUV_PLANAR_422, VLC_CODEC_YUV_PACKED, \
    VLC_CODEC_YUV_PLANAR_444, VLC_CODEC_YUV_PLANAR_440, \
    VLC_CODEC_I411, VLC_CODEC_YUV_PLANAR_410, VLC_CODEC_Y211

static const vlc_fourcc_t p_I420_fallback[] = {
    VLC_CODEC_I420, VLC_CODEC_YV12, VLC_CODEC_J420, VLC_CODEC_FALLBACK_420, 0
};
static const vlc_fourcc_t p_J420_fallback[] = {
    VLC_CODEC_J420, VLC_CODEC_I420, VLC_CODEC_YV12, VLC_CODEC_FALLBACK_420, 0
};
static const vlc_fourcc_t p_YV12_fallback[] = {
    VLC_CODEC_YV12, VLC_CODEC_I420, VLC_CODEC_J420, VLC_CODEC_FALLBACK_420, 0
};

#define VLC_CODEC_FALLBACK_422 \
    VLC_CODEC_YUV_PACKED, VLC_CODEC_YUV_PLANAR_420, \
    VLC_CODEC_YUV_PLANAR_444, VLC_CODEC_YUV_PLANAR_440, \
    VLC_CODEC_I411, VLC_CODEC_YUV_PLANAR_410, VLC_CODEC_Y211

static const vlc_fourcc_t p_I422_fallback[] = {
    VLC_CODEC_I422, VLC_CODEC_J422, VLC_CODEC_FALLBACK_422, 0
};
static const vlc_fourcc_t p_J422_fallback[] = {
    VLC_CODEC_J422, VLC_CODEC_I422, VLC_CODEC_FALLBACK_422, 0
};

#define VLC_CODEC_FALLBACK_444 \
    VLC_CODEC_YUV_PLANAR_422, VLC_CODEC_YUV_PACKED, \
    VLC_CODEC_YUV_PLANAR_420, VLC_CODEC_YUV_PLANAR_440, \
    VLC_CODEC_I411, VLC_CODEC_YUV_PLANAR_410, VLC_CODEC_Y211

static const vlc_fourcc_t p_I444_fallback[] = {
    VLC_CODEC_I444, VLC_CODEC_J444, VLC_CODEC_FALLBACK_444, 0
};
static const vlc_fourcc_t p_J444_fallback[] = {
    VLC_CODEC_J444, VLC_CODEC_I444, VLC_CODEC_FALLBACK_444, 0
};

static const vlc_fourcc_t p_I440_fallback[] = {
    VLC_CODEC_I440,
    VLC_CODEC_YUV_PLANAR_420,
    VLC_CODEC_YUV_PLANAR_422,
    VLC_CODEC_YUV_PLANAR_444,
    VLC_CODEC_YUV_PACKED,
    VLC_CODEC_I411, VLC_CODEC_YUV_PLANAR_410, VLC_CODEC_Y211, 0
};

#define VLC_CODEC_FALLBACK_PACKED \
    VLC_CODEC_YUV_PLANAR_422, VLC_CODEC_YUV_PLANAR_420, \
    VLC_CODEC_YUV_PLANAR_444, VLC_CODEC_YUV_PLANAR_440, \
    VLC_CODEC_I411, VLC_CODEC_YUV_PLANAR_410, VLC_CODEC_Y211

static const vlc_fourcc_t p_YUYV_fallback[] = {
    VLC_CODEC_YUYV,
    VLC_CODEC_YVYU,
    VLC_CODEC_UYVY,
    VLC_CODEC_VYUY,
    VLC_CODEC_FALLBACK_PACKED, 0
};
static const vlc_fourcc_t p_YVYU_fallback[] = {
    VLC_CODEC_YVYU,
    VLC_CODEC_YUYV,
    VLC_CODEC_UYVY,
    VLC_CODEC_VYUY,
    VLC_CODEC_FALLBACK_PACKED, 0
};
static const vlc_fourcc_t p_UYVY_fallback[] = {
    VLC_CODEC_UYVY,
    VLC_CODEC_VYUY,
    VLC_CODEC_YUYV,
    VLC_CODEC_YVYU,
    VLC_CODEC_FALLBACK_PACKED, 0
};
static const vlc_fourcc_t p_VYUY_fallback[] = {
    VLC_CODEC_VYUY,
    VLC_CODEC_UYVY,
    VLC_CODEC_YUYV,
    VLC_CODEC_YVYU,
    VLC_CODEC_FALLBACK_PACKED, 0
};

static const vlc_fourcc_t *pp_YUV_fallback[] = {
    p_YV12_fallback,
    p_I420_fallback,
    p_J420_fallback,
    p_I422_fallback,
    p_J422_fallback,
    p_I444_fallback,
    p_J444_fallback,
    p_I440_fallback,
    p_YUYV_fallback,
    p_YVYU_fallback,
    p_UYVY_fallback,
    p_VYUY_fallback,
    NULL,
};

static const vlc_fourcc_t p_list_YUV[] = {
    VLC_CODEC_YUV_PLANAR_420,
    VLC_CODEC_YUV_PLANAR_422,
    VLC_CODEC_YUV_PLANAR_440,
    VLC_CODEC_YUV_PLANAR_444,
    VLC_CODEC_YUV_PACKED,
    VLC_CODEC_I411, VLC_CODEC_YUV_PLANAR_410, VLC_CODEC_Y211,
    0,
};

/* */
static const vlc_fourcc_t p_RGB32_fallback[] = {
    VLC_CODEC_RGB32,
    VLC_CODEC_RGB24,
    VLC_CODEC_RGB16,
    VLC_CODEC_RGB15,
    VLC_CODEC_RGB8,
    0,
};
static const vlc_fourcc_t p_RGB24_fallback[] = {
    VLC_CODEC_RGB24,
    VLC_CODEC_RGB32,
    VLC_CODEC_RGB16,
    VLC_CODEC_RGB15,
    VLC_CODEC_RGB8,
    0,
};
static const vlc_fourcc_t p_RGB16_fallback[] = {
    VLC_CODEC_RGB16,
    VLC_CODEC_RGB24,
    VLC_CODEC_RGB32,
    VLC_CODEC_RGB15,
    VLC_CODEC_RGB8,
    0,
};
static const vlc_fourcc_t p_RGB15_fallback[] = {
    VLC_CODEC_RGB15,
    VLC_CODEC_RGB16,
    VLC_CODEC_RGB24,
    VLC_CODEC_RGB32,
    VLC_CODEC_RGB8,
    0,
};
static const vlc_fourcc_t p_RGB8_fallback[] = {
    VLC_CODEC_RGB8,
    VLC_CODEC_RGB15,
    VLC_CODEC_RGB16,
    VLC_CODEC_RGB24,
    VLC_CODEC_RGB32,
    0,
};
static const vlc_fourcc_t *pp_RGB_fallback[] = {
    p_RGB32_fallback,
    p_RGB24_fallback,
    p_RGB16_fallback,
    p_RGB15_fallback,
    p_RGB8_fallback,
    NULL,
};


/* */
static const vlc_fourcc_t *GetFallback( vlc_fourcc_t i_fourcc,
                                        const vlc_fourcc_t *pp_fallback[],
                                        const vlc_fourcc_t p_list[] )
{
    for( unsigned i = 0; pp_fallback[i]; i++ )
    {
        if( pp_fallback[i][0] == i_fourcc )
            return pp_fallback[i];
    }
    return p_list;
}

const vlc_fourcc_t *vlc_fourcc_GetYUVFallback( vlc_fourcc_t i_fourcc )
{
    return GetFallback( i_fourcc, pp_YUV_fallback, p_list_YUV );
}
const vlc_fourcc_t *vlc_fourcc_GetRGBFallback( vlc_fourcc_t i_fourcc )
{
    return GetFallback( i_fourcc, pp_RGB_fallback, p_RGB32_fallback );
}

bool vlc_fourcc_AreUVPlanesSwapped( vlc_fourcc_t a, vlc_fourcc_t b )
{
    static const vlc_fourcc_t pp_swapped[][4] = {
        { VLC_CODEC_YV12, VLC_CODEC_I420, VLC_CODEC_J420, 0 },
        { VLC_CODEC_YV9,  VLC_CODEC_I410, 0 },
        { 0 }
    };

    for( int i = 0; pp_swapped[i][0]; i++ )
    {
        if( pp_swapped[i][0] == b )
        {
            vlc_fourcc_t t = a;
            a = b;
            b = t;
        }
        if( pp_swapped[i][0] != a )
            continue;
        for( int j = 1; pp_swapped[i][j]; j++ )
        {
            if( pp_swapped[i][j] == b )
                return true;
        }
    }
    return false;
}

bool vlc_fourcc_IsYUV(vlc_fourcc_t fcc)
{
    for( unsigned i = 0; p_list_YUV[i]; i++ )
    {
        if( p_list_YUV[i] == fcc )
            return true;
    }
    return false;
}

