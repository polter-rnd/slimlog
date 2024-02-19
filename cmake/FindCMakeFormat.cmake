# [cmake_documentation] FindCMakeFormat.cmake
#
# The module defines the following variables:
# @arg __CMakeFormat_EXECUTABLE__: Path to cmake-format executable
# @arg __CMakeFormat_FOUND__: `TRUE` if the cmake-format executable was found
# @arg __CMakeFormat_VERSION_STRING__: The version of cmake-format found
# @arg __CMakeFormat_VERSION_MAJOR__: The cmake-format major version
# @arg __CMakeFormat_VERSION_MINOR__: The cmake-format minor version
# @arg __CMakeFormat_VERSION_PATCH__: The cmake-format patch version
# [/cmake_documentation]

find_program(
    CMakeFormat_EXECUTABLE
    NAMES cmake-format
    DOC "cmake-format executable"
)

mark_as_advanced(CMakeFormat_EXECUTABLE)

# Extract version from command "cmake-format --version"
if(CMakeFormat_EXECUTABLE)
    execute_process(
        COMMAND ${CMakeFormat_EXECUTABLE} --version
        OUTPUT_VARIABLE _CMakeFormat_VERSION
        ERROR_VARIABLE CMakeFormat_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_CMakeFormat_VERSION MATCHES "([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(CMakeFormat_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" CMakeFormat_VERSION_MAJOR
                             ${CMakeFormat_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" CMakeFormat_VERSION_MINOR
                             ${CMakeFormat_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" CMakeFormat_VERSION_PATCH
                             ${CMakeFormat_VERSION_STRING}
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    CMakeFormat
    REQUIRED_VARS CMakeFormat_EXECUTABLE
    VERSION_VAR CMakeFormat_VERSION_STRING
)
