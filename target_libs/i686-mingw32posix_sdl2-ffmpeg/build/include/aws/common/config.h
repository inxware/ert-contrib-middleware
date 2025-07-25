#ifndef AWS_COMMON_CONFIG_H
#define AWS_COMMON_CONFIG_H

/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

/*
 * This header exposes compiler feature test results determined during cmake
 * configure time to inline function implementations. The macros defined here
 * should be considered to be an implementation detail, and can change at any
 * time.
 */
#define AWS_HAVE_GCC_OVERFLOW_MATH_EXTENSIONS
#define AWS_HAVE_GCC_INLINE_ASM
/* #undef AWS_HAVE_MSVC_INTRINSICS_X64 */
#define AWS_HAVE_POSIX_LARGE_FILE_SUPPORT
/* #undef AWS_HAVE_EXECINFO */
#define AWS_HAVE_WINAPI_DESKTOP
/* #undef AWS_HAVE_LINUX_IF_LINK_H */

#endif
