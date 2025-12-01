#!/bin/bash
#set -e

IMAGE_NAME=inxware/esp32_ubuntu22.04-build-essential

echo "source 1"
source ./source-scripts/inx-dockersetup-source-me.sh 
check_and_run_docker $IMAGE_NAME

#project name - appended to target variant and version info
# Don't set up a project name as we want to build a generic sysroot
#INX_PROJECT_NAME=esp32-native-freertos

# todo remove this should be identified in sourced script
PROCESSORS=8
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=xtensa
OS=esp32_freertos
#version libc - this is in the linux toolchain
INX_GLIBC_VERSION=""
# Empty as we are using the toolchains
EXIT_ON_FAIL=true
# This is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/
# Leave blank for using the default host compiler
TOOLCHAIN_VERSION="xtensa-esp32-elf-4.4.1"

#Optional: prefix for the compiler of not just gcc.
TOOLCHAIN_BIN_PREFIX="xtensa-esp32-elf-"
#i686-pc-linux-gnu-

source ./source-scripts/inx-xbuilder-source-me-espidf.sh


########################################################################################################
## Components to build
##
## build_component [package_name] [version] [optional: config parameters]  [optional: target directory] [optional envirionment variables to set]
#########################################################################################################

# note to do a make clean we need to drop into each component
# todo add make clean

#set to false if you don't want to rebuild the components, but copy artefacts to the build directory
if [ 1 = 1 ]; then

#Essential ones
#Built bootloader
build_bootloader esp-idf -4.4.1 bootloader 
build_partition_table esp-idf -4.4.1 partition_table
#-----

build_component esp-idf -4.4.1 bootloader 
build_component esp-idf -4.4.1 bootloader_support
build_component esp-idf -4.4.1 esp32
#.... 

# build_component esp-idf -4.4.1 esp_common
# build_component esp-idf -4.4.1 mbedtls
# build_component esp-idf -4.4.1 openthread
# build_component esp-idf -4.4.1 lwip
# build_component esp-idf -4.4.1 esp_wifi
# build_component esp-idf -4.4.1 freertos
# build_component esp-idf -4.4.1 expat

# build_component esp-idf -4.4.1 app_trace
# build_component esp-idf -4.4.1 app_update
# build_component esp-idf -4.4.1 bt
# #build_component esp-idf -4.4.1 cbor # include path not exisit
# #build_component esp-idf -4.4.1 cmock #include path not exisit
# build_component esp-idf -4.4.1 coap
# build_component esp-idf -4.4.1 console
# build_component esp-idf -4.4.1 cxx # include path not exisit
# build_component esp-idf -4.4.1 driver
# build_component esp-idf -4.4.1 efuse
# build_component esp-idf -4.4.1 esp_adc_cal
# build_component esp-idf -4.4.1 esp_eth
# build_component esp-idf -4.4.1 esp_event
# build_component esp-idf -4.4.1 esp_gdbstub
# build_component esp-idf -4.4.1 esp_hid
# build_component esp-idf -4.4.1 esp_http_client
# build_component esp-idf -4.4.1 esp_http_server
# build_component esp-idf -4.4.1 esp_https_ota
# #build_component esp-idf -4.4.1 esp_https_server # include path not exisit
# build_component esp-idf -4.4.1 esp_hw_support
# build_component esp-idf -4.4.1 esp_ipc
# build_component esp-idf -4.4.1 esp_lcd
# build_component esp-idf -4.4.1 esp_local_ctrl
# build_component esp-idf -4.4.1 esp_netif
# build_component esp-idf -4.4.1 esp_phy
# build_component esp-idf -4.4.1 esp_pm
# build_component esp-idf -4.4.1 esp_ringbuf
# build_component esp-idf -4.4.1 esp_rom
# build_component esp-idf -4.4.1 esp_serial_slave_link
# build_component esp-idf -4.4.1 esp_system
# build_component esp-idf -4.4.1 esp_timer
# build_component esp-idf -4.4.1 esp-tls
# build_component esp-idf -4.4.1 esp_wifi
# build_component esp-idf -4.4.1 fatfs
# build_component esp-idf -4.4.1 freemodbus
# build_component esp-idf -4.4.1 freertos
# build_component esp-idf -4.4.1 hal
# build_component esp-idf -4.4.1 heap
# build_component esp-idf -4.4.1 idf_test
# #build_component esp-idf -4.4.1 ieee802154  #make: *** No rule to make target 'component-ieee802154-build'.  Stop.
# build_component esp-idf -4.4.1 jsmn
# build_component esp-idf -4.4.1 json
# build_component esp-idf -4.4.1 libsodium
# #build_component esp-idf -4.4.1 linux #make: *** No rule to make target 'component-linux-build'.  Stop.
# build_component esp-idf -4.4.1 log
# build_component esp-idf -4.4.1 mdns
# build_component esp-idf -4.4.1 mqtt
# build_component esp-idf -4.4.1 newlib
# build_component esp-idf -4.4.1 nghttp
# build_component esp-idf -4.4.1 nvs_flash
# build_component esp-idf -4.4.1 openssl
# build_component esp-idf -4.4.1 openthread
# #build_component esp-idf -4.4.1 partition_table # include path not exisit
# build_component esp-idf -4.4.1 perfmon
# build_component esp-idf -4.4.1 protobuf-c
# build_component esp-idf -4.4.1 protocomm
# build_component esp-idf -4.4.1 pthread
# #build_component esp-idf -4.4.1 riscv #make: *** No rule to make target 'component-riscv-build'.  Stop.
# build_component esp-idf -4.4.1 sdmmc
# build_component esp-idf -4.4.1 soc
# build_component esp-idf -4.4.1 spiffs
# build_component esp-idf -4.4.1 spi_flash
# build_component esp-idf -4.4.1 tcpip_adapter
# build_component esp-idf -4.4.1 tcp_transport
# #build_component esp-idf -4.4.1 tinyusb #make: *** No rule to make target 'component-tinyusb-build'.  Stop.
# #build_component esp-idf -4.4.1 touch_element #make: *** No rule to make target 'component-touch_element-build'.  Stop.
# build_component esp-idf -4.4.1 ulp
# build_component esp-idf -4.4.1 unity
# #build_component esp-idf -4.4.1 usb #make: *** No rule to make target 'component-usb-build'.  Stop.
# build_component esp-idf -4.4.1 vfs
# build_component esp-idf -4.4.1 wear_levelling
# build_component esp-idf -4.4.1 wifi_provisioning
# build_component esp-idf -4.4.1 wpa_supplicant
# build_component esp-idf -4.4.1 xtensa
# build_component esp-idf -4.4.1 esp_littlefs

