# [cmake_documentation] StaticCodeAnalysis.cmake
#
# Defines the following functions:
# - @ref add_static_analysis_targets
#
# Creates target for static code analysis using various tools.
# Usage example:
#
# ~~~{.cmake}
# include(StaticCodeAnalysis)
# set(TARGET hello)
# add_executable(hello main.cpp)
# add_static_analysis_targets(TARGET hello ANALYSIS_TARGET codeanalysis)
# ~~~
#
# In example above, will be created the following targets:
# - `hello-cppcheck`:  analysis of target `hello` using `cppcheck`
# - `hello-clangtidy`: analysis of target `hello` using `clang-tidy`
# - `hello-iwyu`:      analysis of target `hello` using `include-what-you-use`
# - `codeanalysis`:    calls all targets above
#
# Uses the following parameters:
# @arg __ENABLE_CPPCHECK__:   Enable basic static analysis of C/C++ code
# @arg __ENABLE_CLANG_TIDY__: Enable clang-based analysis and linting of C/C++ code
# @arg __ENABLE_IWYU__:       Enable include file analysis in C and C++ source files
# [/cmake_documentation]

include(Helpers)
find_package_switchable(
    Cppcheck
    OPTION ENABLE_CPPCHECK
    PURPOSE "Basic static analysis of C/C++ code"
)
find_package_switchable(
    ClangTidy
    OPTION ENABLE_CLANG_TIDY
    PURPOSE "Clang-based analysis and linting of C/C++ code"
)
find_package_switchable(
    Iwyu
    OPTION ENABLE_IWYU
    PURPOSE "Analyze includes in C and C++ source files"
)

