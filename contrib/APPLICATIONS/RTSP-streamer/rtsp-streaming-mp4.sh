#!/bin/bash

export LD_LIBRARY_PATH=${PWD}/lib 
export GST_PLUGIN_PATH=${PWD}/lib/gstreamer-0.10 
./generic-RTSP-server-x86 ""  8554 "streamx" "rtsp-path=stream" "filesrc location=${1} ! qtdemux ! rtph264pay pt=96 name=pay0 "



