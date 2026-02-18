#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

#include "Request.hpp"
#include "routing.hpp"
#include "test_helpers.hpp"

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

std::vector<config::Server> setServers()
{
	std::vector<config::Server> server;
	server.resize(2);

	server[0].server_names.resize(2);
	server[0].server_names[0] = "example.com";
	server[0].server_names[1] = "www.example.com";
	server[0].locations.resize(2);
	server[0].locations[0].path = "/upload";
	server[0].locations[0].alias = "/var/www";
	server[0].locations[0].autoindex = false;
	server[0].locations[0].client_max_body_size = 10485760;
	server[0].locations[0].index = "index.html";
	server[0].locations[1].path = "/";
	server[0].locations[1].alias = "/var/www";
	server[0].locations[1].autoindex = false;
	server[0].locations[1].client_max_body_size = 10485760;
	server[0].locations[1].index = "index.html";

	server[1].server_names.resize(2);
	server[1].server_names[0] = "example.com";
	server[1].server_names[1] = "www.example.com";
	server[1].locations.resize(2);
	server[1].locations[0].path = "/upload";
	server[1].locations[0].alias = "/var/www";
	server[1].locations[0].autoindex = false;
	server[1].locations[0].client_max_body_size = 10485760;
	server[1].locations[0].index = "index.html";
	server[1].locations[1].path = "/";
	server[1].locations[1].alias = "/var/www";
	server[1].locations[1].autoindex = false;
	server[1].locations[1].client_max_body_size = 10485760;
	server[1].locations[1].index = "index.html";

	return server;
}

TEST_CASE("valid request target"){
	std::string data = "GET /upload/cat.png HTTP/1.1\r\nHost: example.com\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    // Chunked read with 3 bytes per chunk
    Request* r = parseFromString(data, errorMsg, 3);
	std::vector<config::Server> servers = setServers();
	config::Location loc;
	bool status = findLocation(servers, *r, loc);
	CHECK(status == true);
	CHECK(r->getMethod() == "GET");
	CHECK(r->getTarget() == "/upload/cat.png");
	CHECK(r->getPath() == "/upload/cat.png");
	CHECK(r->getQuery().empty());
	CHECK(r->getHost() == "example.com");
	CHECK(loc.path == "/upload");

	delete r;
}