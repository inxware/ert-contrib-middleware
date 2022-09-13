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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <pthread.h>


/* begin gstreamer includes   */
#include <string.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

/* define this if you want the resource to only be available when using
 * user/admin as the password */
#undef WITH_AUTH
/* end of gstreamer includes  */



static char *name = NULL;

static unsigned short int gPortnumber=0;
static char* gStreamname=NULL;
static char gStreamPath[256]="/INXRTSP";
static char* gStreamArgs[5]={NULL,NULL,NULL,NULL,NULL};




/* this timeout is periodically run to clean up the expired sessions from the
 * pool. This needs to be run explicitly currently but might be done
 * automatically as part of the mainloop. */
static gboolean
timeout (GstRTSPServer * server, gboolean ignored)
{
  GstRTSPSessionPool *pool;
//  printf("->Entering gst RTSP session pool cleanup\n");
  pool = gst_rtsp_server_get_session_pool (server);
  gst_rtsp_session_pool_cleanup (pool);
  g_object_unref (pool);
//  printf("<-Exiting gst RTSP session pool cleanup\n");
  return TRUE;
}

int main (int argc, char *argv[]) {
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
	char* port;
	char* stream;
	int streamlength=0;
	char* text;
	char* cmdline_ipaddress;
	gchar* ipaddress=NULL;

	//Check that we have the correct number of arguments
	if (argc != 6) {
		printf("\n Incorrect number of arguments. Arguments should be \"port number\" \"stream name\" \"stream text\" \"gst pipeline\".\n");
		return -1;
	}

  	gst_init (&argc, &argv);
	
	//get arguments in meaningful variable names
	cmdline_ipaddress=argv[1];
	port=argv[2];
	stream=argv[3];
	text=argv[4];
	
	//test the length of the ip address argument, if none zero then copy it in to place
	if(strlen(cmdline_ipaddress)>0){
		ipaddress=(gchar*)malloc((strlen(cmdline_ipaddress)+1)*sizeof(char));
		strcpy((char*)ipaddress,cmdline_ipaddress);
	}

	//try to convert the port number to an integer
	gPortnumber=(unsigned short int)strtoul(port,NULL,10);
	if(gPortnumber==0){
		printf("First argument incorrect for a port number. Should be an integer value.\n");
		return -1;
	}
	//set the global stream name
	gStreamname=stream;
	//parse the text, should be pipe ( | ) separated key value pairs. i.e name=value|name2=value2|name3=value3
	//first get an array of strings separated by pipes, maximum of five
   	const char *ptr = text;
	char *ptr2 = NULL;
   	char field [ 256 ];
	char key[256];
	char value[256];
   	int n;
	int nn;
	int args_read=0;
	int valid_keyvaluepair=0;
	char* fullpath;
	printf("Text is %s\n",text);
   	while ( sscanf(ptr, "%255[^|]%n", field, &n) == 1 && args_read<5 )
   	{
		//check this to see if it is a valid key value pair i.e key1=value1
		//printf("found field %s\n",field);
		ptr2=field;
		valid_keyvaluepair=0;
		if(sscanf(ptr2,"%255[^=]%n",key,&nn)==1){
			ptr2+=nn;
			//printf("found key %s\n",key);
			//There is a key so look for a value
			if(*ptr2=='='){
				ptr2++;
				if(sscanf(ptr2,"%255[^=]%n",value,&nn)==1){
					//printf("found value %s\n",value);
					//yes it is
					valid_keyvaluepair=1;
				}
			}
		}
		if(valid_keyvaluepair==1){
			//is this the key value pair for setting the streaming path
			if(strcmp(key,"rtsp-path")==0){
				//test if value starts with a forward slash
				if(value[0]!='/'){
					//it does not so prepend one
					fullpath=(char*)malloc((strlen(value)+2)*sizeof(char));
					fullpath[0]='/';
					fullpath[1]='\0';
					strcat(fullpath,value);
				}else{
					fullpath=value;
				}
				strcpy(gStreamPath,fullpath);
			}
/*
			//add this key value pair to the list of args
			gStreamArgs[args_read]=(char*)malloc((strlen(field)+1)*sizeof(char)); // we never dealloc.
			strcpy(gStreamArgs[args_read],field); 
			args_read++;
*/
		}
		//test to see if we have reached the end of arguments
      		ptr += n; /* advance the pointer by the number of characters read */
      		if ( *ptr != '|' )
      		{
         		break; /* didn't find an expected delimiter, done? */
      		}
      		++ptr; /* skip the delimiter */
   	}

	/* add the gst pipleine commands */
	pipeline=argv[5];
	fullpipeline= (char*)malloc((strlen(pipeline)+10)*sizeof(char));
	fullpipeline[0]='(';
	fullpipeline[1]='\0';
	strcat(fullpipeline,pipeline);
	strcat(fullpipeline," )");

  	loop = g_main_loop_new (NULL, FALSE);

  	/* create a server instance */
  	server = gst_rtsp_server_new ();
	
	//set the ip address to listen on
	if(ipaddress!=NULL){
		gst_rtsp_server_set_address (server, ipaddress);
	}
	if (port!=NULL) {
		gst_rtsp_server_set_service(server, port);
	}

	printf("Streaming on %s:%d%s\n",ipaddress,gPortnumber,gStreamPath);

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
	//printf("gStreamPath is %s\n",gStreamPath);
  	gst_rtsp_media_mapping_add_factory (mapping, gStreamPath, factory); //TODO this should be parametised probably

  	/* don't need the ref to the mapper anymore */
  	g_object_unref (mapping);

  	/* attach the server to the default maincontext */
  	if (gst_rtsp_server_attach (server, NULL) == 0)
    	goto failed;

  	/* add a timeout for the session cleanup */
  	g_timeout_add_seconds (2, (GSourceFunc) timeout, server);


  	/* start serving, this never stops */
  	g_main_loop_run (loop);
  	printf("*******************************\nExited gstreamer OK it appears\n*******************************\n");
  	return 0;

  	/* ERRORS */
	failed:
  	{
    		g_print ("failed to attach the server\n");
    		return -1;
  	}
}
