# [cmake_documentation] GcovrCodeCoverage.cmake
#
# Enables code coverage report generation with Gcovr:
#
# ~~~{.cmake}
# include(GcovrCodeCoverage)
# enable_coverage()
# add_gcovr_coverage_target(
#     HTML COBERTURA COVERALLS SONARQUBE
# )
# ~~~
#
# In example above, will be created the following targets:
# - __coverage-clean__:  Reset coverage counters to zero
# - __coverage__:        Generate coverage report assuming that tests were already run
#
# Defines the following functions:
# - @ref enable_coverage
# - @ref target_enable_coverage
# - @ref add_gcovr_coverage_target
#
# The module defines the following variables:
# @arg __COVERAGE_COMPILER_FLAGS__: Compiler flags to enable code coverage support
# [/cmake_documentation]

set(COVERAGE_COMPILER_FLAGS
    "-O0 --coverage"
    CACHE INTERNAL ""
)
if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-fprofile-abs-path HAVE_cxx_fprofile_abs_path)
    if(HAVE_cxx_fprofile_abs_path)
        set(COVERAGE_COMPILER_FLAGS "${COVERAGE_COMPILER_FLAGS} -fprofile-abs-path")
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "(GNU|Clang)")
    include(CheckCCompilerFlag)
    check_c_compiler_flag(-fprofile-abs-path HAVE_c_fprofile_abs_path)
    if(HAVE_c_fprofile_abs_path)
        set(COVERAGE_COMPILER_FLAGS "${COVERAGE_COMPILER_FLAGS} -fprofile-abs-path")
    endif()
else()
    set(COVERAGE_COMPILER_FLAGS "")
endif()

# [cmake_documentation] enable_coverage()
#
# The function adds coverage compiler and linker flags
# to global `CMAKE_C_FLAGS` and `CMAKE_CXX_FLAGS` variables.
# [/cmake_documentation]
function(enable_coverage)
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}"
        PARENT_SCOPE
    )
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}"
        PARENT_SCOPE
    )
endfunction()

# [cmake_documentation] target_enable_coverage(name)
#
# The function adds coverage compiler and linker flags only for particular target.
#
# Arguments:
# @arg __target__: Target name for which coverage should be enabled
# [/cmake_documentation]
function(target_enable_coverage name)
    separate_arguments(flag_list NATIVE_COMMAND "${COVERAGE_COMPILER_FLAGS}")
    target_compile_options(${name} PRIVATE ${flag_list})
    target_link_options(${name} PRIVATE "--coverage")
endfunction()

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
# @param SOURCE_DIRS          List of directories with source files
# @param OUTPUT_DIRECTORY     Path to output dir (default: `\${PROJECT_BINARY_DIR}/coverage_results`)
# @param GCOV_EXECUTABLE      Name of `gcov` executable
#                             (default: `gcov` for GCC or `llvm-cov gcov` for Clang)
# @param GCOVR_FILTER         List of patterns to include to coverage
# @param GCOVR_EXCLUDE        List of patterns to exclude from coverage
# @param GCOVR_OPTIONS        List of extra options for `gcovr`
# [/cmake_documentation]
function(add_gcovr_coverage_target)
    set(options HTML COBERTURA COVERALLS SONARQUBE)
    set(oneValueArgs COVERAGE_TARGET COVERAGE_INIT_TARGET CHECK_TARGET OUTPUT_DIRECTORY
                     GCOV_EXECUTABLE
    )
    set(multiValueArgs GCOVR_EXCLUDE GCOVR_FILTER ARG_GCOVR_OPTIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_COVERAGE_TARGET)
        message(FATAL_ERROR "GcovrCodeCoverage target name is not specified!")
    endif()

    if(NOT ARG_COVERAGE_INIT_TARGET)
        message(FATAL_ERROR "GcovrCodeCoverage init target name is not specified!")
    endif()

    if(NOT ARG_OUTPUT_DIRECTORY)
        message(FATAL_ERROR "GcovrCodeCoverage output directory is not specified!")
    endif()

    if(COVERAGE_COMPILER_FLAGS)
        if(NOT ARG_GCOV_EXECUTABLE)
            if(CMAKE_CXX_COMPILER_VERSION)
                string(REGEX REPLACE "([0-9]+)\\..*" "\\1" COMPILER_VERSION_MAJOR
                                     ${CMAKE_CXX_COMPILER_VERSION}
                )
            elseif(CMAKE_C_COMPILER_VERSION)
                string(REGEX REPLACE "([0-9]+)\\..*" "\\1" COMPILER_VERSION_MAJOR
                                     ${CMAKE_C_COMPILER_VERSION}
                )
            endif()
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
                find_program(
                    LlvmCov_EXECUTABLE
                    NAMES llvm-cov-${COMPILER_VERSION_MAJOR} llvm-cov
                    DOC "llvm-cov executable"
                )
                set(ARG_GCOV_EXECUTABLE "${LlvmCov_EXECUTABLE} gcov")
            else()
                find_program(
                    Gcov_EXECUTABLE
                    NAMES gcov-${COMPILER_VERSION_MAJOR} gcov
                    DOC "gcov executable"
                )
                set(ARG_GCOV_EXECUTABLE "${Gcov_EXECUTABLE}")
            endif()
        endif()
        set(gcovr_dirs --root ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
        set(gcovr_options --exclude-unreachable-branches --exclude-throw-branches --print-summary
                          --gcov-executable ${ARG_GCOV_EXECUTABLE} ${ARG_GCOVR_OPTIONS}
        )

        list(APPEND gcovr_options --exclude "${PROJECT_BINARY_DIR}/*")
        if(ARG_GCOVR_EXCLUDE)
            foreach(exclude IN LISTS ARG_GCOVR_EXCLUDE)
                list(APPEND gcovr_options --exclude "${exclude}")
            endforeach()
        endif()

        if(ARG_GCOVR_FILTER)
            foreach(filter IN LISTS ARG_GCOVR_FILTER)
                list(APPEND gcovr_options --filter "${filter}")
            endforeach()
        endif()

        # Gcovr is used to generate coverage reports
        find_package(Gcovr REQUIRED)

        set(GCOVR_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUTPUT_DIRECTORY})
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
                GCOVR_COMMANDS
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

        list(APPEND GCOVR_COMMANDS COMMAND ${Gcovr_EXECUTABLE} ${gcovr_options} ${gcovr_dirs})

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
            ${GCOVR_COMMANDS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            COMMENT "Processing code coverage counters and generating reports."
        )

        # As long as ${ARG_CHECK_TARGET} is not defined, calling ${ARG_COVERAGE_TARGET} will not
        # execute any tests (the `check`) but only generate reports. This assumes that the counters
        # were initialized and the tests executed manually before.
        if(TARGET ${ARG_CHECK_TARGET})
            add_dependencies(${ARG_COVERAGE_TARGET} ${ARG_CHECK_TARGET})
            add_dependencies(${ARG_CHECK_TARGET} ${ARG_COVERAGE_INIT_TARGET})
        endif()
    else()
        message(WARNING "Compiler does not support gcov, no coverage target will be created")
    endif()
endfunction()
