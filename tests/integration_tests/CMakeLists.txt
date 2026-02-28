add_test(NAME integration_tests
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_return.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME integration_static_file
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_static_file.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME integration_method
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_method.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME integration_head
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_head.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME integration_concurrency
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_concurrency.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME integration_cgi_zombie
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_cgi_zombie.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
