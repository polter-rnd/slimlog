# [cmake_documentation] FindLSan.cmake
#
# The module defines the following variables:
# @arg __LSan_FOUND__: `TRUE` if the compiler supports leak sanitizer
# [/cmake_documentation]

set(FLAG_CANDIDATES
    # GNU/Clang
    "-g -fsanitize=leak"
    # MSVC uses
    "/fsanitize=leak"
)

include(Helpers)
check_compiler_flags_list("${FLAG_CANDIDATES}" "LeakSanitizer" "LSan")

if(LSan_FLAG_DETECTED)
    set(LSan_SUPPORTED "LeakSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LSan DEFAULT_MSG LSan_SUPPORTED)
