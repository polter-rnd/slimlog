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
    include(StaticCodeAnalysis)
endif()

# Option for enabling / disabling sanitizers
option(ENABLE_SANITIZERS "Code sanitizers" OFF)
add_feature_info("CodeSanitizers" ENABLE_SANITIZERS "address/leak/UB/thread sanitizers")
if(ENABLE_SANITIZERS)
    include(CodeSanitizer)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package_switchable(
        Gcovr
        OPTION ENABLE_COVERAGE
        PURPOSE "Code coverage reports by gcovr"
    )
    if(ENABLE_COVERAGE)
        # Generate code coverage counters and HTML report
        include(GcovrCodeCoverage)
        add_gcovr_coverage_target(
            HTML COBERTURA COVERALLS SONARQUBE
            COVERAGE_TARGET coverage
            COVERAGE_INIT_TARGET coverage-clean
            OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/coverage
        )
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

if(BUILD_DOCS)
    include(DoxygenDocumentation)
    add_doxygen_documentation_target(
        CMAKE_DOCS
        DOXYGEN_TARGET docs
        DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
        DOXYGEN_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs
        SOURCE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif()

# Summary of enabled and disabled features
feature_summary(WHAT ALL)

# Summary of available options
message(STATUS "Options summary:")
message("")
dump_option_variables("^ENABLE_|^BUILD_")
message("")
