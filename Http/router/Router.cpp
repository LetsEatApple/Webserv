#include "Router.hpp"
#include "Request.hpp"

Router::Router() : defaultHandler(NULL) {}

void Router::get(const std::string& path, HandlerFunc handler) {
    Route r;
    r.method = "GET";
    r.path = path;
    r.isPrefix = false;
    r.handler = handler;
    routes.push_back(r);
}

void Router::prefix(const std::string& pathPrefix, HandlerFunc handler) {
    Route r;
    r.method = "";
    r.path = pathPrefix;
    r.isPrefix = true;
    r.handler = handler;
    routes.push_back(r);
}

void Router::setDefault(HandlerFunc handler) {
    defaultHandler = handler;
}

void Router::process(Response::Writer& w, const Request& req) {
    for (size_t i = 0; i < routes.size(); i++) {
        const Route& r = routes[i];

        if (!r.method.empty() && r.method != req.getMethod())
            continue;

        if (r.isPrefix) {
            if (req.getTarget().compare(0, r.path.size(), r.path) == 0) {
                r.handler(w, req);
                return;
            }
        } else {
            if (req.getTarget() == r.path) {
                r.handler(w, req);
                return;
            }
        }
    }

    if (defaultHandler)
        defaultHandler(w, req);
}
