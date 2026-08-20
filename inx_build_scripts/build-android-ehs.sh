#!/bin/bash
# build-android-ehs.sh — Build EHS contrib libraries for an Android ABI target.
#
# Produces static artefacts in:
#   ../target_libs/<OUTTARGET>/build/
#
# That path is what ert-components consumes when COMPONENT_BASE_TECHNOLOGIES
# matches OUTTARGET (verify with: make chkconfig in ert-components).
#
# Builds:
#   - libpthread-stubs, OpenSSL, curl, libarchive, expat, libxml2
#   - LiteRT (TFLite C API static library — liblitert_c.a + C API headers)
#
# Note: libidn is intentionally not built for Android.  Android's bionic libc
#   provides its own IDNA (internationalized domain names) implementation, and
#   libidn 1.33 requires gtk-doc-tools (gtkdocize) to autoreconf — not worth
#   pulling in.  Older scripts used libidn-1.16/1.30 but NDK clang 18 (r27c)
#   rejects its inline asm.  curl is built --without-libidn accordingly.
#
# Usage:
#   cd inx_build_scripts
#   bash build-android-ehs.sh --abi armeabi-v7a --api 30
#   bash build-android-ehs.sh --abi arm64-v8a   --api 30
#   bash build-android-ehs.sh --abi x86_64       --api 30
#
# Options:
#   --abi   Android ABI  (armeabi-v7a | arm64-v8a | x86 | x86_64)   [required]
#   --api   Android API level (integer, e.g. 30)                      [required]
#   --jobs  Parallel build jobs (default: 8)
#
# Docker:
#   When not already running inside a container this script re-launches itself
#   inside inxware/ubuntu22.04-android-ndk-build, which provides cmake and
#   Android NDK r27c at standard paths.  Build it once with:
#     docker build -f Dockerfile.android-ndk-build \
#                  -t inxware/ubuntu22.04-android-ndk-build .
#
# Prerequisites (when running outside Docker):
#   cmake >= 3.16, git >= 2.25, python3, rsync, wget, unzip

set -euo pipefail

# ---------------------------------------------------------------------------
# Docker re-launch — re-run this script inside the Android NDK build image
# if we are not already inside a container.  All arguments ($@) are forwarded.
# Build the image once with:
#   docker build -f Dockerfile.android-ndk-build -t inxware/ubuntu22.04-android-ndk-build .
# ---------------------------------------------------------------------------
source ./source-scripts/inx-dockersetup-source-me.sh
check_and_run_docker "inxware/ubuntu22.04-android-ndk-build" "$@"

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
ANDROID_ABI=""
ANDROID_API=""
PROCESSORS=8

while [[ $# -gt 0 ]]; do
    case "$1" in
        --abi)  ANDROID_ABI="$2";  shift 2 ;;
        --api)  ANDROID_API="$2";  shift 2 ;;
        --jobs) PROCESSORS="$2";   shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

if [[ -z "${ANDROID_ABI}" || -z "${ANDROID_API}" ]]; then
    echo "Usage: bash build-android-ehs.sh --abi <abi> --api <level>"
    echo "  --abi  armeabi-v7a | arm64-v8a | x86 | x86_64"
    echo "  --api  Android API level (e.g. 30)"
    exit 1
fi

