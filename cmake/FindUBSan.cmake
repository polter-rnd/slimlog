# [cmake_documentation] FindUBSan.cmake
#
# The module defines the following variables:
# @arg __UBSan_FOUND__: `TRUE` if the compiler supports undefined behavior sanitizer
# [/cmake_documentation]

set(FLAG_CANDIDATES
    # Clang
    "-g -fsanitize=undefined -fno-omit-frame-pointer \
     -fsanitize=array-bounds -fsanitize=implicit-conversion \
     -fsanitize=nullability -fsanitize=integer"
    # GNU
    "-g -fsanitize=undefined -fno-omit-frame-pointer"
    # MSVC uses
    "/fsanitize=undefined"
)

include(Helpers)
check_compiler_flags_list("${FLAG_CANDIDATES}" "UndefinedBehaviorSanitizer" "UBSan")

if(UBSan_FLAG_DETECTED)
    set(UBSan_SUPPORTED "UndefinedBehaviorSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UBSan DEFAULT_MSG UBSan_SUPPORTED)
