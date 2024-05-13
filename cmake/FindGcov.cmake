# [cmake_documentation] FindGcov.cmake
#
# The module defines the following variables:
# @arg __FindGcov_EXECUTABLE__: Path to clang-format executable
# @arg __FindGcov_FOUND__: `TRUE` if the clang-format executable was found
# @arg __FindGcov_VERSION_STRING__: The version of clang-format found
# @arg __FindGcov_VERSION_MAJOR__: The clang-format major version
# @arg __FindGcov_VERSION_MINOR__: The clang-format minor version
# @arg __FindGcov_VERSION_PATCH__: The clang-format patch version
# [/cmake_documentation]

include(Helpers)
set_versioned_compiler_names(
    Gcov
    COMPILER GNU
    NAMES gcov
)

find_program(
    Gcov_EXECUTABLE
    NAMES ${Gcov_NAMES} gcov
    HINTS ${Gcov_HINTS}
    DOC "gcov executable"
)

mark_as_advanced(Gcov_EXECUTABLE)

# Get the version string
if(Gcov_EXECUTABLE)
    execute_process(
        COMMAND ${Gcov_EXECUTABLE} -version
        OUTPUT_VARIABLE _Gcov_VERSION
        ERROR_VARIABLE _Gcov_VERSION
    )
    if(_Gcov_VERSION MATCHES "gcov .* ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(Gcov_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" Gcov_VERSION_MAJOR
                             ${Gcov_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" Gcov_VERSION_MINOR
                             ${Gcov_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" Gcov_VERSION_PATCH
                             ${Gcov_VERSION_STRING}
        )

        set(Gcov_VERSION_STRING
            ${Gcov_VERSION_STRING}
            PARENT_SCOPE
        )
        set(Gcov_VERSION_MAJOR
            ${Gcov_VERSION_MAJOR}
            PARENT_SCOPE
        )
        set(Gcov_VERSION_MINOR
            ${Gcov_VERSION_MINOR}
            PARENT_SCOPE
        )
        set(Gcov_VERSION_PATCH
            ${Gcov_VERSION_PATCH}
            PARENT_SCOPE
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Gcov
    REQUIRED_VARS Gcov_EXECUTABLE
    VERSION_VAR Gcov_VERSION_STRING
)
