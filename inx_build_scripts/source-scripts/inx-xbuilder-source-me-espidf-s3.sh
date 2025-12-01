#!/bin/bash
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
echo "INX_PROJECT_NAME=esp32-s3"
echo "# todo remove this should be identified in sourced script"
echo "PROCESSORS=8"
echo "#gnu standard form of target architecture - prefix for existing libc version and created target directory"
echo "TARGET=$ARCH-$OS"
echo "#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ "
echo "# if you want to carry on regardless set to 'false'"
echo "EXIT_ON_FAIL=true"
echo "#this is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/ "
echo "#leave blank for using the default host compiler"
echo "TOOLCHAIN_VERSION=$TOOLCHAIN_VERSION"
echo "#Optional: prefix for the compiler of not just gcc. "
echo "TOOLCHAIN_BIN_PREFIX=$TOOLCHAIN_BIN_PREFIX"
echo ""
echo 'source ./source-scripts/$(basename "$0")'
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
# deleteme = ${USRLIB_LIBRARY_PATH}:${LD_LIBRARY_PATH}:"/home/pdrezet/workspace/inxware2.0-Release/INX/ert-build-support/toolchains/i686-pc-linux-gnu-4.4.6/lib"
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
# deleteme = ${USRLIB_LIBRARY_PATH}:${LD_LIBRARY_PATH}:"/home/pdrezet/workspace/inxware2.0-Release/INX/ert-build-support/toolchains/i686-pc-linux-gnu-4.4.6/lib"
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
export IDF_PYTHON_ENV_BASE="${TEMP_PWD}/../../TARGET_TREES/esp32_venv/"
## Create python virtual environment, install requirements and export it to PATH
python3 -m venv ${IDF_PYTHON_ENV_BASE} > /dev/null 2>&1
export IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_BASE}/bin"

#note the follwing paths are usally via a softlink from the specific compiler version to a shared folder with the common tools.
#todo we want to remove the soft links and we can just use the hard path to the espressive${IDF_VERSION} shared tools directory
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif${IDF_VERSION}/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/bin"
_PATH="${_PATH:+${_PATH}:}${IDF_PYTHON_ENV_PATH}"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif${IDF_VERSION}/tools/esptool_py/esptool"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif${IDF_VERSION}/tools/tools"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif${IDF_VERSION}/tools/espcoredump"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif${IDF_VERSION}/tools/partition_table"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif${IDF_VERSION}/tools/app_update"
_PATH="${_PATH:+${_PATH}:}${TOOLCHAIN_PATH}/espressif${IDF_VERSION}/tools/additional_tools"
export PATH="${_PATH}${PATH:+:${PATH}}"
echo $PATH

# Alias python3 to python
alias python=$IDF_PYTHON_ENV_PATH/python3
PYTHON="${TEMP_PWD}/../../TARGET_TREES/esp32_venv/bin"/python3
## Python install requirement modules
$PYTHON -m pip install -r "${TEMP_PWD}/source-scripts/python-pip-requirements_inx-xbuilder-source-me-espidf.txt"

fi #end python / pip install
fi # end this is not a docker shell option
######################################################################################################################################################

export STAGING_DIR="${TEMP_PWD}/../../ERT_CONTRIB_MIDDLEWARE_STAGING/"
#python -m pip install -U idf-component-manager~=1.2

##################################################################
# make the root directory for pooled libs for this target 
#mkdir -p ${USRBUILD_ROOT}
# We fon't have make install so need to create these:
mkdir -p ${USRLIB_INCLUDE_PATH}
mkdir -p ${USRLIB_LIBRARY_PATH}
mkdir -p ${STAGING_DIR}

