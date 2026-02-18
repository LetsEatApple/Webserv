#include <catch2/catch_test_macros.hpp>
#include <string>
#include <cstring>
#include "Response.hpp"
#include "test_helpers.hpp"

TEST_CASE("writeChunkedBody writes correct wire format", "[response][chunked]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);

    Response::Writer w(fds[1]);
    const char data[] = "Hello";
    REQUIRE(w.writeChunkedBody(data, 5));
    close(fds[1]);

    std::string output = readAll(fds[0]);
    close(fds[0]);

    CHECK(output == "5\r\nHello\r\n");
}

TEST_CASE("writeChunkedBodyDone writes terminal chunk", "[response][chunked]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);

    Response::Writer w(fds[1]);
    REQUIRE(w.writeChunkedBodyDone());
    close(fds[1]);

    std::string output = readAll(fds[0]);
    close(fds[0]);

    CHECK(output == "0\r\n\r\n");
}

TEST_CASE("Multiple chunks in sequence", "[response][chunked]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);

    Response::Writer w(fds[1]);
    REQUIRE(w.writeChunkedBody("Hi", 2));
    REQUIRE(w.writeChunkedBody("World!", 6));
    REQUIRE(w.writeChunkedBodyDone());
    close(fds[1]);

    std::string output = readAll(fds[0]);
    close(fds[0]);

    CHECK(output == "2\r\nHi\r\n6\r\nWorld!\r\n0\r\n\r\n");
}
