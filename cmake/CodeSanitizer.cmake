# [cmake_documentation] CodeSanitizer.cmake
#
# Enables various sanitizers for specified target (libasan, liblsan, libubsan and libtsan).
# Usage example:
#
# ~~~{.cmake}
# set(TARGET hello)
# add_executable(hello main.cpp)
# include(CodeSanitizer)
# target_enable_sanitizers(hello)
# ~~~
#
# Defines the following functions:
# - @ref target_enable_sanitizers
#
# Uses the following parameters:
# @arg __ENABLE_ASAN__:  Enable address sanitizer (`libasan`)
# @arg __ENABLE_LSAN__:  Enable leak sanitizer (`liblsan`)
# @arg __ENABLE_UBSAN__: Enable undefined behavior sanitizer (`libubsan`)
# @arg __ENABLE_TSAN__:  Enable thread sanitizer (`libtsan`)
# [/cmake_documentation]

include(Helpers)
find_package_switchable(
    ASan
    OPTION ENABLE_ASAN
    PURPOSE "Address sanitizer"
)
find_package_switchable(
    LSan
    OPTION ENABLE_LSAN
    PURPOSE "Leak sanitizer"
)
find_package_switchable(
    UBSan
    OPTION ENABLE_UBSAN
    PURPOSE "Undefined behavior sanitizer"
)
find_package_switchable(
    TSan
    OPTION ENABLE_TSAN
    DEFAULT OFF
    PURPOSE "Thread sanitizer"
)

if((ENABLE_ASAN OR ENABLE_LSAN) AND ENABLE_TSAN)
    message(
        FATAL_ERROR "AddressSanitizer/LeakSanitizer and ThreadSanitizer cannot be enabled together"
    )
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)" AND NOT CMAKE_CXX_COMPILER_ID MATCHES
                                                       "(GNU|Clang)"
)
    set(ENABLE_ASAN OFF)
    set(ENABLE_LSAN OFF)
    set(ENABLE_UBSAN OFF)
    set(ENABLE_TSAN OFF)
    message(WARNING "Sanitizers are only supported by GNU GCC and Clang")
endif()

# [cmake_documentation] target_enable_sanitizers(targetName)
#
# The function enables sanitizer compilation options
# depending on existing sanitizers and CMake option values.
#
# Required arguments:
# @arg __targetName__: Name of the target for which sanitizers should be enabled
#
# [/cmake_documentation]
function(target_enable_sanitizers targetName)
    # Enable AddressSanitizer
    if(ENABLE_ASAN)
        target_compile_options(${targetName} PRIVATE -fsanitize=address)
        target_link_libraries(${targetName} PUBLIC -fsanitize=address)
    endif()

    # Enable UndefinedBehaviorSanitizer
    if(ENABLE_UBSAN)
        target_compile_options(
            ${targetName}
            PRIVATE -fsanitize=undefined
                    -fno-sanitize-recover=undefined
                    -fsanitize=vptr
                    -fsanitize=enum
                    -fsanitize=bool
                    -fsanitize=returns-nonnull-attribute
                    -fsanitize=nonnull-attribute
                    -fsanitize=float-cast-overflow
                    -fsanitize=float-divide-by-zero
                    -fsanitize=alignment
                    -fsanitize=bounds
                    -fsanitize=signed-integer-overflow
                    -fsanitize=return
                    -fsanitize=null
                    -fsanitize=vla-bound
                    -fsanitize=unreachable
                    -fsanitize=integer-divide-by-zero
                    -fsanitize=shift
        )

        # The object size sanitizer has no effect at -O0
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Coverage" AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(
                ${targetName}
                PRIVATE -fsanitize=object-size
            )
        endif()

        target_link_libraries(${targetName} PUBLIC -fsanitize=undefined)
    endif()

    # Enable LeakSanitizer
    if(ENABLE_LSAN)
        target_compile_options(${targetName} PRIVATE -fsanitize=leak)
        target_link_libraries(${targetName} PUBLIC -fsanitize=leak)
    endif()

    # Enable ThreadSanitizer
    if(ENABLE_TSAN)
        target_compile_options(${targetName} PRIVATE -fsanitize=thread)
        target_link_libraries(${targetName} PUBLIC -fsanitize=thread)
    endif()
endfunction()
