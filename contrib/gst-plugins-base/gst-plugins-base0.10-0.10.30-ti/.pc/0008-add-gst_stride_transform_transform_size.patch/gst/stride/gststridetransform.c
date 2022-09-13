/* GStreamer
 *
 * Copyright (C) 2009 Texas Instruments, Inc - http://www.ti.com/
 *
 * Description: V4L2 sink element
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


static const GstElementDetails stridetransform_details =
GST_ELEMENT_DETAILS ("Stride transform",
    "Filter/Converter/Video",
    "Convert between video buffers with and without stride, or with differing stride",
    "Rob Clark <rob@ti.com>,");


/* TODO: add rgb formats too! */
#define SUPPORTED_CAPS                                                        \
  GST_VIDEO_CAPS_YUV_STRIDED ("{ I420, YV12, YUY2 }", "[ 0, max ]")


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
static GstCaps *gst_stride_transform_transform_caps (GstBaseTransform *base,
    GstPadDirection direction, GstCaps *caps);
static gboolean gst_stride_transform_set_caps (GstBaseTransform *base,
    GstCaps *incaps, GstCaps *outcaps);
static GstFlowReturn gst_stride_transform_transform (GstBaseTransform *base,
    GstBuffer *inbuf, GstBuffer *outbuf);
static GstFlowReturn gst_stride_transform_transform_ip (GstBaseTransform *base,
    GstBuffer *buf);

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
  basetransform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform_caps);
  basetransform_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_stride_transform_set_caps);
  basetransform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform_ip);
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
 * helper to add all fields, other than rowstride to @caps, copied from @s.
 */
static void
add_all_fields (GstCaps *caps, const gchar *name, GstStructure *s, gboolean rowstride)
{
  gint idx;
  GstStructure *new_s = gst_structure_new (name, NULL);

  if (rowstride) {
    gst_structure_set (new_s, "rowstride", GST_TYPE_INT_RANGE, 1, 1000, NULL);  // TODO
  }

  idx = gst_structure_n_fields (s) - 1;
  while (idx >= 0) {
    const gchar *name = gst_structure_nth_field_name (s, idx);
    if (strcmp ("rowstride", name)) {
      const GValue *val = gst_structure_get_value (s, name);
      gst_structure_set_value (new_s, name, val);
    }
    idx--;
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

    add_all_fields (ret, "video/x-raw-yuv", s, FALSE);
    add_all_fields (ret, "video/x-raw-yuv-strided", s, TRUE);

  } else if (gst_structure_has_name (s, "video/x-raw-rgb") ||
      gst_structure_has_name (s, "video/x-raw-rgb-strided")) {

    add_all_fields (ret, "video/x-raw-rgb", s, FALSE);
    add_all_fields (ret, "video/x-raw-rgb-strided", s, TRUE);

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
  GstVideoFormat format;
  gint width, height;

  LOG_CAPS (self, incaps);
  LOG_CAPS (self, outcaps);

  g_return_val_if_fail (gst_video_format_parse_caps_strided (incaps,
      &self->format, &self->width, &self->height, &self->in_rowstride), FALSE);
  g_return_val_if_fail (gst_video_format_parse_caps_strided (outcaps,
      &format, &width, &height, &self->out_rowstride), FALSE);

  g_return_val_if_fail (self->format == format, FALSE);
  g_return_val_if_fail (self->width  == width,  FALSE);
  g_return_val_if_fail (self->height == height, FALSE);

  return TRUE;
}

/* ************************************************************************* */

/**
 * Convert from one stride to another... like memmove, but can convert stride in
 * the process.  This function is not aware of pixels, only of bytes.  So widths
 * are given in bytes, not pixels.  The new_buf and orig_buf can point to the
 * same buffers to do an in-place conversion, but the buffer should be large
 * enough.
 */
static void
stridemove (guchar *new_buf, guchar *orig_buf, gint new_width, gint orig_width, gint height)
{
  int row;

  GST_DEBUG ("new_buf=%p, orig_buf=%p, new_width=%d, orig_width=%d, height=%d",
      new_buf, orig_buf, new_width, orig_width, height);
  /* if increasing the stride, work from bottom-up to avoid overwriting data
   * that has not been moved yet.. otherwise, work in the opposite order,
   * for the same reason.
   */
  if (new_width > orig_width) {
    for (row=height-1; row>=0; row--) {
      memmove (new_buf+(new_width*row), orig_buf+(orig_width*row), orig_width);
    }
  } else {
    for (row=0; row<height; row++) {
      memmove (new_buf+(new_width*row), orig_buf+(orig_width*row), orig_width);
    }
  }
}


/**
 * Convert from a non-strided buffer to strided.  The two buffer pointers could
 * be pointing to the same memory block for in-place transform.. assuming that
 * the buffer is large enough
 *
 * @strided:    the pointer to the resulting strided buffer
 * @unstrided:  the pointer to the initial unstrided buffer
 * @fourcc:     the color format
 * @stride:     the stride, in bytes
 * @width:      the width in pixels
 * @height:     the height in pixels
 */
static GstFlowReturn
stridify (GstStrideTransform *self, guchar *strided, guchar *unstrided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->out_rowstride;

  switch (self->format) {
#if 0 /* TODO */
    case GST_VIDEO_FORMAT_NV12:
      g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);
      stridemove (strided, unstrided, stride, width, height * 1.5);
      return GST_FLOW_OK;
#endif
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
      g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);
      stridemove (
          strided + (int)(height*stride*1.5),
          unstrided + (int)(height*width*1.5),
          stride, width/2, height);                             /* move U/V */
      stridemove (
          strided + (height*stride),
          unstrided + (height*width),
          stride, width/2, height);                             /* move V/U */
      stridemove (strided, unstrided, stride, width, height);   /* move Y */
      return GST_FLOW_OK;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
      g_return_val_if_fail (stride >= (width*2), GST_FLOW_ERROR);
      stridemove (strided, unstrided, stride, width*2, height);
      return GST_FLOW_OK;
    default:
      GST_WARNING ("unknown color format!\n");
      return GST_FLOW_ERROR;
  }
}


