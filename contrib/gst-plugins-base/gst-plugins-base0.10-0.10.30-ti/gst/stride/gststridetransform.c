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

GST_DEBUG_CATEGORY (stridetransform_debug);
#define GST_CAT_DEFAULT stridetransform_debug

/* type functions */
static void gst_stride_transform_dispose (GObject * obj);

/* GstBaseTransform functions */
static gboolean gst_stride_transform_event (GstBaseTransform * trans,
    GstEvent * event);
static gboolean gst_stride_transform_get_unit_size (GstBaseTransform * base,
    GstCaps * caps, guint * size);
static gboolean gst_stride_transform_transform_size (GstBaseTransform * base,
    GstPadDirection direction,
    GstCaps * caps, guint size, GstCaps * othercaps, guint * othersize);
static GstCaps *gst_stride_transform_transform_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps);
static gboolean gst_stride_transform_set_caps (GstBaseTransform * base,
    GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_stride_transform_transform (GstBaseTransform * base,
    GstBuffer * inbuf, GstBuffer * outbuf);
static GstCaps * get_all_templ_caps (GstPadDirection direction);

GST_BOILERPLATE (GstStrideTransform, gst_stride_transform, GstVideoFilter,
    GST_TYPE_VIDEO_FILTER);


static void
gst_stride_transform_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (stridetransform_debug, "stride", 0,
      "stride transform element");

  gst_element_class_set_details_simple (gstelement_class,
      "Stride transform", "Filter/Converter/Video",
      "Convert between video buffers with and without stride, or with differing stride",
      "Rob Clark <rob@ti.com>,");

  gst_element_class_add_pad_template (gstelement_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          get_all_templ_caps (GST_PAD_SINK)));
  gst_element_class_add_pad_template (gstelement_class,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          get_all_templ_caps (GST_PAD_SRC)));
}

static void
gst_stride_transform_class_init (GstStrideTransformClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *basetransform_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->dispose = gst_stride_transform_dispose;

  basetransform_class->event =
      GST_DEBUG_FUNCPTR (gst_stride_transform_event);
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
gst_stride_transform_init (GstStrideTransform * self,
    GstStrideTransformClass * klass)
{
  GST_DEBUG_OBJECT (self, "ENTER");
  self->cached_caps[0] = NULL;
  self->cached_caps[1] = NULL;
}


static void
gst_stride_transform_dispose (GObject * object)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (object);
  GST_DEBUG_OBJECT (self, "ENTER");
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gst_stride_transform_event (GstBaseTransform * trans, GstEvent * event)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (trans);

  GST_DEBUG_OBJECT (self, "event %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    /* if we get a crop, we don't change output size (yet, although it
     * would be nice to be able to figure out if the sink supported
     * cropping and if it does not perform the crop ourselves.. which
     * would involve adjusting output caps appropriately).  For now
     * we just treat it as an optimization and avoid copying the data
     * that will be later cropped out by the sink.
     */
    case GST_EVENT_CROP:
      gst_event_parse_crop (event, &self->crop_top, &self->crop_left,
          &self->crop_width, &self->crop_height);
      self->needs_refresh = TRUE;
      GST_DEBUG_OBJECT (self, "cropping at %d,%d %dx%d", self->crop_top,
          self->crop_left, self->crop_width, self->crop_height);
    default:
      break;
  }

  /* forward all events */
  return TRUE;
}

/**
 * figure out the required buffer size based on @caps
 */
