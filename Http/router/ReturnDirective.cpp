#include "ReturnDirective.hpp"
#include "Request.hpp"

static Response::StatusCode toStatusCode(int code) {
	switch (code) {
		case 200: return Response::StatusOk;
		case 301: return Response::StatusMovedPermanently;
		case 302: return Response::StatusFound;
		case 400: return Response::StatusBadRequest;
		case 404: return Response::StatusNotFound;
		case 405: return Response::StatusMethodNotAllowed;
		case 500: return Response::StatusInternalServerError;
		case 501: return Response::StatusNotImplemented;
		default:  return Response::StatusNotImplemented;
	}
}

static const config::Location* matchLocation(const config::Server& server, const std::string& target) {
	const config::Location* matched = NULL;
	for (size_t i = 0; i < server.locations.size(); ++i) {
		const config::Location& loc = server.locations[i];
		if (target.compare(0, loc.path.size(), loc.path) == 0) {
			if (matched == NULL || loc.path.size() > matched->path.size())
				matched = &loc;
		}
	}
	return matched;
}

bool serveReturn(Response::Writer& w, const Request& req, const std::vector<config::Server>& servers) {
	if (servers.empty())
		return false;

	const config::Server& server = servers[0];
	const config::Location* loc = matchLocation(server, req.getTarget());
	if (loc == NULL || loc->ret.empty())
		return false;

	std::map<int, std::string>::const_iterator it = loc->ret.begin();
	int code = it->first;
	const std::string& value = it->second;
	Response::StatusCode sc = toStatusCode(code);

	w.writeStatusLine(sc);
	if (code == 301 || code == 302) {
		Headers h = Response::getDefaultHeaders(0);
		h.set("Location", value);
		w.writeHeaders(h);
	} else {
		w.writeHeaders(Response::getDefaultHeaders(static_cast<int>(value.size())));
		w.writeBody(value.c_str(), value.size());
	}
	return true;
}
