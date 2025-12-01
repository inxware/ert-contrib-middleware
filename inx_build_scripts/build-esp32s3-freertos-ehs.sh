#!/bin/bash
#set -e

IMAGE_NAME=inxware/esp32s3_ubuntu22.04-build-essential

echo "source 1"
#left to do
source ./source-scripts/inx-dockersetup-source-me.sh 
check_and_run_docker $IMAGE_NAME

#project name - appended to target variant and version info
# Don't set up a project name as we want to build a generic sysroot
#INX_PROJECT_NAME=esp32-native-freertos

# todo remove this should be identified in sourced script
PROCESSORS=8
#gnu standard form of target architecture - prefix for existing libc version and created target directory
ARCH=xtensa
OS=esp32s3_freertos
#version libc - this is in the linux toolchain
INX_GLIBC_VERSION=""
# Empty as we are using the toolchains
EXIT_ON_FAIL=true
# This is the variant & version of the compiler as defined by ls  ../inx-core-uspace/toolchains/
# Leave blank for using the default host compiler
IDF_VERSION="-4.4.4"
TOOLCHAIN_VERSION="xtensa-esp32s3-elf${IDF_VERSION}"

#Optional: prefix for the compiler of not just gcc.
TOOLCHAIN_BIN_PREFIX="xtensa-esp32s3-elf-"
#i686-pc-linux-gnu-

source ./source-scripts/inx-xbuilder-source-me-espidf-s3.sh


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
build_bootloader esp-idf ${IDF_VERSION} bootloader 
build_partition_table esp-idf ${IDF_VERSION} partition_table
#-----

build_component esp-idf ${IDF_VERSION} bootloader 
build_component esp-idf ${IDF_VERSION} bootloader_support
build_component esp-idf ${IDF_VERSION} esp32s3
#.... 

 build_component esp-idf ${IDF_VERSION} esp_common
 build_component esp-idf ${IDF_VERSION} mbedtls
 build_component esp-idf ${IDF_VERSION} openthread
 build_component esp-idf ${IDF_VERSION} lwip
 build_component esp-idf ${IDF_VERSION} esp_wifi
 build_component esp-idf ${IDF_VERSION} freertos
 build_component esp-idf ${IDF_VERSION} expat

 build_component esp-idf ${IDF_VERSION} app_trace
 build_component esp-idf ${IDF_VERSION} app_update
 build_component esp-idf ${IDF_VERSION} bt
 #build_component esp-idf ${IDF_VERSION} cbor # include path not exisit
 #build_component esp-idf ${IDF_VERSION} cmock #include path not exisit
 build_component esp-idf ${IDF_VERSION} coap
 build_component esp-idf ${IDF_VERSION} console
 build_component esp-idf ${IDF_VERSION} cxx # include path not exisit
 build_component esp-idf ${IDF_VERSION} driver
 build_component esp-idf ${IDF_VERSION} efuse
 build_component esp-idf ${IDF_VERSION} esp_adc_cal
 build_component esp-idf ${IDF_VERSION} esp_eth
 build_component esp-idf ${IDF_VERSION} esp_event
 build_component esp-idf ${IDF_VERSION} esp_gdbstub
 build_component esp-idf ${IDF_VERSION} esp_hid
 build_component esp-idf ${IDF_VERSION} esp_http_client
 build_component esp-idf ${IDF_VERSION} esp_http_server
 build_component esp-idf ${IDF_VERSION} esp_https_ota
# #build_component esp-idf ${IDF_VERSION} esp_https_server # include path not exisit
 build_component esp-idf ${IDF_VERSION} esp_hw_support
 build_component esp-idf ${IDF_VERSION} esp_ipc
 build_component esp-idf ${IDF_VERSION} esp_lcd
 build_component esp-idf ${IDF_VERSION} esp_local_ctrl
 build_component esp-idf ${IDF_VERSION} esp_netif
 build_component esp-idf ${IDF_VERSION} esp_phy
 build_component esp-idf ${IDF_VERSION} esp_pm
 build_component esp-idf ${IDF_VERSION} esp_ringbuf
 build_component esp-idf ${IDF_VERSION} esp_rom
 build_component esp-idf ${IDF_VERSION} esp_serial_slave_link
 build_component esp-idf ${IDF_VERSION} esp_system
 build_component esp-idf ${IDF_VERSION} esp_timer
 build_component esp-idf ${IDF_VERSION} esp-tls
 build_component esp-idf ${IDF_VERSION} esp_wifi
 build_component esp-idf ${IDF_VERSION} fatfs
 build_component esp-idf ${IDF_VERSION} freemodbus
 build_component esp-idf ${IDF_VERSION} freertos
 build_component esp-idf ${IDF_VERSION} hal
 build_component esp-idf ${IDF_VERSION} heap
 build_component esp-idf ${IDF_VERSION} idf_test
