# [cmake_documentation] FindIWYU.cmake
#
# The module defines the following variables:
# @arg __IWYU_EXECUTABLE__: Path to include-what-you-use executable
# @arg __IWYU_FOUND__: `TRUE` if the include-what-you-use executable was found
# @arg __IWYU_VERSION_STRING__: The version of include-what-you-use found
# @arg __IWYU_VERSION_MAJOR__: The include-what-you-use major version
# @arg __IWYU_VERSION_MINOR__: The include-what-you-use minor version
# @arg __IWYU_VERSION_PATCH__: The include-what-you-use patch version
# [/cmake_documentation]

find_program(
    IWYU_EXECUTABLE
    NAMES include-what-you-use
    DOC "include-what-you-use executable"
)

mark_as_advanced(IWYU_EXECUTABLE)

# Get the version string
if(IWYU_EXECUTABLE)
    execute_process(
        COMMAND ${IWYU_EXECUTABLE} --version
        OUTPUT_VARIABLE _IWYU_VERSION
        ERROR_VARIABLE _IWYU_VERSION
    )
    if(_IWYU_VERSION MATCHES "include-what-you-use ([0-9]+\\.[0-9]+)")
        set(IWYU_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" IWYU_VERSION_MAJOR ${IWYU_VERSION_STRING})
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" IWYU_VERSION_MINOR ${IWYU_VERSION_STRING})
        set(IWYU_VERSION_PATCH 0)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    IWYU
    REQUIRED_VARS IWYU_EXECUTABLE
    VERSION_VAR IWYU_VERSION_STRING
)
