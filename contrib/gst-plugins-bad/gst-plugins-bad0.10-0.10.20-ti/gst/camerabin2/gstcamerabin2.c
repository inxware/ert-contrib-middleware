/* GStreamer
 * Copyright (C) 2010 Thiago Santos <thiago.sousa.santos@collabora.co.uk>
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
 * SECTION:element-gstcamerabin2
 *
 * The gstcamerabin2 element does FIXME stuff.
 *
 * Note that camerabin2 is still UNSTABLE, EXPERIMENTAL and under heavy
 * development.
 */

/*
 * Detail Topics:
 *
 * videorecordingbin state management (for now on called 'videobin')
 * - The problem: keeping videobin state in sync with camerabin will make it
 *                go to playing when it might not be used, causing its internal
 *                filesink to open a file that might be left blank.
 * - The solution: videobin state is set to locked upon its creation and camerabin
 *                 registers itself on the notify::ready-for-capture of the src.
 *                 Whenever the src readyness goes to FALSE it means a new
 *                 capture is starting. If we are on video mode, the videobin's
 *                 state is set to NULL and then PLAYING (in between this we
 *                 have room to set the destination filename).
 *                 There is no problem to leave it on playing after an EOS, so
 *                 no action is taken on stop-capture.
 *
 * - TODO: What happens when an error pops?
 * - TODO: Should we split properties in image/video variants? We already do so
 *         for some of them
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gst/basecamerabinsrc/gstbasecamerasrc.h>
#include "gstcamerabin2.h"

#define GST_CAMERA_BIN_PROCESSING_INC(c)                                \
{                                                                       \
  gint bef = g_atomic_int_exchange_and_add (&c->processing_counter, 1); \
  if (bef == 0)                                                         \
    g_object_notify (G_OBJECT (c), "idle");                             \
  GST_DEBUG_OBJECT ((c), "Processing counter incremented to: %d",       \
      bef + 1);                                                         \
}

#define GST_CAMERA_BIN_PROCESSING_DEC(c)                                \
{                                                                       \
  if (g_atomic_int_dec_and_test (&c->processing_counter))               \
    g_object_notify (G_OBJECT (c), "idle");                             \
  GST_DEBUG_OBJECT ((c), "Processing counter decremented");             \
}

#define GST_CAMERA_BIN_RESET_PROCESSING_COUNTER(c)                      \
{                                                                       \
  g_atomic_int_set (&c->processing_counter, 0);                         \
  GST_DEBUG_OBJECT ((c), "Processing counter reset");                   \
}

GST_DEBUG_CATEGORY_STATIC (gst_camera_bin_debug);
#define GST_CAT_DEFAULT gst_camera_bin_debug

/* prototypes */

enum
{
  PROP_0,
  PROP_MODE,
  PROP_LOCATION,
  PROP_CAMERA_SRC,
  PROP_IMAGE_CAPTURE_SUPPORTED_CAPS,
  PROP_VIDEO_CAPTURE_SUPPORTED_CAPS,
  PROP_IMAGE_CAPTURE_CAPS,
  PROP_VIDEO_CAPTURE_CAPS,
  PROP_POST_PREVIEWS,
  PROP_PREVIEW_CAPS,
  PROP_VIDEO_ENCODING_PROFILE,
  PROP_IMAGE_FILTER,
  PROP_VIDEO_FILTER,
  PROP_VIEWFINDER_FILTER,
  PROP_PREVIEW_FILTER,
  PROP_VIEWFINDER_SINK,
  PROP_VIEWFINDER_SUPPORTED_CAPS,
  PROP_VIEWFINDER_CAPS,
  PROP_AUDIO_SRC,
  PROP_MUTE_AUDIO,
  PROP_AUDIO_CAPTURE_SUPPORTED_CAPS,
  PROP_AUDIO_CAPTURE_CAPS,
  PROP_ZOOM,
  PROP_MAX_ZOOM,
  PROP_IMAGE_ENCODING_PROFILE,
  PROP_IDLE
};

enum
{
  /* action signals */
  START_CAPTURE_SIGNAL,
  STOP_CAPTURE_SIGNAL,
  /* emit signals */
  LAST_SIGNAL
};
static guint camerabin_signals[LAST_SIGNAL];

#define DEFAULT_MODE MODE_IMAGE
#define DEFAULT_VID_LOCATION "vid_%d"
#define DEFAULT_IMG_LOCATION "img_%d"
#define DEFAULT_POST_PREVIEWS TRUE
#define DEFAULT_MUTE_AUDIO FALSE
#define DEFAULT_IDLE TRUE

#define DEFAULT_AUDIO_SRC "autoaudiosrc"

/********************************
 * Standard GObject boilerplate *
 * and GObject types            *
 ********************************/

static GstPipelineClass *parent_class;
static void gst_camera_bin_class_init (GstCameraBinClass * klass);
static void gst_camera_bin_base_init (gpointer klass);
static void gst_camera_bin_init (GstCameraBin * camera);
static void gst_camera_bin_dispose (GObject * object);
static void gst_camera_bin_finalize (GObject * object);

static void gst_camera_bin_handle_message (GstBin * bin, GstMessage * message);
static gboolean gst_camera_bin_send_event (GstElement * element,
    GstEvent * event);

GType
gst_camera_bin_get_type (void)
{
  static GType gst_camera_bin_type = 0;
  static const GInterfaceInfo camerabin_tagsetter_info = {
    NULL,
    NULL,
    NULL,
  };

  if (!gst_camera_bin_type) {
    static const GTypeInfo gst_camera_bin_info = {
      sizeof (GstCameraBinClass),
      (GBaseInitFunc) gst_camera_bin_base_init,
      NULL,
      (GClassInitFunc) gst_camera_bin_class_init,
      NULL,
      NULL,
      sizeof (GstCameraBin),
      0,
      (GInstanceInitFunc) gst_camera_bin_init,
      NULL
    };

    gst_camera_bin_type =
        g_type_register_static (GST_TYPE_PIPELINE, "GstCameraBin2",
        &gst_camera_bin_info, 0);

    g_type_add_interface_static (gst_camera_bin_type, GST_TYPE_TAG_SETTER,
        &camerabin_tagsetter_info);
  }

  return gst_camera_bin_type;
}

/* GObject class functions */
static void gst_camera_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_camera_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

/* Element class functions */
static GstStateChangeReturn
gst_camera_bin_change_state (GstElement * element, GstStateChange trans);


/* Camerabin functions */

static GstEvent *
gst_camera_bin_new_event_renegotiate (void)
{
  return gst_event_new_custom (GST_EVENT_CUSTOM_BOTH,
      gst_structure_new ("renegotiate", NULL));
}

