# [cmake_documentation] FindMSan.cmake
#
# The module defines the following variables:
# @arg __MSan_FOUND__: `TRUE` if the compiler supports memory sanitizer
# [/cmake_documentation]

set(flag_candidates
    # GNU/Clang
    "-fsanitize=memory"
    # MSVC uses
    "/fsanitize=memory"
)

if(NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    message(WARNING "MemorySanitizer disabled for target ${TARGET} because "
                    "MemorySanitizer is supported for 64bit systems only."
    )
else()
    include(Helpers)
    check_compiler_flags_list("${flag_candidates}" "MemorySanitizer" "MSan")
endif()

if(MSan_COMPILERS)
    set(MSan_SUPPORTED "MemorySanitizer is supported (${MSan_COMPILERS})")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MSan DEFAULT_MSG MSan_SUPPORTED)
