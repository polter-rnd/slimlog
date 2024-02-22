# [cmake_documentation] FindCppcheck.cmake
#
# The module defines the following variables:
# @arg __Cppcheck_EXECUTABLE__: Path to cppcheck executable
# @arg __Cppcheck_FOUND__: `TRUE` if the cppcheck executable was found
# @arg __Cppcheck_VERSION_STRING__: The version of cppcheck found
# @arg __Cppcheck_VERSION_MAJOR__: The cppcheck major version
# @arg __Cppcheck_VERSION_MINOR__: The cppcheck minor version
# @arg __Cppcheck_VERSION_PATCH__: The cppcheck patch version
# [/cmake_documentation]

find_program(
    Cppcheck_EXECUTABLE
    NAMES cppcheck
    DOC "cppcheck executable"
)

mark_as_advanced(Cppcheck_EXECUTABLE)

# Get the version string
if(Cppcheck_EXECUTABLE)
    execute_process(
        COMMAND ${Cppcheck_EXECUTABLE} --version
        OUTPUT_VARIABLE _Cppcheck_VERSION
        ERROR_VARIABLE _Cppcheck_VERSION
    )
    if(_Cppcheck_VERSION MATCHES "Cppcheck ([0-9]+\\.[0-9]+)")
        set(Cppcheck_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" Cppcheck_VERSION_MAJOR
                             ${Cppcheck_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" Cppcheck_VERSION_MINOR
                             ${Cppcheck_VERSION_STRING}
        )
        if(Cppcheck_VERSION_STRING MATCHES "[0-9]+\\.[0-9]+\\.([0-9]+)")
            set(Cppcheck_VERSION_PATCH "${CMAKE_MATCH_1}")
        endif()
    else()
        set(Cppcheck_VERSION_STRING "")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cppcheck
    REQUIRED_VARS Cppcheck_EXECUTABLE
    VERSION_VAR Cppcheck_VERSION_STRING
)
