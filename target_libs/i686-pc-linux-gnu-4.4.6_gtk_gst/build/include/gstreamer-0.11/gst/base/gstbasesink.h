/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *
 * gstbasesink.h:
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

#ifndef __GST_BASE_SINK_H__
#define __GST_BASE_SINK_H__

#include <gst/gst.h>

G_BEGIN_DECLS


#define GST_TYPE_BASE_SINK              (gst_base_sink_get_type())
#define GST_BASE_SINK(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_BASE_SINK,GstBaseSink))
#define GST_BASE_SINK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_BASE_SINK,GstBaseSinkClass))
#define GST_BASE_SINK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_BASE_SINK, GstBaseSinkClass))
#define GST_IS_BASE_SINK(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_BASE_SINK))
#define GST_IS_BASE_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_BASE_SINK))
#define GST_BASE_SINK_CAST(obj)         ((GstBaseSink *) (obj))

/**
 * GST_BASE_SINK_PAD:
 * @obj: base sink instance
 *
 * Gives the pointer to the #GstPad object of the element.
 */
#define GST_BASE_SINK_PAD(obj)          (GST_BASE_SINK_CAST (obj)->sinkpad)

#define GST_BASE_SINK_GET_PREROLL_LOCK(pad)   (GST_BASE_SINK_CAST(pad)->preroll_lock)
#define GST_BASE_SINK_PREROLL_LOCK(pad)       (g_mutex_lock(GST_BASE_SINK_GET_PREROLL_LOCK(pad)))
#define GST_BASE_SINK_PREROLL_TRYLOCK(pad)    (g_mutex_trylock(GST_BASE_SINK_GET_PREROLL_LOCK(pad)))
#define GST_BASE_SINK_PREROLL_UNLOCK(pad)     (g_mutex_unlock(GST_BASE_SINK_GET_PREROLL_LOCK(pad)))

#define GST_BASE_SINK_GET_PREROLL_COND(pad)   (GST_BASE_SINK_CAST(pad)->preroll_cond)
#define GST_BASE_SINK_PREROLL_WAIT(pad)       \
      g_cond_wait (GST_BASE_SINK_GET_PREROLL_COND (pad), GST_BASE_SINK_GET_PREROLL_LOCK (pad))
#define GST_BASE_SINK_PREROLL_TIMED_WAIT(pad, timeval) \
      g_cond_timed_wait (GST_BASE_SINK_GET_PREROLL_COND (pad), GST_BASE_SINK_GET_PREROLL_LOCK (pad), timeval)
#define GST_BASE_SINK_PREROLL_SIGNAL(pad)     g_cond_signal (GST_BASE_SINK_GET_PREROLL_COND (pad));
#define GST_BASE_SINK_PREROLL_BROADCAST(pad)  g_cond_broadcast (GST_BASE_SINK_GET_PREROLL_COND (pad));

typedef struct _GstBaseSink GstBaseSink;
typedef struct _GstBaseSinkClass GstBaseSinkClass;
typedef struct _GstBaseSinkPrivate GstBaseSinkPrivate;

/**
 * GstBaseSink:
 *
 * The opaque #GstBaseSink data structure.
 */
struct _GstBaseSink {
  GstElement     element;

  /*< protected >*/
  GstPad        *sinkpad;
  GstActivateMode       pad_mode;

  /*< protected >*/ /* with LOCK */
  guint64        offset;
  gboolean       can_activate_pull;
  gboolean       can_activate_push;

  /*< protected >*/ /* with PREROLL_LOCK */
  GMutex        *preroll_lock;
  GCond         *preroll_cond;
  gboolean       eos;
  gboolean       eos_queued;
  gboolean       need_preroll;
  gboolean       have_preroll;
  gboolean       playing_async;

  /*< protected >*/ /* with STREAM_LOCK */
  gboolean       have_newsegment;
  GstSegment     segment;
  GstSegment     clip_segment;

  /*< private >*/ /* with LOCK */
  GstClockID     clock_id;
  GstClockTime   end_time;
  gboolean       sync;
  gboolean       flushing;
  gboolean       running;

  gint64         max_lateness;

  /*< private >*/
  GstBaseSinkPrivate *priv;

  gpointer _gst_reserved[GST_PADDING_LARGE];
};

/**
 * GstBaseSinkClass:
 * @parent_class: Element parent class
 * @get_caps: Called to get sink pad caps from the subclass
 * @set_caps: Notify subclass of changed caps
 * @fixate: Only useful in pull mode, this vmethod will be called in response to
 *     gst_pad_fixate_caps() being called on the sink pad. Implement if you have
 *     ideas about what should be the default values for the caps you support.
 * @activate_pull: Subclasses should override this when they can provide an
 *     alternate method of spawning a thread to drive the pipeline in pull mode.
 *     Should start or stop the pulling thread, depending on the value of the
 *     "active" argument. Called after actually activating the sink pad in pull
 *     mode. The default implementation starts a task on the sink pad.
 * @get_times: Called to get the start and end times for synchronising
 *     the passed buffer to the clock
 * @propose_allocation: configure the allocation query
 * @start: Start processing. Ideal for opening resources in the subclass
 * @stop: Stop processing. Subclasses should use this to close resources.
 * @unlock: Unlock any pending access to the resource. Subclasses should
 *     unblock any blocked function ASAP
 * @unlock_stop: Clear the previous unlock request. Subclasses should clear
 *     any state they set during unlock(), such as clearing command queues.
 * @event: Override this to handle events arriving on the sink pad
 * @preroll: Called to present the preroll buffer if desired
 * @render: Called when a buffer should be presented or output, at the
 *     correct moment if the #GstBaseSink has been set to sync to the clock.
 * @render_list: Same as @render but used whith buffer lists instead of
 *     buffers. Since: 0.10.24
 * @query: perform a #GstQuery on the element. Since: 0.10.36
 *
 * Subclasses can override any of the available virtual methods or not, as
 * needed. At the minimum, the @render method should be overridden to
 * output/present buffers.
 */
