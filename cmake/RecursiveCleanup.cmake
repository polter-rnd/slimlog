# Script to recursively remove files and folders matching patterns from a directory
#
# Usage: cmake -DCLEANUP_DIR=<dir> -DCLEANUP_PATTERNS="*.gcda;..." -P RecursiveCleanup.cmake

if(NOT CLEANUP_DIR)
    message(FATAL_ERROR "CLEANUP_DIR is not specified")
endif()

if(NOT CLEANUP_PATTERNS)
    message(FATAL_ERROR "CLEANUP_PATTERNS is not specified")
endif()

# Build glob expressions for all patterns
unset(glob_expressions)
foreach(pattern IN LISTS CLEANUP_PATTERNS)
    list(APPEND glob_expressions "${CLEANUP_DIR}/${pattern}")
endforeach()

# Find all matching files recursively
file(GLOB_RECURSE to_remove ${glob_expressions})

# Remove found files and folders
if(to_remove)
    file(REMOVE_RECURSE ${to_remove})
    list(LENGTH to_remove num_files)
    message(VERBOSE "Removed ${num_files} item(s) from ${CLEANUP_DIR}")
endif()
