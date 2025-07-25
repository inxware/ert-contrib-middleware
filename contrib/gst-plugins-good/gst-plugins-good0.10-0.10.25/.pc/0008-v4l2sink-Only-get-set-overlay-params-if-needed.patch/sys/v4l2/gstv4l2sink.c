/* GStreamer
 *
 * Copyright (C) 2009 Texas Instruments, Inc - http://www.ti.com/
 *
 * Description: V4L2 sink element
 *  Created on: Jul 2, 2009
 *      Author: Rob Clark <rob@ti.com>
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

/**
 * SECTION:element-v4l2sink
 *
 * v4l2sink can be used to display video to v4l2 devices (screen overlays
 * provided by the graphics hardware, tv-out, etc)
 *
 * <refsect2>
 * <title>Example launch lines</title>
 * |[
 * gst-launch videotestsrc ! v4l2sink device=/dev/video1
 * ]| This pipeline displays a test pattern on /dev/video1
 * </refsect2>
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gstv4l2colorbalance.h"
#if 0                           /* overlay is still not implemented #ifdef HAVE_XVIDEO */
#include "gstv4l2xoverlay.h"
#endif
#include "gstv4l2vidorient.h"

#include "gstv4l2sink.h"
#include "gst/gst-i18n-plugin.h"

#include <string.h>

GST_DEBUG_CATEGORY (v4l2sink_debug);
#define GST_CAT_DEFAULT v4l2sink_debug

#define PROP_DEF_QUEUE_SIZE         8
#define DEFAULT_PROP_DEVICE   "/dev/video1"

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
  PROP_QUEUE_SIZE,
  PROP_OVERLAY_TOP,
  PROP_OVERLAY_LEFT,
  PROP_OVERLAY_WIDTH,
  PROP_OVERLAY_HEIGHT,
};


GST_IMPLEMENT_V4L2_PROBE_METHODS (GstV4l2SinkClass, gst_v4l2sink);
GST_IMPLEMENT_V4L2_COLOR_BALANCE_METHODS (GstV4l2Sink, gst_v4l2sink);
#if 0                           /* overlay is still not implemented #ifdef HAVE_XVIDEO */
GST_IMPLEMENT_V4L2_XOVERLAY_METHODS (GstV4l2Sink, gst_v4l2sink);
#endif
GST_IMPLEMENT_V4L2_VIDORIENT_METHODS (GstV4l2Sink, gst_v4l2sink);

static gboolean
gst_v4l2sink_iface_supported (GstImplementsInterface * iface, GType iface_type)
{
  GstV4l2Object *v4l2object = GST_V4L2SINK (iface)->v4l2object;

#if 0                           /* overlay is still not implemented #ifdef HAVE_XVIDEO */
  g_assert (iface_type == GST_TYPE_X_OVERLAY ||
      iface_type == GST_TYPE_COLOR_BALANCE ||
      iface_type == GST_TYPE_VIDEO_ORIENTATION);
#else
  g_assert (iface_type == GST_TYPE_COLOR_BALANCE ||
      iface_type == GST_TYPE_VIDEO_ORIENTATION);
#endif

  if (v4l2object->video_fd == -1)
    return FALSE;

#if 0                           /* overlay is still not implemented #ifdef HAVE_XVIDEO */
  if (iface_type == GST_TYPE_X_OVERLAY && !GST_V4L2_IS_OVERLAY (v4l2object))
    return FALSE;
#endif

  return TRUE;
}

static void
gst_v4l2sink_interface_init (GstImplementsInterfaceClass * klass)
{
  /*
   * default virtual functions
   */
  klass->supported = gst_v4l2sink_iface_supported;
}

