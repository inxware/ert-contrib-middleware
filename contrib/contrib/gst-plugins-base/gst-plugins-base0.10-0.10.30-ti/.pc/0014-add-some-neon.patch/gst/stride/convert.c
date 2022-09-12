/* GStreamer
 *
 * Copyright (C) 2009 Texas Instruments, Inc - http://www.ti.com/
 *
 * Description: stride transform conversion utilities
 *  Created on: Nov 27, 2009
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/video/video.h>

#include "gststridetransform.h"


GST_DEBUG_CATEGORY_EXTERN (stridetransform_debug);
#define GST_CAT_DEFAULT stridetransform_debug


/*
 * Conversion utilities:
 */

static void
memmove_demux (guchar *new_buf, guchar *orig_buf, gint sz, gint pxstride)
{
  if (new_buf > orig_buf) {
    /* copy backwards */
    new_buf += ((sz - 1) * pxstride);
    orig_buf += sz - 1;
    while(sz--) {
      *new_buf = *orig_buf;
      new_buf -= pxstride;
      orig_buf--;
    }
  } else {
    while(sz--) {
      *new_buf = *orig_buf;
      new_buf += pxstride;
      orig_buf++;
    }
  }
}

static void
stridemove_demux (guchar *new_buf, guchar *orig_buf, gint new_width, gint orig_width, gint height, gint pxstride)
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
      memmove_demux (new_buf+(new_width*row), orig_buf+(orig_width*row), orig_width, pxstride);
    }
  } else {
    for (row=0; row<height; row++) {
      memmove_demux (new_buf+(new_width*row), orig_buf+(orig_width*row), new_width, pxstride);
    }
  }
}

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
      memmove (new_buf+(new_width*row), orig_buf+(orig_width*row), new_width);
    }
  }
}

/*
 * Conversion Functions:
 */

/** convert 4:2:0 semiplanar to same 4:2:0 semiplanar */
static GstFlowReturn
unstridify_420sp_420sp (GstStrideTransform *self, guchar *unstrided, guchar *strided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->in_rowstride;

  g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);

  stridemove (unstrided, strided, width, stride,
      (GST_ROUND_UP_2 (height) * 3) / 2);

  return GST_FLOW_OK;
}
static GstFlowReturn
stridify_420sp_420sp (GstStrideTransform *self, guchar *strided, guchar *unstrided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->out_rowstride;

  g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);

  g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);
  stridemove (strided, unstrided, stride, width,
      (GST_ROUND_UP_2 (height) * 3) / 2);

  return GST_FLOW_OK;
}

/** convert 4:2:0 planar to same 4:2:0 planar */
static GstFlowReturn
unstridify_420p_420p (GstStrideTransform *self, guchar *unstrided, guchar *strided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->in_rowstride;

  g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);

  stridemove (unstrided, strided, width, stride, height);   /* move Y */
  stridemove (
      unstrided + (height*width),
      strided + (height*stride),
      width/2, stride, height);                             /* move V/U */
  /* XXX odd widths/heights/strides: */
  stridemove (
      unstrided + (int)(height*width*1.5),
      strided + (int)(height*stride*1.5),
      width/2, stride, height);                             /* move U/V */

  return GST_FLOW_OK;
}
static GstFlowReturn
stridify_420p_420p (GstStrideTransform *self, guchar *strided, guchar *unstrided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->out_rowstride;

  g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);

  /* XXX odd widths/heights/strides: */
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
}

/** convert 4:2:2 packed to same 4:2:2 packed */
static GstFlowReturn
unstridify_422i_422i (GstStrideTransform *self, guchar *unstrided, guchar *strided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->in_rowstride;

  g_return_val_if_fail (stride >= (width*2), GST_FLOW_ERROR);

  stridemove (unstrided, strided, width*2, stride, height);

  return GST_FLOW_OK;
}
static GstFlowReturn
stridify_422i_422i (GstStrideTransform *self, guchar *strided, guchar *unstrided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->out_rowstride;

  g_return_val_if_fail (stride >= (width*2), GST_FLOW_ERROR);

  stridemove (strided, unstrided, stride, width*2, height);

  return GST_FLOW_OK;
}

/** convert I420 unstrided to NV12 strided */
static GstFlowReturn
stridify_i420_nv12 (GstStrideTransform *self, guchar *strided, guchar *unstrided)
{
  gint width  = self->width;
  gint height = self->height;
  gint stride = self->out_rowstride;

  g_return_val_if_fail (stride >= width, GST_FLOW_ERROR);

  /* note: if not an in-place conversion, then doing the U&V in one pass
   * would be more efficient... but if it is an in-place conversion, I'd
   * need to think about whether it is potential for the new UV plane to
   * corrupt the V plane before it is done copying..
   */
  stridemove_demux (
      strided + (height*stride) + 1,
      unstrided + (int)(height*width*1.25),
      stride, width/2, height/2, 2);                        /* move V */
  stridemove_demux (
      strided + (height*stride),
      unstrided + (height*width),
      stride, width/2, height/2, 2);                        /* move U */
  stridemove (strided, unstrided, stride, width, height);   /* move Y */

  return GST_FLOW_OK;
}

/* last entry has GST_VIDEO_FORMAT_UNKNOWN for in/out formats */
Conversion stride_conversions[] = {
  { { GST_VIDEO_FORMAT_NV12, GST_VIDEO_FORMAT_NV12 }, stridify_420sp_420sp, unstridify_420sp_420sp },
  { { GST_VIDEO_FORMAT_I420, GST_VIDEO_FORMAT_I420 }, stridify_420p_420p,   unstridify_420p_420p },
  { { GST_VIDEO_FORMAT_YV12, GST_VIDEO_FORMAT_YV12 }, stridify_420p_420p,   unstridify_420p_420p },
  { { GST_VIDEO_FORMAT_YUY2, GST_VIDEO_FORMAT_YUY2 }, stridify_422i_422i,   unstridify_422i_422i },
  { { GST_VIDEO_FORMAT_UYVY, GST_VIDEO_FORMAT_UYVY }, stridify_422i_422i,   unstridify_422i_422i },
  { { GST_VIDEO_FORMAT_I420, GST_VIDEO_FORMAT_NV12 }, stridify_i420_nv12,   NULL },
  /* add new entries before here */
  { { GST_VIDEO_FORMAT_UNKNOWN } }
};


