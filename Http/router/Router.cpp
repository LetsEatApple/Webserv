#include "Router.hpp"
#include "Request.hpp"

Router::Router() : defaultHandler(NULL) {}

Router::~Router() {
    for (size_t i = 0; i < owned.size(); i++) {
        delete owned[i];
    }
}

RouteHandler* Router::wrap(HandlerFunc f) {
    FuncHandler* h = new FuncHandler(f);
    owned.push_back(h);
    return h;
}

void Router::use(MiddlewareFunc mw) {
    middlewares.push_back(mw);
}

void Router::get(const std::string& path, HandlerFunc handler) {
    get(path, *wrap(handler));
}

void Router::get(const std::string& path, RouteHandler& handler) {
    Route r;
    r.method = "GET";
    r.path = path;
    r.isPrefix = false;
    r.handler = &handler;
    routes.push_back(r);
}

void Router::prefix(const std::string& pathPrefix, HandlerFunc handler) {
    prefix(pathPrefix, *wrap(handler));
}

void Router::prefix(const std::string& pathPrefix, RouteHandler& handler) {
    Route r;
    r.method = "";
    r.path = pathPrefix;
    r.isPrefix = true;
    r.handler = &handler;
    routes.push_back(r);
}

void Router::setDefault(HandlerFunc handler) {
    setDefault(*wrap(handler));
}

void Router::setDefault(RouteHandler& handler) {
    defaultHandler = &handler;
}

static bool startsWith(const std::string& str, const std::string& pfx) {
    if (str.size() < pfx.size()) return false;
    return str.compare(0, pfx.size(), pfx) == 0;
}

void Router::handle(Response::Writer& w, const Request& req) {
    for (size_t i = 0; i < middlewares.size(); i++) {
        if (!middlewares[i](w, req)) {
            return;
        }
    }

    for (size_t i = 0; i < routes.size(); i++) {
        const Route& r = routes[i];

        if (!r.method.empty() && r.method != req.getMethod()) {
            continue;
        }

        if (r.isPrefix) {
            if (startsWith(req.getTarget(), r.path)) {
                r.handler->handle(w, req);
                return;
            }
        } else {
            if (req.getTarget() == r.path) {
                r.handler->handle(w, req);
                return;
            }
        }
    }

    if (defaultHandler) {
        defaultHandler->handle(w, req);
    }
}
