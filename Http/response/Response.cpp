#include "Response.hpp"
#include <cstring>
#include <cstdio>
#include <sstream>
#include <ctime>

struct HeaderAppender {
	std::string& buf;
	explicit HeaderAppender(std::string& b) : buf(b) {}
	void operator()(const std::string& name, const std::string& value) const {
		buf += name + ": " + value + "\r\n";
	}
};

Headers Response::getDefaultHeaders(const size_t contentLen) {
	Headers h;
	std::ostringstream oss;
	oss << contentLen;
	h.set("Content-Length", oss.str());
	h.set("Connection", "keep-alive");
	h.set("Content-Type", "text/plain");
	const std::time_t now = std::time(NULL);
	char dateBuf[30];
	std::strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
	h.set("Date", dateBuf);
	return h;
}

std::string Response::getDefaultErrorPage(const StatusCode code) {
	const char* line = statusCodeToLine(code);
	// extract "404 Not Found" from "HTTP/1.1 404 Not Found\r\n"
	std::string reason(line + 9);
	reason.erase(reason.size() - 2); // strip \r\n
	return "<html><body><h1>" + reason + "</h1></body></html>";
}

Response::Writer::Writer(std::string& buf) : _buf(&buf) {}

const char* Response::statusCodeToLine(const StatusCode code) {
	switch (code) {
		case StatusOk:                return "HTTP/1.1 200 OK\r\n";
		case StatusNoContent:         return "HTTP/1.1 204 No Content\r\n";
		case StatusMovedPermanently:  return "HTTP/1.1 301 Moved Permanently\r\n";
		case StatusFound:             return "HTTP/1.1 302 Found\r\n";
		case StatusTemporaryRedirect: return "HTTP/1.1 307 Temporary Redirect\r\n";
		case StatusPermanentRedirect: return "HTTP/1.1 308 Permanent Redirect\r\n";
		case StatusBadRequest:        return "HTTP/1.1 400 Bad Request\r\n";
		case StatusForbidden:         return "HTTP/1.1 403 Forbidden\r\n";
		case StatusNotFound:          return "HTTP/1.1 404 Not Found\r\n";
		case StatusMethodNotAllowed:  return "HTTP/1.1 405 Method Not Allowed\r\n";
		case StatusRequestTimeout:   return "HTTP/1.1 408 Request Timeout\r\n";
		case StatusPayloadTooLarge:  return "HTTP/1.1 413 Payload Too Large\r\n";
		case StatusInternalServerError: return "HTTP/1.1 500 Internal Server Error\r\n";
		case StatusNotImplemented:    return "HTTP/1.1 501 Not Implemented\r\n"; 
		case StatusBadGateway:        return "HTTP/1.1 502 Bad Gateway\r\n";
		case StatusGatewayTimeout:  return "HTTP/1.1 504 Gateway Timeout\r\n";
		default:                      return "HTTP/1.1 500 Internal Server Error\r\n";
	}
}

Response::StatusCode Response::intToStatusCode(const int code) {
	switch (code) {
		case 200: return StatusOk;
		case 204: return StatusNoContent;
		case 301: return StatusMovedPermanently;
		case 302: return StatusFound;
		case 307: return StatusTemporaryRedirect;
		case 308: return StatusPermanentRedirect;
		case 400: return StatusBadRequest;
		case 403: return StatusForbidden;
		case 404: return StatusNotFound;
		case 405: return StatusMethodNotAllowed;
		case 408: return StatusRequestTimeout;
		case 413: return StatusPayloadTooLarge;
		case 500: return StatusInternalServerError;
		default:  return StatusNotImplemented;
	}
}

void Response::Writer::writeStatusLine(const StatusCode statusCode) const {
	const char* statusLine = statusCodeToLine(statusCode);
	_buf->append(statusLine, std::strlen(statusLine));
}

void Response::Writer::writeHeaders(const Headers& h) const {
	std::string buf;
	h.forEach(HeaderAppender(buf));
	buf += "\r\n";
	_buf->append(buf);
}

void Response::Writer::writeBody(const char* data, const size_t len) const {
	_buf->append(data, len);
}

void Response::Writer::writeChunkedBody(const char* data, const size_t len) const {
	char hexBuf[32];
	const int hexLen = std::sprintf(hexBuf, "%x\r\n", static_cast<unsigned int>(len));
	writeBody(hexBuf, static_cast<unsigned int>(hexLen));
	writeBody(data, len);
	writeBody("\r\n", 2);
}

void Response::Writer::writeChunkedBodyDone() const {
	writeBody("0\r\n\r\n", 5);
}
