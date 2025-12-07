# [cmake_documentation] StaticCodeAnalysis.cmake
#
# Enables static code analysis (clang-tidy, cppcheck, include-what-you-use) for specified target.
# Usage example:
#
# ~~~{.cmake}
# set(TARGET hello)
# add_executable(hello main.cpp)
# include(StaticCodeAnalysis)
# detect_available_static_analysers()
# target_enable_static_analysis(hello CLANG_TIDY ON
#                                     IWYU       ON
#                                     CPPCHECK   ON)
# ~~~
#
# Defines the following functions:
# - @ref detect_available_static_analysers
# - @ref target_enable_static_analysis
# [/cmake_documentation]

# [cmake_documentation] detect_available_static_analysers()
#
# Detects available static code analyzers and sets corresponding options to toggle them.
# Default option names are:
# - ANALYZE_CPPCHECK:     Enable `cppcheck` analyzer
# - ANALYZE_CLANG_TIDY:   Enable `clang-tidy` analyzer
# - ANALYZE_IWYU:         Enable `include-what-you-use` analyzer
#
# @param CPPCHECK_OPTION         Name of the flag for detection `cppcheck` analyzer
# @param CPPCHECK_MIN_VERSION    Minimum required version for `cppcheck`
# @param CLANG_TIDY_OPTION       Name of the flag for detection `clang-tidy` analyzer
# @param CLANG_TIDY_MIN_VERSION  Minimum required version for `clang-tidy`
# @param IWYU_OPTION             Name of the flag for detection `include-what-you-use` analyzer
# @param IWYU_MIN_VERSION        Minimum required version for `include-what-you-use`
# [/cmake_documentation]
function(detect_available_static_analysers)
    set(options "")
    set(oneValueArgs CPPCHECK_OPTION CLANG_TIDY_OPTION IWYU_OPTION CPPCHECK_MIN_VERSION
                     CLANG_TIDY_MIN_VERSION IWYU_MIN_VERSION
    )
    set(multipleValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    if(NOT ARG_CLANG_TIDY_OPTION)
        set(ARG_CLANG_TIDY_OPTION ANALYZE_CLANG_TIDY)
    endif()
    if(NOT ARG_IWYU_OPTION)
        set(ARG_IWYU_OPTION ANALYZE_IWYU)
    endif()
    if(NOT ARG_CPPCHECK_OPTION)
        set(ARG_CPPCHECK_OPTION ANALYZE_CPPCHECK)
    endif()

    include(Helpers)
    find_package_switchable(
        Cppcheck
        OPTION ${ARG_CPPCHECK_OPTION}
        PURPOSE "Basic static analysis of C/C++ code"
        MIN_VERSION ${ARG_CPPCHECK_MIN_VERSION}
    )
    find_package_switchable(
        ClangTidy
        OPTION ${ARG_CLANG_TIDY_OPTION}
        PURPOSE "Clang-based analysis and linting of C/C++ code"
        MIN_VERSION ${ARG_CLANG_TIDY_MIN_VERSION}
    )
    find_package_switchable(
        Iwyu
        OPTION ${ARG_IWYU_OPTION}
        PURPOSE "Analyze includes in C and C++ source files"
        MIN_VERSION ${ARG_IWYU_MIN_VERSION}
    )

    set(${ARG_CPPCHECK_OPTION}
        ${${ARG_CPPCHECK_OPTION}}
        PARENT_SCOPE
    )
    set(${ARG_CLANG_TIDY_OPTION}
        ${${ARG_CLANG_TIDY_OPTION}}
        PARENT_SCOPE
    )
    set(${ARG_IWYU_OPTION}
        ${${ARG_IWYU_OPTION}}
        PARENT_SCOPE
    )
endfunction()

# [cmake_documentation] target_enable_static_analysis()
#
# Enable static analysis features for a specific target.
# Have to be called before adding affected targets.
#
# Required arguments:
# @arg __targetName__: Name of the target for which analyzers should be enabled
#
# @param CPPCHECK                    Enable `cppcheck` analyzer
# @param CPPCHECK_EXTRA_ARGS         Additional flags for `cppcheck`
# @param CLANG_TIDY                  Enable `clang-tidy` analyzer
# @param CLANG_TIDY_EXTRA_ARGS       Additional arguments for `clang-tidy`
# @param IWYU                        Enable `include-what-you-use` analyzer
# @param IWYU_EXTRA_ARGS             Additional arguments for `include-what-you-use`
# [/cmake_documentation]
function(target_enable_static_analysis targetName)
    set(options "")
    set(oneValueArgs CPPCHECK CLANG_TIDY IWYU)
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

    if(ARG_CPPCHECK)
        foreach(language ${target_languages})
            if(language MATCHES "^C|CXX$")
                set_property(
                    TARGET ${targetName} PROPERTY ${language}_CPPCHECK ${Cppcheck_EXECUTABLE}
                                                  ${ARG_CPPCHECK_EXTRA_ARGS}
                )
            endif()
        endforeach()
    endif()

    if(ARG_CLANG_TIDY)
        foreach(language ${target_languages})
            if(language MATCHES "^C|CXX|OBJC|OBJCXX$")
                set_property(
                    TARGET ${targetName} PROPERTY ${language}_CLANG_TIDY ${ClangTidy_EXECUTABLE}
                                                  ${ARG_CLANG_TIDY_EXTRA_ARGS}
                )
            endif()
        endforeach()
    endif()

    if(ARG_IWYU)
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
