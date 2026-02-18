#include <catch2/catch_test_macros.hpp>
#include "MimeTypes.hpp"
#include "ResponseDispatcher.hpp"
#include "test_helpers.hpp"

static config::Location makeFileLocation(const std::string& alias,
										 const std::string& path = "/") {
	config::Location loc;
	loc.path = path;
	loc.alias = alias;
	loc.index = "index.html";
	loc.autoindex = false;
	loc.client_max_body_size = 0;
	return loc;
}

TEST_CASE("getMimeType returns correct MIME type", "[mime]") {
	CHECK(getMimeType("style.css") == "text/css");
	CHECK(getMimeType("index.html") == "text/html");
	CHECK(getMimeType("page.htm") == "text/html");
	CHECK(getMimeType("app.js") == "application/javascript");
	CHECK(getMimeType("data.json") == "application/json");
	CHECK(getMimeType("readme.txt") == "text/plain");
	CHECK(getMimeType("photo.jpg") == "image/jpeg");
	CHECK(getMimeType("photo.jpeg") == "image/jpeg");
	CHECK(getMimeType("image.png") == "image/png");
	CHECK(getMimeType("icon.ico") == "image/x-icon");
	CHECK(getMimeType("doc.pdf") == "application/pdf");
	CHECK(getMimeType("unknown.xyz") == "application/octet-stream");
	CHECK(getMimeType("noextension") == "application/octet-stream");
}

TEST_CASE("serve regular file", "[static]") {
	TempDir dir;
	TempFile file(dir.path + "/hello.txt", "hello world");

	Request req = makeRequest("GET", "/hello.txt");
	config::Location loc = makeFileLocation(dir.path);

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(extractStatusLine(out).find("200 OK") != std::string::npos);
	CHECK(out.find("text/plain") != std::string::npos);
	CHECK(extractBody(out) == "hello world");
}

TEST_CASE("serve nonexistent file returns 404", "[static]") {
	TempDir dir;

	Request req = makeRequest("GET", "/nope.txt");
	config::Location loc = makeFileLocation(dir.path);

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 404);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(extractStatusLine(out).find("404 Not Found") != std::string::npos);
}