# #build_component esp-idf ${IDF_VERSION} ieee802154  #make: *** No rule to make target 'component-ieee802154-build'.  Stop.
 build_component esp-idf ${IDF_VERSION} jsmn
 build_component esp-idf ${IDF_VERSION} json
 build_component esp-idf ${IDF_VERSION} libsodium
 #build_component esp-idf ${IDF_VERSION} linux #make: *** No rule to make target 'component-linux-build'.  Stop.
 build_component esp-idf ${IDF_VERSION} log
 build_component esp-idf ${IDF_VERSION} mdns
 build_component esp-idf ${IDF_VERSION} mqtt
 build_component esp-idf ${IDF_VERSION} newlib
 build_component esp-idf ${IDF_VERSION} nghttp
 build_component esp-idf ${IDF_VERSION} nvs_flash
 build_component esp-idf ${IDF_VERSION} openssl
 build_component esp-idf ${IDF_VERSION} openthread
# #build_component esp-idf ${IDF_VERSION} partition_table # include path not exisit
 build_component esp-idf ${IDF_VERSION} perfmon
 build_component esp-idf ${IDF_VERSION} protobuf-c
 build_component esp-idf ${IDF_VERSION} protocomm
 build_component esp-idf ${IDF_VERSION} pthread
# #build_component esp-idf ${IDF_VERSION} riscv #make: *** No rule to make target 'component-riscv-build'.  Stop.
 build_component esp-idf ${IDF_VERSION} sdmmc
 build_component esp-idf ${IDF_VERSION} soc
 build_component esp-idf ${IDF_VERSION} spiffs
 build_component esp-idf ${IDF_VERSION} spi_flash
 build_component esp-idf ${IDF_VERSION} tcpip_adapter
 build_component esp-idf ${IDF_VERSION} tcp_transport
# #build_component esp-idf ${IDF_VERSION} tinyusb #make: *** No rule to make target 'component-tinyusb-build'.  Stop.
# #build_component esp-idf ${IDF_VERSION} touch_element #make: *** No rule to make target 'component-touch_element-build'.  Stop.
 build_component esp-idf ${IDF_VERSION} ulp
 build_component esp-idf ${IDF_VERSION} unity
# #build_component esp-idf ${IDF_VERSION} usb #make: *** No rule to make target 'component-usb-build'.  Stop.
 build_component esp-idf ${IDF_VERSION} vfs
 build_component esp-idf ${IDF_VERSION} wear_levelling
 build_component esp-idf ${IDF_VERSION} wifi_provisioning
 build_component esp-idf ${IDF_VERSION} wpa_supplicant
 build_component esp-idf ${IDF_VERSION} xtensa
 build_component esp-idf ${IDF_VERSION} esp_littlefs

#  ############################################## LVGL ########################################################
#  build_component esp-idf ${IDF_VERSION} lvgl
#  build_component esp-idf ${IDF_VERSION} lvgl_esp32_drivers
#  ############################################## LVGL ########################################################

# build_prepare_kconfig_files esp-idf ${IDF_VERSION} prepare_kconfig_files
fi
###################################################################################################################################################

