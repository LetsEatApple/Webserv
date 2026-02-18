#ifndef RESPONSE_DISPATCHER_HPP
#define RESPONSE_DISPATCHER_HPP

#include "Response.hpp"
#include "Request.hpp"
#include "Config.hpp"

// Each serve method writes a complete HTTP response and returns the status code it sent.
class ResponseDispatcher {
public:
	ResponseDispatcher(Response::Writer& w, const Request& req, const config::Location& loc);
	int serve();
	int serveReturn();
	int serveFile();

private:
	Response::Writer&		_w;
	const Request&			_req;
	const config::Location&	_loc;
};

#endif
