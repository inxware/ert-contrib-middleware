#!/bin/bash
# litert-build.sh — sourced by build-android-ehs.sh (and future platform scripts).
#
# Provides build_litert_static(), which:
#   1. Sparse-clones the LiteRT/TFLite source at a pinned version tag into a
#      shared staging directory (re-used across ABI builds; only fetched once).
#   2. Configures and builds libtensorflowlite_c (static) via CMake using the
#      Android NDK's official toolchain file.
#   3. Merges all resulting .a archives into a single fat archive liblitert_c.a
#      so ert-components links against one file with no ordering sensitivity.
#   4. Installs the C API headers alongside the archive.
#
# Inputs — set by the caller (build-android-ehs.sh) before sourcing:
#
#   LITERT_VERSION    TFLite/LiteRT version tag without 'v' prefix, e.g. "2.14.0"
#   ANDROID_ABI       Android ABI string:  armeabi-v7a | arm64-v8a | x86 | x86_64
#   ANDROID_API       Android API level integer, e.g. "30"
#   CMAKE             Full path to the cmake binary
#   ANDROID_NDK       Full path to the Android NDK root (contains build/cmake/android.toolchain.cmake)
#   AR                ar binary used for merging archives (e.g. arm-linux-androideabi-ar)
#   USRLIB_LIBRARY_PATH  Output directory for liblitert_c.a
#   USRLIB_INCLUDE_PATH  Output directory for C API headers
#   PROCESSORS        Parallel build job count

