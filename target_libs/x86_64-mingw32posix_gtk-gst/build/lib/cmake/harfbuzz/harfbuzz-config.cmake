
get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()


set_and_check(HARFBUZZ_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/include/harfbuzz")

set(HARFBUZZ_VERSION "14.3.1")

function(_harfbuzz_set_imported_library target library_name)
  set_target_properties("${target}" PROPERTIES
    IMPORTED_LOCATION "${PACKAGE_PREFIX_DIR}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}${library_name}-0${CMAKE_SHARED_LIBRARY_SUFFIX}")
  if (YES)
    set_target_properties("${target}" PROPERTIES
      IMPORTED_IMPLIB "${PACKAGE_PREFIX_DIR}/lib/${CMAKE_IMPORT_LIBRARY_PREFIX}${library_name}${CMAKE_IMPORT_LIBRARY_SUFFIX}")
  endif ()
endfunction()

# Add the libraries.
add_library(harfbuzz::harfbuzz UNKNOWN IMPORTED)
set_target_properties(harfbuzz::harfbuzz PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include/harfbuzz")
_harfbuzz_set_imported_library(harfbuzz::harfbuzz harfbuzz)

add_library(harfbuzz::icu UNKNOWN IMPORTED)
set_target_properties(harfbuzz::icu PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include/harfbuzz"
  INTERFACE_LINK_LIBRARIES "harfbuzz::harfbuzz")
_harfbuzz_set_imported_library(harfbuzz::icu harfbuzz-icu)

add_library(harfbuzz::subset UNKNOWN IMPORTED)
set_target_properties(harfbuzz::subset PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include/harfbuzz"
  INTERFACE_LINK_LIBRARIES "harfbuzz::harfbuzz")
_harfbuzz_set_imported_library(harfbuzz::subset harfbuzz-subset)

# Only add the gobject library if it was built.
if (YES)
  add_library(harfbuzz::gobject UNKNOWN IMPORTED)
  set_target_properties(harfbuzz::gobject PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include/harfbuzz"
    INTERFACE_LINK_LIBRARIES "harfbuzz::harfbuzz")
  _harfbuzz_set_imported_library(harfbuzz::gobject harfbuzz-gobject)
endif ()

check_required_components(harfbuzz)
