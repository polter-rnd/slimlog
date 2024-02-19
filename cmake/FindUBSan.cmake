# [cmake_documentation] FindUBSan.cmake
#
# The module defines the following variables:
# @arg __UBSan_LIBRARY__: Path to libubsan library
# @arg __UBSan_FOUND__: `TRUE` if the libubsan library was found
# [/cmake_documentation]

find_library(
    UBSan_LIBRARY
    NAMES libubsan.so libubsan.so.1 libubsan.so.1.0.0
    PATHS /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib64 /usr/local/lib
          ${CMAKE_PREFIX_PATH}/lib
    DOC "libubsan library"
)

mark_as_advanced(UBSan_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UBSan DEFAULT_MSG UBSan_LIBRARY)
