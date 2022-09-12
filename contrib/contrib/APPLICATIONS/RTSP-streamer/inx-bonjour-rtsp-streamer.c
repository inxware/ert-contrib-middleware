/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
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

modification made by inx limited on behalf of Kcatalix Ltd to include:
1. Command line changes to the gstreamer configuration
2. Provide method to define the published name
3. Added bonjour support 19-05-2011 - this is not yet synchronised with RTSP mode.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* begin avahi-bonjour includes */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <avahi-core/core.h>
#include <avahi-core/publish.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/alternative.h>
#include <avahi-common/error.h>

#include <pthread.h>

/* end avahi-bonjour includes */

/* begin gstreamer includes   */
#include <string.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

/* define this if you want the resource to only be available when using
 * user/admin as the password */
#undef WITH_AUTH
/* end of gstreamer includes  */



static AvahiSEntryGroup *group = NULL;
static AvahiSimplePoll *simple_poll = NULL;
static char *name = NULL;

static void create_services(AvahiServer *s);

static void entry_group_callback(AvahiServer *s, AvahiSEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
    assert(s);
    assert(g == group);

    /* Called whenever the entry group state changes */

    switch (state) {

        case AVAHI_ENTRY_GROUP_ESTABLISHED:

            /* The entry group has been established successfully */
            fprintf(stderr, "Service '%s' successfully established.\n", name);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION: {
            char *n;

            /* A service name collision happened. Let's pick a new name */
            n = avahi_alternative_service_name(name);
            avahi_free(name);
            name = n;

            fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

            /* And recreate the services */
            create_services(s);
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            fprintf(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_server_errno(s)));

            /* Some kind of failure happened while we were registering our services */
            avahi_simple_poll_quit(simple_poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

static void create_services(AvahiServer *s) {
    char r[128];
    int ret;
    assert(s);

    /* If this is the first time we're called, let's create a new entry group */
    if (!group)
        if (!(group = avahi_s_entry_group_new(s, entry_group_callback, NULL))) {
            fprintf(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_server_errno(s)));
            goto fail;
        }

    fprintf(stderr, "Adding service '%s'\n", name);

    /* Create some random TXT data */
    snprintf(r, sizeof(r), "random=%i", rand());

    /* Add the service for IPP */
    if ((ret = avahi_server_add_service(s, group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, "INX RTSP", "_rtsp._tcp", NULL, NULL, 8554, "INX RTSP", r, NULL)) < 0) {
        fprintf(stderr, "Failed to add _rtsp._tcp service: %s\n", avahi_strerror(ret));
        goto fail;
    }
    /* Tell the server to register the service */
    if ((ret = avahi_s_entry_group_commit(group)) < 0) {
        fprintf(stderr, "Failed to commit entry_group: %s\n", avahi_strerror(ret));
        goto fail;
    }

    return;

fail:
    avahi_simple_poll_quit(simple_poll);
}

static void server_callback(AvahiServer *s, AvahiServerState state, AVAHI_GCC_UNUSED void * userdata) {
    assert(s);
    /* Called whenever the server state changes */
    switch (state) {

        case AVAHI_SERVER_RUNNING:
            /* The serve has startup successfully and registered its host
             * name on the network, so it's time to create our services */

            if (!group)
                create_services(s);

            break;

        case AVAHI_SERVER_COLLISION: {
            char *n;
            int r;

            /* A host name collision happened. Let's pick a new name for the server */
            n = avahi_alternative_host_name(avahi_server_get_host_name(s));
            fprintf(stderr, "Host name collision, retrying with '%s'\n", n);
            r = avahi_server_set_host_name(s, n);
            avahi_free(n);

            if (r < 0) {
                fprintf(stderr, "Failed to set new host name: %s\n", avahi_strerror(r));

                avahi_simple_poll_quit(simple_poll);
                return;
            }

        }

            /* Fall through */

        case AVAHI_SERVER_REGISTERING:

	    /* Let's drop our registered services. When the server is back
             * in AVAHI_SERVER_RUNNING state we will register them
             * again with the new host name. */
            if (group)
                avahi_s_entry_group_reset(group);

            break;

        case AVAHI_SERVER_FAILURE:

            /* Terminate on failure */

            fprintf(stderr, "Server failure: %s\n", avahi_strerror(avahi_server_errno(s)));
            avahi_simple_poll_quit(simple_poll);
            break;

        case AVAHI_SERVER_INVALID:
            ;
    }
}

void * start_bonjour(void *nothing) {
    AvahiServerConfig config;
    AvahiServer *server = NULL;
    int error;
    int ret = 1;

    /* Initialize the pseudo-RNG */
    srand(time(NULL));

    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new())) {
        fprintf(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

    name = avahi_strdup("INX");

    /* Let's set the host name for this server. */
    avahi_server_config_init(&config);
    config.host_name = avahi_strdup("INX RTSP Server");
    config.publish_workstation = 0;

    /* Allocate a new server */
    server = avahi_server_new(avahi_simple_poll_get(simple_poll), &config, server_callback, NULL, &error);

    /* Free the configuration data */
    avahi_server_config_free(&config);

    /* Check wether creating the server object succeeded */
    if (!server) {
        fprintf(stderr, "Failed to create server: %s\n", avahi_strerror(error));
        goto fail;
    }

    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);

    ret = 0;

fail:

    /* Cleanup things */

    if (server)
        avahi_server_free(server);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);

    avahi_free(name);

    pthread_exit(NULL);

    return NULL;
}


