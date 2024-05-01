cmake_minimum_required(VERSION 3.15)

project(
    "plaincloud"
    VERSION 0.1.0
    LANGUAGES CXX
)

# Compiler / language configuration.
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

# CMake include files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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
option(ENABLE_CODE_FORMATTING "Code formatting" OFF)
add_feature_info("CodeFormatting" ENABLE_CODE_FORMATTING "formatting and checking code style")
if(ENABLE_CODE_FORMATTING)
    include(CodeFormatting)
    add_code_format_targets(
        CHECK_TARGET formatcheck
        FORMAT_TARGET format
        SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        EXCLUDE_DIRS ${CMAKE_BINARY_DIR}
    )
endif()

# Option for enabling / disabling static analyzers
option(ENABLE_CODE_ANALYSIS "Static code analysis" OFF)
add_feature_info("StaticCodeAnalysis" ENABLE_CODE_ANALYSIS "code linting and static analysis")
if(ENABLE_CODE_ANALYSIS)
    set(CPPCHECK_MIN_VERSION 2.10)
    set(CLANG_TIDY_MIN_VERSION 14.0)
    set(IWYU_MIN_VERSION 0.20)
    include(StaticCodeAnalysis)
    add_static_analysis_target(ANALYSIS_TARGET codeanalysis)
endif()

# Option for enabling / disabling sanitizers
option(ENABLE_SANITIZERS "Code sanitizers" OFF)
add_feature_info("CodeSanitizing" ENABLE_SANITIZERS "address/leak/UB/thread sanitizers")
if(ENABLE_SANITIZERS)
    include(CodeSanitizing)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package_switchable(
        Gcovr
        OPTION ENABLE_COVERAGE
        PURPOSE "Code coverage reports by gcovr"
    )
    add_feature_info("GcovrCodeCoverage" ENABLE_COVERAGE "code test coverage")

    if(ENABLE_COVERAGE)
        include(GcovrCodeCoverage)
        # cmake-format: off
        add_gcovr_coverage_target(
            HTML COBERTURA COVERALLS SONARQUBE
            COVERAGE_TARGET coverage COVERAGE_INIT_TARGET coverage-init
        )
        # cmake-format: on
    endif()
endif()

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
    parse_cmake_documentation(
        INCLUDES ${ARG_CMAKE_INCLUDES}
        EXCLUDES ${ARG_CMAKE_EXCLUDES}
        START_FLAG "[cmake_documentation]"
        END_FLAG "[/cmake_documentation]"
    )
    set(cmake_doxygen_file ${PROJECT_BINARY_DIR}/cmake.dox)
    write_cmake_documentation("${cmake_doxygen_file}" SORTED)

    # Create docs target
    set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/docs)
    set(DOXYGEN_PROJECT_NAME "PlainCloud")
    doxygen_add_docs(
        docs
        ${PROJECT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
endif()

# Summary of enabled and disabled features
feature_summary(WHAT ALL)

# Summary of available options
message(STATUS "Options summary:")
message("")
dump_option_variables("^ENABLE_|^BUILD_|^SANITIZE_")
message("")
