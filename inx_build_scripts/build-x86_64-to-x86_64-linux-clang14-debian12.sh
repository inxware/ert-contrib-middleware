#!/bin/bash
set -e

# Build contrib middleware for x86_64 Debian 12 (clang 14, default toolchain).
#
# Output: target_libs/x86_64-linux-gnu-clang14_debian12_base/
#   build/lib/     — static AWS CRT archives + TFLite shared libs (link-time)
#   build/include/ — AWS CRT + TFLite C API headers
#   target_packages/cslib/ — shared libs deployed to target (TFLite only)
#
# Debian 12 (bookworm) provides opencv 4.6, ffmpeg 5.1, abseil, TBB etc. via apt.
# Only components absent from Debian 12 repositories are built here:
#   - AWS CRT stack  (aws-lc, s2n-tls, aws-c-common/cal/io/compression/http/mqtt)
#   - TensorFlow Lite 2.17.0 (libtensorflowlite_c.so, libtensorflow-lite.so)
#
# AWS CRT version set — same source tarballs as the Debian 13 build:
#   aws-lc            v1.69.0   → contrib/aws-lc/aws-lc-v1.69.0/
#   aws-c-common      v0.12.6   → contrib/aws-c-common/aws-c-common-v0.12.6/
#   s2n-tls           1.7.1     → contrib/s2n-tls/s2n-tls-1.7.1/
#   aws-c-cal         v0.9.13   → contrib/aws-c-cal/aws-c-cal-v0.9.13/
#   aws-c-io          v0.26.1   → contrib/aws-c-io/aws-c-io-v0.26.1/
#   aws-c-compression (latest)  → contrib/aws-c-compression/aws-c-compression/
#   aws-c-http        v0.10.11  → contrib/aws-c-http/aws-c-http-v0.10.11/
#   aws-c-mqtt        v0.14.0   → contrib/aws-c-mqtt/aws-c-mqtt-v0.14.0/

IMAGE_NAME=inxware/inx-debian12-clang14

source ./source-scripts/inx-dockersetup-source-me.sh
check_and_run_docker "$IMAGE_NAME" "$@"

##############################################################################
# Path setup
##############################################################################

OUTTARGET=x86_64-linux-gnu-clang14_debian12_base
PROCESSORS=$(nproc)

SCRIPTS_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTRIB_ROOT="$(cd "${SCRIPTS_DIR}/.." && pwd)"

OUT="${CONTRIB_ROOT}/target_libs/${OUTTARGET}/build"
CSLIB="${CONTRIB_ROOT}/target_libs/${OUTTARGET}/target_packages/cslib"
STAGING="${CONTRIB_ROOT}/staging"

mkdir -p "${OUT}/lib" "${OUT}/include" "${CSLIB}"
mkdir -p "${STAGING}/src" "${STAGING}/build"

export CC=clang
export CXX=clang++
export AR=llvm-ar
CMAKE_COMMON="-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${OUT} -DCMAKE_MODULE_PATH=${OUT}/lib/cmake -DBUILD_TESTING=OFF"

AWSLC_DIR="${CONTRIB_ROOT}/contrib/aws-lc/aws-lc-v1.69.0"
AWS_COMMON_DIR="${CONTRIB_ROOT}/contrib/aws-c-common/aws-c-common-v0.12.6"
S2N_DIR="${CONTRIB_ROOT}/contrib/s2n-tls/s2n-tls-1.7.1"
AWS_CAL_DIR="${CONTRIB_ROOT}/contrib/aws-c-cal/aws-c-cal-v0.9.13"
AWS_IO_DIR="${CONTRIB_ROOT}/contrib/aws-c-io/aws-c-io-v0.26.1"
AWS_COMPRESSION_DIR="${CONTRIB_ROOT}/contrib/aws-c-compression/aws-c-compression"
AWS_HTTP_DIR="${CONTRIB_ROOT}/contrib/aws-c-http/aws-c-http-v0.10.11"
AWS_MQTT_DIR="${CONTRIB_ROOT}/contrib/aws-c-mqtt/aws-c-mqtt-v0.14.0"

for DIR in "${AWSLC_DIR}" "${AWS_COMMON_DIR}" "${S2N_DIR}" "${AWS_CAL_DIR}" \
           "${AWS_IO_DIR}" "${AWS_COMPRESSION_DIR}" "${AWS_HTTP_DIR}" "${AWS_MQTT_DIR}"; do
    if [ ! -d "${DIR}" ]; then
        echo "ERROR: source directory not found: ${DIR}"
        echo "Run fetch-aws-crt-sources.sh first."
        exit 1
    fi
done

##############################################################################
# AWS CRT stack — built as static libraries
##############################################################################

build_cmake() {
    local SRC="$1"; shift
    local EXTRA_ARGS="$*"
    local BUILD="${SRC}/build-debian12"
    rm -rf "${BUILD}" && mkdir "${BUILD}"
    cmake -S "${SRC}" -B "${BUILD}" ${CMAKE_COMMON} ${EXTRA_ARGS}
    cmake --build "${BUILD}" --parallel "${PROCESSORS}"
    cmake --install "${BUILD}"
}

echo "### Building aws-lc ###"
build_cmake "${AWSLC_DIR}" -DDISABLE_GO=ON

echo "### Building aws-c-common ###"
build_cmake "${AWS_COMMON_DIR}"

