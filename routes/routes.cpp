#include "routes.hpp"
#include "Headers.hpp"

#include <ctime>
#include <sstream>
#include <cstring>
#include <iostream>

// Helper: convert number to string (C++98)
static std::string toString(long n) {
	std::ostringstream oss;
	oss << n;
	return oss.str();
}

// Helper functor for forEachHeader (C++98 compatible)
struct AppendHeader {
	std::string* out;
	AppendHeader(std::string* s) : out(s) {}
	void operator()(const std::string& name, const std::string& value) const {
		*out += "  " + name + ": " + value + "\n";
	}
};

// Middleware: logs every request
bool logRequest(Response::Writer& w, const Request& req) {
	(void)w;
	std::cout << req.getMethod() << " " << req.getTarget() << std::endl;
	return true;
}

// GET /
void handleHome(Response::Writer& w, const Request& req) {
	(void)req;
	const char* body = "<html><body><h1>Welcome Home</h1></body></html>";
	int bodyLen = static_cast<int>(std::strlen(body));
	Headers h = Response::getDefaultHeaders(bodyLen);
	h.replace("content-type", "text/html");
	w.writeStatusLine(Response::StatusOk);
	w.writeHeaders(h);
	w.writeBody(body, bodyLen);
}

// GET /health
void handleHealth(Response::Writer& w, const Request& req) {
	(void)req;
	const char* body = "{\"status\":\"ok\"}";
	int bodyLen = static_cast<int>(std::strlen(body));
	Headers h = Response::getDefaultHeaders(bodyLen);
	h.replace("content-type", "application/json");
	w.writeStatusLine(Response::StatusOk);
	w.writeHeaders(h);
	w.writeBody(body, bodyLen);
}

// GET /echo - returns request headers as response
void handleEcho(Response::Writer& w, const Request& req) {
	std::string body = "Method: " + req.getMethod() + "\n";
	body += "Target: " + req.getTarget() + "\n";
	body += "Headers:\n";
	req.forEachHeader(AppendHeader(&body));

	int bodyLen = static_cast<int>(body.size());
	Headers h = Response::getDefaultHeaders(bodyLen);
	h.replace("content-type", "text/plain");
	w.writeStatusLine(Response::StatusOk);
	w.writeHeaders(h);
	w.writeBody(body.c_str(), bodyLen);
}

// PREFIX /api/* - simple JSON API
void handleApi(Response::Writer& w, const Request& req) {
	std::string path = req.getTarget();
	std::string body;

	if (path == "/api/users") {
		body = "[{\"id\":1,\"name\":\"Alice\"},{\"id\":2,\"name\":\"Bob\"}]";
	} else if (path == "/api/time") {
		body = "{\"timestamp\":" + toString(std::time(NULL)) + "}";
	} else {
		body = "{\"error\":\"not found\",\"path\":\"" + path + "\"}";
	}

	int bodyLen = static_cast<int>(body.size());
	Headers h = Response::getDefaultHeaders(bodyLen);
	h.replace("content-type", "application/json");
	w.writeStatusLine(Response::StatusOk);
	w.writeHeaders(h);
	w.writeBody(body.c_str(), bodyLen);
}

// Default 404 handler
void handleNotFound(Response::Writer& w, const Request& req) {
	(void)req;
	const char* body = "<html><body><h1>404 Not Found</h1></body></html>";
	int bodyLen = static_cast<int>(std::strlen(body));
	Headers h = Response::getDefaultHeaders(bodyLen);
	h.replace("content-type", "text/html");
	w.writeStatusLine(Response::StatusBadRequest);
	w.writeHeaders(h);
	w.writeBody(body, bodyLen);
}
