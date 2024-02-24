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

#if(NOT CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)" AND NOT CMAKE_CXX_COMPILER_ID MATCHES
#                                                       "(GNU|Clang)"
#)
#    set(ENABLE_ASAN OFF)
#    set(ENABLE_LSAN OFF)
#    set(ENABLE_UBSAN OFF)
#    set(ENABLE_TSAN OFF)
#    message(WARNING "Sanitizers are only supported by GNU GCC and Clang")
#endif()

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
        # Check if this target will be compiled by exactly one compiler. Other-
        # wise sanitizers can't be used and a warning should be printed once.
        get_target_property(TARGET_TYPE ${TARGET} TYPE)
        if (TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
            message(WARNING "Can't use any sanitizers for target ${TARGET}, "
                    "because it is an interface library and cannot be "
                    "compiled directly.")
            return()
        endif ()
        sanitizer_target_compilers(${TARGET} TARGET_COMPILER)
        list(LENGTH TARGET_COMPILER NUM_COMPILERS)
        if (NUM_COMPILERS GREATER 1)
            message(WARNING "Can't use any sanitizers for target ${TARGET}, "
                    "because it will be compiled by incompatible compilers. "
                    "Target will be compiled without sanitizers.")
            return()

        elseif (NUM_COMPILERS EQUAL 0)
            # If the target is compiled by no known compiler, give a warning.
            message(WARNING "Sanitizers for target ${TARGET} may not be"
                    " usable, because it uses no or an unknown compiler. "
                    "This is a false warning for targets using only "
                    "object lib(s) as input.")
        endif ()

    # Enable AddressSanitizer
    if(ENABLE_ASAN)
        sanitizer_add_flags(${targetName} "AddressSanitizer" "ASan")
    endif()

    # Enable UndefinedBehaviorSanitizer
    if(ENABLE_UBSAN)
        sanitizer_add_flags(${targetName} "UndefinedBehaviorSanitizer" "UBSan")
    endif()

    # Enable LeakSanitizer
    if(ENABLE_LSAN)
        sanitizer_add_flags(${targetName} "ThreadSanitizer" "LSan")
    endif()

    # Enable MemorySanitizer
    if(ENABLE_LSAN)
        sanitizer_add_flags(${targetName} "MemorySanitizer" "MSan")
    endif()

    # Enable ThreadSanitizer
    if(ENABLE_TSAN)
        sanitizer_add_flags(${targetName} "ThreadSanitizer" "TSan")
    endif()
endfunction()