echo "### Building s2n-tls ###"
build_cmake "${S2N_DIR}" -DS2N_NO_PQ=ON

echo "### Building aws-c-cal ###"
build_cmake "${AWS_CAL_DIR}"

echo "### Building aws-c-io ###"
build_cmake "${AWS_IO_DIR}" "-DCMAKE_C_FLAGS=-DINTEL_NO_ITTNOTIFY_API" "-DCMAKE_CXX_FLAGS=-DINTEL_NO_ITTNOTIFY_API"

echo "### Building aws-c-compression ###"
build_cmake "${AWS_COMPRESSION_DIR}"

echo "### Building aws-c-http ###"
build_cmake "${AWS_HTTP_DIR}"

echo "### Building aws-c-mqtt ###"
build_cmake "${AWS_MQTT_DIR}"

##############################################################################
# TensorFlow Lite 2.17.0 — shared libraries
##############################################################################

LITERT_VERSION=2.17.0
SRC_DIR="${STAGING}/src/litert-v${LITERT_VERSION}"
BUILD_DIR_C="${STAGING}/build/litert-x86_64-linux-debian12-c"
BUILD_DIR_CPP="${STAGING}/build/litert-x86_64-linux-debian12-cpp"

echo "### Fetching TFLite ${LITERT_VERSION} source ###"
if [ ! -d "${SRC_DIR}/.git" ]; then
    git -c credential.helper= clone \
        --depth 1 --branch "v${LITERT_VERSION}" \
        --filter=blob:none --sparse \
        https://github.com/tensorflow/tensorflow.git "${SRC_DIR}"
    pushd "${SRC_DIR}" >/dev/null
    git sparse-checkout set \
        tensorflow/lite tensorflow/core tensorflow/tsl \
        tensorflow/compiler/mlir/lite third_party
    git checkout
    popd >/dev/null
fi

echo "### Building libtensorflowlite_c.so ###"
if [ ! -f "${BUILD_DIR_C}/.configure_done" ]; then
    rm -rf "${BUILD_DIR_C}" && mkdir -p "${BUILD_DIR_C}"
    cmake -S "${SRC_DIR}/tensorflow/lite/c" -B "${BUILD_DIR_C}" \
        -DTENSORFLOW_SOURCE_DIR="${SRC_DIR}" \
        -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
        -DTFLITE_C_BUILD_SHARED_LIBS=ON -DTFLITE_ENABLE_XNNPACK=ON \
        -DTFLITE_ENABLE_NNAPI=OFF -DTFLITE_ENABLE_GPU=OFF \
        -DTFLITE_BUILD_UNIT_TESTS=OFF -DTFLITE_ENABLE_INSTALL=OFF \
        -DTFLITE_ENABLE_EXAMPLES=OFF -DTFLITE_ENABLE_BENCHMARKS=OFF
    touch "${BUILD_DIR_C}/.configure_done"
fi
cmake --build "${BUILD_DIR_C}" --target tensorflowlite_c --parallel "${PROCESSORS}"
find "${BUILD_DIR_C}" -name "libtensorflowlite_c.so" | head -1 | xargs -I{} cp {} "${OUT}/lib/"

echo "### Building libtensorflow-lite.so ###"
if [ ! -f "${BUILD_DIR_CPP}/.configure_done" ]; then
    rm -rf "${BUILD_DIR_CPP}" && mkdir -p "${BUILD_DIR_CPP}"
    cmake -S "${SRC_DIR}/tensorflow/lite" -B "${BUILD_DIR_CPP}" \
        -DTENSORFLOW_SOURCE_DIR="${SRC_DIR}" \
        -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
        -DTFLITE_ENABLE_XNNPACK=ON -DTFLITE_ENABLE_NNAPI=OFF \
        -DTFLITE_ENABLE_GPU=OFF -DTFLITE_BUILD_UNIT_TESTS=OFF \
        -DTFLITE_ENABLE_INSTALL=OFF -DTFLITE_ENABLE_EXAMPLES=OFF \
        -DTFLITE_ENABLE_BENCHMARKS=OFF
    touch "${BUILD_DIR_CPP}/.configure_done"
fi
cmake --build "${BUILD_DIR_CPP}" --target tensorflow-lite --parallel "${PROCESSORS}"
find "${BUILD_DIR_CPP}" -name "libtensorflow-lite.so" | head -1 | xargs -I{} cp {} "${OUT}/lib/"

##############################################################################
# TFLite headers + cslib
##############################################################################

mkdir -p "${OUT}/include/tensorflow/lite/c" \
         "${OUT}/include/tensorflow/lite/core/c" \
         "${OUT}/include/tensorflow/lite/delegates/xnnpack"
rsync -a --quiet "${SRC_DIR}/tensorflow/lite/c/"     "${OUT}/include/tensorflow/lite/c/"
rsync -a --quiet "${SRC_DIR}/tensorflow/lite/core/c/" "${OUT}/include/tensorflow/lite/core/c/"
rsync -a --quiet "${SRC_DIR}/tensorflow/lite/delegates/xnnpack/" \
                 "${OUT}/include/tensorflow/lite/delegates/xnnpack/"

cp "${OUT}/lib/libtensorflowlite_c.so" "${CSLIB}/"
cp "${OUT}/lib/libtensorflow-lite.so"  "${CSLIB}/"

echo "### Build complete: ${OUTTARGET} ###"
