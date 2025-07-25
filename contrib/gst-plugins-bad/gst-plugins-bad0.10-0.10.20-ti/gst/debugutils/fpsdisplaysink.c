/*
 *  Copyright 2009 Nokia Corporation <multimedia@maemo.org>
 *            2006 Zeeshan Ali <zeeshan.ali@nokia.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * SECTION:element-fpsdisplaysink
 *
 * Can display the current and average framerate as a testoverlay or on stdout.
 *
 * <refsect2>
 * <title>Example launch lines</title>
 * |[
 * gst-launch videotestsrc ! fpsdisplaysink
 * gst-launch videotestsrc ! fpsdisplaysink text-overlay=false
 * gst-launch filesrc location=video.avi ! decodebin2 name=d ! queue ! fpsdisplaysink d. ! queue ! fakesink sync=true
 * ]|
 * </refsect2>
 */
/* FIXME:
 * - can we avoid plugging the textoverlay?
 * - gst-seek 15 "videotestsrc ! fpsdisplaysink" dies when closing gst-seek
 *
 * NOTE:
 * - if we make ourself RANK_PRIMARY+10 or something that autovideosink would
 *   select and fpsdisplaysink is set to use autovideosink as its internal sink
 *   it doesn't work. Reason: autovideosink creates a fpsdisplaysink, that
 *   creates an autovideosink, that...
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debugutils-marshal.h"
#include "fpsdisplaysink.h"
#include <gst/interfaces/xoverlay.h>

#define DEFAULT_SIGNAL_FPS_MEASUREMENTS FALSE
#define DEFAULT_FPS_UPDATE_INTERVAL_MS 500      /* 500 ms */
#define DEFAULT_FONT "Sans 20"

/* generic templates */
static GstStaticPadTemplate fps_display_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (fps_display_sink_debug);
#define GST_CAT_DEFAULT fps_display_sink_debug

enum
{
  /* FILL ME */
  SIGNAL_FPS_MEASUREMENTS,
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_SYNC,
  ARG_TEXT_OVERLAY,
  ARG_VIDEO_SINK,
  ARG_FPS_UPDATE_INTERVAL,
  ARG_MAX_FPS,
  ARG_MIN_FPS,
  ARG_SIGNAL_FPS_MEASUREMENTS
      /* FILL ME */
};

static GstBinClass *parent_class = NULL;

static GstStateChangeReturn fps_display_sink_change_state (GstElement * element,
    GstStateChange transition);
static void fps_display_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void fps_display_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void fps_display_sink_dispose (GObject * object);

static gboolean display_current_fps (gpointer data);

static guint fpsdisplaysink_signals[LAST_SIGNAL] = { 0 };