static void
gst_v4l2sink_init_interfaces (GType type)
{
  static const GInterfaceInfo v4l2iface_info = {
    (GInterfaceInitFunc) gst_v4l2sink_interface_init,
    NULL,
    NULL,
  };
#if 0                           /* overlay is still not implemented #ifdef HAVE_XVIDEO */
  static const GInterfaceInfo v4l2_xoverlay_info = {
    (GInterfaceInitFunc) gst_v4l2sink_xoverlay_interface_init,
    NULL,
    NULL,
  };
#endif
  static const GInterfaceInfo v4l2_colorbalance_info = {
    (GInterfaceInitFunc) gst_v4l2sink_color_balance_interface_init,
    NULL,
    NULL,
  };
  static const GInterfaceInfo v4l2_videoorientation_info = {
    (GInterfaceInitFunc) gst_v4l2sink_video_orientation_interface_init,
    NULL,
    NULL,
  };
  static const GInterfaceInfo v4l2_propertyprobe_info = {
    (GInterfaceInitFunc) gst_v4l2sink_property_probe_interface_init,
    NULL,
    NULL,
  };

  g_type_add_interface_static (type,
      GST_TYPE_IMPLEMENTS_INTERFACE, &v4l2iface_info);
#if 0                           /* overlay is still not implemented #ifdef HAVE_XVIDEO */
  g_type_add_interface_static (type, GST_TYPE_X_OVERLAY, &v4l2_xoverlay_info);
#endif
  g_type_add_interface_static (type,
      GST_TYPE_COLOR_BALANCE, &v4l2_colorbalance_info);
  g_type_add_interface_static (type,
      GST_TYPE_VIDEO_ORIENTATION, &v4l2_videoorientation_info);
  g_type_add_interface_static (type, GST_TYPE_PROPERTY_PROBE,
      &v4l2_propertyprobe_info);
}


GST_BOILERPLATE_FULL (GstV4l2Sink, gst_v4l2sink, GstVideoSink,
    GST_TYPE_VIDEO_SINK, gst_v4l2sink_init_interfaces);


static void gst_v4l2sink_dispose (GObject * object);
static void gst_v4l2sink_finalize (GstV4l2Sink * v4l2sink);

/* GObject methods: */
static void gst_v4l2sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_v4l2sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);


/* GstElement methods: */
static GstStateChangeReturn gst_v4l2sink_change_state (GstElement * element,
    GstStateChange transition);

/* GstBaseSink methods: */
static GstCaps *gst_v4l2sink_get_caps (GstBaseSink * bsink);
static gboolean gst_v4l2sink_set_caps (GstBaseSink * bsink, GstCaps * caps);
static GstFlowReturn gst_v4l2sink_buffer_alloc (GstBaseSink * bsink,
    guint64 offset, guint size, GstCaps * caps, GstBuffer ** buf);
static GstFlowReturn gst_v4l2sink_show_frame (GstBaseSink * bsink,
    GstBuffer * buf);


static void
gst_v4l2sink_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);
  GstV4l2SinkClass *gstv4l2sink_class = GST_V4L2SINK_CLASS (g_class);

  gstv4l2sink_class->v4l2_class_devices = NULL;

  GST_DEBUG_CATEGORY_INIT (v4l2sink_debug, "v4l2sink", 0, "V4L2 sink element");

  gst_element_class_set_details_simple (gstelement_class,
      "Video (video4linux2) Sink", "Sink/Video",
      "Displays frames on a video4linux2 device", "Rob Clark <rob@ti.com>,");

  gst_element_class_add_pad_template
      (gstelement_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_v4l2_object_get_all_caps ()));
}

