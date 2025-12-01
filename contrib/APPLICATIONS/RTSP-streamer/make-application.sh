#!/bin/bash
echo --------- Building RTSP Server using $1 target libraries -------------
#if we are building for a PC
if [[ "${1}" == "x86" ]];then

export PKG_CONFIG_PATH=${PWD}/../../../target_libs/i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/pkgconfig
export LD_LIBRARY_PATH=${PWD}/../../../target_libs/i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/
#export PKG_CONFIG_PATH=${PWD}/../../../target_libs/i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/pkgconfig
#export LD_LIBRARY_PATH=${PWD}/../../../target_libs/i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/
mkdir -p ./lib/gstreamer-0.10


cp -P ${PWD}/../../../target_libs/i686-linux-gnu-i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/lib*.so* ./lib/
cp -P ${PWD}/../../../target_libs/i686-linux-gnu-i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/gstreamer-0.10/lib*so ./lib/gstreamer-0.10
#old versions:
#cp -P ${PWD}/../../../target_libs/i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/lib*.so* ./lib/
#cp -P ${PWD}/../../../target_libs/i686-linux-gnu-glibc-2.11.3-raymarine${2}/build/lib/gstreamer-0.10/lib*so ./lib/gstreamer-0.10
gcc ../mdhrtsp-streamer/mdhrtsp-streamer-0.1/mdhrtsp-streamer.c -o ./inx-bonjour-rtsp-streamer-x86 `pkg-config --cflags --libs gst-rtsp-server-0.10 --libs avahi-core`
gcc generic-RTSP-server.c -o ./generic-RTSP-server-x86 `pkg-config --cflags --libs gst-rtsp-server-0.10 --libs avahi-core`

exit
fi

#if we are cross compiling for arm on x86 host
if [[ "${1}" == "arm" ]];then

export PREFIX=${PWD}/../../../target_libs/arm-linux-gnueabi-glibc-2.12.2-raymarine${2}/build
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
export LD_LIBRARY_PATH=${PREFIX}/lib/
export LIBRARIES=${PREFIX}/lib
export INCLUDE=${PREFIX}/include

mkdir -p ./lib-arm/gstreamer-0.10
cp -P ${PREFIX}/lib/lib*.so* ./lib-arm/
cp -P ${PREFIX}/lib/gstreamer-0.10/lib*so ./lib-arm/gstreamer-0.10

#ls -l $LD_LIBRARY_PATH

export PATH=${PWD}/../../../../EHS-build-support/toolchains/arm-none-linux-gnueabi-4.4.6-oldhost/bin/:$PATH

#arm-none-linux-gnueabi-gcc -g inx-bonjour-rtsp-streamer.c -o ./inx-bonjour-rtsp-streamer-arm -L${LIBRARIES} -I${INCLUDE} -I${INCLUDE}/glib-2.0 -I${INCLUDE}/gstreamer-0.10 -lssp -lgstrtp-0.10 -lgstrtsp-0.10 -lgstsdp-0.10 -lgstapp-0.10 `pkg-config --cflags --libs glib-2.0 gst-rtsp-server-0.10 avahi-core `pkg-config --cflags --libs glib-2.0 gst-rtsp-server-0.10 avahi-core

exit
fi

#if we are building on the host:
if [[ "${1}" == "arm-host" ]];then
gcc ../mdhrtsp-streamer/mdhrtsp-streamer-0.1/mdhrtsp-streamer.c -o ./inx-bonjour-rtsp-streamer-arm `pkg-config --cflags --libs gst-rtsp-server-0.10 --libs avahi-core`
exit
fi
echo "*** No Target Provided ***"
echo "run with argument:"
echo "     x86  :PC version" 
echo "     arm  :OMAP 4 version"
echo " or"
echo "     arm-host to build an arm version in an Ubunutu OMAP4 board "
echo "Direct build dependencies are  glib, gstreamer, gstreamer-plugins-base and avahi development packages"
echo "For the x86 and arm prefic based builds, you can  add a second argument to use an alternative prefix extension for library deps  (concated on to raymarine)"


