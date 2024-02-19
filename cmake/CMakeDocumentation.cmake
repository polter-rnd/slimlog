# [cmake_documentation] CMakeDocumentation.cmake
#
# Defines the following functions:
# - @ref parse_cmake_documentation
# - @ref write_cmake_documentation
#
# How to document your CMake files:
# -# Use `<CMAKE_DOC_START_FLAG> <name>` flag to start your documentation
# -# Document your cmake file like you want (you can use doxygen syntax)
# -# Use `<CMAKE_DOC_END_FLAG>` flag to finish your documentation
#
# The parser will generate a documentation section for each filenames
# parsed with a section for each macros/functions under the file parsed.
#
# `<name>` could be:
# - A file name:
#   In this case, the parser will add the doxygen documentation of the filename.
# - A simple name (function/macro) name:
#   In this case, the parser will add a section under the filename one.
#
# Example:
# ~~~{.cmake}
#    ## <CMAKE_DOC_START_FLAG> FindMySDK.cmake
#    ##
#    ## Try to find MySDK.
#    ## Once done this will define:
#    ##  @a MYSDK_FOUND - system has MYSDK
#    ##  @a MYSDK_INCLUDE_DIR - the MYSDK include directory
#    ##  @a MYSDK_LIBRARIES - The libraries provided by MYSDK
#    ##
#    ## You can use the MYSDK_DIR environment variable
#    ##
#    ## <CMAKE_DOC_END_FLAG>
# ~~~
#
# This file is used to generated cmake documentation.
# It allows to create cmakedoc custom_target (see docs/CMakeLists.txt)
#
# In your CMakeLists.txt, just include this file. Or call subsequently:
#
# ~~~{.cmake}
# include(CMakeDocumentation)
# parse_cmake_documentation()
# write_cmake_documentation("\${CMAKE_SOURCE_DIR}/cmake/cmake.dox" SORTED)
# ~~~
#
# Then the standard Doxygen application will be able to parse your cmake.dox documentation to be
# include.
#
# [/cmake_documentation]

# Pragma once
include_guard(GLOBAL)

