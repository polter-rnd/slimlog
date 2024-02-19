# [cmake_documentation] FindLSan.cmake
#
# The module defines the following variables:
# @arg __LSan_LIBRARY__: Path to liblsan library
# @arg __LSan_FOUND__: `TRUE` if the liblsan library was found
# [/cmake_documentation]

find_library(
    LSan_LIBRARY
    NAMES liblsan.so liblsan.so.0 liblsan.so.0.0.0
    PATHS /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib64 /usr/local/lib
          ${CMAKE_PREFIX_PATH}/lib
    DOC "liblsan library"
)

mark_as_advanced(LSan_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LSan DEFAULT_MSG LSan_LIBRARY)
