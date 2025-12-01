#!/bin/bash

#project name - appended to target variant and version info
INX_PROJECT_NAME=gtk_gst
# todo remove this should be identified in sourced script
PROCESSORS=8
#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ 
INX_GLIBC_VERSION=-glibc-2.12.2
# if you want to carry on regardless set to 'false
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=arm
OS=linux-gnu
EXIT_ON_FAIL=true
#this is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/ 
#leave blank for using the default host compiler
TOOLCHAIN_VERSION="arm-none-linux-gnueabi-4.4.6"
#gcc-4.3.3-i686-pc-linux-gnu
#Optional: prefix for the compiler of not just gcc. 
TOOLCHAIN_BIN_PREFIX="arm-none-linux-gnueabi-"
#i686-pc-linux-gnu-

source ./source-scripts/inx-xbuilder-source-me.sh

########################################################################################################
## Components to build                                                                                 
##                                                                                                     
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################

build_component zlib -1.2.5 
build_component zlib -1.2.5
build_openssl_component openssl -1.0.0a 
build_component curl -7.21.2 " --without-random"

build_component libarchive -3.0.0a "--without-xml2"

if [ 1 = 0 ];then

build_component glib -2.26.0  "" "glib_cv_stack_grows=no\n glib_cv_uscore=no\n ac_cv_func_posix_getgrgid_r=yes\n ac_cv_func_posix_getpwuid_r=yes\n" #--without-zlib
build_component sqlite -autoconf-3070400
build_component libidn -1.16
## Build GTK & dependencies
build_component util-macros -1.10.1
build_component renderproto -0.11.1
build_component xproto -7.0.13
build_component xcb-proto -1.6
build_component libpthread-stubs -0.3
build_component libXau -1.0.6
build_component libxcb -1.7 "--disable-xinerama"
build_component xcb-util -0.3.6
build_component xtrans -1.2.1
build_component xextproto -7.1.1
build_component inputproto -2.0
build_component kbproto -1.0.4
build_component libX11 -1.3.2
build_component expat -2.0.1
build_component libxml2 .X   "--without-python"
build_component freetype -2.4.3
build_component fontconfig -2.8.0 "--enable-libxml2  --without-expat" 
build_component libXrender -0.9.5
build_component libXext -1.1.1
build_component pixman -0.19.4 "--disable-gtk"
build_component tiff -3.9.4 #uses libtool, g++,
build_component jpeg -8b
build_component libpng -1.4.4
build_component cairo -1.10.0
build_component pango -1.28.3 "--enable-debug=no" 
build_component gdk-pixbuf -2.22.0 "--without-libjpeg"
build_component compositeproto -0.4.1
build_component fixesproto -4.1.1
build_component libXfixes -4.0.4
build_component libXcomposite -0.4.1
build_component librsvg -2.32.0 "--disable-gtk-theme"  # this should be moved up the list really.
build_component atk -1.29.2
build_component gtk +-2.22.0 "--disable-cups --disable-papi --disable-xinerama"  #--x-libraries=$TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR/$BUILD_INSTALL_DIR/lib/ --x-includes=$TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR/$BUILD_INSTALL_DIR/lib/fs"
build_component alsa-lib -1.0.23
#build_component alsa-utils -1.0.23 "--disable-alsatest --disable-alsamixer  --disable-xmlto" #Needs libncurses5-dev - with mixer
build_component faad2 -2.7
pushd . ;cd ${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}/target_packages/cslib/ ; ln -fs libfaad.so.2 libfaad.so.0; popd # vlc/ffmpeg looks for libfaad.so.0 (no  pkgconfig generated for faad it seems).
build_component ffmpeg -0.6 " --enable-version3 --disable-ffmpeg --enable-pthreads --disable-ffplay --disable-ffserver --disable-doc  --enable-libfaadbin --enable-postproc --enable-shared --enable-avfilter --enable-gpl --enable-nonfree --disable-libmp3lame --disable-libx264 --disable-libtheora --disable-libvorbis --enable-runtime-cpudetect --extra-cflags=-I${USRLIB_BUILD_ROOT}/include --extra-cflags=-L${USRLIB_BUILD_ROOT}/lib"

build_component gstreamer 0.10-0.10.30-ti "--disable-loadsave --disable-tests --disable-examples --disable-manpages" #build_component gst-plugins-base 0.10-0.10.30-ti "--disable-x --dis$
build_component gst-plugins-good 0.10-0.10.25 "--disable-x --disable-speex --disable-shout2 --disable-shout2tes --disable-oss --disable-oss4 "
build_component gst-plugins-bad 0.10-0.10.20-ti "--enable-DEBUG --enable-debug --disable-vp8 --disable-examples --enable-experimental --disable-camerabin --disable-selector"
build_component gst-plugins-ugly -0.10.17
build_component titiler-memmgr -0.24.12.1 "--enable-tilermgr"
KRNLSRC=${TEMP_PWD}"/../../EHS-build-support/kernel-dependencies/linux/2.6.38-1208-omap4" PREFIX=${TEMP_PWD}"/../../EHS-build-support/" build_component tisyslink "-0.24.13.1/syslink/" 
build_component tisyslink-d2cmap -0.24.12.1
build_component tiopenmax-domx -0.24.13.1
build_component gst-openmax -0.10.0.4+ti0.24.13.3
cd ../contrib/omap4-firmware/omap4-firmware-1/
mkdir -p ${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/lib/firmware/omap4
cp ./* ${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/lib/firmware/omap4

fi


source source-scripts/inx-install-target-libs.sh



