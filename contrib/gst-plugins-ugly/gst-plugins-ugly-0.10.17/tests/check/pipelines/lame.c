/* GStreamer
 *
 * unit test for lame
 *
 * Copyright (C) 2007 Thomas Vander Stichele <thomas at apestaart dot org>
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

#include <gst/check/gstcheck.h>
#include <gst/check/gstbufferstraw.h>

#ifndef GST_DISABLE_PARSE

GST_START_TEST (test_format)
{
  GstElement *bin;
  GstPad *pad;
  gchar *pipe_str;
  GstBuffer *buffer;
  GError *error = NULL;

  pipe_str = g_strdup_printf ("audiotestsrc num-buffers=1 "
      "! audio/x-raw-int, rate=22050, channels=1 "
      "! lame bitrate=24 mode=3 " "! audio/mpeg,rate=22050 ! fakesink");

  bin = gst_parse_launch (pipe_str, &error);
  fail_unless (bin != NULL, "Error parsing pipeline: %s",
      error ? error->message : "(invalid error)");
  g_free (pipe_str);

  /* get the pad */
  {
    GstElement *sink = gst_bin_get_by_name (GST_BIN (bin), "fakesink0");

    fail_unless (sink != NULL, "Could not get fakesink out of bin");
    pad = gst_element_get_static_pad (sink, "sink");
    fail_unless (pad != NULL, "Could not get pad out of fakesink");
    gst_object_unref (sink);
  }

  gst_buffer_straw_start_pipeline (bin, pad);

  buffer = gst_buffer_straw_get_buffer (bin, pad);

  gst_buffer_straw_stop_pipeline (bin, pad);

  gst_buffer_unref (buffer);
  gst_object_unref (pad);
  gst_object_unref (bin);
}

GST_END_TEST;

#endif /* #ifndef GST_DISABLE_PARSE */

Suite *
lame_suite (void)
{
  Suite *s = suite_create ("lame");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

#ifndef GST_DISABLE_PARSE
  tcase_add_test (tc_chain, test_format);
#endif

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = lame_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
