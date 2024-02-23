# [cmake_documentation] FindGcovr.cmake
#
# The module defines the following variables:
# @arg __Gcovr_EXECUTABLE__: Path to gcovr executable
# @arg __Gcovr_FOUND__: `TRUE` if the gcovr executable was found
# @arg __Gcovr_VERSION_STRING__: The version of gcovr found
# @arg __Gcovr_VERSION_MAJOR__: The gcovr major version
# @arg __Gcovr_VERSION_MINOR__: The gcovr minor version
# @arg __Gcovr_VERSION_PATCH__: The gcovr patch version
# [/cmake_documentation]

find_program(
    Gcovr_EXECUTABLE
    NAMES gcovr
    DOC "gcovr executable"
)

mark_as_advanced(Gcovr_EXECUTABLE)

# Get the version string
if(Gcovr_EXECUTABLE)
    execute_process(
        COMMAND ${Gcovr_EXECUTABLE} --version
        OUTPUT_VARIABLE _Gcovr_VERSION
        ERROR_VARIABLE _Gcovr_VERSION
    )
    if(_Gcovr_VERSION MATCHES "gcovr ([0-9]+\\.[0-9]+)")
        set(Gcovr_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" Gcovr_VERSION_MAJOR ${Gcovr_VERSION_STRING})
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" Gcovr_VERSION_MINOR ${Gcovr_VERSION_STRING})

        set(Gcovr_VERSION_STRING
            ${Gcovr_VERSION_STRING}
            PARENT_SCOPE
        )
        set(Gcovr_VERSION_MAJOR
            ${Gcovr_VERSION_MAJOR}
            PARENT_SCOPE
        )
        set(Gcovr_VERSION_MINOR
            ${Gcovr_VERSION_MINOR}
            PARENT_SCOPE
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Gcovr
    REQUIRED_VARS Gcovr_EXECUTABLE
    VERSION_VAR Gcovr_VERSION_STRING
)