static void
fps_display_sink_class_init (GstFPSDisplaySinkClass * klass)
{
  GObjectClass *gobject_klass = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_klass = GST_ELEMENT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_klass->set_property = fps_display_sink_set_property;
  gobject_klass->get_property = fps_display_sink_get_property;
  gobject_klass->dispose = fps_display_sink_dispose;

  g_object_class_install_property (gobject_klass, ARG_SYNC,
      g_param_spec_boolean ("sync",
          "Sync", "Sync on the clock (if the internally used sink doesn't "
          "have this property it will be ignored", TRUE,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_klass, ARG_TEXT_OVERLAY,
      g_param_spec_boolean ("text-overlay",
          "text-overlay",
          "Whether to use text-overlay", TRUE,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_klass, ARG_VIDEO_SINK,
      g_param_spec_object ("video-sink",
          "video-sink",
          "Video sink to use (Must only be called on NULL state)",
          GST_TYPE_ELEMENT, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_klass, ARG_FPS_UPDATE_INTERVAL,
      g_param_spec_int ("fps-update-interval", "Fps update interval",
          "Time between consecutive frames per second measures and update "
          " (in ms). Should be set on NULL state", 1, G_MAXINT,
          DEFAULT_FPS_UPDATE_INTERVAL_MS,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_klass, ARG_MAX_FPS,
      g_param_spec_double ("max-fps", "Max fps",
          "Maximum fps rate measured. Reset when going from NULL to READY."
          "-1 means no measurement has yet been done", -1, G_MAXDOUBLE, -1,
          G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  g_object_class_install_property (gobject_klass, ARG_MIN_FPS,
      g_param_spec_double ("min-fps", "Min fps",
          "Minimum fps rate measured. Reset when going from NULL to READY."
          "-1 means no measurement has yet been done", -1, G_MAXDOUBLE, -1,
          G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  g_object_class_install_property (gobject_klass, ARG_SIGNAL_FPS_MEASUREMENTS,
      g_param_spec_boolean ("signal-fps-measurements",
          "Signal fps measurements",
          "If the fps-measurements signal should be emited.",
          DEFAULT_SIGNAL_FPS_MEASUREMENTS,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  /**
   * GstFPSDisplaySink::fps-measurements:
   * @fpsdisplaysink: a #GstFPSDisplaySink
   * @fps: The current measured fps
   * @droprate: The rate at which buffers are being dropped
   * @avgfps: The average fps
   *
   * Signals the application about the measured fps
   *
   * Since: 0.10.20
   */
  fpsdisplaysink_signals[SIGNAL_FPS_MEASUREMENTS] =
      g_signal_new ("fps-measurements", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      __gst_debugutils_marshal_VOID__DOUBLE_DOUBLE_DOUBLE,
      G_TYPE_NONE, 3, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

  gstelement_klass->change_state = fps_display_sink_change_state;

  gst_element_class_add_pad_template (gstelement_klass,
      gst_static_pad_template_get (&fps_display_sink_template));

  gst_element_class_set_details_simple (gstelement_klass,
      "Measure and show framerate on videosink", "Sink/Video",
      "Shows the current frame-rate and drop-rate of the videosink as overlay or text on stdout",
      "Zeeshan Ali <zeeshan.ali@nokia.com>, Stefan Kost <stefan.kost@nokia.com>");
}

static gboolean
on_video_sink_data_flow (GstPad * pad, GstMiniObject * mini_obj,
    gpointer user_data)
{
  GstFPSDisplaySink *self = GST_FPS_DISPLAY_SINK (user_data);

#if 0
  if (GST_IS_BUFFER (mini_obj)) {
    GstBuffer *buf = GST_BUFFER_CAST (mini_obj);

    if (GST_CLOCK_TIME_IS_VALID (self->next_ts)) {
      if (GST_BUFFER_TIMESTAMP (buf) <= self->next_ts) {
        self->frames_rendered++;
      } else {
        GST_WARNING_OBJECT (self, "dropping frame : ts %" GST_TIME_FORMAT
            " < expected_ts %" GST_TIME_FORMAT,
            GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
            GST_TIME_ARGS (self->next_ts));
        self->frames_dropped++;
      }
    } else {
      self->frames_rendered++;
    }
  } else
#endif
  if (GST_IS_EVENT (mini_obj)) {
    GstEvent *ev = GST_EVENT_CAST (mini_obj);

    if (GST_EVENT_TYPE (ev) == GST_EVENT_QOS) {
      GstClockTimeDiff diff;
      GstClockTime ts;

      gst_event_parse_qos (ev, NULL, &diff, &ts);
      if (diff <= 0.0) {
        self->frames_rendered++;
      } else {
        self->frames_dropped++;
      }

      if (G_UNLIKELY (!self->timeout_id)) {
        self->last_ts = self->start_ts = gst_util_get_timestamp ();
        self->timeout_id =
            g_timeout_add (self->fps_update_interval, display_current_fps,
            (gpointer) self);
      }
    }
  }
  return TRUE;
}

static void
update_sub_sync (GstElement * sink, gpointer data)
{
  /* Some sinks (like autovideosink) don't have the sync property so
   * we check it exists before setting it to avoid a warning at
   * runtime. */
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (sink), "sync"))
    g_object_set (sink, "sync", *((gboolean *) data), NULL);
  else
    GST_WARNING ("Internal sink doesn't have sync property");
}

static void
fps_display_sink_update_sink_sync (GstFPSDisplaySink * self)
{
  GstIterator *iterator;

  if (self->video_sink == NULL)
    return;

  if (GST_IS_BIN (self->video_sink)) {
    iterator = gst_bin_iterate_sinks (GST_BIN (self->video_sink));
    gst_iterator_foreach (iterator, (GFunc) update_sub_sync,
        (void *) &self->sync);
    gst_iterator_free (iterator);
  } else
    update_sub_sync (self->video_sink, (void *) &self->sync);

}

static void
update_video_sink (GstFPSDisplaySink * self, GstElement * video_sink)
{
  GstPad *sink_pad;

  if (self->video_sink) {

    /* remove pad probe */
    sink_pad = gst_element_get_static_pad (self->video_sink, "sink");
    gst_pad_remove_data_probe (sink_pad, self->data_probe_id);
    gst_object_unref (sink_pad);
    self->data_probe_id = -1;

    /* remove ghost pad target */
    gst_ghost_pad_set_target (GST_GHOST_PAD (self->ghost_pad), NULL);

    /* remove old sink */
    gst_bin_remove (GST_BIN (self), self->video_sink);
    gst_object_unref (self->video_sink);
  }

  /* create child elements */
  self->video_sink = video_sink;

  if (self->video_sink == NULL)
    return;

  fps_display_sink_update_sink_sync (self);

  /* take a ref before bin takes the ownership */
  gst_object_ref (self->video_sink);

  gst_bin_add (GST_BIN (self), self->video_sink);

  /* attach or pad probe */
  sink_pad = gst_element_get_static_pad (self->video_sink, "sink");
  self->data_probe_id = gst_pad_add_data_probe (sink_pad,
      G_CALLBACK (on_video_sink_data_flow), (gpointer) self);
  gst_object_unref (sink_pad);
}

static void
fps_display_sink_init (GstFPSDisplaySink * self,
    GstFPSDisplaySinkClass * g_class)
{
  self->sync = FALSE;
  self->signal_measurements = DEFAULT_SIGNAL_FPS_MEASUREMENTS;
  self->use_text_overlay = TRUE;
  self->fps_update_interval = DEFAULT_FPS_UPDATE_INTERVAL_MS;
  self->video_sink = NULL;
  self->max_fps = -1;
  self->min_fps = -1;

  self->ghost_pad = gst_ghost_pad_new_no_target ("sink", GST_PAD_SINK);
  gst_element_add_pad (GST_ELEMENT (self), self->ghost_pad);
}

static gboolean
display_current_fps (gpointer data)
{
  GstFPSDisplaySink *self = GST_FPS_DISPLAY_SINK (data);
  guint64 frames_rendered, frames_dropped;
  gdouble rr, dr, average_fps;
  gchar fps_message[256];
  gdouble time_diff, time_elapsed;
  GstClockTime current_ts = gst_util_get_timestamp ();

  frames_rendered = self->frames_rendered;
  frames_dropped = self->frames_dropped;

  if ((frames_rendered + frames_dropped) == 0) {
    /* in case timer fired and we didn't yet get any QOS events */
    return TRUE;
  }

  time_diff = (gdouble) (current_ts - self->last_ts) / GST_SECOND;
  time_elapsed = (gdouble) (current_ts - self->start_ts) / GST_SECOND;

  rr = (gdouble) (frames_rendered - self->last_frames_rendered) / time_diff;
  dr = (gdouble) (frames_dropped - self->last_frames_dropped) / time_diff;

  average_fps = (gdouble) frames_rendered / time_elapsed;

  if (self->max_fps == -1 || rr > self->max_fps) {
    self->max_fps = rr;
    GST_DEBUG_OBJECT (self, "Updated max-fps to %f", rr);
  }
  if (self->min_fps == -1 || rr < self->min_fps) {
    self->min_fps = rr;
    GST_DEBUG_OBJECT (self, "Updated min-fps to %f", rr);
  }

  if (self->signal_measurements) {
    GST_LOG_OBJECT (self, "Signaling measurements: fps:%f droprate:%f "
        "avg-fps:%f", rr, dr, average_fps);
    g_signal_emit (G_OBJECT (self),
        fpsdisplaysink_signals[SIGNAL_FPS_MEASUREMENTS], 0, rr, dr,
        average_fps);
  }

  /* Display on a single line to make it easier to read and import
   * into, for example, excel..  note: it would be nice to show
   * timestamp too.. need to check if there is a sane way to log
   * timestamp of last rendered buffer, so we could correlate dips
   * in framerate to certain positions in the stream.
   */
  if (dr == 0.0) {
    g_snprintf (fps_message, 255,
        "rendered: %llu\t dropped: %llu\t current: %.2f\t average: %.2f",
        frames_rendered, frames_dropped, rr, average_fps);
  } else {
    g_snprintf (fps_message, 255,
        "rendered: %llu\t dropped: %llu\t fps: %.2f\t drop rate: %.2f",
        frames_rendered, frames_dropped, rr, dr);
  }

  if (self->use_text_overlay) {
    g_object_set (self->text_overlay, "text", fps_message, NULL);
  } else {
    g_print ("%s\n", fps_message);
  }

  self->last_frames_rendered = frames_rendered;
  self->last_frames_dropped = frames_dropped;
  self->last_ts = current_ts;

  return TRUE;
}

static void
fps_display_sink_start (GstFPSDisplaySink * self)
{
  GstPad *target_pad = NULL;

  /* Init counters */
  self->frames_rendered = G_GUINT64_CONSTANT (0);
  self->frames_dropped = G_GUINT64_CONSTANT (0);
  self->max_fps = -1;
  self->min_fps = -1;

  GST_DEBUG_OBJECT (self, "Use text-overlay? %d", self->use_text_overlay);

  if (self->use_text_overlay) {
    if (!self->text_overlay) {
      self->text_overlay =
          gst_element_factory_make ("textoverlay", "fps-display-text-overlay");
      if (!self->text_overlay) {
        GST_WARNING_OBJECT (self, "text-overlay element could not be created");
        self->use_text_overlay = FALSE;
        goto no_text_overlay;
      }
      gst_object_ref (self->text_overlay);
      g_object_set (self->text_overlay,
          "font-desc", DEFAULT_FONT, "silent", FALSE, NULL);
      gst_bin_add (GST_BIN (self), self->text_overlay);

      if (!gst_element_link (self->text_overlay, self->video_sink)) {
        GST_ERROR_OBJECT (self, "Could not link elements");
      }
    }
    target_pad = gst_element_get_static_pad (self->text_overlay, "video_sink");
  }
no_text_overlay:
  if (!self->use_text_overlay) {
    if (self->text_overlay) {
      gst_element_unlink (self->text_overlay, self->video_sink);
      gst_bin_remove (GST_BIN (self), self->text_overlay);
      self->text_overlay = NULL;
    }
    target_pad = gst_element_get_static_pad (self->video_sink, "sink");
  }
  gst_ghost_pad_set_target (GST_GHOST_PAD (self->ghost_pad), target_pad);
  gst_object_unref (target_pad);

  /* Set a timeout for the fps display */
  GST_DEBUG_OBJECT (self, "setting a timeout with a %dms interval",
      self->fps_update_interval);
}

static void
fps_display_sink_stop (GstFPSDisplaySink * self)
{
  /* remove the timeout */
  if (self->timeout_id) {
    g_source_remove (self->timeout_id);
    self->timeout_id = 0;
  }

  if (self->text_overlay) {
    gst_element_unlink (self->text_overlay, self->video_sink);
    gst_bin_remove (GST_BIN (self), self->text_overlay);
    gst_object_unref (self->text_overlay);
    self->text_overlay = NULL;
  } else {
    /* print the max and minimum fps values */
    g_print ("Max-fps: %0.2f\nMin-fps: %0.2f\n", self->max_fps, self->min_fps);
  }
}

static void
fps_display_sink_dispose (GObject * object)
{
  GstFPSDisplaySink *self = GST_FPS_DISPLAY_SINK (object);

  if (self->video_sink) {
    gst_object_unref (self->video_sink);
    self->video_sink = NULL;
  }

  if (self->text_overlay) {
    gst_object_unref (self->text_overlay);
    self->text_overlay = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
fps_display_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstFPSDisplaySink *self = GST_FPS_DISPLAY_SINK (object);

  switch (prop_id) {
    case ARG_SYNC:
      self->sync = g_value_get_boolean (value);
      fps_display_sink_update_sink_sync (self);
      break;
    case ARG_TEXT_OVERLAY:
      self->use_text_overlay = g_value_get_boolean (value);

      if (self->text_overlay) {
        if (!self->use_text_overlay) {
          GST_DEBUG_OBJECT (self, "text-overlay set to false");
          g_object_set (self->text_overlay, "text", "", "silent", TRUE, NULL);
        } else {
          GST_DEBUG_OBJECT (self, "text-overlay set to true");
          g_object_set (self->text_overlay, "silent", FALSE, NULL);
        }
      }
      break;
    case ARG_VIDEO_SINK:
      /* FIXME should we add a state-lock or a lock around here?
       * need to check if it is possible that a state change NULL->READY can
       * happen while this code is executing on a different thread */
      if (GST_STATE (self) != GST_STATE_NULL) {
        g_warning ("Can't set video-sink property of fpsdisplaysink if not on "
            "NULL state");
        break;
      }
      update_video_sink (self, (GstElement *) g_value_get_object (value));
      break;
    case ARG_FPS_UPDATE_INTERVAL:
      self->fps_update_interval = g_value_get_int (value);
      break;
    case ARG_SIGNAL_FPS_MEASUREMENTS:
      self->signal_measurements = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
fps_display_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstFPSDisplaySink *self = GST_FPS_DISPLAY_SINK (object);

  switch (prop_id) {
    case ARG_SYNC:
      g_value_set_boolean (value, self->sync);
      break;
    case ARG_TEXT_OVERLAY:
      g_value_set_boolean (value, self->use_text_overlay);
      break;
    case ARG_VIDEO_SINK:
      g_value_set_object (value, self->video_sink);
      break;
    case ARG_FPS_UPDATE_INTERVAL:
      g_value_set_int (value, self->fps_update_interval);
      break;
    case ARG_MAX_FPS:
      g_value_set_double (value, self->max_fps);
      break;
    case ARG_MIN_FPS:
      g_value_set_double (value, self->min_fps);
      break;
    case ARG_SIGNAL_FPS_MEASUREMENTS:
      g_value_set_boolean (value, self->signal_measurements);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
fps_display_sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstFPSDisplaySink *self = GST_FPS_DISPLAY_SINK (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:

      if (self->video_sink == NULL) {
        GstElement *video_sink;

        GST_DEBUG_OBJECT (self, "No video sink set, creating autovideosink");
        video_sink = gst_element_factory_make ("autovideosink",
            "fps-display-video_sink");
        update_video_sink (self, video_sink);
      }

      if (self->video_sink != NULL) {
        fps_display_sink_start (self);
      } else {
        GST_ELEMENT_ERROR (self, LIBRARY, INIT,
            ("No video sink set and autovideosink is not available"), (NULL));
        ret = GST_STATE_CHANGE_FAILURE;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      /* reinforce our sync to children, as they might have changed
       * internally */
      fps_display_sink_update_sink_sync (self);
      break;
    default:
      break;
  }

  ret = GST_CALL_PARENT_WITH_DEFAULT (GST_ELEMENT_CLASS, change_state,
      (element, transition), GST_STATE_CHANGE_SUCCESS);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      fps_display_sink_stop (self);
      break;
    default:
      break;
  }

  return ret;
}

GType
fps_display_sink_get_type (void)
{
  static GType fps_display_sink_type = 0;

  if (!fps_display_sink_type) {
    static const GTypeInfo fps_display_sink_info = {
      sizeof (GstFPSDisplaySinkClass),
      NULL,
      NULL,
      (GClassInitFunc) fps_display_sink_class_init,
      NULL,
      NULL,
      sizeof (GstFPSDisplaySink),
      0,
      (GInstanceInitFunc) fps_display_sink_init,
    };

    fps_display_sink_type = g_type_register_static (GST_TYPE_BIN,
        "GstFPSDisplaySink", &fps_display_sink_info, 0);

    GST_DEBUG_CATEGORY_INIT (fps_display_sink_debug, "fpsdisplaysink", 0,
        "FPS Display Sink");
  }

  return fps_display_sink_type;
}
