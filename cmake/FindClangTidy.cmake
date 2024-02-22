# [cmake_documentation] FindClangTidy.cmake
#
# The module defines the following variables:
# @arg __FindClangTidy_EXECUTABLE__: Path to clang-format executable
# @arg __FindClangTidy_FOUND__: `TRUE` if the clang-format executable was found
# @arg __FindClangTidy_VERSION_STRING__: The version of clang-format found
# @arg __FindClangTidy_VERSION_MAJOR__: The clang-format major version
# @arg __FindClangTidy_VERSION_MINOR__: The clang-format minor version
# @arg __FindClangTidy_VERSION_PATCH__: The clang-format patch version
# [/cmake_documentation]

find_program(
    ClangTidy_EXECUTABLE
    NAMES clang-tidy clang-tidy-8 clang-tidy-7 clang-tidy-6.0
    DOC "clang-tidy executable"
)

mark_as_advanced(ClangTidy_EXECUTABLE)

# Get the version string
if(ClangTidy_EXECUTABLE)
    execute_process(
        COMMAND ${ClangTidy_EXECUTABLE} -version
        OUTPUT_VARIABLE _ClangTidy_VERSION
        ERROR_VARIABLE _ClangTidy_VERSION
    )
    if(_ClangTidy_VERSION MATCHES "LLVM version ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(ClangTidy_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" ClangTidy_VERSION_MAJOR
                             ${ClangTidy_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" ClangTidy_VERSION_MINOR
                             ${ClangTidy_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" ClangTidy_VERSION_PATCH
                             ${ClangTidy_VERSION_STRING}
        )
    else()
        set(ClangTidy_VERSION_STRING "")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ClangTidy
    REQUIRED_VARS ClangTidy_EXECUTABLE
    VERSION_VAR ClangTidy_VERSION_STRING
)
