#ifndef RESPONSE_DISPATCHER_HPP
#define RESPONSE_DISPATCHER_HPP

#include <ctime>

#include "Response.hpp"
#include "Request.hpp"
#include "Config.hpp"

// Each serve method writes a complete HTTP response and returns the status code it sent.
class ResponseDispatcher {
public:
	ResponseDispatcher(Response::Writer& w, const Request& req, const config::Location& loc);
	int serve(int* fileFd = 0, size_t* fileSize = 0);
	int serveReturn();
	int serveFile(int* fileFd = 0, size_t* fileSize = 0);
	int serveDelete();
	int serveUpload();
	int sendError(Response::StatusCode code);
	int serveAutoindex(const std::string &fsPath, const std::string &reqPath, const std::string &reqMethod);

	struct dirEntry
	{
		std::string name;
		time_t mod_time;
		long int size;
	};

private:
	Response::Writer&		_w;
	const Request&			_req;
	const config::Location&	_loc;
};

#endif
