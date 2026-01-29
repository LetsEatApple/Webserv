# Webserv Architecture

## Overview

Webserv is an HTTP/1.1 web server implementation using the **Reactor Pattern** for event-driven I/O. It's written in C++98 and uses Linux `epoll` for efficient I/O multiplexing.

## Directory Structure

```
Webserv/
├── CMakeLists.txt                  # Main build configuration
├── main.cpp                        # Application entry point
├── Http/                           # HTTP protocol implementation
│   ├── CMakeLists.txt
│   ├── request/
│   │   ├── Headers.cpp/hpp         # HTTP header parsing
│   │   └── Request.cpp/hpp         # HTTP request parsing
│   ├── response/
│   │   └── Response.cpp/hpp        # HTTP response writing
│   └── router/
│       ├── RequestHandler.hpp      # Abstract handler interface
│       └── Router.cpp/hpp          # Route matching and dispatch
├── Reactor/                        # Event-driven I/O
│   ├── CMakeLists.txt
│   ├── lib.h                       # Common includes
│   ├── AEventHandler.cpp/hpp       # Abstract event handler
│   ├── handler.cpp/hpp             # Concrete handlers
│   ├── reactor.cpp/hpp             # Event dispatcher
│   └── server.cpp/hpp              # Socket management
├── routes/                         # Application routes
│   └── routes.cpp/hpp
└── tests/                          # Unit tests (Catch2)
    ├── headers_test.cpp
    ├── request_test.cpp
    └── response_test.cpp
```

## Component Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│                            main.cpp                                │
│  1. Create Router + register routes/middleware                     │
│  2. Create Server (binds listening sockets)                        │
│  3. Create Reactor (server, &router)                               │
│  4. Run event loop until signal                                    │
└──────────────────────────┬─────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
   ┌─────────┐       ┌──────────┐       ┌─────────────────┐
   │ Router  │       │  Server  │       │     Reactor     │
   │ (HTTP)  │       │ (Sockets)│       │ (epoll + router)│
   └────┬────┘       └────┬─────┘       └────────┬────────┘
        │                 │                      │
        │                 │    ┌─────────────────┴────────────┐
        │                 │    │        Event Handlers        │
        │                 │    ├───────────┬──────────────────┤
        │                 │    │ Acceptor  │    Connection    │
        │                 │    │ (listen)  │     (client)     │
        │                 │    └─────┬─────┴────────┬─────────┘
        │                 │          │              │
        │                 └──────────┘              │
        │                                           │
        └───────────────────────────────────────────┘
                    Router.handle(w, req)
```

## Component Responsibilities

### HTTP Module

| Class | File | Responsibility |
|-------|------|----------------|
| `Headers` | Http/request/Headers.hpp | Parse and store HTTP headers (case-insensitive) |
| `Request` | Http/request/Request.hpp | Incremental HTTP request parsing (supports chunked encoding) |
| `Response::Writer` | Http/response/Response.hpp | Write HTTP responses to socket |
| `RequestHandler` | Http/router/RequestHandler.hpp | Abstract interface for request handling |
| `Router` | Http/router/Router.hpp | Route matching, middleware chain, dispatch |

### Reactor Module

| Class | File | Responsibility |
|-------|------|----------------|
| `EventHandler` | Reactor/AEventHandler.hpp | Abstract base for all event handlers |
| `Connection` | Reactor/handler.hpp | Handle client HTTP connections |
| `Acceptor` | Reactor/handler.hpp | Accept new connections on listening socket |
| `Signal` | Reactor/handler.hpp | Handle SIGINT/SIGTERM via self-pipe |
| `Stdin` | Reactor/handler.hpp | Handle stdin for graceful shutdown |
| `Server` | Reactor/server.hpp | Create and manage listening sockets |
| `Reactor` | Reactor/reactor.hpp | epoll-based event dispatcher |

## Dependency Graph

```
┌─────────────┐
│   http_lib  │  (Headers, Request, Response, Router)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ reactor_lib │  (EventHandler, Connection, Acceptor, Server, Reactor)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   webserv   │  (main.cpp + routes)
└─────────────┘
```

## Design Patterns

### Reactor Pattern (Initiation Dispatcher)

The core architecture follows the Reactor pattern:

1. **Synchronous Event Demultiplexer**: `epoll_wait()` blocks until events ready
2. **Event Handler Interface**: `EventHandler` with `handleRead()`/`handleWrite()`
3. **Concrete Handlers**: `Acceptor`, `Connection`, `Signal`, `Stdin`
4. **Initiation Dispatcher**: `Reactor` class dispatches events to handlers

### Strategy Pattern

Different `EventHandler` implementations provide different strategies for handling I/O events.

### Chain of Responsibility

Router middleware forms a chain - each middleware can short-circuit or pass to next.

## Key Features

- **Non-blocking I/O**: All sockets set to `O_NONBLOCK`
- **Incremental Parsing**: HTTP requests can span multiple `recv()` calls
- **Chunked Transfer Encoding**: Supports chunked request bodies
- **Middleware Support**: Pre-request processing chain
- **Route Matching**: Exact path and prefix matching
- **Signal Safety**: Self-pipe trick for safe signal handling
- **C++98 Compliant**: No modern C++ features used
