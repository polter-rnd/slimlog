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
# @param MIN_VERSION Minimum requireed package version
# @param PURPOSE     Which features this package enables in the project
# @param DESCRIPTION A short description of package
# [/cmake_documentation]
function(find_package_switchable package)
    set(options "")
    set(oneValueArgs OPTION DEFAULT PURPOSE DESCRIPTION MIN_VERSION)
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
        find_package(${package} ${ARG_MIN_VERSION})
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
            find_package(${package} ${ARG_MIN_VERSION} REQUIRED)
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
    get_cmake_property(_var_names VARIABLES)
    list(SORT _var_names)
    foreach(_var_name ${_var_names})
        if(filter)
            # cmake-format: off
            get_property(_var_type CACHE ${_var_name} PROPERTY TYPE)
            get_property(_is_advanced CACHE ${_var_name} PROPERTY ADVANCED)
            # cmake-format: on
            if("${_var_type}" STREQUAL "INTERNAL"
               OR "${_var_type}" STREQUAL "UNINITIALIZED"
               OR _is_advanced
            )
                continue()
            endif()

            # Case insenstitive match
            string(TOLOWER "${filter}" filter_lower)
            string(TOLOWER "${_var_name}" _var_name_lower)

            unset(MATCHED)
            string(REGEX MATCH ${filter_lower} matched ${_var_name_lower})
            if(NOT matched)
                continue()
            endif()
        endif()
        message(" * ${_var_name}=${${_var_name}}")
    endforeach()
endfunction()

