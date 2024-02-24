# [cmake_documentation] FindUBSan.cmake
#
# The module defines the following variables:
# @arg __UBSan_FOUND__: `TRUE` if the libubsan library was found
# [/cmake_documentation]

set(FLAG_CANDIDATES
    # MSVC uses
    "/fsanitize=undefined"
    # GNU/Clang
    "-g -fsanitize=undefined"
)

include(SanitizeHelpers)
sanitizer_check_compiler_flags("${FLAG_CANDIDATES}" "UndefinedBehaviorSanitizer" "UBSan")

if (UBSan_FLAG_DETECTED)
    set(UBSan_SUPPORTED "UndefinedBehaviorSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UBSan DEFAULT_MSG UBSan_SUPPORTED)
