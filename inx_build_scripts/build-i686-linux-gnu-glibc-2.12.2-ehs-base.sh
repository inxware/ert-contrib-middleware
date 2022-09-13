#!/bin/bash

#hacks
#this is because clib is not built properly.
#export LIBS=-lc_nonshared

#project name - appended to target variant and version info
INX_PROJECT_NAME=base
# todo remove this should be identified in sourced script
PROCESSORS=8
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=i686
OS=linux-gnu
#CLIBTARGET_OVERRIDE=i586-linux-gnu-glibc-2.11.3 # set to something that doesn't exist and the system libraries will be used!
#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ 
INX_GLIBC_VERSION=""
# if you want to carry on regardless set to 'false'
EXIT_ON_FAIL=true
#this is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/ 
#leave blank for using the default host compiler
TOOLCHAIN_VERSION="i686-pc-linux-gnu-4.4.6"
#TOOLCHAIN_VERSION=""
#gcc-4.3.3-i686-pc-linux-gnu
#Optional: prefix for the compiler of not just gcc. 
TOOLCHAIN_BIN_PREFIX="i686-pc-linux-gnu-"
#TOOLCHAIN_BIN_PREFIX=""
#i686-pc-linux-gnu-
KERNEL_HEADERS="linux/2.6.35.30"

source ./source-scripts/inx-xbuilder-source-me.sh

########################################################################################################
## Components to build                                                                                 
##                                                                                                     
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################

build_component zlib -1.2.5 ## Seems this needs to be run twice in cross compile mode...
build_openssl_component openssl -1.0.0a 
build_component curl -7.21.2 " --without-random"
if [ 1 == 0] ; then
#export LDFLAGS+=-lc_nonshared
build_component libpthread-stubs -0.3
build_component zlib -1.2.5 ## Seems this needs to be run twice in cross compile mode...
build_component libidn -1.16
build_component curl -7.21.2
build_component libxml2 .X
build_component libarchive -3.0.0a "--with-sysroot=${SYSROOT}"

#build_component glib -2.26.0 "--enable-malloc0returnsnull" "glib_cv_stack_grows=no glib_cv_uscore=no ac_cv_func_posix_getgrgid_r=yes ac_cv_func_posix_getpwuid_r=yes"
#export CPPFLAGS=-U_FORTIFY_SOURCE
#unset CPPFLAbuild_component sqlite -autoconf-3070400
#build some misc. networking stuff.x
## Build GTK & dependencies 
#build_component util-macros -1.10.1


#build_component renderproto -0.11.1
#build_component xproto -7.0.13
#build_component xcb-proto -1.6
#build_component libXau -1.0.6
#build_component libxcb -1.7 "--disable-xinerama"
#build_component xcb-util -0.3.6
#build_component xtrans -1.2.1
#build_component xextproto -7.1.1
#build_component inputproto -2.0
#build_component kbproto -1.0.4
#build_component libX11 -1.3.2 "--with-keysymdef=${USRLIB_BUILD_ROOT}/include/X11/keysymdef.h --enable-malloc0returnsnull"
#build_component expat -2.0.1
#build_component pixman -0.19.4 "--disable-gtk"
#build_component tiff -3.9.4 #uses libtool, g++,
#build_component jpeg -8b
#build_component libpng -1.4.4
#build_component libfontenc -1.1.0
#build_component fontsproto -2.1.1
#"--with-keysymdef=${USRLIB_BUILD_ROOT}/include/X11/keysymdef.h"
#build_component freetype -2.1.10 #Only very old ones work with fontconfig on x86 it appears and we don't care what it is ...
#build_component libXfont -1.4.4
#--enable-libxml2  

