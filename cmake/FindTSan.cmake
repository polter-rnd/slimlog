# [cmake_documentation] FindTSan.cmake
#
# The module defines the following variables:
# @arg __TSan_LIBRARY__: Path to libtsan library
# @arg __TSan_FOUND__: `TRUE` if the libtsan library was found
# [/cmake_documentation]

find_library(
    TSan_LIBRARY
    NAMES libtsan.so
          libtsan.so.2
          libtsan.so.2.0.0
          libtsan.so.1
          libtsan.so.1.0.0
          libtsan.so.0
          libtsan.so.0.0.0
    PATHS /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib64 /usr/local/lib
          ${CMAKE_PREFIX_PATH}/lib
    DOC "libtsan library"
)

mark_as_advanced(TSan_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TSan DEFAULT_MSG TSan_LIBRARY)
