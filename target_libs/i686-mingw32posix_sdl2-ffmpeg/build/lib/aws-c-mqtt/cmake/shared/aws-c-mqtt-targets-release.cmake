#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "AWS::aws-c-mqtt" for configuration "Release"
set_property(TARGET AWS::aws-c-mqtt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(AWS::aws-c-mqtt PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libaws-c-mqtt.dll.a"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libaws-c-mqtt.dll"
  )

list(APPEND _cmake_import_check_targets AWS::aws-c-mqtt )
list(APPEND _cmake_import_check_files_for_AWS::aws-c-mqtt "${_IMPORT_PREFIX}/lib/libaws-c-mqtt.dll.a" "${_IMPORT_PREFIX}/bin/libaws-c-mqtt.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
