# [cmake_documentation] ClangCodeFormatting.cmake
#
# Defines the following functions:
# - @ref add_clang_code_format_targets
# [/cmake_documentation]

# [cmake_documentation] add_clang_code_format_targets()
#
# The function creates format-clang and formatcheck-clang targets which runs
# `clang-format` tool to format or check formatting respectively.
#
# @param CHECK_TARGET  Code format check target name (mandatory)
# @param FORMAT_TARGET Code formatting target name (mandatory)
# @param SOURCE_DIRS   List of source directories to be formatted (mandatory)
# @param EXCLUDE_DIRS  List of directories to be excluded from formatting
# [/cmake_documentation]
function(add_clang_code_format_targets)
    find_program(Diff_EXECUTABLE diff REQUIRED)

    set(options "")
    set(oneValueArgs FORMAT_TARGET CHECK_TARGET)
    set(multiValueArgs SOURCE_DIRS EXCLUDE_DIRS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_FORMAT_TARGET)
        message(FATAL_ERROR "ClangFormat format target is not specified!")
    endif()

    if(NOT ARG_CHECK_TARGET)
        message(FATAL_ERROR "ClangFormat check target is not specified!")
    endif()

    if(NOT ARG_SOURCE_DIRS)
        message(FATAL_ERROR "ClangFormat source directory list is not specified!")
    endif()

    set(supported_file_excensions
        "*.cpp"
        "*.cxx"
        "*.c++"
        "*.c"
        "*.h"
        "*.h++"
        "*.hxx"
        "*.hpp"
    )

    set(check_commands)
    set(format_commands)
    foreach(dir IN LISTS ARG_SOURCE_DIRS)
        foreach(ext IN LISTS supported_file_excensions)
            file(
                GLOB_RECURSE tmp_files
                LIST_DIRECTORIES false
                "${dir}/${ext}"
            )
            foreach(exclude_dir IN LISTS ARG_EXCLUDE_DIRS)
                list(FILTER tmp_files EXCLUDE REGEX "^${exclude_dir}")
            endforeach()
            foreach(file IN LISTS tmp_files)
                # cmake-format: off
                list(APPEND check_commands
                    COMMAND ${ClangFormat_EXECUTABLE}
                            "${file}" | ${Diff_EXECUTABLE} -u "${file}" -
                )
                list(APPEND format_commands
                    COMMAND ${ClangFormat_EXECUTABLE} -i "${file}"
                )
                # cmake-format: on
            endforeach()
        endforeach()
    endforeach()

    add_custom_target(
        ${ARG_FORMAT_TARGET}
        ${format_commands}
        VERBATIM USES_TERMINAL
        COMMENT "Formatting code by 'clang-format'"
    )

    add_custom_target(
        ${ARG_CHECK_TARGET}
        ${check_commands}
        VERBATIM USES_TERMINAL
        COMMENT "Checking code formatting by 'clang-format'"
    )
endfunction()
