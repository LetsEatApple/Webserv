#ifndef ROUTES_HPP
#define ROUTES_HPP

#include "../response/Response.hpp"
#include "../request/Request.hpp"

// Middleware
bool logRequest(Response::Writer& w, const Request& req);

// Route handlers
void handleHome(Response::Writer& w, const Request& req);
void handleHealth(Response::Writer& w, const Request& req);
void handleEcho(Response::Writer& w, const Request& req);
void handleApi(Response::Writer& w, const Request& req);
void handleNotFound(Response::Writer& w, const Request& req);

#endif
