cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# xcore-ehs-app — xcommon_cmake wrapper for Phase 2 of the eRT xcore build
#
# Links the eRT static library (Phase 1 output from 'make all') against
# the XMOS fwk_rtos SDK to produce the final ehs.xe firmware image.
#
# Invoked by targetenv_xmos.sh inside the XMOS Docker container:
#   cmake -S <this dir> -B <build dir>
#         -DXMOS_CMAKE_PATH=<path to xcommon_cmake/>
#         -DXMOS_BOARD_TARGET=XCORE-AI-EXPLORER
#         -DEHS_LIB=<absolute path to ehs_$(TARGET).a>
#
# Required cmake variables (all passed by targetenv_xmos.sh):
#   XMOS_CMAKE_PATH    — path to xcommon_cmake/ clone
#   XMOS_BOARD_TARGET  — board name, e.g. XCORE-AI-EXPLORER
#   EHS_LIB            — absolute path to ehs_$(TARGET).a from Phase 1
# ---------------------------------------------------------------------------

# xcommon.cmake MUST be included before project() so it can set up the
# XCORE toolchain (xcore_xs.cmake sets xcc as compiler and .xe as output).
# XMOS_CMAKE_PATH is passed as a cmake variable by targetenv_xmos.sh.
include(${XMOS_CMAKE_PATH}/xcommon.cmake)

project(xcore_ehs_app)

# ---------------------------------------------------------------------------
# Hardware target — XMOS board name as recognised by xcc / XTC Tools.
# Passed via -DXMOS_BOARD_TARGET= by targetenv_xmos.sh.
# ---------------------------------------------------------------------------
set(APP_HW_TARGET ${XMOS_BOARD_TARGET})

# ---------------------------------------------------------------------------
# XMOS_SANDBOX_DIR — root directory that contains fwk_rtos/, fwk_io/,
# fwk_core/, etc.  xcommon_cmake locates dependencies relative to this.
# targetenv_xmos.sh symlinks deps into _build/dependencies/ first so
# fwk_rtos's fetch_ci_deps.cmake skips all network fetches.
# ---------------------------------------------------------------------------
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---------------------------------------------------------------------------
# Module dependencies — xcommon_cmake resolves these relative to
# XMOS_SANDBOX_DIR.  When fwk_rtos is loaded as a module (not top-level)
# its fetch_ci_deps.cmake is NOT invoked, so network access is not needed.
# ---------------------------------------------------------------------------
set(APP_DEPENDENT_MODULES fwk_rtos fwk_io fwk_core)

# ---------------------------------------------------------------------------
# Compiler flags — match Phase 1 (xcore_freertos-xcore/toolchain.mk).
# -march=xs3a  : XS3 architecture (xcore.ai)
# -Os -g       : optimise for size, include debug info
# ---------------------------------------------------------------------------
set(APP_COMPILER_FLAGS -march=xs3a -Os -g -ffunction-sections -fdata-sections)

# ---------------------------------------------------------------------------
# Register the XMOS application — creates the .xe target(s) and processes
# APP_DEPENDENT_MODULES via XMOS_REGISTER_DEPS().
# APP_BUILD_TARGETS is populated by this macro with the target name(s).
# ---------------------------------------------------------------------------
XMOS_REGISTER_APP()

# ---------------------------------------------------------------------------
# Link the Phase 1 eRT static library into every build target.
# EHS_LIB is the absolute path to ehs_$(TARGET).a passed by cmake -D.
# ---------------------------------------------------------------------------
if(NOT DEFINED EHS_LIB OR NOT EXISTS "${EHS_LIB}")
    message(FATAL_ERROR
        "EHS_LIB must point to the Phase 1 static library.\n"
        "  Got: '${EHS_LIB}'\n"
        "  Run 'make all' or 'make all_docker' first.")
endif()

add_library(ehs_lib STATIC IMPORTED)
set_target_properties(ehs_lib PROPERTIES IMPORTED_LOCATION "${EHS_LIB}")

foreach(target ${APP_BUILD_TARGETS})
    target_link_libraries(${target} PRIVATE ehs_lib)
endforeach()
