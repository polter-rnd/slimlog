# [cmake_documentation] Helpers.cmake
#
# Defines the following functions:
# - @ref find_package_switchable
# - @ref dump_option_variables
#
# Various helper functions and macros.
# [/cmake_documentation]

# [cmake_documentation] find_package_switchable(package)
#
# The function function finds a package toggled by option.
#
# Use this when you need switch option for particular feature depending on package.
# Without `OPTION` and `DEFAULT` it just calls set_package_properties() and find_package().
# `DEFAULT` option can be used to specify default package state (e.g. `ON` or `OFF`).
# When `OPTION` is set (e.g. `OPTION ENABLE_MYPKG`), this function does the following:
#
# - `-DENABLE_MYPKG=` flag is not set or empty: treats package as recommended
#                     and switches to default state if it was found;
# - `-DENABLE_MYPKG=ON`: marks package as required and fails if it is not found;
# - `-DENABLE_MYPKG=OFF`: disables package and does not call find_package().
#
# ~~~{.cmake}
# include(Helpers)
# find_package_switchable(MyPackage
#     OPTION DENABLE_MYPKG
#     PURPOSE "Some features provided by MyPackage"
# )
# find_package_switchable(AnotherPackage
#     OPTION ENABLE_ANOTHER_PKG
#     PURPOSE "Features provided by AnotherPackage"
# )
# ~~~
#
# Required arguments:
# @arg __package__: Required package name
#
# @param OPTION      Name of the flag to toggle feature on/off, i.e. `ENABLE_MYPKG`
# @param DEFAULT     Default feature state if switch option is not specified
# @param PURPOSE     Which features this package enables in the project
# @param DESCRIPTION A short description of package
# [/cmake_documentation]
function(find_package_switchable package)
    set(options "")
    set(oneValueArgs OPTION DEFAULT PURPOSE DESCRIPTION)
    set(multipleValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    if(NOT package)
        message(FATAL_ERROR "find_package_switchable(): missing package argument")
    endif()
    if(NOT DEFINED ARG_DEFAULT)
        set(ARG_DEFAULT ON)
    elseif(NOT DEFINED ARG_OPTION)
        message(FATAL_ERROR "find_package_switchable(): got DEFAULT argument without OPTION")
    endif()

    include(FeatureSummary)
    # cmake-format: off
    set_package_properties(${package}
        PROPERTIES
            TYPE RECOMMENDED
            PURPOSE ${ARG_PURPOSE}
        DESCRIPTION ${ARG_DESCRIPTION}
    )
    # cmake-format: on

    if(NOT DEFINED ${ARG_OPTION})
        find_package(${package})
        if(${${package}_FOUND} AND ${ARG_DEFAULT})
            set(${ARG_OPTION}
                ON
                PARENT_SCOPE
            )
        else()
            set(${ARG_OPTION}
                OFF
                PARENT_SCOPE
            )
        endif()
    else()
        if(${${ARG_OPTION}})
            find_package(${package} REQUIRED)
        endif()
        option(${ARG_OPTION} ${ARG_PURPOSE} ${${ARG_OPTION}})
    endif()
endfunction()

# [cmake_documentation] dump_option_variables(filter)
#
# Dumps all options except `INTERNAL` and `UNINITIALIZED` types and without `ADVANCED` property.
# Can be filtered by regex:
#
# ~~~
# dump_option_variables("^ENABLE_")
# ~~~
#
# Optional arguments:
# @arg __filter__: Regular expression to filter variables
# [/cmake_documentation]
function(dump_option_variables filter)
    get_cmake_property(_VAR_NAMES VARIABLES)
    list(SORT _VAR_NAMES)
    foreach(_VAR_NAME ${_VAR_NAMES})
        if(filter)
            # cmake-format: off
            get_property(_VAR_TYPE CACHE ${_VAR_NAME} PROPERTY TYPE)
            get_property(_IS_ADVANCED CACHE ${_VAR_NAME} PROPERTY ADVANCED)
            # cmake-format: on
            if("${_VAR_TYPE}" STREQUAL "INTERNAL"
               OR "${_VAR_TYPE}" STREQUAL "UNINITIALIZED"
               OR _IS_ADVANCED
            )
                continue()
            endif()

            unset(MATCHED)

            # Case insenstitive match
            string(TOLOWER "${filter}" filter_lower)
            string(TOLOWER "${_VAR_NAME}" _VAR_NAME_lower)
            string(REGEX MATCH ${filter_lower} MATCHED ${_VAR_NAME_lower})

            if(NOT MATCHED)
                continue()
            endif()
        endif()
        message(" * ${_VAR_NAME}=${${_VAR_NAME}}")
    endforeach()
endfunction()
