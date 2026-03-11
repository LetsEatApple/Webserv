#include <catch2/catch_test_macros.hpp>
#include <string>
#include "CgiResponse.hpp"

TEST_CASE("Parse simple CGI output", "[cgi]") {
    std::string cgiOutput = "Content-Type: text/html\r\n\r\nHello";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK(result.valid);
    CHECK(result.status == 200);
    CHECK(result.body == "Hello");
    CHECK(result.headers.get("Content-Type") == "text/html");
}

TEST_CASE("CGI Status header overrides default 200", "[cgi]") {
    std::string cgiOutput = "Status: 404 Not Found\r\nContent-Type: text/plain\r\n\r\noops";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK(result.valid);
    CHECK(result.status == 404);
    CHECK(result.body == "oops");
    // Status header should not be forwarded
    CHECK(result.headers.get("Status").empty());
}

TEST_CASE("CGI with LF-only separator (Perl style)", "[cgi]") {
    std::string cgiOutput = "Content-Type: text/html\n\nPerl output";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK(result.valid);
    CHECK(result.body == "Perl output");
}

TEST_CASE("CGI with Set-Cookie headers", "[cgi]") {
    std::string cgiOutput =
        "Content-Type: text/html\r\n"
        "Set-Cookie: a=1\r\n"
        "Set-Cookie: b=2\r\n"
        "\r\n"
        "cookies";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK(result.valid);
    CHECK(result.body == "cookies");
}

TEST_CASE("CGI with no header separator is invalid", "[cgi]") {
    std::string cgiOutput = "just some garbage with no headers";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK_FALSE(result.valid);
}

TEST_CASE("CGI with empty body", "[cgi]") {
    std::string cgiOutput = "Content-Type: text/html\r\n\r\n";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK(result.valid);
    CHECK(result.status == 200);
    CHECK(result.body.empty());
}

TEST_CASE("CGI Content-Length is stripped (server computes it)", "[cgi]") {
    std::string cgiOutput = "Content-Type: text/html\r\nContent-Length: 999\r\n\r\nhi";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK(result.valid);
    CHECK(result.headers.get("Content-Length").empty());
}

TEST_CASE("CGI default Content-Type is text/html", "[cgi]") {
    std::string cgiOutput = "Status: 200\r\n\r\nno content type";
    CgiResponse result = parseCgiOutput(cgiOutput);

    CHECK(result.valid);
    CHECK(result.headers.get("Content-Type") == "text/html");
}
