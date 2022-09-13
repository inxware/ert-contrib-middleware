#!/bin/bash

DISTCLEAN="false"
ARCHIVE_DIRECTORY="/home/pdrezet/temp/atom-platform-archives"

#do we want to clean up the directory before tarballing
if [ "${2}" == "-c" ]; then
	DISTCLEAN="true"
fi

if [ ! -z "${1}" ]; then
	ARCHIVE_DIRECTORY=${1}
fi

make_archive() {
	if [ ${DISTCLEAN} == "true" ]; then
		echo ">>>>> make distclean for ${1}"
		make distclean
	fi
	echo ">>>>> making tarball for ${1}"
	tar -cj --exclude=.svn -f ../${1}.tar.bz2 ./*
	echo ">>>>> copying tarball for ${1}"
	cp ../${1}.tar.bz2 ${ARCHIVE_DIRECTORY}/
	echo ">>>>> copied tarball for ${1}. Done for this one." 
}

echo ">>>>> Creating archives for atom platform. Target directory is ${ARCHIVE_DIRECTORY}"
#create the archive directory
mkdir -p ${ARCHIVE_DIRECTORY}

pushd .
cd ../contrib/APPLICATIONS/mdhrtsp-streamer/mdhrtsp-streamer-0.1
make_archive mdhrtsp-streamer-0.1
popd

pushd .
cd ../contrib/gst-rtsp/gst-rtsp-0.10.8
make_archive gst-rtsp-0.10.8
popd



pushd . 
cd ../contrib/gst-openmax/gst-openmax-0.10.0.4+ti0.24.13.3
make_archive gst-openmax-0.10.0.4+ti0.24.13.3
popd

pushd .
cd ../contrib/gst-plugins-bad/gst-plugins-bad0.10-0.10.20-ti
make_archive gst-plugins-bad0.10-0.10.20-ti
popd

pushd .
cd ../contrib/gst-plugins-base/gst-plugins-base0.10-0.10.30-ti
make_archive gst-plugins-base0.10-0.10.30-ti
popd

pushd .
cd ../contrib/gst-plugins-good/gst-plugins-good0.10-0.10.25
make_archive gst-plugins-good0.10-0.10.25
popd

pushd .
cd ../contrib/gstreamer/gstreamer0.10-0.10.30-ti
make_archive gstreamer0.10-0.10.30-ti
popd


pushd .
cd ../contrib/omap4-firmware/omap4-firmware-1.1
make_archive omap4-firmware-1.1
popd

pushd .
cd ../contrib/tiopenmax-domx/tiopenmax-domx-0.24.13.1
make_archive tiopenmax-domx-0.24.13.1
popd

pushd .
cd ../contrib/tisyslink/tisyslink-0.24.13.1
make_archive tisyslink-0.24.13.1
popd

pushd .
cd ../contrib/tisyslink-d2cmap/tisyslink-d2cmap-0.24.12.1
make_archive tisyslink-d2cmap-0.24.12.1
popd

pushd .
cd ../contrib/titiler-memmgr/titiler-memmgr-0.24.12.1
make_archive titiler-memmgr-0.24.12.1
popd

echo ">>>>> Creating codesourcery toolchain tarball"
pushd .
cd ../../EHS-build-support/toolchains
tar -cjf ${ARCHIVE_DIRECTORY}/atom-codesourcery-toolchain.tar.bz2 ./atom-codesourcery-toolchain
popd
echo ">>>>> Created codesourcery toolchain tarball"
echo ">>>>> Done. Your tarballs are in ${ARCHIVE_DIRECTORY}"