/**
 * Convert from a strided buffer to non-strided.  The two buffer pointers could
 * be pointing to the same memory block for in-place transform..
 *
 * @unstrided:  the pointer to the resulting unstrided buffer
 * @strided:    the pointer to the initial strided buffer
 */
static GstFlowReturn
unstridify (GstStrideTransform *self, guchar *unstrided, guchar *strided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->in_rowstride;

  switch (self->format) {
#if 0 /* TODO */
    case GST_VIDEO_FORMAT_NV12:
      g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);
      stridemove (unstrided, strided, width, stride, height * 1.5);
      return GST_FLOW_OK;
#endif
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
      g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);
      stridemove (unstrided, strided, width, stride, height);   /* move Y */
      stridemove (
          unstrided + (height*width),
          strided + (height*stride),
          width/2, stride, height);                             /* move V/U */
      stridemove (
          unstrided + (int)(height*width*1.5),
          strided + (int)(height*stride*1.5),
          width/2, stride, height);                             /* move U/V */
      return GST_FLOW_OK;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
      g_return_val_if_fail (stride >= (width*2), GST_FLOW_ERROR);
      stridemove (unstrided, strided, width*2, stride, height);
      return GST_FLOW_OK;
    default:
      GST_WARNING ("unknown color format!\n");
      return GST_FLOW_ERROR;
  }
}


static GstFlowReturn
gst_stride_transform_transform (GstBaseTransform *base,
    GstBuffer *inbuf, GstBuffer *outbuf)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);

  GST_DEBUG_OBJECT (self, "inbuf=%p, outbuf=%p", inbuf, outbuf);

  if (self->in_rowstride && self->out_rowstride) {
    GST_DEBUG_OBJECT (self, "not implemented");  // TODO
    return GST_FLOW_ERROR;
  } else if (self->in_rowstride) {
    return unstridify (self,
        GST_BUFFER_DATA (outbuf), GST_BUFFER_DATA (inbuf));
  } else if (self->out_rowstride) {
    return stridify (self,
        GST_BUFFER_DATA (outbuf), GST_BUFFER_DATA (inbuf));
  }

  GST_DEBUG_OBJECT (self, "this shouldn't happen!  in_rowstride=%d, out_rowstride=%d",
      self->in_rowstride, self->out_rowstride);

  return GST_FLOW_ERROR;
}

static GstFlowReturn
gst_stride_transform_transform_ip (GstBaseTransform *base,
    GstBuffer *buf)
{
  /* transform function is safe to call with same buffer ptr:
   */
  return gst_stride_transform_transform (base, buf, buf);
}
