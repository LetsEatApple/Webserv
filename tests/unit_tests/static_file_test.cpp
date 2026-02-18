#include <catch2/catch_test_macros.hpp>
#include "MimeTypes.hpp"

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
