#include "ReturnDirective.hpp"
#include <sstream>
#include <cstring>

static bool isRedirect(int code) {
	return code == 301 || code == 302 || code == 307 || code == 308;
}

bool serveReturn(Response::Writer& w, const Request& req) {
	config::Location loc = req.getLocation();
	if (loc.ret.empty())
		return false;

	std::map<int, std::string>::const_iterator it = loc.ret.begin();
	int code = it->first;
	const std::string& value = it->second;

	Response::StatusCode status = Response::intToStatusCode(code);
	w.writeStatusLine(status);

	if (isRedirect(code)) {
		Headers h;
		h.set("Content-Length", "0");
		h.set("Connection", "close");
		h.set("location", value);
		w.writeHeaders(h);
	} else {
		w.writeHeaders(Response::getDefaultHeaders(value.size()));
		if (req.getMethod() != "HEAD")
			w.writeBody(value.c_str(), value.size());
	}
	return true;
}
