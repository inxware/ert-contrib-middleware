/* GStreamer encoding bin
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *           (C) 2009 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstencodebin.h"
#include "gstsmartencoder.h"
#include "gststreamsplitter.h"
#include "gststreamcombiner.h"
#include <gst/gst-i18n-plugin.h>

/**
 * SECTION:element-encodebin
 *
 * EncodeBin provides a bin for encoding/muxing various streams according to
 * a specified #GstEncodingProfile.
 *
 * Based on the profile that was set (via the #GstEncodeBin:profile property),
 * EncodeBin will internally select and configure the required elements
 * (encoders, muxers, but also audio and video converters) so that you can
 * provide it raw or pre-encoded streams of data in input and have your
 * encoded/muxed/converted stream in output.
 *
 * <refsect2>
 * <title>Features</title>
 * <itemizedlist>
 * <listitem>
 * Automatic encoder and muxer selection based on elements available on the
 * system.
 * </listitem>
 * <listitem>
 * Conversion of raw audio/video streams (scaling, framerate conversion,
 * colorspace conversion, samplerate conversion) to conform to the profile
 * output format.
 * </listitem>
 * <listitem>
 * Variable number of streams. If the presence property for a stream encoding
 * profile is 0, you can request any number of sink pads for it via the
 * standard request pad gstreamer API or the #GstEncodeBin::request-pad action
 * signal.
 * </listitem>
 * <listitem>
 * Avoid reencoding (passthrough). If the input stream is already encoded and is
 * compatible with what the #GstEncodingProfile expects, then the stream won't
 * be re-encoded but just passed through downstream to the muxer or the output.
 * </listitem>
 * <listitem>
 * Mix pre-encoded and raw streams as input. In addition to the passthrough
 * feature above, you can feed both raw audio/video *AND* already-encoded data
 * to a pad. #GstEncodeBin will take care of passing through the compatible
 * segments and re-encoding the segments of media that need encoding.
 * </listitem>
 * <listitem>
 * Standard behaviour is to use a #GstEncodingContainerProfile to have both
 * encoding and muxing performed. But you can also provide a single stream
 * profile (like #GstEncodingAudioProfile) to only have the encoding done and
 * handle the encoded output yourself.
 * </listitem>
 * <listitem>
 * Audio imperfection corrections. Incoming audio streams can have non perfect
 * timestamps (jitter), like the streams coming from ASF files. #GstEncodeBin
 * will automatically fix those imperfections for you. See
 * #GstEncodeBin:audio-jitter-tolerance for more details.
 * </listitem>
 * <listitem>
 * Variable or Constant video framerate. If your #GstEncodingVideoProfile has
 * the variableframerate property deactivated (default), then the incoming
 * raw video stream will be retimestampped in order to produce a constant
 * framerate.
 * </listitem>
 * <listitem>
 * Cross-boundary re-encoding. When feeding compatible pre-encoded streams that
 * fall on segment boundaries, and for supported formats (right now only H263),
 * the GOP will be decoded/reencoded when needed to produce an encoded output
 * that fits exactly within the request GstSegment.
 * </listitem>
 * <listitem>
 * Missing plugin support. If a #GstElement is missing to encode/mux to the
 * request profile formats, a missing-plugin #GstMessage will be posted on the
 * #GstBus, allowing systems that support the missing-plugin system to offer the
 * user a way to install the missing element.
 * </listitem>
 * </itemizedlist>
 * </refsect2>
 */


/* TODO/FIXME
 *
 * Handling mp3!xing!idv3 and theora!ogg tagsetting scenarios:
 *  Once we have chosen a muxer:
 *   When a new stream is requested:
 *    If muxer is 'Formatter' OR doesn't have a TagSetter interface:
 *      Find a Formatter for the given stream (preferably with TagSetter)
 *       Insert that before muxer
 **/

#define fast_pad_link(a,b) gst_pad_link_full((a),(b),GST_PAD_LINK_CHECK_NOTHING)
#define fast_element_link(a,b) gst_element_link_pads_full((a),"src",(b),"sink",GST_PAD_LINK_CHECK_NOTHING)

