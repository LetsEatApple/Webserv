#ifndef TEST_HELPERS_HPP
#define TEST_HELPERS_HPP

#include <string>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include "Request.hpp"

struct TempDir {
    std::string path;

    TempDir() {
        char tmpl[] = "/tmp/webserv_test_XXXXXX";
        path = mkdtemp(tmpl);
    }

    ~TempDir() {
        rmdir(path.c_str());
    }
};

struct TempFile {
    std::string path;

    TempFile(const std::string& p, const std::string& content) : path(p) {
        std::ofstream f(path.c_str());
        f << content;
        f.close();
    }

    ~TempFile() {
        std::remove(path.c_str());
    }
};

inline std::string readAll(int fd) {
	std::string result;
	char buf[256];
	for (;;) {
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		result.append(buf, n);
	}
	return result;
}

inline Request makeRequest(const std::string& method, const std::string& target,
						   const std::string& host = "localhost") {
	Request req;
	std::string raw = method + " " + target + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
	std::string err;
	req.parse(raw, err);
	return req;
}


inline std::string extractStatusLine(const std::string& response) {
	return response.substr(0, response.find("\r\n"));
}

inline std::string extractBody(const std::string& response) {
	std::string::size_type pos = response.find("\r\n\r\n");
	if (pos == std::string::npos)
		return "";
	return response.substr(pos + 4);
}

#endif
