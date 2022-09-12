#!/bin/bash

export LD_LIBRARY_PATH=${PWD}/lib 
export GST_PLUGIN_PATH=${PWD}/lib/gstreamer-0.10 
./inx-bonjour-rtsp-streamer-x86 ""  8554 "Device ID" "raymarine-mfd-rtsp-path=RAYMARINEMFD|raymarine-mfd-model=MODELID|raymarine-mfd-serial=SERIAL" "videotestsrc ! video/x-raw-yuv, framerate=25/1, width=800, height=480 ! x264enc ! rtph264pay pt=96 name=pay0 "



