#include <catch2/catch_test_macros.hpp>
#include "Headers.hpp"

TEST_CASE("Parse standard headers incrementally", "[headers]") {
    Headers headers;
    std::string data = "Host: example.com\r\nContent-Type: application/json\r\nContent-Length: 42\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 19);
    CHECK(result.done == false);

    result = headers.parse(data.substr(19));
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 32);
    CHECK(result.done == false);

    result = headers.parse(data.substr(51));
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 20);
    CHECK(result.done == false);

    result = headers.parse(data.substr(71));
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 2);
    CHECK(result.done == true);

    CHECK(headers.get("Host") == "example.com");
    CHECK(headers.get("Content-Type") == "application/json");
    CHECK(headers.get("Content-Length") == "42");
}

TEST_CASE("Empty Headers", "[headers]") {
    Headers headers;
    std::string data = "\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 2);
    CHECK(result.done == true);
    CHECK(headers.get("Host").empty());
}

TEST_CASE("Malformed header - missing colon separator", "[headers]") {
    Headers headers;
    std::string data = "Host localhost\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE_FALSE(result.error.empty());
    CHECK(result.bytesConsumed == 0);
    CHECK(result.done == false);
}

TEST_CASE("Malformed Header - space before colon", "[headers]") {
    Headers headers;
    std::string data = "Host : localhost\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE_FALSE(result.error.empty());
    CHECK(result.bytesConsumed == 0);
    CHECK(result.done == false);
}

TEST_CASE("Malformed Header - no space after colon", "[headers]") {
    Headers headers;
    std::string data = "Host:localhost\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE_FALSE(result.error.empty());
    CHECK(result.bytesConsumed == 0);
    CHECK(result.done == false);
}

TEST_CASE("Malformed Header - empty name", "[headers]") {
    Headers headers;
    std::string data = ": value\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE_FALSE(result.error.empty());
    CHECK(result.bytesConsumed == 0);
    CHECK(result.done == false);
}

TEST_CASE("Malformed Header - non-token character in name", "[headers]") {
    Headers headers;
    std::string data = "H\xC2\xA9st: localhost:42069\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE_FALSE(result.error.empty());
    CHECK(result.bytesConsumed == 0);
    CHECK(result.done == false);
}

TEST_CASE("Duplicate Headers", "[headers]") {
    Headers headers;
    std::string data = "Accept: text/html\r\nAccept: application/json\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 19);
    CHECK(result.done == false);
    CHECK(headers.get("Accept") == "text/html");

    result = headers.parse(data.substr(19));
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 26);
    CHECK(result.done == false);
    CHECK(headers.get("Accept") == "text/html, application/json");

    result = headers.parse(data.substr(45));
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 2);
    CHECK(result.done == true);
}

TEST_CASE("Case Insensitive Headers", "[headers]") {
    Headers headers;
    std::string data = "CONTENT-TYPE: text/plain\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 26);
    CHECK(result.done == false);

    CHECK(headers.get("CONTENT-TYPE") == "text/plain");
    CHECK(headers.get("content-type") == "text/plain");
    CHECK(headers.get("Content-Type") == "text/plain");
    CHECK(headers.get("CoNtEnT-TyPe") == "text/plain");
}

TEST_CASE("Missing End of Headers - no CRLF", "[headers]") {
    Headers headers;
    std::string data = "Host: localhost";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 0);
    CHECK(result.done == false);
}

TEST_CASE("Missing End of Headers - single header with CRLF", "[headers]") {
    Headers headers;
    std::string data = "Host: localhost\r\n";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 17);
    CHECK(result.done == false);
    CHECK(headers.get("Host") == "localhost");
}

TEST_CASE("Missing End of Headers - trailing lone CR", "[headers]") {
    Headers headers;
    std::string data = "Host: localhost\r\n\r";

    Headers::ParseResult result = headers.parse(data);
    REQUIRE(result.error.empty());
    CHECK(result.bytesConsumed == 17);
    CHECK(result.done == false);
}

TEST_CASE("Remove existing header", "[headers]") {
    Headers headers;
    std::string data = "Host: example.com\r\nContent-Type: text/plain\r\n\r\n";

    Headers::ParseResult result = headers.parse(data);
    result = headers.parse(data.substr(result.bytesConsumed));
    result = headers.parse(data.substr(19 + 26));

    CHECK(headers.get("Host") == "example.com");
    CHECK(headers.get("Content-Type") == "text/plain");

    headers.remove("Host");
    CHECK(headers.get("Host").empty());
    CHECK(headers.get("Content-Type") == "text/plain");
}

TEST_CASE("Remove non-existent header does not crash", "[headers]") {
    Headers headers;
    std::string data = "Host: example.com\r\n\r\n";

    headers.parse(data);
    headers.parse(data.substr(19));

    headers.remove("X-Missing");
    CHECK(headers.get("Host") == "example.com");
}

TEST_CASE("Remove is case-insensitive", "[headers]") {
    Headers headers;
    std::string data = "Content-Type: text/html\r\n\r\n";

    headers.parse(data);
    headers.parse(data.substr(25));

    headers.remove("CONTENT-TYPE");
    CHECK(headers.get("Content-Type").empty());
    CHECK(headers.get("content-type").empty());
}