# ---------------------------------------------------------------------------
# Per-ABI toolchain configuration
#
# Maps Android ABI names to the NDK compiler triple, ar prefix, ert-components
# OUTTARGET path, and CMake processor name.
# ---------------------------------------------------------------------------
case "${ANDROID_ABI}" in
    armeabi-v7a)
        _CC_TRIPLE="armv7a-linux-androideabi${ANDROID_API}-clang"
        _CXX_TRIPLE="armv7a-linux-androideabi${ANDROID_API}-clang++"
        _AR_PREFIX="arm-linux-androideabi-"
        _AUTOTOOLS_ARCH="arm"      # simple form accepted by old config.sub
        _OUTTARGET="armv7a-linux-android"
        _CMAKE_PROCESSOR="arm"
        _NDK_TOOLCHAIN_DIR="armv7a-linux-android"   # dir name under ert-build-support
        _NDK_SYSROOT_ARCH="arm-linux-androideabi"   # sysroot lib dir for libc++_shared.so
        ;;
    arm64-v8a)
        _CC_TRIPLE="aarch64-linux-android${ANDROID_API}-clang"
        _CXX_TRIPLE="aarch64-linux-android${ANDROID_API}-clang++"
        _AR_PREFIX="aarch64-linux-android-"
        _AUTOTOOLS_ARCH="aarch64"
        _OUTTARGET="arm64-linux-android"
        _CMAKE_PROCESSOR="aarch64"
        _NDK_TOOLCHAIN_DIR="armv7a-linux-android"   # NDK dir contains all ABIs
        _NDK_SYSROOT_ARCH="aarch64-linux-android"
        ;;
    x86)
        _CC_TRIPLE="i686-linux-android${ANDROID_API}-clang"
        _CXX_TRIPLE="i686-linux-android${ANDROID_API}-clang++"
        _AR_PREFIX="i686-linux-android-"
        _AUTOTOOLS_ARCH="i686"
        _OUTTARGET="x86-linux-android"
        _CMAKE_PROCESSOR="x86"
        _NDK_TOOLCHAIN_DIR="armv7a-linux-android"
        _NDK_SYSROOT_ARCH="i686-linux-android"
        ;;
    x86_64)
        _CC_TRIPLE="x86_64-linux-android${ANDROID_API}-clang"
        _CXX_TRIPLE="x86_64-linux-android${ANDROID_API}-clang++"
        _AR_PREFIX="x86_64-linux-android-"
        _AUTOTOOLS_ARCH="x86_64"
        _OUTTARGET="x86_64-linux-android"
        _CMAKE_PROCESSOR="x86_64"
        _NDK_TOOLCHAIN_DIR="armv7a-linux-android"
        _NDK_SYSROOT_ARCH="x86_64-linux-android"
        ;;
    *)
        echo "Unsupported ABI: ${ANDROID_ABI}"
        echo "Supported: armeabi-v7a | arm64-v8a | x86 | x86_64"
        exit 1
        ;;
esac

export _CMAKE_PROCESSOR   # read by litert-build.sh when writing the toolchain file

echo "==========================================================="
echo " EHS Android contrib build"
echo "   ABI      : ${ANDROID_ABI}"
echo "   API      : ${ANDROID_API}"
echo "   OUTTARGET: ${_OUTTARGET}"
echo "   Jobs     : ${PROCESSORS}"
echo "==========================================================="

# ---------------------------------------------------------------------------
# xbuilder environment — sets up CC, CXX, AR, RANLIB, PATH, SYSROOT, and
# the USRLIB_* output paths used by build_component / build_openssl_component.
# ---------------------------------------------------------------------------
INX_PROJECT_NAME="android-api${ANDROID_API}"
INX_GLIBC_VERSION=""
# ARCH/OS produce TARGET="${ARCH}-${OS}" inside xbuilder, which is passed as
# --host to autotools configure.  Use the simple form (arm-linux, etc.) so
# that old config.sub scripts in the contrib packages accept it.
# The actual compiler is controlled by CC/CXX (INX_CC/INX_CXX), not TARGET.
ARCH="${_AUTOTOOLS_ARCH}"
OS="linux"
EXIT_ON_FAIL=true
TOOLCHAIN_VERSION="${_NDK_TOOLCHAIN_DIR}"
TOOLCHAIN_BIN_PREFIX="${_AR_PREFIX}"
INX_CC="${_CC_TRIPLE}"
INX_CXX="${_CXX_TRIPLE}"
INX_LD="${_CC_TRIPLE}"

# inx-xbuilder-source-me.sh and the build_component / build_openssl_component
# functions it defines use optional positional arguments ($3–$6) that are
# unbound when not supplied — incompatible with nounset (-u).  Disable for
# the rest of the script from here.
set +u
source ./source-scripts/inx-xbuilder-source-me.sh

