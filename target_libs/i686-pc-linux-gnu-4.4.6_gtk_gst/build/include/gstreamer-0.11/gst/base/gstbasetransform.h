/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2005 Wim Taymans <wim@fluendo.com>
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

#ifndef __GST_BASE_TRANSFORM_H__
#define __GST_BASE_TRANSFORM_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_BASE_TRANSFORM		   (gst_base_transform_get_type())
#define GST_BASE_TRANSFORM(obj)		   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_BASE_TRANSFORM,GstBaseTransform))
#define GST_BASE_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_BASE_TRANSFORM,GstBaseTransformClass))
#define GST_BASE_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_BASE_TRANSFORM,GstBaseTransformClass))
#define GST_IS_BASE_TRANSFORM(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_BASE_TRANSFORM))
#define GST_IS_BASE_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_BASE_TRANSFORM))
/* since 0.10.4 */
#define GST_BASE_TRANSFORM_CAST(obj)	((GstBaseTransform *)(obj))

/**
 * GST_BASE_TRANSFORM_SINK_NAME:
 *
 * The name of the templates for the sink pad.
 */
#define GST_BASE_TRANSFORM_SINK_NAME	"sink"
/**
 * GST_BASE_TRANSFORM_SRC_NAME:
 *
 * The name of the templates for the source pad.
 */
#define GST_BASE_TRANSFORM_SRC_NAME	"src"

/**
 * GST_BASE_TRANSFORM_SRC_PAD:
 * @obj: base transform instance
 *
 * Gives the pointer to the source #GstPad object of the element.
 *
 * Since: 0.10.4
 */
#define GST_BASE_TRANSFORM_SRC_PAD(obj)		(GST_BASE_TRANSFORM_CAST (obj)->srcpad)

/**
 * GST_BASE_TRANSFORM_SINK_PAD:
 * @obj: base transform instance
 *
 * Gives the pointer to the sink #GstPad object of the element.
 *
 * Since: 0.10.4
 */
#define GST_BASE_TRANSFORM_SINK_PAD(obj)	(GST_BASE_TRANSFORM_CAST (obj)->sinkpad)

/**
 * GST_BASE_TRANSFORM_FLOW_DROPPED:
 *
 * A #GstFlowReturn that can be returned from transform and transform_ip to
 * indicate that no output buffer was generated.
 *
 * Since: 0.10.13
 */
#define GST_BASE_TRANSFORM_FLOW_DROPPED   GST_FLOW_CUSTOM_SUCCESS

/**
 * GST_BASE_TRANSFORM_LOCK:
 * @obj: base transform instance
 *
 * Obtain a lock to protect the transform function from concurrent access.
 *
 * Since: 0.10.13
 */
#define GST_BASE_TRANSFORM_LOCK(obj)   g_mutex_lock (GST_BASE_TRANSFORM_CAST (obj)->transform_lock)

/**
 * GST_BASE_TRANSFORM_UNLOCK:
 * @obj: base transform instance
 *
 * Release the lock that protects the transform function from concurrent access.
 *
 * Since: 0.10.13
 */
#define GST_BASE_TRANSFORM_UNLOCK(obj) g_mutex_unlock (GST_BASE_TRANSFORM_CAST (obj)->transform_lock)

typedef struct _GstBaseTransform GstBaseTransform;
typedef struct _GstBaseTransformClass GstBaseTransformClass;
typedef struct _GstBaseTransformPrivate GstBaseTransformPrivate;

/**
 * GstBaseTransform:
 *
 * The opaque #GstBaseTransform data structure.
 */
struct _GstBaseTransform {
  GstElement	 element;

  /*< protected >*/
  /* source and sink pads */
  GstPad	*sinkpad;
  GstPad	*srcpad;

  /* Set by sub-class */
  gboolean	 passthrough;
  gboolean	 always_in_place;

