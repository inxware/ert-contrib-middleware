###############################################################
## Copyright inx limited UK,
## License: GPL2
## 
## ./inx-xbuilder-source-me.sh --help 
############################################################### 
set -e
if [[ "$1" == "help" || "$1" == "-help" || "$1" == "--help" ]]; then
echo "To use this script create an executable text file with the following elements and edit these as required:"
echo ""
echo "#project name - appended to target variant and version info"
echo "INX_PROJECT_NAME=gtk_vlc"
echo "# todo remove this should be identified in sourced script"
echo "PROCESSORS=8"
echo "#gnu standard form of target architecture - prefix for existing libc version and created target directory"
echo "TARGET=i686-linux-gnu"
echo "#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ "
echo "# if you want to carry on regardless set to 'false'"
echo "EXIT_ON_FAIL=true"
echo "#this is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/ "
echo "#leave blank for using the default host compiler"
echo "TOOLCHAIN_VERSION=gcc-4.3.3-i686-pc-linux-gnu"
echo "#Optional: prefix for the compiler of not just gcc. "
echo "TOOLCHAIN_BIN_PREFIX=i686-pc-linux-gnu-"
echo ""
echo "source ./source-scripts/inx-xbuilder-source-me.sh"
echo ""
echo "########################################################################################################"
echo "## Components to build"                                                                                 
echo "##                                                                                                     "
echo "## build_component [package_name] [version] [optional: config parameters]  \\"
echo "                   [optional: config cache entries] [relative directory from package root to build in]"
echo "#########################################################################################################"
echo "e.g."
echo "build_component [package name1] [-X.X.X]"
echo "build_component [package name2] [-X.X.X]"
echo "build_component [package name3] \"\" [cache value] [Make directory] "
echo "e.t.c. ..."
fi

## We run from a specific location relative to the packages.
TEMP_PWD=${PWD} 
export BUILDHOSTPREFIX=`uname -m`

## Assume we are a linux PC - not checking!
export BUILDHOST="i686-linux-gnu"

#Setup the parallel build parameters if not defined
test -z ${PROCESSORS} && PROCESSORS=2 && echo "** Warning PROCESSORS not set. Setting to ${PROCESSOS} **" 


test -z "${ARCH}" &&  exit 1
#echo "** warning ARCH not set. Using i686"
test -z "${OS}" &&  exit 1
#echo "** Warning OS not set. Using linux-gnu"
export TARGET="${ARCH}-${OS}"

#adding support for specifying a project name which will affect the OUTTARGET directory
test -z ${INX_PROJECT_NAME} && INX_PROJECT_NAME="" && echo "** Warning INX_PROJECT_NAME not set. **"

#Set the Default GLIB version we are using
test -z ${TARGET} && TARGET=i686-linux-gnu  && echo "** Warning TARGET not set. Setting to ${TARGET} **" 

#Set a default GLIBC version
#test -z  ${INX_GLIBC_VERSION} && INX_GLIBC_VERSION=${TOOLCHAIN_VERSION}   && echo "** Warning INX_GLIBC_VERSION not set. Setting to ${INX_GLIBC_VERSION} **" 

#Set a default component build failure behaviour 
test -z ${EXIT_ON_FAIL} && EXIT_ON_FAIL="true"  

#support for remake flag, if true then clean everything first and do a configure, if not then skip configure and make clean
#todo make this a script arg 
test "${REMAKE}" != "true" -a "${REMAKE}" != "false"  && REMAKE="true" 

#set a default version of gcc
test -z ${TOOLCHAIN_VERSION} && TOOLCHAIN_VERSION="" && echo "** Warning TOOLCHAIN_VERSION not set. Setting to ${TOOLCHAIN_VERSION} **" 
#gcc-4.3.3-i686-pc-linux-gnu

#set a default prefix
if [ -z ${TOOLCHAIN_BIN_PREFIX} ];then
	TOOLCHAIN_BIN_PREFIX="" 
	echo "** Warning TOOLCHAIN_BIN_PREFIX not set. Setting to ${TOOLCHAIN_BIN_PREFIX} **" 
