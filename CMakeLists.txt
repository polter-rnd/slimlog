cmake_minimum_required(VERSION 3.23...3.26)

if(BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES "testing")
endif()

project(
    "slimlog"
    VERSION 0.1.0
    LANGUAGES CXX
)

# Compiler / language configuration.
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

# Set build type if none specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Choose the type of build." FORCE
    )
    # Set the possible values of build type for cmake-gui
    set_property(
        CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

# Include as system headers
option(SLIMLOG_SYSTEM_INCLUDES "Include as system headers (skip for clang-tidy)." OFF)

# CMake include files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# TODO: do we really need to change standard variables in cache?

# Compilation database is required for static code analysis
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL ""
)

# Specify all standard include directories so that IWYU will able to check them
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

# To generate fancy report of used features
include(FeatureSummary)
include(Helpers)

# Option for enabling / disabling code formatters
option(ENABLE_FORMATTERS "Code formatters" OFF)
add_feature_info("CodeFormatting" ENABLE_FORMATTERS "formatting and checking code style")
if(ENABLE_FORMATTERS)
    set(CLANG_FORMAT_MIN_VERSION 15.0)
    include(CodeFormatting)
    add_code_format_targets(
        CHECK_TARGET formatcheck
        FORMAT_TARGET format
        SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src
        EXCLUDE_DIRS ${CMAKE_BINARY_DIR}
    )
endif()

# TODO: do not touch standard CMake variables in cache

# Reset all static analyzers by default
unset(CMAKE_CXX_CPPCHECK CACHE)
unset(CMAKE_CXX_CLANG_TIDY CACHE)
unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE CACHE)

# Option for enabling / disabling static analyzers
option(ENABLE_ANALYZERS "Static code analyzers" OFF)
add_feature_info("StaticCodeAnalysis" ENABLE_ANALYZERS "code linting and static analysis")
if(ENABLE_ANALYZERS)
    set(CPPCHECK_MIN_VERSION 2.10)
    set(CLANG_TIDY_MIN_VERSION 17.0)
    set(CLANG_FORMAT_MIN_VERSION 17.0)
    set(IWYU_MIN_VERSION 0.23)
    include(StaticCodeAnalysis)

    # Static analyzer configuration
    set(CLANG_TIDY_EXTRA_ARGS "--header-filter=${CMAKE_SOURCE_DIR}/(include|src|test)/.*")
    set(IWYU_EXTRA_ARGS
        --cxx17ns
        --quoted_includes_first
        --no_fwd_decls
        --verbose=3
        --mapping_file=${CMAKE_SOURCE_DIR}/.iwyu-mappings
        --check_also=${CMAKE_SOURCE_DIR}/include/*
        --transitive_includes_only
    )
    set(CPPCHECK_EXTRA_ARGS --enable=warning,performance,portability,information,missingInclude
                            --inline-suppr --suppress=missingIncludeSystem --check-level=exhaustive
    )
endif()

# Option for enabling / disabling sanitizers
option(ENABLE_SANITIZERS "Code sanitizers" OFF)
add_feature_info("CodeSanitizing" ENABLE_SANITIZERS "address/leak/UB/thread sanitizers")
if(ENABLE_SANITIZERS)
    include(CodeSanitizing)
endif()

# Enable coverage only for Debug builds on GCC and Clang compilers
if((CMAKE_BUILD_TYPE STREQUAL "Coverage" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
   AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
)
    find_package_switchable(
        Gcovr
        OPTION ENABLE_COVERAGE
        PURPOSE "Code coverage reports by gcovr"
    )
    add_feature_info("GcovrCodeCoverage" ENABLE_COVERAGE "code test coverage")

    if(ENABLE_COVERAGE)
        include(GcovrCodeCoverage)
        add_gcovr_coverage_target(
            GCOV COBERTURA HTML
            COVERAGE_TARGET coverage
            COVERAGE_INIT_TARGET coverage-clean
            GCOVR_EXCLUDE ${CMAKE_SOURCE_DIR}/test
        )
    endif()
endif()

# fmtlib is recommended for better formatting performance
set(FMT_OPTION ENABLE_FMTLIB)
set(FMT_PURPOSE "Implementation of C++20 std::format")
if(ENABLE_FMTLIB AND ENABLE_FMTLIB_HO)
    message(FATAL_ERROR "ENABLE_FMTLIB and ENABLE_FMTLIB_HO are mutually exclusive")
elseif(ENABLE_FMTLIB)
    option(ENABLE_FMTLIB_HO "${FMT_PURPOSE} (header-only)" OFF)
else()
    option(ENABLE_FMTLIB ${FMT_PURPOSE} OFF)
    set(FMT_OPTION ENABLE_FMTLIB_HO)
    set(FMT_PURPOSE "${FMT_PURPOSE} (header-only)")
endif()
find_package_switchable(
    fmt
    MIN_VERSION 8.1.0
    OPTION ${FMT_OPTION}
    DEFAULT ON
    PURPOSE ${FMT_PURPOSE}
)

# Include library targets
add_subdirectory(src)

# Option for building documentation
find_package_switchable(
    Doxygen
    OPTION BUILD_DOCS
    DEFAULT OFF
    PURPOSE "Build Doxygen documentation"
)
add_feature_info("DoxygenDocumentation" BUILD_DOCS "build documentation")

if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    # Write documentation for CMake files
    include(CMakeDocumentation)
    parse_cmake_documentation(INCLUDES "cmake/*")
    set(cmake_doxygen_file ${PROJECT_BINARY_DIR}/cmake.dox)
    write_cmake_documentation("${cmake_doxygen_file}" SORTED)

    # Create docs target
    set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/docs)
    set(DOXYGEN_PROJECT_NAME "SlimLog")
    doxygen_add_docs(
        docs ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/cmake.dox
        COMMENT "Generating API documentation with Doxygen"
    )
endif()

# Option for building tests
option(BUILD_TESTING "Build unit tests" OFF)
add_feature_info("UnitTests" BUILD_TESTING "isolated logger tests")

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory(test)
endif()

# Summary of enabled and disabled features
feature_summary(WHAT ALL)

# Summary of available options
message(STATUS "Options summary:")
message("")
dump_option_variables("^ENABLE_|^BUILD_|^SANITIZE_|^ANALYZE_|^FORMAT_")
message("")
