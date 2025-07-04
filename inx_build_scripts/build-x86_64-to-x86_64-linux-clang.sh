#!/bin/bash
set -e

#Set the docker image name to build this in
IMAGE_NAME=inxware/inx-debian10-clang10

echo "source 1"
source ./source-scripts/inx-dockersetup-source-me.sh 
check_and_run_docker $IMAGE_NAME

#project name - appended to target variant and version info
INX_PROJECT_NAME=base
# todo remove this should be identified in sourced script
PROCESSORS=8
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=x86_64
OS=linux-gnu
#CLIBTARGET_OVERRIDE=i586-linux-gnu-glibc-2.11.3 # set to something that doesn't exist and the system libraries will be used!
#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ 
INX_GLIBC_VERSION=""
EXIT_ON_FAIL=true
# This is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/
# Leave blank for using the default host compiler
TOOLCHAIN_VERSION="clang10"

#Optional: prefix for the compiler of not just gcc. 
TOOLCHAIN_BIN_PREFIX=""
#these toolchiain binaries are the ert-vuild-suppoty ones, that don't run many places.
#INX_CC=$(pwd)/../../ert-build-support/toolchains/x86_64/x86_64-linux-gnu_clang10ubuntu18/bin/clang
#INX_CXX=$(pwd)/../../ert-build-support/toolchains/x86_64/x86_64-linux-gnu_clang10ubuntu18/bin/clang++
#INX_LD=$(pwd)/../../ert-build-support/toolchains/x86_64/x86_64-linux-gnu_clang10ubuntu18/bin/clang

export HOST_BUILD=yes
INX_CC=clang
INX_CXX=clang++
INX_LD=clang

#seems new ubuntu uses fpic as he default, which stops the libs linking as it adds a Global offset table.
#export CFLAGS=-fno-pic
#export CXXFLAGS=-fno-pic


#TOOLCHAIN_BIN_PREFIX=""
#i686-pc-linux-gnu-
KERNEL_HEADERS="linux/2.6.35.30"


source ./source-scripts/inx-xbuilder-source-me.sh
########################################################################################################
## Components to build
##
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################


# note to do a make clean we need to drop into each component
# todo add make clean

#set to false if you don't want to rebuild the components, but copy artefacts to the build directory
build_aws_lc
build_aws_c_common
build_aws_s2n
build_aws_c_cal
build_aws_c_io
build_aws_c_compression
build_aws_c_http
build_aws_c_mqtt


#These are needed only if we want to build from scratch rather than using debian lib*-dev packages.
if [ 1 = 0 ];then
#build_component expat -2.0.1
#build_component libidn -1.33

if [ 1 = 1 ];then
build_component curl -7.21.2 " --without-random"

build_component libxml2 .X "--with-sysroot=${SYSROOT}"
#this has deprecated stuff clang10 doesn't like in it so bumping up the version: build_component libarchive -3.0.0a "--with-sysroot=${SYSROOT}"
build_component libarchive -3.6.1 "--with-sysroot=${SYSROOT}"
fi


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
fi
