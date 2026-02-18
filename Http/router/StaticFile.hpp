#ifndef STATIC_FILE_HPP
#define STATIC_FILE_HPP

#include "Response.hpp"
#include "Request.hpp"

bool serveFile(Response::Writer& w, const Request& req);

#endif
