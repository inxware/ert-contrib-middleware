#!/bin/bash

#project name - appended to target variant and version info
INX_PROJECT_NAME="gtk_gstX"
#Add an X so it doesn't build against the native libraries -these having an incompatible clib
#glibc-2.12.1-ti-blaze-ubuntu-10_10
#
#Dont give a project name and all should fit on the current clib and cslib paths 
# todo remove this should be identified in sourced script
PROCESSORS=8
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=arm
OS=linux-gnu
#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ 
INX_GLIBC_VERSION=-glibc-2.12.1-ti-blaze-ubuntu-10_10
# if you want to carry on regardless set to 'false'
EXIT_ON_FAIL=true
#this is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/ 
#leave blank for using the default host compiler
TOOLCHAIN_VERSION="arm-none-linux-gnueabi-4.4.6"
#gcc-4.3.3-i686-pc-linux-gnu
#Optional: prefix for the compiler of not just gcc. 
TOOLCHAIN_BIN_PREFIX="arm-none-linux-gnueabi-"
#i686-pc-linux-gnu-
KERNEL_HEADERS="linux/2.6.35-980-omap4"

source ./source-scripts/inx-xbuilder-source-me.sh

########################################################################################################
## Components to build                                                                                 
##                                                                                                     
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################
if [ 1 = 1 ];then
build_component zlib -1.2.5 
build_component zlib -1.2.5
build_openssl_component openssl -1.0.0a 
build_component curl -7.21.2 " --without-random"

build_component libarchive -3.0.0a "--without-xml2"

#--with-sysroot=${SYSROOT}
#Note we don't distribute these binaries so no target env. copy.
#we need these in build only to build EHS against - then we use the native version on the blaze board
fi


source source-scripts/inx-install-target-libs.sh




