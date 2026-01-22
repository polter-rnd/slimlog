cmake_minimum_required(VERSION 3.14)

if(SLIMLOG_TESTS)
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
option(SLIMLOG_FORMATTERS "Code formatters" OFF)
add_feature_info("CodeFormatting" SLIMLOG_FORMATTERS "formatting and checking code style")
if(SLIMLOG_FORMATTERS)
    include(CodeFormatting)
    detect_available_code_formatters(
        CLANG_FORMAT_OPTION SLIMLOG_FORMAT_CLANG
        CLANG_FORMAT_MIN_VERSION 15.0
        CMAKE_FORMAT_OPTION SLIMLOG_FORMAT_CMAKE
        CMAKE_FORMAT_MIN_VERSION 0.6.0
    )
    add_code_format_targets(
        CHECK_TARGET formatcheck
        FORMAT_TARGET format
        SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        EXCLUDE_DIRS ${CMAKE_BINARY_DIR}
        CLANG_FORMAT ${SLIMLOG_FORMAT_CLANG}
        CMAKE_FORMAT ${SLIMLOG_FORMAT_CMAKE}
    )
endif()

# Option for enabling / disabling static analyzers
option(SLIMLOG_ANALYZERS "Static code analyzers" OFF)
add_feature_info("StaticCodeAnalysis" SLIMLOG_ANALYZERS "code linting and static analysis")
if(SLIMLOG_ANALYZERS)
    include(StaticCodeAnalysis)
    detect_available_static_analysers(
        CPPCHECK_OPTION SLIMLOG_ANALYZE_CPPCHECK
        CPPCHECK_MIN_VERSION 2.10
        CLANG_TIDY_OPTION SLIMLOG_ANALYZE_CLANG_TIDY
        CLANG_TIDY_MIN_VERSION 17.0
        IWYU_OPTION SLIMLOG_ANALYZE_IWYU
        IWYU_MIN_VERSION 0.23
    )

    # Static analyzer configuration
    if(SLIMLOG_ANALYZE_CLANG_TIDY)
        set(CLANG_TIDY_EXTRA_ARGS "--header-filter=${PROJECT_SOURCE_DIR}/(include|src|test)/.*")
        if(MSVC)
            set(CLANG_TIDY_EXTRA_ARGS ${CLANG_TIDY_EXTRA_ARGS} --extra-arg=/EHsc)
        endif()
    endif()
    if(SLIMLOG_ANALYZE_IWYU)
        set(IWYU_EXTRA_ARGS
            --cxx17ns
            --quoted_includes_first
            --no_fwd_decls
            --verbose=3
            --mapping_file=${PROJECT_SOURCE_DIR}/.iwyu-mappings
            --check_also=${PROJECT_SOURCE_DIR}/include/*
            --transitive_includes_only
        )
        # Specify all standard include directories so that IWYU will able to check them
        set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    endif()
    if(SLIMLOG_ANALYZE_CPPCHECK)
        set(CPPCHECK_EXTRA_ARGS
            --enable=warning,performance,portability,information,missingInclude --inline-suppr
            --suppress=missingIncludeSystem --suppress=unmatchedSuppression
            --check-level=exhaustive
        )
    endif()
endif()

# Option for enabling / disabling sanitizers
option(SLIMLOG_SANITIZERS "Code sanitizers" OFF)
add_feature_info("CodeSanitizing" SLIMLOG_SANITIZERS "address/leak/UB/thread sanitizers")
if(SLIMLOG_SANITIZERS)
    include(CodeSanitizing)
    detect_available_sanitizers(
        ASAN_OPTION SLIMLOG_SANITIZE_ADDRESS
        LSAN_OPTION SLIMLOG_SANITIZE_LEAK
        MSAN_OPTION SLIMLOG_SANITIZE_MEMORY
        UBSAN_OPTION SLIMLOG_SANITIZE_UNDEFINED
        TSAN_OPTION SLIMLOG_SANITIZE_THREAD
    )
endif()

# Option for enabling / disabling code coverage
find_package_switchable(
    Gcovr
    OPTION SLIMLOG_COVERAGE
    DEFAULT OFF
    PURPOSE "Code coverage reports by gcovr"
)
add_feature_info("GcovrCodeCoverage" SLIMLOG_COVERAGE "code test coverage")

if(SLIMLOG_COVERAGE)
    include(GcovrCodeCoverage)
    add_gcovr_coverage_target(
        GCOV COBERTURA HTML
        COVERAGE_TARGET coverage
        COVERAGE_INIT_TARGET coverage-clean
        GCOVR_EXCLUDE ${PROJECT_SOURCE_DIR}/test ${PROJECT_SOURCE_DIR}/src
    )
endif()

# fmtlib is recommended for better formatting performance
set(fmt_option SLIMLOG_FMTLIB)
set(fmt_purpose "Implementation of C++20 std::format")
if(SLIMLOG_FMTLIB AND SLIMLOG_FMTLIB_HO)
    message(FATAL_ERROR "SLIMLOG_FMTLIB and SLIMLOGSLIMLOG_FMTLIB mutually exclusive")
elseif(SLIMLOG_FMTLIB)
    option(SLIMLOG_FMTLIB_HO "${fmt_purpose} (header-only)" OFF)
else()
    option(SLIMLOG_FMTLIB ${fmt_purpose} OFF)
    set(fmt_option SLIMLOG_FMTLIB_HO)
    set(fmt_purpose "${fmt_purpose} (header-only)")
endif()
find_package_switchable(
    fmt
    MIN_VERSION 8.1.0
    OPTION ${fmt_option}
    DEFAULT ON
    PURPOSE ${fmt_purpose}
)

# Include library targets
add_subdirectory(src)

# Option for building documentation
find_package_switchable(
    Doxygen
    OPTION SLIMLOG_DOCS
    DEFAULT OFF
    PURPOSE "Build Doxygen documentation"
)
add_feature_info("DoxygenDocumentation" SLIMLOG_DOCS "build documentation")

if(SLIMLOG_DOCS)
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
option(SLIMLOG_TESTS "Build unit tests" OFF)
add_feature_info("UnitTests" SLIMLOG_TESTS "isolated logger tests")
if(SLIMLOG_TESTS)
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
