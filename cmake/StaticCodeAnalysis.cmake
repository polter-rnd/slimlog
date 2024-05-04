# [cmake_documentation] StaticCodeAnalysis.cmake
#
# Defines the following functions:
# - @ref enable_static_code_analysis
#
# Creates target for static code analysis using various tools.
# Usage example:
#
# ~~~{.cmake}
# include(StaticCodeAnalysis)
# enable_static_code_analysis()
# ~~~
#
# Uses the following parameters:
# @arg __ANALYZE_CPPCHECK__:       Enable basic static analysis of C/C++ code
# @arg __CPPCHECK_MIN_VERSION__:   Minimum required version for `cppcheck`
# @arg __ANALYZE_CLANG_TIDY__:     Enable clang-based analysis and linting of C/C++ code
# @arg __CLANG_TIDY_MIN_VERSION__: Minimum required version for `clang-tidy`
# @arg __ANALYZE_IWYU__:           Enable include file analysis in C and C++ source files
# @arg __IWYU_MIN_VERSION__:       Minimum required version for `include-what-you-use`
# [/cmake_documentation]

include(Helpers)
find_package_switchable(
    Cppcheck
    OPTION ANALYZE_CPPCHECK
    PURPOSE "Basic static analysis of C/C++ code"
    MIN_VERSION ${CPPCHECK_MIN_VERSION}
)
find_package_switchable(
    ClangTidy
    OPTION ANALYZE_CLANG_TIDY
    PURPOSE "Clang-based analysis and linting of C/C++ code"
    MIN_VERSION ${CLANG_TIDY_MIN_VERSION}
)
find_package_switchable(
    Iwyu
    OPTION ANALYZE_IWYU
    PURPOSE "Analyze includes in C and C++ source files"
    MIN_VERSION ${IWYU_MIN_VERSION}
)

# Reset all analyzers by default
foreach(language C CXX OBJC OBJCXX)
    unset(CMAKE_${language}_CPPCHECK CACHE)
    unset(CMAKE_${language}_CLANG_TIDY CACHE)
    unset(CMAKE_${language}_INCLUDE_WHAT_YOU_USE CACHE)
endforeach()

# [cmake_documentation] enable_static_code_analysis()
#
# Enable static analysis features for all targets.
# Have to be called before adding affected targets.
#
# @param CPPCHECK_EXTRA_ARGS         Additional flags for `cppcheck`
# @param CLANG_TIDY_EXTRA_ARGS       Additional arguments for `clang-tidy`
# @param IWYU_EXTRA_ARGS             Additional arguments for `include-what-you-use`
# [/cmake_documentation]
function(enable_static_code_analysis)
    set(options "")
    set(oneValueArgs "")
    set(multipleValueArgs CPPCHECK_EXTRA_ARGS CLANG_TIDY_EXTRA_ARGS IWYU_EXTRA_ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    get_property(enabled_languages GLOBAL PROPERTY ENABLED_LANGUAGES)

    if(ANALYZE_CPPCHECK)
        find_program(Cppcheck_EXECUTABLE NAMES cppcheck REQUIRED)
        foreach(language ${enabled_languages})
            if(language MATCHES "^C|CXX$")
                set(CMAKE_${language}_CPPCHECK
                    ${Cppcheck_EXECUTABLE}
                    ${ARG_CPPCHECK_EXTRA_ARGS}
                    CACHE STRING ""
                )
            endif()
        endforeach()
    endif()

    if(ANALYZE_CLANG_TIDY)
        if(ARG_CLANG_TIDY_EXTRA_ARGS)
            set(clang_tidy_extra_args "")
            foreach(extra_arg ${ARG_CLANG_TIDY_EXTRA_ARGS})
                list(APPEND clang_tidy_extra_args -extra-arg=${extra_arg})
            endforeach()
        endif()
        foreach(language ${enabled_languages})
            if(language MATCHES "^C|CXX|OBJC|OBJCXX$")
                set(CMAKE_${language}_CLANG_TIDY
                    ${ClangTidy_EXECUTABLE} ${clang_tidy_extra_args}
                    CACHE STRING ""
                )
            endif()
        endforeach()
    endif()

    if(ANALYZE_IWYU)
        if(ARG_IWYU_EXTRA_ARGS)
            set(iwyu_extra_args "")
            foreach(extra_arg ${ARG_IWYU_EXTRA_ARGS})
                list(APPEND iwyu_extra_args -Xiwyu ${extra_arg})
            endforeach()
        endif()
        foreach(language ${enabled_languages})
            if(language MATCHES "^C|CXX$")
                set(CMAKE_${language}_INCLUDE_WHAT_YOU_USE
                    ${Iwyu_EXECUTABLE}
                    -Xiwyu
                    --error=1
                    -Xiwyu
                    --check_also=${PROJECT_SOURCE_DIR}/*
                    ${iwyu_mapping_files_args}
                    ${iwyu_extra_args}
                    CACHE STRING ""
                )
            endif()
        endforeach()
    endif()
endfunction()