build_litert_static() {

    local VERSION="${LITERT_VERSION}"
    local ABI="${ANDROID_ABI}"
    local API="${ANDROID_API}"

    # XNNPACK is enabled for all ABIs including armeabi-v7a (ARM32 NEON is a
    # first-class XNNPACK target).  The '=t' x87 asm constraint that broke ARM32
    # builds with clang 18 / NDK r27c was a bug in the specific XNNPACK commit
    # bundled by TFLite 2.14.0 — it is fixed in newer XNNPACK (2.17.0+).
    local XNNPACK_ENABLED="ON"
    local OUT_LIB="${USRLIB_LIBRARY_PATH}"
    local OUT_INC="${USRLIB_INCLUDE_PATH}"
    local JOBS="${PROCESSORS:-4}"
    local CMAKE_BIN="${CMAKE}"
    local NDK="${ANDROID_NDK}"
    local AR_BIN="${AR}"

    # Staging root sits at the ert-contrib-middleware root (one level above inx_build_scripts/).
    local SCRIPTS_DIR
    SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
    local STAGING_ROOT
    STAGING_ROOT="$(cd "${SCRIPTS_DIR}/.." && pwd)/staging"

    local SRC_DIR="${STAGING_ROOT}/src/litert-v${VERSION}"
    local BUILD_DIR="${STAGING_ROOT}/build/litert-${ABI}-api${API}"
    local FAT_ARCHIVE="${OUT_LIB}/liblitert_c.a"

    echo ""
    echo "### Building LiteRT ${VERSION} static lib (${ABI}, Android API ${API}) ###"
    echo "  cmake   : ${CMAKE_BIN}"
    echo "  NDK     : ${NDK}"
    echo "  Source  : ${SRC_DIR}"
    echo "  Build   : ${BUILD_DIR}"
    echo "  Output  : ${FAT_ARCHIVE}"
    echo ""

    mkdir -p "${STAGING_ROOT}/src" "${STAGING_ROOT}/build" "${OUT_LIB}" "${OUT_INC}"

    # -------------------------------------------------------------------------
    # 1. Clone (or reuse) the TFLite source — sparse checkout of the lite/
    #    subtree only.  Deps (flatbuffers, abseil, XNNPACK, …) are fetched by
    #    CMake FetchContent at configure time; internet access is required.
    #    The source dir is shared across ABI builds — clone only once.
    # -------------------------------------------------------------------------
    if [ ! -d "${SRC_DIR}/.git" ]; then
        echo "  [1/4] Cloning LiteRT v${VERSION} source (sparse, depth 1)..."
        git clone \
            --depth 1 \
            --branch "v${VERSION}" \
            --filter=blob:none \
            --sparse \
            https://github.com/tensorflow/tensorflow.git \
            "${SRC_DIR}"
        pushd "${SRC_DIR}" >/dev/null
        # tensorflow/lite              — TFLite core + CMakeLists.txt
        # tensorflow/core              — platform headers referenced by TFLite CMake
        # tensorflow/tsl               — TensorFlow Standard Library (stats_calculator.cc
        #                               referenced by unconditional benchmark/example targets)
        # tensorflow/compiler/mlir/lite — MLIR TFLite components required since 2.17.0:
        #                               schema/schema_utils.cc, kernels/internal/*.h, etc.
        # third_party                  — BUILD/cmake files for bundled deps
        git sparse-checkout set \
            tensorflow/lite \
            tensorflow/core \
            tensorflow/tsl \
            tensorflow/compiler/mlir/lite \
            third_party
        git checkout
        popd >/dev/null
        echo "  Source clone complete."
    else
        echo "  [1/4] Source already present at ${SRC_DIR} — skipping clone."
    fi

    # -------------------------------------------------------------------------
    # 2 & 3. CMake configure + build.
    #
    # Uses the Android NDK's official toolchain file so the compiler, sysroot,
    # ABI flags, and API level are all set correctly without a hand-written file.
    #
    # Key options:
    #   ANDROID_STL=c++_shared     → link against libc++_shared.so (system lib
    #                                 on Android 5.0+); avoids embedding the C++
    #                                 runtime in the archive and the duplicate-
    #                                 symbol problems that c++_static causes when
    #                                 multiple .so files are loaded in one process.
    #   TFLITE_C_BUILD_SHARED_LIB=OFF → produce static libtensorflowlite_c
    #   BUILD_SHARED_LIBS=OFF         → all dependencies static too
    #   TFLITE_ENABLE_XNNPACK         → OFF for armeabi-v7a (32-bit ARM, see
    #                                   XNNPACK_ENABLED above); ON for arm64+
    #   TFLITE_ENABLE_NNAPI=OFF       → NNAPI deprecated since Android 13
    #   TFLITE_ENABLE_GPU=OFF         → GPU delegate is a future addition
    # -------------------------------------------------------------------------
    # Use a sentinel file to track successful configure completion.
    # cmake can write a partial CMakeCache.txt even on failure (e.g. a
    # FetchContent FATAL_ERROR mid-configure), so checking CMakeCache.txt alone
    # is not sufficient to distinguish success from partial failure.
    local CONFIGURE_DONE_SENTINEL="${BUILD_DIR}/.configure_done"

    if [ ! -f "${CONFIGURE_DONE_SENTINEL}" ]; then
        echo "  [2/4] Configuring CMake build..."
        # Wipe any stale partial build tree from a previous failed configure.
        # cmake (especially 3.22) can leave root-owned CMakeFiles/ subdirectories
        # that prevent a clean re-configure if the host rm -rf didn't fully succeed.
        # Running rm -rf here (inside the container, as root) ensures a clean slate.
        rm -rf "${BUILD_DIR}"
        mkdir -p "${BUILD_DIR}"

        if ! "${CMAKE_BIN}" \
                -S "${SRC_DIR}/tensorflow/lite/c" \
                -B "${BUILD_DIR}" \
                -DTENSORFLOW_SOURCE_DIR="${SRC_DIR}" \
                -DCMAKE_TOOLCHAIN_FILE="${NDK}/build/cmake/android.toolchain.cmake" \
                -DANDROID_ABI="${ABI}" \
                -DANDROID_PLATFORM="android-${API}" \
                -DANDROID_STL="c++_shared" \
                -DCMAKE_BUILD_TYPE=Release \
                -DBUILD_SHARED_LIBS=OFF \
                -DTFLITE_C_BUILD_SHARED_LIBS=OFF \
                -DTFLITE_ENABLE_XNNPACK="${XNNPACK_ENABLED}" \
                -DTFLITE_ENABLE_NNAPI=OFF \
                -DTFLITE_ENABLE_GPU=OFF \
                -DTFLITE_BUILD_UNIT_TESTS=OFF \
                -DTFLITE_ENABLE_INSTALL=OFF \
                -DTFLITE_ENABLE_EXAMPLES=OFF \
                -DTFLITE_ENABLE_BENCHMARKS=OFF \
                -DCMAKE_VERBOSE_MAKEFILE=OFF; then

            echo ""
            echo "ERROR: CMake configure failed for ${ABI}."
            echo ""
            # Print FetchContent subbuild logs — these contain the nested
            # cmake/compiler error that triggered the top-level failure.
            for log_file in \
                "${BUILD_DIR}/_deps/xnnpack-subbuild/CMakeFiles/"*"/last_err.log" \
                "${BUILD_DIR}/_deps/xnnpack-subbuild/CMakeFiles/"*"/last_out.log" \
                "${BUILD_DIR}/_deps/"*"-subbuild/CMakeFiles/"*"/last_err.log"; do
                # shellcheck disable=SC2086
                for f in $log_file; do
                    [ -f "$f" ] || continue
                    echo "--- ${f} ---"
                    cat "$f"
                    echo ""
                done
            done
            echo "Hint: delete ${BUILD_DIR} and re-run to start a clean configure."
            return 1
        fi
        touch "${CONFIGURE_DONE_SENTINEL}"
    else
        echo "  [2/4] CMake already configured — skipping (delete ${BUILD_DIR} to reconfigure)."
    fi

    echo "  [3/4] Building libtensorflowlite_c target (${JOBS} parallel jobs)..."
    "${CMAKE_BIN}" --build "${BUILD_DIR}" \
        --target tensorflowlite_c \
        --parallel "${JOBS}"

    # -------------------------------------------------------------------------
    # 4. Merge all static archives from the build tree into a single fat archive.
    #
    #    ar -M reads an MRI script:
    #      CREATE  — start a new archive
    #      ADDLIB  — include all objects from an existing archive
    #      SAVE / END
    #
    #    Sorting the list ensures reproducible output across runs.
    # -------------------------------------------------------------------------
    echo "  [4/4] Merging static archives → ${FAT_ARCHIVE}..."

    {
        echo "CREATE ${FAT_ARCHIVE}"
        find "${BUILD_DIR}" -name "*.a" -not -name "liblitert_c.a" \
            | sort \
            | while read -r lib; do echo "ADDLIB ${lib}"; done
        echo "SAVE"
        echo "END"
    } | "${AR_BIN}" -M

    echo "  ${FAT_ARCHIVE} — $(du -sh "${FAT_ARCHIVE}" | cut -f1)"

    # Install C API headers from the source tree (idempotent via rsync).
    mkdir -p "${OUT_INC}/tensorflow/lite/c" \
             "${OUT_INC}/tensorflow/lite/core/c" \
             "${OUT_INC}/tensorflow/lite/delegates/xnnpack"

    rsync -a --quiet "${SRC_DIR}/tensorflow/lite/c/"             "${OUT_INC}/tensorflow/lite/c/"
    rsync -a --quiet "${SRC_DIR}/tensorflow/lite/core/c/"        "${OUT_INC}/tensorflow/lite/core/c/"
    rsync -a --quiet "${SRC_DIR}/tensorflow/lite/delegates/xnnpack/" \
                     "${OUT_INC}/tensorflow/lite/delegates/xnnpack/"

    echo "  Headers installed: ${OUT_INC}/tensorflow/lite/c/c_api.h"
    echo ""
    echo "### LiteRT ${VERSION} build complete (${ABI}) ###"
    echo ""
}
