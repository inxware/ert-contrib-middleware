/*
 * Copyright (C) 2007-2009 Nokia Corporation.
 *
 * Author: Felipe Contreras <felipe.contreras@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "gstomx_base_filter.h"
#include "gstomx.h"
#include "gstomx_interface.h"

enum
{
    ARG_0,
    ARG_COMPONENT_ROLE,
    ARG_COMPONENT_NAME,
    ARG_LIBRARY_NAME,
    ARG_USE_TIMESTAMPS,
    ARG_NUM_INPUT_BUFFERS,
    ARG_NUM_OUTPUT_BUFFERS,
};

static void init_interfaces (GType type);
GSTOMX_BOILERPLATE_FULL (GstOmxBaseFilter, gst_omx_base_filter, GstElement, GST_TYPE_ELEMENT, init_interfaces);


static GstFlowReturn push_buffer (GstOmxBaseFilter *self, GstBuffer *buf);
static GstFlowReturn pad_chain (GstPad *pad, GstBuffer *buf);
static gboolean pad_event (GstPad *pad, GstEvent *event);


static void
setup_ports (GstOmxBaseFilter *self)
{
    OMX_PARAM_PORTDEFINITIONTYPE param;

    /* Input port configuration. */

    G_OMX_PORT_GET_DEFINITION (self->in_port, &param);
    g_omx_port_setup (self->in_port, &param);
    gst_pad_set_element_private (self->sinkpad, self->in_port);

    /* Output port configuration. */

    G_OMX_PORT_GET_DEFINITION (self->out_port, &param);
    g_omx_port_setup (self->out_port, &param);
    gst_pad_set_element_private (self->srcpad, self->out_port);

    if (g_getenv ("OMX_ALLOCATE_ON"))
    {
        GST_DEBUG_OBJECT (self, "OMX_ALLOCATE_ON");
        self->in_port->omx_allocate = TRUE;
        self->out_port->omx_allocate = TRUE;
        self->in_port->share_buffer = FALSE;
        self->out_port->share_buffer = FALSE;
    }
    else if (g_getenv ("OMX_SHARE_HACK_ON"))
    {
        GST_DEBUG_OBJECT (self, "OMX_SHARE_HACK_ON");
        self->in_port->share_buffer = TRUE;
        self->out_port->share_buffer = TRUE;
    }
    else if (g_getenv ("OMX_SHARE_HACK_OFF"))
    {
        GST_DEBUG_OBJECT (self, "OMX_SHARE_HACK_OFF");
        self->in_port->share_buffer = FALSE;
        self->out_port->share_buffer = FALSE;
    }

    GST_DEBUG_OBJECT (self, "in_port->omx_allocate=%d, out_port->omx_allocate=%d",
            self->in_port->omx_allocate, self->out_port->omx_allocate);
    GST_DEBUG_OBJECT (self, "in_port->share_buffer=%d, out_port->share_buffer=%d",
            self->in_port->share_buffer, self->out_port->share_buffer);
}

