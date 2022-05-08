function(tut_add_test     TEST_NAME   TEST_SOURCES)
    set(TGT_NAME   "TGT_${TEST_NAME}")
    add_executable(${TGT_NAME} "${TEST_SOURCES}")
    set_target_properties(${TGT_NAME} PROPERTIES OUTPUT_NAME "${TEST_NAME}")
    target_link_libraries(${TGT_NAME}
        Boost::unit_test_framework
        Boost::system
        Boost::timer
        thread_supervisor
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()
