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

static config::Location makeLocationWithErrorPage(const std::string& alias,
												  int code, const std::string& pagePath) {
	config::Location loc = makeFileLocation(alias);
	loc.error_page[code] = pagePath;
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

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);

	CHECK(extractStatusLine(out).find("200 OK") != std::string::npos);
	CHECK(out.find("text/plain") != std::string::npos);
	CHECK(extractBody(out) == "hello world");
	CHECK(out.find("date: ") != std::string::npos);
}

TEST_CASE("serve HTML file has correct content-type", "[static]") {
	TempDir dir;
	TempFile file(dir.path + "/page.html", "<html>hi</html>");

	Request req = makeRequest("GET", "/page.html");
	config::Location loc = makeFileLocation(dir.path);

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);

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

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);

	CHECK(extractBody(out) == "deep content");
	rmdir(subdir.c_str());
}

TEST_CASE("non-root location strips prefix", "[static]") {
	TempDir dir;
	TempFile file(dir.path + "/hello.txt", "hello world");

	Request req = makeRequest("GET", "/files/hello.txt");
	config::Location loc = makeFileLocation(dir.path, "/files");

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);

	CHECK(extractBody(out) == "hello world");
}

TEST_CASE("HEAD static file has empty body", "[static]") {
	TempDir dir;
	TempFile file(dir.path + "/hello.txt", "hello world");

	Request req = makeRequest("HEAD", "/hello.txt");
	config::Location loc = makeFileLocation(dir.path);

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);

	CHECK(extractStatusLine(out).find("200 OK") != std::string::npos);
	CHECK(out.find("content-length: 11") != std::string::npos);
	CHECK(extractBody(out).empty());
}

TEST_CASE("DELETE existing file returns 204", "[delete]") {
	TempDir dir;
	TempFile file(dir.path + "/trash.txt", "delete me");

	Request req = makeRequest("DELETE", "/trash.txt");
	config::Location loc = makeFileLocation(dir.path);

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveDelete() == 204);

	CHECK(extractStatusLine(out).find("204 No Content") != std::string::npos);

	struct stat st;
	CHECK(stat((dir.path + "/trash.txt").c_str(), &st) < 0);
}

TEST_CASE("DELETE nonexistent file returns 404", "[delete]") {
	TempDir dir;

	Request req = makeRequest("DELETE", "/nope.txt");
	config::Location loc = makeFileLocation(dir.path);

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveDelete() == 404);

	CHECK(extractStatusLine(out).find("404 Not Found") != std::string::npos);
}

TEST_CASE("filename with double dots is allowed", "[static][security]") {
	TempDir dir;
	TempFile file(dir.path + "/archive.tar.gz", "tarball");

	Request req = makeRequest("GET", "/archive.tar.gz");
	config::Location loc = makeFileLocation(dir.path);

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 200);
	CHECK(extractBody(out) == "tarball");
}

TEST_CASE("serve nonexistent file returns 404", "[static]") {
	TempDir dir;

	Request req = makeRequest("GET", "/nope.txt");
	config::Location loc = makeFileLocation(dir.path);

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).serveFile() == 404);

	CHECK(extractStatusLine(out).find("404 Not Found") != std::string::npos);
	CHECK(out.find("date: ") != std::string::npos);
}

TEST_CASE("path traversal with .. is blocked", "[static][security]") {
	// Create /tmp/webserv_XXX/sub/ as document root with a file in the parent
	TempDir dir;
	std::string subdir = dir.path + "/sub";
	mkdir(subdir.c_str(), 0755);
	TempFile file(dir.path + "/secret.txt", "secret");

	// alias points to subdir, request tries to escape via ..
	// resolvePath: subdir + "/../secret.txt" = dir.path + "/sub/../secret.txt"
	// which the OS resolves to dir.path + "/secret.txt"
	Request req = makeRequest("GET", "/../secret.txt");
	config::Location loc = makeFileLocation(subdir);

	std::string out;
	Response::Writer w(out);
	int status = ResponseDispatcher(w, req, loc).serveFile();

	// should be 403 or 404, NOT 200
	CHECK(status != 200);
	CHECK(extractBody(out).find("secret") == std::string::npos);
	rmdir(subdir.c_str());
}

TEST_CASE("deep path traversal is blocked", "[static][security]") {
	TempDir dir;
	std::string subdir = dir.path + "/www";
	mkdir(subdir.c_str(), 0755);
	TempFile file(dir.path + "/outside.txt", "outside");

	Request req = makeRequest("GET", "/../../../../../../.." + dir.path + "/outside.txt");
	config::Location loc = makeFileLocation(subdir);

	std::string out;
	Response::Writer w(out);
	int status = ResponseDispatcher(w, req, loc).serveFile();

	CHECK(status != 200);
	CHECK(extractBody(out).find("outside") == std::string::npos);
	rmdir(subdir.c_str());
}

TEST_CASE("custom error page served on 404", "[error_page]") {
	TempDir dir;
	TempFile errorFile(dir.path + "/404.html", "<h1>Custom Not Found</h1>");

	Request req = makeRequest("GET", "/nope.txt");
	config::Location loc = makeLocationWithErrorPage(dir.path, 404, dir.path + "/404.html");

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).sendError(Response::StatusNotFound) == 404);

	CHECK(extractStatusLine(out).find("404 Not Found") != std::string::npos);
	CHECK(extractBody(out) == "<h1>Custom Not Found</h1>");
}

TEST_CASE("custom error page with missing file falls back to default", "[error_page]") {
	TempDir dir;

	Request req = makeRequest("GET", "/nope.txt");
	config::Location loc = makeLocationWithErrorPage(dir.path, 404, dir.path + "/nonexistent.html");

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).sendError(Response::StatusNotFound) == 404);

	CHECK(extractStatusLine(out).find("404 Not Found") != std::string::npos);
	CHECK(extractBody(out).find("404 Not Found") != std::string::npos);
}

TEST_CASE("no custom error page uses default", "[error_page]") {
	TempDir dir;

	Request req = makeRequest("GET", "/nope.txt");
	config::Location loc = makeFileLocation(dir.path);

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).sendError(Response::StatusNotFound) == 404);

	CHECK(extractBody(out).find("404 Not Found") != std::string::npos);
}

TEST_CASE("custom error page for 403", "[error_page]") {
	TempDir dir;
	TempFile errorFile(dir.path + "/403.html", "Forbidden!");

	Request req = makeRequest("GET", "/");
	config::Location loc = makeLocationWithErrorPage(dir.path, 403, dir.path + "/403.html");

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).sendError(Response::StatusForbidden) == 403);

	CHECK(extractBody(out) == "Forbidden!");
}

TEST_CASE("custom error page only matches its status code", "[error_page]") {
	TempDir dir;
	TempFile errorFile(dir.path + "/404.html", "Custom 404");

	Request req = makeRequest("GET", "/");
	config::Location loc = makeLocationWithErrorPage(dir.path, 404, dir.path + "/404.html");

	std::string out;
	Response::Writer w(out);
	CHECK(ResponseDispatcher(w, req, loc).sendError(Response::StatusBadRequest) == 400);

	// 400 should NOT use the 404 custom page
	CHECK(extractBody(out).find("Custom 404") == std::string::npos);
	CHECK(extractBody(out).find("400 Bad Request") != std::string::npos);
}
