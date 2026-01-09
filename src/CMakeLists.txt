# Required for install path variables
include(GNUInstallDirs)

set(SLIMLOG_INCLUDES_LEVEL "")
option(SLIMLOG_SYSTEM_INCLUDES "Use system includes for SlimLog headers" OFF)
if(SLIMLOG_SYSTEM_INCLUDES)
    message(WARNING "SYSTEM INCLUDES")
    set(SLIMLOG_INCLUDES_LEVEL "SYSTEM")
endif()

# ---------------------------------------------------------------------------------------
# Static/shared library
# ---------------------------------------------------------------------------------------
add_library(slimlog slimlog.cpp)
add_library(slimlog::slimlog ALIAS slimlog)

file(GLOB PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/include/slimlog/*.h
     ${PROJECT_SOURCE_DIR}/include/slimlog/sinks/*.h ${PROJECT_SOURCE_DIR}/include/slimlog/util/*.h
)

target_sources(
    slimlog
    PUBLIC FILE_SET
           public_headers
           TYPE
           HEADERS
           BASE_DIRS
           ${PROJECT_SOURCE_DIR}/include
           FILES
           ${PUBLIC_HEADERS}
           FILE_SET
           generated_headers
           TYPE
           HEADERS
           BASE_DIRS
           ${CMAKE_CURRENT_BINARY_DIR}
           FILES
           ${CMAKE_CURRENT_BINARY_DIR}/slimlog_export.h
)

# Compiler / language configuration.
set_target_properties(
    slimlog
    PROPERTIES CXX_EXTENSIONS OFF
               CXX_VISIBILITY_PRESET "hidden"
               VISIBILITY_INLINES_HIDDEN TRUE
)
target_compile_features(slimlog PUBLIC cxx_std_20)
target_compile_options(
    slimlog PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall
                    -Wextra -Wpedantic -Werror>
)

target_include_directories(
    slimlog ${SLIMLOG_INCLUDES_LEVEL}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Generate a header with export macros
include(GenerateExportHeader)
set(export_custom_content
    [[
#ifdef SLIMLOG_HEADER_ONLY
#  undef SLIMLOG_EXPORT
#  define SLIMLOG_EXPORT
#  undef SLIMLOG_NO_EXPORT
#  define SLIMLOG_NO_EXPORT
#endif
]]
)
generate_export_header(slimlog CUSTOM_CONTENT_FROM_VARIABLE export_custom_content)

if(SLIMLOG_SANITIZERS AND COMMAND target_enable_sanitizers)
    target_enable_sanitizers(
        slimlog
        ASAN ${SLIMLOG_SANITIZE_ADDRESS}
        LSAN ${SLIMLOG_SANITIZE_LEAK}
        MSAN ${SLIMLOG_SANITIZE_MEMORY}
        UBSAN ${SLIMLOG_SANITIZE_UNDEFINED}
        TSAN ${SLIMLOG_SANITIZE_THREAD}
    )
endif()

if(SLIMLOG_COVERAGE AND COMMAND target_enable_coverage)
    target_enable_coverage(slimlog)
endif()

if(SLIMLOG_ANALYZERS AND COMMAND target_enable_static_analysis)
    target_enable_static_analysis(
        slimlog
        CLANG_TIDY ${SLIMLOG_ANALYZE_CLANG_TIDY}
        CLANG_TIDY_EXTRA_ARGS ${CLANG_TIDY_EXTRA_ARGS}
        IWYU ${SLIMLOG_ANALYZE_IWYU}
        IWYU_EXTRA_ARGS ${IWYU_EXTRA_ARGS}
        CPPCHECK ${SLIMLOG_ANALYZE_CPPCHECK}
        CPPCHECK_EXTRA_ARGS ${CPPCHECK_EXTRA_ARGS}
    )
endif()

# ---------------------------------------------------------------------------------------
# Header only version
# ---------------------------------------------------------------------------------------
add_library(slimlog-header-only INTERFACE)
add_library(slimlog::slimlog-header-only ALIAS slimlog-header-only)

target_compile_features(slimlog-header-only INTERFACE cxx_std_20)
target_compile_options(slimlog-header-only INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_definitions(slimlog-header-only INTERFACE SLIMLOG_HEADER_ONLY)

target_include_directories(
    slimlog-header-only ${SLIMLOG_INCLUDES_LEVEL}
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---------------------------------------------------------------------------------------
# Use fmt package if required
# ---------------------------------------------------------------------------------------
if(SLIMLOG_FMTLIB OR SLIMLOG_FMTLIB_HO)
    if(NOT TARGET fmt::fmt)
        find_package(fmt CONFIG REQUIRED)
    endif()
    target_compile_definitions(slimlog PUBLIC SLIMLOG_FMTLIB)
    target_compile_definitions(slimlog-header-only INTERFACE SLIMLOG_FMTLIB)

    if(SLIMLOG_FMTLIB)
        target_link_libraries(slimlog PUBLIC fmt::fmt)
        target_link_libraries(slimlog-header-only INTERFACE fmt::fmt)
    else()
        target_link_libraries(slimlog PUBLIC fmt::fmt-header-only)
        target_link_libraries(slimlog-header-only INTERFACE fmt::fmt-header-only)
    endif()

    # Check which unicode types are supported
    include(CheckCXXSymbolExists)
    if(${fmt_VERSION} VERSION_GREATER_EQUAL 11.0.0)
        set(fmt_format_context "fmt::buffered_context")
    else()
        set(fmt_format_context "fmt::buffer_context")
    endif()

    # Push check state and set required includes and definitions for fmtlib
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    get_target_property(FMT_INCLUDE_DIRS fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
    set(CMAKE_REQUIRED_INCLUDES ${FMT_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_DEFINITIONS -DFMT_HEADER_ONLY)

    check_cxx_symbol_exists("clock_gettime" "ctime" SLIMLOG_HAS_CLOCK_GETTIME)
    if(SLIMLOG_HAS_CLOCK_GETTIME)
        target_compile_definitions(slimlog PUBLIC SLIMLOG_HAS_CLOCK_GETTIME)
        target_compile_definitions(slimlog-header-only INTERFACE SLIMLOG_HAS_CLOCK_GETTIME)
    endif()

    foreach(size 8 16 32)
        check_cxx_symbol_exists(
            "fmt::make_format_args<${fmt_format_context}<char${size}_t>, std::chrono::sys_seconds>"
            "fmt/format.h;fmt/chrono.h;chrono" SLIMLOG_CHAR${size}_T_FORMATTABLE
        )
        option(SLIMLOG_CHAR${size}_T "Enable support for char${size}_t"
               SLIMLOG_${$CHAR${size}_T_FORMATTABLE}
        )
        if(SLIMLOG_CHAR${size}_T)
            if(SLIMLOG_CHAR${size}_T_FORMATTABLE)
                target_compile_definitions(slimlog PUBLIC SLIMLOG_CHAR${size}_T)
                target_compile_definitions(slimlog-header-only INTERFACE SLIMLOG_CHAR${size}_T)
            else()
                message(FATAL_ERROR "fmtlib does not support char${size}_t on this platform")
            endif()
        endif()
    endforeach()

    # Restore check state
    cmake_pop_check_state()

    # Add dependency to pkg-config file
    set(PKG_CONFIG_REQUIRES fmt)
else()
    # If fmtlib is not available, check for C++20 std::format()
    include(CheckCXXSymbolExists)
    check_cxx_symbol_exists(
        "std::make_format_args<std::format_context, std::chrono::sys_seconds>" "format;chrono"
        HAS_CXX20_FORMAT
    )
    if(NOT HAS_CXX20_FORMAT)
        message(FATAL_ERROR "C++20 std::format() is not available, please enable fmtlib support")
    endif()
endif()

# ---------------------------------------------------------------------------------------
# Install the library and headers
# ---------------------------------------------------------------------------------------
install(
    TARGETS slimlog slimlog-header-only
    EXPORT slimlog-targets
    FILE_SET public_headers FILE_SET generated_headers
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# ---------------------------------------------------------------------------------------
# Install CMake config files
# ---------------------------------------------------------------------------------------
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/dist/slimlog-config.cmake.in"
    "${PROJECT_BINARY_DIR}/slimlog-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/slimlog-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Save targets to binary dir for use without installing
export(
    TARGETS slimlog slimlog-header-only
    NAMESPACE slimlog::
    FILE ${PROJECT_BINARY_DIR}/slimlog-targets.cmake
)
install(
    EXPORT slimlog-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    NAMESPACE slimlog::
    FILE slimlog-targets.cmake
)

install(FILES "${PROJECT_BINARY_DIR}/slimlog-config.cmake"
              "${PROJECT_BINARY_DIR}/slimlog-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

# ---------------------------------------------------------------------------------------
# Install pkg-config file
# ---------------------------------------------------------------------------------------
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(PKG_CONFIG_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(PKG_CONFIG_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(PKG_CONFIG_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
else()
    set(PKG_CONFIG_LIBDIR "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()

get_property(
    PKG_CONFIG_DEFINES
    TARGET slimlog
    PROPERTY INTERFACE_COMPILE_DEFINITIONS
)
list(TRANSFORM PKG_CONFIG_DEFINES PREPEND "-D")
string(REPLACE ";" " " PKG_CONFIG_DEFINES "${PKG_CONFIG_DEFINES}")

configure_file(
    "${PROJECT_SOURCE_DIR}/dist/${PROJECT_NAME}.pc.in" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
    @ONLY
)
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
