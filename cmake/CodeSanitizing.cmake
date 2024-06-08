# [cmake_documentation] CodeSanitizing.cmake
#
# Enables various sanitizers for specified target (libasan, liblsan, libmsan, libubsan and libtsan).
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
# @arg __SANITIZE_ADDRESS__:     Enable address sanitizer (`libasan`)
# @arg __SANITIZE_LEAK__:        Enable leak sanitizer (`liblsan`)
# @arg __SANITIZE_MEMORY__:      Enable memory sanitizer (`libmsan`)
# @arg __SANITIZE_UNDEFINED__:   Enable undefined behavior sanitizer (`libubsan`)
# @arg __SANITIZE_THREAD__:      Enable thread sanitizer (`libtsan`)
# [/cmake_documentation]

include(Helpers)

find_package_switchable(
    ASan
    OPTION SANITIZE_ADDRESS
    PURPOSE "Address sanitizer"
)
find_package_switchable(
    LSan
    OPTION SANITIZE_LEAK
    PURPOSE "Leak sanitizer"
)
find_package_switchable(
    MSan
    OPTION SANITIZE_MEMORY
    DEFAULT OFF
    PURPOSE "Memory sanitizer"
)
find_package_switchable(
    UBSan
    OPTION SANITIZE_UNDEFINED
    PURPOSE "Undefined behavior sanitizer"
)
find_package_switchable(
    TSan
    OPTION SANITIZE_THREAD
    DEFAULT OFF
    PURPOSE "Thread sanitizer"
)

if((SANITIZE_LEAK OR SANITIZE_MEMORY) AND SANITIZE_THREAD)
    message(FATAL_ERROR "ThreadSanitizer is not compatible with MemorySanitizer or LeakSanitizer.")
endif()

if(SANITIZE_ADDRESS AND (SANITIZE_THREAD OR SANITIZE_MEMORY))
    message(
        FATAL_ERROR "AddressSanitizer is not compatible with ThreadSanitizer or MemorySanitizer."
    )
endif()

# [cmake_documentation] sanitizer_add_blacklist_file(fileName)
#
# Adds specific file to blacklist for usage in sanitizers.
#
# Required arguments:
# @arg __fileName__: File name to be blackisted (relative to `\${PROJECT_SOURCE_DIR}`)
#
# [/cmake_documentation]
function(sanitizer_add_blacklist_file fileName)
    if(NOT IS_ABSOLUTE ${fileName})
        set(fileName "${PROJECT_SOURCE_DIR}/${fileName}")
    endif()
    get_filename_component(fileName "${fileName}" REALPATH)

    check_compiler_flags_list("-fsanitize-blacklist=${fileName}" "SanitizerBlacklist" "SanBlist")
endfunction()

# [cmake_documentation] sanitizer_add_flags(targetName, targetCompiler, varPrefix)
#
# Helper to assign sanitizer flags for `targetName`.
#
# Required arguments:
# @arg __targetName__: Name of the target
# @arg __targetCompiler__: Name of the compiler used to build target
# @arg __varPrefix__: Prefix for resulting variables
#
# [/cmake_documentation]
function(sanitizer_add_flags targetName targetCompiler varPrefix)
    # Get list of compilers used by target and check, if sanitizer is available for this target.
    # Other compiler checks like check for conflicting compilers will be done in add_sanitizers
    # function.
    set(sanitizer_flags "${${varPrefix}_${targetCompiler}_FLAGS}")
    if(sanitizer_flags STREQUAL "")
        return()
    endif()

    separate_arguments(
        flags_list UNIX_COMMAND "${sanitizer_flags} ${SanBlist_${targetCompiler}_FLAGS}"
    )
    target_compile_options(${targetName} PUBLIC ${flags_list})
    target_link_options(${targetName} PUBLIC ${flags_list})
endfunction()

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
    # Check if this target will be compiled by exactly one compiler. Otherwise sanitizers can't be
    # used and a warning should be printed once.
    get_target_property(target_type ${targetName} TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        message(WARNING "Can't use any sanitizers for target ${fileName}, "
                        "because it is an interface library and cannot be compiled directly."
        )
        return()
    endif()

    get_target_compilers(${targetName} target_compiler target_lang)
    list(LENGTH target_compiler num_compilers)
    if(num_compilers GREATER 1)
        message(WARNING "Can't use any sanitizers for target ${targetName}, "
                        "because it will be compiled by incompatible compilers. "
                        "Target will be compiled without sanitizers."
        )
        return()
    elseif(num_compilers EQUAL 0)
        # If the target is compiled by no known compiler, give a warning.
        message(WARNING "Sanitizers for target ${targetName} may not be "
                        "usable, because it uses no or an unknown compiler. "
                        "This is a false warning for targets using only object lib(s) as input."
        )
    endif()

    # Enable AddressSanitizer
    if(SANITIZE_ADDRESS)
        sanitizer_add_flags(${targetName} "${target_compiler}" "ASan")
    endif()

    # Enable UndefinedBehaviorSanitizer
    if(SANITIZE_UNDEFINED)
        sanitizer_add_flags(${targetName} "${target_compiler}" "UBSan")
    endif()

    # Enable LeakSanitizer
    if(SANITIZE_LEAK)
        sanitizer_add_flags(${targetName} "${target_compiler}" "LSan")
    endif()

    # Enable MemorySanitizer
    if(SANITIZE_MEMORY)
        sanitizer_add_flags(${targetName} "${target_compiler}" "MSan")
    endif()

    # Enable ThreadSanitizer
    if(SANITIZE_THREAD)
        sanitizer_add_flags(${targetName} "${target_compiler}" "TSan")
    endif()
endfunction()
