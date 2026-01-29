#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "RequestHandler.hpp"
#include <string>
#include <vector>

struct RouteHandler {
    virtual void handle(Response::Writer& w, const Request& req) = 0;
    virtual ~RouteHandler() {}
};

class Router : public RequestHandler {
public:
    typedef void (*HandlerFunc)(Response::Writer& w, const Request& req);
    typedef bool (*MiddlewareFunc)(Response::Writer& w, const Request& req);

    Router();
    ~Router();

    void use(MiddlewareFunc mw);

    void get(const std::string& path, HandlerFunc handler);
    void get(const std::string& path, RouteHandler& handler);

    void prefix(const std::string& pathPrefix, HandlerFunc handler);
    void prefix(const std::string& pathPrefix, RouteHandler& handler);

    void setDefault(HandlerFunc handler);
    void setDefault(RouteHandler& handler);

    void handle(Response::Writer& w, const Request& req);

private:
    struct FuncHandler : public RouteHandler {
        HandlerFunc func;
        FuncHandler(HandlerFunc f) : func(f) {}
        void handle(Response::Writer& w, const Request& req) { func(w, req); }
    };

    struct Route {
        std::string method;
        std::string path;
        bool isPrefix;
        RouteHandler* handler;
    };

    std::vector<MiddlewareFunc> middlewares;
    std::vector<Route> routes;
    RouteHandler* defaultHandler;
    std::vector<FuncHandler*> owned;

    RouteHandler* wrap(HandlerFunc f);
};

#endif
