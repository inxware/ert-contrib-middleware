# CMake toolchain file for XMOS xcore.ai targets using XTC Tools (xcc)
#
# Used by build-xmos-libs.sh when invoking CMake to build XMOS SDK libraries.
# XMOS_TOOL_PATH and XMOS_BOARD_TARGET must be set in the environment.

set(CMAKE_SYSTEM_NAME XCORE_XS3A)
set(CMAKE_SYSTEM_PROCESSOR xcore)

# Locate xcc from XMOS_TOOL_PATH or PATH
if(DEFINED ENV{XMOS_TOOL_PATH})
    set(XTC_BIN "$ENV{XMOS_TOOL_PATH}/bin")
else()
    set(XTC_BIN "")
endif()

find_program(CMAKE_C_COMPILER   xcc HINTS "${XTC_BIN}" REQUIRED)
find_program(CMAKE_CXX_COMPILER xcc HINTS "${XTC_BIN}")
find_program(CMAKE_ASM_COMPILER xcc HINTS "${XTC_BIN}")
set(CMAKE_LINKER ${CMAKE_C_COMPILER})

# Compilation flags — -march=xs3a selects the XS3 architecture for .c/.xc compilation.
# The board XN file is only required at final .xe link time and is passed by the ERT
# build system (toolchain.mk) as a positional linker argument; it is not needed here
# because we are only building static libraries, not firmware images.
set(CMAKE_C_FLAGS_INIT   "-march=xs3a -Os -g -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}")
set(CMAKE_ASM_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")

# Skip compiler test (cross-compiling)
set(CMAKE_C_COMPILER_WORKS   1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Output suffix
set(CMAKE_EXECUTABLE_SUFFIX ".xe")
