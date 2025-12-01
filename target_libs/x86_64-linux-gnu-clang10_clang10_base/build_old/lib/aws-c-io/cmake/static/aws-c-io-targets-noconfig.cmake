#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "AWS::aws-c-io" for configuration ""
set_property(TARGET AWS::aws-c-io APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(AWS::aws-c-io PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libaws-c-io.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS AWS::aws-c-io )
list(APPEND _IMPORT_CHECK_FILES_FOR_AWS::aws-c-io "${_IMPORT_PREFIX}/lib/libaws-c-io.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
