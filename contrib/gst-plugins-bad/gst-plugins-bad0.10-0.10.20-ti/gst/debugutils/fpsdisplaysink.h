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

#ifndef __FPS_DISPLAY_SINK_H__
#define __FPS_DISPLAY_SINK_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_FPS_DISPLAY_SINK \
  (fps_display_sink_get_type())
#define GST_FPS_DISPLAY_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_FPS_DISPLAY_SINK,GstFPSDisplaySink))
#define GST_FPS_DISPLAY_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_FPS_DISPLAY_SINK,GstFPSDisplaySinkClass))
#define GST_IS_FPS_DISPLAY_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_FPS_DISPLAY_SINK))
#define GST_IS_FPS_DISPLAY_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_FPS_DISPLAY_SINK))

GType fps_display_sink_get_type (void);

typedef struct _GstFPSDisplaySink GstFPSDisplaySink;
typedef struct _GstFPSDisplaySinkClass GstFPSDisplaySinkClass;

struct _GstFPSDisplaySink
{
  GstBin bin;

  /*< private >*/
  /* gstreamer components */
  GstElement *text_overlay;
  GstElement *video_sink;
  GstPad *ghost_pad;

  /* statistics */
  guint64 frames_rendered, last_frames_rendered;
  guint64 frames_dropped, last_frames_dropped;

  GstClockTime start_ts;
  GstClockTime last_ts;
  guint timeout_id;
  guint data_probe_id;

  /* properties */
  gboolean sync;
  gboolean use_text_overlay;
  gboolean signal_measurements;
  gint fps_update_interval;
  gdouble max_fps;
  gdouble min_fps;
};

struct _GstFPSDisplaySinkClass
{
  GstBinClass parent_class;
};

G_END_DECLS

#endif /* __FPS_DISPLAY_SINK_H__ */
