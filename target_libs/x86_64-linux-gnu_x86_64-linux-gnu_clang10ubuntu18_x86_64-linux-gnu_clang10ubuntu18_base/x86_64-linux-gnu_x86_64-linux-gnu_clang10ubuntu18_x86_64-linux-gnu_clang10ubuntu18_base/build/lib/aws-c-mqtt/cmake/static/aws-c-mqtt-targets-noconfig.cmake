#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "AWS::aws-c-mqtt" for configuration ""
set_property(TARGET AWS::aws-c-mqtt APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(AWS::aws-c-mqtt PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libaws-c-mqtt.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS AWS::aws-c-mqtt )
list(APPEND _IMPORT_CHECK_FILES_FOR_AWS::aws-c-mqtt "${_IMPORT_PREFIX}/lib/libaws-c-mqtt.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