static void
gst_v4l2sink_class_init (GstV4l2SinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseSinkClass *basesink_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  basesink_class = GST_BASE_SINK_CLASS (klass);

  gobject_class->dispose = gst_v4l2sink_dispose;
  gobject_class->finalize = (GObjectFinalizeFunc) gst_v4l2sink_finalize;
  gobject_class->set_property = gst_v4l2sink_set_property;
  gobject_class->get_property = gst_v4l2sink_get_property;

  element_class->change_state = gst_v4l2sink_change_state;

  gst_v4l2_object_install_properties_helper (gobject_class,
      DEFAULT_PROP_DEVICE);
  g_object_class_install_property (gobject_class, PROP_QUEUE_SIZE,
      g_param_spec_uint ("queue-size", "Queue size",
          "Number of buffers to be enqueud in the driver in streaming mode",
          GST_V4L2_MIN_BUFFERS, GST_V4L2_MAX_BUFFERS, PROP_DEF_QUEUE_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_TOP,
      g_param_spec_int ("overlay-top", "Overlay top",
          "The topmost (y) coordinate of the video overlay; top left corner of screen is 0,0",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_LEFT,
      g_param_spec_int ("overlay-left", "Overlay left",
          "The leftmost (x) coordinate of the video overlay; top left corner of screen is 0,0",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_WIDTH,
      g_param_spec_uint ("overlay-width", "Overlay width",
          "The width of the video overlay; default is equal to negotiated image width",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_HEIGHT,
      g_param_spec_uint ("overlay-height", "Overlay height",
          "The height of the video overlay; default is equal to negotiated image height",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  basesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_v4l2sink_get_caps);
  basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_v4l2sink_set_caps);
  basesink_class->buffer_alloc = GST_DEBUG_FUNCPTR (gst_v4l2sink_buffer_alloc);
  basesink_class->preroll = GST_DEBUG_FUNCPTR (gst_v4l2sink_show_frame);
  basesink_class->render = GST_DEBUG_FUNCPTR (gst_v4l2sink_show_frame);
}

static void
gst_v4l2sink_init (GstV4l2Sink * v4l2sink, GstV4l2SinkClass * klass)
{
  v4l2sink->v4l2object = gst_v4l2_object_new (GST_ELEMENT (v4l2sink),
      V4L2_BUF_TYPE_VIDEO_OUTPUT, DEFAULT_PROP_DEVICE,
      gst_v4l2_get_input, gst_v4l2_set_input, NULL);

  /* same default value for video output device as is used for
   * v4l2src/capture is no good..  so lets set a saner default
   * (which can be overridden by the one creating the v4l2sink
   * after the constructor returns)
   */
  g_object_set (v4l2sink, "device", "/dev/video1", NULL);

  /* number of buffers requested */
  v4l2sink->num_buffers = PROP_DEF_QUEUE_SIZE;

  v4l2sink->probed_caps = NULL;
  v4l2sink->current_caps = NULL;

  v4l2sink->overlay_fields_set = 0;
  v4l2sink->state = 0;
}


static void
gst_v4l2sink_dispose (GObject * object)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (object);

  if (v4l2sink->probed_caps) {
    gst_caps_unref (v4l2sink->probed_caps);
  }

  if (v4l2sink->current_caps) {
    gst_caps_unref (v4l2sink->current_caps);
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
gst_v4l2sink_finalize (GstV4l2Sink * v4l2sink)
{
  gst_v4l2_object_destroy (v4l2sink->v4l2object);

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (v4l2sink));
}


/*
 * State values
 */
enum
{
  STATE_OFF = 0,
  STATE_PENDING_STREAMON,
  STATE_STREAMING
};

/*
 * flags to indicate which overlay properties the user has set (and therefore
 * which ones should override the defaults from the driver)
 */
enum
{
  OVERLAY_TOP_SET = 0x01,
  OVERLAY_LEFT_SET = 0x02,
  OVERLAY_WIDTH_SET = 0x04,
  OVERLAY_HEIGHT_SET = 0x08
};

static void
gst_v4l2sink_sync_overlay_fields (GstV4l2Sink * v4l2sink)
{
  if (GST_V4L2_IS_OPEN (v4l2sink->v4l2object)) {

    gint fd = v4l2sink->v4l2object->video_fd;
    struct v4l2_format format;

    memset (&format, 0x00, sizeof (struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;

    g_return_if_fail (v4l2_ioctl (fd, VIDIOC_G_FMT, &format) >= 0);

    if (v4l2sink->overlay_fields_set) {
      if (v4l2sink->overlay_fields_set & OVERLAY_TOP_SET)
        format.fmt.win.w.top = v4l2sink->overlay.top;
      if (v4l2sink->overlay_fields_set & OVERLAY_LEFT_SET)
        format.fmt.win.w.left = v4l2sink->overlay.left;
      if (v4l2sink->overlay_fields_set & OVERLAY_WIDTH_SET)
        format.fmt.win.w.width = v4l2sink->overlay.width;
      if (v4l2sink->overlay_fields_set & OVERLAY_HEIGHT_SET)
        format.fmt.win.w.height = v4l2sink->overlay.height;

      g_return_if_fail (v4l2_ioctl (fd, VIDIOC_S_FMT, &format) >= 0);
      v4l2sink->overlay_fields_set = 0;
    }

    v4l2sink->overlay = format.fmt.win.w;
  }
}


static void
gst_v4l2sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (object);

  if (!gst_v4l2_object_set_property_helper (v4l2sink->v4l2object,
          prop_id, value, pspec)) {
    switch (prop_id) {
      case PROP_QUEUE_SIZE:
        v4l2sink->num_buffers = g_value_get_uint (value);
        break;
      case PROP_OVERLAY_TOP:
        v4l2sink->overlay.top = g_value_get_int (value);
        v4l2sink->overlay_fields_set |= OVERLAY_TOP_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      case PROP_OVERLAY_LEFT:
        v4l2sink->overlay.left = g_value_get_int (value);
        v4l2sink->overlay_fields_set |= OVERLAY_LEFT_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      case PROP_OVERLAY_WIDTH:
        v4l2sink->overlay.width = g_value_get_uint (value);
        v4l2sink->overlay_fields_set |= OVERLAY_WIDTH_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      case PROP_OVERLAY_HEIGHT:
        v4l2sink->overlay.height = g_value_get_uint (value);
        v4l2sink->overlay_fields_set |= OVERLAY_HEIGHT_SET;
        gst_v4l2sink_sync_overlay_fields (v4l2sink);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
  }
}


static void
gst_v4l2sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (object);

  if (!gst_v4l2_object_get_property_helper (v4l2sink->v4l2object,
          prop_id, value, pspec)) {
    switch (prop_id) {
      case PROP_QUEUE_SIZE:
        g_value_set_uint (value, v4l2sink->num_buffers);
        break;
      case PROP_OVERLAY_TOP:
        g_value_set_int (value, v4l2sink->overlay.top);
        break;
      case PROP_OVERLAY_LEFT:
        g_value_set_int (value, v4l2sink->overlay.left);
        break;
      case PROP_OVERLAY_WIDTH:
        g_value_set_uint (value, v4l2sink->overlay.width);
        break;
      case PROP_OVERLAY_HEIGHT:
        g_value_set_uint (value, v4l2sink->overlay.height);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
  }
}

static GstStateChangeReturn
gst_v4l2sink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (element);

  GST_DEBUG_OBJECT (v4l2sink, "%d -> %d",
      GST_STATE_TRANSITION_CURRENT (transition),
      GST_STATE_TRANSITION_NEXT (transition));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      /* open the device */
      if (!gst_v4l2_object_start (v4l2sink->v4l2object))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (v4l2sink->state == STATE_STREAMING) {
        if (!gst_v4l2_object_stop_streaming (v4l2sink->v4l2object)) {
          return GST_STATE_CHANGE_FAILURE;
        }
        v4l2sink->state = STATE_OFF;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      if (NULL != v4l2sink->pool)
        gst_v4l2_buffer_pool_destroy (v4l2sink->pool);
      v4l2sink->pool = NULL;
      /* close the device */
      if (!gst_v4l2_object_stop (v4l2sink->v4l2object))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  return ret;
}


static GstCaps *
gst_v4l2sink_get_caps (GstBaseSink * bsink)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (bsink);
  GstCaps *ret;
  GSList *walk;
  GSList *formats;

  if (!GST_V4L2_IS_OPEN (v4l2sink->v4l2object)) {
    /* FIXME: copy? */
    GST_DEBUG_OBJECT (v4l2sink, "device is not open");
    return
        gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SINK_PAD
            (v4l2sink)));
  }

  if (v4l2sink->probed_caps) {
    LOG_CAPS (v4l2sink, v4l2sink->probed_caps);
    return gst_caps_ref (v4l2sink->probed_caps);
  }

  formats = gst_v4l2_object_get_format_list (v4l2sink->v4l2object);

  ret = gst_caps_new_empty ();

  for (walk = v4l2sink->v4l2object->formats; walk; walk = walk->next) {
    struct v4l2_fmtdesc *format;

    GstStructure *template;

    format = (struct v4l2_fmtdesc *) walk->data;

    template = gst_v4l2_object_v4l2fourcc_to_structure (format->pixelformat);

    if (template) {
      GstCaps *tmp;

      tmp =
          gst_v4l2_object_probe_caps_for_format (v4l2sink->v4l2object,
          format->pixelformat, template);
      if (tmp)
        gst_caps_append (ret, tmp);

      gst_structure_free (template);
    } else {
      GST_DEBUG_OBJECT (v4l2sink, "unknown format %u", format->pixelformat);
    }
  }

  v4l2sink->probed_caps = gst_caps_ref (ret);

  GST_INFO_OBJECT (v4l2sink, "probed caps: %p", ret);
  LOG_CAPS (v4l2sink, ret);

  return ret;
}

static gboolean
gst_v4l2sink_set_caps (GstBaseSink * bsink, GstCaps * caps)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (bsink);
  gint w = 0, h = 0;
  struct v4l2_fmtdesc *format;
  guint fps_n, fps_d;
  guint size;

  LOG_CAPS (v4l2sink, caps);

  if (!GST_V4L2_IS_OPEN (v4l2sink->v4l2object)) {
    GST_DEBUG_OBJECT (v4l2sink, "device is not open");
    return FALSE;
  }

  if (v4l2sink->current_caps) {
    GST_DEBUG_OBJECT (v4l2sink, "already have caps set.. are they equal?");
    LOG_CAPS (v4l2sink, v4l2sink->current_caps);
    if (gst_caps_is_equal (v4l2sink->current_caps, caps)) {
      GST_DEBUG_OBJECT (v4l2sink, "yes they are!");
      return TRUE;
    }
    GST_DEBUG_OBJECT (v4l2sink, "no they aren't!");
  }

  if (v4l2sink->pool) {
    /* TODO: if we've already allocated buffers, we probably need to
     * do something here to free and reallocate....
     *
     *   gst_v4l2_object_stop_streaming()
     *   gst_v4l2_buffer_pool_destroy()
     *
     */
    GST_DEBUG_OBJECT (v4l2sink, "warning, changing caps not supported yet");
    return FALSE;
  }

  /* we want our own v4l2 type of fourcc codes */
  if (!gst_v4l2_object_get_caps_info (v4l2sink->v4l2object, caps,
          &format, &w, &h, &fps_n, &fps_d, &size)) {
    GST_DEBUG_OBJECT (v4l2sink, "can't get capture format from caps %p", caps);
    return FALSE;
  }

  if (!format) {
    GST_DEBUG_OBJECT (v4l2sink, "unrecognized caps!!");
    return FALSE;
  }

  if (!gst_v4l2_object_set_format (v4l2sink->v4l2object, format->pixelformat, w,
          h)) {
    /* error already posted */
    return FALSE;
  }

  gst_v4l2sink_sync_overlay_fields (v4l2sink);

  v4l2sink->current_caps = gst_caps_ref (caps);

  return TRUE;
}

/* buffer alloc function to implement pad_alloc for upstream element */
static GstFlowReturn
gst_v4l2sink_buffer_alloc (GstBaseSink * bsink, guint64 offset, guint size,
    GstCaps * caps, GstBuffer ** buf)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (bsink);
  GstV4l2Buffer *v4l2buf;

  if (v4l2sink->v4l2object->vcap.capabilities & V4L2_CAP_STREAMING) {

    /* initialize the buffer pool if not initialized yet (first buffer): */
    if (G_UNLIKELY (!v4l2sink->pool)) {

      /* set_caps() might not be called yet.. so just to make sure: */
      if (!gst_v4l2sink_set_caps (bsink, caps)) {
        return GST_FLOW_ERROR;
      }

      GST_V4L2_CHECK_OPEN (v4l2sink->v4l2object);

      if (!(v4l2sink->pool = gst_v4l2_buffer_pool_new (GST_ELEMENT (v4l2sink),
                  v4l2sink->v4l2object->video_fd,
                  v4l2sink->num_buffers, caps, FALSE,
                  V4L2_BUF_TYPE_VIDEO_OUTPUT))) {
        return GST_FLOW_ERROR;
      }
      v4l2sink->state = STATE_PENDING_STREAMON;

      GST_INFO_OBJECT (v4l2sink, "outputting buffers via mmap()");

      if (v4l2sink->num_buffers != v4l2sink->pool->buffer_count) {
        v4l2sink->num_buffers = v4l2sink->pool->buffer_count;
        g_object_notify (G_OBJECT (v4l2sink), "queue-size");
      }
    }

    v4l2buf = gst_v4l2_buffer_pool_get (v4l2sink->pool);

    if (G_LIKELY (v4l2buf)) {
      GST_DEBUG_OBJECT (v4l2sink, "allocated buffer: %p", v4l2buf);
      *buf = GST_BUFFER (v4l2buf);
      return GST_FLOW_OK;
    } else {
      GST_DEBUG_OBJECT (v4l2sink, "failed to allocate buffer");
      return GST_FLOW_ERROR;
    }

  } else {
    GST_ERROR_OBJECT (v4l2sink, "only supporting streaming mode for now...");
    return GST_FLOW_ERROR;
  }
}

/* called after A/V sync to render frame */
static GstFlowReturn
gst_v4l2sink_show_frame (GstBaseSink * bsink, GstBuffer * buf)
{
  GstV4l2Sink *v4l2sink = GST_V4L2SINK (bsink);
  GstBuffer *newbuf = NULL;

  GST_DEBUG_OBJECT (v4l2sink, "render buffer: %p", buf);

  if (!GST_IS_V4L2_BUFFER (buf)) {
    GstFlowReturn ret;

    GST_DEBUG_OBJECT (v4l2sink, "slow-path.. I got a %s so I need to memcpy",
        g_type_name (G_OBJECT_TYPE (buf)));

    ret = gst_v4l2sink_buffer_alloc (bsink,
        GST_BUFFER_OFFSET (buf), GST_BUFFER_SIZE (buf), GST_BUFFER_CAPS (buf),
        &newbuf);

    if (GST_FLOW_OK != ret) {
      return ret;
    }

    memcpy (GST_BUFFER_DATA (newbuf),
        GST_BUFFER_DATA (buf),
        MIN (GST_BUFFER_SIZE (newbuf), GST_BUFFER_SIZE (buf)));

    GST_DEBUG_OBJECT (v4l2sink, "render copied buffer: %p", newbuf);

    buf = newbuf;
  }

  if (!gst_v4l2_buffer_pool_qbuf (v4l2sink->pool, GST_V4L2_BUFFER (buf))) {
    return GST_FLOW_ERROR;
  }
  if (v4l2sink->state == STATE_PENDING_STREAMON) {
    if (!gst_v4l2_object_start_streaming (v4l2sink->v4l2object)) {
      return GST_FLOW_ERROR;
    }
    v4l2sink->state = STATE_STREAMING;
  }

  if (!newbuf) {
    gst_buffer_ref (buf);
  }

  /* if the driver has more than one buffer, ie. more than just the one we
   * just queued, then dequeue one immediately to make it available via
   * _buffer_alloc():
   */
  if (gst_v4l2_buffer_pool_available_buffers (v4l2sink->pool) > 1) {
    GstV4l2Buffer *v4l2buf = gst_v4l2_buffer_pool_dqbuf (v4l2sink->pool);

    /* note: if we get a buf, we don't want to use it directly (because
     * someone else could still hold a ref).. but instead we release our
     * reference to it, and if no one else holds a ref it will be returned
     * to the pool of available buffers..  and if not, we keep looping.
     */
    if (v4l2buf) {
      gst_buffer_unref (GST_BUFFER (v4l2buf));
    }
  }

  return GST_FLOW_OK;
}