# build_prepare_kconfig_files esp-idf -4.4.1 prepare_kconfig_files
fi
###################################################################################################################################################

# Add miscellaneous non-standard headers
echo "Copying stray files to ${USRLIB_INCLUDE_PATH}"
#Headers in non-standard source locations that need copying
cp  ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/expat/expat/expat/lib/expat.h  ${USRLIB_INCLUDE_PATH}/
cp  ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/expat/expat/expat/lib/expat_external.h  ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/lwip/lwip/src/include/*  ${USRLIB_INCLUDE_PATH}/
cp ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/lwip/port/esp32/include/lwipopts.h ${USRLIB_INCLUDE_PATH}/lwip/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/lwip/port/esp32/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/newlib/platform_include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/freertos/port/xtensa/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/build/include/* ${USRLIB_INCLUDE_PATH}/
#copy esp32 specific headers
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/soc/esp32/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/xtensa/esp32/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/hal/esp32/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/fatfs/vfs/*.h ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/fatfs/src/*.h ${USRLIB_INCLUDE_PATH}/
# Copying stray mbedtls headers
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/mbedtls/mbedtls/include/* ${USRLIB_INCLUDE_PATH}/
mkdir -p ${USRLIB_INCLUDE_PATH}/port/include
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/mbedtls/port/include/* ${USRLIB_INCLUDE_PATH}/port/include/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/mbedtls/port/include/mbedtls/esp_config.h ${USRLIB_INCLUDE_PATH}/esp_config.h
#mqtt


cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/mqtt/esp-mqtt/include/* ${USRLIB_INCLUDE_PATH}/
#esp-tls
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/esp-tls/* ${USRLIB_INCLUDE_PATH}/

#copy the libraries & linker files
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/esp_rom/esp32/ld/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/esp_wifi/lib/esp32/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/esp_phy/lib/esp32/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/xtensa/esp32/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/build/esp_system/ld/* ${USRLIB_LIBRARY_PATH}/
cp -Rf  ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/components/soc/esp32/ld/* ${USRLIB_LIBRARY_PATH}/
#cp  ${TEMP_PWD}/../contrib/esp-idf/esp-idf-4.4.1/build/.../.../lib/ ${USRLIB_LIBRARY_PATH}/

echo "All done!"
