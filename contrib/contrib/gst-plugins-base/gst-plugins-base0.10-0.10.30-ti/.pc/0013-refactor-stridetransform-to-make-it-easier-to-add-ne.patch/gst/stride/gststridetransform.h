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

#ifndef __GSTSTRIDETRANSFORM_H__
#define __GSTSTRIDETRANSFORM_H__


#include <gst/video/gstvideofilter.h>
#include <gst/video/video.h>


G_BEGIN_DECLS

#define GST_TYPE_STRIDE_TRANSFORM \
  (gst_stride_transform_get_type())
#define GST_STRIDE_TRANSFORM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_STRIDE_TRANSFORM,GstStrideTransform))
#define GST_STRIDE_TRANSFORM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_STRIDE_TRANSFORM,GstStrideTransformClass))
#define GST_IS_STRIDE_TRANSFORM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_STRIDE_TRANSFORM))
#define GST_IS_STRIDE_TRANSFORM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_STRIDE_TRANSFORM))

typedef struct _GstStrideTransform GstStrideTransform;
typedef struct _GstStrideTransformClass GstStrideTransformClass;

/**
 * GstStrideTransform:
 *
 * Opaque datastructure.
 */
struct _GstStrideTransform {
  GstVideoFilter videofilter;

  /*< private >*/
  GstVideoFormat in_format, out_format;
  gint width, height;
  gint in_rowstride;
  gint out_rowstride;

  /* for caching the tranform_size() results.. */
  GstCaps *cached_caps[2];
  guint cached_size[2];
};

struct _GstStrideTransformClass {
  GstVideoFilterClass parent_class;
};

GType gst_stride_transform_get_type (void);

G_END_DECLS


#define LOG_CAPS(obj, caps)  GST_DEBUG_OBJECT (obj, "%s: %"GST_PTR_FORMAT, #caps, caps)


#endif /* __GSTSTRIDETRANSFORM_H__ */
