#!/bin/bash

#hacks
#this is because clib is not built properly.
#export LIBS=-lc_nonshared

#project name - appended to target variant and version info
INX_PROJECT_NAME=mesa_test
# todo remove this should be identified in sourced script
PROCESSORS=8
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=arm-none
OS=linux-gnueabi
#CLIBTARGET_OVERRIDE=i586-linux-gnu-glibc-2.11.3 # set to something that doesn't exist and the system libraries will be used!
#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ 
INX_GLIBC_VERSION=""
# if you want to carry on regardless set to 'false'
EXIT_ON_FAIL=true
#this is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/ 
#leave blank for using the default host compiler
TOOLCHAIN_VERSION="arm-none-linux-gnueabi-4.4.6"
#TOOLCHAIN_VERSION="arm-4.5-codesourcery-toolchain"
#TOOLCHAIN_VERSION="arm-2012.03"

#TOOLCHAIN_VERSION=""
#gcc-4.3.3-i686-pc-linux-gnu
#Optional: prefix for the compiler of not just gcc. 
TOOLCHAIN_BIN_PREFIX="arm-none-linux-gnueabi-"
#TOOLCHAIN_BIN_PREFIX=""
#i686-pc-linux-gnu-
KERNEL_HEADERS="linux/2.6.35.30"

source ./source-scripts/inx-xbuilder-source-me.sh

########################################################################################################
## Components to build                                                                                 
##                                                                                                     
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################


if [ 1 == 0 ];then

if [ 1 == 0 ];then
build_component zlib -1.2.5 
fi

#build_component util-macros -1.10.1
#build_component renderproto -0.11.1
#build_component xproto -7.0.13
#build_component xcb-proto -1.6
#build_component libpthread-stubs -0.3
#build_component libXau -1.0.6
#build_component libxcb -1.7 "--disable-xinerama"

#build_component icu -49-1-2 "" "" "" "target_${OUTTARGET}"

export CFLAGS=-DMESA_EGL_NO_X11_HEADERS
export CXX_FOR_BUILD=g++
export CC_FOR_BUILD=gcc

build_component expat -2.0.1 

build_component mesa -8.0.4 "--with-gnu-ld --disable-xorg --disable-xlib-glx --disable-glu --disable-dri --disable-glx  --disable-glu --with-egl-platforms=fbdev  --enable-gles2 --enable-gles1"
## maybe OK but works without: --with-sysroot=${TOOLCHAINBASE}/arm-none-linux-gnueabi/libc

#"--enable-gles2 --enable-egl --with-egl-platforms=fbdev --enable-gallium-egl  --with-driver=no"
#"--enable-gles2 --enable-egl --with-egl-platforms=fbdev --enable-gallium-egl  --with-driver=no"
#"--disable-xorg --disable-xlib-glx --disable-glu --disable-dri --disable-glx" --with-sysroot=${TOOLCHAINBASE}/arm-none-linux-gnueabi/libc 

#build_component dbus -1.6.8 --enable-abstract-sockets
#export LFLAGS = -L${USRLIB_BUILD_ROOT}/lib
fi

#export REMAKE="false"
if [ "${TOOLCHAIN_VERSION}" == "arm-2012.03" ];then

build_component qt -5.0.0  "-v -prefix ${USRLIB_BUILD_ROOT} -sysroot ${USRLIB_BUILD_ROOT} -opengl -no-pch -xplatform linux-arm-gnueabi-g++-pmld --linuxfb -no-largefile -no-accessibility -qt-zlib -qt-libpng -qt-libjpeg -no-openssl -no-nis -no-cups -qt-sql-sqlite -opensource -confirm-license -no-dbus -nomake tests -nomake tools -nomake demos -nomake examples -nomake docs -no-rpath -nomake webkit" "NO_AUTOTOOLS_CROSS_COMPILE_HINTS"
else
# the confg script needs help finding libraries so we give the -I and -L options...
build_component qt -5.0.0  "-v -I ${USRLIB_BUILD_ROOT}/include -L ${USRLIB_BUILD_ROOT}/lib -prefix ${USRLIB_BUILD_ROOT} -sysroot ${SYSROOT} -v  -opengl -no-pch -xplatform linux-arm-gnueabi-g++-pmld --linuxfb -no-largefile -no-accessibility -qt-zlib -qt-libpng -qt-libjpeg -no-openssl -no-nis -no-cups -qt-sql-sqlite -opensource -confirm-license -no-dbus -nomake tests -nomake tools -nomake demos -nomake examples -nomake docs -no-rpath -nomake webkit" "NO_AUTOTOOLS_CROSS_COMPILE_HINTS"
fi
#-continue -no-make tools -hostprefix ${USRLIB_BUILD_ROOT} -prefix ${USRLIB_BUILD_ROOT} -prefix-install
#export REMAKE="true"


# build_component qt -4.6.4 "-v -hostprefix ${USRLIB_BUILD_ROOT} -prefix ${USRLIB_BUILD_ROOT} -prefix-install ${USRLIB_BUILD_ROOT}   -no-pch -nomake tools -xplatform linux-arm-gnueabi-g++-pmld -linuxfb -no-largefile -no-accessibility -qt-zlib -qt-libpng -qt-libjpeg -no-openssl -no-nis -no-cups -qt-sql-sqlite -nomake demos -nomake examples -opensource -no-dbus -nomake tests" "NO_AUTOTOOLS_CROSS_COMPILE_HINTS"
#opengl ${USRLIB_BUILD_ROOT}



