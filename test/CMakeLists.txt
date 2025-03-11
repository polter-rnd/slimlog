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
                                           ./test_${name}
        )
    else()
        add_test(NAME test_${name} COMMAND test_${name})
    endif()
    target_link_libraries(test_${name} PRIVATE Boost::ut slimlog::slimlog)
    target_compile_definitions(test_${name} PRIVATE BOOST_UT_DISABLE_MODULE)
    target_sources(test_${name} PRIVATE ${TEST_HELPERS})
endfunction()

slimlog_test(basics)

target_sources(test_basics
  PRIVATE

)
