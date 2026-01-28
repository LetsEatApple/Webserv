#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "Response.hpp"

class Request;

class RequestHandler {
public:
    virtual void handle(Response::Writer& w, const Request& req) = 0;
    virtual ~RequestHandler() {}
};

#endif
