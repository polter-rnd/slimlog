# [cmake_documentation] FindMSan.cmake
#
# The module defines the following variables:
# @arg __MSan_LIBRARY__: Path to liblsan library
# @arg __MSan_FOUND__: `TRUE` if the liblsan library was found
# [/cmake_documentation]

set(FLAG_CANDIDATES
    # MSVC uses
    "/fsanitize=memory"
    # GNU/Clang
    "-g -fsanitize=memory"
)

include(SanitizeHelpers)

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
message(WARNING "MemorySanitizer disabled for target ${TARGET} because "
    "MemorySanitizer is supported for Linux systems only.")
elseif (NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
message(WARNING "MemorySanitizer disabled for target ${TARGET} because "
    "MemorySanitizer is supported for 64bit systems only.")
else ()
sanitizer_check_compiler_flags("${FLAG_CANDIDATES}" "MemorySanitizer"
    "MSan")
endif ()

include(SanitizeHelpers)
sanitizer_check_compiler_flags("${FLAG_CANDIDATES}" "MemorySanitizer" "MSan")

if (MSan_FLAG_DETECTED)
    set(MSan_SUPPORTED "MemorySanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MSan DEFAULT_MSG MSan_SUPPORTED)