else
## check we  don't have a default toolchain when the target is not a PC
  if [ -z ${TOOLCHAIN_VERSION} ]; then 
    case "$TARGET" in
	i?86* | x86 ) echo "using host gcc for x86 host"  ;;
    	      *?) echo "You need to specifiy a tool chain that can build for $TARGET if you're running on an x86 host"; exit ;; 
    esac
  fi
fi

# Some belt and braces to capture our target path if configure doesn't do it 
INX_HOST_ARCH=$(uname -m)
TOOLCHAIN_PATH="${TEMP_PWD}/../../ert-build-support/toolchains/${INX_HOST_ARCH}/${TOOLCHAIN_VERSION}"
echo "##################################### ${TOOLCHAIN_PATH}"
TOOLCHAIN_BIN_PATH="${TOOLCHAIN_PATH}/bin"
# export CFLAGS=${CGLAGS}" -O2 " # set default configuration and add any previous stuff
export PATH="${TOOLCHAIN_BIN_PATH}":${PATH}
#Need to set LD_LIBRARY_PATHS for gcc dependencies :
export LD_LIBRARY_PATH="${TEMP_PWD}/../../ert-build-support/toolchains/${INX_HOST_ARCH}/${TOOLCHAIN_VERSION}/lib:${LD_LIBRARY_PATH}"
# deleteme = ${USRLIB_LIBRARY_PATH}:${LD_LIBRARY_PATH}:/home/pdrezet/workspace/inxware2.0-Release/INX/ert-build-support/toolchains/i686-pc-linux-gnu-4.4.6/lib"
# export LD_LIBRARY_PATH=${USRLIB_LIBRARY_PATH}:${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/lib:$LD_LIBRARY_PATH
# test for a gcc sysroot directory, else use the libc directory
export TOOLCHAINBASE="${TEMP_PWD}/../../ert-build-support/toolchains/${INX_HOST_ARCH}/${TOOLCHAIN_VERSION}"

if test -e  "${TOOLCHAINBASE}/sysroot"
then
 echo "Setting sysroot to ${TOOLCHAINBASE}/sysroot"
 export SYSROOT="${TOOLCHAINBASE}/sysroot"
else
  if test -e  "${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
  then
    echo "Setting sysroot to ${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
    export SYSROOT="${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
  else 
#	if test -e  "${TOOLCHAINBASE}/"
#	then
#	 echo "Setting sysroot to ${TOOLCHAINBASE}/"
#	 export SYSROOT="${TOOLCHAINBASE}/"
#	fi
#     if test -e  "${USRLIB_BUILD_ROOT}"
#     then
#       echo "Setting sysroot to ${USRLIB_BUILD_ROOT}"
#       export SYSROOT="${USRLIB_BUILD_ROOT}"
#     fi
echo "no sysroot found...."
  fi
fi 

## Done fixing up inputs 
#################################

## set up paths
#Path the Clibrary (Optional path)
#

