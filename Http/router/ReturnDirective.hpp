#ifndef RETURN_DIRECTIVE_HPP
#define RETURN_DIRECTIVE_HPP

#include "Response.hpp"
#include "Request.hpp"

bool serveReturn(Response::Writer& w, const Request& req);

#endif
