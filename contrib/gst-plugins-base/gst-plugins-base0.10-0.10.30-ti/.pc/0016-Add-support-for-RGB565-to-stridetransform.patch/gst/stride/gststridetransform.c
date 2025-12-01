/* GStreamer
 *
 * Copyright (C) 2009 Texas Instruments, Inc - http://www.ti.com/
 *
 * Description: stride transform element
 *  Created on: Jul 30, 2009
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
 * SECTION:element-stridetransform
 *
 * stridetransform can be used to convert between video buffers
 * with and without stride, or between buffers with differing
 * stride
 *
 * <refsect2>
 * <title>Example launch lines</title>
 * |[
 * gst-launch videotestsrc ! video/x-raw-yuv,format=(fourcc)YUY2,width=320,height=240,framerate=30/1 !
 * stridetransform ! video/x-raw-yuv-strided,format=(fourcc)YUY2,width=320,height=240,rowstride=700,framerate=30/1 !
 * stridetransform ! video/x-raw-yuv,format=(fourcc)YUY2,width=320,height=240,framerate=30/1 !
 * v4l2sink
 * ]| This pipeline ???? TODO
 * </refsect2>
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gst/video/video.h>

#include "gst/gst-i18n-plugin.h"
#include "gststridetransform.h"


/* last entry has GST_VIDEO_FORMAT_UNKNOWN for in/out formats */
extern const Conversion stride_conversions[];


static const GstElementDetails stridetransform_details =
GST_ELEMENT_DETAILS ("Stride transform",
    "Filter/Converter/Video",
    "Convert between video buffers with and without stride, or with differing stride",
    "Rob Clark <rob@ti.com>,");


/* TODO: add rgb formats too! */
#define SUPPORTED_CAPS                                                        \
  GST_VIDEO_CAPS_YUV_STRIDED ("{ I420, YV12, YUY2, UYVY, NV12 }", "[ 0, max ]")


static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SUPPORTED_CAPS)
);

static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SUPPORTED_CAPS)
);


GST_DEBUG_CATEGORY (stridetransform_debug);
#define GST_CAT_DEFAULT stridetransform_debug

/* type functions */
static void gst_stride_transform_dispose (GObject *obj);

/* GstBaseTransform functions */
static gboolean gst_stride_transform_get_unit_size (GstBaseTransform *base,
    GstCaps *caps, guint *size);
static gboolean gst_stride_transform_transform_size (GstBaseTransform *base,
    GstPadDirection direction,
    GstCaps *caps, guint size,
    GstCaps *othercaps, guint *othersize);
static GstCaps *gst_stride_transform_transform_caps (GstBaseTransform *base,
    GstPadDirection direction, GstCaps *caps);
static gboolean gst_stride_transform_set_caps (GstBaseTransform *base,
    GstCaps *incaps, GstCaps *outcaps);
static GstFlowReturn gst_stride_transform_transform (GstBaseTransform *base,
    GstBuffer *inbuf, GstBuffer *outbuf);

GST_BOILERPLATE (GstStrideTransform, gst_stride_transform, GstVideoFilter, GST_TYPE_VIDEO_FILTER);


static void
gst_stride_transform_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (stridetransform_debug, "stride", 0, "stride transform element");

  gst_element_class_set_details (gstelement_class, &stridetransform_details);

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));
}

static void
gst_stride_transform_class_init (GstStrideTransformClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *basetransform_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->dispose = gst_stride_transform_dispose;

  basetransform_class->get_unit_size =
      GST_DEBUG_FUNCPTR (gst_stride_transform_get_unit_size);
  basetransform_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform_size);
  basetransform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform_caps);
  basetransform_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_stride_transform_set_caps);
  basetransform_class->transform =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform);

  basetransform_class->passthrough_on_same_caps = TRUE;
}

static void
gst_stride_transform_init (GstStrideTransform *self, GstStrideTransformClass *klass)
{
  GST_DEBUG_OBJECT (self, "not implemented");
}


