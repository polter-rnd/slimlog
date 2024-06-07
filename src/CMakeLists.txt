set(TARGET ${PROJECT_NAME})

add_executable(${TARGET} main.cpp)

target_include_directories(${TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_options(
    ${TARGET} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall
                      -Wextra -Wpedantic -Werror>
)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(__cpp_lib_format "format" HAS_CXX20_LIBFORMAT)

if(NOT HAS_CXX20_LIBFORMAT)
    set(ENABLE_FMTLIB ON)
endif()

find_package_switchable(
    fmt 9.1.0
    MIN_VERSION OPTION ENABLE_FMTLIB
    DEFAULT ENABLE_FMTLIB
    PURPOSE "Implementation of std::format from C++20"
)

set(ENABLE_FMTLIB
    ${ENABLE_FMTLIB}
    PARENT_SCOPE
)
if(ENABLE_FMTLIB)
    find_package(fmt 9.1.0 REQUIRED)
    add_compile_definitions(${TARGET} ENABLE_FMTLIB)
    target_link_libraries(${TARGET} PRIVATE fmt::fmt-header-only)
endif()

if(ENABLE_SANITIZERS AND COMMAND target_enable_sanitizers)
    target_enable_sanitizers(${TARGET})
endif()

if(ENABLE_COVERAGE AND COMMAND target_enable_coverage)
    target_enable_coverage(${TARGET})
endif()
