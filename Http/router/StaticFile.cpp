#include "StaticFile.hpp"

bool serveFile(Response::Writer& w, const Request& req) {
	(void)w;
	(void)req;
	return false;
}
