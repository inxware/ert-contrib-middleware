#!/bin/bash
#---------------------------------------------------------------
# Copyright (C) 2025 inx limited, UK - All Rights Reserved
# LGPLv3 license — see https://www.gnu.org/licenses/lgpl-3.0.txt
#---------------------------------------------------------------
#
# build-xmos-libs.sh
#
# Builds the XMOS SDK libraries for the xcore_freertos-xcore-xtc-15.x target
# and installs the results into ert-contrib-middleware/target_libs/.
#
# Run this script INSIDE the xcore Docker build container:
#   make target_buildenv   (from ert-components)
#   cd /inxware/ert-contrib-middleware/contrib/xmos-sdk
#   ./build-xmos-libs.sh
#
# Prerequisites:
#   - XMOS_TOOL_PATH must be set (done by Docker ENV)
#   - xcc must be on PATH (done by Docker ENV)
#   - Source repositories must be cloned into this directory
#     (see README.md for the git clone commands)
#
# ---------------------------------------------------------------------------
# CURRENT BUILD STATUS — INCOMPLETE / BLOCKED
# ---------------------------------------------------------------------------
#
# CURRENT PROBLEM
# ---------------
# This script drives cmake directly on each XMOS SDK source tree as a
# top-level project.  That is not how the SDK is designed to be used.
# Two failures result:
#
#   1. fwk_rtos CMakeLists.txt only activates the FreeRTOS port, RTOS osal
#      layer, and host-tool suppression when CMAKE_SYSTEM_NAME is "XCORE_XS3A"
#      or "XCORE_XS2A".  Those system name values require cmake platform files
#      that are shipped by xcommon_cmake (not by cmake itself, and not by XTC
#      Tools alone).  Without xcommon_cmake on CMAKE_MODULE_PATH the cmake
#      configure step fails with "FreeRTOS port does not exist for Generic!"
#      and "Unsupported compiler" in fatfs_mkimage (a host tool that should
#      be skipped entirely for xcore targets).
#
#   2. fetch_ci_deps.cmake (included by fwk_rtos when top-level) uses a
#      non-standard IF(EXISTS ${CMAKE_BINARY_DIR}/dependencies/<dep>) check
#      rather than standard FetchContent overrides.  FETCHCONTENT_SOURCE_DIR_*
#      variables have no effect.  The current workaround (pre-symlinking into
#      _build_inx/dependencies/) sidesteps this but is fragile.
#
# HOW THE SDK IS NORMALLY USED
# -----------------------------
# fwk_rtos and companion libs are not standalone build targets.  They are
# consumed as subdirectories by an *application* project built via
# xcommon_cmake.  A normal XMOS application looks like:
#
#   app/
#     CMakeLists.txt    <- sets CMAKE_MODULE_PATH to xcommon_cmake/,
#                          declares the app using XMOS cmake macros
#     deps.cmake        <- lists fwk_rtos, fwk_io, etc. as dependencies
#
# xcommon_cmake provides the XCORE_XS3A platform files, sets up xcc as the
# compiler, and provides XMOS-specific macros.  The application cmake
# configures CMAKE_SYSTEM_NAME itself — it is never set by the developer.
# The result is a single .xe firmware image, not a collection of .a files.
#
# ---------------------------------------------------------------------------
# ALTERNATIVE APPROACHES — ASSESSED
# ---------------------------------------------------------------------------
#
# OPTION 1 — ESP-IDF "dummy app" pattern  [Confidence: MEDIUM ~55%]
#   Drive the build through a minimal xcommon_cmake application project
#   (similar to how ESP32-S3 uses the IDF blink example), then extract the
#   compiled .a files and headers from the build tree and commit them to
#   target_libs/ as pre-built artifacts.
#   Main unknowns: xcommon_cmake may require a full board BSP and XN file
#   even for a library-only configure; it is also unclear whether fwk_rtos
#   emits discrete .a files per library or merges everything into the final
#   .xe link step (in which case extraction is not straightforward).
#
# OPTION 2 — Two-phase build: make all → .a, then targetenv_xmos_docker
#            → .xe  [Confidence: HIGH ~85%]
#
#   This is the recommended approach.  It mirrors what already exists for
#   Unity (targetenv_unity_export_docker) and ESP32 (targetenv_esp32_docker)
#   in ert-components, and avoids extracting .a files from the XMOS build
#   system entirely.
#
#   PHASE 1 — make all / make all_docker  (compiles ert-components to a .a)
#   -------------------------------------------------------------------------
#   The xcore toolchain.mk already compiles .c → .o using xcc.  The only
#   change needed is to archive the objects into a static library instead of
#   linking them into a .xe.  In target/os-arch/xcore_freertos-xcore/:
#
#     target.mk: set FINAL=a, EXE=a
#     toolchain.mk: add AR := xar (XMOS archiver, part of XTC Tools)
#     Makefile link rule: replace $(LINK) ... with $(AR) rcs $(TARGET_NAME).a $(OBJECTS)
#
#   Phase 1 only needs the SDK *headers* (already in target_libs/include/),
#   not the SDK .a files.  It produces:
#     ehs_xcore_freertos-xcore-base.a
#
#   PHASE 2 — make targetenv_xmos_docker  (links final .xe via xcommon_cmake)
#   -------------------------------------------------------------------------
#   A new make target in ert-components, following the Unity/ESP32 pattern:
#
#     Makefile:
#       targetenv_xmos_docker: chkconfig
#           @./target/envbuildscripts/targetenv_xmos_docker.sh $(TARGET)
#
#     targetenv_xmos_docker.sh:
#       ./target/envbuildscripts/target_buildenv_run_command.sh \
#           sh -c "./target/envbuildscripts/targetenv_xmos.sh ${TARGET}"
#       (identical structure to targetenv_unity_export_docker.sh)
#
#     targetenv_xmos.sh  (runs inside the XMOS Docker container):
#       1. Checks that ehs_$(TARGET).a exists (Phase 1 must have run first).
#       2. Invokes cmake on a small "link-only" xcommon_cmake wrapper app in
#          contrib/xmos-sdk/xcore-ehs-app/:
#            cmake -DCMAKE_MODULE_PATH=<xcommon_cmake> \
#                  -DXMOS_BOARD_TARGET=$(XMOS_BOARD_TARGET) \
#                  -DEHS_LIB=<path to ehs_$(TARGET).a>
#       3. cmake + make inside the container produces ehs_$(TARGET).xe.
#       4. Copies ehs_$(TARGET).xe to
#            ../TARGET_TREES/ehs_env-$(TARGET)/bin/ehs.xe
#
#   The xcommon_cmake wrapper app (contrib/xmos-sdk/xcore-ehs-app/) is a
#   minimal XMOS application CMakeLists.txt that:
#     - Sets CMAKE_MODULE_PATH to the local xcommon_cmake/ clone
#     - Declares a XMOS app target using XMOS cmake macros
#     - Adds fwk_rtos (and fwk_io, fwk_core) as subdirectory dependencies
#     - Links against ${EHS_LIB} (the .a from Phase 1)
#     - Passes the XN board file via the standard xcommon_cmake mechanism
#   This is pure XMOS-idiomatic cmake — no cmake hacks required.
#
#   Infrastructure already in place in ert-components:
#     - target/platform/xcore_freertos-xcore-base/Dockerimagename
#     - target/envbuildscripts/target_buildenv_run_command.sh
#     - target/envbuildscripts/all_docker.sh (Phase 1 Docker wrapper)
#     - target/os-arch/xcore_freertos-xcore/toolchain.mk (xcc already set up)
#
# OPTION 3 — Compile SDK sources directly in the ert-components make build
#            [Confidence: MEDIUM ~45%]
#   Add fwk_rtos source files directly to OBJECTS in target.mk.  Eliminates
#   the separate SDK build step.  Risk: fwk_rtos has CMake-generated headers
#   (FreeRTOSConfig, lwIP opts, etc.) that must be manually replicated for
#   make; maintenance burden grows with each SDK update.
#
# OPTION 4 — Full xcommon_cmake application restructure  [Confidence: LOW ~35%]
#   Restructure the entire xcore ert-components build as an xcommon_cmake
#   app.  Eliminates the make/cmake split but replaces the entire xcore
#   toolchain.mk/target.mk with xcommon_cmake — high restructuring cost for
#   one target.
#
# RECOMMENDATION
#   Option 2.  It uses the XMOS SDK exactly as intended (xcommon_cmake app
#   for the final link), keeps the ert-components compile stage in the
#   existing make build, and slots into the targetenv_*_docker pattern that
#   is already established for Unity and ESP32.  This script (build-xmos-
#   libs.sh) would be retired once Option 2 is implemented.
# ---------------------------------------------------------------------------

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MIDDLEWARE_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
OUTPUT_ROOT="${MIDDLEWARE_ROOT}/target_libs/xcore_freertos-xcore-xtc-15.x/build"
OUTPUT_INC="${OUTPUT_ROOT}/include"
OUTPUT_LIB="${OUTPUT_ROOT}/lib"

