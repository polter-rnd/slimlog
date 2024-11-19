# Required for install path variables
include(GNUInstallDirs)

set(SLIMLOG_INCLUDES_LEVEL "")
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
)

target_compile_features(slimlog PUBLIC cxx_std_20)
target_compile_options(
    slimlog PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall
                    -Wextra -Wpedantic -Werror>
)

target_include_directories(
    slimlog ${SLIMLOG_INCLUDES_LEVEL} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(ENABLE_SANITIZERS AND COMMAND target_enable_sanitizers)
    target_enable_sanitizers(slimlog)
endif()

if(ENABLE_COVERAGE AND COMMAND target_enable_coverage)
    target_enable_coverage(slimlog)
endif()

# ---------------------------------------------------------------------------------------
# Header only version
# ---------------------------------------------------------------------------------------
add_library(slimlog-header-only INTERFACE)
add_library(slimlog::slimlog-header-only ALIAS slimlog-header-only)

target_compile_features(slimlog-header-only INTERFACE cxx_std_20)
target_compile_definitions(slimlog-header-only INTERFACE SLIMLOG_HEADER_ONLY)

target_include_directories(
    slimlog-header-only ${SLIMLOG_INCLUDES_LEVEL}
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---------------------------------------------------------------------------------------
# Use fmt package if required
# ---------------------------------------------------------------------------------------
if(ENABLE_FMTLIB OR ENABLE_FMTLIB_HO)
    if(NOT TARGET fmt::fmt)
        find_package(fmt CONFIG REQUIRED)
    endif()
    target_compile_definitions(slimlog PUBLIC SLIMLOG_FMTLIB)
    target_compile_definitions(slimlog-header-only INTERFACE SLIMLOG_FMTLIB)

    if(ENABLE_FMTLIB)
        target_link_libraries(slimlog PUBLIC fmt::fmt)
        target_link_libraries(slimlog-header-only INTERFACE fmt::fmt)
    else()
        target_link_libraries(slimlog PUBLIC fmt::fmt-header-only)
        target_link_libraries(slimlog-header-only INTERFACE fmt::fmt-header-only)
    endif()

    set(PKG_CONFIG_REQUIRES fmt) # add dependency to pkg-config
endif()

# ---------------------------------------------------------------------------------------
# Install the library and headers
# ---------------------------------------------------------------------------------------
install(
    TARGETS slimlog slimlog-header-only
    EXPORT slimlog
    FILE_SET public_headers
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

export(
    TARGETS slimlog slimlog-header-only
    NAMESPACE slimlog::
    FILE ${PROJECT_BINARY_DIR}/slimlog-targets.cmake
)
install(
    EXPORT slimlog
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