struct _GstBaseSinkClass {
  GstElementClass parent_class;

  /* get caps from subclass */
  GstCaps*      (*get_caps)     (GstBaseSink *sink, GstCaps *filter);
  /* notify subclass of new caps */
  gboolean      (*set_caps)     (GstBaseSink *sink, GstCaps *caps);

  /* fixate sink caps during pull-mode negotiation */
  void          (*fixate)       (GstBaseSink *sink, GstCaps *caps);
  /* start or stop a pulling thread */
  gboolean      (*activate_pull)(GstBaseSink *sink, gboolean active);

  /* get the start and end times for syncing on this buffer */
  void          (*get_times)    (GstBaseSink *sink, GstBuffer *buffer,
                                 GstClockTime *start, GstClockTime *end);

  /* propose allocation parameters for upstream */
  gboolean      (*propose_allocation)   (GstBaseSink *sink, GstQuery *query);

  /* start and stop processing, ideal for opening/closing the resource */
  gboolean      (*start)        (GstBaseSink *sink);
  gboolean      (*stop)         (GstBaseSink *sink);

  /* unlock any pending access to the resource. subclasses should unlock
   * any function ASAP. */
  gboolean      (*unlock)       (GstBaseSink *sink);
  /* Clear a previously indicated unlock request not that unlocking is
   * complete. Sub-classes should clear any command queue or indicator they
   * set during unlock */
  gboolean      (*unlock_stop)  (GstBaseSink *sink);

  /* notify subclass of query */
  gboolean      (*query)        (GstBaseSink *sink, GstQuery *query);

  /* notify subclass of event, preroll buffer or real buffer */
  gboolean      (*event)        (GstBaseSink *sink, GstEvent *event);

  GstFlowReturn (*preroll)      (GstBaseSink *sink, GstBuffer *buffer);
  GstFlowReturn (*render)       (GstBaseSink *sink, GstBuffer *buffer);
  /* Render a BufferList */
  GstFlowReturn (*render_list)  (GstBaseSink *sink, GstBufferList *buffer_list);

  /*< private >*/
  gpointer       _gst_reserved[GST_PADDING_LARGE];
};

GType gst_base_sink_get_type(void);

GstFlowReturn   gst_base_sink_do_preroll        (GstBaseSink *sink, GstMiniObject *obj);
GstFlowReturn   gst_base_sink_wait_preroll      (GstBaseSink *sink);

/* synchronizing against the clock */
void            gst_base_sink_set_sync          (GstBaseSink *sink, gboolean sync);
gboolean        gst_base_sink_get_sync          (GstBaseSink *sink);

/* dropping late buffers */
void            gst_base_sink_set_max_lateness  (GstBaseSink *sink, gint64 max_lateness);
gint64          gst_base_sink_get_max_lateness  (GstBaseSink *sink);

/* performing QoS */
void            gst_base_sink_set_qos_enabled   (GstBaseSink *sink, gboolean enabled);
gboolean        gst_base_sink_is_qos_enabled    (GstBaseSink *sink);

/* doing async state changes */
void            gst_base_sink_set_async_enabled (GstBaseSink *sink, gboolean enabled);
gboolean        gst_base_sink_is_async_enabled  (GstBaseSink *sink);

/* tuning synchronisation */
void            gst_base_sink_set_ts_offset     (GstBaseSink *sink, GstClockTimeDiff offset);
GstClockTimeDiff gst_base_sink_get_ts_offset    (GstBaseSink *sink);

/* last buffer */
GstBuffer *     gst_base_sink_get_last_buffer   (GstBaseSink *sink);
void            gst_base_sink_set_last_buffer_enabled (GstBaseSink *sink, gboolean enabled);
gboolean        gst_base_sink_is_last_buffer_enabled (GstBaseSink *sink);

/* latency */
gboolean        gst_base_sink_query_latency     (GstBaseSink *sink, gboolean *live, gboolean *upstream_live,
                                                 GstClockTime *min_latency, GstClockTime *max_latency);
GstClockTime    gst_base_sink_get_latency       (GstBaseSink *sink);

/* render delay */
void            gst_base_sink_set_render_delay  (GstBaseSink *sink, GstClockTime delay);
GstClockTime    gst_base_sink_get_render_delay  (GstBaseSink *sink);

/* blocksize */
void            gst_base_sink_set_blocksize     (GstBaseSink *sink, guint blocksize);
guint           gst_base_sink_get_blocksize     (GstBaseSink *sink);

/* throttle-time */
void            gst_base_sink_set_throttle_time (GstBaseSink *sink, guint64 throttle);
guint64         gst_base_sink_get_throttle_time (GstBaseSink *sink);

GstClockReturn  gst_base_sink_wait_clock        (GstBaseSink *sink, GstClockTime time,
                                                 GstClockTimeDiff * jitter);
GstFlowReturn   gst_base_sink_wait_eos          (GstBaseSink *sink, GstClockTime time,
                                                 GstClockTimeDiff *jitter);

G_END_DECLS

#endif /* __GST_BASE_SINK_H__ */
