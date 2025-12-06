cmake_minimum_required(VERSION 3.23...3.26)

if(SLIMLOG_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "testing")
endif()

project(
    "slimlog"
    VERSION 0.1.0
    LANGUAGES CXX
)

# Require at least C++20
if(PROJECT_IS_TOP_LEVEL AND NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Set build type if none specified
if(PROJECT_IS_TOP_LEVEL
   AND NOT DEFINED CMAKE_BUILD_TYPE
   AND NOT DEFINED CMAKE_CONFIGURATION_TYPES
)
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
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# To generate fancy report of used features
include(FeatureSummary)
include(Helpers)

# Option for enabling / disabling code formatters
option(SLIMLOG_ENABLE_FORMATTERS "Code formatters" OFF)
add_feature_info("CodeFormatting" SLIMLOG_ENABLE_FORMATTERS "formatting and checking code style")
if(SLIMLOG_ENABLE_FORMATTERS)
    set(CLANG_FORMAT_MIN_VERSION 15.0)
    include(CodeFormatting)
    add_code_format_targets(
        CHECK_TARGET formatcheck
        FORMAT_TARGET format
        SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src
        EXCLUDE_DIRS ${CMAKE_BINARY_DIR}
    )
endif()

# Option for enabling / disabling static analyzers
option(SLIMLOG_ENABLE_ANALYZERS "Static code analyzers" OFF)
add_feature_info("StaticCodeAnalysis" SLIMLOG_ENABLE_ANALYZERS "code linting and static analysis")
if(SLIMLOG_ENABLE_ANALYZERS)
    set(CPPCHECK_MIN_VERSION 2.10)
    set(CLANG_TIDY_MIN_VERSION 17.0)
    set(CLANG_FORMAT_MIN_VERSION 17.0)
    set(IWYU_MIN_VERSION 0.23)
    include(StaticCodeAnalysis)

    # Static analyzer configuration
    if(ANALYZE_CLANG_TIDY)
        set(CLANG_TIDY_EXTRA_ARGS "--header-filter=${CMAKE_SOURCE_DIR}/(include|src|test)/.*")
    endif()
    if(ANALYZE_IWYU)
        set(IWYU_EXTRA_ARGS
            --cxx17ns
            --quoted_includes_first
            --no_fwd_decls
            --verbose=3
            --mapping_file=${CMAKE_SOURCE_DIR}/.iwyu-mappings
            --check_also=${CMAKE_SOURCE_DIR}/include/*
            --transitive_includes_only
        )
        # Specify all standard include directories so that IWYU will able to check them
        set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    endif()
    if(ANALYZE_CPPCHECK)
        set(CPPCHECK_EXTRA_ARGS
            --enable=warning,performance,portability,information,missingInclude --inline-suppr
            --suppress=missingIncludeSystem --suppress=unmatchedSuppression
            --check-level=exhaustive
        )
    endif()
endif()

# Option for enabling / disabling sanitizers
option(SLIMLOG_ENABLE_SANITIZERS "Code sanitizers" OFF)
add_feature_info("CodeSanitizing" SLIMLOG_ENABLE_SANITIZERS "address/leak/UB/thread sanitizers")
if(SLIMLOG_ENABLE_SANITIZERS)
    include(CodeSanitizing)
endif()

# Enable coverage only for Debug builds on GCC and Clang compilers
if((CMAKE_BUILD_TYPE STREQUAL "Coverage" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
   AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
)
    find_package_switchable(
        Gcovr
        OPTION SLIMLOG_ENABLE_COVERAGE
        PURPOSE "Code coverage reports by gcovr"
    )
    add_feature_info("GcovrCodeCoverage" SLIMLOG_ENABLE_COVERAGE "code test coverage")

    if(SLIMLOG_ENABLE_COVERAGE)
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
set(FMT_OPTION SLIMLOG_ENABLE_FMTLIB)
set(FMT_PURPOSE "Implementation of C++20 std::format")
if(SLIMLOG_ENABLE_FMTLIB AND SLIMLOG_ENABLE_FMTLIB_HO)
    message(FATAL_ERROR "SLIMLOG_ENABLE_FMTLIB and SLIMLOG_ENABLE_FMTLIB_HO are mutually exclusive")
elseif(SLIMLOG_ENABLE_FMTLIB)
    option(SLIMLOG_ENABLE_FMTLIB_HO "${FMT_PURPOSE} (header-only)" OFF)
else()
    option(SLIMLOG_ENABLE_FMTLIB ${FMT_PURPOSE} OFF)
    set(FMT_OPTION SLIMLOG_ENABLE_FMTLIB_HO)
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
    OPTION SLIMLOG_BUILD_DOCS
    DEFAULT OFF
    PURPOSE "Build Doxygen documentation"
)
add_feature_info("DoxygenDocumentation" SLIMLOG_BUILD_DOCS "build documentation")

if(SLIMLOG_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    # Write documentation for CMake files
    include(CMakeDocumentation)
    parse_cmake_documentation(INCLUDES "cmake/*")
    write_cmake_documentation(${PROJECT_BINARY_DIR}/cmake.dox SORTED)

    # Create docs target
    set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/docs)
    set(DOXYGEN_PROJECT_NAME "SlimLog")
    doxygen_add_docs(
        docs ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/cmake.dox
        COMMENT "Generating API documentation with Doxygen"
    )
endif()

# Option for building tests
option(SLIMLOG_BUILD_TESTS "Build unit tests" OFF)
add_feature_info("UnitTests" SLIMLOG_BUILD_TESTS "isolated logger tests")
if(SLIMLOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Summary of enabled and disabled features
feature_summary(WHAT ALL)

# Summary of available options
message(STATUS "Options summary:")
message("")
dump_option_variables("^SLIMLOG_")
message("")
