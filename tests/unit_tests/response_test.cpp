#include <catch2/catch_test_macros.hpp>
#include <string>
#include <cstring>
#include "Response.hpp"
#include "test_helpers.hpp"

TEST_CASE("writeChunkedBody writes correct wire format", "[response][chunked]") {
    std::string output;
    Response::Writer w(output);
    const char data[] = "Hello";
    REQUIRE(w.writeChunkedBody(data, 5));

    CHECK(output == "5\r\nHello\r\n");
}

TEST_CASE("writeChunkedBodyDone writes terminal chunk", "[response][chunked]") {
    std::string output;
    Response::Writer w(output);
    REQUIRE(w.writeChunkedBodyDone());

    CHECK(output == "0\r\n\r\n");
}

TEST_CASE("Multiple chunks in sequence", "[response][chunked]") {
    std::string output;
    Response::Writer w(output);
    REQUIRE(w.writeChunkedBody("Hi", 2));
    REQUIRE(w.writeChunkedBody("World!", 6));
    REQUIRE(w.writeChunkedBodyDone());

    CHECK(output == "2\r\nHi\r\n6\r\nWorld!\r\n0\r\n\r\n");
}
