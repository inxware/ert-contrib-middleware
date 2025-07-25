/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Wim Taymans <wim@fluendo.com>
 *
 * gstsystemclock.h: A clock implementation based on system time
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


#ifndef __GST_SYSTEM_CLOCK_H__
#define __GST_SYSTEM_CLOCK_H__

#include <gst/gstclock.h>

G_BEGIN_DECLS

#define GST_TYPE_SYSTEM_CLOCK                   (gst_system_clock_get_type ())
#define GST_SYSTEM_CLOCK(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_SYSTEM_CLOCK, GstSystemClock))
#define GST_SYSTEM_CLOCK_CAST(obj)              ((GstSystemClock *)(obj))
#define GST_IS_SYSTEM_CLOCK(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_SYSTEM_CLOCK))
#define GST_SYSTEM_CLOCK_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_SYSTEM_CLOCK, GstSystemClockClass))
#define GST_IS_SYSTEM_CLOCK_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_SYSTEM_CLOCK))
#define GST_SYSTEM_CLOCK_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_SYSTEM_CLOCK, GstSystemClockClass))


typedef struct _GstSystemClock GstSystemClock;
typedef struct _GstSystemClockClass GstSystemClockClass;
typedef struct _GstSystemClockPrivate GstSystemClockPrivate;

/**
 * GstClockType:
 * @GST_CLOCK_TYPE_REALTIME: time since Epoch
 * @GST_CLOCK_TYPE_MONOTONIC: monotonic time since some unspecified starting
 *                            point
 *
 * The different kind of clocks.
 */
typedef enum {
  GST_CLOCK_TYPE_REALTIME       = 0,
  GST_CLOCK_TYPE_MONOTONIC      = 1
} GstClockType;

/**
 * GstSystemClock:
 *
 * The default implementation of a #GstClock that uses the system time.
 */
struct _GstSystemClock {
  GstClock       clock;

  /*< private >*/
  GThread       *thread;        /* thread for async notify */
  gboolean       stopping;

  /* ABI added */
  GstSystemClockPrivate *priv;

  gpointer _gst_reserved[GST_PADDING];
};

struct _GstSystemClockClass {
  GstClockClass  parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

GType                   gst_system_clock_get_type       (void);

GstClock*               gst_system_clock_obtain         (void);

G_END_DECLS

#endif /* __GST_SYSTEM_CLOCK_H__ */