# Add miscellaneous non-standard headers
echo "Copying stray files to ${USRLIB_INCLUDE_PATH}"
#Headers in non-standard source locations that need copying
cp  ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/expat/expat/expat/lib/expat.h  ${USRLIB_INCLUDE_PATH}/
cp  ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/expat/expat/expat/lib/expat_external.h  ${USRLIB_INCLUDE_PATH}/
#find ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lwip/lwip/src/include/ -type f -name "*" | xargs -I {} cp -Rf {} ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lwip/lwip/src/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/wpa_supplicant/esp_supplicant/include/* ${USRLIB_INCLUDE_PATH}/
mkdir -p ${USRLIB_INCLUDE_PATH}/lwip/
cp -a ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lwip/port/esp32/include/. ${USRLIB_INCLUDE_PATH}/
#find ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lwip/port/esp32/include/ -type f -name "*" | xargs -I {} cp -Rf {} ${USRLIB_INCLUDE_PATH}/
#cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lwip/port/esp32s3/include/* ${USRLIB_INCLUDE_PATH}/
#find ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/newlib/platform_include/ -type f -name "*" | xargs -I {} cp -Rf {} ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/newlib/platform_include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/freertos/port/xtensa/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${STAGING_DIR}/esp-idf${IDF_VERSION}-${OS}/dummy_project/build/config/sdkconfig.h ${USRLIB_INCLUDE_PATH}/
#copy esp32 specific headers
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/soc/esp32s3/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/xtensa/esp32s3/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/hal/esp32s3/include/* ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/fatfs/vfs/*.h ${USRLIB_INCLUDE_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/fatfs/src/*.h ${USRLIB_INCLUDE_PATH}/
# Copying stray mbedtls headers
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/mbedtls/mbedtls/include/* ${USRLIB_INCLUDE_PATH}/
mkdir -p ${USRLIB_INCLUDE_PATH}/include_next
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/mbedtls/port/include/* ${USRLIB_INCLUDE_PATH}/include_next/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/mbedtls/port/include/mbedtls/esp_config.h ${USRLIB_INCLUDE_PATH}/esp_config.h
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/mbedtls/esp_crt_bundle/include/* ${USRLIB_INCLUDE_PATH}/
#mqtt


cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/mqtt/esp-mqtt/include/* ${USRLIB_INCLUDE_PATH}/
#esp-tls
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/esp-tls/* ${USRLIB_INCLUDE_PATH}/
cp -Rf  ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/soc/esp32s3/include/* ${USRLIB_INCLUDE_PATH}/

#copy the libraries & linker files
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/esp_rom/esp32s3/ld/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/esp_wifi/lib/esp32s3/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/esp_phy/lib/esp32s3/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/xtensa/esp32s3/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/openthread/lib/esp32s3/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/bt/controller/lib_esp32c3_family/esp32s3/* ${USRLIB_LIBRARY_PATH}/
cp -Rf ${STAGING_DIR}/esp-idf${IDF_VERSION}-${OS}/dummy_project/build/esp-idf/esp_system/ld/* ${USRLIB_LIBRARY_PATH}/
mkdir -p ${USRLIB_LIBRARY_PATH}/mbedtls/OLD/library
cp -Rf ${STAGING_DIR}/esp-idf${IDF_VERSION}-${OS}/dummy_project/build/esp-idf/mbedtls/mbedtls/library/*.a ${USRLIB_LIBRARY_PATH}/mbedtls/OLD/library
cp -Rf  ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/soc/esp32s3/ld/* ${USRLIB_LIBRARY_PATH}/

#  ############################################## LVGL ########################################################
# cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lvgl_esp32_drivers/*.h ${USRLIB_INCLUDE_PATH}/
# mkdir -p ${USRLIB_INCLUDE_PATH}/lvgl_i2c
# mkdir -p ${USRLIB_INCLUDE_PATH}/lvgl_tft
# mkdir -p ${USRLIB_INCLUDE_PATH}/lvgl_touch
# cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lvgl_esp32_drivers/lvgl_i2c/*.h ${USRLIB_INCLUDE_PATH}/lvgl_i2c
# cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lvgl_esp32_drivers/lvgl_tft/*.h ${USRLIB_INCLUDE_PATH}/lvgl_tft
# cp -Rf ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lvgl_esp32_drivers/lvgl_touch/*.h ${USRLIB_INCLUDE_PATH}/lvgl_touch
# 
# pushd ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/components/lvgl
# find . -name "*.h" | xargs cp --parents -t ${USRLIB_INCLUDE_PATH}
# popd
# mkdir -p ${USRLIB_INCLUDE_PATH}/lvgl
# cp ${USRLIB_INCLUDE_PATH}/lvgl.h ${USRLIB_INCLUDE_PATH}/lvgl/lvgl.h
#  ############################################## LVGL ########################################################

# Combine mbedtls libraries because of name clashes of these when found in the same directory.
pushd ${USRLIB_LIBRARY_PATH}/
mv libmbedtls.a mbedtls/OLD
mkdir -p mbedtls/NEW
${TOOLCHAIN_BIN_PREFIX}ar -M <<EOM
CREATE libmbedtls.a
ADDLIB mbedtls/OLD/libmbedtls.a
ADDLIB mbedtls/OLD/library/libmbedtls.a
ADDLIB mbedtls/OLD/library/libmbedcrypto.a
ADDLIB mbedtls/OLD/library/libmbedx509.a
SAVE
END
EOM
${TOOLCHAIN_BIN_PREFIX}ranlib libmbedtls.a
popd
#cp  ${TEMP_PWD}/../contrib/esp-idf/esp-idf${IDF_VERSION}/build/.../.../lib/ ${USRLIB_LIBRARY_PATH}/

echo "All done!"
