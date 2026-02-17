add_test(NAME integration_tests
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_return.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
