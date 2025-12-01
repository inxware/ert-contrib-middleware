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
ARCH=armv7l
OS=pc-linux-gnueabihf
INX_GLIBC_VERSION=""
EXIT_ON_FAIL=true
#leave blank for using the default host gcc compiler
TOOLCHAIN_VERSION="x86_64-linux-gnu_clang10ubuntu18"
#gcc-4.3.3-i686-pc-linux-gnu
#Optional: prefix for the compiler of not just gcc. 
TOOLCHAIN_BIN_PREFIX=""
#Use a specific toolchain 
# one that onyl works on one machine in the whole world
INX_CC=$(pwd)/../../ert-build-support/toolchains/x86_64/x86_64-linux-gnu_clang10ubuntu18/bin/clang
INX_CXX=$(pwd)/../../ert-build-support/toolchains/x86_64/x86_64-linux-gnu_clang10ubuntu18/bin/clang++
#TOOLCHAIN_BIN_PREFIX=""

source ./source-scripts/inx-xbuilder-source-me.sh
########################################################################################################
## Components to build
##
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################

export INX_ORIG_CFLAGS="${CFLAGS}"
export CFLAGS="-L${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8 -fPIC -v --target=armv7l-pc-linux-gnueabihf -mfloat-abi=hard -I${USRLIB_BUILD_ROOT}/usr/include -I${USRLIB_BUILD_ROOT}/usr/include/asm-generic -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include/ -I${USRLIB_BUILD_ROOT}/usr/include/arm-linux-gnueabihf -I${USRLIB_BUILD_ROOT}/usr/include/c++/8 -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include-fixed ${INX_ORIG_CFLAGS}"
export INX_ORIG_CC="${CC}"
export CFLAGS="-Wl,--sysroot=${USRLIB_BUILD_ROOT},-v,-m,armelf_linux_eabi ${CFLAGS}"
export CC="${CC} -nostdinc --sysroot=${USRLIB_BUILD_ROOT} -B ${USRLIB_BUILD_ROOT}/usr/lib/ -B ${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8"
#we have to specify the python dir or libxml2 will bind to the host python
#build_component libxml2 -v2.9.11 "--with-python=${USRLIB_BUILD_ROOT} --with-sysroot=${USRLIB_BUILD_ROOT}"

export CFLAGS="-fPIC -v --target=armv7l-pc-linux-gnueabihf -mfloat-abi=hard -I${USRLIB_BUILD_ROOT}/usr/include -I${USRLIB_BUILD_ROOT}/usr/include/asm-generic -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include -I${USRLIB_BUILD_ROOT}/usr/include/arm-linux-gnueabihf -I${USRLIB_BUILD_ROOT}/usr/include/c++/8 -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include-fixed -L${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8 ${INX_ORIG_CFLAGS}"
export CC="${INX_ORIG_CC} -nostdinc --sysroot=${USRLIB_BUILD_ROOT} -B ${USRLIB_BUILD_ROOT}/usr/lib/ -B ${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8"
#build_component libarchive -3.6.0 "--with-sysroot=${USRLIB_BUILD_ROOT} --without-openssl"
#build_component curl -7.21.2 " --without-random"
export CFLAGS="-v --target=armv7l-pc-linux-gnueabihf -mfloat-abi=hard -I${USRLIB_BUILD_ROOT}/usr/include -I${USRLIB_BUILD_ROOT}/usr/include/asm-generic -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include -I${USRLIB_BUILD_ROOT}/usr/include/arm-linux-gnueabihf -I${USRLIB_BUILD_ROOT}/usr/include/c++/8 -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include-fixed -L${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8 ${INX_ORIG_CFLAGS}"
#build_aws_c_common

export CFLAGS="-Wno-gnu-include-next -Wno-incomplete-setjmp-declaration -v --target=armv7l-pc-linux-gnueabihf -mfloat-abi=hard -I${USRLIB_BUILD_ROOT}/usr/include -I${USRLIB_BUILD_ROOT}/usr/include/asm-generic -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include -I${USRLIB_BUILD_ROOT}/usr/include/arm-linux-gnueabihf -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include ${INX_ORIG_CFLAGS} -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include-fixed"
export INX_ORIG_CXX="${CXX}"
#
export CXX="${INX_ORIG_CXX} -v --target=armv7l-pc-linux-gnueabihf -mfloat-abi=hard --sysroot=${USRLIB_BUILD_ROOT} -B ${USRLIB_BUILD_ROOT}/usr/lib/ -B ${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8 -I${USRLIB_BUILD_ROOT}/usr/include/arm-linux-gnueabihf -I${USRLIB_BUILD_ROOT}/usr/include"
#-L${USRLIB_BUILD_ROOT}/usr/lib
export CXXFLAGS="-v"
#build_aws_lc "-DCMAKE_CROSSCOMPILING=True -DBUILD_TESTING=OFF -DOPENSSL_NO_ASM=1"
build_aws_s2n
build_aws_c_cal
build_aws_c_io
build_aws_c_compression
build_aws_c_http
build_aws_c_mqtt

export CFLAGS="-L${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8 -fPIC -v --target=armv7l-pc-linux-gnueabihf -mfloat-abi=hard -I${USRLIB_BUILD_ROOT}/usr/include -I${USRLIB_BUILD_ROOT}/usr/include/asm-generic -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include/ -I${USRLIB_BUILD_ROOT}/usr/include/arm-linux-gnueabihf -I${USRLIB_BUILD_ROOT}/usr/include/c++/8 -I${USRLIB_BUILD_ROOT}/usr/lib/gcc/arm-linux-gnueabihf/8/include-fixed ${INX_ORIG_CFLAGS}"
#build_component expat -2.0.1

#build_component libidn -1.33