# Override OUTTARGET — the source script derives a longer composite name;
# ert-components expects the short form (make chkconfig confirms the match).
OUTTARGET="${_OUTTARGET}"
export TARGET_PATH_FROM_COMPONENT_SOURCE_DIR="${TEMP_PWD}/../target_libs/${OUTTARGET}"
export USRLIB_BUILD_ROOT="${TARGET_PATH_FROM_COMPONENT_SOURCE_DIR}/build"
export USRLIB_INCLUDE_PATH="${USRLIB_BUILD_ROOT}/include"
export USRLIB_LIBRARY_PATH="${USRLIB_BUILD_ROOT}/lib"
mkdir -p "${USRLIB_BUILD_ROOT}/lib" "${USRLIB_BUILD_ROOT}/include"

# Expose the NDK toolchain path for litert-build.sh (toolchain file generation).
export TOOLCHAIN_PATH="${TEMP_PWD}/../../ert-build-support/toolchains/${BUILDHOSTPREFIX}/${_NDK_TOOLCHAIN_DIR}"

# Compiler flags common to all Android targets.
export CFLAGS="-DANDROID -fPIC -D__ANDROID_API__=${ANDROID_API}"

# ---------------------------------------------------------------------------
# Standard EHS contrib components (curl, openssl, xml, etc.)
# These are all autotools-based and cross-compile cleanly with the above env.
#
# If artefacts already exist in USRLIB_LIBRARY_PATH (e.g. from a previous
# build run), the builds are skipped automatically.  Set FORCE_REBUILD=1
# to rebuild everything from source.
# ---------------------------------------------------------------------------
if [ -f "${USRLIB_LIBRARY_PATH}/libcrypto.a" ] && \
   [ -f "${USRLIB_LIBRARY_PATH}/libcurl.a" ] && \
   [ -f "${USRLIB_LIBRARY_PATH}/libexpat.a" ]; then
    echo "NOTE: Standard contrib libs already present — skipping autotools rebuilds."
    echo "      (Set FORCE_REBUILD=1 to rebuild from source.)"
    if [ -z "${FORCE_REBUILD:-}" ]; then
        export BUILD_OPTIONS=SKIP_MAKE
    fi
fi

build_component libpthread-stubs -0.3
# libidn (internationalized domain names) is not required for Android builds.
build_openssl_component openssl -1.0.1u "linux-armv4 --prefix=${USRLIB_BUILD_ROOT} --openssldir=${USRLIB_BUILD_ROOT}/ssl no-shared no-asm -fPIC"
build_component curl -7.88.1 "--without-random --without-libidn --with-ssl=${USRLIB_BUILD_ROOT}"
build_component libarchive -3.6.1 "--with-sysroot=${SYSROOT} --without-xml2"
build_component expat -2.0.1
build_component libxml2 .X "--without-python"

# Restore BUILD_OPTIONS before LiteRT (must actually build).
unset BUILD_OPTIONS

# ---------------------------------------------------------------------------
# LiteRT (TFLite) C API — static library built from source via CMake.
#
# Source is cloned once into staging/src/litert-v<VERSION>/ and reused
# across ABI builds.  CMake build trees are per-ABI in staging/build/.
# Output is a single merged fat archive: liblitert_c.a
#
# cmake and an Android NDK with android.toolchain.cmake are required.
# They are discovered below in priority order; override by setting CMAKE
# and/or ANDROID_NDK in the environment before running this script.
#
# See source-scripts/litert-build.sh for full implementation notes.
# ---------------------------------------------------------------------------
export LITERT_VERSION="2.17.0"
export ANDROID_ABI ANDROID_API PROCESSORS

