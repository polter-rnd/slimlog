# [cmake_documentation] CMakeFormatting.cmake
#
# Defines the following functions:
# - @ref add_cmake_code_format_targets
# [/cmake_documentation]

# [cmake_documentation] add_cmake_code_format_targets()
#
# The function creates `format-cmake` and `formatcheck-cmake` targets which runs
# `cmake-format` tool to format or check formatting respectively.
#
# @param CHECK_TARGET  Code format check target name (mandatory)
# @param FORMAT_TARGET Code formatting target name (mandatory)
# @param SOURCE_DIRS   List of source directories to be formatted (mandatory)
# @param EXCLUDE_DIRS  List of directories to be excluded from formatting
# [/cmake_documentation]
function(add_cmake_code_format_targets)
    find_package(CMakeFormat REQUIRED)
    find_program(Diff_EXECUTABLE diff REQUIRED)

    set(options "")
    set(oneValueArgs FORMAT_TARGET CHECK_TARGET)
    set(multiValueArgs SOURCE_DIRS EXCLUDE_DIRS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_FORMAT_TARGET)
        message(FATAL_ERROR "CMakeFormat format target is not specified!")
    endif()

    if(NOT ARG_CHECK_TARGET)
        message(FATAL_ERROR "CMakeFormat check target is not specified!")
    endif()

    if(NOT ARG_SOURCE_DIRS)
        message(FATAL_ERROR "CMakeFormat source directory list is not specified!")
    endif()

    set(ARG_SUPPORTED_FILE_EXTENSIONS "CMakeLists.txt" "*.cmake")

    set(ARG_CHECK_COMMANDS)
    set(ARG_FORMAT_COMMANDS)
    foreach(dir IN LISTS ARG_SOURCE_DIRS)
        file(
            GLOB_RECURSE tmp_files
            LIST_DIRECTORIES false
            ARG_SOURCE_DIRS ${ARG_SUPPORTED_FILE_EXTENSIONS}
        )

        foreach(exclude_dir IN LISTS ARG_EXCLUDE_DIRS)
            list(FILTER tmp_files EXCLUDE REGEX "^${exclude_dir}")
        endforeach()

        foreach(file IN LISTS tmp_files)
            # cmake-format: off
            list(APPEND ARG_CHECK_COMMANDS
                 COMMAND ${CMakeFormat_EXECUTABLE} "${file}" | ${Diff_EXECUTABLE} -u "${file}" -
            )
            list(APPEND ARG_FORMAT_COMMANDS
                 COMMAND ${CMakeFormat_EXECUTABLE} -i "${file}"
            )
            # cmake-format: on
        endforeach()
    endforeach()

    add_custom_target(
        ${ARG_FORMAT_TARGET}
        ${ARG_FORMAT_COMMANDS}
        VERBATIM USES_TERMINAL
        COMMENT "Formatting cmake listfiles by 'cmake-format'"
    )

    add_custom_target(
        ${ARG_CHECK_TARGET}
        ${ARG_CHECK_COMMANDS}
        VERBATIM USES_TERMINAL
        COMMENT "Checking cmake listfiles formatting by 'cmake-format'"
    )
endfunction()
