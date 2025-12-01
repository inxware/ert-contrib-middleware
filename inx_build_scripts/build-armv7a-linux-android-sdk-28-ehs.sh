#!/bin/bash

#Set the docker image name to build this in
#IMAGE_NAME=inxware/ubuntu22.04-build-essential

#source ./source-scripts/inx-dockersetup-source-me.sh 
#check_and_run_docker $IMAGE_NAME

#project name - appended to target variant and version info
INX_PROJECT_NAME=android-native-sdk28
# todo remove this should be identified in sourced script
PROCESSORS=8
#version libc - this is in the linux toolchain
INX_GLIBC_VERSION=""
# Empty as we are using the toolchains

# if you want to carry on regardless set to 'false
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=arm
OS=linux
EXIT_ON_FAIL=true
#this is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/ 
#leave blank for using the default host compiler
TOOLCHAIN_VERSION="armv7a-linux-android"
#gcc-4.3.3-i686-pc-linux-gnu
#Optional: prefix for the compiler of not just gcc. 
TOOLCHAIN_BIN_PREFIX="arm-linux-androideabi-28-"

#todo - change the above to arm-linux-androideabi-30-

source ./source-scripts/inx-xbuilder-source-me.sh
## on
########################################################################################################
## Components to build                                                                                 
##                                                                                                     
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################

export CFLAGS="-DANDROID -fPIC"

build_component libpthread-stubs -0.3
build_component libidn -1.30
build_openssl_component openssl -1.0.1u " -fPIC"
build_component curl -7.21.2 " --without-random --with-libidn=${USRLIB_BUILD_ROOT} --with-ssl=${USRLIB_BUILD_ROOT}"
build_component libarchive -3.1.0 "--with-sysroot=${SYSROOT} --without-xml2"
build_component expat -2.0.1
build_component libxml2 .X  --without-python

#"--without-debug --disable-dependency-tracking --without-python  --disable-rebuild-docs --without-docbook"

#build_component libxml2 .X   "--without-debug --disable-dependency-tracking --without-python --without-regexps --without-xpath --without-schemas --without-schematron --without-modules --without-xinclude  --disable-rebuild-docs --without-docbook"

# --with-minimum --without-push --without-python --without-regexps --without-xpath --without-writer --without-schemas --without-schematron --without-modules --without-xinclude  --disable-rebuild-docs --without-docbook  

#build_component freetype -2.4.3
#build_component fontconfig -2.8.0 "--enable-libxml2  --without-expat" 

source source-scripts/inx-install-target-libs.sh