static GstStateChangeReturn
change_state (GstElement *element,
              GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstOmxBaseFilter *self;
    GOmxCore *core;

    self = GST_OMX_BASE_FILTER (element);
    core = self->gomx;

    GST_INFO_OBJECT (self, "begin: changing state %s -> %s",
                     gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
                     gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

    switch (transition)
    {
        case GST_STATE_CHANGE_NULL_TO_READY:
            g_omx_core_init (core);
            if (core->omx_state != OMX_StateLoaded)
            {
                ret = GST_STATE_CHANGE_FAILURE;
                goto leave;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            self->in_port->enabled = TRUE;
            g_omx_port_resume (self->in_port);
            self->out_port->enabled = TRUE;
            g_omx_port_resume (self->out_port);
            break;            
        default:
            break;
    }

    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

    if (ret == GST_STATE_CHANGE_FAILURE)
        goto leave;

    switch (transition)
    {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            g_mutex_lock (self->ready_lock);
            if (self->ready)
            {
                g_omx_core_flush_start (core);
                g_omx_core_flush_stop (core);
            }
            g_mutex_unlock (self->ready_lock);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            g_mutex_lock (self->ready_lock);
            if (self->ready)
            {
                /* unlock */
                g_omx_port_finish (self->in_port);
                g_omx_port_finish (self->out_port);

                g_omx_core_stop (core);
                g_omx_core_unload (core);
                self->ready = FALSE;
            }
            g_mutex_unlock (self->ready_lock);
            if (core->omx_state != OMX_StateLoaded &&
                core->omx_state != OMX_StateInvalid)
            {
                ret = GST_STATE_CHANGE_FAILURE;
                goto leave;
            }
            break;

        case GST_STATE_CHANGE_READY_TO_NULL:
        {
            gboolean in_allocate, in_share, out_allocate, out_share;
            GstBuffer * (*out_alloc)(GOmxPort *port, gint len);

            /* g_omx_core_deinit deallocates ports, so we need to save and
             * restore state */
            in_allocate = self->in_port->omx_allocate;
            in_share = self->in_port->share_buffer;
            out_allocate = self->out_port->omx_allocate;
            out_share = self->out_port->share_buffer;
            out_alloc = self->out_port->buffer_alloc;

            g_omx_core_deinit (core);

            self->in_port = g_omx_core_get_port (self->gomx, "in", 0);
            self->in_port->omx_allocate = in_allocate;
            self->in_port->share_buffer = in_share;

            self->out_port = g_omx_core_get_port (self->gomx, "out", 1);
            self->out_port->omx_allocate = out_allocate;
            self->out_port->share_buffer = out_share;
            self->out_port->buffer_alloc = out_alloc;

            break;
        }

        default:
            break;
    }

leave:
    GST_LOG_OBJECT (self, "end");

    return ret;
}

static void
finalize (GObject *obj)
{
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (obj);

    if (self->codec_data)
    {
        gst_buffer_unref (self->codec_data);
        self->codec_data = NULL;
    }

    g_omx_core_free (self->gomx);

    g_free (self->omx_role);
    g_free (self->omx_component);
    g_free (self->omx_library);

    g_mutex_free (self->ready_lock);

    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
set_property (GObject *obj,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (obj);

    switch (prop_id)
    {
        case ARG_COMPONENT_ROLE:
            g_free (self->omx_role);
            self->omx_role = g_value_dup_string (value);
            break;
        case ARG_COMPONENT_NAME:
            g_free (self->omx_component);
            self->omx_component = g_value_dup_string (value);
            break;
        case ARG_LIBRARY_NAME:
            g_free (self->omx_library);
            self->omx_library = g_value_dup_string (value);
            break;
        case ARG_USE_TIMESTAMPS:
            self->gomx->use_timestamps = g_value_get_boolean (value);
            break;
        case ARG_NUM_INPUT_BUFFERS:
        case ARG_NUM_OUTPUT_BUFFERS:
            {
                OMX_PARAM_PORTDEFINITIONTYPE param;
                OMX_U32 nBufferCountActual = g_value_get_uint (value);
                GOmxPort *port = (prop_id == ARG_NUM_INPUT_BUFFERS) ?
                        self->in_port : self->out_port;

                G_OMX_PORT_GET_DEFINITION (port, &param);

                g_return_if_fail (nBufferCountActual >= param.nBufferCountMin);
                param.nBufferCountActual = nBufferCountActual;

                G_OMX_PORT_SET_DEFINITION (port, &param);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
            break;
    }
}

static void
get_property (GObject *obj,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (obj);

    switch (prop_id)
    {
        case ARG_COMPONENT_ROLE:
            g_value_set_string (value, self->omx_role);
            break;
        case ARG_COMPONENT_NAME:
            g_value_set_string (value, self->omx_component);
            break;
        case ARG_LIBRARY_NAME:
            g_value_set_string (value, self->omx_library);
            break;
        case ARG_USE_TIMESTAMPS:
            g_value_set_boolean (value, self->gomx->use_timestamps);
            break;
        case ARG_NUM_INPUT_BUFFERS:
        case ARG_NUM_OUTPUT_BUFFERS:
            {
                OMX_PARAM_PORTDEFINITIONTYPE param;
                GOmxPort *port = (prop_id == ARG_NUM_INPUT_BUFFERS) ?
                        self->in_port : self->out_port;

                G_OMX_PORT_GET_DEFINITION (port, &param);

                g_value_set_uint (value, param.nBufferCountActual);
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
            break;
    }
}

static void
type_base_init (gpointer g_class)
{
}

static void
type_class_init (gpointer g_class,
                 gpointer class_data)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstOmxBaseFilterClass *bclass;

    gobject_class = G_OBJECT_CLASS (g_class);
    gstelement_class = GST_ELEMENT_CLASS (g_class);
    bclass = GST_OMX_BASE_FILTER_CLASS (g_class);

    gobject_class->finalize = finalize;
    gstelement_class->change_state = change_state;
    bclass->push_buffer = push_buffer;
    bclass->pad_chain = pad_chain;
    bclass->pad_event = pad_event;

    /* Properties stuff */
    {
        gobject_class->set_property = set_property;
        gobject_class->get_property = get_property;

        g_object_class_install_property (gobject_class, ARG_COMPONENT_ROLE,
                                         g_param_spec_string ("component-role", "Component role",
                                                              "Role of the OpenMAX IL component",
                                                              NULL, G_PARAM_READWRITE));

        g_object_class_install_property (gobject_class, ARG_COMPONENT_NAME,
                                         g_param_spec_string ("component-name", "Component name",
                                                              "Name of the OpenMAX IL component to use",
                                                              NULL, G_PARAM_READWRITE));

        g_object_class_install_property (gobject_class, ARG_LIBRARY_NAME,
                                         g_param_spec_string ("library-name", "Library name",
                                                              "Name of the OpenMAX IL implementation library to use",
                                                              NULL, G_PARAM_READWRITE));

        g_object_class_install_property (gobject_class, ARG_USE_TIMESTAMPS,
                                         g_param_spec_boolean ("use-timestamps", "Use timestamps",
                                                               "Whether or not to use timestamps",
                                                               TRUE, G_PARAM_READWRITE));

        /* note: the default values for these are just a guess.. since we wouldn't know
         * until the OMX component is constructed.  But that is ok, these properties are
         * only for debugging
         */
        g_object_class_install_property (gobject_class, ARG_NUM_INPUT_BUFFERS,
                                         g_param_spec_uint ("input-buffers", "Input buffers",
                                                            "The number of OMX input buffers",
                                                            1, 10, 4, G_PARAM_READWRITE));
        g_object_class_install_property (gobject_class, ARG_NUM_OUTPUT_BUFFERS,
                                         g_param_spec_uint ("output-buffers", "Output buffers",
                                                            "The number of OMX output buffers",
                                                            1, 10, 4, G_PARAM_READWRITE));
    }
}

static GstFlowReturn
push_buffer (GstOmxBaseFilter *self,
             GstBuffer *buf)
{
    GstFlowReturn ret;

    GST_BUFFER_DURATION (buf) = self->duration;

    PRINT_BUFFER (self, buf);

    /** @todo check if tainted */
    GST_LOG_OBJECT (self, "begin");
    ret = gst_pad_push (self->srcpad, buf);
    GST_LOG_OBJECT (self, "end");

    return ret;
}

static void
output_loop (gpointer data)
{
    GstPad *pad;
    GOmxCore *gomx;
    GOmxPort *out_port;
    GstOmxBaseFilter *self;
    GstFlowReturn ret = GST_FLOW_OK;
    GstOmxBaseFilterClass *bclass;

    pad = data;
    self = GST_OMX_BASE_FILTER (gst_pad_get_parent (pad));
    gomx = self->gomx;

    bclass = GST_OMX_BASE_FILTER_GET_CLASS (self);

    GST_LOG_OBJECT (self, "begin");

    if (!self->ready)
    {
        g_error ("not ready");
        return;
    }

    out_port = self->out_port;

    if (G_LIKELY (out_port->enabled))
    {
        gpointer obj = g_omx_port_recv (out_port);

        if (G_UNLIKELY (!obj))
        {
            GST_WARNING_OBJECT (self, "null buffer: leaving");
            ret = GST_FLOW_WRONG_STATE;
            goto leave;
        }

        if (G_LIKELY (GST_IS_BUFFER (obj)))
        {
            if (G_UNLIKELY (GST_BUFFER_FLAG_IS_SET (obj, GST_BUFFER_FLAG_IN_CAPS)))
            {
                GstCaps *caps = NULL;
                GstStructure *structure;
                GValue value = { 0 };

                caps = gst_pad_get_negotiated_caps (self->srcpad);
                caps = gst_caps_make_writable (caps);
                structure = gst_caps_get_structure (caps, 0);

                g_value_init (&value, GST_TYPE_BUFFER);
                gst_value_set_buffer (&value, obj);
                gst_buffer_unref (obj);
                gst_structure_set_value (structure, "codec_data", &value);
                g_value_unset (&value);

                gst_pad_set_caps (self->srcpad, caps);
            }
            else
            {
                GstBuffer *buf = GST_BUFFER (obj);
                ret = bclass->push_buffer (self, buf);
                GST_DEBUG_OBJECT (self, "ret=%s", gst_flow_get_name (ret));
            }
        }
        else if (GST_IS_EVENT (obj))
        {
            GST_DEBUG_OBJECT (self, "got eos");
            gst_pad_push_event (self->srcpad, obj);
            ret = GST_FLOW_UNEXPECTED;
            goto leave;
        }

    }

leave:

    self->last_pad_push_return = ret;

    if (gomx->omx_error != OMX_ErrorNone)
    {
        GST_DEBUG_OBJECT (self, "omx_error=%s", g_omx_error_to_str (gomx->omx_error));
        ret = GST_FLOW_ERROR;
    }

    if (ret != GST_FLOW_OK)
    {
        GST_INFO_OBJECT (self, "pause task, reason:  %s",
                         gst_flow_get_name (ret));
        gst_pad_pause_task (self->srcpad);

    }

    GST_LOG_OBJECT (self, "end");

    gst_object_unref (self);
}

static GstFlowReturn
pad_chain (GstPad *pad,
           GstBuffer *buf)
{
    GOmxCore *gomx;
    GOmxPort *in_port;
    GstOmxBaseFilter *self;
    GstFlowReturn ret = GST_FLOW_OK;

    self = GST_OMX_BASE_FILTER (GST_OBJECT_PARENT (pad));

    PRINT_BUFFER (self, buf);

    gomx = self->gomx;

    GST_LOG_OBJECT (self, "begin: size=%u, state=%d", GST_BUFFER_SIZE (buf), gomx->omx_state);

    if (G_UNLIKELY (gomx->omx_state == OMX_StateLoaded))
    {
        g_mutex_lock (self->ready_lock);

        GST_INFO_OBJECT (self, "omx: prepare");

        /** @todo this should probably go after doing preparations. */
        if (self->omx_setup)
        {
            self->omx_setup (self);
        }

        setup_ports (self);

        g_omx_core_prepare (self->gomx);

        if (gomx->omx_state == OMX_StateIdle)
        {
            self->ready = TRUE;
            gst_pad_start_task (self->srcpad, output_loop, self->srcpad);
        }

        g_mutex_unlock (self->ready_lock);

        if (gomx->omx_state != OMX_StateIdle)
            goto out_flushing;
    }

    in_port = self->in_port;

    if (G_LIKELY (in_port->enabled))
    {
        if (G_UNLIKELY (gomx->omx_state == OMX_StateIdle))
        {
            GST_INFO_OBJECT (self, "omx: play");
            g_omx_core_start (gomx);

            if (gomx->omx_state != OMX_StateExecuting)
                goto out_flushing;

            /* send buffer with codec data flag */
            if (self->codec_data)
            {
                GST_BUFFER_FLAG_SET (self->codec_data, GST_BUFFER_FLAG_IN_CAPS);  /* just in case */
                g_omx_port_send (in_port, self->codec_data);
            }

        }

        if (G_UNLIKELY (gomx->omx_state != OMX_StateExecuting))
        {
            GST_ERROR_OBJECT (self, "Whoa! very wrong");
        }

        while (TRUE)
        {
            gint sent;

            if (self->last_pad_push_return != GST_FLOW_OK ||
                !(gomx->omx_state == OMX_StateExecuting ||
                  gomx->omx_state == OMX_StatePause))
            {
                GST_DEBUG_OBJECT (self, "last_pad_push_return=%d", self->last_pad_push_return);
                goto out_flushing;
            }

            sent = g_omx_port_send (in_port, buf);

            if (G_UNLIKELY (sent < 0))
            {
                ret = GST_FLOW_WRONG_STATE;
                goto out_flushing;
            }
            else if (sent < GST_BUFFER_SIZE (buf))
            {
                GstBuffer *subbuf = gst_buffer_create_sub (buf, sent,
                        GST_BUFFER_SIZE (buf) - sent);
                gst_buffer_unref (buf);
                buf = subbuf;
            }
            else
            {
                gst_buffer_unref (buf);
                break;
            }
        }
    }
    else
    {
        GST_WARNING_OBJECT (self, "done");
        ret = GST_FLOW_UNEXPECTED;
    }

leave:

    GST_LOG_OBJECT (self, "end");

    return ret;

    /* special conditions */
out_flushing:
    {
        const gchar *error_msg = NULL;

        if (gomx->omx_error)
        {
            error_msg = "Error from OpenMAX component";
        }
        else if (gomx->omx_state != OMX_StateExecuting &&
                 gomx->omx_state != OMX_StatePause)
        {
            error_msg = "OpenMAX component in wrong state";
        }

        if (error_msg)
        {
            GST_ELEMENT_ERROR (self, STREAM, FAILED, (NULL), (error_msg));
            ret = GST_FLOW_ERROR;
        }

        gst_buffer_unref (buf);

        goto leave;
    }
}

static gboolean
pad_event (GstPad *pad,
           GstEvent *event)
{
    GstOmxBaseFilter *self;
    GOmxCore *gomx;
    gboolean ret = TRUE;

    self = GST_OMX_BASE_FILTER (GST_OBJECT_PARENT (pad));
    gomx = self->gomx;

    GST_INFO_OBJECT (self, "begin: event=%s", GST_EVENT_TYPE_NAME (event));

    switch (GST_EVENT_TYPE (event))
    {
        case GST_EVENT_EOS:
            /* if we are init'ed, and there is a running loop; then
             * if we get a buffer to inform it of EOS, let it handle the rest
             * in any other case, we send EOS */
            if (self->ready && self->last_pad_push_return == GST_FLOW_OK)
            {
                if (g_omx_port_send (self->in_port, event) >= 0)
                {
                    gst_event_unref (event);
                    break;
                }
            }

            /* we tried, but it's up to us here */
            ret = gst_pad_push_event (self->srcpad, event);
            break;

        case GST_EVENT_FLUSH_START:
            gst_pad_push_event (self->srcpad, event);
            self->last_pad_push_return = GST_FLOW_WRONG_STATE;

            g_omx_core_flush_start (gomx);

            gst_pad_pause_task (self->srcpad);

            ret = TRUE;
            break;

        case GST_EVENT_FLUSH_STOP:
            gst_pad_push_event (self->srcpad, event);
            self->last_pad_push_return = GST_FLOW_OK;

            g_omx_core_flush_stop (gomx);

            if (self->ready)
                gst_pad_start_task (self->srcpad, output_loop, self->srcpad);

            ret = TRUE;
            break;

        case GST_EVENT_NEWSEGMENT:
            ret = gst_pad_push_event (self->srcpad, event);
            break;

        default:
            ret = gst_pad_push_event (self->srcpad, event);
            break;
    }

    GST_LOG_OBJECT (self, "end");

    return ret;
}

static gboolean
activate_push (GstPad *pad,
               gboolean active)
{
    gboolean result = TRUE;
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (gst_pad_get_parent (pad));

    if (active)
    {
        GST_DEBUG_OBJECT (self, "activate");
        self->last_pad_push_return = GST_FLOW_OK;

        /* we do not start the task yet if the pad is not connected */
        if (gst_pad_is_linked (pad))
        {
            if (self->ready)
            {
                /** @todo link callback function also needed */
                g_omx_port_resume (self->in_port);
                g_omx_port_resume (self->out_port);

                result = gst_pad_start_task (pad, output_loop, pad);
            }
        }
    }
    else
    {
        GST_DEBUG_OBJECT (self, "deactivate");

        if (self->ready)
        {
            /** @todo disable this until we properly reinitialize the buffers. */
#if 0
            /* flush all buffers */
            OMX_SendCommand (self->gomx->omx_handle, OMX_CommandFlush, OMX_ALL, NULL);
#endif

            /* unlock loops */
            g_omx_port_pause (self->in_port);
            g_omx_port_pause (self->out_port);
        }

        /* make sure streaming finishes */
        result = gst_pad_stop_task (pad);
    }

    gst_object_unref (self);

    return result;
}

/**
 * overrides the default buffer allocation for output port to allow
 * pad_alloc'ing from the srcpad
 */
static GstBuffer *
buffer_alloc (GOmxPort *port, gint len)
{
    GstOmxBaseFilter *self = port->core->object;
    GstBuffer *buf;
    GstFlowReturn ret;

#if 1
    /** @todo remove this check */
    if (G_LIKELY (self->in_port->enabled))
    {
        GstCaps *caps = NULL;

        caps = gst_pad_get_negotiated_caps (self->srcpad);

        if (!caps)
        {
            /** @todo We shouldn't be doing this. */
            GOmxCore *gomx = self->gomx;
            GST_WARNING_OBJECT (self, "faking settings changed notification");
            if (gomx->settings_changed_cb)
                gomx->settings_changed_cb (gomx);
        }
        else
        {
            GST_LOG_OBJECT (self, "caps already fixed: %" GST_PTR_FORMAT, caps);
            gst_caps_unref (caps);
        }
    }
#endif

    ret = gst_pad_alloc_buffer_and_set_caps (
            self->srcpad, GST_BUFFER_OFFSET_NONE,
            len, GST_PAD_CAPS (self->srcpad), &buf);

    if (ret == GST_FLOW_OK) return buf;

    return NULL;
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *self;
    GstElementClass *element_class;
    GstOmxBaseFilterClass *bclass;

    element_class = GST_ELEMENT_CLASS (g_class);
    bclass = GST_OMX_BASE_FILTER_CLASS (g_class);

    self = GST_OMX_BASE_FILTER (instance);

    GST_LOG_OBJECT (self, "begin");

    /* GOmx */
    self->gomx = g_omx_core_new (self, g_class);
    self->in_port = g_omx_core_get_port (self->gomx, "in", 0);
    self->out_port = g_omx_core_get_port (self->gomx, "out", 1);

    self->out_port->buffer_alloc = buffer_alloc;

    self->in_port->omx_allocate = TRUE;
    self->out_port->omx_allocate = TRUE;
    self->in_port->share_buffer = FALSE;
    self->out_port->share_buffer = FALSE;

    self->ready_lock = g_mutex_new ();

    self->sinkpad =
        gst_pad_new_from_template (gst_element_class_get_pad_template (element_class, "sink"), "sink");

    gst_pad_set_chain_function (self->sinkpad, bclass->pad_chain);
    gst_pad_set_event_function (self->sinkpad, bclass->pad_event);

    self->srcpad =
        gst_pad_new_from_template (gst_element_class_get_pad_template (element_class, "src"), "src");

    gst_pad_set_activatepush_function (self->srcpad, activate_push);

    gst_pad_use_fixed_caps (self->srcpad);

    gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);
    gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

    self->duration = GST_CLOCK_TIME_NONE;

    GST_LOG_OBJECT (self, "end");
}

static void
omx_interface_init (GstImplementsInterfaceClass *klass)
{
}

static gboolean
interface_supported (GstImplementsInterface *iface,
                     GType type)
{
    g_assert (type == GST_TYPE_OMX);
    return TRUE;
}

static void
interface_init (GstImplementsInterfaceClass *klass)
{
    klass->supported = interface_supported;
}

static void
init_interfaces (GType type)
{
    GInterfaceInfo *iface_info;
    GInterfaceInfo *omx_info;


    iface_info = g_new0 (GInterfaceInfo, 1);
    iface_info->interface_init = (GInterfaceInitFunc) interface_init;

    g_type_add_interface_static (type, GST_TYPE_IMPLEMENTS_INTERFACE, iface_info);
    g_free (iface_info);

    omx_info = g_new0 (GInterfaceInfo, 1);
    omx_info->interface_init = (GInterfaceInitFunc) omx_interface_init;

    g_type_add_interface_static (type, GST_TYPE_OMX, omx_info);
    g_free (omx_info);
}

