# libxml2-config.cmake
# --------------------
#
# Libxml2 cmake module.
# This module sets the following variables:
#
# ::
#
#   LIBXML2_INCLUDE_DIR        - Directory where LibXml2 headers are located.
#   LIBXML2_INCLUDE_DIRS       - list of the include directories needed to use LibXml2.
#   LIBXML2_LIBRARY            - path to the LibXml2 library.
#   LIBXML2_LIBRARIES          - xml2 libraries to link against.
#   LIBXML2_DEFINITIONS        - the compiler switches required for using LibXml2.
#   LIBXML2_VERSION_MAJOR      - The major version of libxml2.
#   LIBXML2_VERSION_MINOR      - The minor version of libxml2.
#   LIBXML2_VERSION_PATCH      - The patch version of libxml2.
#   LIBXML2_VERSION_STRING     - version number as a string (ex: "2.3.4")
#   LIBXML2_MODULES            - whether libxml2 has dso support
#   LIBXML2_XMLLINT_EXECUTABLE - path to the XML checking tool xmllint coming with LibXml2
#
# The following targets are defined:
#
#   LibXml2::LibXml2          - the LibXml2 library
#   LibXml2::xmllint          - the xmllint command-line executable

get_filename_component(_libxml2_rootdir ${CMAKE_CURRENT_LIST_DIR}/../../../ ABSOLUTE)

set(LIBXML2_VERSION_MAJOR  2)
set(LIBXML2_VERSION_MINOR  11)
set(LIBXML2_VERSION_MICRO  5)
set(LIBXML2_VERSION_STRING "2.11.5")
set(LIBXML2_DEFINITIONS    "")
set(LIBXML2_INSTALL_PREFIX ${_libxml2_rootdir})
set(LIBXML2_INCLUDE_DIR    ${_libxml2_rootdir}/include/libxml2)
set(LIBXML2_LIBRARY_DIR    ${_libxml2_rootdir}/lib)

find_library(LIBXML2_LIBRARY NAMES xml2 HINTS ${LIBXML2_LIBRARY_DIR} NO_DEFAULT_PATH)
find_program(LIBXML2_XMLCATALOG_EXECUTABLE NAMES xmlcatalog HINTS ${_libxml2_rootdir}/bin NO_DEFAULT_PATH)
find_program(LIBXML2_XMLLINT_EXECUTABLE NAMES xmllint HINTS ${_libxml2_rootdir}/bin NO_DEFAULT_PATH)

set(LIBXML2_LIBRARIES ${LIBXML2_LIBRARY})
set(LIBXML2_INCLUDE_DIRS ${LIBXML2_INCLUDE_DIR})
unset(LIBXML2_INTERFACE_LINK_LIBRARIES)

include(CMakeFindDependencyMacro)

set(LIBXML2_WITH_ICONV 1)
set(LIBXML2_WITH_THREADS 1)
set(LIBXML2_WITH_ICU 0)
set(LIBXML2_WITH_LZMA 1)
set(LIBXML2_WITH_ZLIB 1)

if(LIBXML2_WITH_ICONV)
  find_dependency(Iconv)
  list(APPEND LIBXML2_LIBRARIES    ${Iconv_LIBRARIES})
  list(APPEND LIBXML2_INCLUDE_DIRS ${Iconv_INCLUDE_DIRS})
  list(APPEND LIBXML2_INTERFACE_LINK_LIBRARIES "Iconv::Iconv")
endif()

if(LIBXML2_WITH_THREADS)
  find_dependency(Threads)
  list(APPEND LIBXML2_LIBRARIES    ${CMAKE_THREAD_LIBS_INIT})
  list(APPEND LIBXML2_INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:Threads::Threads>")
endif()

if(LIBXML2_WITH_ICU)
  find_dependency(ICU COMPONENTS data i18n uc)
  list(APPEND LIBXML2_LIBRARIES    ${ICU_LIBRARIES})
  list(APPEND LIBXML2_INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:ICU::data>;\$<LINK_ONLY:ICU::i18n>;\$<LINK_ONLY:ICU::uc>")
endif()

if(LIBXML2_WITH_LZMA)
  find_dependency(LibLZMA)
  list(APPEND LIBXML2_LIBRARIES    ${LIBLZMA_LIBRARIES})
  list(APPEND LIBXML2_INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:LibLZMA::LibLZMA>")
endif()

if(LIBXML2_WITH_ZLIB)
  find_dependency(ZLIB)
  list(APPEND LIBXML2_LIBRARIES    ${ZLIB_LIBRARIES})
  list(APPEND LIBXML2_INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:ZLIB::ZLIB>")
endif()

if(UNIX)
  list(APPEND LIBXML2_LIBRARIES    m)
  list(APPEND LIBXML2_INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:m>")
endif()

if(WIN32)
  list(APPEND LIBXML2_LIBRARIES    ws2_32)
  list(APPEND LIBXML2_INTERFACE_LINK_LIBRARIES "\$<LINK_ONLY:ws2_32>")
endif()

# whether libxml2 has dso support
set(LIBXML2_MODULES 1)

mark_as_advanced(LIBXML2_LIBRARY LIBXML2_XMLCATALOG_EXECUTABLE LIBXML2_XMLLINT_EXECUTABLE)

if(NOT TARGET LibXml2::LibXml2 AND DEFINED LIBXML2_LIBRARY AND DEFINED LIBXML2_INCLUDE_DIRS)
  add_library(LibXml2::LibXml2 UNKNOWN IMPORTED)
  set_target_properties(LibXml2::LibXml2 PROPERTIES IMPORTED_LOCATION "${LIBXML2_LIBRARY}")
  set_target_properties(LibXml2::LibXml2 PROPERTIES INTERFACE_COMPILE_OPTIONS "${LIBXML2_DEFINITIONS}")
  set_target_properties(LibXml2::LibXml2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBXML2_INCLUDE_DIRS}")
  set_target_properties(LibXml2::LibXml2 PROPERTIES INTERFACE_LINK_LIBRARIES "${LIBXML2_INTERFACE_LINK_LIBRARIES}")
endif()

if(NOT TARGET LibXml2::xmlcatalog AND DEFINED LIBXML2_XMLCATALOG_EXECUTABLE)
  add_executable(LibXml2::xmlcatalog IMPORTED)
  set_target_properties(LibXml2::xmlcatalog PROPERTIES IMPORTED_LOCATION "${LIBXML2_XMLCATALOG_EXECUTABLE}")
endif()

if(NOT TARGET LibXml2::xmllint AND DEFINED LIBXML2_XMLLINT_EXECUTABLE)
  add_executable(LibXml2::xmllint IMPORTED)
  set_target_properties(LibXml2::xmllint PROPERTIES IMPORTED_LOCATION "${LIBXML2_XMLLINT_EXECUTABLE}")
endif()
