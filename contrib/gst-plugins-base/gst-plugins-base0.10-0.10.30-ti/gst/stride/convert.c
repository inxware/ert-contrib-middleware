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


/* note: some parts of code support in-place transform.. some do not..  I'm
 * not sure if zip/interleave functions could really support in-place copy..
 * I need to think about this after having some sleep ;-)
 */

#define WEAK __attribute__((weak))

/*
 * Conversion utilities:
 */

void stride_copy_zip2 (guchar * new_buf, guchar * orig_buf1,
    guchar * orig_buf2, gint sz);
void stride_copy_zip3a (guchar * new_buf, guchar * orig_buf1,
    guchar * orig_buf2, guchar * orig_buf3, gint sz);
void stride_copy (guchar * new_buf, guchar * orig_buf, gint sz);

WEAK void
stride_copy_zip2 (guchar * out, guchar * in1, guchar * in2, gint sz)
{
  while (sz--) {
    *out++ = *in1++;
    *out++ = *in2++;
  }
}

WEAK void
stride_copy_zip3a (guchar * out,
    guchar * in1, guchar * in2, guchar * in3, gint sz)
{
  while (sz > 1) {
    *out++ = *in1++;
    *out++ = *in2++;
    *out++ = *in1++;
    *out++ = *in3++;
    sz -= 2;
  }
}

WEAK void
stride_copy (guchar * out, guchar * in, gint sz)
{
  memcpy (out, in, sz);
}


/**
 * move to strided buffer, interleaving two planes of identical dimensions
 */
static void
stridemove_zip2 (guchar * out, guchar * in1, guchar * in2,
    gint out_bpl, gint in_bpl, gint width, gint height)
{
  int row;

  GST_DEBUG
      ("out=%p, in1=%p, in2=%p, out_bpl=%d, in_bpl=%d, width=%d, height=%d",
      out, in1, in2, out_bpl, in_bpl, width, height);

  for (row = 0; row < height; row++) {
    stride_copy_zip2 (out + (out_bpl * row),
        in1 + (in_bpl * row),
        in2 + (in_bpl * row), width);
  }
}

/**
 * move to strided buffer, interleaving three planes, where the first plane
 * (orig_buf1) has 2x as many samples.. Ie. ABACABAC..
 */
