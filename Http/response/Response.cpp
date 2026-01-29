#include "Response.hpp"
#include <cstring>
#include <cstdio>
#include <sstream>
#include <unistd.h>

struct HeaderAppender {
    std::string& buf;
    HeaderAppender(std::string& b) : buf(b) {}
    void operator()(const std::string& name, const std::string& value) const {
        buf += name + ": " + value + "\r\n";
    }
};

Headers Response::getDefaultHeaders(int contentLen) {
    Headers h;
    std::ostringstream oss;
    oss << contentLen;
    h.set("Content-Length", oss.str());
    h.set("Connection", "close");
    h.set("Content-Type", "text/plain");
    return h;
}

Response::Writer::Writer(int fd) : fd(fd) {}

bool Response::Writer::writeStatusLine(StatusCode statusCode) const {
    const char* statusLine = NULL;
    switch (statusCode) {
        case StatusOk:
            statusLine = "HTTP/1.1 200 OK\r\n";
            break;
        case StatusBadRequest:
            statusLine = "HTTP/1.1 400 Bad Request\r\n";
            break;
        case StatusInternalServerError:
            statusLine = "HTTP/1.1 500 Internal Server Error\r\n";
            break;
        default:
            return false;
    }
    size_t len = std::strlen(statusLine);
    ssize_t n = ::write(fd, statusLine, len);
    return n == static_cast<ssize_t>(len);
}

bool Response::Writer::writeHeaders(const Headers& h) const {
    std::string buf;
    h.forEach(HeaderAppender(buf));
    buf += "\r\n";
    ssize_t n = ::write(fd, buf.c_str(), buf.size());
    return n == static_cast<ssize_t>(buf.size());
}

bool Response::Writer::writeBody(const char* data, size_t len) const {
    ssize_t n = ::write(fd, data, len);
    return n == static_cast<ssize_t>(len);
}

bool Response::Writer::writeChunkedBody(const char* data, size_t len) const {
    char hexBuf[32];
    int hexLen = std::sprintf(hexBuf, "%x\r\n", static_cast<unsigned int>(len));
    if (!writeBody(hexBuf, hexLen)) {
        return false;
    }
    if (!writeBody(data, len)) {
        return false;
    }
    return writeBody("\r\n", 2);
}

bool Response::Writer::writeChunkedBodyDone() const {
    return writeBody("0\r\n\r\n", 5);
}