# Board target — must match XMOS_BOARD_TARGET in platform config.mk
BOARD_TARGET="${XMOS_BOARD_TARGET:-XCORE-AI-EXPLORER}"

echo "=== XMOS SDK library build ==="
echo "    XMOS_TOOL_PATH : ${XMOS_TOOL_PATH}"
echo "    Board target   : ${BOARD_TARGET}"
echo "    Output root    : ${OUTPUT_ROOT}"
echo ""

# Validate environment
if [ -z "${XMOS_TOOL_PATH}" ]; then
    echo "ERROR: XMOS_TOOL_PATH is not set. Run this inside the xcore Docker container."
    exit 1
fi

if ! command -v xcc &> /dev/null; then
    echo "ERROR: xcc not found on PATH. Run this inside the xcore Docker container."
    exit 1
fi

mkdir -p "${OUTPUT_INC}" "${OUTPUT_LIB}"

# ---------------------------------------------------------------------------
# fwk_io is a dependency of fwk_rtos fetched at cmake time via FetchContent.
# The build container has no SSH, so we must provide a local pre-cloned copy
# and set FETCHCONTENT_FULLY_DISCONNECTED to block all network access.
# Clone on the host before running this script:
#   git clone --recurse-submodules https://github.com/xmos/fwk_io.git
# ---------------------------------------------------------------------------
if [ ! -d "${SCRIPT_DIR}/fwk_io" ]; then
    echo "ERROR: fwk_io/ not found."
    echo "       Clone it on the host before running this script:"
    echo "         git clone --recurse-submodules https://github.com/xmos/fwk_io.git"
    exit 1
