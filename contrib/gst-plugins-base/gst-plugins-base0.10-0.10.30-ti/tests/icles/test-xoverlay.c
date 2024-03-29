/* GStreamer
 * Copyright (C) <2008> Stefan Kost <ensonic@users.sf.net>
 *
 * test-xoverlay: test xoverlay custom event handling and subregions
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
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <gst/video/gstvideosink.h>

#if !GTK_CHECK_VERSION (2, 17, 7)
static void
gtk_widget_get_allocation (GtkWidget * w, GtkAllocation * a)
{
  *a = w->allocation;
}
#endif

static struct
{
  gint w, h;
  GstXOverlay *overlay;
  GtkWidget *widget;
  gdouble a, p;
  GstVideoRectangle rect;
  gboolean running;
} anim_state;

static gboolean verbose = FALSE;

static gboolean
animate_render_rect (gpointer user_data)
{
  if (anim_state.running) {
    GstVideoRectangle *r = &anim_state.rect;
    gdouble s = sin (3.0 * anim_state.a);
    gdouble c = cos (2.0 * anim_state.a);

    anim_state.a += anim_state.p;
    if (anim_state.a > (M_PI + M_PI))
      anim_state.a -= (M_PI + M_PI);

    r->w = anim_state.w / 2;
    r->x = (r->w - (r->w / 2)) + c * (r->w / 2);
    r->h = anim_state.h / 2;
    r->y = (r->h - (r->h / 2)) + s * (r->h / 2);

    gst_x_overlay_set_render_rectangle (anim_state.overlay, r->x, r->y,
        r->w, r->h);
    gtk_widget_queue_draw (anim_state.widget);
  }
  return TRUE;
}

static gboolean
handle_resize_cb (GtkWidget * widget, GdkEventConfigure * event,
    gpointer user_data)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);

  if (verbose) {
    g_print ("resize(%p): %dx%d\n", widget, allocation.width,
        allocation.height);
  }
  anim_state.w = allocation.width;
  anim_state.h = allocation.height;
  animate_render_rect (NULL);

  return FALSE;
}

static gboolean
handle_expose_cb (GtkWidget * widget, GdkEventExpose * event,
    gpointer user_data)
{
  GstVideoRectangle *r = &anim_state.rect;
  GtkAllocation allocation;
  GdkWindow *window;
  GtkStyle *style;

  style = gtk_widget_get_style (widget);
  window = gtk_widget_get_window (widget);
  gtk_widget_get_allocation (widget, &allocation);

  /* we should only redraw outside of the video rect! */
  /*
     gdk_draw_rectangle (widget->window, widget->style->bg_gc[0], TRUE,
     0, 0, widget->allocation.width, widget->allocation.height);
     gdk_draw_rectangle (widget->window, widget->style->bg_gc[0], TRUE,
     event->area.x, event->area.y, event->area.width, event->area.height);
   */
  gdk_draw_rectangle (window, style->bg_gc[0], TRUE,
      0, event->area.y, r->x, event->area.height);
  gdk_draw_rectangle (window, style->bg_gc[0], TRUE,
      r->x + r->w, event->area.y,
      allocation.width - (r->x + r->w), event->area.height);

  gdk_draw_rectangle (window, style->bg_gc[0], TRUE,
      event->area.x, 0, event->area.width, r->y);
  gdk_draw_rectangle (window, style->bg_gc[0], TRUE,
      event->area.x, r->y + r->h,
      event->area.width, allocation.height - (r->y + r->h));
  if (verbose) {
    g_print ("expose(%p)\n", widget);
  }
  gst_x_overlay_expose (anim_state.overlay);
  return FALSE;
}

static void
window_closed (GtkWidget * widget, GdkEvent * event, gpointer user_data)
{
  GstElement *pipeline = user_data;

  if (verbose) {
    g_print ("stopping\n");
  }
  anim_state.running = FALSE;
  gtk_widget_hide_all (widget);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gtk_main_quit ();
}

gint
main (gint argc, gchar ** argv)
{
  GdkWindow *video_window_xwindow;
  GtkWidget *window, *video_window;
  GstElement *pipeline, *src, *sink;
  GstStateChangeReturn sret;
  gulong embed_xid = 0;
  gboolean force_aspect = FALSE, draw_borders = FALSE;

  if (!g_thread_supported ())
    g_thread_init (NULL);

  gst_init (&argc, &argv);
  gtk_init (&argc, &argv);

  if (argc) {
    gint arg;
    for (arg = 0; arg < argc; arg++) {
      if (!strcmp (argv[arg], "-a"))
        force_aspect = TRUE;
      else if (!strcmp (argv[arg], "-b"))
        draw_borders = TRUE;
      else if (!strcmp (argv[arg], "-v"))
        verbose = TRUE;
    }
  }

  /* prepare the pipeline */

  pipeline = gst_pipeline_new ("xvoverlay");
  src = gst_element_factory_make ("videotestsrc", NULL);
  sink = gst_element_factory_make ("xvimagesink", NULL);
  gst_bin_add_many (GST_BIN (pipeline), src, sink, NULL);
  gst_element_link (src, sink);

  g_object_set (G_OBJECT (sink), "handle-events", FALSE,
      "force-aspect-ratio", force_aspect, "draw-borders", draw_borders, NULL);

  /* prepare the ui */

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete-event",
      G_CALLBACK (window_closed), (gpointer) pipeline);
  gtk_window_set_default_size (GTK_WINDOW (window), 320, 240);

  video_window = gtk_drawing_area_new ();
  gtk_widget_set_double_buffered (video_window, FALSE);
  gtk_container_add (GTK_CONTAINER (window), video_window);

  /* show the gui and play */
  gtk_widget_show_all (window);

  /* realize window now so that the video window gets created and we can
   * obtain its XID before the pipeline is started up and the videosink
   * asks for the XID of the window to render onto */
  gtk_widget_realize (window);

  video_window_xwindow = gtk_widget_get_window (video_window);
  embed_xid = GDK_WINDOW_XID (video_window_xwindow);
  if (verbose) {
    g_print ("Window realize: got XID %lu\n", embed_xid);
  }

  /* we know what the video sink is in this case (xvimagesink), so we can
   * just set it directly here now (instead of waiting for a prepare-xwindow-id
   * element message in a sync bus handler and setting it there) */
  gst_x_overlay_set_window_handle (GST_X_OVERLAY (sink), embed_xid);

  anim_state.overlay = GST_X_OVERLAY (sink);
  anim_state.widget = video_window;
  anim_state.w = 320;
  anim_state.h = 240;
  anim_state.a = 0.0;
  anim_state.p = (M_PI + M_PI) / 200.0;

  handle_resize_cb (video_window, NULL, sink);
  g_signal_connect (video_window, "configure-event",
      G_CALLBACK (handle_resize_cb), NULL);
  g_signal_connect (video_window, "expose-event",
      G_CALLBACK (handle_expose_cb), NULL);

  g_timeout_add (50, (GSourceFunc) animate_render_rect, NULL);

  /* run the pipeline */
  sret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  if (sret == GST_STATE_CHANGE_FAILURE)
    gst_element_set_state (pipeline, GST_STATE_NULL);
  else {
    anim_state.running = TRUE;
    gtk_main ();
  }

  gst_object_unref (pipeline);
  return 0;
}
