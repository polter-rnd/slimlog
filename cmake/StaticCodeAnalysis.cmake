# [cmake_documentation] StaticCodeAnalysis.cmake
#
# Enables static code analysis (clang-tidy, cppcheck, include-what-you-use) for specified target.
# Usage example:
#
# ~~~{.cmake}
# include(StaticCodeAnalysis)
# target_enable_static_analysis()
# ~~~
#
# Defines the following functions:
# - @ref target_enable_static_analysis
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

# [cmake_documentation] target_enable_static_analysis()
#
# Enable static analysis features for a specific target.
# Have to be called before adding affected targets.
#
# Required arguments:
# @arg __targetName__: Name of the target for which analyzers should be enabled
#
# @param CPPCHECK_EXTRA_ARGS         Additional flags for `cppcheck`
# @param CLANG_TIDY_EXTRA_ARGS       Additional arguments for `clang-tidy`
# @param IWYU_EXTRA_ARGS             Additional arguments for `include-what-you-use`
# [/cmake_documentation]
function(target_enable_static_analysis targetName)
    set(options "")
    set(oneValueArgs "")
    set(multipleValueArgs CPPCHECK_EXTRA_ARGS CLANG_TIDY_EXTRA_ARGS IWYU_EXTRA_ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    get_target_compilers(${targetName} target_compilers target_languages)
    list(LENGTH target_compilers num_compilers)
    if(num_compilers EQUAL 0)
        # If the target is compiled by no known compiler, give a warning.
        message(WARNING "Static analyzers for target ${targetName} may not be "
                        "usable, because it uses no or an unknown compiler. "
                        "This is a false warning for targets using only object lib(s) as input."
        )
    endif()

    if(ANALYZE_CPPCHECK)
        foreach(language ${target_languages})
            if(language MATCHES "^C|CXX$")
                set_property(
                    TARGET ${targetName} PROPERTY ${language}_CPPCHECK ${Cppcheck_EXECUTABLE}
                                                  ${ARG_CPPCHECK_EXTRA_ARGS}
                )
            endif()
        endforeach()
    endif()

    if(ANALYZE_CLANG_TIDY)
        foreach(language ${target_languages})
            if(language MATCHES "^C|CXX|OBJC|OBJCXX$")
                set_property(
                    TARGET ${targetName} PROPERTY ${language}_CLANG_TIDY ${ClangTidy_EXECUTABLE}
                                                  ${ARG_CLANG_TIDY_EXTRA_ARGS}
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
        foreach(language ${target_languages})
            if(language MATCHES "^C|CXX$")
                set_property(
                    TARGET ${targetName}
                    PROPERTY ${language}_INCLUDE_WHAT_YOU_USE ${Iwyu_EXECUTABLE} -Xiwyu --error=1
                             ${iwyu_extra_args}
                )
            endif()
        endforeach()
    endif()
endfunction()
