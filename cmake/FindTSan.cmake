# [cmake_documentation] FindTSan.cmake
#
# The module defines the following variables:
# @arg __TSan_FOUND__: `TRUE` if the compiller supports thread sanitizer
# [/cmake_documentation]

set(flag_candidates
    # GNU/Clang
    "-fsanitize=thread"
    # MSVC uses
    "/fsanitize=thread"
)

if(NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    message(WARNING "ThreadSanitizer disabled for target ${TARGET} because "
                    "ThreadSanitizer is supported for 64bit systems only."
    )
else()
    include(Helpers)
    check_compiler_flags_list("${flag_candidates}" "ThreadSanitizer" "TSan")
endif()

if(TSan_COMPILERS)
    set(TSan_SUPPORTED "ThreadSanitizer is supported (${TSan_COMPILERS})")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TSan DEFAULT_MSG TSan_SUPPORTED)