static gboolean
gst_stride_transform_get_unit_size (GstBaseTransform * base,
    GstCaps * caps, guint * size)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GstVideoFormat format;
  gint width, height, rowstride;

  g_return_val_if_fail (gst_video_format_parse_caps_strided (caps, &format,
          &width, &height, &rowstride), FALSE);

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
gst_stride_transform_transform_size (GstBaseTransform * base,
    GstPadDirection direction,
    GstCaps * caps, guint size, GstCaps * othercaps, guint * othersize)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  guint idx = (direction == GST_PAD_SINK) ? 0 : 1;

  if (self->cached_caps[idx] != othercaps) {
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

static inline GstCaps *
get_templ_caps (GstVideoFormat fmt, gboolean strided)
{
  return gst_video_format_new_caps_simple (fmt,
      strided ? -1 : 0,
      "width", GST_TYPE_INT_RANGE, 1, G_MAXINT,
      "height", GST_TYPE_INT_RANGE, 1, G_MAXINT,
      "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1,
      NULL);
}

/**
 * Utility to get all possible template caps for given direction
 */
static GstCaps *
get_all_templ_caps (GstPadDirection direction)
{
  int i;
  gint to_format = (direction == GST_PAD_SINK) ? 1 : 0;
  GstCaps *templ = gst_caps_new_empty ();

  for (i = 0; stride_conversions[i].format[0]; i++) {
    const Conversion *c = &stride_conversions[i];
    gst_caps_append (templ, get_templ_caps (c->format[to_format], TRUE));
    gst_caps_append (templ, get_templ_caps (c->format[to_format], FALSE));
  }

  gst_caps_do_simplify (templ);

  GST_DEBUG ("template %s caps: %"GST_PTR_FORMAT,
      (direction == GST_PAD_SINK) ? "sink" : "src", templ);

  return templ;
}

static inline gboolean
is_filtered_field (const gchar *name)
{
  static const gchar * filtered_fields[] = {
      "rowstride", "format", "bpp", "depth", "endianness",
      "red_mask", "green_mask", "blue_mask"
  };
  gint i;
  for (i = 0; i < G_N_ELEMENTS (filtered_fields); i++)
    if (!strcmp (filtered_fields[i], name))
      return TRUE;
  return FALSE;
}

static inline GstCaps *
get_caps (GstVideoFormat fmt, gboolean strided, GstStructure *s)
{
  gint idx;
  GstCaps *ret =
      gst_video_format_new_caps_simple (fmt, strided ? -1 : 0, NULL);

  idx = gst_structure_n_fields (s) - 1;
  while (idx >= 0) {
    const gchar *name = gst_structure_nth_field_name (s, idx);

    idx--;

    /* filter out certain format specific fields.. copy everything else
     * from the original struct
     */
    if (!is_filtered_field (name)) {
      const GValue *val = gst_structure_get_value (s, name);
      gst_caps_set_value (ret, name, val);
    }
  }

  return ret;
}

/**
 * Utility to get all possible caps that can be converted to/from (depending
 * on 'direction') the specified 'fmt'.  The rest of the fields are populated
 * from 's'
 */
static GstCaps *
get_all_caps (GstPadDirection direction, GstVideoFormat fmt, GstStructure *s)
{
  GstCaps *ret = gst_caps_new_empty ();
  gint to_format = (direction == GST_PAD_SINK) ? 1 : 0;
  gint from_format = (direction == GST_PAD_SRC) ? 1 : 0;
  gint i;

  for (i = 0; stride_conversions[i].format[0]; i++) {
    const Conversion *c = &stride_conversions[i];
    if (c->format[from_format] == fmt) {
      gst_caps_append (ret, get_caps (c->format[to_format], TRUE, s));
      gst_caps_append (ret, get_caps (c->format[to_format], FALSE, s));
    }
  }

  return ret;
}

/** convert GValue holding fourcc to GstVideoFormat (for YUV) */
static inline GstVideoFormat
fmt_from_val (const GValue *val)
{
  return gst_video_format_from_fourcc (gst_value_get_fourcc (val));
}

/** convert structure to GstVideoFormat (for RGB) */
static inline GstVideoFormat
fmt_from_struct (const GstStructure *s)
{
  /* hmm.. this is not supporting any case where ranges/lists are used
   * for any of the rgb related fields in the caps.  But I'm not quite
   * sure a sane way to handle that..  rgb caps suck
   */
  gint depth, bpp, endianness;
  gint red_mask, green_mask, blue_mask, alpha_mask;
  gboolean have_alpha, ok = TRUE;

  ok &= gst_structure_get_int (s, "depth", &depth);
  ok &= gst_structure_get_int (s, "bpp", &bpp);
  ok &= gst_structure_get_int (s, "endianness", &endianness);
  ok &= gst_structure_get_int (s, "red_mask", &red_mask);
  ok &= gst_structure_get_int (s, "green_mask", &green_mask);
  ok &= gst_structure_get_int (s, "blue_mask", &blue_mask);
  have_alpha = gst_structure_get_int (s, "alpha_mask", &alpha_mask);

  if (!ok)
    return GST_VIDEO_FORMAT_UNKNOWN;

  if (depth == 24 && bpp == 32 && endianness == G_BIG_ENDIAN) {
    if (red_mask == 0xff000000 && green_mask == 0x00ff0000 &&
        blue_mask == 0x0000ff00) {
      return GST_VIDEO_FORMAT_RGBx;
    }
    if (red_mask == 0x0000ff00 && green_mask == 0x00ff0000 &&
        blue_mask == 0xff000000) {
      return GST_VIDEO_FORMAT_BGRx;
    }
    if (red_mask == 0x00ff0000 && green_mask == 0x0000ff00 &&
        blue_mask == 0x000000ff) {
      return GST_VIDEO_FORMAT_xRGB;
    }
    if (red_mask == 0x000000ff && green_mask == 0x0000ff00 &&
        blue_mask == 0x00ff0000) {
      return GST_VIDEO_FORMAT_xBGR;
    }
  } else if (depth == 32 && bpp == 32 && endianness == G_BIG_ENDIAN &&
      have_alpha) {
    if (red_mask == 0xff000000 && green_mask == 0x00ff0000 &&
        blue_mask == 0x0000ff00 && alpha_mask == 0x000000ff) {
      return GST_VIDEO_FORMAT_RGBA;
    }
    if (red_mask == 0x0000ff00 && green_mask == 0x00ff0000 &&
        blue_mask == 0xff000000 && alpha_mask == 0x000000ff) {
      return GST_VIDEO_FORMAT_BGRA;
    }
    if (red_mask == 0x00ff0000 && green_mask == 0x0000ff00 &&
        blue_mask == 0x000000ff && alpha_mask == 0xff000000) {
      return GST_VIDEO_FORMAT_ARGB;
    }
    if (red_mask == 0x000000ff && green_mask == 0x0000ff00 &&
        blue_mask == 0x00ff0000 && alpha_mask == 0xff000000) {
      return GST_VIDEO_FORMAT_ABGR;
    }
  } else if (depth == 24 && bpp == 24 && endianness == G_BIG_ENDIAN) {
    if (red_mask == 0xff0000 && green_mask == 0x00ff00 &&
        blue_mask == 0x0000ff) {
      return GST_VIDEO_FORMAT_RGB;
    }
    if (red_mask == 0x0000ff && green_mask == 0x00ff00 &&
        blue_mask == 0xff0000) {
      return GST_VIDEO_FORMAT_BGR;
    }
  } else if ((depth == 15 || depth == 16) && bpp == 16 &&
      endianness == G_BYTE_ORDER) {
    if (red_mask == GST_VIDEO_COMP1_MASK_16_INT
        && green_mask == GST_VIDEO_COMP2_MASK_16_INT
        && blue_mask == GST_VIDEO_COMP3_MASK_16_INT) {
      return GST_VIDEO_FORMAT_RGB16;
    }
    if (red_mask == GST_VIDEO_COMP3_MASK_16_INT
        && green_mask == GST_VIDEO_COMP2_MASK_16_INT
        && blue_mask == GST_VIDEO_COMP1_MASK_16_INT) {
      return GST_VIDEO_FORMAT_BGR16;
    }
    if (red_mask == GST_VIDEO_COMP1_MASK_15_INT
        && green_mask == GST_VIDEO_COMP2_MASK_15_INT
        && blue_mask == GST_VIDEO_COMP3_MASK_15_INT) {
      return GST_VIDEO_FORMAT_RGB15;
    }
    if (red_mask == GST_VIDEO_COMP3_MASK_15_INT
        && green_mask == GST_VIDEO_COMP2_MASK_15_INT
        && blue_mask == GST_VIDEO_COMP1_MASK_15_INT) {
      return GST_VIDEO_FORMAT_BGR15;
    }
  }

  return GST_VIDEO_FORMAT_UNKNOWN;
}

/**
 * we can transform @caps to strided or non-strided caps with otherwise
 * identical parameters
 */
static GstCaps *
gst_stride_transform_transform_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GstCaps *ret = gst_caps_new_empty ();
  int i;

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *s = gst_caps_get_structure (caps, i);
    const char *name = gst_structure_get_name (s);

    /* this is a bit ugly.. ideally it would be easier to parse caps
     * a bit more generically without having to care so much about
     * difference between RGB and YUV.. but YUV can be specified as
     * a list of format params, whereas RGB is a combination of many
     * fields..
     */
    if (g_str_has_prefix (name, "video/x-raw-yuv")) {
      const GValue *val = gst_structure_get_value (s, "format");

      if (GST_VALUE_HOLDS_FOURCC (val)) {
        gst_caps_append (ret,
            get_all_caps (direction, fmt_from_val (val), s));
      } else if (GST_VALUE_HOLDS_LIST (val)) {
        gint j;
        for (j = 0; j < gst_value_list_get_size (val); j++) {
          const GValue *list_val = gst_value_list_get_value (val, j);
          if (GST_VALUE_HOLDS_FOURCC (list_val)) {
            gst_caps_append (ret,
                get_all_caps (direction, fmt_from_val (list_val), s));
          } else {
            GST_WARNING_OBJECT (self,
                "malformed format in caps: %"GST_PTR_FORMAT, s);
            break;
          }
        }
      } else {
        GST_WARNING_OBJECT (self, "malformed yuv caps: %"GST_PTR_FORMAT, s);
      }
    } else if (g_str_has_prefix (name, "video/x-raw-rgb")) {
      gst_caps_append (ret, get_all_caps (direction, fmt_from_struct (s), s));
    } else {
      GST_WARNING_OBJECT (self, "ignoring: %"GST_PTR_FORMAT, s);
    }
  }

  gst_caps_do_simplify (ret);

  LOG_CAPS (self, ret);

  return ret;
}

