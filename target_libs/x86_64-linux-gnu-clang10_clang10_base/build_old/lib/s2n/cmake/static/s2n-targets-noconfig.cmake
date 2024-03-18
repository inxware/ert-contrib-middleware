#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "AWS::s2n" for configuration ""
set_property(TARGET AWS::s2n APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(AWS::s2n PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "ASM;C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libs2n.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS AWS::s2n )
list(APPEND _IMPORT_CHECK_FILES_FOR_AWS::s2n "${_IMPORT_PREFIX}/lib/libs2n.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
