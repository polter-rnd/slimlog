# [cmake_documentation] FindTSan.cmake
#
# The module defines the following variables:
# @arg __TSan_FOUND__: `TRUE` if the libtsan library was found
# [/cmake_documentation]

set(FLAG_CANDIDATES
    # MSVC uses
    "/fsanitize=thread"
    # GNU/Clang
    "-g -fsanitize=thread"
)

include(SanitizeHelpers)

  if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND
      NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        message(WARNING "ThreadSanitizer disabled for target ${TARGET} because "
          "ThreadSanitizer is supported for Linux systems and macOS only.")
    elseif (NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        message(WARNING "ThreadSanitizer disabled for target ${TARGET} because "
            "ThreadSanitizer is supported for 64bit systems only.")
    else ()
        sanitizer_check_compiler_flags("${FLAG_CANDIDATES}" "ThreadSanitizer"
            "TSan")
    endif ()

if (TSan_FLAG_DETECTED)
    set(TSan_SUPPORTED "ThreadSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TSan DEFAULT_MSG TSan_SUPPORTED)
