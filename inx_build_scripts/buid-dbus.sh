

#!/bin/bash

#hacks
#this is because clib is not built properly.
#export LIBS=-lc_nonshared

#project name - appended to target variant and version info
INX_PROJECT_NAME=dbus_test-deleteme
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

build_component expat -2.0.1 

build_component dbus -1.6.8 --enable-abstract-sockets


