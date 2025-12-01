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
 * gst-launch ???? TODO
 * ]| This pipeline ???? TODO
 * </refsect2>
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gststridetransform.h"
#include "gst/gst-i18n-plugin.h"


static const GstElementDetails stridetransform_details =
GST_ELEMENT_DETAILS ("Stride transform",
    "Filter/Converter/Video",
    "Convert between video buffers with and without stride, or with differing stride",
    "Rob Clark <rob@ti.com>,");

GST_DEBUG_CATEGORY (stridetransform_debug);
#define GST_CAT_DEFAULT stridetransform_debug

/* type functions */
static void gst_stride_transform_dispose (GObject * obj);

/* GstBaseTransform functions */
static gboolean gst_stride_transform_get_unit_size (GstBaseTransform * base,
    GstCaps * caps, guint * size);
static GstCaps *gst_stride_transform_transform_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps);
static void gst_stride_transform_fixate_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);
static gboolean gst_stride_transform_set_caps (GstBaseTransform * base,
    GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_stride_transform_transform (GstBaseTransform * base,
    GstBuffer * inbuf, GstBuffer * outbuf);
static GstFlowReturn gst_stride_transform_transform_ip (GstBaseTransform * base,
    GstBuffer * buf);

GST_BOILERPLATE (GstStrideTransform, gst_stride_transform, GstVideoFilter, GST_TYPE_VIDEO_FILTER);


static void
gst_stride_transform_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  GST_DEBUG_CATEGORY_INIT (stridetransform_debug, "stride", 0, "stride transform element");

  gst_element_class_set_details (gstelement_class, &stridetransform_details);
}

static void
gst_stride_transform_class_init (GstStrideTransformClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *basetransform_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->dispose = gst_stride_transform_dispose;

  basetransform_class->get_unit_size =
      GST_DEBUG_FUNCPTR (gst_stride_transform_get_unit_size);
  basetransform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform_caps);
  basetransform_class->fixate_caps =
      GST_DEBUG_FUNCPTR (gst_stride_transform_fixate_caps);
  basetransform_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_stride_transform_set_caps);
  basetransform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform_ip);
  basetransform_class->transform =
      GST_DEBUG_FUNCPTR (gst_stride_transform_transform);

  basetransform_class->passthrough_on_same_caps = TRUE;
}

static void
gst_stride_transform_init (GstStrideTransform * self, GstStrideTransformClass * klass)
{
  GST_DEBUG_OBJECT (self, "not implemented");
}


static void
gst_stride_transform_dispose (GObject * object)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (object);
  GST_DEBUG_OBJECT (self, "not implemented");
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gst_stride_transform_get_unit_size (GstBaseTransform * base,
    GstCaps * caps, guint * size)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GST_DEBUG_OBJECT (self, "not implemented");
  return FALSE;
}

static GstCaps *
gst_stride_transform_transform_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GST_DEBUG_OBJECT (self, "not implemented");
  return NULL;
}

static void
gst_stride_transform_fixate_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GST_DEBUG_OBJECT (self, "not implemented");
}

static gboolean
gst_stride_transform_set_caps (GstBaseTransform * base,
    GstCaps * incaps, GstCaps * outcaps)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GST_DEBUG_OBJECT (self, "not implemented");
  return FALSE;
}

static GstFlowReturn gst_stride_transform_transform (GstBaseTransform * base,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GST_DEBUG_OBJECT (self, "not implemented");
  return GST_FLOW_ERROR;
}

static GstFlowReturn
gst_stride_transform_transform_ip (GstBaseTransform * base,
    GstBuffer * buf)
{
  GstStrideTransform *self = GST_STRIDE_TRANSFORM (base);
  GST_DEBUG_OBJECT (self, "not implemented");
  return GST_FLOW_ERROR;
}
