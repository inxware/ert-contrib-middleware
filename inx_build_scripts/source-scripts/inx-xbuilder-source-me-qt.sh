###############################################################
## Copyright inx limited UK,
## License: GPL2
## 
## Source me in a script (see template created by running 
## ./inx-xbuilder-source-me.sh --help 
############################################################### 

if [ $1 != "help" -o $1 != "-help" -o $1 != "--help" ]; then
echo "To use this script create an executable text file with the following elements and edit these as required:"
echo ""
echo "#project name - appended to target variant and version info"
echo "INX_PROJECT_NAME=gtk_vlc"
echo "# todo remove this should be identified in sourced script"
echo "PROCESSORS=8"
echo "#gnu standard form of target architecture - prefix for existing libc version and created target directory"
echo "TARGET=i686-linux-gnu"
echo "#version libc - ls  ../inx-core-uspace/toolchains/target_libs/ "
echo "INX_GLIBC_VERSION=glibc-2.11.3"
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

## Assume we are a linux PC - not checking!
export BUILDHOST="i686-linux-gnu"

#Setup the parallel build parameters if not defined
test -z ${PROCESSORS} && PROCESSORS=2 && echo "** Warning PROCESSORS not set. Setting to ${PROCESSOS} **" 


test -z ${ARCH} && ARCH="i686" && echo "** warning ARCH not set. Using i686"
test -z ${OS} && OS="linux-gnu" && echo "** Warning OS not set. Using linux-gnu"
export TARGET="${ARCH}-${OS}"

#adding support for specifying a project name which will affect the OUTTARGET directory
test -z ${INX_PROJECT_NAME} && INX_PROJECT_NAME="" && echo "** Warning INX_PROJECT_NAME not set. **"

#Set the Default GLIB version we are using
test -z ${TARGET} && TARGET=i686-linux-gnu  && echo "** Warning TARGET not set. Setting to ${TARGET} **" 

#Set a default GLIBC version
test -z  ${INX_GLIBC_VERSION} && INX_GLIBC_VERSION=${TOOLCHAIN_VERSION}   && echo "** Warning INX_GLIBC_VERSION not set. Setting to ${INX_GLIBC_VERSION} **" 

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

test -z ${KERNEL_HEADERS} && KERNEL_HEADERS="" && echo "** Warning kernel header location is nor provided. Setting to blank"

## Done fixing up inputs 
#################################

## set up paths
#Path the Clibrary (Optional path)
test -z ${CLIBTARGET_OVERRIDE} && CLIBTARGET=${TARGET}-${INX_GLIBC_VERSION} || CLIBTARGET=${CLIBTARGET_OVERRIDE}
#Path to the component Library
test  -z ${INX_PROJECT_NAME} && OUTTARGET=${TARGET}-${INX_GLIBC_VERSION} || OUTTARGET=${TARGET}-${INX_GLIBC_VERSION}-${INX_PROJECT_NAME}