static void
gst_camera_bin_start_capture (GstCameraBin * camerabin)
{
  const GstTagList *taglist;

  GST_DEBUG_OBJECT (camerabin, "Received start-capture");
  GST_CAMERA_BIN_PROCESSING_INC (camerabin);

  taglist = gst_tag_setter_get_tag_list (GST_TAG_SETTER (camerabin));
  if (taglist) {
    GstPad *active_pad;

    GST_DEBUG_OBJECT (camerabin, "Pushing tags from application: %"
        GST_PTR_FORMAT, taglist);

    if (camerabin->mode == MODE_IMAGE) {
      active_pad = gst_element_get_static_pad (camerabin->src,
          GST_BASE_CAMERA_SRC_IMAGE_PAD_NAME);
    } else {
      active_pad = gst_element_get_static_pad (camerabin->src,
          GST_BASE_CAMERA_SRC_VIDEO_PAD_NAME);
    }

    gst_pad_push_event (active_pad,
        gst_event_new_tag (gst_tag_list_copy (taglist)));
    gst_object_unref (active_pad);
  }

  if (camerabin->mode == MODE_VIDEO && camerabin->audio_src) {
    gst_element_set_state (camerabin->audio_src, GST_STATE_READY);
    /* need to reset eos status (pads could be flushing) */
    gst_element_set_state (camerabin->audio_queue, GST_STATE_READY);
    gst_element_set_state (camerabin->audio_convert, GST_STATE_READY);
    gst_element_set_state (camerabin->audio_capsfilter, GST_STATE_READY);
    gst_element_set_state (camerabin->audio_volume, GST_STATE_READY);

    gst_element_sync_state_with_parent (camerabin->audio_queue);
    gst_element_sync_state_with_parent (camerabin->audio_convert);
    gst_element_sync_state_with_parent (camerabin->audio_capsfilter);
    gst_element_sync_state_with_parent (camerabin->audio_volume);
  }

  g_signal_emit_by_name (camerabin->src, "start-capture", NULL);
  if (camerabin->mode == MODE_VIDEO && camerabin->audio_src)
    gst_element_set_state (camerabin->audio_src, GST_STATE_PLAYING);
}

static void
gst_camera_bin_stop_capture (GstCameraBin * camerabin)
{
  GST_DEBUG_OBJECT (camerabin, "Received stop-capture");
  if (camerabin->src)
    g_signal_emit_by_name (camerabin->src, "stop-capture", NULL);

  if (camerabin->mode == MODE_VIDEO && camerabin->audio_src) {
    gst_element_send_event (camerabin->audio_src, gst_event_new_eos ());
  }
}

static void
gst_camera_bin_change_mode (GstCameraBin * camerabin, gint mode)
{
  if (mode == camerabin->mode)
    return;

  GST_DEBUG_OBJECT (camerabin, "Changing mode to %d", mode);

  /* stop any ongoing capture */
  gst_camera_bin_stop_capture (camerabin);
  camerabin->mode = mode;
  if (camerabin->src)
    g_object_set (camerabin->src, "mode", mode, NULL);
}

static void
gst_camera_bin_src_notify_readyforcapture (GObject * obj, GParamSpec * pspec,
    gpointer user_data)
{
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (user_data);
  gboolean ready;

  g_object_get (camera->src, "ready-for-capture", &ready, NULL);
  if (!ready) {
    gchar *location = NULL;

    if (camera->mode == MODE_VIDEO) {
      /* a video recording is about to start, we reset the videobin to clear eos/flushing state
       * also need to clean the queue ! capsfilter before it */
      gst_element_set_state (camera->video_encodebin, GST_STATE_NULL);
      gst_element_set_state (camera->videosink, GST_STATE_NULL);
      gst_element_set_state (camera->videobin_queue, GST_STATE_NULL);
      gst_element_set_state (camera->videobin_capsfilter, GST_STATE_NULL);
      location =
          g_strdup_printf (camera->video_location, camera->video_index++);
      GST_DEBUG_OBJECT (camera, "Switching videobin location to %s", location);
      g_object_set (camera->videosink, "location", location, NULL);
      g_free (location);
      gst_element_set_state (camera->video_encodebin, GST_STATE_PLAYING);
      gst_element_set_state (camera->videosink, GST_STATE_PLAYING);
      gst_element_set_state (camera->videobin_capsfilter, GST_STATE_PLAYING);
      gst_element_set_state (camera->videobin_queue, GST_STATE_PLAYING);
    } else if (camera->mode == MODE_IMAGE) {
      gst_element_set_state (camera->image_encodebin, GST_STATE_NULL);
      gst_element_set_state (camera->imagesink, GST_STATE_NULL);
      gst_element_set_state (camera->imagebin_queue, GST_STATE_NULL);
      gst_element_set_state (camera->imagebin_capsfilter, GST_STATE_NULL);
      GST_DEBUG_OBJECT (camera, "Switching imagebin location to %s", location);
      g_object_set (camera->imagesink, "location", camera->image_location,
          NULL);
      gst_element_set_state (camera->image_encodebin, GST_STATE_PLAYING);
      gst_element_set_state (camera->imagesink, GST_STATE_PLAYING);
      gst_element_set_state (camera->imagebin_capsfilter, GST_STATE_PLAYING);
      gst_element_set_state (camera->imagebin_queue, GST_STATE_PLAYING);
    }

  }
}

static void
gst_camera_bin_dispose (GObject * object)
{
  GstCameraBin *camerabin = GST_CAMERA_BIN_CAST (object);

  g_free (camerabin->image_location);
  g_free (camerabin->video_location);

  if (camerabin->src_capture_notify_id)
    g_signal_handler_disconnect (camerabin->src,
        camerabin->src_capture_notify_id);
  if (camerabin->src)
    gst_object_unref (camerabin->src);
  if (camerabin->user_src)
    gst_object_unref (camerabin->user_src);

  if (camerabin->audio_src)
    gst_object_unref (camerabin->audio_src);
  if (camerabin->user_audio_src)
    gst_object_unref (camerabin->user_audio_src);

  if (camerabin->audio_capsfilter)
    gst_object_unref (camerabin->audio_capsfilter);
  if (camerabin->audio_queue)
    gst_object_unref (camerabin->audio_queue);
  if (camerabin->audio_convert)
    gst_object_unref (camerabin->audio_convert);
  if (camerabin->audio_volume)
    gst_object_unref (camerabin->audio_volume);

  if (camerabin->viewfinderbin)
    gst_object_unref (camerabin->viewfinderbin);
  if (camerabin->viewfinderbin_queue)
    gst_object_unref (camerabin->viewfinderbin_queue);
  if (camerabin->viewfinderbin_capsfilter)
    gst_object_unref (camerabin->viewfinderbin_capsfilter);

  if (camerabin->video_encodebin_signal_id)
    g_signal_handler_disconnect (camerabin->video_encodebin,
        camerabin->video_encodebin_signal_id);

  if (camerabin->videosink_probe) {
    GstPad *pad = gst_element_get_static_pad (camerabin->videosink, "sink");
    gst_pad_remove_data_probe (pad, camerabin->videosink_probe);
    gst_object_unref (pad);
  }

  if (camerabin->videosink)
    gst_object_unref (camerabin->videosink);
  if (camerabin->video_encodebin)
    gst_object_unref (camerabin->video_encodebin);
  if (camerabin->videobin_queue)
    gst_object_unref (camerabin->videobin_queue);
  if (camerabin->videobin_capsfilter)
    gst_object_unref (camerabin->videobin_capsfilter);

  if (camerabin->image_encodebin_signal_id)
    g_signal_handler_disconnect (camerabin->image_encodebin,
        camerabin->image_encodebin_signal_id);
  if (camerabin->imagesink)
    gst_object_unref (camerabin->imagesink);
  if (camerabin->image_encodebin)
    gst_object_unref (camerabin->image_encodebin);
  if (camerabin->imagebin_queue)
    gst_object_unref (camerabin->imagebin_queue);
  if (camerabin->imagebin_capsfilter)
    gst_object_unref (camerabin->imagebin_capsfilter);

  if (camerabin->video_filter)
    gst_object_unref (camerabin->video_filter);
  if (camerabin->image_filter)
    gst_object_unref (camerabin->image_filter);
  if (camerabin->viewfinder_filter)
    gst_object_unref (camerabin->viewfinder_filter);

  if (camerabin->user_video_filter)
    gst_object_unref (camerabin->user_video_filter);
  if (camerabin->user_image_filter)
    gst_object_unref (camerabin->user_image_filter);
  if (camerabin->user_viewfinder_filter)
    gst_object_unref (camerabin->user_viewfinder_filter);

  if (camerabin->video_profile)
    gst_encoding_profile_unref (camerabin->video_profile);
  if (camerabin->image_profile)
    gst_encoding_profile_unref (camerabin->image_profile);

  if (camerabin->preview_caps)
    gst_caps_replace (&camerabin->preview_caps, NULL);
  if (camerabin->preview_filter) {
    gst_object_unref (camerabin->preview_filter);
    camerabin->preview_filter = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_camera_bin_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_camera_bin_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class, "CameraBin2",
      "Generic/Bin/Camera", "CameraBin2",
      "Thiago Santos <thiago.sousa.santos@collabora.co.uk>");
}

