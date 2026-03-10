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


add_test(NAME integration_header_limit
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_header_limit.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_test(NAME integration_idle_timeout
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_idle_timeout.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(integration_idle_timeout PROPERTIES
    ENVIRONMENT "CLIENT_TIMEOUT_SEC=${CLIENT_TIMEOUT_SEC}"
)

add_test(NAME integration_cgi_zombie
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_cgi_zombie.py
        --build-dir ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

execute_process(
        COMMAND pip show requests
        RESULT_VARIABLE EXIT_CODE
        OUTPUT_QUIET
)

if (NOT ${EXIT_CODE} EQUAL 0)
    message(
            FATAL_ERROR
            "The \"requests\" Python3 package is not installed. Please install it using the following command: \"pip3 install requests\"."
    )
endif()

add_test(NAME integration_cookies
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_cookies.py
        --build-dir ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)