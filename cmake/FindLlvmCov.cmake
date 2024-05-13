# [cmake_documentation] FindLlvmCov.cmake
#
# The module defines the following variables:
# @arg __FindLlvmCov_EXECUTABLE__: Path to clang-format executable
# @arg __FindLlvmCov_FOUND__: `TRUE` if the clang-format executable was found
# @arg __FindLlvmCov_VERSION_STRING__: The version of clang-format found
# @arg __FindLlvmCov_VERSION_MAJOR__: The clang-format major version
# @arg __FindLlvmCov_VERSION_MINOR__: The clang-format minor version
# @arg __FindLlvmCov_VERSION_PATCH__: The clang-format patch version
# [/cmake_documentation]

include(Helpers)
if(NOT LLVM_DIR AND NOT LLVM_ROOT)
set_versioned_compiler_names(
    LlvmCov
    COMPILER Clang
    NAMES llvm-cov
)
else()
set_directory_hints(LlvmCov HINTS LLVM_DIR LLVM_ROOT)
list(TRANSFORM LlvmCov_HINTS APPEND /bin)
unset(LlvmCov_EXECUTABLE CACHE)
endif()

find_program(
    LlvmCov_EXECUTABLE
    NAMES ${LlvmCov_NAMES} llvm-cov
    HINTS ${LlvmCov_HINTS}
    DOC "llvm-cov executable"
)

mark_as_advanced(LlvmCov_EXECUTABLE)

# Get the version string
if(LlvmCov_EXECUTABLE)
    execute_process(
        COMMAND ${LlvmCov_EXECUTABLE} -version
        OUTPUT_VARIABLE _LlvmCov_VERSION
        ERROR_VARIABLE _LlvmCov_VERSION
    )
    if(_LlvmCov_VERSION MATCHES "LLVM version ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(LlvmCov_VERSION_STRING "${CMAKE_MATCH_1}")

        string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" LlvmCov_VERSION_MAJOR
                             ${LlvmCov_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" LlvmCov_VERSION_MINOR
                             ${LlvmCov_VERSION_STRING}
        )
        string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" LlvmCov_VERSION_PATCH
                             ${LlvmCov_VERSION_STRING}
        )

        set(LlvmCov_VERSION_STRING
            ${LlvmCov_VERSION_STRING}
            PARENT_SCOPE
        )
        set(LlvmCov_VERSION_MAJOR
            ${LlvmCov_VERSION_MAJOR}
            PARENT_SCOPE
        )
        set(LlvmCov_VERSION_MINOR
            ${LlvmCov_VERSION_MINOR}
            PARENT_SCOPE
        )
        set(LlvmCov_VERSION_PATCH
            ${LlvmCov_VERSION_PATCH}
            PARENT_SCOPE
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LlvmCov
    REQUIRED_VARS LlvmCov_EXECUTABLE
    VERSION_VAR LlvmCov_VERSION_STRING
)