#build_component fontconfig -2.8.0 " --with-arch=$TARGET --enable-libxml2 --without-expat --with-freetype-config=${USRLIB_BUILD_ROOT}/bin/freetype-config" 
#build_component atk -1.29.2
#export FONTCONFIG_CFLAGS="-I$USRLIB_BUILD_ROOT/include/fontconfig"
#export FONTCONFIG_LIBS="$USRLIB_BUILD_ROOT/lib/libfontconfig.so"
#export FREETYPE_CFLAGS="-I$USRLIB_BUILD_ROOT/include/freetype2"
#export FREETYPE_LIBS="$USRLIB_BUILD_ROOT/lib/libfreetype.so"
#build_component cairo -1.10.0 "--x-libraries=${USRLIB_BUILD_ROOT}/lib --x-includes=${USRLIB_BUILD_ROOT}"
#export have_cairo=true
#export have_cairo_freetype=true
#export CAIRO_CFLAGS="-I$USRLIB_BUILD_ROOT/include/cairo"
#export CAIRO_LIBS="$USRLIB_BUILD_ROOT/lib/libcairo.so"
#export LDFLAGS="-L${USRLIB_BUILD_ROOT}/lib -lfreetype -lfontconfig -lz -lxml2 " # none of the gtk overrides work properly - so do it old fashioned w
#export FREETYPE_CONFIG=$USRLIB_BUILD_ROOT/bin/freetype-config
#build_component pango -1.28.3 " --enable-debug=no --with-included-modules=no --with-dynamic-modules=yes --x-libraries=${USRLIB_BUILD_ROOT}/lib --x-includes=${USRLIB_BUILD_ROOT}"
#build_component libXrender -0.9.5  "--x-libraries=${USRLIB_BUILD_ROOT}/lib --x-includes=${USRLIB_BUILD_ROOT} --enable-malloc0returnsnull"
#build_component libXext -1.1.1 "--enable-malloc0returnsnull"
#build_component gdk-pixbuf -2.22.0 "--without-libjpeg"
#build_component compositeproto -0.4.1
#build_component fixesproto -4.1.1
#build_component libXfixes -4.0.4
#build_component libXcomposite -0.4.1
#build_component librsvg -2.32.0 #"--disable-gtk-theme"  # this should be moved up the list really.
#export LDFLAGS="-L${USRLIB_BUILD_ROOT}/lib -lfreetype -lfontconfig -lz -lxml2 -lgthread-2.0 -lglib-2.0 -lgobject-2.0 -lgdk_pixbuf-2.0 -lgio-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lcairo -lpng14 -lm -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lpixman-1 -lX11 -lxcb -lXau -lpangoft2-1.0 -lstdc++ -lpthread -lXrender " # none of the gtk overrides work properly - so do it old fashioned way
#export CFLAGS="-I${USRLIB_BUILD_ROOT}/include/gdk-pixbuf-2.0 -I$USRLIB_BUILD_ROOT/include/glib-2.0"
#build_component gtk +-2.22.0 " --enable-explicit-deps=no --disable-cups -disable-papi --disable-xinerama --disable-glibtest --disable-dependency-tracking --x-libraries=${USRLIB_BUILD_ROOT}/lib --x-includes=${USRLIB_BUILD_ROOT}/include/X11 --with-gdktarget=x11"
#build_component alsa-lib -1.0.23 -disable-python --disable-seq --disable-rawmidi --disable-alisp #--disable-old-symbols
#build_component alsa-utils -1.0.23 "--disable-alsatest --disable-alsamixer  --enable-xkb --disable-xmlto" #Needs libncurses5-dev - with mixer
#build_component faad2 -2.7
#pushd ${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}/target_packages/cslib/ && ln -fs libfaad.so.2 libfaad.so.0 
#popd # vlc/ffmpeg looks for libfaad.so.0 (no  pkgconfig generated for faad it seems).
#build_component ffmpeg -0.6 " --cross-prefix=${TOOLCHAIN_BIN_PREFIX} --enable-cross-compile  --sysroot=${SYSROOT} --arch=${ARCH} --target-os=linux --enable-version3 --disable-ffmpeg --enable-pthreads --disable-ffplay --disable-ffserver --disable-doc  --enable-libfaadbin --enable-postproc --enable-shared --enable-avfilter --enable-gpl --enable-nonfree --disable-libmp3lame --disable-libx264 --disable-libtheora --disable-libvorbis --enable-runtime-cpudetect --extra-cflags=-I${USRLIB_BUILD_ROOT}/include --extra-cflags=-L${USRLIB_BUILD_ROOT}/lib"  "NO_AUTOTOOLS_CROSS_COMPILE_HINTS"
#some stubs
fi

source source-scripts/inx-install-target-libs.sh