# [cmake_documentation] add_static_analysis_targets()
#
# The function creates codeanalysis target which in its turn calls
# cppcheck, clang-tidy and include-what-you-mean to generate analysis reports.
#
# @param TARGET                      Which target to analyze (mandatory)
# @param ANALYSIS_TARGET             Name of global analysis target (mandatory)
# @param CPPCHECK_EXTRA_ARGS         Additional flags for `cppcheck`
# @param CPPCHECK_SUPPRESSIONS_LISTS Suppression lists for `cppcheck`
# @param CLANG_TIDY_EXTRA_ARGS       Additional arguments for `clang-tidy`
# @param CLANG_TIDY_HEADER_FILTER    Header filter for `clang-tidy` (default: `.*`)
# @param IWYU_EXTRA_ARGS             Additional arguments for `include-what-you-use`
# @param IWYU_MAPPING_FILES_ARGS     Mapping files for `include-what-you-use`
# [/cmake_documentation]
function(add_static_analysis_targets)
    set(options "")
    set(oneValueArgs TARGET ANALYSIS_TARGET)
    set(multipleValueArgs CPPCHECK_EXTRA_ARGS CPPCHECK_SUPPRESSIONS_LISTS CLANG_TIDY_EXTRA_ARGS
                          CLANG_TIDY_HEADER_FILTER IWYU_MAPPING_FILES_ARGS
    )
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "StaticCodeAnalysis target is not specified!")
    endif()

    if(NOT ARG_ANALYSIS_TARGET)
        message(FATAL_ERROR "StaticCodeAnalysis target name is not specified!")
    endif()

    get_target_property(target_source_dir ${ARG_TARGET} SOURCE_DIR)
    get_target_property(target_sources ${ARG_TARGET} SOURCES)

    # Include only cpp files and convert to absolute path
    set(target_sources_absolute "")
    list(FILTER target_sources INCLUDE REGEX ".*\.cpp")
    foreach(source IN LISTS target_sources)
        get_filename_component(source_absolute "${source}" REALPATH BASE_DIR "${target_source_dir}")
        list(APPEND target_sources_absolute "${source_absolute}")
    endforeach()

    if(NOT TARGET ${ARG_ANALYSIS_TARGET})
        add_custom_target(${ARG_ANALYSIS_TARGET})
    endif()

    if(ENABLE_CPPCHECK)
        find_package(Cppcheck REQUIRED)
        if(ARG_CPPCHECK_SUPPRESSIONS_LISTS)
            set(cppcheck_args_suppressions_list "")
            foreach(suppression ${ARG_CPPCHECK_SUPPRESSIONS_LISTS})
                list(APPEND cppcheck_args_suppressions_list "--suppressions-list=${suppression}")
            endforeach()
        endif()
        # TO DO: Analyzes all files from compilation database rather than just the target_sources
        add_custom_target(
            ${ARG_TARGET}-cppcheck
            COMMAND
                ${Cppcheck_EXECUTABLE}
                --enable=warning,performance,portability,information,missingInclude
                # Cppcheck does not need standard library headers to get proper results
                --suppress=missingIncludeSystem
                # Ignore build folder
                -i ${CMAKE_BINARY_DIR}
                # Enable inline suppressions
                --inline-suppr
                # Suppress warnings in some specific files
                ${cppcheck_args_suppressions_list}
                --project=${CMAKE_BINARY_DIR}/compile_commands.json
                # User-specified args
                ${ARG_CPPCHECK_EXTRA_ARGS}
                # Find all source files from the root of the repo
                $<LIST:TRANSFORM,${target_sources_absolute},PREPEND,--file-filter=>
            USES_TERMINAL
            COMMENT "Analyzing code by 'cppcheck'"
        )
        add_dependencies(${ARG_ANALYSIS_TARGET} ${ARG_TARGET}-cppcheck)
    endif()

    if(ENABLE_CLANG_TIDY)
        find_program(
            RunClangTidy_EXECUTABLE
            NAMES run-clang-tidy.py run-clang-tidy
            HINTS /usr/share/clang REQUIRED
        )
        if(ARG_CLANG_TIDY_EXTRA_ARGS)
            set(clang_tidy_extra_args "")
            foreach(extra_arg ${ARG_CLANG_TIDY_EXTRA_ARGS})
                list(APPEND clang_tidy_extra_args "-extra-arg=${extra_arg}")
            endforeach()
        endif()
        # if no override is specified, set default: '-header-filter=.*'
        if(NOT ARG_CLANG_TIDY_HEADER_FILTER)
            set(ARG_CLANG_TIDY_HEADER_FILTER ".*")
        endif()
        add_custom_target(
            ${ARG_TARGET}-clangtidy
            COMMAND
                ${RunClangTidy_EXECUTABLE} -quiet -p=${CMAKE_BINARY_DIR}
                -header-filter=${ARG_CLANG_TIDY_HEADER_FILTER} ${clang_tidy_extra_args}
                ${target_sources_absolute}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            USES_TERMINAL
            COMMENT "Analyzing code by 'clang-tidy'"
        )
        add_dependencies(${ARG_ANALYSIS_TARGET} ${ARG_TARGET}-clangtidy)
    endif()

    if(ENABLE_IWYU)
        find_program(IwyuTool_EXECUTABLE iwyu_tool.py iwyu_tool REQUIRED)
        if(ARG_IWYU_MAPPING_FILES_ARGS)
            set(iwyu_mapping_files_args "")
            foreach(mapping_file ${ARG_IWYU_MAPPING_FILES_ARGS})
                list(APPEND iwyu_mapping_files_args "-Xiwyu --mapping_file=${mapping_file}")
            endforeach()
        endif()
        if(ARG_IWYU_EXTRA_ARGS)
            set(iwyu_extra_args "")
            foreach(extra_arg ${ARG_IWYU_EXTRA_ARGS})
                list(APPEND iwyu_extra_args "-Xiwyu ${extra_arg}")
            endforeach()
        endif()
        add_custom_target(
            ${ARG_TARGET}-iwyu
            COMMAND
                ${CMAKE_COMMAND} -E env IWYU_BINARY=${Iwyu_EXECUTABLE} ${IwyuTool_EXECUTABLE} -p
                ${CMAKE_BINARY_DIR}/compile_commands.json ${target_sources_absolute} -- -Xiwyu
                --quoted_includes_first -Xiwyu --cxx17ns -Xiwyu --no_fwd_decls
                ${iwyu_mapping_files_args} ${iwyu_extra_args}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            USES_TERMINAL
            COMMENT "Analyzing code by 'include-what-you-use'"
        )
        add_dependencies(${ARG_ANALYSIS_TARGET} ${ARG_TARGET}-iwyu)
    endif()
endfunction()
