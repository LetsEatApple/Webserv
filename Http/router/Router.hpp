#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Response.hpp"
#include <string>
#include <vector>

class Request;

class Router {
public:
    typedef void (*HandlerFunc)(Response::Writer& w, const Request& req);

    Router();

    void get(const std::string& path, HandlerFunc handler);
    void prefix(const std::string& pathPrefix, HandlerFunc handler);
    void setDefault(HandlerFunc handler);

    void process(Response::Writer& w, const Request& req);

private:
    struct Route {
        std::string method;
        std::string path;
        bool isPrefix;
        HandlerFunc handler;
    };

    std::vector<Route> routes;
    HandlerFunc defaultHandler;
};

#endif
