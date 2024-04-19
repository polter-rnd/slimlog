# [cmake_documentation] DoxygenDocumentation.cmake
#
# Enables documentation generation with Doxygen:
#
# ~~~{.cmake}
# include(DoxygenDocumentation)
# add_doxygen_documentation_target(
#     DOXYGEN_TARGET docs
#     DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
#     DOXYGEN_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs
#     SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
# )
# ~~~
#
# Defines the following functions:
# - @ref add_doxygen_documentation_target
# [/cmake_documentation]

# [cmake_documentation] add_doxygen_documentation_target()
#
# The function creates target for generating Doxygen documentation
#
# Available options:
# @arg __CMAKE_DOCS__    If documentation for cmake modules should also be generated
#
# @param DOXYGEN_TARGET  Doxygen documentation target name (mandatory)
# @param DOXYGEN_IN      Doxygen.in configuration file (mandatory)
# @param DOXYGEN_OUT_DIR Documentation output directory (mandatory)
# @param SOURCE_DIRS     List of directories with source files
# @param CMAKE_INCLUDES  List of patterns for cmake files to include in docs
# @param CMAKE_EXCLUDES  List of patterns for cmake files to exclude from docs
# [/cmake_documentation]
function(add_doxygen_documentation_target)
    # Parse parameters
    set(options CMAKE_DOCS)
    set(oneValueArgs DOXYGEN_TARGET DOXYGEN_IN DOXYGEN_OUT_DIR)
    set(multipleValueArgs SOURCE_DIRS CMAKE_INCLUDES CMAKE_EXCLUDES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    find_package(Doxygen REQUIRED)

    if(NOT ARG_DOXYGEN_TARGET)
        message(FATAL_ERROR "DoxygenDocumentation target is not specified!")
    endif()

    if(NOT ARG_DOXYGEN_IN)
        message(FATAL_ERROR "DoxygenDocumentation input file is not specified!")
    endif()

    if(NOT ARG_DOXYGEN_OUT_DIR)
        message(FATAL_ERROR "DoxygenDocumentation output directory is not specified!")
    endif()

    if(ARG_CMAKE_DOCS)
        # Write documentation for CMake files
        include(CMakeDocumentation)
        parse_cmake_documentation(
            INCLUDES ${ARG_CMAKE_INCLUDES}
            EXCLUDES ${ARG_CMAKE_EXCLUDES}
            START_FLAG "[cmake_documentation]"
            END_FLAG "[/cmake_documentation]"
        )
        set(cmake_doxygen_file ${CMAKE_CURRENT_BINARY_DIR}/cmake.dox)
        write_cmake_documentation("${cmake_doxygen_file}" SORTED)
    endif()

    # request to configure the file
    set(DOXYGEN_OUTPUT_DIRECTORY "\"${ARG_DOXYGEN_OUT_DIR}\"")
    string(REGEX REPLACE ";" "\"\n" DOXYGEN_INPUT "${ARG_SOURCE_DIRS}")
    if(ARG_CMAKE_DOCS AND EXISTS "${cmake_doxygen_file}")
        set(DOXYGEN_INPUT "\"${DOXYGEN_INPUT}\" \"${cmake_doxygen_file}\"")
    endif()
    configure_file(${ARG_DOXYGEN_IN} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    add_custom_target(
        ${ARG_DOXYGEN_TARGET}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM USES_TERMINAL
    )
endfunction()