/**
 * at this point, we have identical fourcc, width, and height for @incaps
 * and @outcaps..  so we need to extract these to use for transforming,
 * plus the requested rowstride of the @incaps and @outcaps
 */
static gboolean
gst_stride_transform_set_caps (GstBaseTransform * base,
    GstCaps * incaps, GstCaps * outcaps)
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

  for (i = 0; stride_conversions[i].format[0] != GST_VIDEO_FORMAT_UNKNOWN; i++) {
    if ((stride_conversions[i].format[0] == in_format) &&
        (stride_conversions[i].format[1] == out_format)) {
      GST_DEBUG_OBJECT (self, "found stride_conversion: %d", i);
      self->conversion = &stride_conversions[i];
      self->needs_refresh = TRUE;
      break;
    }
  }

  GST_DEBUG_OBJECT (self,
      "conversion[%d]=%p, in_rowstride=%d, out_rowstride=%d",
      i, self->conversion, self->in_rowstride, self->out_rowstride);

  g_return_val_if_fail (self->conversion, FALSE);
  g_return_val_if_fail (self->width == width, FALSE);
  g_return_val_if_fail (self->height == height, FALSE);

  GST_DEBUG_OBJECT (self, "caps are ok");

  return TRUE;
}

static GstFlowReturn
gst_stride_transform_transform (GstBaseTransform * base,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);

  GST_DEBUG_OBJECT (self, "inbuf=%p (size=%d), outbuf=%p (size=%d)",
      inbuf, GST_BUFFER_SIZE (inbuf), outbuf, GST_BUFFER_SIZE (outbuf));

  if (self->conversion) {
    return self->conversion->convert (self,
        GST_BUFFER_DATA (outbuf), GST_BUFFER_DATA (inbuf));
  }

  GST_DEBUG_OBJECT (self,
      "this shouldn't happen!  in_rowstride=%d, out_rowstride=%d, conversion=%p",
      self->in_rowstride, self->out_rowstride, self->conversion);

  return GST_FLOW_ERROR;
}
