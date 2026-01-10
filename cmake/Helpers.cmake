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
# If package is found, the following variables are propagated to parent scope:
#
# - `\${package}_FOUND`: whether the package was found;
# - `\${package}_VERSION`: full provided version string;
# - `\${package}_VERSION_MAJOR`: major version if provided, else 0;
# - `\${package}_VERSION_MINOR`: minor version if provided, else 0;
# - `\${package}_VERSION_PATCH`: patch version if provided, else 0;
# - `\${package}_VERSION_TWEAK`: tweak version if provided, else 0;
# - `\${package}_VERSION_COUNT`: number of version components, 0 to 4.
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
    set_package_properties(
        ${package} PROPERTIES
        TYPE RECOMMENDED
        PURPOSE ${ARG_PURPOSE}
        DESCRIPTION ${ARG_DESCRIPTION}
    )

    if(NOT DEFINED ${ARG_OPTION})
        find_package(${package} ${ARG_MIN_VERSION} QUIET)
        if(${package}_FOUND AND ARG_DEFAULT)
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
        if(${ARG_OPTION})
            find_package(${package} ${ARG_MIN_VERSION} REQUIRED)
        endif()
        option(${ARG_OPTION} ${ARG_PURPOSE} ${${ARG_OPTION}})
    endif()
    if(${package}_FOUND)
        # Expand version variables visibility to parent scope
        foreach(
            suffix
            FOUND
            VERSION
            VERSION_MAJOR
            VERSION_MINOR
            VERSION_PATCH
            VERSION_TWEAK
            VERSION_COUNT
        )
            set(${package}_${suffix}
                ${${package}_${suffix}}
                PARENT_SCOPE
            )
        endforeach()
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
    get_cmake_property(var_names VARIABLES)
    list(SORT var_names)
    list(REMOVE_DUPLICATES var_names)
    foreach(var_name ${var_names})
        if(filter)
            # cmake-format: off
            get_property(_var_type CACHE ${var_name} PROPERTY TYPE)
            get_property(_is_advanced CACHE ${var_name} PROPERTY ADVANCED)
            # cmake-format: on
            if("${_var_type}" STREQUAL "INTERNAL"
               OR "${_var_type}" STREQUAL "UNINITIALIZED"
               OR _is_advanced
            )
                continue()
            endif()

            unset(MATCHED)
            string(REGEX MATCH ${filter} matched ${var_name})
            if(NOT matched)
                continue()
            endif()
        endif()
        message(" * ${var_name}=${${var_name}}")
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

# [cmake_documentation] set_versioned_compiler_names(package)
#
# Appends compiler version to names, e.g. for Clang 15 it will make
# `clang-format` -> `clang-format-15`.
# If languages specified, will check compiler version only for those languages
# and take the maximum version if there are multiple versions of the same compiler.
#
# ~~~
# set_versioned_compiler_names(ClangFormat COMPILER Clang NAMES clang-format)
# message("Versioned cmake-format: ${ClangFormat_NAMES}")
# ~~~
#
# Required arguments:
# @arg __package__: Package name (result will be stored in `package`_NAMES variable).
#
# @param COMPILER Name of the compiler which vesion to append (e.g. `Clang`)
# @param LANGS Languages to check the compiler for (e.g. `C CXX`)
# @param NAMES Input binary names, (e.g. `clang-format`)
# [/cmake_documentation]
function(set_versioned_compiler_names package)
    set(options "")
    set(oneValueArgs COMPILER)
    set(multipleValueArgs LANGS NAMES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    if(NOT ARG_COMPILER)
        get_property(ARG_LANGS GLOBAL PROPERTY ENABLED_LANGUAGES)
    endif()

    if(NOT ARG_LANGS)
        get_property(ARG_LANGS GLOBAL PROPERTY ENABLED_LANGUAGES)
    endif()

    set(${package}_NAMES
        ""
        PARENT_SCOPE
    )
    set(max_compiler_ver -1)
    foreach(lang ${ARG_LANGS})
        if(CMAKE_${lang}_COMPILER_ID STREQUAL "${ARG_COMPILER}")
            set(compiler_ver ${CMAKE_${lang}_COMPILER_VERSION})
            string(REGEX REPLACE "([0-9]+)\\..*" "\\1" compiler_ver ${compiler_ver})
            if(compiler_ver GREATER max_compiler_ver)
                set(max_compiler_ver ${compiler_ver})
            endif()
        endif()
    endforeach()
    if(max_compiler_ver GREATER -1)
        list(TRANSFORM ARG_NAMES APPEND -${max_compiler_ver} OUTPUT_VARIABLE ${package}_NAMES)
        set(${package}_NAMES
            ${${package}_NAMES}
            PARENT_SCOPE
        )
    endif()
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
# message("Is address sanitizer supported for C++: \${supported_flag}")
# ~~~
# [/cmake_documentation]
function(check_compiler_flags flag lang variable)
    unset(${variable} CACHE)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "${flag}")
    if(lang STREQUAL "C")
        include(CheckCCompilerFlag)
        check_c_compiler_flag("" ${variable})
    elseif(lang STREQUAL "CXX")
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("" ${variable})
    elseif(lang STREQUAL "Fortran")
        include(CheckFortranCompilerFlag)
        check_fortran_compiler_flag("" ${variable})
    elseif(NOT CMAKE_REQUIRED_QUIET)
        message(STATUS "Language ${lang} is not supported for checking compiler flags")
    endif()
    cmake_pop_check_state()
