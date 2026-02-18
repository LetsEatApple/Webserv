#include <catch2/catch_test_macros.hpp>
#include "ReturnDirective.hpp"
#include "test_helpers.hpp"

static std::vector<config::Server> oneReturn(const std::string& path, int code, const std::string& value) {
    config::Location loc;
    loc.path = path;
    loc.autoindex = false;
    loc.client_max_body_size = 0;
    loc.ret.insert(std::make_pair(code, value));
    config::Server srv;
    srv.locations.push_back(loc);
    std::vector<config::Server> servers;
    servers.push_back(srv);
    return servers;
}

TEST_CASE("return 200 with body", "[return]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);
    Response::Writer w(fds[1]);

    std::vector<config::Server> servers = oneReturn("/", 200, "hello");
    Request req = makeRequest("GET", "/");
    REQUIRE(findLocation(servers, req));
    REQUIRE(serveReturn(w, req));
    close(fds[1]);

    std::string out = readAll(fds[0]);
    close(fds[0]);
    std::string statusLine = extractStatusLine(out);
    std::string body = extractBody(out);
    CHECK(statusLine == "HTTP/1.1 200 OK");
    CHECK(body == "hello");
}

TEST_CASE("return 301 redirect", "[return]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);
    Response::Writer w(fds[1]);

    std::vector<config::Server> servers = oneReturn("/", 301, "/new");
    Request req = makeRequest("GET", "/old");
    REQUIRE(findLocation(servers, req));
    REQUIRE(serveReturn(w, req));
    close(fds[1]);

    std::string out = readAll(fds[0]);
    close(fds[0]);
    std::string statusLine = extractStatusLine(out);
    std::string body = extractBody(out);
    CHECK(statusLine == "HTTP/1.1 301 Moved Permanently");
    CHECK(out.find("location: /new\r\n") != std::string::npos);
    CHECK(body.empty());
}

TEST_CASE("unknown status code returns 501", "[return]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);
    Response::Writer w(fds[1]);

    std::vector<config::Server> servers = oneReturn("/", 418, "teapot");
    Request req = makeRequest("GET", "/");
    REQUIRE(findLocation(servers, req));
    REQUIRE(serveReturn(w, req));
    close(fds[1]);

    std::string out = readAll(fds[0]);
    close(fds[0]);
    std::string statusLine = extractStatusLine(out);
    std::string body = extractBody(out);
    CHECK(statusLine == "HTTP/1.1 501 Not Implemented");
    CHECK(body == "teapot");
}

TEST_CASE("no return directive falls through", "[return]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);
    Response::Writer w(fds[1]);

    config::Server srv;
    config::Location loc;
    loc.path = "/";
    loc.autoindex = false;
    loc.client_max_body_size = 0;
    srv.locations.push_back(loc);
    std::vector<config::Server> servers;
    servers.push_back(srv);

    Request req = makeRequest("GET", "/");
    REQUIRE(findLocation(servers, req));
    CHECK_FALSE(serveReturn(w, req));
    close(fds[1]);
    close(fds[0]);
}

TEST_CASE("two listeners with different return bodies", "[return]") {
    std::vector<config::Server> port8081 = oneReturn("/", 200, "This is 8081");
    std::vector<config::Server> port8082 = oneReturn("/", 200, "This is 8082");

    int fds1[2];
    REQUIRE(pipe(fds1) == 0);
    Response::Writer w1(fds1[1]);
    Request req1 = makeRequest("GET", "/");
    REQUIRE(findLocation(port8081, req1));
    REQUIRE(serveReturn(w1, req1));
    close(fds1[1]);
    std::string out1 = readAll(fds1[0]);
    close(fds1[0]);

    int fds2[2];
    REQUIRE(pipe(fds2) == 0);
    Response::Writer w2(fds2[1]);
    Request req2 = makeRequest("GET", "/");
    REQUIRE(findLocation(port8082, req2));
    REQUIRE(serveReturn(w2, req2));
    close(fds2[1]);
    std::string out2 = readAll(fds2[0]);
    close(fds2[0]);

    std::string body1 = extractBody(out1);
    std::string body2 = extractBody(out2);
    CHECK(body1 == "This is 8081");
    CHECK(body2 == "This is 8082");
}

