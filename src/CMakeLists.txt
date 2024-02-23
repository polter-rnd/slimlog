set(TARGET ${PROJECT_NAME})

add_executable(${TARGET} main.cpp)

target_include_directories(${TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/include)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(__cpp_lib_format "format" HAS_CXX20_LIBFORMAT)
if(NOT HAS_CXX20_LIBFORMAT)
    find_package(fmt 9.1.0 REQUIRED)

    if(COMMAND set_package_properties)
        set_package_properties(fmt PROPERTIES PURPOSE "Implementation of std::format from C++20")
    endif()
    target_link_libraries(${TARGET} PRIVATE fmt::fmt-header-only)
endif()

if(ENABLE_CODE_ANALYSIS AND COMMAND add_static_analysis_targets)
    add_static_analysis_targets(TARGET ${TARGET} ANALYSIS_TARGET codeanalysis)
endif()

if(ENABLE_SANITIZERS AND COMMAND target_enable_sanitizers)
    target_enable_sanitizers(${TARGET})
endif()

if(ENABLE_COVERAGE AND COMMAND target_enable_coverage)
    target_enable_coverage(${TARGET})
endif()