/* generic templates */
static GstStaticPadTemplate muxer_src_template =
GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate video_sink_template =
GST_STATIC_PAD_TEMPLATE ("video_%d",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate audio_sink_template =
GST_STATIC_PAD_TEMPLATE ("audio_%d",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);
/* static GstStaticPadTemplate text_sink_template = */
/* GST_STATIC_PAD_TEMPLATE ("text_%d", */
/*     GST_PAD_SINK, */
/*     GST_PAD_REQUEST, */
/*     GST_STATIC_CAPS_ANY); */
static GstStaticPadTemplate private_sink_template =
GST_STATIC_PAD_TEMPLATE ("private_%d",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

struct _GstEncodeBin
{
  GstBin parent;

  /* the profile field is only valid if it could be entirely setup */
  GstEncodingProfile *profile;

  GList *streams;               /* List of StreamGroup, not sorted */

  GstElement *muxer;
  /* Ghostpad with changing target */
  GstPad *srcpad;

  /* TRUE if in PAUSED/PLAYING */
  gboolean active;

  /* available muxers, encoders and parsers */
  GList *muxers;
  GList *encoders;
  GList *parsers;

  /* Increasing counter for unique pad name */
  guint last_pad_id;

  /* Cached caps for identification */
  GstCaps *raw_video_caps;
  GstCaps *raw_audio_caps;
  /* GstCaps *raw_text_caps; */

  guint queue_buffers_max;
  guint queue_bytes_max;
  guint64 queue_time_max;

  guint64 tolerance;
  gboolean avoid_reencoding;
};

struct _GstEncodeBinClass
{
  GstBinClass parent;

  /* Action Signals */
  GstPad *(*request_pad) (GstEncodeBin * encodebin, GstCaps * caps);
};

typedef struct _StreamGroup StreamGroup;

struct _StreamGroup
{
  GstEncodeBin *ebin;
  GstEncodingProfile *profile;
  GstPad *ghostpad;             /* Sink ghostpad */
  GstElement *inqueue;          /* Queue just after the ghostpad */
  GstElement *splitter;
  GList *converters;            /* List of conversion GstElement */
  GstElement *capsfilter;       /* profile->restriction (if non-NULL/ANY) */
  GstElement *encoder;          /* Encoder (can be NULL) */
  GstElement *combiner;
  GstElement *parser;
  GstElement *smartencoder;
  GstElement *outfilter;        /* Output capsfilter (streamprofile.format) */
  GstElement *outqueue;         /* Queue just before the muxer */
};

/* Default for queues (same defaults as queue element) */
#define DEFAULT_QUEUE_BUFFERS_MAX  200
#define DEFAULT_QUEUE_BYTES_MAX    10 * 1024 * 1024
#define DEFAULT_QUEUE_TIME_MAX     GST_SECOND
#define DEFAULT_AUDIO_JITTER_TOLERANCE 20 * GST_MSECOND
#define DEFAULT_AVOID_REENCODING   FALSE

#define DEFAULT_RAW_CAPS			\
  "video/x-raw-yuv; "				\
  "video/x-raw-rgb; "				\
  "video/x-raw-gray; "				\
  "audio/x-raw-int; "				\
  "audio/x-raw-float; "				\
  "text/plain; "				\
  "text/x-pango-markup; "			\
  "video/x-dvd-subpicture; "			\
  "subpicture/x-pgs"

/* Properties */
enum
{
  PROP_0,
  PROP_PROFILE,
  PROP_QUEUE_BUFFERS_MAX,
  PROP_QUEUE_BYTES_MAX,
  PROP_QUEUE_TIME_MAX,
  PROP_AUDIO_JITTER_TOLERANCE,
  PROP_AVOID_REENCODING,
  PROP_LAST
};

/* Signals */
enum
{
  SIGNAL_REQUEST_PAD,
  LAST_SIGNAL
};

static guint gst_encode_bin_signals[LAST_SIGNAL] = { 0 };

static GstStaticCaps default_raw_caps = GST_STATIC_CAPS (DEFAULT_RAW_CAPS);

GST_DEBUG_CATEGORY_STATIC (gst_encode_bin_debug);
#define GST_CAT_DEFAULT gst_encode_bin_debug

G_DEFINE_TYPE (GstEncodeBin, gst_encode_bin, GST_TYPE_BIN);

static void gst_encode_bin_dispose (GObject * object);
static void gst_encode_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_encode_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_encode_bin_change_state (GstElement * element,
    GstStateChange transition);

static GstPad *gst_encode_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_encode_bin_release_pad (GstElement * element, GstPad * pad);

static gboolean
gst_encode_bin_set_profile (GstEncodeBin * ebin, GstEncodingProfile * profile);
static void gst_encode_bin_tear_down_profile (GstEncodeBin * ebin);
static gboolean gst_encode_bin_setup_profile (GstEncodeBin * ebin,
    GstEncodingProfile * profile);

static StreamGroup *_create_stream_group (GstEncodeBin * ebin,
    GstEncodingProfile * sprof, const gchar * sinkpadname, GstCaps * sinkcaps);
static void stream_group_remove (GstEncodeBin * ebin, StreamGroup * sgroup);
static GstPad *gst_encode_bin_request_pad_signal (GstEncodeBin * encodebin,
    GstCaps * caps);

static void
gst_encode_bin_class_init (GstEncodeBinClass * klass)
{
  GObjectClass *gobject_klass;
  GstElementClass *gstelement_klass;

  gobject_klass = (GObjectClass *) klass;
  gstelement_klass = (GstElementClass *) klass;

  gobject_klass->dispose = gst_encode_bin_dispose;
  gobject_klass->set_property = gst_encode_bin_set_property;
  gobject_klass->get_property = gst_encode_bin_get_property;

  /* Properties */

  /**
   * GstEncodeBin:profile:
   *
   * The #GstEncodingProfile to use. This property must be set before going
   * to %GST_STATE_PAUSED or higher.
   */
  g_object_class_install_property (gobject_klass, PROP_PROFILE,
      gst_param_spec_mini_object ("profile", "Profile",
          "The GstEncodingProfile to use", GST_TYPE_ENCODING_PROFILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_QUEUE_BUFFERS_MAX,
      g_param_spec_uint ("queue-bytes-max", "Max. size (kB)",
          "Max. amount of data in the queue (bytes, 0=disable)",
          0, G_MAXUINT, DEFAULT_QUEUE_BYTES_MAX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_QUEUE_BYTES_MAX,
      g_param_spec_uint ("queue-buffers-max", "Max. size (buffers)",
          "Max. number of buffers in the queue (0=disable)", 0, G_MAXUINT,
          DEFAULT_QUEUE_BUFFERS_MAX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_QUEUE_TIME_MAX,
      g_param_spec_uint64 ("queue-time-max", "Max. size (ns)",
          "Max. amount of data in the queue (in ns, 0=disable)", 0, G_MAXUINT64,
          DEFAULT_QUEUE_TIME_MAX, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_AUDIO_JITTER_TOLERANCE,
      g_param_spec_uint64 ("audio-jitter-tolerance", "Audio jitter tolerance",
          "Amount of timestamp jitter/imperfection to allow on audio streams before inserting/dropping samples (ns)",
          0, G_MAXUINT64, DEFAULT_AUDIO_JITTER_TOLERANCE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_klass, PROP_AVOID_REENCODING,
      g_param_spec_boolean ("avoid-reencoding", "Avoid re-encoding",
          "Whether to re-encode portions of compatible video streams that lay on segment boundaries",
          DEFAULT_AVOID_REENCODING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* Signals */
  /**
   * GstEncodeBin::request-pad
   * @encodebin: a #GstEncodeBin instance
   * @caps: a #GstCaps
   *
   * Use this method to request an unused sink request #GstPad that can take the
   * provided @caps as input. You must release the pad with
   * gst_element_release_request_pad() when you are done with it.
   *
   * Returns: A compatible #GstPad, or %NULL if no compatible #GstPad could be
   * created or is available.
   */
  gst_encode_bin_signals[SIGNAL_REQUEST_PAD] =
      g_signal_new ("request-pad", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (GstEncodeBinClass,
          request_pad), NULL, NULL, gst_encode_marshal_OBJECT__BOXED,
      GST_TYPE_PAD, 1, GST_TYPE_CAPS);

  klass->request_pad = gst_encode_bin_request_pad_signal;

  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&muxer_src_template));
  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&video_sink_template));
  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&audio_sink_template));
  /* gst_element_class_add_pad_template (gstelement_klass, */
  /*     gst_static_pad_template_get (&text_sink_template)); */
  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&private_sink_template));

  gstelement_klass->change_state =
      GST_DEBUG_FUNCPTR (gst_encode_bin_change_state);
  gstelement_klass->request_new_pad_full =
      GST_DEBUG_FUNCPTR (gst_encode_bin_request_new_pad);
  gstelement_klass->release_pad =
      GST_DEBUG_FUNCPTR (gst_encode_bin_release_pad);

  gst_element_class_set_details_simple (gstelement_klass,
      "Encoder Bin",
      "Generic/Bin/Encoder",
      "Convenience encoding/muxing element",
      "Edward Hervey <edward.hervey@collabora.co.uk>");
}

static void
gst_encode_bin_dispose (GObject * object)
{
  GstEncodeBin *ebin = (GstEncodeBin *) object;

  if (ebin->muxers)
    gst_plugin_feature_list_free (ebin->muxers);

  if (ebin->encoders)
    gst_plugin_feature_list_free (ebin->encoders);

  if (ebin->parsers)
    gst_plugin_feature_list_free (ebin->parsers);

  gst_encode_bin_tear_down_profile (ebin);

  if (ebin->raw_video_caps)
    gst_caps_unref (ebin->raw_video_caps);
  if (ebin->raw_audio_caps)
    gst_caps_unref (ebin->raw_audio_caps);
  /* if (ebin->raw_text_caps) */
  /*   gst_caps_unref (ebin->raw_text_caps); */

  G_OBJECT_CLASS (gst_encode_bin_parent_class)->dispose (object);
}

static void
gst_encode_bin_init (GstEncodeBin * encode_bin)
{
  GstPadTemplate *tmpl;
  GList *formatters;

  encode_bin->muxers =
      gst_element_factory_list_get_elements (GST_ELEMENT_FACTORY_TYPE_MUXER,
      GST_RANK_MARGINAL);
  formatters =
      gst_element_factory_list_get_elements (GST_ELEMENT_FACTORY_TYPE_FORMATTER,
      GST_RANK_SECONDARY);
  encode_bin->muxers = g_list_concat (encode_bin->muxers, formatters);

  encode_bin->encoders =
      gst_element_factory_list_get_elements (GST_ELEMENT_FACTORY_TYPE_ENCODER,
      GST_RANK_MARGINAL);

  encode_bin->parsers =
      gst_element_factory_list_get_elements (GST_ELEMENT_FACTORY_TYPE_PARSER,
      GST_RANK_MARGINAL);

  encode_bin->raw_video_caps =
      gst_caps_from_string ("video/x-raw-yuv;video/x-raw-rgb;video/x-raw-gray");
  encode_bin->raw_audio_caps =
      gst_caps_from_string ("audio/x-raw-int;audio/x-raw-float");
  /* encode_bin->raw_text_caps = */
  /*     gst_caps_from_string ("text/plain;text/x-pango-markup"); */

  encode_bin->queue_buffers_max = DEFAULT_QUEUE_BUFFERS_MAX;
  encode_bin->queue_bytes_max = DEFAULT_QUEUE_BYTES_MAX;
  encode_bin->queue_time_max = DEFAULT_QUEUE_TIME_MAX;
  encode_bin->tolerance = DEFAULT_AUDIO_JITTER_TOLERANCE;
  encode_bin->avoid_reencoding = DEFAULT_AVOID_REENCODING;

  tmpl = gst_static_pad_template_get (&muxer_src_template);
  encode_bin->srcpad = gst_ghost_pad_new_no_target_from_template ("src", tmpl);
  gst_object_unref (tmpl);
  gst_element_add_pad (GST_ELEMENT_CAST (encode_bin), encode_bin->srcpad);
}

static void
gst_encode_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEncodeBin *ebin = (GstEncodeBin *) object;

  switch (prop_id) {
    case PROP_PROFILE:
      gst_encode_bin_set_profile (ebin,
          (GstEncodingProfile *) gst_value_get_mini_object (value));
      break;
    case PROP_QUEUE_BUFFERS_MAX:
      ebin->queue_buffers_max = g_value_get_uint (value);
      break;
    case PROP_QUEUE_BYTES_MAX:
      ebin->queue_bytes_max = g_value_get_uint (value);
      break;
    case PROP_QUEUE_TIME_MAX:
      ebin->queue_time_max = g_value_get_uint64 (value);
      break;
    case PROP_AUDIO_JITTER_TOLERANCE:
      ebin->tolerance = g_value_get_uint64 (value);
      break;
    case PROP_AVOID_REENCODING:
      ebin->avoid_reencoding = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_encode_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstEncodeBin *ebin = (GstEncodeBin *) object;

  switch (prop_id) {
    case PROP_PROFILE:
      gst_value_set_mini_object (value, (GstMiniObject *) ebin->profile);
      break;
    case PROP_QUEUE_BUFFERS_MAX:
      g_value_set_uint (value, ebin->queue_buffers_max);
      break;
    case PROP_QUEUE_BYTES_MAX:
      g_value_set_uint (value, ebin->queue_bytes_max);
      break;
    case PROP_QUEUE_TIME_MAX:
      g_value_set_uint64 (value, ebin->queue_time_max);
      break;
    case PROP_AUDIO_JITTER_TOLERANCE:
      g_value_set_uint64 (value, ebin->tolerance);
      break;
    case PROP_AVOID_REENCODING:
      g_value_set_boolean (value, ebin->avoid_reencoding);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static inline gboolean
are_raw_caps (const GstCaps * caps)
{
  GstCaps *raw = gst_static_caps_get (&default_raw_caps);

  if (gst_caps_can_intersect (caps, raw)) {
    gst_caps_unref (raw);
    return TRUE;
  }
  gst_caps_unref (raw);
  return FALSE;
}

/* Returns the number of time a given stream profile is currently used
 * in encodebin */
static inline guint
stream_profile_used_count (GstEncodeBin * ebin, GstEncodingProfile * sprof)
{
  guint nbprofused = 0;
  GList *tmp;

  for (tmp = ebin->streams; tmp; tmp = tmp->next) {
    StreamGroup *sgroup = (StreamGroup *) tmp->data;

    if (sgroup->profile == sprof)
      nbprofused++;
  }

  return nbprofused;
}

static inline GstEncodingProfile *
next_unused_stream_profile (GstEncodeBin * ebin, GType ptype, GstCaps * caps)
{
  GST_DEBUG_OBJECT (ebin, "ptype:%s, caps:%" GST_PTR_FORMAT,
      g_type_name (ptype), caps);

  if (G_UNLIKELY (ptype == G_TYPE_NONE && caps != NULL)) {
    /* Identify the profile type based on raw caps */
    if (gst_caps_can_intersect (ebin->raw_video_caps, caps))
      ptype = GST_TYPE_ENCODING_VIDEO_PROFILE;
    else if (gst_caps_can_intersect (ebin->raw_audio_caps, caps))
      ptype = GST_TYPE_ENCODING_AUDIO_PROFILE;
    /* else if (gst_caps_can_intersect (ebin->raw_text_caps, caps)) */
    /*   ptype = GST_TYPE_ENCODING_TEXT_PROFILE; */
    GST_DEBUG_OBJECT (ebin, "Detected profile type as being %s",
        g_type_name (ptype));
  }

  if (GST_IS_ENCODING_CONTAINER_PROFILE (ebin->profile)) {
    const GList *tmp;

    for (tmp =
        gst_encoding_container_profile_get_profiles
        (GST_ENCODING_CONTAINER_PROFILE (ebin->profile)); tmp;
        tmp = tmp->next) {
      GstEncodingProfile *sprof = (GstEncodingProfile *) tmp->data;

      /* Pick an available Stream profile for which:
       * * either it is of the compatibly raw type,
       * * OR we can pass it through directly without encoding
       */
      if (G_TYPE_FROM_INSTANCE (sprof) == ptype) {
        guint presence = gst_encoding_profile_get_presence (sprof);
        GST_DEBUG ("Found a stream profile with the same type");
        if ((presence == 0)
            || (presence > stream_profile_used_count (ebin, sprof)))
          return sprof;
      } else if ((caps != NULL) && (ptype == G_TYPE_NONE)) {
        GstCaps *outcaps;
        gboolean res;

        outcaps = gst_encoding_profile_get_input_caps (sprof);
        GST_DEBUG ("Unknown stream, seeing if it's compatible with %"
            GST_PTR_FORMAT, outcaps);
        res = gst_caps_can_intersect (outcaps, caps);
        gst_caps_unref (outcaps);

        if (res)
          return sprof;
      }
    }
  }

  return NULL;
}

static GstPad *
request_pad_for_stream (GstEncodeBin * encodebin, GType ptype,
    const gchar * name, GstCaps * caps)
{
  StreamGroup *sgroup;
  GstEncodingProfile *sprof;

  GST_DEBUG_OBJECT (encodebin, "name:%s caps:%" GST_PTR_FORMAT, name, caps);

  /* Figure out if we have a unused GstEncodingProfile we can use for
   * these caps */
  sprof = next_unused_stream_profile (encodebin, ptype, caps);

  if (G_UNLIKELY (sprof == NULL))
    goto no_stream_profile;

  sgroup = _create_stream_group (encodebin, sprof, name, caps);
  if (G_UNLIKELY (sgroup == NULL))
    goto no_stream_group;

  return sgroup->ghostpad;

no_stream_profile:
  {
    GST_WARNING_OBJECT (encodebin, "Couldn't find a compatible stream profile");
    return NULL;
  }

no_stream_group:
  {
    GST_WARNING_OBJECT (encodebin, "Couldn't create a StreamGroup");
    return NULL;
  }
}

static GstPad *
gst_encode_bin_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstEncodeBin *ebin = (GstEncodeBin *) element;
  GstPad *res = NULL;

  GST_DEBUG_OBJECT (element, "templ:%s, name:%s", templ->name_template, name);

  /* Identify the stream group */
  if (caps != NULL) {
    res = request_pad_for_stream (ebin, G_TYPE_NONE, name, (GstCaps *) caps);
  }

  if (res == NULL) {
    GType ptype = G_TYPE_NONE;

    if (!strcmp (templ->name_template, "video_%d"))
      ptype = GST_TYPE_ENCODING_VIDEO_PROFILE;
    else if (!strcmp (templ->name_template, "audio_%d"))
      ptype = GST_TYPE_ENCODING_AUDIO_PROFILE;
    /* else if (!strcmp (templ->name_template, "text_%d")) */
    /*   ptype = GST_TYPE_ENCODING_TEXT_PROFILE; */

    /* FIXME : Check uniqueness of pad */
    /* FIXME : Check that the requested number is the last one, and if not,
     * update the last_pad_id variable so that we don't create a pad with
     * the same name/number in the future */

    res = request_pad_for_stream (ebin, ptype, name, NULL);
  }

  return res;
}

static GstPad *
gst_encode_bin_request_pad_signal (GstEncodeBin * encodebin, GstCaps * caps)
{
  GstPad *pad = request_pad_for_stream (encodebin, G_TYPE_NONE, NULL, caps);

  return pad ? GST_PAD_CAST (gst_object_ref (pad)) : NULL;
}

static inline StreamGroup *
find_stream_group_from_pad (GstEncodeBin * ebin, GstPad * pad)
{
  GList *tmp;

  for (tmp = ebin->streams; tmp; tmp = tmp->next) {
    StreamGroup *sgroup = (StreamGroup *) tmp->data;
    if (G_UNLIKELY (sgroup->ghostpad == pad))
      return sgroup;
  }

  return NULL;
}

static void
gst_encode_bin_release_pad (GstElement * element, GstPad * pad)
{
  GstEncodeBin *ebin = (GstEncodeBin *) element;
  StreamGroup *sgroup;

  /* Find the associated StreamGroup */

  sgroup = find_stream_group_from_pad (ebin, pad);
  if (G_UNLIKELY (sgroup == NULL))
    goto no_stream_group;

  /* Release objects/data associated with the StreamGroup */
  stream_group_remove (ebin, sgroup);

  return;

no_stream_group:
  {
    GST_WARNING_OBJECT (ebin, "Couldn't find corresponding StreamGroup");
    return;
  }
}

/* Create a parser for the given stream profile */
static inline GstElement *
_get_parser (GstEncodeBin * ebin, GstEncodingProfile * sprof)
{
  GList *parsers1, *parsers, *tmp;
  GstElement *parser = NULL;
  GstElementFactory *parserfact = NULL;
  const GstCaps *format;

  format = gst_encoding_profile_get_format (sprof);

  GST_DEBUG ("Getting list of parsers for format %" GST_PTR_FORMAT, format);

  /* FIXME : requesting twice the parsers twice is a bit ugly, we should
   * have a method to request on more than one condition */
  parsers1 =
      gst_element_factory_list_filter (ebin->parsers, format,
      GST_PAD_SRC, FALSE);
  parsers =
      gst_element_factory_list_filter (parsers1, format, GST_PAD_SINK, FALSE);
  gst_plugin_feature_list_free (parsers1);

  if (G_UNLIKELY (parsers == NULL)) {
    GST_DEBUG ("Couldn't find any compatible parsers");
    return NULL;
  }

  for (tmp = parsers; tmp; tmp = tmp->next) {
    /* FIXME : We're only picking the first one so far */
    /* FIXME : signal the user if he wants this */
    parserfact = (GstElementFactory *) tmp->data;
    break;
  }

  if (parserfact)
    parser = gst_element_factory_create (parserfact, NULL);

  gst_plugin_feature_list_free (parsers);

  return parser;
}

static GstElement *
_create_element_and_set_preset (GstElementFactory * factory,
    const gchar * preset, const gchar * name)
{
  GstElement *res = NULL;

  GST_DEBUG ("Creating element from factory %s",
      GST_PLUGIN_FEATURE_NAME (factory));
  res = gst_element_factory_create (factory, name);
  if (preset && GST_IS_PRESET (res) &&
      !gst_preset_load_preset (GST_PRESET (res), preset)) {
    GST_WARNING ("Couldn't set preset [%s] on element [%s]",
        preset, GST_PLUGIN_FEATURE_NAME (factory));
    gst_object_unref (res);
    res = NULL;
  }

  return res;
}

/* Create the encoder for the given stream profile */
static inline GstElement *
_get_encoder (GstEncodeBin * ebin, GstEncodingProfile * sprof)
{
  GList *encoders, *tmp;
  GstElement *encoder = NULL;
  GstElementFactory *encoderfact = NULL;
  const GstCaps *format;
  const gchar *preset;

  format = gst_encoding_profile_get_format (sprof);
  preset = gst_encoding_profile_get_preset (sprof);

  GST_DEBUG ("Getting list of encoders for format %" GST_PTR_FORMAT, format);

  /* If stream caps are raw, return identity */
  if (G_UNLIKELY (are_raw_caps (format))) {
    GST_DEBUG ("Stream format is raw, returning identity as the encoder");
    encoder = gst_element_factory_make ("identity", NULL);
    goto beach;
  }

  encoders =
      gst_element_factory_list_filter (ebin->encoders, format,
      GST_PAD_SRC, FALSE);

  if (G_UNLIKELY (encoders == NULL)) {
    GST_DEBUG ("Couldn't find any compatible encoders");
    goto beach;
  }

  for (tmp = encoders; tmp; tmp = tmp->next) {
    encoderfact = (GstElementFactory *) tmp->data;
    if ((encoder = _create_element_and_set_preset (encoderfact, preset, NULL)))
      break;
  }

  gst_plugin_feature_list_free (encoders);

beach:
  return encoder;
}

static GstPad *
local_element_request_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * name)
{
  GstPad *newpad = NULL;
  GstElementClass *oclass;

  oclass = GST_ELEMENT_GET_CLASS (element);

  if (oclass->request_new_pad)
    newpad = (oclass->request_new_pad) (element, templ, name);

  if (newpad)
    gst_object_ref (newpad);

  return newpad;
}

static GstPad *
gst_element_get_pad_from_template (GstElement * element, GstPadTemplate * templ)
{
  GstPad *ret = NULL;
  GstPadPresence presence;

  /* If this function is ever exported, we need check the validity of `element'
   * and `templ', and to make sure the template actually belongs to the
   * element. */

  presence = GST_PAD_TEMPLATE_PRESENCE (templ);

  switch (presence) {
    case GST_PAD_ALWAYS:
    case GST_PAD_SOMETIMES:
      ret = gst_element_get_static_pad (element, templ->name_template);
      if (!ret && presence == GST_PAD_ALWAYS)
        g_warning
            ("Element %s has an ALWAYS template %s, but no pad of the same name",
            GST_OBJECT_NAME (element), templ->name_template);
      break;

    case GST_PAD_REQUEST:
      ret = gst_element_request_pad (element, templ, NULL, NULL);
      break;
  }

  return ret;
}

/* FIXME : Improve algorithm for finding compatible muxer sink pad */
static inline GstPad *
get_compatible_muxer_sink_pad (GstEncodeBin * ebin, GstElement * encoder,
    const GstCaps * sinkcaps)
{
  GstPad *sinkpad;
  GstPadTemplate *srctempl = NULL;
  GstPadTemplate *sinktempl;

  if (encoder) {
    GstPad *srcpad;
    srcpad = gst_element_get_static_pad (encoder, "src");

    srctempl = gst_pad_get_pad_template (srcpad);

    GST_DEBUG_OBJECT (ebin,
        "Attempting to find pad from muxer %s compatible with %s:%s",
        GST_ELEMENT_NAME (ebin->muxer), GST_DEBUG_PAD_NAME (srcpad));

    gst_object_unref (srcpad);
    sinktempl = gst_element_get_compatible_pad_template (ebin->muxer, srctempl);
  } else {
    srctempl =
        gst_pad_template_new ("whatever", GST_PAD_SRC, GST_PAD_ALWAYS,
        gst_caps_copy (sinkcaps));
    g_assert (srctempl != NULL);
    sinktempl = gst_element_get_compatible_pad_template (ebin->muxer, srctempl);
    g_object_unref (srctempl);
  }

  if (G_UNLIKELY (sinktempl == NULL))
    goto no_template;

  sinkpad = gst_element_get_pad_from_template (ebin->muxer, sinktempl);

  return sinkpad;

no_template:
  {
    GST_WARNING_OBJECT (ebin, "No compatible pad available on muxer");
    return NULL;
  }
}

/* FIXME : Add handling of streams that don't need encoding  */
/* FIXME : Add handling of streams that don't require conversion elements */
/*
 * Create the elements, StreamGroup, add the sink pad, link it to the muxer
 *
 * sinkpadname: If non-NULL, that name will be assigned to the sink ghost pad
 * sinkcaps: If non-NULL will be used to figure out how to setup the group */
static StreamGroup *
_create_stream_group (GstEncodeBin * ebin, GstEncodingProfile * sprof,
    const gchar * sinkpadname, GstCaps * sinkcaps)
{
  StreamGroup *sgroup = NULL;
  GstPad *sinkpad, *srcpad, *muxerpad = NULL;
  /* Element we will link to the encoder */
  GstElement *last = NULL;
  GList *tmp, *tosync = NULL;
  const GstCaps *format;
  const GstCaps *restriction;

  format = gst_encoding_profile_get_format (sprof);
  restriction = gst_encoding_profile_get_restriction (sprof);

  GST_DEBUG ("Creating group. format %" GST_PTR_FORMAT ", for caps %"
      GST_PTR_FORMAT, format, sinkcaps);
  GST_DEBUG ("avoid_reencoding:%d", ebin->avoid_reencoding);

  sgroup = g_slice_new0 (StreamGroup);
  sgroup->ebin = ebin;
  sgroup->profile = sprof;

  /* NOTE for people reading this code:
   * 
   * We construct the group starting by the furthest downstream element
   * and making our way up adding/syncing/linking as we go.
   *
   * There are two parallel paths:
   * * One for raw data which goes through converters and encoders
   * * One for already encoded data
   */

  /* Exception to the rule above:
   * We check if we have an available encoder so we can abort early */
  /* FIXME : What if we only want to do passthrough ??? */
  GST_LOG ("Checking for encoder availability");
  sgroup->encoder = _get_encoder (ebin, sprof);
  if (G_UNLIKELY (sgroup->encoder == NULL))
    goto no_encoder;

  /* Muxer.
   * If we are handling a container profile, figure out if the muxer has a
   * sinkpad compatible with the selected profile */
  if (ebin->muxer) {
    muxerpad = get_compatible_muxer_sink_pad (ebin, NULL, format);
    if (G_UNLIKELY (muxerpad == NULL))
      goto no_muxer_pad;
  }

  /* Output Queue.
   * We only use a 1buffer long queue here, the actual queueing will be done
   * in the intput queue */
  last = sgroup->outqueue = gst_element_factory_make ("queue", NULL);
  g_object_set (sgroup->outqueue, "max-size-buffers", (guint32) 1,
      "max-size-bytes", (guint32) 0, "max-size-time", (guint64) 0, NULL);

  gst_bin_add (GST_BIN (ebin), sgroup->outqueue);
  tosync = g_list_append (tosync, sgroup->outqueue);
  srcpad = gst_element_get_static_pad (sgroup->outqueue, "src");
  if (muxerpad) {
    if (G_UNLIKELY (fast_pad_link (srcpad, muxerpad) != GST_PAD_LINK_OK)) {
      goto muxer_link_failure;
    }
    gst_object_unref (muxerpad);
  } else {
    gst_ghost_pad_set_target (GST_GHOST_PAD (ebin->srcpad), srcpad);
  }
  gst_object_unref (srcpad);

  /* Output capsfilter
   * This will receive the format caps from the streamprofile */
  GST_DEBUG ("Adding output capsfilter for %" GST_PTR_FORMAT, format);
  sgroup->outfilter = gst_element_factory_make ("capsfilter", NULL);
  g_object_set (sgroup->outfilter, "caps", format, NULL);

  gst_bin_add (GST_BIN (ebin), sgroup->outfilter);
  tosync = g_list_append (tosync, sgroup->outfilter);
  if (G_UNLIKELY (!fast_element_link (sgroup->outfilter, last)))
    goto outfilter_link_failure;
  last = sgroup->outfilter;


  /* FIXME :
   *
   *   The usage of parsers in encoding/muxing scenarios is
   * just too undefined to just use as-is.
   *
   * Take the use-case where you want to re-mux a stream of type
   * "my/media". You create a StreamEncodingProfile with that type
   * as the target (as-is). And you use decodebin2/uridecodebin
   * upstream.
   *
   * * demuxer exposes "my/media"
   * * a parser is available for "my/media" which has a source pad
   *   caps of "my/media,parsed=True"
   * * decodebin2/uridecodebin exposes a new pad with the parsed caps
   * * You request a new stream from encodebin, which will match the
   *   streamprofile and creates a group (i.e. going through this method)
   *   There is a matching parser (the same used in the decoder) whose
   *   source pad caps intersects with the stream profile caps, you
   *   therefore use it...
   * * ... but that parser has a "my/media,parsed=False" sink pad caps
   * * ... and you can't link your decodebin pad to encodebin.
   *
   * In the end, it comes down to parsers only taking into account the
   * decoding use-cases.
   *
   * One way to solve that might be to :
   * * Make parsers sink pad caps be "framed={False,True}" and the
   *   source pad caps be "framed=True"
   * * Modify decodebin2 accordingly to avoid looping and chaining
   *   an infinite number of parsers
   *
   * Another way would be to have "well-known" caps properties to specify
   * whether a stream has been parsed or not.
   * * currently we fail. aacparse uses 'framed' and mp3parse uses 'parsed'
   */
  /* FIXME : Re-enable once parser situation is un-$#*@(%$#ed */
#if 0
  /* Parser.
   * FIXME : identify smart parsers (used for re-encoding) */
  sgroup->parser = _get_parser (ebin, sprof);

  if (sgroup->parser != NULL) {
    GST_DEBUG ("Got a parser %s", GST_ELEMENT_NAME (sgroup->parser));
    gst_bin_add (GST_BIN (ebin), sgroup->parser);
    tosync = g_list_append (tosync, sgroup->parser);
    if (G_UNLIKELY (!gst_element_link (sgroup->parser, last)))
      goto parser_link_failure;
    last = sgroup->parser;
  }
#endif

  /* Stream combiner */
  sgroup->combiner = g_object_new (GST_TYPE_STREAM_COMBINER, NULL);

  gst_bin_add (GST_BIN (ebin), sgroup->combiner);
  tosync = g_list_append (tosync, sgroup->combiner);
  if (G_UNLIKELY (!fast_element_link (sgroup->combiner, last)))
    goto combiner_link_failure;


  /* Stream splitter */
  sgroup->splitter = g_object_new (GST_TYPE_STREAM_SPLITTER, NULL);

  gst_bin_add (GST_BIN (ebin), sgroup->splitter);
  tosync = g_list_append (tosync, sgroup->splitter);

  /* Input queue
   * FIXME : figure out what max-size to use for the input queue */
  sgroup->inqueue = gst_element_factory_make ("queue", NULL);
  g_object_set (sgroup->inqueue, "max-size-buffers",
      (guint32) ebin->queue_buffers_max, "max-size-bytes",
      (guint32) ebin->queue_bytes_max, "max-size-time",
      (guint64) ebin->queue_time_max, NULL);

  gst_bin_add (GST_BIN (ebin), sgroup->inqueue);
  tosync = g_list_append (tosync, sgroup->inqueue);
  if (G_UNLIKELY (!fast_element_link (sgroup->inqueue, sgroup->splitter)))
    goto splitter_link_failure;

  /* Expose input queue sink pad as ghostpad */
  sinkpad = gst_element_get_static_pad (sgroup->inqueue, "sink");
  if (sinkpadname == NULL) {
    gchar *pname =
        g_strdup_printf ("%s_%d", gst_encoding_profile_get_type_nick (sprof),
        ebin->last_pad_id++);
    GST_DEBUG ("Adding ghost pad %s", pname);
    sgroup->ghostpad = gst_ghost_pad_new (pname, sinkpad);
    g_free (pname);
  } else
    sgroup->ghostpad = gst_ghost_pad_new (sinkpadname, sinkpad);
  gst_object_unref (sinkpad);


  /* Path 1 : Already-encoded data */
  sinkpad =
      local_element_request_pad (sgroup->combiner, NULL, "passthroughsink");
  if (G_UNLIKELY (sinkpad == NULL))
    goto no_combiner_sinkpad;

  if (ebin->avoid_reencoding) {
    GstCaps *tmpcaps;

    GST_DEBUG ("Asked to use Smart Encoder");
    sgroup->smartencoder = g_object_new (GST_TYPE_SMART_ENCODER, NULL);

    /* Check if stream format is compatible */
    srcpad = gst_element_get_static_pad (sgroup->smartencoder, "src");
    tmpcaps = gst_pad_get_caps (srcpad);
    if (!gst_caps_can_intersect (tmpcaps, format)) {
      GST_DEBUG ("We don't have a smart encoder for the stream format");
      gst_object_unref (sgroup->smartencoder);
      sgroup->smartencoder = NULL;
    } else {
      gst_bin_add ((GstBin *) ebin, sgroup->smartencoder);
      fast_pad_link (srcpad, sinkpad);
      tosync = g_list_append (tosync, sgroup->smartencoder);
      sinkpad = gst_element_get_static_pad (sgroup->smartencoder, "sink");
    }
    gst_caps_unref (tmpcaps);
    g_object_unref (srcpad);
  }

  srcpad = local_element_request_pad (sgroup->splitter, NULL, "passthroughsrc");
  if (G_UNLIKELY (srcpad == NULL))
    goto no_splitter_srcpad;

  /* Go straight to splitter */
  if (G_UNLIKELY (fast_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK))
    goto passthrough_link_failure;
  g_object_unref (sinkpad);
  g_object_unref (srcpad);


  /* Path 2 : Conversion / Encoding */

  /* 1. Create the encoder */
  GST_LOG ("Adding encoder");
  last = sgroup->encoder;
  gst_bin_add ((GstBin *) ebin, sgroup->encoder);
  tosync = g_list_append (tosync, sgroup->encoder);

  sinkpad = local_element_request_pad (sgroup->combiner, NULL, "encodingsink");
  if (G_UNLIKELY (sinkpad == NULL))
    goto no_combiner_sinkpad;
  srcpad = gst_element_get_static_pad (sgroup->encoder, "src");
  if (G_UNLIKELY (fast_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK))
    goto encoder_link_failure;
  g_object_unref (sinkpad);
  g_object_unref (srcpad);


  /* 3. Create the conversion/restriction elements */
  /* 3.1. capsfilter */
  if (restriction && !gst_caps_is_any (restriction)) {
    GST_LOG ("Adding capsfilter for restriction caps : %" GST_PTR_FORMAT,
        restriction);

    last = sgroup->capsfilter = gst_element_factory_make ("capsfilter", NULL);
    g_object_set (sgroup->capsfilter, "caps", restriction, NULL);
    gst_bin_add ((GstBin *) ebin, sgroup->capsfilter);
    tosync = g_list_append (tosync, sgroup->capsfilter);
    fast_element_link (sgroup->capsfilter, sgroup->encoder);
  }

  /* 3.2. restriction elements */
  /* FIXME : Once we have properties for specific converters, use those */
  if (GST_IS_ENCODING_VIDEO_PROFILE (sprof)) {
    GstElement *cspace, *scale, *vrate, *cspace2;

    GST_LOG ("Adding conversion elements for video stream");

    cspace = gst_element_factory_make ("stridetransform", NULL);
    scale = gst_element_factory_make ("identity", NULL);
    /* 4-tap scaling and black borders */
#if 0
    g_object_set (scale, "method", 2, "add-borders", TRUE, NULL);
#endif
    cspace2 = gst_element_factory_make ("stridetransform", NULL);

    gst_bin_add_many ((GstBin *) ebin, cspace, scale, cspace2, NULL);
    tosync = g_list_append (tosync, cspace);
    tosync = g_list_append (tosync, scale);
    tosync = g_list_append (tosync, cspace2);

    sgroup->converters = g_list_prepend (sgroup->converters, cspace);
    sgroup->converters = g_list_prepend (sgroup->converters, scale);
    sgroup->converters = g_list_prepend (sgroup->converters, cspace2);

    if (!fast_element_link (cspace, scale) ||
        !fast_element_link (scale, cspace2))
      goto converter_link_failure;

    if (!gst_encoding_video_profile_get_variableframerate
        (GST_ENCODING_VIDEO_PROFILE (sprof))) {
      vrate = gst_element_factory_make ("videorate", NULL);
      gst_bin_add ((GstBin *) ebin, vrate);
      tosync = g_list_prepend (tosync, vrate);
      sgroup->converters = g_list_prepend (sgroup->converters, vrate);
      if (!fast_element_link (cspace2, vrate) ||
          !fast_element_link (vrate, last))
        goto converter_link_failure;
    } else if (!fast_element_link (cspace2, last))
      goto converter_link_failure;

    last = cspace;

  } else if (GST_IS_ENCODING_AUDIO_PROFILE (sprof)) {
    GstElement *aconv, *ares, *arate, *aconv2;

    GST_LOG ("Adding conversion elements for audio stream");

    arate = gst_element_factory_make ("audiorate", NULL);
    g_object_set (arate, "tolerance", (guint64) ebin->tolerance, NULL);
    aconv = gst_element_factory_make ("audioconvert", NULL);
    aconv2 = gst_element_factory_make ("audioconvert", NULL);
    ares = gst_element_factory_make ("audioresample", NULL);

    gst_bin_add_many ((GstBin *) ebin, arate, aconv, ares, aconv2, NULL);
    tosync = g_list_append (tosync, arate);
    tosync = g_list_append (tosync, aconv);
    tosync = g_list_append (tosync, ares);
    tosync = g_list_append (tosync, aconv2);
    if (!fast_element_link (arate, aconv) ||
        !fast_element_link (aconv, ares) ||
        !fast_element_link (ares, aconv2) || !fast_element_link (aconv2, last))
      goto converter_link_failure;

    sgroup->converters = g_list_prepend (sgroup->converters, arate);
    sgroup->converters = g_list_prepend (sgroup->converters, aconv);
    sgroup->converters = g_list_prepend (sgroup->converters, ares);
    sgroup->converters = g_list_prepend (sgroup->converters, aconv2);

    last = arate;
  }

  /* Link to stream splitter */
  sinkpad = gst_element_get_static_pad (last, "sink");
  srcpad = local_element_request_pad (sgroup->splitter, NULL, "encodingsrc");
  if (G_UNLIKELY (srcpad == NULL))
    goto no_splitter_srcpad;
  if (G_UNLIKELY (fast_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK))
    goto splitter_encoding_failure;
  g_object_unref (sinkpad);
  g_object_unref (srcpad);

  /* End of Stream 2 setup */

  /* Sync all elements to parent state */
  for (tmp = tosync; tmp; tmp = tmp->next)
    gst_element_sync_state_with_parent ((GstElement *) tmp->data);
  g_list_free (tosync);

  /* Add ghostpad */
  GST_DEBUG ("Adding ghostpad %s:%s", GST_DEBUG_PAD_NAME (sgroup->ghostpad));
  gst_pad_set_active (sgroup->ghostpad, TRUE);
  gst_element_add_pad ((GstElement *) ebin, sgroup->ghostpad);

  /* Add StreamGroup to our list of streams */

  GST_DEBUG
      ("Done creating elements, adding StreamGroup to our controlled stream list");

  ebin->streams = g_list_prepend (ebin->streams, sgroup);

  return sgroup;

splitter_encoding_failure:
  GST_ERROR_OBJECT (ebin, "Error linking splitter to encoding stream");
  goto cleanup;

no_encoder:
  GST_ERROR_OBJECT (ebin, "Couldn't create encoder for format %" GST_PTR_FORMAT,
      format);
  /* missing plugin support */
  gst_element_post_message (GST_ELEMENT_CAST (ebin),
      gst_missing_encoder_message_new (GST_ELEMENT_CAST (ebin), format));
  GST_ELEMENT_ERROR (ebin, CORE, MISSING_PLUGIN, (NULL),
      ("Couldn't create encoder for format %" GST_PTR_FORMAT, format));
  goto cleanup;

no_muxer_pad:
  GST_ERROR_OBJECT (ebin,
      "Couldn't find a compatible muxer pad to link encoder to");
  goto cleanup;

encoder_link_failure:
  GST_ERROR_OBJECT (ebin, "Failed to link the encoder");
  goto cleanup;

muxer_link_failure:
  GST_ERROR_OBJECT (ebin, "Couldn't link encoder to muxer");
  goto cleanup;

outfilter_link_failure:
  GST_ERROR_OBJECT (ebin, "Couldn't link output filter to output queue");
  goto cleanup;

passthrough_link_failure:
  GST_ERROR_OBJECT (ebin, "Failed linking splitter in passthrough mode");
  goto cleanup;

no_splitter_srcpad:
  GST_ERROR_OBJECT (ebin, "Couldn't get a source pad from the splitter");
  goto cleanup;

no_combiner_sinkpad:
  GST_ERROR_OBJECT (ebin, "Couldn't get a sink pad from the combiner");
  goto cleanup;

splitter_link_failure:
  GST_ERROR_OBJECT (ebin, "Failure linking to the splitter");
  goto cleanup;

combiner_link_failure:
  GST_ERROR_OBJECT (ebin, "Failure linking to the combiner");
  goto cleanup;

#if 0
parser_link_failure:
  GST_ERROR_OBJECT (ebin, "Failure linking the parser");
  goto cleanup;
#endif

converter_link_failure:
  GST_ERROR_OBJECT (ebin, "Failure linking the video converters");
  goto cleanup;

cleanup:
  /* FIXME : Actually properly cleanup everything */
  g_slice_free (StreamGroup, sgroup);
  return NULL;
}

static gboolean
_factory_can_sink_caps (GstElementFactory * factory, const GstCaps * caps)
{
  GList *templates = factory->staticpadtemplates;

  while (templates) {
    GstStaticPadTemplate *template = (GstStaticPadTemplate *) templates->data;

    if (template->direction == GST_PAD_SINK) {
      GstCaps *tmp = gst_static_caps_get (&template->static_caps);

      if (gst_caps_can_intersect (tmp, caps)) {
        gst_caps_unref (tmp);
        return TRUE;
      }
      gst_caps_unref (tmp);
    }
    templates = g_list_next (templates);
  }

  return FALSE;
}

static inline GstElement *
_get_muxer (GstEncodeBin * ebin)
{
  GList *muxers, *tmpmux;
  GstElement *muxer = NULL;
  GstElementFactory *muxerfact = NULL;
  const GList *tmp;
  const GstCaps *format;
  const gchar *preset;

  format = gst_encoding_profile_get_format (ebin->profile);
  preset = gst_encoding_profile_get_preset (ebin->profile);

  GST_DEBUG ("Getting list of muxers for format %" GST_PTR_FORMAT, format);

  muxers =
      gst_element_factory_list_filter (ebin->muxers, format, GST_PAD_SRC, TRUE);

  if (muxers == NULL)
    goto beach;

  /* FIXME : signal the user if he wants this */
  for (tmpmux = muxers; tmpmux; tmpmux = tmpmux->next) {
    gboolean cansinkstreams = TRUE;
    const GList *profiles =
        gst_encoding_container_profile_get_profiles
        (GST_ENCODING_CONTAINER_PROFILE (ebin->profile));

    muxerfact = (GstElementFactory *) tmpmux->data;

    GST_DEBUG ("Trying muxer %s", GST_PLUGIN_FEATURE_NAME (muxerfact));

    /* See if the muxer can sink all of our stream profile caps */
    for (tmp = profiles; tmp; tmp = tmp->next) {
      GstEncodingProfile *sprof = (GstEncodingProfile *) tmp->data;

      if (!_factory_can_sink_caps (muxerfact,
              gst_encoding_profile_get_format (sprof))) {
        GST_DEBUG ("Skipping muxer because it can't sink caps %" GST_PTR_FORMAT,
            gst_encoding_profile_get_format (sprof));
        cansinkstreams = FALSE;
        break;
      }
    }

    /* Only use a muxer than can use all streams and than can accept the
     * preset (which may be present or not) */
    if (cansinkstreams && (muxer =
            _create_element_and_set_preset (muxerfact, preset, "muxer")))
      break;
  }

  gst_plugin_feature_list_free (muxers);

beach:
  return muxer;
}

static gboolean
create_elements_and_pads (GstEncodeBin * ebin)
{
  gboolean ret = TRUE;
  GstElement *muxer = NULL;
  GstPad *muxerpad;
  const GList *tmp, *profiles;
  GstEncodingProfile *sprof;

  GST_DEBUG ("Current profile : %s",
      gst_encoding_profile_get_name (ebin->profile));

  if (GST_IS_ENCODING_CONTAINER_PROFILE (ebin->profile)) {
    /* 1. Get the compatible muxer */
    muxer = _get_muxer (ebin);
    if (G_UNLIKELY (muxer == NULL))
      goto no_muxer;

    /* Record the muxer */
    ebin->muxer = muxer;
    gst_bin_add ((GstBin *) ebin, muxer);

    /* 2. Ghost the muxer source pad */

    /* FIXME : We should figure out if it's a static/request/dyamic pad, 
     * but for the time being let's assume it's a static pad :) */
    muxerpad = gst_element_get_static_pad (muxer, "src");
    if (G_UNLIKELY (muxerpad == NULL))
      goto no_muxer_pad;

    if (!gst_ghost_pad_set_target (GST_GHOST_PAD (ebin->srcpad), muxerpad))
      goto no_muxer_ghost_pad;

    gst_object_unref (muxerpad);
    /* 3. Activate fixed presence streams */
    profiles =
        gst_encoding_container_profile_get_profiles
        (GST_ENCODING_CONTAINER_PROFILE (ebin->profile));
    for (tmp = profiles; tmp; tmp = tmp->next) {
      sprof = (GstEncodingProfile *) tmp->data;

      GST_DEBUG ("Trying stream profile with presence %d",
          gst_encoding_profile_get_presence (sprof));

      if (gst_encoding_profile_get_presence (sprof) != 0) {
        if (G_UNLIKELY (_create_stream_group (ebin, sprof, NULL, NULL) == NULL))
          goto stream_error;
      }
    }
  } else {
    if (G_UNLIKELY (_create_stream_group (ebin, ebin->profile, NULL,
                NULL) == NULL))
      goto stream_error;
  }

  return ret;

no_muxer:
  {
    GST_WARNING ("No available muxer for %" GST_PTR_FORMAT,
        gst_encoding_profile_get_format (ebin->profile));
    /* missing plugin support */
    gst_element_post_message (GST_ELEMENT_CAST (ebin),
        gst_missing_encoder_message_new (GST_ELEMENT_CAST (ebin),
            gst_encoding_profile_get_format (ebin->profile)));
    GST_ELEMENT_ERROR (ebin, CORE, MISSING_PLUGIN, (NULL),
        ("No available muxer for format %" GST_PTR_FORMAT,
            gst_encoding_profile_get_format (ebin->profile)));
    return FALSE;
  }

no_muxer_pad:
  {
    GST_WARNING ("Can't get source pad from muxer (%s)",
        GST_ELEMENT_NAME (muxer));
    gst_bin_remove (GST_BIN (ebin), muxer);
    return FALSE;
  }

no_muxer_ghost_pad:
  {
    GST_WARNING ("Couldn't set %s:%s as source ghostpad target",
        GST_DEBUG_PAD_NAME (muxerpad));
    gst_bin_remove (GST_BIN (ebin), muxer);
    gst_object_unref (muxerpad);
    return FALSE;
  }

stream_error:
  {
    GST_WARNING ("Could not create Streams");
    if (muxer)
      gst_bin_remove (GST_BIN (ebin), muxer);
    ebin->muxer = NULL;
    return FALSE;
  }
}

static void
release_pads (GstPad * pad, GstElement * elt)
{
  GstPad *peer = NULL;

  GST_DEBUG_OBJECT (elt, "Releasing pad %s:%s", GST_DEBUG_PAD_NAME (pad));

  /* Unlink from its peer pad */
  if ((peer = gst_pad_get_peer (pad))) {
    if (GST_PAD_DIRECTION (peer) == GST_PAD_SRC)
      gst_pad_unlink (peer, pad);
    else
      gst_pad_unlink (pad, peer);
    gst_object_unref (peer);
  }

  /* Release it from the object */
  gst_element_release_request_pad (elt, pad);

  /* And remove the reference added by the iterator */
  gst_object_unref (pad);
}

static void inline
stream_group_free (GstEncodeBin * ebin, StreamGroup * sgroup)
{
  GList *tmp;
  GstPad *tmppad;
  GstPad *pad;

  GST_DEBUG_OBJECT (ebin, "Freeing StreamGroup %p", sgroup);

  if (ebin->muxer) {
    /* outqueue - Muxer */
    tmppad = gst_element_get_static_pad (sgroup->outqueue, "src");
    pad = gst_pad_get_peer (tmppad);

    /* Remove muxer request sink pad */
    gst_pad_unlink (tmppad, pad);
    gst_element_release_request_pad (ebin->muxer, pad);
    gst_object_unref (tmppad);
    gst_object_unref (pad);
  }
  if (sgroup->outqueue)
    gst_element_set_state (sgroup->outqueue, GST_STATE_NULL);

  /* Capsfilter - outqueue */
  gst_element_set_state (sgroup->outfilter, GST_STATE_NULL);
  gst_element_unlink (sgroup->outfilter, sgroup->outqueue);
  gst_bin_remove (GST_BIN (ebin), sgroup->outqueue);

  /* streamcombiner - parser - capsfilter */
  if (sgroup->parser) {
    gst_element_set_state (sgroup->parser, GST_STATE_NULL);
    gst_element_unlink (sgroup->parser, sgroup->outfilter);
    gst_element_unlink (sgroup->combiner, sgroup->parser);
  }

  /* Sink Ghostpad */
  if (sgroup->ghostpad)
    gst_element_remove_pad (GST_ELEMENT_CAST (ebin), sgroup->ghostpad);

  if (sgroup->inqueue)
    gst_element_set_state (sgroup->inqueue, GST_STATE_NULL);

  if (sgroup->encoder)
    gst_element_set_state (sgroup->encoder, GST_STATE_NULL);
  if (sgroup->outfilter)
    gst_element_set_state (sgroup->outfilter, GST_STATE_NULL);
  if (sgroup->smartencoder)
    gst_element_set_state (sgroup->smartencoder, GST_STATE_NULL);

  if (sgroup->capsfilter) {
    gst_element_set_state (sgroup->capsfilter, GST_STATE_NULL);
    gst_element_unlink (sgroup->capsfilter, sgroup->encoder);
    gst_bin_remove ((GstBin *) ebin, sgroup->capsfilter);
  }

  for (tmp = sgroup->converters; tmp; tmp = tmp->next) {
    GstElement *elt = (GstElement *) tmp->data;

    gst_element_set_state (elt, GST_STATE_NULL);
    gst_bin_remove ((GstBin *) ebin, elt);
  }
  if (sgroup->converters)
    g_list_free (sgroup->converters);

  if (sgroup->combiner) {
    GstIterator *it = gst_element_iterate_sink_pads (sgroup->combiner);
    GstIteratorResult itret = GST_ITERATOR_OK;

    while (itret == GST_ITERATOR_OK || itret == GST_ITERATOR_RESYNC) {
      itret = gst_iterator_foreach (it, (GFunc) release_pads, sgroup->combiner);
      gst_iterator_resync (it);
    }
    gst_iterator_free (it);
  }

  if (sgroup->splitter) {
    GstIterator *it = gst_element_iterate_src_pads (sgroup->splitter);
    GstIteratorResult itret = GST_ITERATOR_OK;
    while (itret == GST_ITERATOR_OK || itret == GST_ITERATOR_RESYNC) {
      itret = gst_iterator_foreach (it, (GFunc) release_pads, sgroup->splitter);
      gst_iterator_resync (it);
    }
    gst_iterator_free (it);
  }

  if (sgroup->inqueue)
    gst_bin_remove ((GstBin *) ebin, sgroup->inqueue);
  if (sgroup->encoder)
    gst_bin_remove ((GstBin *) ebin, sgroup->encoder);
  if (sgroup->smartencoder)
    gst_bin_remove ((GstBin *) ebin, sgroup->smartencoder);

  g_slice_free (StreamGroup, sgroup);
}

static void
stream_group_remove (GstEncodeBin * ebin, StreamGroup * sgroup)
{
  ebin->streams = g_list_remove (ebin->streams, sgroup);

  stream_group_free (ebin, sgroup);
}

static void
gst_encode_bin_tear_down_profile (GstEncodeBin * ebin)
{
  if (G_UNLIKELY (ebin->profile == NULL))
    return;

  GST_DEBUG ("Tearing down profile %s",
      gst_encoding_profile_get_name (ebin->profile));

  while (ebin->streams)
    stream_group_remove (ebin, (StreamGroup *) ebin->streams->data);

  /* Set ghostpad target to NULL */
  gst_ghost_pad_set_target (GST_GHOST_PAD (ebin->srcpad), NULL);

  /* Remove muxer if present */
  if (ebin->muxer) {
    gst_bin_remove (GST_BIN (ebin), ebin->muxer);
    ebin->muxer = NULL;
  }

  /* free/clear profile */
  gst_encoding_profile_unref (ebin->profile);
  ebin->profile = NULL;
}

static gboolean
gst_encode_bin_setup_profile (GstEncodeBin * ebin, GstEncodingProfile * profile)
{
  gboolean res;

  g_return_val_if_fail (ebin->profile == NULL, FALSE);

  GST_DEBUG ("Setting up profile %s (type:%s)",
      gst_encoding_profile_get_name (profile),
      gst_encoding_profile_get_type_nick (profile));

  ebin->profile = profile;
  gst_mini_object_ref ((GstMiniObject *) ebin->profile);

  /* Create elements */
  res = create_elements_and_pads (ebin);
  if (res == FALSE)
    gst_encode_bin_tear_down_profile (ebin);

  return res;
}

static gboolean
gst_encode_bin_set_profile (GstEncodeBin * ebin, GstEncodingProfile * profile)
{
  g_return_val_if_fail (GST_IS_ENCODING_PROFILE (profile), FALSE);

  GST_DEBUG_OBJECT (ebin, "profile : %s",
      gst_encoding_profile_get_name (profile));

  if (G_UNLIKELY (ebin->active)) {
    GST_WARNING_OBJECT (ebin, "Element already active, can't change profile");
    return FALSE;
  }

  /* If we're not active, we can deactivate the previous profile */
  if (ebin->profile) {
    gst_encode_bin_tear_down_profile (ebin);
  }

  return gst_encode_bin_setup_profile (ebin, profile);
}

static inline gboolean
gst_encode_bin_activate (GstEncodeBin * ebin)
{
  ebin->active = ebin->profile != NULL;
  return ebin->active;
}

static void
gst_encode_bin_deactivate (GstEncodeBin * ebin)
{
  ebin->active = FALSE;
}

static GstStateChangeReturn
gst_encode_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstEncodeBin *ebin = (GstEncodeBin *) element;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      if (!gst_encode_bin_activate (ebin)) {
        ret = GST_STATE_CHANGE_FAILURE;
        goto beach;
      }
      break;
    default:
      break;
  }

  ret =
      GST_ELEMENT_CLASS (gst_encode_bin_parent_class)->change_state (element,
      transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto beach;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_encode_bin_deactivate (ebin);
      break;
    default:
      break;
  }

beach:
  return ret;
}


static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean res;

  GST_DEBUG_CATEGORY_INIT (gst_encode_bin_debug, "encodebin", 0, "encoder bin");

#ifdef ENABLE_NLS
  GST_DEBUG ("binding text domain %s to locale dir %s", GETTEXT_PACKAGE,
      LOCALEDIR);
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */


  res = gst_element_register (plugin, "encodebin", GST_RANK_NONE,
      GST_TYPE_ENCODE_BIN);

  return res;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "encoding",
    "various encoding-related elements", plugin_init, VERSION, GST_LICENSE,
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
