# [cmake_documentation] FindClangFormat.cmake
#
# The module defines the following variables:
# @arg __ClangFormat_EXECUTABLE__: Path to clang-format executable
# @arg __ClangFormat_FOUND__: `TRUE` if the clang-format executable was found
# @arg __ClangFormat_VERSION_STRING__: The version of clang-format found
# @arg __ClangFormat_VERSION_MAJOR__: The clang-format major version
# @arg __ClangFormat_VERSION_MINOR__: The clang-format minor version
# @arg __ClangFormat_VERSION_PATCH__: The clang-format patch version
#
# This module accepts the following variables:
# @arg ClangFormat directory hints:
#      __LLVM_DIR__, __LLVM_ROOT__, __ENV{LLVM_DIR}__, __ENV{LLVM_ROOT}__
# [/cmake_documentation]

# Hints where to look for ClangFormat executable Environment and user variables
set(ClangFormat_HINTS "")
foreach(hint LLVM_DIR LLVM_ROOT)
    if(DEFINED ${hint})
        if((EXISTS "${${hint}}") AND (IS_DIRECTORY "${${hint}}"))
            list(APPEND ClangFormat_HINTS "${${hint}}")
        endif()
    endif()
    if(DEFINED ENV{${hint}})
        if((EXISTS "$ENV{${hint}}") AND (IS_DIRECTORY "$ENV{${hint}}"))
            list(APPEND ClangFormat_HINTS "$ENV{${hint}}")
        endif()
    endif()
endforeach()

find_program(
    ClangFormat_EXECUTABLE
    NAMES clang-format clang-format-8 clang-format-7 clang-format-6.0
    HINTS ${ClangFormat_HINTS}
    DOC "clang-format executable"
)

mark_as_advanced(ClangFormat_EXECUTABLE)

# Extract version from command "clang-format -version"
if(ClangFormat_EXECUTABLE)
    execute_process(
        COMMAND ${ClangFormat_EXECUTABLE} -version
        OUTPUT_VARIABLE _ClangFormat_VERSION
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_ClangFormat_VERSION MATCHES "clang-format version ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(ClangFormat_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" ClangFormat_VERSION_MAJOR
                             ${ClangFormat_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" ClangFormat_VERSION_MINOR
                             ${ClangFormat_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" ClangFormat_VERSION_PATCH
                             ${ClangFormat_VERSION_STRING}
        )
    else()
        set(ClangFormat_VERSION_STRING "")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ClangFormat
    REQUIRED_VARS ClangFormat_EXECUTABLE
    VERSION_VAR ClangFormat_VERSION_STRING
)