void start_bonjour_thread() {
	pthread_t thread;
	pthread_create(&thread, NULL, start_bonjour, NULL);

}




/* this timeout is periodically run to clean up the expired sessions from the
 * pool. This needs to be run explicitly currently but might be done
 * automatically as part of the mainloop. */
static gboolean
timeout (GstRTSPServer * server, gboolean ignored)
{
  GstRTSPSessionPool *pool;

  pool = gst_rtsp_server_get_session_pool (server);
  gst_rtsp_session_pool_cleanup (pool);
  g_object_unref (pool);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMediaMapping *mapping;
  GstRTSPMediaFactory *factory;
#ifdef WITH_AUTH
  GstRTSPAuth *auth;
  gchar *basic;
#endif
	char* pipeline;
	char* fullpipeline;

	if (argc < 1) {
		printf("\n No parameters provided - Use run-me-test-video.sh with no arguments for default streaming config\n Exiting...\n");
	}

  	gst_init (&argc, &argv);
	/* add the gst pipleine commands */
	pipeline=argv[1];
	fullpipeline= (char*)malloc((strlen(pipeline)+3)*sizeof(char));
	fullpipeline[0]='(';
	fullpipeline[1]='\0';
	strcat(fullpipeline,pipeline);
	strcat(fullpipeline," )");

  loop = g_main_loop_new (NULL, FALSE);

  /* create a server instance */
  server = gst_rtsp_server_new ();

  /* get the mapping for this server, every server has a default mapper object
   * that be used to map uri mount points to media factories */
  mapping = gst_rtsp_server_get_media_mapping (server);

#ifdef WITH_AUTH
  /* make a new authentication manager. it can be added to control access to all
   * the factories on the server or on individual factories. */
  auth = gst_rtsp_auth_new ();
  basic = gst_rtsp_auth_make_basic ("user", "admin");
  gst_rtsp_auth_set_basic (auth, basic);
  g_free (basic);
  /* configure in the server */
  gst_rtsp_server_set_auth (server, auth);
#endif

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines. 
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory, fullpipeline);
  /* tell the media factory to share the pipeline between clients, this is used because the omx plugins can not be initialised more than once */
  gst_rtsp_media_factory_set_shared(factory,1);	
  /* attach the test factory to the /test url */
  gst_rtsp_media_mapping_add_factory (mapping, "/INXRTSP", factory); //TODO this should be parametised probably

  /* don't need the ref to the mapper anymore */
  g_object_unref (mapping);

  /* attach the server to the default maincontext */
  if (gst_rtsp_server_attach (server, NULL) == 0)
    goto failed;

  /* add a timeout for the session cleanup */
  g_timeout_add_seconds (2, (GSourceFunc) timeout, server);

  start_bonjour_thread(); //@todo need to make this sync with streams

  /* start serving, this never stops */
  g_main_loop_run (loop);
  return 0;

  /* ERRORS */
failed:
  {
    g_print ("failed to attach the server\n");
    return -1;
  }
}
