#!/bin/bash

NEW_ATOM=""
OLD_ATOM=""
PATCH_DIRECTORY=""

if [ ! $# == "3" ]; then
	echo "usage is ./create-atom-platform-pathch.sh /full/path/to/clean/atom/platform /full/path/to/dirty/atom/platform /full/path/to/patch/release/directory"
	exit 0
fi

if [ ! -z "${1}" ]; then
	OLD_ATOM=${1}
fi

if [ ! -z "${2}" ]; then
	NEW_ATOM=${2}
fi

if [ ! -z "${3}" ]; then
	PATCH_DIRECTORY=${3}
fi

echo ">>>>> Creating the source tarballs and putting them in ${PATCH_DIRECTORY}/tarcontents/archive"
mkdir -p ${PATCH_DIRECTORY}/tarcontents/archive
./create-tarballs-for-atom-platform.sh ${PATCH_DIRECTORY}/tarcontents/archive -c
echo ">>>>> Done creating source tarballs."

echo ">>>>> Creating patches to convert ${OLD_ATOM} to ${NEW_ATOM}"
cd ${NEW_ATOM}
diff -U 3 -H -d -r -N ${OLD_ATOM}/patches ./patches > ${PATCH_DIRECTORY}/tarcontents/buildroot-and-kernel.patch
diff -U 3 -H -d -r -N ${OLD_ATOM}/scripts ./scripts > ${PATCH_DIRECTORY}/tarcontents/scripts.patch
echo ">>>>> Done. Your patch should be ready in ${PATCH_DIRECTORY}"