  GstCaps	*cache_caps1;
  gsize		 cache_caps1_size;
  GstCaps	*cache_caps2;
  gsize		 cache_caps2_size;
  gboolean	 have_same_caps;

  gboolean	 negotiated;

  gboolean       have_segment;

  /* MT-protected (with STREAM_LOCK) */
  GstSegment     segment;

  GMutex	*transform_lock;

  /*< private >*/
  GstBaseTransformPrivate *priv;

  gpointer       _gst_reserved[GST_PADDING_LARGE];
};

/**
 * GstBaseTransformClass:
 * @parent_class:   Element parent class
 * @passthrough_on_same_caps: If set to TRUE, passthrough mode will be
 *                            automatically enabled if the caps are the same.
 * @transform_caps: Optional.  Given the pad in this direction and the given
 *                  caps, what caps are allowed on the other pad in this
 *                  element ?
 * @fixate_caps:    Optional. Given the pad in this direction and the given
 *                  caps, fixate the caps on the other pad.
 * @accept_caps:    Optional. Since 0.10.30
 *                  Subclasses can override this method to check if @caps can be
 *                  handled by the element. The default implementation might not be
 *                  the most optimal way to check this in all cases.
 * @set_caps:       allows the subclass to be notified of the actual caps set.
 * @query:          Optional Since 0.10.36
 *                  Handle a requested query. Subclasses that implement this
 *                  should must chain up to the parent if they didn't handle the
 *                  query
 * @propose_allocation: Propose buffer allocation parameters for upstream elements.
 *                      This function is only called when not operating in
 *                      passthrough mode. The default implementation is NULL.
 * @decide_allocation: Setup the allocation parameters for allocating output
 *                    buffers. The passed in query contains the result of the
 *                    downstream allocation query. This function is only called
 *                    when not operating in passthrough mode. The default
 *                    implementation is NULL.
 * @transform_size: Optional. Given the size of a buffer in the given direction
 *                  with the given caps, calculate the size in bytes of a buffer
 *                  on the other pad with the given other caps.
 *                  The default implementation uses get_unit_size and keeps
 *                  the number of units the same.
 * @get_unit_size:  Required if the transform is not in-place.
 *                  get the size in bytes of one unit for the given caps.
 * @start:          Optional.
 *                  Called when the element starts processing.
 *                  Allows opening external resources.
 * @stop:           Optional.
 *                  Called when the element stops processing.
 *                  Allows closing external resources.
 * @sink_event:     Optional.
 *                  Event handler on the sink pad. The default implementation
 *                  handles the event and forwards it downstream.
 * @src_event:      Optional.
 *                  Event handler on the source pad. The default implementation
 *                  handles the event and forwards it upstream.
 * @prepare_output_buffer: Optional.
 *                         Subclasses can override this to do their own
 *                         allocation of output buffers.  Elements that only do
 *                         analysis can return a subbuffer or even just
 *                         return a reference to the input buffer (if in
 *                         passthrough mode). The default implementation will
 *                         use the negotiated allocator or bufferpool and
 *                         transform_size to allocate an output buffer or it
 *                         will return the input buffer in passthrough mode.
 * @copy_metadata: Optional.
 *                 Copy the metadata from the input buffer to the output buffer.
 *                 The default implementation will copy the flags, timestamps and
 *                 offsets of the buffer.
 * @before_transform: Optional. Since 0.10.22
 *                    This method is called right before the base class will
 *                    start processing. Dynamic properties or other delayed
 *                    configuration could be performed in this method.
 * @transform:      Required if the element does not operate in-place.
 *                  Transforms one incoming buffer to one outgoing buffer.
 *                  The function is allowed to change size/timestamp/duration
 *                  of the outgoing buffer.
 * @transform_ip:   Required if the element operates in-place.
 *                  Transform the incoming buffer in-place.
 *
 * Subclasses can override any of the available virtual methods or not, as
 * needed. At minimum either @transform or @transform_ip need to be overridden.
 * If the element can overwrite the input data with the results (data is of the
 * same type and quantity) it should provide @transform_ip.
 */