#################################################################
# Copy the dummy project and essential files to staging directory
#################################################################
test -d ${STAGING_DIR} && rm -rf ${STAGING_DIR}/*

cp_new () { mkdir -p `dirname "${!#}"` ; local args=$(echo "$@" | tr -d '\047') ; cp $args; }

cp_new_f () { 
	mkdir -p `dirname "${!#}"` ; 
	local argv=( "$@" )
	dst=${argv[-1]} ; unset 'argv[-1]'
	src=${argv[-1]} ; unset 'argv[-1]'
	rest=${argv[@]}
	find `dirname "${src}"` -type f -name `basename "${src}"` | xargs -I {} cp ${rest} {} "${dst}"
#	local args=$(echo "$@" | tr -d '\047') ; 
#	cp $args; 
}

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
	if test -d "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project"; then
		echo
	else
		cp_new -r "${IDF_PATH}/examples/get-started/blink" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project"
		rm -f "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/"sdkconfig*
		cp_new -r "${IDF_PATH}/ert_config_files/${OS}/"* "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/"
		mv "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/sdkconfig" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/sdkconfig.defaults"
	fi
	pushd "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build"
	ninja app
	popd
	# Copy the generated static library to the lib folder
#	cp "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build/esp-idf/${SUBCOMPONENTNAME}"/*.a "${TEMP_PWD}/../target_libs/${ARCH}-${OS}-${TOOLCHAIN_VERSION}/build/lib/"
	
	#INCLUDE_DIRECTORY=${TEMP_PWD}/../target_libs/esp32-idf-xtensa/build/include/
	# note todo - this usually ges created for us in source me with the relevant target name, but doing it again ..
	#mkdir -p ${INCLUDE_DIRECTORY} 
	# Copying header files with assumption there is no duplication after component subdirectories are maintained (cp with no clobbber flag to check) 
	#  Also assuming these headers under include are sufficient for all use of the libraries.
	test -d ${IDF_PATH}/components/${SUBCOMPONENTNAME}/include && cp -Rfn ${IDF_PATH}/components/${SUBCOMPONENTNAME}/include/* ${USRLIB_INCLUDE_PATH}/
	if [ "$(ls ${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build/esp-idf/${SUBCOMPONENTNAME}/*.a ||:)"  ];then  
#		find ${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build/esp-idf/${SUBCOMPONENTNAME}/ -type f -name "*.a" | xargs cp -Rfn -t ${USRLIB_LIBRARY_PATH}/
		cp -Rfn ${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build/esp-idf/${SUBCOMPONENTNAME}/*.a ${USRLIB_LIBRARY_PATH}/

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
	# not neeeded export BUILD_DIR_BASE=${TEMP_PWD}/../target_libs/esp32-idf-xtensa/build/lib/
	#
	if test -d "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project"; then
		echo
	else
		cp_new -r "${IDF_PATH}/examples/get-started/blink" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project"
		rm -f "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/"sdkconfig*
		cp_new -r "${IDF_PATH}/ert_config_files/${OS}/*" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/"
		mv "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/sdkconfig" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/sdkconfig.defaults"

	fi
	pushd "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project"
	test -d build && rm -rf build
	mkdir build ; cd build
	echo "Enter directory ${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build"
	cmake .. -G Ninja -DIDF_TARGET=esp32s3
	ninja bootloader
	# Copy the generated static library to the lib folder
	cp_new -r "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build/bootloader/bootloader.bin" "${TEMP_PWD}/../target_libs/${ARCH}-${OS}-${TOOLCHAIN_VERSION}/build/lib/"
	#	test -f ${IDF_PATH}/build/bootloader/bootloader.bin && cp ${IDF_PATH}/build/bootloader/bootloader.bin ${TEMP_PWD}/../target_libs/xtensa-esp32_freertos-xtensa-esp32-elf${IDF_VERSION}/build/lib/
	echo "Leave directory ${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build"
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
	if test -d "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project"; then
		echo
	else
		cp_new -r "${IDF_PATH}/examples/get-started/blink" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project"
		rm -f "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/"sdkconfig*
		cp_new -r "${IDF_PATH}/ert_config_files/${OS}/*" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/"
		mv "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/sdkconfig" "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/sdkconfig.defaults"
	fi
	pushd "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build"
	ninja partition-table
	popd
	
	# Copy the generated static library to the lib folder
	cp_new -Rf "${STAGING_DIR}/${COMPONENTNAME}${COMPONENTVERSION}-${OS}/dummy_project/build/partition_table/partition-table.bin" "${TEMP_PWD}/../target_libs/${ARCH}-${OS}-${TOOLCHAIN_VERSION}/build/lib/" # partitions_singleapp.bin?
	
#	test -f  ${IDF_PATH}/build/partitions.bin && cp ${IDF_PATH}/build/partitions.bin ${TEMP_PWD}/../target_libs/xtensa-esp32_freertos-xtensa-esp32-elf${IDF_VERSION}/build/lib/partitions_singleapp.bin
}

###################################################################################