# --- cmake discovery --------------------------------------------------------
if [ -z "${CMAKE}" ]; then
    if command -v cmake &>/dev/null; then
        export CMAKE="$(command -v cmake)"
    else
        # Unity Android Player bundles cmake — use it if present.
        _UNITY_CMAKE=$(find /home "${HOME}" /opt -maxdepth 8 \
            -path "*/PlaybackEngines/AndroidPlayer/SDK/cmake/*/bin/cmake" \
            -type f 2>/dev/null | sort -V | tail -1)
        if [ -n "${_UNITY_CMAKE}" ]; then
            export CMAKE="${_UNITY_CMAKE}"
        else
            echo "ERROR: cmake not found. Install cmake >= 3.16 or set CMAKE=<path>."
            exit 1
        fi
    fi
fi
echo "  cmake : ${CMAKE} ($(${CMAKE} --version | head -1))"

# --- Android NDK discovery --------------------------------------------------
if [ -z "${ANDROID_NDK}" ]; then
    # Priority order:
    #  1. Docker image standard path  (/opt/android-ndk-r27c set by Dockerfile ENV)
    #  2. ert-build-support toolchain dir (stripped NDK — has android.toolchain.cmake)
    #  3. Unity Android Player NDK   (developer workstations with Unity installed)
    if [ -f "/opt/android-ndk-r27c/build/cmake/android.toolchain.cmake" ]; then
        export ANDROID_NDK="/opt/android-ndk-r27c"
    elif [ -f "${TOOLCHAIN_PATH}/../build/cmake/android.toolchain.cmake" ]; then
        export ANDROID_NDK="$(realpath "${TOOLCHAIN_PATH}/..")"
    else
        _UNITY_NDK=$(find /home "${HOME}" /opt -maxdepth 8 \
            -path "*/PlaybackEngines/AndroidPlayer/NDK/build/cmake/android.toolchain.cmake" \
            -type f 2>/dev/null | head -1 | sed 's|/build/cmake/android.toolchain.cmake||')
        if [ -n "${_UNITY_NDK}" ]; then
            export ANDROID_NDK="${_UNITY_NDK}"
        else
            echo "ERROR: Android NDK not found. Options:"
            echo "  1. Run inside Docker: docker build -f Dockerfile.android-ndk-build -t inxware/ubuntu22.04-android-ndk-build ."
            echo "  2. Set ANDROID_NDK=<ndk-root> (must contain build/cmake/android.toolchain.cmake)"
            exit 1
        fi
    fi
fi
echo "  NDK   : ${ANDROID_NDK}"

source ./source-scripts/litert-build.sh
build_litert_static

# ---------------------------------------------------------------------------
# Extract libc++_shared.so from the NDK — version-coherent with liblitert_c.a.
#
# liblitert_c.a was compiled with NDK r27c's clang 18.  The libc++_shared.so
# that ships at runtime must come from the same NDK so ABI and symbol versions
# match.  We copy it into USRLIB_LIBRARY_PATH alongside liblitert_c.a so that
# ert-components' targetenv_make_apk.sh and targetenv_unity_export.sh can find
# it at a predictable path (EHS_COMPONENT_SUPPORT_LIBS/libc++_shared.so) and
# bundle it into the APK's jniLibs directory.
# ---------------------------------------------------------------------------
_NDK_LIBCPP="${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/${_NDK_SYSROOT_ARCH}/libc++_shared.so"
if [ -f "${_NDK_LIBCPP}" ]; then
    cp "${_NDK_LIBCPP}" "${USRLIB_LIBRARY_PATH}/libc++_shared.so"
    echo "  libc++_shared.so extracted from NDK r27c (${_NDK_SYSROOT_ARCH}) → ${USRLIB_LIBRARY_PATH}/libc++_shared.so"
else
    echo "WARNING: libc++_shared.so not found at ${_NDK_LIBCPP}"
fi

# ---------------------------------------------------------------------------
# Collect runtime .so files into target_packages/cslib for APK packaging.
# (LiteRT is static so it adds nothing here; curl, libxml2 etc. are static too.)
# The install script's cp may fail when no real .so files exist (only dangling
# autotools symlinks) — tolerate that with set +e here.
# ---------------------------------------------------------------------------
set +e
source source-scripts/inx-install-target-libs.sh
set -e
