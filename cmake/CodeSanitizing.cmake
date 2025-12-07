# [cmake_documentation] CodeSanitizing.cmake
#
# Enables various sanitizers for specified target (libasan, liblsan, libmsan, libubsan and libtsan).
# Usage example:
#
# ~~~{.cmake}
# set(TARGET hello)
# add_executable(hello main.cpp)
# include(CodeSanitizer)
# detect_available_sanitizers()
# target_enable_sanitizers(hello ASAN ON
#                                LSAN ON
#                                UBSAN ON)
# ~~~
#
# Defines the following functions:
# - @ref detect_available_sanitizers
# - @ref target_enable_sanitizers
#
# [/cmake_documentation]

# [cmake_documentation] detect_available_sanitizers()
#
# Detects available static code analyzers and sets corresponding options to toggle them.
# Default option names are:
# - SANITIZE_ADDRESS:     Enable address sanitizer
# - SANITIZE_LEAK:        Enable leak sanitizer
# - SANITIZE_MEMORY:      Enable memory sanitizer
# - SANITIZE_UNDEFINED:   Enable undefined behavior sanitizer
# - SANITIZE_THREAD:      Enable thread sanitizer
#
# @param ASAN_OPTION      Name of the flag for detection address sanitizer (`libasan`)
# @param LSAN_OPTION      Name of the flag for detection leak sanitizer (`liblsan`)
# @param MSAN_OPTION      Name of the flag for detection memory sanitizer (`libmsan`)
# @param UBSAN_OPTION     Name of the flag for detection undefined behavior sanitizer (`libubsan`)
# @param TSAN_OPTION      Name of the flag for detection thread sanitizer (`libtsan`)
# [/cmake_documentation]
function(detect_available_sanitizers)
    set(options "")
    set(oneValueArgs ASAN_OPTION LSAN_OPTION MSAN_OPTION UBSAN_OPTION TSAN_OPTION)
    set(multipleValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

    if(NOT ARG_ASAN_OPTION)
        set(ARG_ASAN_OPTION SANITIZE_ADDRESS)
    endif()
    if(NOT ARG_LSAN_OPTION)
        set(ARG_LSAN_OPTION SANITIZE_LEAK)
    endif()
    if(NOT ARG_MSAN_OPTION)
        set(ARG_MSAN_OPTION SANITIZE_MEMORY)
    endif()
    if(NOT ARG_UBSAN_OPTION)
        set(ARG_UBSAN_OPTION SANITIZE_UNDEFINED)
    endif()
    if(NOT ARG_TSAN_OPTION)
        set(ARG_TSAN_OPTION SANITIZE_THREAD)
    endif()

    include(Helpers)
    find_package_switchable(
        ASan
        OPTION ${ARG_ASAN_OPTION}
        PURPOSE "Address sanitizer"
    )
    find_package_switchable(
        LSan
        OPTION ${ARG_LSAN_OPTION}
        PURPOSE "Leak sanitizer"
    )
    find_package_switchable(
        MSan
        OPTION ${ARG_MSAN_OPTION}
        DEFAULT OFF
        PURPOSE "Memory sanitizer"
    )
    find_package_switchable(
        UBSan
        OPTION ${ARG_UBSAN_OPTION}
        PURPOSE "Undefined behavior sanitizer"
    )
    find_package_switchable(
        TSan
        OPTION ${ARG_TSAN_OPTION}
        DEFAULT OFF
        PURPOSE "Thread sanitizer"
    )

    set(${ARG_ASAN_OPTION}
        ${${ARG_ASAN_OPTION}}
        PARENT_SCOPE
    )
    set(${ARG_LSAN_OPTION}
        ${${ARG_LSAN_OPTION}}
        PARENT_SCOPE
    )
    set(${ARG_MSAN_OPTION}
        ${${ARG_MSAN_OPTION}}
        PARENT_SCOPE
    )
    set(${ARG_UBSAN_OPTION}
        ${${ARG_UBSAN_OPTION}}
        PARENT_SCOPE
    )
    set(${ARG_TSAN_OPTION}
        ${${ARG_TSAN_OPTION}}
        PARENT_SCOPE
    )
endfunction()

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
# @param ASAN      Enable `AddressSanitizer` (`libasan`)
# @param LSAN      Enable `LeakSanitizer` (`liblsan`)
# @param MSAN      Enable `MemorySanitizer` (`libmsan`)
# @param UBSAN     Enable `UndefinedBehaviorSanitizer` (`libubsan`)
# @param TSAN      Enable `ThreadSanitizer` (`libtsan`)
# [/cmake_documentation]
function(target_enable_sanitizers targetName)
    set(options "")
    set(oneValueArgs ASAN LSAN MSAN UBSAN TSAN)
    set(multipleValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multipleValueArgs}" ${ARGN})

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

    if((ARG_LSAN OR ARG_MSAN) AND ARG_TSAN)
        message(
            FATAL_ERROR "ThreadSanitizer is not compatible with MemorySanitizer or LeakSanitizer."
        )
    endif()

    if(ARG_ASAN AND (ARG_TSAN OR ARG_MSAN))
        message(
            FATAL_ERROR
                "AddressSanitizer is not compatible with ThreadSanitizer or MemorySanitizer."
        )
    endif()

    # Enable AddressSanitizer
    if(ARG_ASAN)
        sanitizer_add_flags(${targetName} "${target_compiler}" "ASan")
    endif()

    # Enable UndefinedBehaviorSanitizer
    if(ARG_UBSAN)
        sanitizer_add_flags(${targetName} "${target_compiler}" "UBSan")
    endif()

    # Enable LeakSanitizer
    if(ARG_LSAN)
        sanitizer_add_flags(${targetName} "${target_compiler}" "LSan")
    endif()

    # Enable MemorySanitizer
    if(ARG_MSAN)
        sanitizer_add_flags(${targetName} "${target_compiler}" "MSan")
    endif()

    # Enable ThreadSanitizer
    if(ARG_TSAN)
        sanitizer_add_flags(${targetName} "${target_compiler}" "TSan")
    endif()
endfunction()