#export these as they maybe used the target tree constructor script
TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR=${TEMP_PWD}/../../EHS-build-support/support_libs/target_libs/${CLIBTARGET}
export TARGET_PATH_FROM_COMPONENT_SOURCE_DIR=${TEMP_PWD}/../target_libs/${OUTTARGET}
export BUILD_INSTALL_DIR="build"
export USRLIB_BUILD_ROOT=${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}/${BUILD_INSTALL_DIR}
export USRLIB_INCLUDE_PATH=${USRLIB_BUILD_ROOT}/include
export USRLIB_LIBRARY_PATH=${USRLIB_BUILD_ROOT}/lib
export CORELIB_INCLUDE_PATH=${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/include
export CORELIB_LIBRARY_PATH=${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/lib
export CORELIB_KERNEL_HEADERS_PATH=${TEMP_PWD}/../../EHS-build-support/kernel-dependencies/${KERNEL_HEADERS}
###############################################################################
## SetUp some global environment variables that autotools build trees typically use.
##
 export CC=${TOOLCHAIN_BIN_PREFIX}gcc
 export LD=${TOOLCHAIN_BIN_PREFIX}ld
 export CXX=${TOOLCHAIN_BIN_PREFIX}g++  
 export AR=${TOOLCHAIN_BIN_PREFIX}ar
 export RANLIB=${TOOLCHAIN_BIN_PREFIX}ranlib


# Some belt and braces to capture our target path if configure doesn't do it 
TOOLCHAIN_PATH="${TEMP_PWD}/../../EHS-build-support/toolchains/${TOOLCHAIN_VERSION}"
echo "##################################### ${TOOLCHAIN_PATH}"
TOOLCHAIN_BIN_PATH="${TOOLCHAIN_PATH}/bin"
# export CFLAGS=${CGLAGS}" -O2 " # set default configuration and add any previous stuff
export PATH="${TEMP_PWD}/../../EHS-build-support/toolchains/${TOOLCHAIN_VERSION}/bin/":${PATH}
#Need to set LD_LIBRARY_PATHS for gcc dependencies :
export LD_LIBRARY_PATH="${TEMP_PWD}/../../EHS-build-support/toolchains/${TOOLCHAIN_VERSION}/lib:${LD_LIBRARY_PATH}"
# deleteme = ${USRLIB_LIBRARY_PATH}:${LD_LIBRARY_PATH}:/home/pdrezet/workspace/inxware2.0-Release/INX/EHS-build-support/toolchains/i686-pc-linux-gnu-4.4.6/lib"
# export LD_LIBRARY_PATH=${USRLIB_LIBRARY_PATH}:${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}/lib:$LD_LIBRARY_PATH
# test for a gcc sysroot directory, else use the libc directory
if test -e  "${TEMP_PWD}/../../EHS-build-support/toolchains/${TOOLCHAIN_VERSION}/sysroot"
then
 echo "Setting sysroot to ${TEMP_PWD}/../../EHS-build-support/toolchains/${TOOLCHAIN_VERSION}/sysroot"
 export SYSROOT="${TEMP_PWD}/../../EHS-build-support/toolchains/${TOOLCHAIN_VERSION}/sysroot"
else
 echo "Not setting sysroot"
 #echo "Setting sysroot to ${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
 #export SYSROOT="${TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR}/${BUILD_INSTALL_DIR}"
fi 
#Overwrite the system default pkg config path with libc's (well at least obscure the system's as we are not interested in the systems stuff
 export PKG_CONFIG_LIBDIR=$USRLIB_LIBRARY_PATH/pkgconfig 
#$TARGET_PATH_FROM_CORESUPPORT_SOURCE_DIR/$BUILD_INSTALL_DIR/lib/pkgconfig
#add the other paths used by component builds.
 
unset PKG_CONFIG_PATH
 export PKG_CONFIG_PATH=$USRLIB_LIBRARY_PATH/pkgconfig:$USRLIB_BUILD_ROOT/share/pkgconfig # te core library are references above if they provide pkg config - which they generally don't
if [ -n "${SYSROOT}" ];then
 export CFLAGS=" --sysroot=${SYSROOT} -I${USRLIB_INCLUDE_PATH} -I${CORELIB_INCLUDE_PATH}/ -I${CORELIB_KERNEL_HEADERS_PATH} " 
#-I${SYSROOT}/usr/include ${CFLAGS} "
 export LDFLAGS=" --sysroot=${SYSROOT} -L${USRLIB_LIBRARY_PATH}  -L${CORELIB_LIBRARY_PATH} -L${SYSROOT}/usr/lib -lc_nonshared ${LDFLAGS} "
#The followinf is required for cc1's dependency on dynamic gloog  etc. 
else
 export  "-I${USRLIB_INCLUDE_PATH} -I${CORELIB_INCLUDE_PATH}/ -I${CORELIB_KERNEL_HEADERS_PATH} " 
 export "-L${USRLIB_LIBRARY_PATH}  -L${CORELIB_LIBRARY_PATH} -L${SYSROOT}/usr/lib -lc_nonshared ${LDFLAGS} "
fi

## Report what we're doing:
 echo -e "Building with: \n     CC=$CC\n     AR=$AR\n RANLIB=$RANLIB\n CFLAGS=$CFLAGS\nLDFLAGS=$LDFLAGS"
 echo -e "Toolchain bin [${TEMP_PWD}/../../EHS-build-support/toolchains/${TOOLCHAIN_VERSION}/bin/${TOOLCHAIN_BIN_PREFIX}*] =\n"
 echo    "Installing target to $OUTTARGET"
 echo    "ANY KEY TO START - Ctrl-C to exit"
 read -n 1

##################################################################
# make the root directory for pooled libs for this target 
mkdir -p ../target_libs/$OUTTARGET


##################################################################
## Call this function to build a component and install it if an install script is available
build_component() {
pwd
#reset the compiler flags ..., adding previous stuff...	
#	export PKG_CONFIG_PATH=$USRLIB_LIBRARY_PATH/pkgconfig:$USRLIB_BUILD_ROOT/share/pkgconfig # te core library are references above if they provide pkg config - which they generally don't
#	export CFLAGS=" --sysroot=${SYSROOT} -I${USRLIB_INCLUDE_PATH}/ -I${CORELIB_INCLUDE_PATH}/ -I${CORELIB_KERNEL_HEADERS_PATH} -I${SYSROOT}/usr/include ${CFLAGS}"
# 	export LDFLAGS=" --sysroot=${SYSROOT} -L${USRLIB_LIBRARY_PATH}/  -L${CORELIB_LIBRARY_PATH} -L${SYSROOT}/usr/lib -lc_nonshared ${LDFLAGS}"
#export these variables for the target population scirpt to use	
	export COMPONENTNAME=$1
	export COMPONENTVERSION=$2 
	export CONFIGPARMS=$3 # these are the configure switches appropriate for the device
	export CONFIG_CACHE=$4 # this is used for cross compilation to give configure hints so it doesn't run host tests- THIS IS TARGET SPECIFIC-ISH.... also checked to drop using standard cross-compile configure lines 
	export EXTRA_INSTALL_FLAGS=$5
	export MAKE_REL_PATH=$6 # if make is not in root of package give the directory name here
	echo "############# Making $COMPONENTNAME${COMPONENTVERSION} ############################################"
if [ "${BUILD_OPTIONS}" != "SKIP_MAKE" ];then	
	cd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit

     if [ "${REMAKE}" = "true" ]; then
	echo "cleaning source tree"
	make -s clean
	make -s distclean
	if [  -n "${CONFIG_CACHE}" ]; then
		echo -e ${CONFIG_CACHE} > config-${TARGET}.cache
		CACHE_FILE=--cache-file=config-${TARGET}.cache 
	else 
		CACHE_FILE=""
	fi
#NM-nm is needed for glibc - there is a bug in this version affecting  older toolchains.
	if ! test -e ./configure
	then
	./bootstrap.sh  || ./autogen.sh --noconfigure # || or we will fail the next bit with a descriptive error prompt
	fi
	if [ "${CONFIG_CACHE}" == "NO_AUTOTOOLS_CROSS_COMPILE_HINTS" ] ;then
		./configure ${CONFIGPARMS}   --prefix=${USRLIB_BUILD_ROOT}  || exit #--build=i386-linux --program-prefix=${TOOLCHAIN_BIN_PREFIX}	
	else
		NM=nm ./configure ${CONFIGPARMS}   --prefix=${USRLIB_BUILD_ROOT} --host=${TARGET} ${CACHE_FILE} --build=i386-linux  || exit #--build=i386-linux --program-prefix=${TOOLCHAIN_BIN_PREFIX}
	fi
     fi	
echo "XXXXXXXXXXXXXXXXXXXXXXXXXX finished configure XXXXXXXXXXXXXXXXXXXXXXXXXXX"
#read -n 1
	test ! -z ${MAKE_REL_PATH} && cd ./${MAKE_REL_PATH} #in case we have a specific make directory 
	make -s clean #do this again if we changed directories or we had some dregs but no Makefile from a configure	
	if [ $PROCESSORS -gt 1 ]; then	# seperate call if make -j 1 is troublesome
	    make -s -j $PROCESSORS || exit
	else 
	    make -s  || exit #-j $PROCESSORS
	fi
#now we are ready to run make install to the prefix location
	make install
	if [ "${REMAKE}" = "true" ]; then
		make -s clean
		make -s distclean
	fi
fi
	cd ${TEMP_PWD}
}


#################################### SPECIALS #######################################################
# Non autoconf:

build_openssl_component() {
pwd
	#export these variables for the target population scirpt to use	
	export COMPONENTNAME=$1
	export COMPONENTVERSION=$2 
	export CONFIGPARMS=$3 # these are the configure switches appropriate for the device
	export CONFIG_CACHE=$4 # this is used for cross compilation to give configure hints so it doesn't run host tests- THIS IS TARGET SPECIFIC-ISH.... also checked to drop using standard cross-compile configure lines 
	export EXTRA_INSTALL_FLAGS=$5
	export MAKE_REL_PATH=$6 # if make is not in root of package give the directory name here
	echo "############# Making $COMPONENTNAME${COMPONENTVERSION} ############################################"
if [ "${BUILD_OPTIONS}" != "SKIP_MAKE" ];then	
	cd ${TEMP_PWD}/../contrib/${COMPONENTNAME}/${COMPONENTNAME}${COMPONENTVERSION} || exit

     if [ "${REMAKE}" = "true" ]; then
	echo "cleaning source tree"
	make -s clean
#NM-nm is needed for glibc - there is a bug in this version affecting  older toolchains.
	./Configure ${CONFIGPARMS}   --prefix=${USRLIB_BUILD_ROOT} --openssldir=${USRLIB_BUILD_ROOT} -I${USRLIB_INCLUDE_PATH}/ -I${CORELIB_INCLUDE_PATH}/ -I${CORELIB_KERNEL_HEADERS_PATH} -I${SYSROOT}/usr/include -D_REENTRANT \
	-L${USRLIB_LIBRARY_PATH}/  -L${CORELIB_LIBRARY_PATH} -L${SYSROOT}/usr/lib -lc_nonshared dist threads\
  	|| exit #--build=i386-linux --program-prefix=${TOOLCHAIN_BIN_PREFIX}
     fi	
#echo XXXXXXXXXXXXXXXXXXXXXXXXXX
#read -n 1

	make -s CC="${CC}" AR="${AR} r" RANLIB="${RANLIB}"   || exit # Need a strange appendage to AR??
#now we are ready to run make install to the prefix location
	make install
	if [ "${REMAKE}" = "true" ]; then
		make -s clean
	fi
fi
	cd ${TEMP_PWD}
}