static void
gst_stride_transform_dispose (GObject *object)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (object);
  GST_DEBUG_OBJECT (self, "not implemented");
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/**
 * figure out the required buffer size based on @caps
 */
static gboolean
gst_stride_transform_get_unit_size (GstBaseTransform *base,
    GstCaps *caps, guint *size)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GstVideoFormat format;
  gint width, height, rowstride;

  g_return_val_if_fail (gst_video_format_parse_caps_strided (
      caps, &format, &width, &height, &rowstride), FALSE);

  *size = gst_video_format_get_size_strided (format, width, height, rowstride);

  GST_DEBUG_OBJECT (self,
      "format=%d, width=%d, height=%d, rowstride=%d -> size=%d",
      format, width, height, rowstride, *size);

  return TRUE;
}

/**
 * Default transform_size function is no good, as it assumes that the output
 * buffer size is a multiple of the unit size.. which doesn't hold true.
 */
static gboolean
gst_stride_transform_transform_size (GstBaseTransform *base,
    GstPadDirection direction,
    GstCaps *caps, guint size,
    GstCaps *othercaps, guint *othersize)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  guint idx = (direction == GST_PAD_SINK) ? 0 : 1;

  if (self->cached_caps[idx] != othercaps)
  {
    guint sz;
    if (!gst_stride_transform_get_unit_size (base, othercaps, &sz)) {
      return FALSE;
    }
    if (self->cached_caps[idx]) {
      gst_caps_unref (self->cached_caps[idx]);
    }
    self->cached_size[idx] = sz;
    self->cached_caps[idx] = gst_caps_ref (othercaps);
  }

  *othersize = self->cached_size[idx];

  return TRUE;
}



/**
 * helper to add all fields, other than rowstride to @caps, copied from @s.
 */
static void
add_all_fields (GstCaps *caps, const gchar *name, GstStructure *s, gboolean rowstride, GstPadDirection direction)
{
  gint idx;
  GstStructure *new_s = gst_structure_new (name, NULL);

  if (rowstride) {
    gst_structure_set (new_s, "rowstride", GST_TYPE_INT_RANGE, 1, G_MAXINT, NULL);
  }

  idx = gst_structure_n_fields (s) - 1;
  while (idx >= 0) {
    const gchar *name = gst_structure_nth_field_name (s, idx);
    idx--;

    /* for format field, check the stride_conversions table to see what
     * we can support:
     */
    if (!strcmp ("format", name)) {
      guint fourcc;

      /* XXX double check this: */
      gint to_format = (direction == GST_PAD_SINK) ? 1 : 0;
      gint from_format = (direction == GST_PAD_SRC) ? 1 : 0;

      if (gst_structure_get_fourcc (s, "format", &fourcc)) {
        GValue formats = {0};
        GValue fourccval = {0};
        gint i;
        GstVideoFormat format = gst_video_format_from_fourcc (fourcc);

        g_value_init (&formats, GST_TYPE_LIST);
        g_value_init (&fourccval, GST_TYPE_FOURCC);

        for (i=0; stride_conversions[i].format[0]!=GST_VIDEO_FORMAT_UNKNOWN; i++) {
          if (stride_conversions[i].format[from_format] == format) {
            gst_value_set_fourcc (&fourccval, gst_video_format_to_fourcc
                (stride_conversions[i].format[to_format]));
            gst_value_list_append_value (&formats, &fourccval);
          }
        }

        continue;
      }
    }

    /* copy over all other non-rowstride fields: */
    if (strcmp ("rowstride", name)) {
      const GValue *val = gst_structure_get_value (s, name);
      gst_structure_set_value (new_s, name, val);
    }
  }

  gst_caps_merge_structure (caps, new_s);
}


/**
 * we can transform @caps to strided or non-strided caps with otherwise
 * identical parameters
 */
