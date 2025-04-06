include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${PROJECT_BINARY_DIR}/deps)

fetchcontent_declare(
    mettle
    GIT_REPOSITORY https://github.com/polter-rnd/mettle.git
    EXCLUDE_FROM_ALL # don't build by default
    FIND_PACKAGE_ARGS # try find_package() first
)
fetchcontent_makeavailable(mettle)

if(ENABLE_SANITIZERS
   AND COMMAND target_enable_sanitizers
   AND TARGET libmettle
)
    target_enable_sanitizers(libmettle)
endif()

file(GLOB TEST_HELPERS ${PROJECT_SOURCE_DIR}/test/helpers/*.h)

function(slimlog_test name)
    add_executable(test_${name} ${name}.cpp)

    if(ENABLE_SANITIZERS AND COMMAND target_enable_sanitizers)
        target_enable_sanitizers(test_${name})
    endif()

    if(ENABLE_COVERAGE AND COMMAND target_enable_coverage)
        target_enable_coverage(test_${name})
    endif()

    if(ENABLE_ANALYZERS AND COMMAND target_enable_static_analysis)
        target_enable_static_analysis(
            test_${name}
            CLANG_TIDY_EXTRA_ARGS ${CLANG_TIDY_EXTRA_ARGS}
            IWYU_EXTRA_ARGS ${IWYU_EXTRA_ARGS} --check_also=${CMAKE_SOURCE_DIR}/test/*.h
            CPPCHECK_EXTRA_ARGS ${CPPCHECK_EXTRA_ARGS} --suppress=*:${CMAKE_BINARY_DIR}/*
        )
    endif()

    if(ENABLE_TEST_VALGRIND)
        add_test(NAME test_${name} COMMAND valgrind --leak-check=full --error-exitcode=1
                                           ./test_${name} --no-subproc -o xunit -f test_${name}.xml
        )
    else()
        add_test(NAME test_${name} COMMAND test_${name} --no-subproc -o xunit -f test_${name}.xml)
    endif()
    target_link_libraries(test_${name} PRIVATE mettle::mettle slimlog::slimlog)
    target_sources(test_${name} PRIVATE ${TEST_HELPERS})
endfunction()

slimlog_test(basic)
