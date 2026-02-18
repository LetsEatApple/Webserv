#include <catch2/catch_test_macros.hpp>
#include <sys/stat.h>
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

TEST_CASE("serve HTML file has correct content-type", "[static]") {
	TempDir dir;
	TempFile file(dir.path + "/page.html", "<html>hi</html>");

	Request req = makeRequest("GET", "/page.html");
	config::Location loc = makeFileLocation(dir.path);

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(out.find("text/html") != std::string::npos);
	CHECK(extractBody(out) == "<html>hi</html>");
}

TEST_CASE("serve nested path", "[static]") {
	TempDir dir;
	std::string subdir = dir.path + "/sub";
	mkdir(subdir.c_str(), 0755);
	TempFile file(subdir + "/deep.txt", "deep content");

	Request req = makeRequest("GET", "/sub/deep.txt");
	config::Location loc = makeFileLocation(dir.path);

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(extractBody(out) == "deep content");
	rmdir(subdir.c_str());
}

TEST_CASE("non-root location strips prefix", "[static]") {
	TempDir dir;
	TempFile file(dir.path + "/hello.txt", "hello world");

	Request req = makeRequest("GET", "/files/hello.txt");
	config::Location loc = makeFileLocation(dir.path, "/files");

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(extractBody(out) == "hello world");
}

TEST_CASE("HEAD static file has empty body", "[static]") {
	TempDir dir;
	TempFile file(dir.path + "/hello.txt", "hello world");

	Request req = makeRequest("HEAD", "/hello.txt");
	config::Location loc = makeFileLocation(dir.path);

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(extractStatusLine(out).find("200 OK") != std::string::npos);
	CHECK(out.find("content-length: 11") != std::string::npos);
	CHECK(extractBody(out).empty());
}

TEST_CASE("DELETE existing file returns 200", "[delete]") {
	TempDir dir;
	TempFile file(dir.path + "/trash.txt", "delete me");

	Request req = makeRequest("DELETE", "/trash.txt");
	config::Location loc = makeFileLocation(dir.path);

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveDelete() == 200);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(extractStatusLine(out).find("200 OK") != std::string::npos);

	struct stat st;
	CHECK(stat((dir.path + "/trash.txt").c_str(), &st) < 0);
}

TEST_CASE("DELETE nonexistent file returns 404", "[delete]") {
	TempDir dir;

	Request req = makeRequest("DELETE", "/nope.txt");
	config::Location loc = makeFileLocation(dir.path);

	int fds[2];
	REQUIRE(pipe(fds) == 0);
	Response::Writer w(fds[1]);
	CHECK(ResponseDispatcher(w, req, loc).serveDelete() == 404);
	close(fds[1]);

	std::string out = readAll(fds[0]);
	close(fds[0]);
	CHECK(extractStatusLine(out).find("404 Not Found") != std::string::npos);
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
