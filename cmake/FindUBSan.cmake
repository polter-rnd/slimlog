# [cmake_documentation] FindUBSan.cmake
#
# The module defines the following variables:
# @arg __UBSan_FOUND__: `TRUE` if the compiler supports undefined behavior sanitizer
# [/cmake_documentation]

set(flag_candidates
    # GNU
    "-g -fsanitize=undefined -fno-omit-frame-pointer"
    # Clang
    "-g -fsanitize=undefined -fno-omit-frame-pointer \
     -fsanitize=array-bounds -fsanitize=implicit-conversion \
     -fsanitize=nullability -fsanitize=integer"
    # MSVC uses
    "/fsanitize=undefined"
)

include(Helpers)
check_compiler_flags_list("${flag_candidates}" "UndefinedBehaviorSanitizer" "UBSan")

if(UBSan_DETECTED)
    set(UBSan_SUPPORTED "UndefinedBehaviorSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UBSan DEFAULT_MSG UBSan_SUPPORTED)
