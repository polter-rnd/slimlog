# [cmake_documentation] FindASan.cmake
#
# The module defines the following variables:
# @arg __ASan_FOUND__: `TRUE` if the libasan library was found
# [/cmake_documentation]

set(FLAG_CANDIDATES
    # MSVC uses
    "/fsanitize=address"

    # Clang 3.2+ use this version. The no-omit-frame-pointer option is optional.
    "-g -fsanitize=address -fno-omit-frame-pointer"
    "-g -fsanitize=address"

    # Older deprecated flag for ASan
    "-g -faddress-sanitizer"
)

include(SanitizeHelpers)
sanitizer_check_compiler_flags("${FLAG_CANDIDATES}" "AddressSanitizer" "ASan")

if (ASan_FLAG_DETECTED)
    set(ASan_SUPPORTED "AddressSanitizer is supported by compiler")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASan DEFAULT_MSG ASan_SUPPORTED)
