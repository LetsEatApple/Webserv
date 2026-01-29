# Webserv Documentation

## Contents

| Document | Description |
|----------|-------------|
| [architecture.md](architecture.md) | System architecture, components, and design patterns |
| [request-flow.md](request-flow.md) | Visual sequence diagram of HTTP request processing |
| [request-handler.md](request-handler.md) | RequestHandler interface: bridge between Reactor and HTTP |
| [code-review.md](code-review.md) | Code review findings and recommendations |

## Quick Start

```bash
# Build
mkdir build && cd build
cmake ..
make

# Run (specify port(s))
./webserv 8080

# Test
curl http://localhost:8080/
curl http://localhost:8080/health
curl http://localhost:8080/api/users
```

## Architecture Summary

```
┌─────────────┐     ┌─────────────┐
│   Router    │     │   Server    │
│  (routes)   │     │  (sockets)  │
└──────┬──────┘     └──────┬──────┘
       │                   │
       └─────────┬─────────┘
                 ▼
          ┌─────────────┐
          │   Reactor   │
          │   (epoll)   │
          └──────┬──────┘
                 │
    ┌────────────┼────────────┐
    ▼            ▼            ▼
┌────────┐ ┌──────────┐ ┌─────────┐
│Acceptor│ │Connection│ │ Signal  │
│(listen)│ │ (client) │ │(ctrl+c) │
└────────┘ └──────────┘ └─────────┘
```

## Key Files

| File | Purpose |
|------|---------|
| `main.cpp` | Entry point, router setup |
| `Http/request/Request.hpp` | HTTP request parser |
| `Http/router/Router.hpp` | Route matching |
| `Reactor/reactor.hpp` | epoll event loop |
| `Reactor/handler.hpp` | Connection/Acceptor handlers |
