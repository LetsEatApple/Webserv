set(TEST_MAIN "unit_tests")
set(TESTS_SOURCES
        headers_test.cpp
        request_test.cpp
        response_test.cpp
        return_directive_test.cpp
        config_lexer_test.cpp
        config_parser_test.cpp
        config_validator_test.cpp
        config_builder_test.cpp
)

add_executable(${TEST_MAIN} ${TESTS_SOURCES})
target_link_libraries(${TEST_MAIN} PUBLIC ${HTTP_LIBRARY} ${CONFIG_LIBRARY} ${REACTOR_LIBRARY} Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(${TEST_MAIN})