struct _GstBaseTransformClass {
  GstElementClass parent_class;

  /*< public >*/
  gboolean       passthrough_on_same_caps;

  /* virtual methods for subclasses */
  GstCaps*	(*transform_caps) (GstBaseTransform *trans,
                                   GstPadDirection direction,
                                   GstCaps *caps, GstCaps *filter);
  void		(*fixate_caps)	  (GstBaseTransform *trans,
                                   GstPadDirection direction, GstCaps *caps,
                                   GstCaps *othercaps);
  gboolean      (*accept_caps)    (GstBaseTransform *trans, GstPadDirection direction,
                                   GstCaps *caps);
  gboolean      (*set_caps)       (GstBaseTransform *trans, GstCaps *incaps,
                                   GstCaps *outcaps);
  gboolean      (*query)          (GstBaseTransform *trans, GstPadDirection direction,
                                   GstQuery *query);

  /* propose allocation query parameters for input buffers */
  gboolean      (*propose_allocation) (GstBaseTransform *trans, GstQuery *query);
  /* decide allocation query for output buffers */
  gboolean      (*decide_allocation) (GstBaseTransform *trans, GstQuery *query);

  /* transform size */
  gboolean      (*transform_size) (GstBaseTransform *trans,
                                   GstPadDirection direction,
                                   GstCaps *caps, gsize size,
                                   GstCaps *othercaps, gsize *othersize);

  gboolean      (*get_unit_size)  (GstBaseTransform *trans, GstCaps *caps,
                                   gsize *size);

  /* states */
  gboolean      (*start)        (GstBaseTransform *trans);
  gboolean      (*stop)         (GstBaseTransform *trans);

  /* sink and src pad event handlers */
  gboolean      (*sink_event)   (GstBaseTransform *trans, GstEvent *event);
  gboolean      (*src_event)    (GstBaseTransform *trans, GstEvent *event);

  GstFlowReturn (*prepare_output_buffer) (GstBaseTransform * trans,
                                          GstBuffer *input, GstBuffer **outbuf);

  gboolean      (*copy_metadata)     (GstBaseTransform * trans, GstBuffer *input,
                                      GstBuffer *outbuf);

  void          (*before_transform)  (GstBaseTransform *trans, GstBuffer *buffer);

  /* transform */
  GstFlowReturn (*transform)    (GstBaseTransform *trans, GstBuffer *inbuf,
                                 GstBuffer *outbuf);
  GstFlowReturn (*transform_ip) (GstBaseTransform *trans, GstBuffer *buf);

  /*< private >*/
  gpointer       _gst_reserved[GST_PADDING_LARGE];
};

GType           gst_base_transform_get_type         (void);

void		gst_base_transform_set_passthrough  (GstBaseTransform *trans,
	                                             gboolean passthrough);
gboolean	gst_base_transform_is_passthrough   (GstBaseTransform *trans);

void		gst_base_transform_set_in_place     (GstBaseTransform *trans,
	                                             gboolean in_place);
gboolean	gst_base_transform_is_in_place      (GstBaseTransform *trans);

void		gst_base_transform_update_qos       (GstBaseTransform *trans,
						     gdouble proportion,
						     GstClockTimeDiff diff,
						     GstClockTime timestamp);
void		gst_base_transform_set_qos_enabled  (GstBaseTransform *trans,
		                                     gboolean enabled);
gboolean	gst_base_transform_is_qos_enabled   (GstBaseTransform *trans);

void            gst_base_transform_set_gap_aware    (GstBaseTransform *trans,
                                                     gboolean gap_aware);

void		gst_base_transform_suggest          (GstBaseTransform *trans,
	                                             GstCaps *caps, gsize size);
void		gst_base_transform_reconfigure      (GstBaseTransform *trans);
G_END_DECLS

#endif /* __GST_BASE_TRANSFORM_H__ */
