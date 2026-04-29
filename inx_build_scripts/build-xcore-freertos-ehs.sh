#!/bin/bash
#---------------------------------------------------------------
# Copyright (C) 2025 inx limited, UK - All Rights Reserved
# LGPLv3 license — see https://www.gnu.org/licenses/lgpl-3.0.txt
#---------------------------------------------------------------
#
# build-xcore-freertos-ehs.sh
#
# Clones the XMOS xcore.ai FreeRTOS SDK source repos into
# contrib/xmos-sdk/ so that make all_docker (Phase 1) can find
# FreeRTOS headers via the source tree directly (Option B).
#
# Run from inx_build_scripts/ on the host before running
# make all_docker for the xcore_freertos-xcore-base platform.
#
# No Docker image or XTC Tools are required for this step.
# The repos are referenced read-only at compile time; no SDK
# libraries are built here.  Phase 2 (make targetenv_xmos_docker)
# handles the final link via xcommon_cmake inside the XMOS
# Docker container.
#
# fwk_rtos submodules use SSH URLs (git@github.com:...) and
# cannot be fetched via --recurse-submodules without GitHub SSH
# keys.  The two submodules needed for Phase 1 header access are
# cloned separately via HTTPS into the paths git would have used.

XMOS_SDK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../contrib/xmos-sdk" && pwd)"

# ---- Main SDK repos -------------------------------------------------------

for REPO in fwk_rtos fwk_io fwk_core xcommon_cmake lib_xcore_math lib_i2c lib_uart lib_i2s lib_qspi_fast_read; do
    if [ ! -d "${XMOS_SDK_DIR}/${REPO}" ] || [ -z "$(ls -A "${XMOS_SDK_DIR}/${REPO}")" ]; then
        echo "Cloning ${REPO} into contrib/xmos-sdk/ ..."
        rm -rf "${XMOS_SDK_DIR}/${REPO}"
        git clone --recurse-submodules "https://github.com/xmos/${REPO}.git" "${XMOS_SDK_DIR}/${REPO}"
    fi
done

# ---- fwk_rtos SSH submodules (cloned via HTTPS) ---------------------------

FREERTOS_KERNEL_DIR="${XMOS_SDK_DIR}/fwk_rtos/modules/FreeRTOS/FreeRTOS-SMP-Kernel"
FREERTOS_TCP_DIR="${XMOS_SDK_DIR}/fwk_rtos/modules/FreeRTOS/FreeRTOS-Plus-TCP"

if [ ! -d "${FREERTOS_KERNEL_DIR}" ] || [ -z "$(ls -A "${FREERTOS_KERNEL_DIR}")" ]; then
    echo "Cloning FreeRTOS-SMP-Kernel into fwk_rtos submodule path ..."
    rm -rf "${FREERTOS_KERNEL_DIR}"
    git clone --branch smp "https://github.com/FreeRTOS/FreeRTOS-Kernel.git" "${FREERTOS_KERNEL_DIR}"
fi

if [ ! -d "${FREERTOS_TCP_DIR}" ] || [ -z "$(ls -A "${FREERTOS_TCP_DIR}")" ]; then
    echo "Cloning FreeRTOS-Plus-TCP into fwk_rtos submodule path ..."
    rm -rf "${FREERTOS_TCP_DIR}"
    git clone "https://github.com/xmos/FreeRTOS-Plus-TCP.git" "${FREERTOS_TCP_DIR}"
fi

echo "contrib/xmos-sdk/ is ready for make all_docker."