static GstCaps *
gst_stride_transform_transform_caps (GstBaseTransform *base,
    GstPadDirection direction, GstCaps *caps)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GstCaps *ret;
  GstStructure *s;

  g_return_val_if_fail (GST_CAPS_IS_SIMPLE (caps), NULL);

  GST_DEBUG_OBJECT (self, "direction=%d, caps=%p", direction, caps);
  LOG_CAPS (self, caps);

  ret = gst_caps_new_empty ();
  s = gst_caps_get_structure (caps, 0);

  if (gst_structure_has_name (s, "video/x-raw-yuv") ||
      gst_structure_has_name (s, "video/x-raw-yuv-strided")) {

    add_all_fields (ret, "video/x-raw-yuv", s, FALSE, direction);
    add_all_fields (ret, "video/x-raw-yuv-strided", s, TRUE, direction);

  } else if (gst_structure_has_name (s, "video/x-raw-rgb") ||
      gst_structure_has_name (s, "video/x-raw-rgb-strided")) {

    add_all_fields (ret, "video/x-raw-rgb", s, FALSE, direction);
    add_all_fields (ret, "video/x-raw-rgb-strided", s, TRUE, direction);

  }

  LOG_CAPS (self, ret);

  return ret;
}

/**
 * at this point, we have identical fourcc, width, and height for @incaps
 * and @outcaps..  so we need to extract these to use for transforming,
 * plus the requested rowstride of the @incaps and @outcaps
 */
static gboolean
gst_stride_transform_set_caps (GstBaseTransform *base,
    GstCaps *incaps, GstCaps *outcaps)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  gint width, height;
  GstVideoFormat in_format, out_format;
  gint i;

  LOG_CAPS (self, incaps);
  LOG_CAPS (self, outcaps);

  g_return_val_if_fail (gst_video_format_parse_caps_strided (incaps,
      &in_format, &self->width, &self->height, &self->in_rowstride), FALSE);
  g_return_val_if_fail (gst_video_format_parse_caps_strided (outcaps,
      &out_format, &width, &height, &self->out_rowstride), FALSE);

  self->conversion = NULL;

  for (i=0; stride_conversions[i].format[0]!=GST_VIDEO_FORMAT_UNKNOWN; i++) {
    if ((stride_conversions[i].format[0] == in_format) &&
        (stride_conversions[i].format[1] == out_format)) {
      GST_DEBUG_OBJECT (self, "found stride_conversion: %d", i);
      self->conversion = &stride_conversions[i];
      break;
    }
  }

  g_return_val_if_fail (self->conversion, FALSE);
  g_return_val_if_fail (self->conversion->unstridify || !self->in_rowstride, FALSE);
  g_return_val_if_fail (self->conversion->stridify || !self->out_rowstride, FALSE);
  g_return_val_if_fail (self->width  == width,  FALSE);
  g_return_val_if_fail (self->height == height, FALSE);

  return TRUE;
}

static GstFlowReturn
gst_stride_transform_transform (GstBaseTransform *base,
    GstBuffer *inbuf, GstBuffer *outbuf)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);

  GST_DEBUG_OBJECT (self, "inbuf=%p (size=%d), outbuf=%p (size=%d)",
      inbuf, GST_BUFFER_SIZE (inbuf),
      outbuf, GST_BUFFER_SIZE (outbuf));

  if (self->in_rowstride && self->out_rowstride) {
    GST_DEBUG_OBJECT (self, "not implemented");  // TODO
    return GST_FLOW_ERROR;
  } else if (self->in_rowstride) {
    return self->conversion->unstridify (self,
        GST_BUFFER_DATA (outbuf), GST_BUFFER_DATA (inbuf));
  } else if (self->out_rowstride) {
    return self->conversion->stridify (self,
        GST_BUFFER_DATA (outbuf), GST_BUFFER_DATA (inbuf));
  }

  GST_DEBUG_OBJECT (self, "this shouldn't happen!  in_rowstride=%d, out_rowstride=%d",
      self->in_rowstride, self->out_rowstride);

  return GST_FLOW_ERROR;
}
