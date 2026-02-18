#include "ResponseDispatcher.hpp"
#include "MimeTypes.hpp"
#include <sys/stat.h>

static std::string resolvePath(const config::Location& loc, const std::string& reqPath) {
	return loc.alias + reqPath.substr(loc.path.size());
}

static bool isRedirect(int code) {
	return code == 301 || code == 302 || code == 307 || code == 308;
}

ResponseDispatcher::ResponseDispatcher(Response::Writer& w, const Request& req, const config::Location& loc)
	: _w(w), _req(req), _loc(loc) {}

int ResponseDispatcher::serve() {
	if (!_loc.ret.empty())
		return serveReturn();
	return serveFile();
}

int ResponseDispatcher::serveReturn() {
	std::map<int, std::string>::const_iterator it = _loc.ret.begin();
	int code = it->first;
	const std::string& value = it->second;

	Response::StatusCode status = Response::intToStatusCode(code);
	_w.writeStatusLine(status);

	if (isRedirect(code)) {
		Headers h;
		h.set("Content-Length", "0");
		h.set("Connection", "close");
		h.set("location", value);
		_w.writeHeaders(h);
	} else {
		_w.writeHeaders(Response::getDefaultHeaders(value.size()));
		if (_req.getMethod() != "HEAD")
			_w.writeBody(value.c_str(), value.size());
	}
	return code;
}

int ResponseDispatcher::serveFile() {
	// 1. resolve filesystem path from alias + request path
    std::string fsPath = resolvePath(_loc, _req.getPath());
	struct stat st;
    // 2. stat() the path
    // 3. not found -> 404
	if (stat(fsPath.c_str(), &st) < 0) {
		_w.writeStatusLine(Response::StatusNotFound);
		_w.writeHeaders(Response::getDefaultHeaders(0));
		return 404;
	}
	// 4. extension matches _loc.cgi -> hand off to CGI
	// 5. regular file -> read, write status 200 + Content-Type from getMimeType()
	// 6. HEAD -> same headers, skip body
	// 7. return the status code sent

	return 0;
}