TEST_CASE("redirect from /old to /new with return 200", "[return]") {
    config::Location oldLoc;
    oldLoc.path = "/old";
    oldLoc.autoindex = false;
    oldLoc.client_max_body_size = 0;
    oldLoc.ret.insert(std::make_pair(301, "/new"));

    config::Location newLoc;
    newLoc.path = "/new";
    newLoc.autoindex = false;
    newLoc.client_max_body_size = 0;
    newLoc.ret.insert(std::make_pair(200, "you made it"));

    config::Server srv;
    srv.locations.push_back(oldLoc);
    srv.locations.push_back(newLoc);
    std::vector<config::Server> servers;
    servers.push_back(srv);

    // First request: GET /old -> 301 redirect to /new
    int fds1[2];
    REQUIRE(pipe(fds1) == 0);
    Response::Writer w1(fds1[1]);
    Request req1 = makeRequest("GET", "/old");
    REQUIRE(findLocation(servers, req1));
    REQUIRE(serveReturn(w1, req1));
    close(fds1[1]);
    std::string out1 = readAll(fds1[0]);
    close(fds1[0]);

    std::string statusLine1 = extractStatusLine(out1);
    CHECK(statusLine1 == "HTTP/1.1 301 Moved Permanently");
    CHECK(out1.find("location: /new\r\n") != std::string::npos);

    // Second request: GET /new -> 200 with body
    int fds2[2];
    REQUIRE(pipe(fds2) == 0);
    Response::Writer w2(fds2[1]);
    Request req2 = makeRequest("GET", "/new");
    REQUIRE(findLocation(servers, req2));
    REQUIRE(serveReturn(w2, req2));
    close(fds2[1]);
    std::string out2 = readAll(fds2[0]);
    close(fds2[0]);

    std::string statusLine2 = extractStatusLine(out2);
    std::string body2 = extractBody(out2);
    CHECK(statusLine2 == "HTTP/1.1 200 OK");
    CHECK(body2 == "you made it");
}

TEST_CASE("longest prefix match wins", "[return]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);
    Response::Writer w(fds[1]);

    config::Location root;
    root.path = "/";
    root.autoindex = false;
    root.client_max_body_size = 0;
    root.ret.insert(std::make_pair(200, "root"));

    config::Location api;
    api.path = "/api";
    api.autoindex = false;
    api.client_max_body_size = 0;
    api.ret.insert(std::make_pair(200, "api"));

    config::Server srv;
    srv.locations.push_back(root);
    srv.locations.push_back(api);
    std::vector<config::Server> servers;
    servers.push_back(srv);

    Request req = makeRequest("GET", "/api/users");
    REQUIRE(findLocation(servers, req));
    REQUIRE(serveReturn(w, req));
    close(fds[1]);

    std::string out = readAll(fds[0]);
    close(fds[0]);
    std::string statusLine = extractStatusLine(out);
    std::string body = extractBody(out);
    CHECK(statusLine == "HTTP/1.1 200 OK");
    CHECK(body == "api");
}

TEST_CASE("HEAD return 200 has empty body", "[return]") {
    int fds[2];
    REQUIRE(pipe(fds) == 0);
    Response::Writer w(fds[1]);

    std::vector<config::Server> servers = oneReturn("/", 200, "hello");
    Request req = makeRequest("HEAD", "/");
    REQUIRE(findLocation(servers, req));
    REQUIRE(serveReturn(w, req));
    close(fds[1]);

    std::string out = readAll(fds[0]);
    close(fds[0]);
    CHECK(extractStatusLine(out) == "HTTP/1.1 200 OK");
    CHECK(out.find("content-length: 5") != std::string::npos);
    CHECK(extractBody(out).empty());
}
