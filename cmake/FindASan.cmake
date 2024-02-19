# [cmake_documentation] FindASan.cmake
#
# The module defines the following variables:
# @arg __ASan_LIBRARY__: Path to libasan library
# @arg __ASan_FOUND__: `TRUE` if the libasan library was found
# [/cmake_documentation]

find_library(
    ASan_LIBRARY
    NAMES libasan.so
          libasan.so.8
          libasan.so.8.0.0
          libasan.so.7
          libasan.so.7.0.0
          libasan.so.6
          libasan.so.6.0.0
          libasan.so.5
          libasan.so.5.0.0
          libasan.so.4
          libasan.so.4.0.0
          libasan.so.3
          libasan.so.3.0.0
          libasan.so.2
          libasan.so.2.0.0
          libasan.so.1
          libasan.so.1.0.0
          libasan.so.0
          libasan.so.0.0.0
    PATHS /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib64 /usr/local/lib
          ${CMAKE_PREFIX_PATH}/lib
    DOC "libasan library"
)

mark_as_advanced(ASan_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASan DEFAULT_MSG ASan_LIBRARY)