test -z ${CLIBTARGET_OVERRIDE} && CLIBTARGET=${TARGET} || CLIBTARGET=${CLIBTARGET_OVERRIDE}
#Path to the component Library
# The following must match the EHS build paths defined by:
#$(EHS_GNU_ARCH)-$(EHS_GNU_OS)_$(TOOLCHAIN_PATH)_$(EHS_GNU_OS_VERSION)_$(COMPONENT_VARIANT)
test  -z ${INX_PROJECT_NAME} && OUTTARGET=${TARGET}-${TOOLCHAIN_VERSION}${INX_GLIBC_VERSION} || OUTTARGET=${TARGET}-${TOOLCHAIN_VERSION}${INX_GLIBC_VERSION}_${INX_PROJECT_NAME}
export OUTTARGET
export CLIBTARGET
#export these as they maybe used the target tree constructor script
TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR=${TEMP_PWD}/../../ert-build-support/support_libs/target_libs/${CLIBTARGET}
export TARGET_PATH_FROM_COMPONENT_SOURCE_DIR=${TEMP_PWD}/../target_libs/${OUTTARGET}
export BUILD_INSTALL_DIR="build"
export USRLIB_BUILD_ROOT=${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}/${BUILD_INSTALL_DIR}
#export CMAKE_INSTALL_PREFIX=${USRLIB_BUILD_ROOT}
#export CMAKE_SYSTEM_PREFIX_PATH=${USRLIB_BUILD_ROOT}
#export CMAKE_PREFIX_PATH="${USRLIB_BUILD_ROOT};${USRLIB_BUILD_ROOT}/usr/local"
export USRLIB_INCLUDE_PATH=${USRLIB_BUILD_ROOT}/include
export USRLIB_LIBRARY_PATH=${USRLIB_BUILD_ROOT}/lib
export CORELIB_INCLUDE_PATH=${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/include
export CORELIB_LIBRARY_PATH=${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/lib
export CORELIB_KERNEL_HEADERS_PATH=${TEMP_PWD}/../../ert-build-support/kernel-dependencies/${KERNEL_HEADERS}
###############################################################################
## SetUp some global environment variables that autotools build trees typically use.
##
if [ -z "${INX_CC}" ]; then
	INX_CC=${TOOLCHAIN_BIN_PREFIX}gcc
fi
if [ -z "${INX_CXX}" ]; then
	INX_CXX=${TOOLCHAIN_BIN_PREFIX}g++
fi
 export CC=${INX_CC}
 export LD=${TOOLCHAIN_BIN_PREFIX}ld
 export CXX=${INX_CXX}  
 export AR=${TOOLCHAIN_BIN_PREFIX}ar
 export RANLIB=${TOOLCHAIN_BIN_PREFIX}ranlib

# Some belt and braces to capture our target path if configure doesn't do it 
INX_HOST_ARCH=$(uname -m)
TOOLCHAIN_PATH="${TEMP_PWD}/../../ert-build-support/toolchains/${INX_HOST_ARCH}/${TOOLCHAIN_VERSION}"
echo "##################################### ${TOOLCHAIN_PATH}"
TOOLCHAIN_BIN_PATH="${TOOLCHAIN_PATH}/bin"
# export CFLAGS=${CGLAGS}" -O2 " # set default configuration and add any previous stuff
export PATH="${TOOLCHAIN_BIN_PATH}":${PATH}
#Need to set LD_LIBRARY_PATHS for gcc dependencies :
export LD_LIBRARY_PATH="${TEMP_PWD}/../../ert-build-support/toolchains/${INX_HOST_ARCH}/${TOOLCHAIN_VERSION}/lib:${LD_LIBRARY_PATH}"
# deleteme = ${USRLIB_LIBRARY_PATH}:${LD_LIBRARY_PATH}:/home/pdrezet/workspace/inxware2.0-Release/INX/ert-build-support/toolchains/i686-pc-linux-gnu-4.4.6/lib"
# export LD_LIBRARY_PATH=${USRLIB_LIBRARY_PATH}:${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/lib:$LD_LIBRARY_PATH
# test for a gcc sysroot directory, else use the libc directory
export TOOLCHAINBASE="${TEMP_PWD}/../../ert-build-support/toolchains/${INX_HOST_ARCH}/${TOOLCHAIN_VERSION}"

if test -e  "${TOOLCHAINBASE}/sysroot"
then
 echo "Setting sysroot to ${TOOLCHAINBASE}/sysroot"
 export SYSROOT="${TOOLCHAINBASE}/sysroot"
else
  if test -e  "${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
  then
    echo "Setting sysroot to ${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
    export SYSROOT="${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
  else 
#	if test -e  "${TOOLCHAINBASE}/"
#	then
#	 echo "Setting sysroot to ${TOOLCHAINBASE}/"
#	 export SYSROOT="${TOOLCHAINBASE}/"
#	fi
#     if test -e  "${USRLIB_BUILD_ROOT}"
#     then
#       echo "Setting sysroot to ${USRLIB_BUILD_ROOT}"
#       export SYSROOT="${USRLIB_BUILD_ROOT}"
#     fi
echo "no sysroot found...."
  fi
fi 


## Report what we're doing:
 echo -e "Building with: \n     CC=$CC\n     AR=$AR\n RANLIB=$RANLIB\n CFLAGS=$CFLAGS\nLDFLAGS=$LDFLAGS"
 echo -e "Toolchain bin [${TEMP_PWD}/../../ert-build-support/toolchains/${TOOLCHAIN_VERSION}/bin/${TOOLCHAIN_BIN_PREFIX}*] =\n"
 echo    "Installing target to $OUTTARGET"
 echo -e "LDFLAGS=${LDFLAGS}"
 #echo    "ANY KEY TO START - Ctrl-C to exit"
 #read -n 1

 #Utilities to check the ESP IDF tools


check_install_package () {
	_t=$(dpkg -s $1 | grep "install ok installed")
	if [[ -z "$_t" ]]; then
		echo Install $1
		sudo apt install --yes $1
	fi
}


if [ -f /.dockerenv ]; then
    echo "Running in Docker - no need to check the setup ..."
else 
####################################################################################################################################
#skip pythopn install?

read -n 1 -p "Not running in Docker - do you want to try to install esp32 dependencies on you host (y/n)" doinstall

if [ "$doinstall" = "yes" ];then

# Check and install essential packages
set +e
check_install_package python3
check_install_package python3-venv
check_install_package python3-pip
check_install_package libffi-dev
check_install_package libssl-dev
check_install_package dfu-util
check_install_package libusb-1.0-0
check_install_package flex
check_install_package gperf
check_install_package python3-setuptools
check_install_package git
check_install_package wget
check_install_package bison
check_install_package cmake
check_install_package ninja-build
check_install_package ccache
set -e


# Add toolchain to PATH
## Bullet-proof way of PATH append/prepend: https://unix.stackexchange.com/a/415028
## https://www.gnu.org/software/bash/manual/html_node/Shell-Parameter-Expansion.html
#export OLD_PATH=$PATH
_PATH="${TOOLCHAIN_PATH}/bin"
#todo-we should considerif we need a nother temporary directory for tools that are built per host machine (TARHGET_TREES is not very descriptive)
export IDF_PYTHON_ENV_BASE="../../TARGET_TREES/esp32_venv/"
## Create python virtual environment, install requirements and export it to PATH
python3 -m venv ${IDF_PYTHON_ENV_BASE} > /dev/null
export IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_BASE}/bin"

#note the follwing paths are usally via a softlink from the specific compiler version to a shared folder with the common tools.
#todo we want to remove the soft links and we can just use the hard path to the espressive-4.4.1 shared tools directory
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif-4.4.1/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/bin"
_PATH="${_PATH:+${_PATH}:}${IDF_PYTHON_ENV_PATH}"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif-4.4.1/tools/esptool_py/esptool"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif-4.4.1/tools/espcoredump"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif-4.4.1/tools/partition_table"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif-4.4.1/tools/app_update"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif-4.4.1/tools/additional_tools"
export PATH="${_PATH}${PATH:+:${PATH}}"

# Alias python3 to python
#alias python=$IDF_PYTHON_ENV_PATH/python3

## Python install requirement modules
pip install -r "${TEMP_PWD}/source-scripts/python-pip-requirements_inx-xbuilder-source-me-espidf.txt" > /dev/null

fi #end python / pip install
fi # end this is not a docker shell option
######################################################################################################################################################

##################################################################
# make the root directory for pooled libs for this target 
#mkdir -p ${USRBUILD_ROOT}
# We fon't have make install so need to create these:
mkdir -p ${USRLIB_INCLUDE_PATH}
mkdir -p ${USRLIB_LIBRARY_PATH}

##################################################################
## Call this function to build a component and install it if an install script is available
build_component() {
#esp32 middleware source path
COMPONENT_VERSION=$2
export IDF_PATH=${TEMP_PWD}/../contrib/esp-idf/esp-idf$COMPONENT_VERSION
# mkdir /home/zuhaib/Desktop/temp/bootloader/ && find ./components/bootloader_support/ -type d | grep "include" | xargs cp -R -t /home/zuhaib/Desktop/temp/bootloader

#reset the compiler flags ..., adding previous stuff...	
#	export PKG_CONFIG_PATH=$USRLIB_LIBRARY_PATH/pkgconfig:$USRLIB_BUILD_ROOT/share/pkgconfig # te core library are references above if they provide pkg config - which they generally don't
#	export CFLAGS=" --sysroot=${SYSROOT} -I${USRLIB_INCLUDE_PATH}/ -I${CORELIB_INCLUDE_PATH}/ -I${CORELIB_KERNEL_HEADERS_PATH} -I${SYSROOT}/usr/include ${CFLAGS}"
# 	export LDFLAGS=" --sysroot=${SYSROOT} -L${USRLIB_LIBRARY_PATH}/  -L${CORELIB_LIBRARY_PATH} -L${SYSROOT}/usr/lib -lc_nonshared ${LDFLAGS}"
#export these variables for the target population scirpt to use	
	export COMPONENTNAME=$1
	export COMPONENTVERSION=$2 
	export SUBCOMPONENTNAME=$3
	#export CONFIGPARMS=$3 # these are the configure switches appropriate for the device
	export CONFIG_CACHE=$4 # this is used for cross compilation to give configure hints so it doesn't run host tests- THIS IS TARGET SPECIFIC-ISH.... also checked to drop using standard cross-compile configure lines 
	export EXTRA_INSTALL_FLAGS=$5
	export MAKE_REL_PATH=$6 # if make is not in root of package give the directory name herecd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	pushd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	# not neeeded export BUILD_DIR_BASE=${TEMP_PWD}/../target_libs/esp32-idf-xtensa/build/lib/

	test ! -z ${MAKE_REL_PATH} && cd ./${MAKE_REL_PATH} #in case we have a specific make directory 
	echo "Making in $PWD - tried XX->$MAKE_REL_PATH <-XX"
	echo $SUBCOMPONENTNAME
	make -s component-$SUBCOMPONENTNAME-clean 
	###make -s clean #do this again if we changed directories or we had some dregs but no Makefile from a configure	
	if [ $PROCESSORS -gt 1 ]; then	# seperate call if make -j 1 is troublesome
	    #make V=1 -j $PROCESSORS component-$SUBCOMPONENTNAME-build || exit
	    make -j $PROCESSORS component-$SUBCOMPONENTNAME-build || exit
	else 
	    #make V=1 -d || component-$SUBCOMPONENTNAME-build exit #-j $PROCESSORS
	    make -d || component-$SUBCOMPONENTNAME-build exit #-j $PROCESSORS
	fi
	popd

	#INCLUDE_DIRECTORY=${TEMP_PWD}/../target_libs/esp32-idf-xtensa/build/include/
	# note todo - this usually ges created for us in source me with the relevant target name, but doing it again ..
	#mkdir -p ${INCLUDE_DIRECTORY} 
	# Copying header files with assumption there is no duplication after component subdirectories are maintained (cp with no clobbber flag to check) 
	#  Also assuming these headers under include are sufficient for all use of the libraries.
	test -d ${IDF_PATH}/components/${SUBCOMPONENTNAME}/include && cp -Rfn ${IDF_PATH}/components/${SUBCOMPONENTNAME}/include/* ${USRLIB_INCLUDE_PATH}/
	if [ "$(ls ${IDF_PATH}/build/${SUBCOMPONENTNAME}/*.a ||:)"  ];then  
		cp -Rfn ${IDF_PATH}/build/${SUBCOMPONENTNAME}/*.a ${USRLIB_LIBRARY_PATH}/
	fi
	cd ${TEMP_PWD}
}

build_bootloader() {
#esp32 middleware source path
COMPONENT_VERSION=$2
export IDF_PATH=${TEMP_PWD}/../contrib/esp-idf/esp-idf$COMPONENT_VERSION
# mkdir /home/zuhaib/Desktop/temp/bootloader/ && find ./components/bootloader_support/ -type d | grep "include" | xargs cp -R -t /home/zuhaib/Desktop/temp/bootloader
	
	export COMPONENTNAME=$1
	export COMPONENTVERSION=$2 
	export SUBCOMPONENTNAME=$3
	#export CONFIGPARMS=$3 # these are the configure switches appropriate for the device
	export CONFIG_CACHE=$4 # this is used for cross compilation to give configure hints so it doesn't run host tests- THIS IS TARGET SPECIFIC-ISH.... also checked to drop using standard cross-compile configure lines 
	export EXTRA_INSTALL_FLAGS=$5
	export MAKE_REL_PATH=$6 # if make is not in root of package give the directory name herecd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	pushd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	# not neeeded export BUILD_DIR_BASE=${TEMP_PWD}/../target_libs/esp32-idf-xtensa/build/lib/

	test ! -z ${MAKE_REL_PATH} && cd ./${MAKE_REL_PATH} #in case we have a specific make directory 
	echo "Making in $PWD - tried XX->$MAKE_REL_PATH <-XX"
	echo $SUBCOMPONENTNAME
	make bootloader-clean
	make bootloader
	test -f ${IDF_PATH}/build/bootloader/bootloader.bin && cp ${IDF_PATH}/build/bootloader/bootloader.bin ${TEMP_PWD}/../target_libs/xtensa-esp32_freertos-xtensa-esp32-elf-4.4.1/build/lib/
	popd
}



build_partition_table() {
#esp32 middleware source path
COMPONENT_VERSION=$2
export IDF_PATH=${TEMP_PWD}/../contrib/esp-idf/esp-idf$COMPONENT_VERSION
# mkdir /home/zuhaib/Desktop/temp/bootloader/ && find ./components/bootloader_support/ -type d | grep "include" | xargs cp -R -t /home/zuhaib/Desktop/temp/bootloader
	
	export COMPONENTNAME=$1
	export COMPONENTVERSION=$2 
	export SUBCOMPONENTNAME=$3
	#export CONFIGPARMS=$3 # these are the configure switches appropriate for the device
	export MAKE_REL_PATH=$6 # if make is not in root of package give the directory name herecd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	pushd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	# not neeeded export BUILD_DIR_BASE=${TEMP_PWD}/../target_libs/esp32-idf-xtensa/build/lib/

	test ! -z ${MAKE_REL_PATH} && cd ./${MAKE_REL_PATH} #in case we have a specific make directory 
	echo "Making in $PWD - tried XX->$MAKE_REL_PATH <-XX"
	echo $SUBCOMPONENTNAME
	make $SUBCOMPONENTNAME-clean
	make $SUBCOMPONENTNAME
	
	
	test -f  ${IDF_PATH}/build/partitions.bin && cp ${IDF_PATH}/build/partitions.bin ${TEMP_PWD}/../target_libs/xtensa-esp32_freertos-xtensa-esp32-elf-4.4.1/build/lib/partitions_singleapp.bin
	popd
}


build_prepare_kconfig_files() {
#esp32 middleware source path
COMPONENT_VERSION=$2
export IDF_PATH=${TEMP_PWD}/../contrib/esp-idf/esp-idf$COMPONENT_VERSION
# mkdir /home/zuhaib/Desktop/temp/bootloader/ && find ./components/bootloader_support/ -type d | grep "include" | xargs cp -R -t /home/zuhaib/Desktop/temp/bootloader
	
	export COMPONENTNAME=$1
	export COMPONENTVERSION=$2 
	export SUBCOMPONENTNAME=$3
	export MAKE_REL_PATH=$6 # if make is not in root of package give the directory name herecd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	pushd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	# not neeeded export BUILD_DIR_BASE=${TEMP_PWD}/../target_libs/esp32-idf-xtensa/build/lib/

	test ! -z ${MAKE_REL_PATH} && cd ./${MAKE_REL_PATH} #in case we have a specific make directory 
	echo "Making in $PWD - tried XX->$MAKE_REL_PATH <-XX"
	echo $SUBCOMPONENTNAME
	#make $SUBCOMPONENTNAME-clean
	make $SUBCOMPONENTNAME 
	export IDF_TARGET=esp32
	echo "ldgen_libraries"
	# This will make the `make` command target not containing `/../` that would fail the build.
	pushd ${IDF_PATH}
	make $(pwd)/build/ldgen_libraries
	popd
	echo "python script"
	python ${IDF_PATH}/tools/ldgen/ldgen.py   --input ${IDF_PATH}/components/esp_system/ld/esp32/sections.ld.in --config ${IDF_PATH}/sdkconfig --fragments ${IDF_PATH}/components/app_trace/linker.lf ${IDF_PATH}/components/esp_common/common.lf ${IDF_PATH}/components/esp_common/soc.lf ${IDF_PATH}/components/esp_event/linker.lf ${IDF_PATH}/components/esp_gdbstub/linker.lf ${IDF_PATH}/components/esp_hw_support/linker.lf ${IDF_PATH}/components/esp_phy/linker.lf ${IDF_PATH}/components/esp_pm/linker.lf ${IDF_PATH}/components/esp_ringbuf/linker.lf ${IDF_PATH}/components/esp_system/linker.lf ${IDF_PATH}/components/esp_system/app.lf ${IDF_PATH}/components/esp_wifi/linker.lf ${IDF_PATH}/components/espcoredump/linker.lf ${IDF_PATH}/components/freertos/linker.lf ${IDF_PATH}/components/hal/linker.lf ${IDF_PATH}/components/heap/linker.lf ${IDF_PATH}/components/log/linker.lf ${IDF_PATH}/components/lwip/linker.lf ${IDF_PATH}/components/newlib/newlib.lf ${IDF_PATH}/components/newlib/system_libs.lf ${IDF_PATH}/components/soc/linker.lf ${IDF_PATH}/components/spi_flash/linker.lf ${IDF_PATH}/components/xtensa/linker.lf --libraries-file ${IDF_PATH}/build/ldgen_libraries --output ${TEMP_PWD}/../target_libs/xtensa-esp32_freertos-xtensa-esp32-elf-4.4.1/build/lib/sections.ld --kconfig ${IDF_PATH}/Kconfig --env "COMPONENT_KCONFIGS=${IDF_PATH}/components/app_trace/Kconfig ${IDF_PATH}/components/asio/Kconfig ${IDF_PATH}/components/bt/Kconfig ${IDF_PATH}/components/coap/Kconfig ${IDF_PATH}/components/driver/Kconfig ${IDF_PATH}/components/efuse/Kconfig ${IDF_PATH}/components/esp-tls/Kconfig ${IDF_PATH}/components/esp32/Kconfig ${IDF_PATH}/components/esp32c3/Kconfig ${IDF_PATH}/components/esp32h2/Kconfig ${IDF_PATH}/components/esp32s2/Kconfig ${IDF_PATH}/components/esp32s3/Kconfig ${IDF_PATH}/components/esp_adc_cal/Kconfig ${IDF_PATH}/components/esp_common/Kconfig ${IDF_PATH}/components/esp_eth/Kconfig ${IDF_PATH}/components/esp_event/Kconfig ${IDF_PATH}/components/esp_gdbstub/Kconfig ${IDF_PATH}/components/esp_http_client/Kconfig ${IDF_PATH}/components/esp_http_server/Kconfig ${IDF_PATH}/components/esp_https_ota/Kconfig ${IDF_PATH}/components/esp_https_server/Kconfig ${IDF_PATH}/components/esp_hw_support/Kconfig ${IDF_PATH}/components/esp_ipc/Kconfig ${IDF_PATH}/components/esp_lcd/Kconfig ${IDF_PATH}/components/esp_netif/Kconfig ${IDF_PATH}/components/esp_phy/Kconfig ${IDF_PATH}/components/esp_pm/Kconfig ${IDF_PATH}/components/esp_system/Kconfig ${IDF_PATH}/components/esp_timer/Kconfig ${IDF_PATH}/components/esp_wifi/Kconfig ${IDF_PATH}/components/espcoredump/Kconfig ${IDF_PATH}/components/fatfs/Kconfig ${IDF_PATH}/components/freemodbus/Kconfig ${IDF_PATH}/components/freertos/Kconfig ${IDF_PATH}/components/hal/Kconfig ${IDF_PATH}/components/heap/Kconfig ${IDF_PATH}/components/jsmn/Kconfig ${IDF_PATH}/components/libsodium/Kconfig ${IDF_PATH}/components/log/Kconfig ${IDF_PATH}/components/lwip/Kconfig ${IDF_PATH}/components/mbedtls/Kconfig ${IDF_PATH}/components/mdns/Kconfig ${IDF_PATH}/components/mqtt/Kconfig ${IDF_PATH}/components/newlib/Kconfig ${IDF_PATH}/components/nvs_flash/Kconfig ${IDF_PATH}/components/openssl/Kconfig ${IDF_PATH}/components/openthread/Kconfig ${IDF_PATH}/components/pthread/Kconfig ${IDF_PATH}/components/spi_flash/Kconfig ${IDF_PATH}/components/spiffs/Kconfig ${IDF_PATH}/components/tcp_transport/Kconfig ${IDF_PATH}/components/unity/Kconfig ${IDF_PATH}/components/vfs/Kconfig ${IDF_PATH}/components/wear_levelling/Kconfig ${IDF_PATH}/components/wifi_provisioning/Kconfig ${IDF_PATH}/components/wpa_supplicant/Kconfig" --env "COMPONENT_KCONFIGS_PROJBUILD= ${IDF_PATH}/components/app_update/Kconfig.projbuild ${IDF_PATH}/components/bootloader/Kconfig.projbuild ${IDF_PATH}/components/esp_rom/Kconfig.projbuild ${IDF_PATH}/components/esptool_py/Kconfig.projbuild ${IDF_PATH}/components/partition_table/Kconfig.projbuild" --env "COMPONENT_KCONFIGS_SOURCE_FILE=${IDF_PATH}/build/kconfigs.in" --env "COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE=${IDF_PATH}/build/kconfigs_projbuild.in" --env "IDF_CMAKE=n" --env "IDF_ENV_FPGA=n" --objdump ${TOOLCHAIN_PATH}/../xtensa-esp32-elf-4.4.1/bin/xtensa-esp32-elf-objdump 
	popd
}


build_component_XXX() {
#esp32 middleware source path
COMPONENT_VERSION=$2
#export IDF_PATH=${TEMP_PWD}/../contrib/esp-idf/esp-idf$COMPONENT_VERSION

#reset the compiler flags ..., adding previous stuff...	
#	export PKG_CONFIG_PATH=$USRLIB_LIBRARY_PATH/pkgconfig:$USRLIB_BUILD_ROOT/share/pkgconfig # te core library are references above if they provide pkg config - which they generally don't
#	export CFLAGS=" --sysroot=${SYSROOT} -I${USRLIB_INCLUDE_PATH}/ -I${CORELIB_INCLUDE_PATH}/ -I${CORELIB_KERNEL_HEADERS_PATH} -I${SYSROOT}/usr/include ${CFLAGS}"
# 	export LDFLAGS=" --sysroot=${SYSROOT} -L${USRLIB_LIBRARY_PATH}/  -L${CORELIB_LIBRARY_PATH} -L${SYSROOT}/usr/lib -lc_nonshared ${LDFLAGS}"
#export these variables for the target population scirpt to use	
	export COMPONENTNAME=$1
	export COMPONENTVERSION=$2 
	export SUBCOMPONENTNAME=$3
	#export CONFIGPARMS=$3 # these are the configure switches appropriate for the device
	export CONFIG_CACHE=$4 # this is used for cross compilation to give configure hints so it doesn't run host tests- THIS IS TARGET SPECIFIC-ISH.... also checked to drop using standard cross-compile configure lines 
	export EXTRA_INSTALL_FLAGS=$5
	export MAKE_REL_PATH=$6 # if make is not in root of package give the directory name herecd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit
	#pushd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit

	#Generate a link (.ld file)	
	# Link component .a and some .o

}
