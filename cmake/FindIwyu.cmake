# [cmake_documentation] FindIwyu.cmake
#
# The module defines the following variables:
# @arg __Iwyu_EXECUTABLE__: Path to include-what-you-use executable
# @arg __Iwyu_FOUND__: `TRUE` if the include-what-you-use executable was found
# @arg __Iwyu_VERSION_STRING__: The version of include-what-you-use found
# @arg __Iwyu_VERSION_MAJOR__: The include-what-you-use major version
# @arg __Iwyu_VERSION_MINOR__: The include-what-you-use minor version
# @arg __Iwyu_VERSION_PATCH__: The include-what-you-use patch version
# [/cmake_documentation]

find_program(
    Iwyu_EXECUTABLE
    NAMES include-what-you-use
    DOC "include-what-you-use executable"
)

mark_as_advanced(Iwyu_EXECUTABLE)

# Get the version string
if(Iwyu_EXECUTABLE)
    execute_process(
        COMMAND ${Iwyu_EXECUTABLE} --version
        OUTPUT_VARIABLE _Iwyu_VERSION
        ERROR_VARIABLE _Iwyu_VERSION
    )
    if(_Iwyu_VERSION MATCHES "include-what-you-use ([0-9]+\\.[0-9]+)")
        set(Iwyu_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" Iwyu_VERSION_MAJOR ${Iwyu_VERSION_STRING})
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" Iwyu_VERSION_MINOR ${Iwyu_VERSION_STRING})
        if(Iwyu_VERSION_STRING MATCHES "[0-9]+\\.[0-9]+\\.([0-9]+)")
            set(Iwyu_VERSION_PATCH
                "${CMAKE_MATCH_1}"
                PARENT_SCOPE
            )
        endif()

        set(Iwyu_VERSION_STRING
            ${Iwyu_VERSION_STRING}
            PARENT_SCOPE
        )
        set(Iwyu_VERSION_MAJOR
            ${Iwyu_VERSION_MAJOR}
            PARENT_SCOPE
        )
        set(Iwyu_VERSION_MINOR
            ${Iwyu_VERSION_MINOR}
            PARENT_SCOPE
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Iwyu
    REQUIRED_VARS Iwyu_EXECUTABLE
    VERSION_VAR Iwyu_VERSION_STRING
)
