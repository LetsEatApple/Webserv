#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#include "Request.hpp"

// Helper to create a socket pair and parse request from string
// chunkSize: if > 0, writes data in chunks of this size to simulate chunked network reads
static Request* parseFromString(const std::string& data, std::string& errorMsg, size_t chunkSize = 0) {
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        errorMsg = "socketpair failed";
        return NULL;
    }

    // Write data to one end (in chunks if chunkSize > 0)
    size_t totalWritten = 0;
    while (totalWritten < data.size()) {
        size_t toWrite = data.size() - totalWritten;
        if (chunkSize > 0 && toWrite > chunkSize) {
            toWrite = chunkSize;
        }
        ssize_t written = write(fds[1], data.c_str() + totalWritten, toWrite);
        if (written < 0) {
            close(fds[0]);
            close(fds[1]);
            errorMsg = "write failed";
            return NULL;
        }
        totalWritten += written;
    }
    close(fds[1]); // Close write end to signal EOF

    // Read and parse from the other end
    Request* r = Request::requestFromSocket(fds[0], errorMsg);
    close(fds[0]);
    return r;
}

TEST_CASE("Good GET Request line", "[request]") {
    std::string data = "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    // Chunked read with 3 bytes per chunk
    Request* r = parseFromString(data, errorMsg, 3);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "GET");
    CHECK(r->getTarget() == "/");
    CHECK(r->getHttpVersion() == "1.1");

    delete r;
}

TEST_CASE("Good GET Request line with path", "[request]") {
    std::string data = "GET /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    // Chunked read with 1 byte per chunk
    Request* r = parseFromString(data, errorMsg, 1);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "GET");
    CHECK(r->getTarget() == "/coffee");
    CHECK(r->getHttpVersion() == "1.1");

    delete r;
}

TEST_CASE("Invalid number of parts in request line", "[request]") {
    std::string data = "/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Good POST Request with path", "[request]") {
    std::string data = "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "POST");
    CHECK(r->getTarget() == "/submit");
    CHECK(r->getHttpVersion() == "1.1");

    delete r;
}

TEST_CASE("Good PUT request", "[request]") {
    std::string data = "PUT /resource HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "PUT");
    CHECK(r->getTarget() == "/resource");
    CHECK(r->getHttpVersion() == "1.1");

    delete r;
}

TEST_CASE("Good DELETE request", "[request]") {
    std::string data = "DELETE /resource/123 HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "DELETE");
    CHECK(r->getTarget() == "/resource/123");
    CHECK(r->getHttpVersion() == "1.1");

    delete r;
}

TEST_CASE("HEAD request", "[request]") {
    std::string data = "HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "HEAD");

    delete r;
}

TEST_CASE("OPTIONS request with asterisk", "[request]") {
    std::string data = "OPTIONS * HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "OPTIONS");
    CHECK(r->getTarget() == "*");

    delete r;
}

TEST_CASE("PATCH request", "[request]") {
    std::string data = "PATCH /resource HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "PATCH");

    delete r;
}

TEST_CASE("Request with query string", "[request]") {
    std::string data = "GET /search?q=hello&page=1 HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getMethod() == "GET");
    CHECK(r->getTarget() == "/search?q=hello&page=1");
    CHECK(r->getHttpVersion() == "1.1");

    delete r;
}

TEST_CASE("Request with fragment", "[request]") {
    std::string data = "GET /page#section HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getTarget() == "/page#section");

    delete r;
}

TEST_CASE("Request with deep nested path", "[request]") {
    std::string data = "GET /api/v1/users/123/posts/456/comments HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getTarget() == "/api/v1/users/123/posts/456/comments");

    delete r;
}

TEST_CASE("Request with URL-encoded characters in path", "[request]") {
    std::string data = "GET /path%20with%20spaces HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getTarget() == "/path%20with%20spaces");

    delete r;
}

// Invalid request line structure tests

TEST_CASE("Invalid - empty request", "[request]") {
    std::string data = "";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == "connection closed");
}

TEST_CASE("Invalid - no CRLF terminator", "[request]") {
    std::string data = "GET / HTTP/1.1";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == "connection closed");
}