static void
gst_camera_bin_class_init (GstCameraBinClass * klass)
{
  GObjectClass *object_class;
  GstElementClass *element_class;
  GstBinClass *bin_class;

  parent_class = g_type_class_peek_parent (klass);
  object_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  bin_class = GST_BIN_CLASS (klass);

  object_class->dispose = gst_camera_bin_dispose;
  object_class->finalize = gst_camera_bin_finalize;
  object_class->set_property = gst_camera_bin_set_property;
  object_class->get_property = gst_camera_bin_get_property;

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_camera_bin_change_state);
  element_class->send_event = GST_DEBUG_FUNCPTR (gst_camera_bin_send_event);

  bin_class->handle_message = gst_camera_bin_handle_message;

  klass->start_capture = gst_camera_bin_start_capture;
  klass->stop_capture = gst_camera_bin_stop_capture;

  /**
   * GstCameraBin:mode:
   *
   * Set the mode of operation: still image capturing or video recording.
   */
  g_object_class_install_property (object_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode",
          "The capture mode (still image capture or video recording)",
          GST_TYPE_CAMERABIN_MODE, DEFAULT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location to save the captured files. A %d might be used on the"
          "filename as a placeholder for a numeric index of the capture."
          "Default for images is img_%d and vid_%d for videos",
          DEFAULT_IMG_LOCATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_CAMERA_SRC,
      g_param_spec_object ("camera-src", "Camera source",
          "The camera source element to be used",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_AUDIO_SRC,
      g_param_spec_object ("audio-src", "Audio source",
          "The audio source element to be used on video recordings",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_MUTE_AUDIO,
      g_param_spec_boolean ("mute", "Mute",
          "If the audio recording should be muted. Note that this still "
          "saves audio data to the resulting file, but they are silent. Use "
          "a video-profile without audio to disable audio completely",
          DEFAULT_MUTE_AUDIO, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_AUDIO_CAPTURE_SUPPORTED_CAPS,
      g_param_spec_boxed ("audio-capture-supported-caps",
          "Audio capture supported caps",
          "Formats supported for capturing audio represented as GstCaps",
          GST_TYPE_CAPS, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_AUDIO_CAPTURE_CAPS,
      g_param_spec_boxed ("audio-capture-caps",
          "Audio capture caps",
          "Format to capture audio for video recording represented as GstCaps",
          GST_TYPE_CAPS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_IMAGE_CAPTURE_SUPPORTED_CAPS,
      g_param_spec_boxed ("image-capture-supported-caps",
          "Image capture supported caps",
          "Formats supported for capturing images represented as GstCaps",
          GST_TYPE_CAPS, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_VIDEO_CAPTURE_SUPPORTED_CAPS,
      g_param_spec_boxed ("video-capture-supported-caps",
          "Video capture supported caps",
          "Formats supported for capturing videos represented as GstCaps",
          GST_TYPE_CAPS, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_IMAGE_CAPTURE_CAPS,
      g_param_spec_boxed ("image-capture-caps",
          "Image capture caps",
          "Caps for image capture",
          GST_TYPE_CAPS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_VIDEO_CAPTURE_CAPS,
      g_param_spec_boxed ("video-capture-caps",
          "Video capture caps",
          "Caps for video capture",
          GST_TYPE_CAPS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_POST_PREVIEWS,
      g_param_spec_boolean ("post-previews", "Post Previews",
          "If capture preview images should be posted to the bus",
          DEFAULT_POST_PREVIEWS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PREVIEW_CAPS,
      g_param_spec_boxed ("preview-caps", "Preview caps",
          "The caps of the preview image to be posted",
          GST_TYPE_CAPS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_VIDEO_ENCODING_PROFILE,
      gst_param_spec_mini_object ("video-profile", "Video Profile",
          "The GstEncodingProfile to use for video recording. Audio is enabled "
          "when this profile supports audio.", GST_TYPE_ENCODING_PROFILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_IMAGE_FILTER,
      g_param_spec_object ("image-filter", "Image filter",
          "The element that will process captured image frames. (Should be"
          " set on NULL state)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_VIDEO_FILTER,
      g_param_spec_object ("video-filter", "Video filter",
          "The element that will process captured video frames. (Should be"
          " set on NULL state)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_VIEWFINDER_FILTER,
      g_param_spec_object ("viewfinder-filter", "Viewfinder filter",
          "The element that will process frames going to the viewfinder."
          " (Should be set on NULL state)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PREVIEW_FILTER,
      g_param_spec_object ("preview-filter", "Preview filter",
          "The element that will process preview buffers."
          " (Should be set on NULL state)",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_VIEWFINDER_SINK,
      g_param_spec_object ("viewfinder-sink", "Viewfinder sink",
          "The video sink of the viewfinder.",
          GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_VIEWFINDER_CAPS,
      g_param_spec_boxed ("viewfinder-caps",
          "Viewfinder caps",
          "Restricts the caps that can be used on the viewfinder",
          GST_TYPE_CAPS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_ZOOM,
      g_param_spec_float ("zoom", "Zoom",
          "Digital zoom factor (e.g. 1.5 means 1.5x)", MIN_ZOOM, MAX_ZOOM,
          DEFAULT_ZOOM, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_MAX_ZOOM,
      g_param_spec_float ("max-zoom", "Maximum zoom level (note: may change "
          "depending on resolution/implementation)",
          "Digital zoom factor (e.g. 1.5 means 1.5x)", MIN_ZOOM, G_MAXFLOAT,
          MAX_ZOOM, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /* TODO
   * Review before stable
   * - We use a profile for video recording properties and here we have
   *   elements for image capture. This is slightly inconsistent.
   * - One problem with using encodebin for images here is how jifmux
   *   autoplugging works. We need to give it a higher rank and fix its
   *   caps (it has image/jpeg on sink and src pads). Preliminary tests
   *   show that jifmux is picked if image/jpeg is the caps of a container
   *   profile. So this could work.
   * - There seems to be a problem with encodebin for images currently as
   *   it autoplugs a videorate that ony starts outputing buffers after
   *   getting the 2nd buffer.
   */
  g_object_class_install_property (object_class, PROP_IMAGE_ENCODING_PROFILE,
      gst_param_spec_mini_object ("image-profile", "Image Profile",
          "The GstEncodingProfile to use for image captures.",
          GST_TYPE_ENCODING_PROFILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  g_object_class_install_property (object_class, PROP_IDLE,
      g_param_spec_boolean ("idle", "Idle",
          "If camerabin2 is idle (not doing captures).", DEFAULT_IDLE,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /* TODO review before going stable
   * We have viewfinder-supported-caps that returns the caps that the
   * camerasrc can produce on its viewfinder pad, this could easily be
   * confused with what the viewfinder-sink accepts.
   *
   * Do we want to add a 'viewfinder-sink-supported-caps' or maybe change
   * the name of this property?
   */
  g_object_class_install_property (object_class,
      PROP_VIEWFINDER_SUPPORTED_CAPS,
      g_param_spec_boxed ("viewfinder-supported-caps",
          "Camera source Viewfinder pad supported caps",
          "The caps that the camera source can produce on the viewfinder pad",
          GST_TYPE_CAPS, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * GstCameraBin::capture-start:
   * @camera: the camera bin element
   *
   * Starts image capture or video recording depending on the Mode.
   */
  camerabin_signals[START_CAPTURE_SIGNAL] =
      g_signal_new ("start-capture",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstCameraBinClass, start_capture),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  /**
   * GstCameraBin::capture-stop:
   * @camera: the camera bin element
   */
  camerabin_signals[STOP_CAPTURE_SIGNAL] =
      g_signal_new ("stop-capture",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (GstCameraBinClass, stop_capture),
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
gst_camera_bin_init (GstCameraBin * camera)
{
  camera->post_previews = DEFAULT_POST_PREVIEWS;
  camera->mode = DEFAULT_MODE;
  camera->video_location = g_strdup (DEFAULT_VID_LOCATION);
  camera->image_location = g_strdup (DEFAULT_IMG_LOCATION);
  camera->viewfinderbin = gst_element_factory_make ("viewfinderbin", "vf-bin");
  camera->zoom = DEFAULT_ZOOM;
  camera->max_zoom = MAX_ZOOM;

  /* capsfilters are created here as we proxy their caps properties and
   * this way we avoid having to store the caps while on NULL state to 
   * set them later */
  camera->videobin_capsfilter = gst_element_factory_make ("capsfilter",
      "videobin-capsfilter");
  camera->imagebin_capsfilter = gst_element_factory_make ("capsfilter",
      "imagebin-capsfilter");
  camera->viewfinderbin_capsfilter = gst_element_factory_make ("capsfilter",
      "viewfinderbin-capsfilter");

  gst_bin_add_many (GST_BIN (camera),
      gst_object_ref (camera->viewfinderbin),
      gst_object_ref (camera->videobin_capsfilter),
      gst_object_ref (camera->imagebin_capsfilter),
      gst_object_ref (camera->viewfinderbin_capsfilter), NULL);

  /* these elements are only added if they are going to be used */
  camera->audio_capsfilter = gst_element_factory_make ("capsfilter",
      "audio-capsfilter");
  camera->audio_volume = gst_element_factory_make ("volume", "audio-volume");
}

static void
gst_image_capture_bin_post_image_done (GstCameraBin * camera,
    const gchar * filename)
{
  GstMessage *msg;

  g_return_if_fail (filename != NULL);

  msg = gst_message_new_element (GST_OBJECT_CAST (camera),
      gst_structure_new ("image-done", "filename", G_TYPE_STRING,
          filename, NULL));

  if (!gst_element_post_message (GST_ELEMENT_CAST (camera), msg))
    GST_WARNING_OBJECT (camera, "Failed to post image-done message");
}

static void
gst_camera_bin_handle_message (GstBin * bin, GstMessage * message)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ELEMENT:{
      const GstStructure *structure = gst_message_get_structure (message);
      const gchar *filename;

      if (gst_structure_has_name (structure, "GstMultiFileSink")) {
        GST_CAMERA_BIN_PROCESSING_DEC (GST_CAMERA_BIN_CAST (bin));
        filename = gst_structure_get_string (structure, "filename");
        if (filename) {
          gst_image_capture_bin_post_image_done (GST_CAMERA_BIN_CAST (bin),
              filename);
        }
      }
    }
      break;
    case GST_MESSAGE_WARNING:{
      GError *err = NULL;
      gchar *debug = NULL;

      gst_message_parse_warning (message, &err, &debug);
      if (err->domain == GST_RESOURCE_ERROR) {
        /* some capturing failed */
        GST_CAMERA_BIN_PROCESSING_DEC (GST_CAMERA_BIN_CAST (bin));
      }
    }
      break;
    case GST_MESSAGE_EOS:{
      GstElement *src = GST_ELEMENT (GST_MESSAGE_SRC (message));
      if (src == GST_CAMERA_BIN_CAST (bin)->videosink) {
        GST_DEBUG_OBJECT (bin, "EOS from video branch");
        GST_CAMERA_BIN_PROCESSING_DEC (GST_CAMERA_BIN_CAST (bin));
      }
    }
      break;
    default:
      break;
  }
  if (message)
    GST_BIN_CLASS (parent_class)->handle_message (bin, message);
}

/*
 * Transforms:
 * ... ! previous_element [ ! current_filter ] ! next_element ! ...
 *
 * into:
 * ... ! previous_element [ ! new_filter ] ! next_element ! ...
 *
 * Where current_filter and new_filter might or might not be NULL
 */
static void
gst_camera_bin_check_and_replace_filter (GstCameraBin * camera,
    GstElement ** current_filter, GstElement * new_filter,
    GstElement * previous_element, GstElement * next_element)
{
  if (*current_filter == new_filter) {
    GST_DEBUG_OBJECT (camera, "Current filter is the same as the previous, "
        "no switch needed.");
    return;
  }

  GST_DEBUG_OBJECT (camera, "Replacing current filter (%s) with new filter "
      "(%s)", *current_filter ? GST_ELEMENT_NAME (*current_filter) : "null",
      new_filter ? GST_ELEMENT_NAME (new_filter) : "null");

  if (*current_filter) {
    gst_bin_remove (GST_BIN_CAST (camera), *current_filter);
    gst_object_unref (*current_filter);
    *current_filter = NULL;
  } else {
    /* unlink the pads */
    gst_element_unlink (previous_element, next_element);
  }

  if (new_filter) {
    *current_filter = gst_object_ref (new_filter);
    gst_bin_add (GST_BIN_CAST (camera), gst_object_ref (new_filter));
    gst_element_link_many (previous_element, new_filter, next_element, NULL);
  } else {
    gst_element_link (previous_element, next_element);
  }
}

static void
encodebin_element_added (GstElement * encodebin, GstElement * new_element,
    GstCameraBin * camera)
{
  GstElementFactory *factory = gst_element_get_factory (new_element);

  if (factory != NULL) {
    if (strcmp (GST_PLUGIN_FEATURE_NAME (factory), "audiorate") == 0 ||
        strcmp (GST_PLUGIN_FEATURE_NAME (factory), "videorate") == 0) {
      g_object_set (new_element, "skip-to-first", TRUE, NULL);
    }
  }
}

#define VIDEO_PAD 1
#define AUDIO_PAD 2
static GstPad *
encodebin_find_pad (GstCameraBin * camera, GstElement * encodebin,
    gint pad_type)
{
  GstPad *pad = NULL;
  GstIterator *iter;
  gboolean done;

  GST_DEBUG_OBJECT (camera, "Looking at encodebin pads, searching for %s pad",
      VIDEO_PAD ? "video" : "audio");

  iter = gst_element_iterate_sink_pads (encodebin);
  done = FALSE;
  while (!done) {
    switch (gst_iterator_next (iter, (gpointer *) & pad)) {
      case GST_ITERATOR_OK:
        if (pad_type == VIDEO_PAD) {
          if (strstr (GST_PAD_NAME (pad), "video") != NULL) {
            GST_DEBUG_OBJECT (camera, "Found video pad %s", GST_PAD_NAME (pad));
            done = TRUE;
            break;
          }
        } else if (pad_type == AUDIO_PAD) {
          if (strstr (GST_PAD_NAME (pad), "audio") != NULL) {
            GST_DEBUG_OBJECT (camera, "Found audio pad %s", GST_PAD_NAME (pad));
            done = TRUE;
            break;
          }
        }
        gst_object_unref (pad);
        pad = NULL;
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iter);
        break;
      case GST_ITERATOR_ERROR:
        pad = NULL;
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        pad = NULL;
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (iter);

  /* no static pad, try requesting one */
  if (pad == NULL) {
    GstElementClass *klass;
    GstPadTemplate *tmpl;

    GST_DEBUG_OBJECT (camera, "No pads found, trying to request one");

    klass = GST_ELEMENT_GET_CLASS (encodebin);
    tmpl = gst_element_class_get_pad_template (klass, pad_type == VIDEO_PAD ?
        "video_%d" : "audio_%d");

    pad = gst_element_request_pad (encodebin, tmpl, NULL, NULL);
    GST_DEBUG_OBJECT (camera, "Got pad: %s", pad ? GST_PAD_NAME (pad) : "null");
    gst_object_unref (tmpl);
  }

  return pad;
}

static gboolean
gst_camera_bin_video_profile_has_audio (GstCameraBin * camera)
{
  const GList *list;

  g_return_val_if_fail (camera->video_profile != NULL, FALSE);

  if (GST_IS_ENCODING_VIDEO_PROFILE (camera->video_profile))
    return FALSE;

  for (list =
      gst_encoding_container_profile_get_profiles ((GstEncodingContainerProfile
              *) camera->video_profile); list; list = g_list_next (list)) {
    GstEncodingProfile *profile = (GstEncodingProfile *) list->data;

    if (GST_IS_ENCODING_AUDIO_PROFILE (profile))
      return TRUE;
  }

  return FALSE;
}

static GstPadLinkReturn
gst_camera_bin_link_encodebin (GstCameraBin * camera, GstElement * encodebin,
    GstElement * element, gint padtype)
{
  GstPadLinkReturn ret;
  GstPad *srcpad;
  GstPad *sinkpad = NULL;

  srcpad = gst_element_get_static_pad (element, "src");
  sinkpad = encodebin_find_pad (camera, encodebin, padtype);

  g_assert (srcpad != NULL);
  g_assert (sinkpad != NULL);

  ret = gst_pad_link (srcpad, sinkpad);
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  return ret;
}

static void
gst_camera_bin_src_notify_max_zoom_cb (GObject * self, GParamSpec * pspec,
    gpointer user_data)
{
  GstCameraBin *camera = (GstCameraBin *) user_data;

  g_object_get (self, "max-zoom", &camera->max_zoom, NULL);
  GST_DEBUG_OBJECT (camera, "Max zoom updated to %f", camera->max_zoom);
  g_object_notify (G_OBJECT (camera), "max-zoom");
}

/**
 * gst_camera_bin_create_elements:
 * @param camera: the #GstCameraBin
 *
 * Creates all elements inside #GstCameraBin
 *
 * Each of the pads on the camera source is linked as follows:
 * .pad ! queue ! capsfilter ! correspondingbin
 *
 * Where 'correspondingbin' is the bin appropriate for
 * the camera source pad.
 */
static gboolean
gst_camera_bin_create_elements (GstCameraBin * camera)
{
  gboolean new_src = FALSE;
  gboolean new_audio_src = FALSE;
  gboolean has_audio;
  gboolean profile_switched = FALSE;

  if (!camera->elements_created) {
    /* TODO check that elements created in _init were really created */
    /* TODO add proper missing plugin error handling */

    camera->video_encodebin = gst_element_factory_make ("encodebin", NULL);
    camera->video_encodebin_signal_id =
        g_signal_connect (camera->video_encodebin, "element-added",
        (GCallback) encodebin_element_added, camera);

    camera->videosink =
        gst_element_factory_make ("filesink", "videobin-filesink");
    g_object_set (camera->videosink, "async", FALSE, NULL);

    /* audio elements */
    camera->audio_queue = gst_element_factory_make ("queue", "audio-queue");
    camera->audio_convert = gst_element_factory_make ("audioconvert",
        "audio-convert");

    if (camera->video_profile == NULL) {
      GstEncodingContainerProfile *prof;
      GstEncodingVideoProfile *video_prof;
      GstCaps *caps;

      caps = gst_caps_new_simple ("video/x-msvideo", NULL);
      prof = gst_encoding_container_profile_new ("avi", "mpeg4+avi",
          caps, NULL);
      gst_caps_unref (caps);

      caps = gst_caps_new_simple ("video/mpeg",
          "mpegversion", G_TYPE_INT, 4,
          "systemstream", G_TYPE_BOOLEAN, FALSE, NULL);
      video_prof = gst_encoding_video_profile_new (caps, NULL, NULL, 1);
      gst_encoding_video_profile_set_variableframerate (video_prof, TRUE);
      if (!gst_encoding_container_profile_add_profile (prof,
              (GstEncodingProfile *) video_prof)) {
        GST_WARNING_OBJECT (camera, "Failed to create encoding profiles");
      }
      gst_caps_unref (caps);

      camera->video_profile = (GstEncodingProfile *) prof;
      camera->video_profile_switch = TRUE;
    }

    camera->image_encodebin = gst_element_factory_make ("encodebin", NULL);
    camera->image_encodebin_signal_id =
        g_signal_connect (camera->image_encodebin, "element-added",
        (GCallback) encodebin_element_added, camera);

    camera->imagesink =
        gst_element_factory_make ("multifilesink", "imagebin-filesink");
    g_object_set (camera->imagesink, "async", FALSE, "post-messages", TRUE,
        NULL);

    if (camera->image_profile == NULL) {
      GstEncodingVideoProfile *prof;
      GstCaps *caps;

      caps = gst_caps_new_simple ("image/jpeg", NULL);
      prof = gst_encoding_video_profile_new (caps, NULL, NULL, 1);
      gst_encoding_video_profile_set_variableframerate (prof, TRUE);
      gst_caps_unref (caps);

      camera->image_profile = (GstEncodingProfile *) prof;
      camera->image_profile_switch = TRUE;
    }

    camera->videobin_queue =
        gst_element_factory_make ("queue", "videobin-queue");
    camera->imagebin_queue =
        gst_element_factory_make ("queue", "imagebin-queue");
    camera->viewfinderbin_queue =
        gst_element_factory_make ("queue", "viewfinderbin-queue");

    g_object_set (camera->viewfinderbin_queue, "leaky", 2, "silent", TRUE,
        NULL);
    g_object_set (camera->imagebin_queue, "max-size-time", (guint64) 0,
        "silent", TRUE, NULL);
    g_object_set (camera->videobin_queue, "silent", TRUE, NULL);

    gst_bin_add_many (GST_BIN_CAST (camera),
        gst_object_ref (camera->video_encodebin),
        gst_object_ref (camera->videosink),
        gst_object_ref (camera->image_encodebin),
        gst_object_ref (camera->imagesink),
        gst_object_ref (camera->videobin_queue),
        gst_object_ref (camera->imagebin_queue),
        gst_object_ref (camera->viewfinderbin_queue), NULL);

    /* Linking can be optimized TODO */
    gst_element_link_many (camera->videobin_queue, camera->videobin_capsfilter,
        NULL);
    gst_element_link (camera->video_encodebin, camera->videosink);

    gst_element_link_many (camera->imagebin_queue, camera->imagebin_capsfilter,
        NULL);
    gst_element_link (camera->image_encodebin, camera->imagesink);
    gst_element_link_many (camera->viewfinderbin_queue,
        camera->viewfinderbin_capsfilter, camera->viewfinderbin, NULL);
    /*
     * Video can't get into playing as its internal filesink will open
     * a file for writing and leave it empty if unused.
     *
     * Its state is managed using the current mode and the source's
     * ready-for-capture notify callback. When we are at video mode and
     * the source's ready-for-capture goes to FALSE it means it is
     * starting recording, so we should prepare the video bin.
     */
    gst_element_set_locked_state (camera->videosink, TRUE);
    gst_element_set_locked_state (camera->imagesink, TRUE);

    g_object_set (camera->videosink, "location", camera->video_location, NULL);
    g_object_set (camera->imagesink, "location", camera->image_location, NULL);
  }

  if (camera->video_profile_switch) {
    GST_DEBUG_OBJECT (camera, "Switching encodebin's profile");
    g_object_set (camera->video_encodebin, "profile", camera->video_profile,
        NULL);
    gst_camera_bin_link_encodebin (camera, camera->video_encodebin,
        camera->videobin_capsfilter, VIDEO_PAD);
    camera->video_profile_switch = FALSE;

    /* used to trigger relinking further down */
    profile_switched = TRUE;
  }

  if (camera->image_profile_switch) {
    GST_DEBUG_OBJECT (camera, "Switching encodebin's profile");
    g_object_set (camera->image_encodebin, "profile", camera->image_profile,
        NULL);
    gst_camera_bin_link_encodebin (camera, camera->image_encodebin,
        camera->imagebin_capsfilter, VIDEO_PAD);
    camera->image_profile_switch = FALSE;
  }

  /* check if we need to replace the camera src */
  if (camera->src) {
    if (camera->user_src && camera->user_src != camera->src) {

      if (camera->src_capture_notify_id)
        g_signal_handler_disconnect (camera->src,
            camera->src_capture_notify_id);

      gst_bin_remove (GST_BIN_CAST (camera), camera->src);
      gst_object_unref (camera->src);
      camera->src = NULL;
    }
  }

  if (!camera->src) {
    if (camera->user_src) {
      camera->src = gst_object_ref (camera->user_src);
    } else {
      camera->src = gst_element_factory_make ("omxcamerabinsrc", "camerasrc");
    }

    new_src = TRUE;
  }

  g_assert (camera->src != NULL);
  g_object_set (camera->src, "mode", camera->mode, NULL);
  if (camera->src) {
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (camera->src),
            "preview-caps")) {
      g_object_set (camera->src, "post-previews", camera->post_previews,
          "preview-caps", camera->preview_caps, "preview-filter",
          camera->preview_filter, NULL);
    }
    g_object_set (camera->src, "zoom", camera->zoom, NULL);
    g_signal_connect (G_OBJECT (camera->src), "notify::max-zoom",
        (GCallback) gst_camera_bin_src_notify_max_zoom_cb, camera);
  }
  if (new_src) {
    gst_bin_add (GST_BIN_CAST (camera), gst_object_ref (camera->src));
    camera->src_capture_notify_id = g_signal_connect (G_OBJECT (camera->src),
        "notify::ready-for-capture",
        G_CALLBACK (gst_camera_bin_src_notify_readyforcapture), camera);
    gst_element_link_pads (camera->src, "vfsrc", camera->viewfinderbin_queue,
        "sink");
    gst_element_link_pads (camera->src, "imgsrc", camera->imagebin_queue,
        "sink");
    gst_element_link_pads (camera->src, "vidsrc", camera->videobin_queue,
        "sink");
  }

  gst_camera_bin_check_and_replace_filter (camera, &camera->image_filter,
      camera->user_image_filter, camera->imagebin_queue,
      camera->imagebin_capsfilter);
  gst_camera_bin_check_and_replace_filter (camera, &camera->video_filter,
      camera->user_video_filter, camera->videobin_queue,
      camera->videobin_capsfilter);
  gst_camera_bin_check_and_replace_filter (camera, &camera->viewfinder_filter,
      camera->user_viewfinder_filter, camera->viewfinderbin_queue,
      camera->viewfinderbin_capsfilter);

  /* check if we need to replace the camera audio src */
  has_audio = gst_camera_bin_video_profile_has_audio (camera);
  if (camera->audio_src) {
    if ((camera->user_audio_src && camera->user_audio_src != camera->audio_src)
        || !has_audio) {
      gst_bin_remove (GST_BIN_CAST (camera), camera->audio_src);
      gst_bin_remove (GST_BIN_CAST (camera), camera->audio_queue);
      gst_bin_remove (GST_BIN_CAST (camera), camera->audio_volume);
      gst_bin_remove (GST_BIN_CAST (camera), camera->audio_capsfilter);
      gst_bin_remove (GST_BIN_CAST (camera), camera->audio_convert);
      gst_object_unref (camera->audio_src);
      camera->audio_src = NULL;
    }
  }

  if (!camera->audio_src && has_audio) {
    if (camera->user_audio_src) {
      camera->audio_src = gst_object_ref (camera->user_audio_src);
    } else {
      camera->audio_src =
          gst_element_factory_make (DEFAULT_AUDIO_SRC, "audiosrc");
    }

    gst_element_set_locked_state (camera->audio_src, TRUE);
    new_audio_src = TRUE;
  }

  if (new_audio_src) {
    gst_bin_add (GST_BIN_CAST (camera), gst_object_ref (camera->audio_src));
    gst_bin_add (GST_BIN_CAST (camera), gst_object_ref (camera->audio_queue));
    gst_bin_add (GST_BIN_CAST (camera), gst_object_ref (camera->audio_volume));
    gst_bin_add (GST_BIN_CAST (camera),
        gst_object_ref (camera->audio_capsfilter));
    gst_bin_add (GST_BIN_CAST (camera), gst_object_ref (camera->audio_convert));

    gst_element_link_many (camera->audio_src, camera->audio_queue,
        camera->audio_volume,
        camera->audio_capsfilter, camera->audio_convert, NULL);
  }

  if ((profile_switched && has_audio) || new_audio_src) {
    gst_camera_bin_link_encodebin (camera, camera->video_encodebin,
        camera->audio_convert, AUDIO_PAD);
  }

  camera->elements_created = TRUE;
  return TRUE;
}

static GstStateChangeReturn
gst_camera_bin_change_state (GstElement * element, GstStateChange trans)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (element);

  switch (trans) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_camera_bin_create_elements (camera)) {
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_CAMERA_BIN_RESET_PROCESSING_COUNTER (camera);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, trans);

  switch (trans) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (GST_STATE (camera->videosink) >= GST_STATE_PAUSED)
        gst_element_set_state (camera->videosink, GST_STATE_READY);
      if (GST_STATE (camera->imagesink) >= GST_STATE_PAUSED)
        gst_element_set_state (camera->imagesink, GST_STATE_READY);
      if (camera->audio_src && GST_STATE (camera->audio_src) >= GST_STATE_READY)
        gst_element_set_state (camera->audio_src, GST_STATE_READY);

      gst_tag_setter_reset_tags (GST_TAG_SETTER (camera));
      GST_CAMERA_BIN_RESET_PROCESSING_COUNTER (camera);

      /* explicitly set to READY as they might be outside of the bin */
      gst_element_set_state (camera->audio_queue, GST_STATE_READY);
      gst_element_set_state (camera->audio_volume, GST_STATE_READY);
      gst_element_set_state (camera->audio_capsfilter, GST_STATE_READY);
      gst_element_set_state (camera->audio_convert, GST_STATE_READY);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_element_set_state (camera->videosink, GST_STATE_NULL);
      gst_element_set_state (camera->imagesink, GST_STATE_NULL);
      if (camera->audio_src)
        gst_element_set_state (camera->audio_src, GST_STATE_NULL);

      /* explicitly set to NULL as they might be outside of the bin */
      gst_element_set_state (camera->audio_queue, GST_STATE_NULL);
      gst_element_set_state (camera->audio_volume, GST_STATE_NULL);
      gst_element_set_state (camera->audio_capsfilter, GST_STATE_NULL);
      gst_element_set_state (camera->audio_convert, GST_STATE_NULL);

      break;
    default:
      break;
  }

  return ret;
}

static gboolean
gst_camera_bin_send_event (GstElement * element, GstEvent * event)
{
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (element);
  gboolean res;

  res = GST_ELEMENT_CLASS (parent_class)->send_event (element, event);
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
    {
      GstState current;

      if (camera->videosink) {
        gst_element_get_state (camera->videosink, &current, NULL, 0);
        if (current <= GST_STATE_READY)
          gst_element_post_message (camera->videosink,
              gst_message_new_eos (GST_OBJECT (camera->videosink)));
      }
      if (camera->imagesink) {
        gst_element_get_state (camera->imagesink, &current, NULL, 0);
        if (current <= GST_STATE_READY)
          gst_element_post_message (camera->imagesink,
              gst_message_new_eos (GST_OBJECT (camera->imagesink)));
      }
      break;
    }

    default:
      break;
  }

  return res;
}

static void
gst_camera_bin_set_location (GstCameraBin * camera, const gchar * location)
{
  GST_DEBUG_OBJECT (camera, "Setting mode %d location to %s", camera->mode,
      location);
  if (camera->mode == MODE_IMAGE) {
    g_free (camera->image_location);
    camera->image_location = g_strdup (location);
  } else {
    g_free (camera->video_location);
    camera->video_location = g_strdup (location);
  }
}

static void
gst_camera_bin_set_audio_src (GstCameraBin * camera, GstElement * src)
{
  GST_DEBUG_OBJECT (GST_OBJECT (camera),
      "Setting audio source %" GST_PTR_FORMAT, src);

  if (camera->user_audio_src)
    g_object_unref (camera->user_audio_src);

  if (src)
    g_object_ref (src);
  camera->user_audio_src = src;
}

static void
gst_camera_bin_set_camera_src (GstCameraBin * camera, GstElement * src)
{
  GST_DEBUG_OBJECT (GST_OBJECT (camera),
      "Setting camera source %" GST_PTR_FORMAT, src);

  if (camera->user_src)
    g_object_unref (camera->user_src);

  if (src)
    g_object_ref (src);
  camera->user_src = src;
}

static void
gst_camera_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (object);

  switch (prop_id) {
    case PROP_MODE:
      gst_camera_bin_change_mode (camera, g_value_get_enum (value));
      break;
    case PROP_LOCATION:
      gst_camera_bin_set_location (camera, g_value_get_string (value));
      break;
    case PROP_CAMERA_SRC:
      gst_camera_bin_set_camera_src (camera, g_value_get_object (value));
      break;
    case PROP_AUDIO_SRC:
      gst_camera_bin_set_audio_src (camera, g_value_get_object (value));
      break;
    case PROP_MUTE_AUDIO:
      g_object_set (camera->audio_volume, "mute", g_value_get_boolean (value),
          NULL);
      break;
    case PROP_AUDIO_CAPTURE_CAPS:{
      GST_DEBUG_OBJECT (camera,
          "Setting audio capture caps to %" GST_PTR_FORMAT,
          gst_value_get_caps (value));

      g_object_set (camera->audio_capsfilter, "caps",
          gst_value_get_caps (value), NULL);
    }
      break;
    case PROP_IMAGE_CAPTURE_CAPS:{
      GstPad *pad = NULL;

      if (camera->src)
        pad =
            gst_element_get_static_pad (camera->src,
            GST_BASE_CAMERA_SRC_IMAGE_PAD_NAME);

      GST_DEBUG_OBJECT (camera,
          "Setting image capture caps to %" GST_PTR_FORMAT,
          gst_value_get_caps (value));

      /* set the capsfilter caps and notify the src to renegotiate */
      g_object_set (camera->imagebin_capsfilter, "caps",
          gst_value_get_caps (value), NULL);
      if (pad) {
        GST_DEBUG_OBJECT (camera, "Pushing renegotiate on %s",
            GST_PAD_NAME (pad));
        GST_PAD_EVENTFUNC (pad) (pad, gst_camera_bin_new_event_renegotiate ());
        gst_object_unref (pad);
      }
    }
      break;
    case PROP_VIDEO_CAPTURE_CAPS:{
      GstPad *pad = NULL;

      if (camera->src)
        pad =
            gst_element_get_static_pad (camera->src,
            GST_BASE_CAMERA_SRC_VIDEO_PAD_NAME);

      GST_DEBUG_OBJECT (camera,
          "Setting video capture caps to %" GST_PTR_FORMAT,
          gst_value_get_caps (value));

      /* set the capsfilter caps and notify the src to renegotiate */
      g_object_set (camera->videobin_capsfilter, "caps",
          gst_value_get_caps (value), NULL);
      if (pad) {
        GST_DEBUG_OBJECT (camera, "Pushing renegotiate on %s",
            GST_PAD_NAME (pad));
        GST_PAD_EVENTFUNC (pad) (pad, gst_camera_bin_new_event_renegotiate ());
        gst_object_unref (pad);
      }
    }
      break;
    case PROP_VIEWFINDER_CAPS:{
      GstPad *pad = NULL;

      if (camera->src)
        pad =
            gst_element_get_static_pad (camera->src,
            GST_BASE_CAMERA_SRC_VIEWFINDER_PAD_NAME);

      GST_DEBUG_OBJECT (camera,
          "Setting viewfinder capture caps to %" GST_PTR_FORMAT,
          gst_value_get_caps (value));

      /* set the capsfilter caps and notify the src to renegotiate */
      g_object_set (camera->viewfinderbin_capsfilter, "caps",
          gst_value_get_caps (value), NULL);
      if (pad) {
        GST_DEBUG_OBJECT (camera, "Pushing renegotiate on %s",
            GST_PAD_NAME (pad));
        GST_PAD_EVENTFUNC (pad) (pad, gst_camera_bin_new_event_renegotiate ());
        gst_object_unref (pad);
      }
    }
      break;
    case PROP_POST_PREVIEWS:
      camera->post_previews = g_value_get_boolean (value);
      if (camera->src
          && g_object_class_find_property (G_OBJECT_GET_CLASS (camera->src),
              "post-previews"))
        g_object_set (camera->src, "post-previews", camera->post_previews,
            NULL);
      break;
    case PROP_PREVIEW_CAPS:
      gst_caps_replace (&camera->preview_caps,
          (GstCaps *) gst_value_get_caps (value));
      if (camera->src
          && g_object_class_find_property (G_OBJECT_GET_CLASS (camera->src),
              "preview-caps"))
        g_object_set (camera->src, "preview-caps", camera->preview_caps, NULL);
      break;
    case PROP_VIDEO_ENCODING_PROFILE:
      if (camera->video_profile)
        gst_encoding_profile_unref (camera->video_profile);
      camera->video_profile =
          (GstEncodingProfile *) gst_value_dup_mini_object (value);
      camera->video_profile_switch = TRUE;
      break;
    case PROP_IMAGE_FILTER:
      if (camera->user_image_filter)
        g_object_unref (camera->user_image_filter);

      camera->user_image_filter = g_value_dup_object (value);
      break;
    case PROP_VIDEO_FILTER:
      if (camera->user_video_filter)
        g_object_unref (camera->user_video_filter);

      camera->user_video_filter = g_value_dup_object (value);
      break;
    case PROP_VIEWFINDER_FILTER:
      if (camera->user_viewfinder_filter)
        g_object_unref (camera->user_viewfinder_filter);

      camera->user_viewfinder_filter = g_value_dup_object (value);
      break;
    case PROP_PREVIEW_FILTER:
      if (camera->preview_filter)
        g_object_unref (camera->preview_filter);

      camera->preview_filter = g_value_dup_object (value);
      if (camera->src
          && g_object_class_find_property (G_OBJECT_GET_CLASS (camera->src),
              "preview-filter"))
        g_object_set (camera->src, "preview-filter", camera->preview_filter,
            NULL);
      break;
    case PROP_VIEWFINDER_SINK:
      g_object_set (camera->viewfinderbin, "video-sink",
          g_value_get_object (value), NULL);
      break;
    case PROP_ZOOM:
      camera->zoom = g_value_get_float (value);
      /* limit to max-zoom */
      if (camera->zoom > camera->max_zoom) {
        GST_DEBUG_OBJECT (camera, "Clipping zoom %f to max-zoom %f",
            camera->zoom, camera->max_zoom);
        camera->zoom = camera->max_zoom;
      }
      if (camera->src)
        g_object_set (camera->src, "zoom", camera->zoom, NULL);
      break;
    case PROP_IMAGE_ENCODING_PROFILE:
      if (camera->image_profile)
        gst_encoding_profile_unref (camera->image_profile);
      camera->image_profile =
          (GstEncodingProfile *) gst_value_dup_mini_object (value);
      camera->image_profile_switch = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_camera_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCameraBin *camera = GST_CAMERA_BIN_CAST (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, camera->mode);
      break;
    case PROP_LOCATION:
      if (camera->mode == MODE_VIDEO) {
        g_value_set_string (value, camera->video_location);
      } else {
        g_value_set_string (value, camera->image_location);
      }
      break;
    case PROP_CAMERA_SRC:
      g_value_set_object (value, camera->src);
      break;
    case PROP_AUDIO_SRC:
      g_value_set_object (value, camera->audio_src);
      break;
    case PROP_MUTE_AUDIO:{
      gboolean mute;

      g_object_get (camera->audio_volume, "mute", &mute, NULL);
      g_value_set_boolean (value, mute);
      break;
    }
    case PROP_AUDIO_CAPTURE_SUPPORTED_CAPS:
    case PROP_VIDEO_CAPTURE_SUPPORTED_CAPS:
    case PROP_VIEWFINDER_SUPPORTED_CAPS:
    case PROP_IMAGE_CAPTURE_SUPPORTED_CAPS:{
      GstPad *pad;
      GstElement *element;
      GstCaps *caps;
      const gchar *padname;

      if (prop_id == PROP_VIDEO_CAPTURE_SUPPORTED_CAPS) {
        element = camera->src;
        padname = GST_BASE_CAMERA_SRC_VIDEO_PAD_NAME;
      } else if (prop_id == PROP_IMAGE_CAPTURE_SUPPORTED_CAPS) {
        element = camera->src;
        padname = GST_BASE_CAMERA_SRC_IMAGE_PAD_NAME;
      } else if (prop_id == PROP_VIEWFINDER_SUPPORTED_CAPS) {
        element = camera->src;
        padname = GST_BASE_CAMERA_SRC_VIEWFINDER_PAD_NAME;
      } else {
        element = camera->audio_src;
        padname = "src";
      }

      if (element) {
        pad = gst_element_get_static_pad (element, padname);

        g_assert (pad != NULL);

        /* TODO not sure if we want get_caps or get_allowed_caps to already
         * consider the full pipeline scenario and avoid picking a caps that
         * won't negotiate. Need to take care on the special case of the
         * pad being unlinked.
         */
        caps = gst_pad_get_caps_reffed (pad);
        if (caps) {
          gst_value_set_caps (value, caps);
          gst_caps_unref (caps);
        }

        gst_object_unref (pad);
      } else {
        GST_DEBUG_OBJECT (camera, "Source not created, can't get "
            "supported caps");
      }
    }
      break;
    case PROP_AUDIO_CAPTURE_CAPS:{
      GstCaps *caps = NULL;
      g_object_get (camera->audio_capsfilter, "caps", &caps, NULL);
      gst_value_set_caps (value, caps);
      gst_caps_unref (caps);
    }
      break;
    case PROP_IMAGE_CAPTURE_CAPS:{
      GstCaps *caps = NULL;
      g_object_get (camera->imagebin_capsfilter, "caps", &caps, NULL);
      gst_value_set_caps (value, caps);
      gst_caps_unref (caps);
    }
      break;
    case PROP_VIDEO_CAPTURE_CAPS:{
      GstCaps *caps = NULL;
      g_object_get (camera->videobin_capsfilter, "caps", &caps, NULL);
      gst_value_set_caps (value, caps);
      gst_caps_unref (caps);
    }
      break;
    case PROP_VIEWFINDER_CAPS:{
      GstCaps *caps = NULL;
      g_object_get (camera->viewfinderbin_capsfilter, "caps", &caps, NULL);
      gst_value_set_caps (value, caps);
      gst_caps_unref (caps);
    }
      break;
    case PROP_POST_PREVIEWS:
      g_value_set_boolean (value, camera->post_previews);
      break;
    case PROP_PREVIEW_CAPS:
      if (camera->preview_caps)
        gst_value_set_caps (value, camera->preview_caps);
      break;
    case PROP_VIDEO_ENCODING_PROFILE:
      if (camera->video_profile) {
        gst_value_set_mini_object (value,
            (GstMiniObject *) camera->video_profile);
      }
      break;
    case PROP_VIDEO_FILTER:
      if (camera->video_filter)
        g_value_set_object (value, camera->video_filter);
      break;
    case PROP_IMAGE_FILTER:
      if (camera->image_filter)
        g_value_set_object (value, camera->image_filter);
      break;
    case PROP_VIEWFINDER_FILTER:
      if (camera->viewfinder_filter)
        g_value_set_object (value, camera->viewfinder_filter);
      break;
    case PROP_PREVIEW_FILTER:
      if (camera->preview_filter)
        g_value_set_object (value, camera->preview_filter);
      break;
    case PROP_VIEWFINDER_SINK:{
      GstElement *sink;

      g_object_get (camera->viewfinderbin, "video-sink", &sink, NULL);
      g_value_take_object (value, sink);
      break;
    }
    case PROP_ZOOM:
      g_value_set_float (value, camera->zoom);
      break;
    case PROP_MAX_ZOOM:
      g_value_set_float (value, camera->max_zoom);
      break;
    case PROP_IMAGE_ENCODING_PROFILE:
      if (camera->image_profile) {
        gst_value_set_mini_object (value,
            (GstMiniObject *) camera->image_profile);
      }
      break;
    case PROP_IDLE:
      g_value_set_boolean (value,
          g_atomic_int_get (&camera->processing_counter) == 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_camera_bin_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_camera_bin_debug, "camerabin2", 0, "CameraBin2");

  return gst_element_register (plugin, "camerabin2", GST_RANK_NONE,
      gst_camera_bin_get_type ());
}
