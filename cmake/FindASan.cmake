# [cmake_documentation] FindASan.cmake
#
# The module defines the following variables:
# @arg __ASan_FOUND__: `TRUE` if the compiler supports address sanitizer
# [/cmake_documentation]

set(flag_candidates
    # GNU/Clang
    "-fsanitize=address -fno-omit-frame-pointer \
     -fno-optimize-sibling-calls -fsanitize-address-use-after-scope"
    # MSVC uses
    "/fsanitize=address"
)

include(Helpers)
check_compiler_flags_list("${flag_candidates}" "AddressSanitizer" "ASan")

if(ASan_COMPILERS)
    set(ASan_SUPPORTED "AddressSanitizer is supported (${ASan_COMPILERS})")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASan DEFAULT_MSG ASan_SUPPORTED)