fi

# ---------------------------------------------------------------------------
# Helper: cmake build for a library using xcommon_cmake
# Usage: build_lib <source_dir> <cmake_target> [extra_cmake_args...]
# ---------------------------------------------------------------------------
build_lib() {
    local SRC_DIR="$1"
    local LIB_NAME="$2"
    shift 2
    local BUILD_DIR="${SRC_DIR}/_build_inx"

    echo "--- Building ${LIB_NAME} ---"
    mkdir -p "${BUILD_DIR}/dependencies"

    # fwk_rtos uses a custom fetch_ci_deps.cmake that checks
    # ${CMAKE_BINARY_DIR}/dependencies/<dep> directly rather than the standard
    # FetchContent override mechanism.  Pre-create symlinks so cmake skips all
    # network fetches (the container has no SSH/network access).
    for DEP in fwk_io fwk_core lib_qspi_fast_read; do
        if [ -d "${SCRIPT_DIR}/${DEP}" ] && [ ! -e "${BUILD_DIR}/dependencies/${DEP}" ]; then
            ln -s "${SCRIPT_DIR}/${DEP}" "${BUILD_DIR}/dependencies/${DEP}"
        fi
    done

    cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/xmos-toolchain.cmake" \
        -DXMOS_BOARD_TARGET="${BOARD_TARGET}" \
        -DCMAKE_INSTALL_PREFIX="${OUTPUT_ROOT}" \
        "$@"
    cmake --build "${BUILD_DIR}" --parallel
    cmake --install "${BUILD_DIR}"
    echo "--- ${LIB_NAME} done ---"
    echo ""
}

# ---------------------------------------------------------------------------
# fwk_rtos — FreeRTOS SMP kernel + RTOS support layer + lwIP + FatFS
# ---------------------------------------------------------------------------
if [ -d "${SCRIPT_DIR}/fwk_rtos" ]; then
    build_lib "${SCRIPT_DIR}/fwk_rtos" "fwk_rtos"
else
    echo "WARNING: fwk_rtos/ not found — skipping. Run: git clone https://github.com/xmos/fwk_rtos.git"
fi

# ---------------------------------------------------------------------------
# lib_xcore_math — VPU-accelerated maths
# ---------------------------------------------------------------------------
if [ -d "${SCRIPT_DIR}/lib_xcore_math" ]; then
    build_lib "${SCRIPT_DIR}/lib_xcore_math" "lib_xcore_math"
else
    echo "WARNING: lib_xcore_math/ not found — skipping."
fi

# ---------------------------------------------------------------------------
# lib_i2c
# ---------------------------------------------------------------------------
if [ -d "${SCRIPT_DIR}/lib_i2c" ]; then
    build_lib "${SCRIPT_DIR}/lib_i2c" "lib_i2c"
else
    echo "WARNING: lib_i2c/ not found — skipping."
fi

# ---------------------------------------------------------------------------
# lib_uart
# ---------------------------------------------------------------------------
if [ -d "${SCRIPT_DIR}/lib_uart" ]; then
    build_lib "${SCRIPT_DIR}/lib_uart" "lib_uart"
else
    echo "WARNING: lib_uart/ not found — skipping."
fi

# ---------------------------------------------------------------------------
# lib_i2s
# ---------------------------------------------------------------------------
if [ -d "${SCRIPT_DIR}/lib_i2s" ]; then
    build_lib "${SCRIPT_DIR}/lib_i2s" "lib_i2s"
else
    echo "WARNING: lib_i2s/ not found — skipping."
fi

echo "=== Build complete ==="
echo "    Headers : ${OUTPUT_INC}"
echo "    Libs    : ${OUTPUT_LIB}"
echo ""
echo "Commit the results to ert-contrib-middleware:"
echo "  git add target_libs/xcore_freertos-xcore-xtc-15.x/"
echo "  git commit -m 'feat(xmos): add pre-built SDK libs for XTC Tools 15.x'"