TEST_CASE("Invalid - LF only terminator", "[request]") {
    // When request line uses LF instead of CRLF, parser finds next CRLF
    // and treats everything before it as the request line, which is malformed
    std::string data = "GET / HTTP/1.1\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid - too many spaces in request line", "[request]") {
    std::string data = "GET / extra HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid - only one part in request line", "[request]") {
    std::string data = "GET\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

// HTTP version validation tests

TEST_CASE("Invalid version in Request line - no slash", "[request]") {
    std::string data = "GET / HTTP1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - wrong prefix", "[request]") {
    std::string data = "GET / HTTPS/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid method (lowercase) Request line", "[request]") {
    std::string data = "GET / http/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - HTTP/1.0", "[request]") {
    std::string data = "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - HTTP/2.0", "[request]") {
    std::string data = "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - empty after slash", "[request]") {
    std::string data = "GET / HTTP/\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - extra dot", "[request]") {
    std::string data = "GET / HTTP/1.1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - multiple slashes", "[request]") {
    std::string data = "GET / HTTP/1/1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}


TEST_CASE("Standard Headers", "[request][headers]") {
    std::string data = "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    // Chunked read with 3 bytes per chunk (like Go test)
    Request* r = parseFromString(data, errorMsg, 3);

    REQUIRE(r != NULL);
    CHECK(r->getHeaders().get("host") == "localhost:42069");
    CHECK(r->getHeaders().get("user-agent") == "curl/7.81.0");
    CHECK(r->getHeaders().get("accept") == "*/*");

    delete r;
}

TEST_CASE("Headers are case-insensitive", "[request][headers]") {
    std::string data = "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getHeaders().get("HOST") == "localhost:42069");
    CHECK(r->getHeaders().get("Host") == "localhost:42069");
    CHECK(r->getHeaders().get("host") == "localhost:42069");
    CHECK(r->getHeaders().get("USER-AGENT") == "curl/7.81.0");

    delete r;
}

TEST_CASE("Malformed Header - missing colon", "[request][headers]") {
    std::string data = "GET / HTTP/1.1\r\nHost localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r == NULL);
}

TEST_CASE("Empty headers section", "[request][headers]") {
    std::string data = "GET / HTTP/1.1\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getHeaders().get("host").empty());

    delete r;
}

TEST_CASE("Multiple headers with same name", "[request][headers]") {
    std::string data = "GET / HTTP/1.1\r\nSet-Cookie: a=1\r\nSet-Cookie: b=2\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getHeaders().get("set-cookie") == "a=1, b=2");

    delete r;
}

TEST_CASE("Header value with leading/trailing whitespace", "[request][headers]") {
    std::string data = "GET / HTTP/1.1\r\nHost:   localhost:42069  \r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getHeaders().get("host") == "localhost:42069");

    delete r;
}

TEST_CASE("Standard Body", "[request][body]") {
    std::string data = "POST /submit HTTP/1.1\r\n"
                       "Host: localhost:42069\r\n"
                       "Content-Length: 13\r\n"
                       "\r\n"
                       "hello world!\n";
    std::string errorMsg;

    // Chunked read with 3 bytes per chunk
    Request* r = parseFromString(data, errorMsg, 3);

    REQUIRE(r != NULL);
    CHECK(r->getBody() == "hello world!\n");
    delete r;
}

TEST_CASE("Body shorter than reported content length", "[request][body]") {
    std::string data = "POST /submit HTTP/1.1\r\n"
                       "Host: localhost:42069\r\n"
                       "Content-Length: 20\r\n"
                       "\r\n"
                       "partial content";
    std::string errorMsg;

    // Chunked read with 3 bytes per chunk
    Request* r = parseFromString(data, errorMsg, 3);

    REQUIRE(r == NULL);
    CHECK(errorMsg == "connection closed");
}

TEST_CASE("Single chunked body", "[request][chunked]") {
    std::string data = "POST /upload HTTP/1.1\r\n"
                       "Host: localhost:42069\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "5\r\nHello\r\n0\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getBody() == "Hello");
    delete r;
}

TEST_CASE("Multiple chunks with hex sizes", "[request][chunked]") {
    // a = 10 in hex, 1a = 26 in hex
    std::string data = "POST /upload HTTP/1.1\r\n"
                       "Host: localhost:42069\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "a\r\n0123456789\r\n"
                       "1a\r\nabcdefghijklmnopqrstuvwxyz\r\n"
                       "0\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getBody() == "0123456789abcdefghijklmnopqrstuvwxyz");
    delete r;
}

TEST_CASE("Chunked body with 1-byte network reads", "[request][chunked]") {
    std::string data = "POST /upload HTTP/1.1\r\n"
                       "Host: localhost:42069\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "5\r\nHello\r\n"
                       "8\r\n World!\n\r\n"
                       "0\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg, 1);

    REQUIRE(r != NULL);
    CHECK(r->getBody() == "Hello World!\n");
    delete r;
}

TEST_CASE("Empty chunked body", "[request][chunked]") {
    std::string data = "POST /upload HTTP/1.1\r\n"
                       "Host: localhost:42069\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "0\r\n\r\n";
    std::string errorMsg;

    Request* r = parseFromString(data, errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->getBody().empty());
    delete r;
}
