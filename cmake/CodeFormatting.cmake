# [cmake_documentation] CodeFormatting.cmake
#
# Creates target for source code and cmake file formatting using various tools.
# Usage example:
#
# ~~~{.cmake}
# include(CodeFormatting)
# add_code_format_targets(
#     CHECK_TARGET formatcheck
#     FORMAT_TARGET format
#     SOURCE_DIRS \${PROJECT_SOURCE_DIR}
#     EXCLUDE_DIRS \${PROJECT_BINARY_DIR}
# )
# ~~~
#
# In example above, will be created the following targets:
# - __formatcheck-clang__: checking code formatting using `clang-format`
# - __formatcheck-cmake__: checking cmake formatting using `cmake-format`
# - __formatcheck__:       calls both `formatcheck-clang` and `formatcheck-cmake`
# - __format-clang__:      formatting source code using `clang-format`
# - __format-cmake__:      formatting cmake files using `cmakae-format`
# - __formatcheck__:       calls both `format-clang` and `format-cmake`
#
# Defines the following functions:
# - @ref add_code_format_targets
#
# Uses the following parameters:
# @arg __FORMAT_CLANG__:             Enable source code formatting with clang-format
# @arg __FORMAT_CMAKE__:             Enable cmake files formatting with cmake-format
# @arg __CLANG_FORMAT_MIN_VERSION__: Minimum required version for `clang-format`
# @arg __CMAKE_FORMAT_MIN_VERSION__: Minimum required version for `cmake-format`
# [/cmake_documentation]

include(Helpers)
find_package_switchable(
    ClangFormat
    OPTION FORMAT_CLANG
    PURPOSE "Enable source code formatting with clang-format"
    MIN_VERSION ${CLANG_FORMAT_MIN_VERSION}
)
find_package_switchable(
    CMakeFormat
    OPTION FORMAT_CMAKE
    PURPOSE "Enable cmake files formatting with cmake-format"
    MIN_VERSION ${CMAKE_FORMAT_MIN_VERSION}
)

# [cmake_documentation] add_code_format_targets()
#
# The function creates formatcheck and codeformat targets
#
# @param CHECK_TARGET  CMake and cpp check target name (mandatory)
# @param FORMAT_TARGET CMake and cpp formatting target name (mandatory)
# @param EXCLUDE_DIRS  List of source directories to be formatted (mandatory)
# @param SOURCE_DIRS   List of directories to be excluded from formatting
# [/cmake_documentation]
function(add_code_format_targets)
    # Parse parameters
    set(options "")
    set(oneValueArgs CHECK_TARGET FORMAT_TARGET)
    set(multipleValueArgs EXCLUDE_DIRS SOURCE_DIRS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    if(NOT ARG_FORMAT_TARGET)
        message(FATAL_ERROR "CodeFormatting format target is not specified!")
    endif()

    if(NOT ARG_CHECK_TARGET)
        message(FATAL_ERROR "CodeFormatting check target is not specified!")
    endif()

    add_custom_target(${ARG_FORMAT_TARGET})
    add_custom_target(${ARG_CHECK_TARGET})

    if(FORMAT_CLANG)
        include(ClangCodeFormatting)
        add_clang_code_format_targets(
            FORMAT_TARGET ${ARG_FORMAT_TARGET}-clang
            CHECK_TARGET ${ARG_CHECK_TARGET}-clang
            EXCLUDE_DIRS ${ARG_EXCLUDE_DIRS}
            SOURCE_DIRS ${ARG_SOURCE_DIRS}
        )
        add_dependencies(${ARG_FORMAT_TARGET} ${ARG_FORMAT_TARGET}-clang)
        add_dependencies(${ARG_CHECK_TARGET} ${ARG_CHECK_TARGET}-clang)
    endif()

    if(FORMAT_CMAKE)
        include(CMakeFormatting)
        add_cmake_code_format_targets(
            FORMAT_TARGET ${ARG_FORMAT_TARGET}-cmake
            CHECK_TARGET ${ARG_CHECK_TARGET}-cmake
            EXCLUDE_DIRS ${ARG_EXCLUDE_DIRS}
            SOURCE_DIRS ${ARG_SOURCE_DIRS}
        )
        add_dependencies(${ARG_FORMAT_TARGET} ${ARG_FORMAT_TARGET}-cmake)
        add_dependencies(${ARG_CHECK_TARGET} ${ARG_CHECK_TARGET}-cmake)
    endif()
endfunction()