static void
stridemove_zip3a (guchar * out,
    guchar * in1, guchar * in2, guchar * in3,
    guint out_bpl, gint in_bpl, gint width, gint height)
{
  GST_DEBUG
      ("out=%p, in1=%p, in2=%p, in3=%p, out_bpl=%d, in_bpl=%d, width=%d, height=%d",
      out, in1, in2, in3, out_bpl, in_bpl, width, height);

  while (height > 0) {

    /* even row */
    stride_copy_zip3a (out, in1, in2, in3, width);
    out += out_bpl;
    in1 += in_bpl;

    /* odd row, recycles same U & V */
    stride_copy_zip3a (out, in1, in2, in3, width);
    out += out_bpl;
    in1 += in_bpl;

    in2 += in_bpl / 2;
    in3 += in_bpl / 2;

    height -= 2;
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
stridemove (guchar * out, guchar * in, gint out_bpl, gint in_bpl,
    gint width, gint height)
{
  int row;

  GST_DEBUG ("out=%p, in=%p, out_bpl=%d, in_bpl=%d, width=%d, height=%d",
      out, in, out_bpl, in_bpl, width, height);

  for (row = 0; row < height; row++) {
    stride_copy (out, in, width);
    out += out_bpl;
    in  += in_bpl;
  }
}

/*
 * Conversion Functions:
 */

/**
 * helper to calculate offsets/sizes that are re-used for each frame (until
 * caps or crop changes)
 * @isx:  input sub-sampling in x direction
 * @osx:  output sub-sampling in x direction
 * @isy:  input sub-sampling in y direction
 * @isx:  input sub-sampling in y direction
 */
static inline gboolean refresh_cache(GstStrideTransform * self,
    gint nplanes, gint bpp, gint * isx, gint * osx, gint * isy, gint * osy)
{
  gint in_off, out_off;
  int i;

  if (((self->crop_top + self->crop_height) > self->height) ||
      ((self->crop_left + self->crop_width) > self->width)) {
    GST_ERROR_OBJECT (self, "invalid crop parameter");
    return GST_FLOW_ERROR;
  }

  in_off = out_off = 0;

  for (i = 0; i < nplanes; i++) {
    Cache * cache = &self->cache[i];

    cache->in_bpl = self->in_rowstride ?
        self->in_rowstride : bpp * self->width;

    cache->out_bpl = self->out_rowstride ?
        self->out_rowstride : bpp * self->width;

    if ((cache->in_bpl < (self->width * bpp)) ||
        (cache->out_bpl < (self->width * bpp))) {
      GST_ERROR_OBJECT (self, "invalid stride parameter");
      return GST_FLOW_ERROR;
    }

    cache->width = self->crop_width ?
        self->crop_width : self->width;

    cache->height = self->crop_height ?
        self->crop_height : self->height;

    if ((cache->width > self->width) ||
        (cache->height > self->height)) {
      GST_ERROR_OBJECT (self, "invalid crop width/height parameter");
      return GST_FLOW_ERROR;
    }

    /* note: everything above here is same for each plane, so in theory we
     * could only calculate on first plane, and copy on subsequent planes
     */

    /* adjust for sub-sampling and bytes per pixel (bpp): */
    cache->in_bpl /= *isx;
    cache->out_bpl /= *osx;
    cache->width *= bpp;
    cache->width /= *isx;
    cache->height /= *isy;

    /* calculate offset to beginning of data to copy/transform: */
    cache->in_off = in_off;
    cache->in_off += (bpp * self->crop_left / *isx) +
        (cache->in_bpl * self->crop_top / *isy);

    cache->out_off = out_off;
    cache->out_off += (bpp * self->crop_left / *osx) +
        (cache->out_bpl * self->crop_top / *osy);

    in_off += (self->height / *isy) * cache->in_bpl;
    out_off += (self->height / *osy) * cache->out_bpl;

    osx++;
    isx++;
    osy++;
    isy++;
  }

  return GST_FLOW_OK;
}

/** perform simple convert between buffers of same format */
static inline GstFlowReturn convert_n_n (GstStrideTransform *self,
    guchar * out, guchar * in, gint nplanes)
{
  int i;

  for (i = 0; i < nplanes; i++) {
    stridemove (out + self->cache[i].out_off, in + self->cache[i].in_off,
        self->cache[i].out_bpl, self->cache[i].in_bpl,
        self->cache[i].width, self->cache[i].height);
  }

  return GST_FLOW_OK;
}

/** convert 4:2:0 semiplanar to same 4:2:0 semiplanar */
static GstFlowReturn
convert_420sp_420sp (GstStrideTransform * self,
    guchar * out, guchar * in)
{
  if (G_UNLIKELY (self->needs_refresh)) {
    gint sx[] = {1, 1};
    gint sy[] = {1, 2};
    if (refresh_cache (self, 2, 1, sx, sx, sy, sy))
      return GST_FLOW_ERROR;
    self->needs_refresh = FALSE;
  }

  return convert_n_n (self, out, in, 2);
}

/** convert 4:2:0 planar to same 4:2:0 planar */
static GstFlowReturn
convert_420p_420p (GstStrideTransform * self,
    guchar * out, guchar * in)
{
  if (G_UNLIKELY (self->needs_refresh)) {
    gint sx[] = {1, 2, 2};
    gint sy[] = {1, 2, 2};
    if (refresh_cache (self, 3, 1, sx, sx, sy, sy))
      return GST_FLOW_ERROR;
    self->needs_refresh = FALSE;
  }

  return convert_n_n (self, out, in, 3);
}

/** convert 4:2:2 packed to same 4:2:2 packed */

static GstFlowReturn
convert_422i_422i (GstStrideTransform * self,
    guchar * out, guchar * in)
{
  if (G_UNLIKELY (self->needs_refresh)) {
    gint sx[] = {1};
    gint sy[] = {1};
    if (refresh_cache (self, 1, 2, sx, sx, sy, sy))
      return GST_FLOW_ERROR;
    self->needs_refresh = FALSE;
  }

  return convert_n_n (self, out, in, 1);
}

/** convert I420 unstrided to NV12 strided */
static GstFlowReturn
convert_i420_nv12 (GstStrideTransform * self,
    guchar * out, guchar * in)
{
  GstFlowReturn ret;

  if (G_UNLIKELY (self->needs_refresh)) {
    gint isx[] = {1, 2, 2};
    gint osx[] = {1, 1, 1};
    gint sy[]  = {1, 2, 2};
    if (refresh_cache (self, 3, 1, isx, osx, sy, sy))
      return GST_FLOW_ERROR;
    self->needs_refresh = FALSE;
  }

  ret = convert_n_n (self, out, in, 1);
  if (ret != GST_FLOW_OK)
    return ret;

  stridemove_zip2 (out + self->cache[1].out_off,
      in + self->cache[1].in_off,      /* U */
      in + self->cache[2].in_off,      /* V */
      self->cache[2].out_bpl,
      self->cache[1].in_bpl,
      self->cache[1].width,
      self->cache[1].height);

  return GST_FLOW_OK;
}

/** convert I420 unstrided to YUY2 strided */
static GstFlowReturn
convert_i420_yuy2 (GstStrideTransform * self,
    guchar * out, guchar * in)
{
  if (G_UNLIKELY (self->needs_refresh)) {
    gint sx[] = {1, 2, 2};
    gint sy[] = {1, 2, 2};
    if (refresh_cache (self, 3, 1, sx, sx, sy, sy))
      return GST_FLOW_ERROR;
    self->needs_refresh = FALSE;
  }

  stridemove_zip3a (out,
      in + self->cache[0].in_off,      /* Y */
      in + self->cache[1].in_off,      /* U */
      in + self->cache[2].in_off,      /* V */
      self->cache[0].out_bpl,
      self->cache[0].in_bpl,
      self->cache[0].width,
      self->cache[0].height);

  return GST_FLOW_OK;
}

/** convert 16bpp rgb formats */
static GstFlowReturn
convert_rgb16_rgb16 (GstStrideTransform * self,
    guchar * out, guchar * in)
{
  /* format is same 2-bytes per pixel */
  return convert_422i_422i (self, out, in);
}

/** convert 32bbp rgb formats */
static GstFlowReturn
convert_rgb32_rgb32 (GstStrideTransform * self,
    guchar * out, guchar * in)
{
  if (G_UNLIKELY (self->needs_refresh)) {
    gint sx[] = {1};
    gint sy[] = {1};
    if (refresh_cache (self, 1, 4, sx, sx, sy, sy))
      return GST_FLOW_ERROR;
    self->needs_refresh = FALSE;
  }

  return convert_n_n (self, out, in, 1);
}

#define CONVERT(tofmt, fromfmt, convert)                        \
		{                                                           \
      { GST_VIDEO_FORMAT_##tofmt, GST_VIDEO_FORMAT_##fromfmt }, \
      convert                                                   \
    }

/* last entry has GST_VIDEO_FORMAT_UNKNOWN for in/out formats */
const Conversion stride_conversions[] = {
  CONVERT (NV12, NV12, convert_420sp_420sp),
  CONVERT (I420, I420, convert_420p_420p),
  CONVERT (YV12, YV12, convert_420p_420p),
  CONVERT (YUY2, YUY2, convert_422i_422i),
  CONVERT (UYVY, UYVY, convert_422i_422i),
  CONVERT (I420, NV12, convert_i420_nv12),
  CONVERT (I420, YUY2, convert_i420_yuy2),
  CONVERT (RGB16, RGB16, convert_rgb16_rgb16),
  CONVERT (RGBx, RGBx, convert_rgb32_rgb32),
  CONVERT (BGRx, BGRx, convert_rgb32_rgb32),
  CONVERT (xRGB, xRGB, convert_rgb32_rgb32),
  CONVERT (xBGR, xBGR, convert_rgb32_rgb32),
  CONVERT (RGBA, RGBA, convert_rgb32_rgb32),
  CONVERT (BGRA, BGRA, convert_rgb32_rgb32),
  CONVERT (ARGB, ARGB, convert_rgb32_rgb32),
  CONVERT (ABGR, ABGR, convert_rgb32_rgb32),
  /* add new entries before here */
  {{GST_VIDEO_FORMAT_UNKNOWN}}
};
