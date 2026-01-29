# RequestHandler: The Bridge

`RequestHandler` is the **abstract interface** that decouples the Reactor (I/O layer) from the HTTP processing (application layer).

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           REACTOR LAYER                                 │
│  ┌─────────────┐                                                        │
│  │ Connection  │  Knows: socket fd, raw bytes, Request parser           │
│  │             │  Does NOT know: what to do with parsed request         │
│  └──────┬──────┘                                                        │
│         │                                                               │
│         │  _handler->handle(writer, request)                            │
│         │                                                               │
└─────────┼───────────────────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      RequestHandler (Interface)                         │
│                                                                         │
│   virtual void handle(Response::Writer& w, const Request& req) = 0;    │
│                                                                         │
│   INPUT:  Request (parsed HTTP: method, path, headers, body)           │
│   OUTPUT: Response::Writer (writes directly to socket)                  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
          │
          │  Router implements RequestHandler
          ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         APPLICATION LAYER                               │
│  ┌─────────────┐                                                        │
│  │   Router    │  Knows: routes, middleware, handlers                   │
│  │             │  Does NOT know: sockets, epoll, raw bytes              │
│  └─────────────┘                                                        │
└─────────────────────────────────────────────────────────────────────────┘
```

## The Interface

```cpp
// RequestHandler.hpp - Just 2 lines of essence
class RequestHandler {
public:
    virtual void handle(Response::Writer& w, const Request& req) = 0;
    virtual ~RequestHandler() {}
};
```

### Input: `const Request& req`

Fully parsed HTTP request with accessors:

| Method | Returns | Example |
|--------|---------|---------|
| `req.getMethod()` | HTTP method | `"GET"`, `"POST"` |
| `req.getTarget()` | Request path | `"/health"`, `"/api/users"` |
| `req.getHeaders()` | Headers object | `headers.get("host")` |
| `req.getBody()` | Request body | `"{\"name\":\"John\"}"` |

### Output: `Response::Writer& w`

Direct socket writer with methods:

| Method | Writes | Example Output |
|--------|--------|----------------|
| `w.writeStatusLine(200)` | Status line | `"HTTP/1.1 200 OK\r\n"` |
| `w.writeHeaders(h)` | Header lines | `"Content-Length: 15\r\n"` |
| `w.writeBody(data, len)` | Body bytes | `{"status":"ok"}` |

## Data Flow

```
      ┌──────────────────────────────────────────────────────────────┐
      │                     Connection::handleRead()                 │
      └──────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  1. RECEIVE RAW BYTES                                                   │
│     recv(fd, buf, 1024) → "GET /health HTTP/1.1\r\nHost: localhost\r\n" │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  2. PARSE INTO Request OBJECT                                           │
│     _request.parse(buffer) → Request {                                  │
│         method: "GET",                                                  │
│         target: "/health",                                              │
│         headers: { "host": "localhost" },                               │
│         body: ""                                                        │
│     }                                                                   │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  3. CREATE WRITER (wraps socket fd)                                     │
│     Response::Writer w(fd);                                             │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  4. DELEGATE TO RequestHandler                                          │
│     _handler->handle(w, _request);  ← THE BRIDGE                       │
│                                                                         │
│     Connection doesn't know what happens here!                          │
│     Could be Router, could be any RequestHandler implementation.        │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  5. INSIDE Router::handle()                                             │
│                                                                         │
│     a) Run middleware chain:                                            │
│        for each middleware:                                             │
│          if (!middleware(w, req)) return;  // short-circuit             │
│                                                                         │
│     b) Match route:                                                     │
│        for each route:                                                  │
│          if (matches(req.getTarget(), route.path))                      │
│            route.handler->handle(w, req);                               │
│            return;                                                      │
│                                                                         │
│     c) No match → default handler (404)                                 │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  6. HANDLER WRITES RESPONSE                                             │
│                                                                         │
│     void handleHealth(Response::Writer& w, const Request& req) {        │
│         w.writeStatusLine(Response::StatusOk);                          │
│         w.writeHeaders(headers);                                        │
│         w.writeBody("{\"status\":\"ok\"}", 15);                         │
│     }                                                                   │
│                                                                         │
│     Writer sends bytes directly to socket fd                            │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
      ┌──────────────────────────────────────────────────────────────┐
      │  7. Connection removes itself from Reactor                   │
      └──────────────────────────────────────────────────────────────┘
```

## Code Example

### Connection calls RequestHandler

```cpp
// handler.cpp - Connection::handleRead()
int Connection::handleRead() {
    char buf[MAX];
    ssize_t n = recv(_fd, buf, MAX - 1, MSG_DONTWAIT);

    // ... error handling ...

    _readBuffer.append(buf, n);
    int consumed = _request.parse(_readBuffer, errorMsg);

    // ... error handling ...

    if (_request.done()) {
        Response::Writer w(_fd);           // Wrap socket fd
        _handler->handle(w, _request);     // Delegate to RequestHandler
        return reactor->removeHandler(this, 0);
    }
    return true;
}
```

### Router implements RequestHandler

```cpp
// Router.hpp
class Router : public RequestHandler {
public:
    void handle(Response::Writer& w, const Request& req);  // implements interface

    void use(MiddlewareFunc mw);           // add middleware
    void get(const std::string& path, HandlerFunc handler);  // add route
    void setDefault(HandlerFunc handler);  // 404 handler
};
```

### Route handler function

```cpp
// routes.cpp
void handleHealth(Response::Writer& w, const Request& req) {
    (void)req;  // unused in this simple handler

    const char* body = "{\"status\":\"ok\"}";
    int bodyLen = std::strlen(body);

    Headers h = Response::getDefaultHeaders(bodyLen);
    h.replace("content-type", "application/json");

    w.writeStatusLine(Response::StatusOk);
    w.writeHeaders(h);
    w.writeBody(body, bodyLen);
}
```

## Why This Design?

### Separation of Concerns

| Layer | Knows | Doesn't Know |
|-------|-------|--------------|
| Connection | Sockets, recv/send, epoll | Routes, business logic |
| RequestHandler | Request/Response objects | Sockets, I/O details |
| Router | Routes, middleware | Raw bytes, network |

### Benefits

1. **Testability**: Unit test HTTP parsing without sockets, test routing without network I/O
2. **Flexibility**: Swap Router for any RequestHandler implementation without changing Reactor
3. **Single Responsibility**: Each layer has one job
4. **Dependency Inversion**: High-level modules (Router) don't depend on low-level modules (Connection)

## Class Hierarchy

```
RequestHandler (abstract interface)
       │
       └── Router (concrete implementation)
               │
               └── Uses RouteHandler for individual routes
                          │
                          └── FuncHandler (wraps function pointers)
```
