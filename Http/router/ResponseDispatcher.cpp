#include "ResponseDispatcher.hpp"
#include "MimeTypes.hpp"

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
	std::string fsPath = _loc.alias + _req.getPath();
	(void)_w;
	return 0;
}
