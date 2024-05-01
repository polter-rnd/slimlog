# [cmake_documentation] DoxygenDocumentation.cmake
#
# Enables documentation generation with Doxygen:
#
# ~~~{.cmake}
# include(DoxygenDocumentation)
# add_doxygen_documentation_target(
#     DOXYGEN_TARGET docs
#     DOXYGEN_IN ${PROJECT_BINARY_DIR}/docs/Doxyfile.in
#     DOXYGEN_OUT_DIR ${PROJECT_BINARY_DIR}/docs
#     SOURCE_DIRS ${PROJECT_BINARY_DIR}/src
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
# @arg __HTML__          Build docs in HTML format
# @arg __LATEX__         Build docs in LATEX format
# @arg __MAN__           Build docs in MAN format
#
# @param DOXYGEN_TARGET  Doxygen documentation target name (mandatory)
# @param SOURCE_DIRS     List of directories with source files
# @param SOURCE_EXCLUDES List of patterns to exclude from docs
# @param CMAKE_INCLUDES  List of patterns for cmake files to include in docs
# @param CMAKE_EXCLUDES  List of patterns for cmake files to exclude from docs
# [/cmake_documentation]
function(add_doxygen_documentation_target)
    # Parse parameters
    set(options CMAKE_DOCS LATEX HTML MAN)
    set(oneValueArgs DOXYGEN_TARGET BRIEF)
    set(multipleValueArgs SOURCE_DIRS SOURCE_EXCLUDES CMAKE_INCLUDES CMAKE_EXCLUDES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    find_package(Doxygen REQUIRED)

    if(NOT ARG_DOXYGEN_TARGET)
        message(FATAL_ERROR "DoxygenDocumentation target is not specified!")
    endif()

    if(NOT ARG_SOURCE_DIRS)
        set(ARG_SOURCE_DIRS ${PROJECT_SOURCE_DIR})
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
        set(cmake_doxygen_file ${PROJECT_BINARY_DIR}/cmake.dox)
        write_cmake_documentation("${cmake_doxygen_file}" SORTED)
    endif()

    # Output directory named the same as target
    set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${ARG_DOXYGEN_TARGET})
    set(DOXYGEN_EXCLUDE_PATTERNS ${ARG_SOURCE_EXCLUDES})
    set(DOXYGEN_PROJECT_BRIEF "String with spaces")

    if(ARG_HTML)
        set(DOXYGEN_GENERATE_HTML ON)
    endif()
    if(ARG_LATEX)
        set(DOXYGEN_GENERATE_LATEX ON)
    endif()
    if(ARG_MAN)
        set(DOXYGEN_GENERATE_MAN ON)
    endif()

    doxygen_add_docs(
        ${ARG_DOXYGEN_TARGET}
        ${ARG_SOURCE_DIRS}
        COMMENT "Generating API documentation with Doxygen"
    )
endfunction()
