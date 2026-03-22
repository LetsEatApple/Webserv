#include <catch2/catch_test_macros.hpp>
#include <string>
#include <cstring>
#include <ctime>
#include "Response.hpp"
#include "ResponseDispatcher.hpp"
#include "routing.hpp"
#include "test_helpers.hpp"

TEST_CASE("writeChunkedBody writes correct wire format", "[response][chunked]") {
    std::string output;
    Response::Writer w(output);
    const char data[] = "Hello";
    w.writeChunkedBody(data, 5);

    CHECK(output == "5\r\nHello\r\n");
}

TEST_CASE("writeChunkedBodyDone writes terminal chunk", "[response][chunked]") {
    std::string output;
    Response::Writer w(output);
    w.writeChunkedBodyDone();

    CHECK(output == "0\r\n\r\n");
}

TEST_CASE("Multiple chunks in sequence", "[response][chunked]") {
    std::string output;
    Response::Writer w(output);
    w.writeChunkedBody("Hi", 2);
    w.writeChunkedBody("World!", 6);
    w.writeChunkedBodyDone();

    CHECK(output == "2\r\nHi\r\n6\r\nWorld!\r\n0\r\n\r\n");
}

TEST_CASE("Date header is present and correctly formatted", "[response][date]") {
    config::Location loc;
    loc.path = "/";
    loc.autoindex = false;
    loc.client_max_body_size = 0;
    loc.ret.insert(std::make_pair(200, "ok"));

    config::Server srv;
    srv.locations.push_back(loc);
    std::vector<config::Server> servers;
    servers.push_back(srv);

    std::string out;
    Response::Writer w(out);
    Request req = makeRequest("GET", "/");
    config::Location found;
    REQUIRE(findLocation(servers, req, found));
    ResponseDispatcher(w, req, found).serveReturn();

    std::string date = extractHeader(out, "date");
    REQUIRE_FALSE(date.empty());

    // IMF-fixdate: "Wed, 19 Feb 2026 14:30:00 GMT"
    // length is always 29
    CHECK(date.size() == 29);
    CHECK(date[3] == ',');
    CHECK(date[4] == ' ');
    CHECK(date.substr(date.size() - 3) == "GMT");

    // verify it parses back to a valid time
    struct std::tm tm;
    std::memset(&tm, 0, sizeof(tm));
    CHECK(strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm) != NULL);
}
