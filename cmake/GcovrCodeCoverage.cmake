# [cmake_documentation] GcovrCodeCoverage.cmake
#
# Enables code coverage report generation with Gcovr:
#
# ~~~{.cmake}
# include(GcovrCodeCoverage)
# add_gcovr_coverage_target(
#     HTML COBERTURA COVERALLS SONARQUBE
#     COVERAGE_TARGET coverage COVERAGE_INIT_TARGET coverage-init
# )
# target_enable_coverage(my_target)
# ~~~
#
# In example above, will be created the following targets:
# - __coverage-init__:  Reset coverage counters to zero
# - __coverage__:       Generate coverage report assuming that tests were already run
#
# Defines the following functions:
# - @ref add_gcovr_coverage_target
# - @ref target_enable_coverage
#
# The module defines the following variables:
# @arg __COVERAGE_coverage_flags__: Compiler flags to enable code coverage support
# [/cmake_documentation]

# [cmake_documentation] add_gcovr_coverage_target()
#
# The function creates targets for coverage report generation.
#
# Options for different report types (can be specified together):
# @arg __HTML__:  Generate HTML report (`\${OUTPUT_DIRECTORY}/html`)
# @arg __COBERTURA__:  Generate HTML report (`\${OUTPUT_DIRECTORY}/cobertura.xml`)
# @arg __COVERALLS__:  Generate HTML report (`\${OUTPUT_DIRECTORY}/coveralls.xml`)
# @arg __SONARQUBE__:  Generate HTML report (`\${OUTPUT_DIRECTORY}/sonarqube.json`)
#
# @param COVERAGE_TARGET      Coverage target name (mandatody)
# @param COVERAGE_INIT_TARGET Coverage counters reset target name (mandatory)
# @param CHECK_TARGET         Test target name. If specified, `COVERAGE_TARGET`
#                             will be dependent on it.
# @param OUTPUT_DIRECTORY     Path to output dir
#                             (default: `\${PROJECT_BINARY_DIR}/coverage`)
# @param GCOV_LANGUAGES       List of languages for which coverage should be generated
#                             (default: `\${PROJECT_LANGUAGES}`)
# @param GCOVR_FILTER         List of patterns to include to coverage
# @param GCOVR_EXCLUDE        List of patterns to exclude from coverage
# @param GCOVR_OPTIONS        List of extra options for `gcovr`
# [/cmake_documentation]
function(add_gcovr_coverage_target)
    set(options HTML COBERTURA COVERALLS SONARQUBE)
    set(oneValueArgs COVERAGE_TARGET COVERAGE_INIT_TARGET CHECK_TARGET OUTPUT_DIRECTORY)
    set(multiValueArgs GCOV_LANGUAGES GCOVR_EXCLUDE GCOVR_FILTER GCOVR_OPTIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_COVERAGE_TARGET)
        message(FATAL_ERROR "GcovrCodeCoverage target name is not specified!")
    endif()

    if(NOT ARG_COVERAGE_INIT_TARGET)
        message(FATAL_ERROR "GcovrCodeCoverage init target name is not specified!")
    endif()

    if(NOT ARG_GCOV_LANGUAGES)
        get_property(ARG_GCOV_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
    endif()

    if(NOT ARG_OUTPUT_DIRECTORY)
        set(ARG_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/coverage)
    endif()

    foreach(language ${ARG_GCOV_LANGUAGES})
        set(_language_compiler ${CMAKE_${language}_COMPILER_ID})
        if(NOT compiler_id)
            set(compiler_id ${_language_compiler})
            set(compiler_version CMAKE_${language}_COMPILER_VERSION)
        elseif(NOT compiler_id STREQUAL _language_compiler)
            message(WARNING "Cannot enable coverage for multiple compilers! "
                            "Please set GCOV_EXECUTABLE or fill GCOV_LANGUAGES "
                            "only with languages using the same compiler."
            )
            return()
        endif()
    endforeach()

    if(compiler_version)
        string(REGEX REPLACE "([0-9]+)\\..*" "\\1" compiler_version_major ${compiler_version})
    endif()

    if(compiler_id MATCHES "Clang")
        set_directory_hints(LlvmCov HINTS LLVM_DIR LLVM_ROOT)
        find_program(
            LlvmCov_EXECUTABLE
            NAMES llvm-cov-${compiler_version_major} llvm-cov
            HINTS ${LlvmCov_HINTS}
            DOC "llvm-cov executable"
        )
        set(ARG_GCOV_EXECUTABLE "${LlvmCov_EXECUTABLE} gcov")
    elseif(compiler_id MATCHES "GNU")
        find_program(
            Gcov_EXECUTABLE
            NAMES gcov-${compiler_version_major} gcov
            DOC "gcov executable"
        )
        set(ARG_GCOV_EXECUTABLE "${Gcov_EXECUTABLE}")
    else()
        message(WARNING "Coverage supported only for GCC or Clang compilers")
        return()
    endif()

    set(gcovr_dirs --root ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
    set(gcovr_options --exclude-unreachable-branches --exclude-throw-branches --print-summary
                      --gcov-executable ${ARG_GCOV_EXECUTABLE} ${ARG_GCOVR_OPTIONS}
    )

    # Exclude paths
    list(APPEND gcovr_options --exclude "${PROJECT_BINARY_DIR}/*")
    if(ARG_GCOVR_EXCLUDE)
        foreach(exclude IN LISTS ARG_GCOVR_EXCLUDE)
            list(APPEND gcovr_options --exclude "${exclude}")
        endforeach()
    endif()

    # Include filter
    if(ARG_GCOVR_FILTER)
        foreach(filter IN LISTS ARG_GCOVR_FILTER)
            list(APPEND gcovr_options --filter "${filter}")
        endforeach()
    endif()

    # Gcovr is used to generate coverage reports
    find_package(Gcovr REQUIRED)

    set(gcovr_commands COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUTPUT_DIRECTORY})
    if(ARG_HTML)
        list(APPEND gcovr_options --html ${ARG_OUTPUT_DIRECTORY}/html/index.html --html-title
             "Code Coverage for ${PROJECT_NAME}"
        )
        if(${Gcovr_VERSION_STRING} VERSION_GREATER_EQUAL 6.0)
            list(APPEND gcovr_options --html-nested)
        else()
            list(APPEND gcovr_options --html-details)
        endif()
        list(
            APPEND
            gcovr_commands
            COMMAND
            ${CMAKE_COMMAND}
            -E
            make_directory
            ${ARG_OUTPUT_DIRECTORY}/html
        )
    endif()
    if(ARG_COBERTURA)
        list(APPEND gcovr_options --xml ${ARG_OUTPUT_DIRECTORY}/cobertura.xml)
    endif()
    if(ARG_SONARQUBE)
        list(APPEND gcovr_options --sonarqube ${ARG_OUTPUT_DIRECTORY}/sonarqube.json)
    endif()
    if(ARG_COVERALLS)
        list(APPEND gcovr_options --coveralls ${ARG_OUTPUT_DIRECTORY}/coveralls.xml)
    endif()

    list(APPEND gcovr_commands COMMAND ${Gcovr_EXECUTABLE} ${gcovr_options} ${gcovr_dirs})

    find_program(FIND_COMMAND find)

    add_custom_target(
        ${ARG_COVERAGE_INIT_TARGET}
        # Cleanup gcov counters
        COMMAND ${FIND_COMMAND} . -name *.gcda -delete
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${ARG_OUTPUT_DIRECTORY}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        VERBATIM
        COMMENT "Resetting code coverage counters."
    )

    add_custom_target(
        ${ARG_COVERAGE_TARGET}
        ${gcovr_commands}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        VERBATIM
        COMMENT "Processing code coverage counters and generating reports."
    )

    # As long as ${ARG_CHECK_TARGET} is not defined, calling ${ARG_COVERAGE_TARGET} will not execute
    # any tests (the `check`) but only generate reports. This assumes that the counters were
    # initialized and the tests executed manually before.
    if(TARGET ${ARG_CHECK_TARGET})
        add_dependencies(${ARG_COVERAGE_TARGET} ${ARG_CHECK_TARGET})
        add_dependencies(${ARG_CHECK_TARGET} ${ARG_COVERAGE_INIT_TARGET})
    endif()
endfunction()

# [cmake_documentation] target_enable_coverage(name)
#
# The function adds coverage compiler and linker flags only for particular target.
#
# Arguments:
# @arg __target__: Target name for which coverage should be enabled
# [/cmake_documentation]
function(target_enable_coverage targetName)
    include(Helpers)
    get_target_compilers(${targetName} target_compiler target_lang)
    list(LENGTH target_compiler num_compilers)
    if(NOT num_compilers EQUAL 1)
        message(WARNING "Can't generate coverage for target ${targetName} "
                        "because it does not have a single compiler"
        )
        return()
    endif()

    # Coverage works only on GCC/LLVM
    set(coverage_flags "-O0 --coverage")
    if(${target_compiler} STREQUAL "GNU")
        set(coverage_flags "${coverage_flags} -fprofile-abs-path")
    endif()

    # Check if compiler supports coverage flags
    if(NOT DEFINED CoverageFlags_DETECTED)
        check_compiler_flags("${coverage_flags}" ${target_lang} CoverageFlags_DETECTED)
        if(NOT CoverageFlags_DETECTED)
            message(WARNING "Coverage flags are not supported by compiler for target ${targetName}")
        endif()
    endif()

    if(NOT CoverageFlags_DETECTED)
        return()
    endif()

    # Add compiler options for current target
    separate_arguments(coverage_flags NATIVE_COMMAND "${coverage_flags}")
    target_compile_options(${targetName} PRIVATE ${coverage_flags})
    target_link_options(${targetName} PRIVATE "--coverage")
endfunction()