# [cmake_documentation] parse_cmake_documentation()
#
# Parse for a list of cmake files in order to recover documentation notes.
#
# ~~~{.cmake}
# parse_cmake_documentation(
#      [INCLUDES   includeFilePathPattern] # default: \${CMAKE_SOURCE_DIR}/*.cmake*
#      [EXCLUDES   excludeFilePathPattern] # default:
#      [START_FLAG matchingStartFlag]      # default: [cmake_documentation]
#      [END_FLAG   matchingEndFlag]        # default: [/cmake_documentation]
# )
# ~~~
#
# @param INCLUDES   List of included files patterns
# @param EXCLUDES   List of excluded files patterns
# @param START_FLAG Start documentation marker
# @param END_FLAG   End documentation marker
#
# [/cmake_documentation]
macro(parse_cmake_documentation)
    set(options "")
    set(oneValueArgs START_FLAG END_FLAG)
    set(multiValueArgs INCLUDES EXCLUDES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # INCLUDES cmake file to the list files
    if(NOT DEFINED ARG_INCLUDES)
        set(ARG_INCLUDES "${CMAKE_SOURCE_DIR}/*.cmake") # all *.cmake by default
    endif()

    foreach(include_pattern ${ARG_INCLUDES})
        file(GLOB_RECURSE cmake_files_include "${include_pattern}")
        list(APPEND cmake_files_list ${cmake_files_include})
    endforeach()

    foreach(exclude_pattern ${ARG_EXCLUDES})
        file(GLOB_RECURSE cmake_files_exclude "${exclude_pattern}")
        if(cmake_files_exclude STREQUAL "") # files found in pattern
            message("Note: no files found matching exclude pattern \"${exclude_pattern}\"")
        else()
            list(REMOVE_ITEM cmake_files_list ${cmake_files_exclude})
        endif()
        # message("Exclude file from cmake doc: \"${cmake_files_exclude}\"")
    endforeach()

    if(NOT DEFINED ARG_START_FLAG)
        set(ARG_START_FLAG "[cmake_documentation]")
    endif()
    if(NOT DEFINED ARG_END_FLAG)
        set(ARG_END_FLAG "[/cmake_documentation]")
    endif()

    # Process for each file of the list
    foreach(cmake_file ${cmake_files_list})
        file(READ ${cmake_file} cmake_file_content)
        # message(STATUS "Generate cmake doc for: \"${cmake_file}\"")

        # Escape all symbols to safely use in regex
        string(REGEX REPLACE "(.)" "\\\\\\1" start_flag_regex "${ARG_START_FLAG}")
        string(REGEX REPLACE "(.)" "\\\\\\1" end_flag_regex "${ARG_END_FLAG}")
        set(start_flag_regex "#+[ \t\r\n]*${start_flag_regex}[ \t\r\n]*")
        set(end_flag_regex "#+[ \t\r\n]*${end_flag_regex}[ \t\r\n]*")

        # Check pair tags for cmake doc
        string(REGEX MATCHALL "${start_flag_regex}" matches_start "${cmake_file_content}")
        list(LENGTH matches_start matches_start_count)
        string(REGEX MATCHALL "${end_flag_regex}" matches_end "${cmake_file_content}")
        list(LENGTH matches_end matches_end_count)
        if(NOT matches_start_count EQUAL matches_end_count)
            message(
                "WARNING: You forgot a tag for cmake documentation generation.
                 Matches ${ARG_START_FLAG} = ${matches_start_count}.
                 Matches ${ARG_END_FLAG} = ${matches_end_count}.
                 The cmake file ${cmake_file} will be ignored for cmake doc generation."
            )
            set(cmake_file_content "") # To skip the parsing
        endif()

        # This is needed to preserve ";" characters. We need to escape them as many times as this
        # variable get passed to any function.
        string(REGEX REPLACE ";" "\\\\\\\\\\\\\\\\\\\\\\\\\\\;" cmake_file_content
                             "${cmake_file_content}"
        )

        # Split into lines
        string(REGEX REPLACE "\r?\n" ";" cmake_file_content "${cmake_file_content}")
        set(section_started false)
        set(doc_section "")
        set(doc_section_content "")
        foreach(line ${cmake_file_content})
            # Find the start balise
            string(REGEX MATCH "${start_flag_regex}.*" match_start "${line}")
            if(match_start AND NOT section_started)
                set(section_started true)
                string(REGEX REPLACE "${start_flag_regex}(.*)" "\\1" doc_section "${match_start}")
            endif()

            # Find the end balise
            string(REGEX MATCH "${end_flag_regex}" match_end "${line}")
            if(match_end AND section_started)
                if(NOT doc_section)
                    set(doc_section "other")
                endif()

                process_cmake_documentation(
                    "${cmake_file}" "${doc_section}" "${doc_section_content}"
                )

                set(section_started false)
                set(doc_section "")
                set(line_comment "")
                set(doc_section_content "")
            endif()

            # Extract comment between balises
            string(REGEX MATCH "^#.*" match_comment ${line})
            # cmake-format: off
            if(match_comment AND section_started AND NOT match_start AND NOT match_end)
                string(REGEX REPLACE "^#+" "" line_comment ${match_comment})
                list(APPEND doc_section_content "${line_comment}")
            endif()
            # cmake-format: on
        endforeach()
    endforeach()
endmacro()

# [cmake_documentation] write_cmake_documentation(outputPathFileName)
#
# Generate the .dox file using globale variables set by @ref parse_cmake_documentation
# ~~~{.cmake}
# write_cmake_documentation(<outputPathFileName>
#      [SORTED]
#      [HEADER myCustomHeaderString]
#      [FOOTER myCustomFooterString]
# )
# ~~~
#
# Required arguments:
# @arg __outputPathFileName__: Output file name
#
# @param SORTED If set file and function lists will be sorted
# @param HEADER Custom header string
# @param FOOTER Custom footer string
#
# [/cmake_documentation]
function(write_cmake_documentation outputPathFileName)
    set(options SORTED)
    set(oneValueArgs HEADER FOOTER)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Sort global lists
    if(ARG_SORTED)
        if(PROCESS_CMAKE_DOCUMENTATION_FILES_LIST)
            list(SORT PROCESS_CMAKE_DOCUMENTATION_FILES_LIST)
        endif()
        if(PROCESS_CMAKE_DOCUMENTATION_FILES_DESCRIPTIONS)
            string(REGEX REPLACE "\\\;" "\\\\\\\;" PROCESS_CMAKE_DOCUMENTATION_FILES_DESCRIPTIONS
                                 "${PROCESS_CMAKE_DOCUMENTATION_FILES_DESCRIPTIONS}"
            )
            list(SORT PROCESS_CMAKE_DOCUMENTATION_FILES_DESCRIPTIONS)
        endif()
        if(PROCESS_CMAKE_DOCUMENTATION_SECTION_LIST)
            list(SORT PROCESS_CMAKE_DOCUMENTATION_SECTION_LIST)
        endif()
        if(PROCESS_CMAKE_DOCUMENTATION_SECTION_CONTENT)
            string(REGEX REPLACE "\\\;" "\\\\\\\;" PROCESS_CMAKE_DOCUMENTATION_SECTION_CONTENT
                                 "${PROCESS_CMAKE_DOCUMENTATION_SECTION_CONTENT}"
            )
            list(SORT PROCESS_CMAKE_DOCUMENTATION_SECTION_CONTENT)
        endif()
    endif()

    # Strip global lists
    if(PROCESS_CMAKE_DOCUMENTATION_FILES_LIST)
        list(REMOVE_DUPLICATES PROCESS_CMAKE_DOCUMENTATION_FILES_LIST)
    endif()

    file(WRITE "${outputPathFileName}" "/*! \\page cmakePage CMake Documentation")

    # Header file
    # cmake-format: off
    list(APPEND header
        "${ARG_HEADER}"
        ""
        "\\section Index"
        ""
        "- \\ref cmake_doc"
        "${PROCESS_CMAKE_DOCUMENTATION_SECTION_LIST}"
        ""
        "- \\ref cmake_files"
        "${PROCESS_CMAKE_DOCUMENTATION_FILES_LIST}"
        ""
    )
    # cmake-format: on
    list(JOIN header "\n" header)
    file(APPEND "${outputPathFileName}" "${header}\n")

    # Fill content documentation
    # cmake-format: off
    set(content
        "<hr />"
        "\\section cmake_doc Functions and Macros"
        ""
        "${PROCESS_CMAKE_DOCUMENTATION_SECTION_CONTENT}"
        "<hr />"
        "\\section cmake_files Files"
        ""
        "${PROCESS_CMAKE_DOCUMENTATION_FILES_DESCRIPTIONS}"
    )
    # cmake-format: on
    list(JOIN content "\n" content)
    file(APPEND "${outputPathFileName}" "${content}\n")

    # Footer file
    set(footer "${ARG_FOOTER}*/")
    file(APPEND "${outputPathFileName}" "${footer}\n")
endfunction()

# ##########################################################################
# ####################   INTERNAL USE   ####################################
# ##########################################################################

# This macro define some global cmake variables to be used when generate the .dox file.
macro(process_cmake_documentation cmakeFile sectionName sectionContent)
    # Strip cmakeFile path
    string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/?\\\\?" "" filename_stripped ${cmakeFile})

    # If sectionName is a fileName, add it to the list a file with its description
    string(REGEX MATCH ".*[.].*" filename_with_extension ${sectionName})
    if(filename_with_extension) # for .cmake
        string(REGEX REPLACE "(.*)[.].*" "\\1" section_file_name ${filename_with_extension})
        string(REGEX MATCH ".*[.].*" filename_with_extension ${section_file_name})
        if(filename_with_extension) # double extension match (for .in)
            string(REGEX REPLACE "(.*)[.].*" "\\1" section_file_name ${filename_with_extension})
        endif()

        # Section file name list
        list(APPEND PROCESS_CMAKE_DOCUMENTATION_FILES_LIST "  - \\ref ${section_file_name}")
        # Section file description list
        set(content "\\subsection ${section_file_name} ${section_file_name}"
                    "\\b Path: ${filename_stripped}" "" "${sectionContent}"
        )
        list(JOIN content "\n" content)
        list(APPEND PROCESS_CMAKE_DOCUMENTATION_FILES_DESCRIPTIONS "${content}\n")
    else()
        # Strip braces from section name (e.g. test_func() -> test_func)
        string(REGEX REPLACE "([^\(]+).*" "\\1" section_anchor "${sectionName}")
        # Section doc name list
        list(APPEND PROCESS_CMAKE_DOCUMENTATION_SECTION_LIST "  - \\ref ${section_anchor}")
        # Section doc description list
        set(content "\\subsection ${section_anchor} ${sectionName}"
                    "\\b From: ${filename_stripped}" "" "${sectionContent}"
        )
        list(JOIN content "\n" content)
        list(APPEND PROCESS_CMAKE_DOCUMENTATION_SECTION_CONTENT "${content}\n")
    endif()
endmacro()
