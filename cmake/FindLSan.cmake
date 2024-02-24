# [cmake_documentation] FindLSan.cmake
#
# The module defines the following variables:
# @arg __LSan_LIBRARY__: Path to liblsan library
# @arg __LSan_FOUND__: `TRUE` if the liblsan library was found
# [/cmake_documentation]

set(FLAG_CANDIDATES
    # MSVC uses
    "/fsanitize=leak"
    # GNU/Clang
    "-g -fsanitize=leak"
)

include(SanitizeHelpers)
sanitizer_check_compiler_flags("${FLAG_CANDIDATES}" "LeakSanitizer" "LSan")

if (LSan_FLAG_DETECTED)
    set(LSan_SUPPORTED "LeakSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LSan DEFAULT_MSG LSan_SUPPORTED)