endfunction()

# [cmake_documentation] get_lang_of_source(fileName, variable)
#
# Helper function to get the language of a source file based on extension.
#
# Example:
#
# ~~~
# get_lang_of_source("main.cpp", main_cpp_lang)
# message("Language of main.cpp: \${main_cpp_lang}")
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
# message("Compilers used in myprogram: \${compilers_list}")
# message("Languages used in myprogram: \${langs_list}")
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
# - `varPrefix`_COMPILERS (e.g. `ASan_COMPILERS`)
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
# check_compiler_flags_list(\${candidates}, Sanitizer, ASan)
# message("Flags for Clang compiler (if supported): \${ASan_Clang_FLAGS}")
# message("Flags for GNU compiler (if supported): \${ASan_GNU_FLAGS}")
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
        if(NOT compiler)
            continue()
        endif()
        if(NOT DEFINED ${varPrefix}_${compiler}_FLAGS)
            foreach(flags ${flagCandidates})
                # Normalize flags by subsequent whitespace with single spaces
                string(REGEX REPLACE "[ \t\n]+" " " flags "${flags}")
                string(STRIP "${flags}" flags)

                if(NOT CMAKE_REQUIRED_QUIET)
                    message(STATUS "Try ${compiler} ${featureName} flags [${flags}]")
                endif()

                check_compiler_flags("${flags}" ${lang} ${varPrefix}_${compiler}_DETECTED)
                if(${varPrefix}_${compiler}_DETECTED)
                    set(${varPrefix}_${compiler}_FLAGS
                        "${flags}"
                        CACHE STRING "${featureName} flags for ${compiler} compiler."
                    )
                    mark_as_advanced(${varPrefix}_${compiler}_FLAGS)
                    break()
                endif()
            endforeach()

            if(NOT ${varPrefix}_${compiler}_DETECTED)
                set(${varPrefix}_${compiler}_FLAGS
                    ""
                    CACHE STRING "${featureName} flags for ${compiler} compiler."
                )
                mark_as_advanced(${varPrefix}_${compiler}_FLAGS)

                if(NOT CMAKE_REQUIRED_QUIET)
                    message(STATUS "${featureName} is NOT available for ${compiler} compiler.")
                endif()
            endif()
        endif()
        if(${varPrefix}_${compiler}_FLAGS AND NOT ${compiler} IN_LIST ${varPrefix}_COMPILERS)
            list(APPEND ${varPrefix}_COMPILERS ${compiler})
            set(${varPrefix}_COMPILERS
                ${${varPrefix}_COMPILERS}
                PARENT_SCOPE
            )
        endif()
    endforeach()
endfunction()
