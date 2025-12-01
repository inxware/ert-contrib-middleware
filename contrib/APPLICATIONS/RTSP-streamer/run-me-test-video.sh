#!/bin/bash

export LD_LIBRARY_PATH=${PWD}/lib 
export GST_PLUGIN_PATH=${PWD}/lib/gstreamer-0.10 
./inx-bonjour-rtsp-streamer-x86 ""  8554 "Device ID" "raymarine-mfd-rtsp-path=RAYMARINEMFD|raymarine-mfd-model=MODELID|raymarine-mfd-serial=SERIAL" "filesrc location=testvid.h264 ! qtdemux ! rtph264pay pt=96 name=pay0 "