# [cmake_documentation] set_directory_hints(package)
#
# Iterates over hint variable names passed by `HINTS` option
# and checks if there is environment or CMake variable with that name
# and if it is a valid directory, then writes it to `package_HINTS`.
#
# Finally you get a list of hint directories set to `package_HINTS`. Example:
#
# ~~~
# set_directory_hints(ClangFormat HINTS LLVM_DIR LLVM_ROOT)
# message("HINTS: ${ClangFormat_HINTS}")
# ~~~
#
# Required arguments:
# @arg __package__: Package name (result will be stored in `package`_HINTS variable).
#
# @param HINTS All available paths to check
# [/cmake_documentation]
function(set_directory_hints package)
    set(options "")
    set(oneValueArgs "")
    set(multipleValueArgs HINTS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    set(${package}_HINTS
        ""
        PARENT_SCOPE
    )
    foreach(hint ${ARG_HINTS})
        if(DEFINED ${hint})
            if((EXISTS "${${hint}}") AND (IS_DIRECTORY "${${hint}}"))
                set(${package}_HINTS
                    ${${package}_HINTS} "${${hint}}"
                    PARENT_SCOPE
                )
            endif()
        endif()
        if(DEFINED ENV{${hint}})
            if((EXISTS "$ENV{${hint}}") AND (IS_DIRECTORY "$ENV{${hint}}"))
                set(${package}_HINTS
                    ${${package}_HINTS} "$ENV{${hint}}"
                    PARENT_SCOPE
                )
            endif()
        endif()
    endforeach()
endfunction()

# [cmake_documentation] check_compiler_flag(flat, lang, variable)
#
# Helper function to check compiler flags for language compiler.
# It checks if `flag` is supported in the compiler of `lang` language,
# storing result in `variable`.
#
# Currently only `CXX`, `C` and `Fortran` languages are supported.
#
# Example:
#
# ~~~
# check_compiler_flags("-fsanitize=address" CXX supported_flag)
# message("Is address sanitizer supported for C++: ${supported_flag}")
# ~~~
# [/cmake_documentation]
function(check_compiler_flags flag lang variable)
    unset(${variable} CACHE)
    set(CMAKE_REQUIRED_FLAGS "${flag}")
    if(${lang} STREQUAL "C")
        include(CheckCCompilerFlag)
        check_c_compiler_flag("" ${variable})
    elseif(${lang} STREQUAL "CXX")
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("" ${variable})
    elseif(${lang} STREQUAL "Fortran")
        include(CheckFortranCompilerFlag)
        check_fortran_compiler_flag("" ${variable})
    elseif(NOT CMAKE_REQUIRED_QUIET)
        message(STATUS "Language ${lang} is not supported for checking compiler flags")
    endif()
endfunction()

# [cmake_documentation] get_lang_of_source(fileName, variable)
#
# Helper function to get the language of a source file based on extension.
#
# Example:
#
# ~~~
# get_lang_of_source("main.cpp", main_cpp_lang)
# message("Language of main.cpp: ${main_cpp_lang}")
# ~~~
#
# Required arguments:
# @arg __fileName__: Source file name
# @arg __variable__: Variable name for result
# [/cmake_documentation]
function(get_lang_of_source fileName variable)
    get_filename_component(longest_ext "${fileName}" EXT)
    # If extension is empty return. This can happen for extensionless headers
    if("${longest_ext}" STREQUAL "")
        set(${variable}
            ""
            PARENT_SCOPE
        )
        return()
    endif()
    # Get shortest extension as some files can have dot in their names
    string(REGEX REPLACE "^.*(\\.[^.]+)$" "\\1" file_ext ${longest_ext})
    string(TOLOWER "${file_ext}" file_ext)
    string(SUBSTRING "${file_ext}" 1 -1 file_ext)

    get_property(enabled_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    foreach(lang ${enabled_languages})
        list(FIND CMAKE_${lang}_SOURCE_FILE_EXTENSIONS "${file_ext}" _temp)
        if(NOT ${_temp} EQUAL -1)
            set(${variable}
                "${lang}"
                PARENT_SCOPE
            )
            return()
        endif()
    endforeach()

    set(${variable}
        ""
        PARENT_SCOPE
    )
endfunction()

# [cmake_documentation] get_target_compilers(targetName, compilersVar, langsVar)
#
# Helper function to get compilers and languages used by a target.
#
# Example:
#
# ~~~
# get_target_compilers("myprogram", compilers_list, langs_list)
# message("Compilers used in myprogram: ${compilers_list}")
# message("Languages used in myprogram: ${langs_list}")
# ~~~
#
# Required arguments:
# @arg __targetName__: Target name
# @arg __compilersVar__: Variable name for the list of compilers used
# @arg __langsVar__: Variable name for the list of languages used
# [/cmake_documentation]
function(get_target_compilers targetName compilersVar langsVar)
    set(bufferCompilers "")
    set(bufferLangs "")

    get_target_property(target_sources ${targetName} SOURCES)
    foreach(file_name ${target_sources})
        # If expression was found, file_name is a generator-expression for an object library. Object
        # libraries will be ignored.
        string(REGEX MATCH "TARGET_OBJECTS:([^ >]+)" _file ${file_name})
        if("${_file}" STREQUAL "")
            get_lang_of_source(${file_name} file_lang)
            if(file_lang)
                list(APPEND bufferLangs ${file_lang})
                list(APPEND bufferCompilers ${CMAKE_${file_lang}_COMPILER_ID})
            endif()
        endif()
    endforeach()

    list(REMOVE_DUPLICATES bufferCompilers)
    set(${compilersVar}
        "${bufferCompilers}"
        PARENT_SCOPE
    )

    list(REMOVE_DUPLICATES bufferLangs)
    set(${langsVar}
        "${bufferLangs}"
        PARENT_SCOPE
    )
endfunction()

# [cmake_documentation] check_compiler_flags_list(flagCandidates, featureName, varPrefix)
#
# Helper function to test list of compiler flags.
# Use it for searching for supported flag combination.
#
# For each used compiler, may set the following variables:
#
# - `varPrefix`_`compiler`_DETECTED (e.g. `ASan_Clang_DETECTED`)
# - `varPrefix`_`compiler`_FLAGS (e.g. `ASan_Clang_FLAGS`)
#
# Example:
#
# ~~~
# set(candidates
#     # GNU/Clang
#     "-g -fsanitize=address"
#     # MSVC uses
#     "/fsanitize=address"
# )
# check_compiler_flags_list(${candidates}, Sanitizer, ASan)
# message("Flags for Clang compiler (if supported): ${ASan_Clang_FLAGS}")
# message("Flags for GNU compiler (if supported): ${ASan_GNU_FLAGS}")
# ~~~
#
# Required arguments:
# @arg __flagCandidates__: List of flags to check
# @arg __featureName__: Feature that is enabled by flags (just for logging)
# @arg __varPrefix__: Prefix for resulting variables
# [/cmake_documentation]
function(check_compiler_flags_list flagCandidates featureName varPrefix)
    set(CMAKE_REQUIRED_QUIET ${${varPrefix}_FIND_QUIETLY})

    get_property(enabled_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    foreach(lang ${enabled_languages})
        # Sanitizer flags are not dependend on language, but the used compiler. So instead of
        # searching flags foreach language, search flags foreach compiler used.
        set(compiler ${CMAKE_${lang}_COMPILER_ID})
        if(compiler AND NOT DEFINED ${varPrefix}_${compiler}_FLAGS)
            foreach(flags ${flagCandidates})
                if(NOT CMAKE_REQUIRED_QUIET)
                    message(STATUS "Try ${compiler} ${featureName} flags = [${flags}]")
                endif()

                check_compiler_flags("${flags}" ${lang} ${varPrefix}_DETECTED)
                if(${varPrefix}_DETECTED)
                    set(${varPrefix}_${compiler}_FLAGS
                        "${flags}"
                        CACHE STRING "${featureName} flags for ${compiler} compiler."
                    )
                    mark_as_advanced(${varPrefix}_${compiler}_FLAGS)
                    break()
                endif()
            endforeach()

            if(NOT ${varPrefix}_DETECTED)
                set(${varPrefix}_${compiler}_FLAGS
                    ""
                    CACHE STRING "${featureName} flags for ${compiler} compiler."
                )
                mark_as_advanced(${varPrefix}_${compiler}_FLAGS)

                message(STATUS "${featureName} is not available for ${compiler} "
                               "compiler. Targets using this compiler will be "
                               "compiled without ${featureName}."
                )
            endif()
        endif()
    endforeach()
endfunction()
