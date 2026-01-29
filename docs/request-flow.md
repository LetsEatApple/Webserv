# HTTP Request Flow

## Sequence Diagram

```
CLIENT                    SERVER
  │                         │
  │  TCP SYN ──────────────►│
  │◄────────────── SYN-ACK  │
  │  ACK ──────────────────►│
  │                         │
  │                         │    ┌─────────────────────────────────────┐
  │                         │    │           REACTOR (epoll)           │
  │                         │    │  ┌─────────┐  ┌─────────┐          │
  │                         │    │  │Acceptor │  │ Signal  │          │
  │                         │    │  │ :8080   │  │ handler │          │
  │                         │    │  └────┬────┘  └─────────┘          │
  │                         │    └───────┼────────────────────────────┘
  │                         │            │
  │  (connection ready)     │            ▼
  │                         │    ┌───────────────────────────────────┐
  │                         │    │ Acceptor::handleRead()            │
  │                         │    │  1. accept() → client_fd = 5      │
  │                         │    │  2. fcntl(5, O_NONBLOCK)          │
  │                         │    │  3. new Connection(5, router)     │
  │                         │    │  4. reactor->registerHandler()    │
  │                         │    └───────────────────────────────────┘
  │                         │
  │                         │    ┌─────────────────────────────────────┐
  │                         │    │           REACTOR (epoll)           │
  │                         │    │  ┌─────────┐  ┌────────────┐       │
  │                         │    │  │Acceptor │  │ Connection │       │
  │                         │    │  │ :8080   │  │  fd=5      │       │
  │                         │    │  └─────────┘  └─────┬──────┘       │
  │                         │    └────────────────────┼───────────────┘
  │                         │                         │
  │  GET /health HTTP/1.1   │                         │
  │  Host: localhost        │                         │
  │  ───────────────────────┼────────────────────────►│
  │                         │                         ▼
  │                         │    ┌────────────────────────────────────┐
  │                         │    │ Connection::handleRead()           │
  │                         │    │                                    │
  │                         │    │  recv(fd=5, buf, 1024)             │
  │                         │    │       │                            │
  │                         │    │       ▼                            │
  │                         │    │  _readBuffer = "GET /health..."   │
  │                         │    │       │                            │
  │                         │    │       ▼                            │
  │                         │    │  _request.parse(_readBuffer)       │
  │                         │    └───────┼────────────────────────────┘
  │                         │            │
  │                         │            ▼
  │                         │    ┌────────────────────────────────────┐
  │                         │    │ Request::parse()                   │
  │                         │    │                                    │
  │                         │    │  State: Init                       │
  │                         │    │    → parse "GET /health HTTP/1.1"  │
  │                         │    │    → method = "GET"                │
  │                         │    │    → target = "/health"            │
  │                         │    │    → version = "1.1"               │
  │                         │    │                                    │
  │                         │    │  State: Headers                    │
  │                         │    │    → parse "Host: localhost\r\n"   │
  │                         │    │    → headers["host"] = "localhost" │
  │                         │    │    → parse "\r\n" (empty line)     │
  │                         │    │                                    │
  │                         │    │  State: Done                       │
  │                         │    └───────┼────────────────────────────┘
  │                         │            │
  │                         │            ▼
  │                         │    ┌────────────────────────────────────┐
  │                         │    │ if (_request.done())               │
  │                         │    │   _requestHandler->handle(w, req)  │
  │                         │    │         │                          │
  │                         │    │         ▼                          │
  │                         │    │  Router::handle()                  │
  │                         │    └───────┼────────────────────────────┘
  │                         │            │
  │                         │            ▼
  │                         │    ┌────────────────────────────────────┐
  │                         │    │ Router::handle()                   │
  │                         │    │                                    │
  │                         │    │  1. Run middleware:                │
  │                         │    │     logRequest() → prints          │
  │                         │    │     "GET /health"                  │
  │                         │    │     returns true (continue)        │
  │                         │    │                                    │
  │                         │    │  2. Match routes:                  │
  │                         │    │     "/" → no match                 │
  │                         │    │     "/health" → MATCH!             │
  │                         │    │                                    │
  │                         │    │  3. Call handler:                  │
  │                         │    │     handleHealth(writer, req)      │
  │                         │    └───────┼────────────────────────────┘
  │                         │            │
  │                         │            ▼
  │                         │    ┌────────────────────────────────────┐
  │                         │    │ handleHealth()                     │
  │                         │    │                                    │
  │                         │    │  body = {"status":"ok"}            │
  │                         │    │                                    │
  │                         │    │  writer.writeStatusLine(200)       │
  │                         │    │    → write "HTTP/1.1 200 OK\r\n"   │
  │                         │    │                                    │
  │                         │    │  writer.writeHeaders(headers)      │
  │                         │    │    → write "Content-Length: 15\r\n"│
  │                         │    │    → write "Content-Type: ...\r\n" │
  │                         │    │    → write "\r\n"                  │
  │                         │    │                                    │
  │                         │    │  writer.writeBody(body, 15)        │
  │                         │    │    → write {"status":"ok"}         │
  │                         │    └───────┼────────────────────────────┘
  │                         │            │
  │◄─────────────────────────────────────┘
  │  HTTP/1.1 200 OK        │
  │  Content-Length: 15     │
  │  Content-Type: app/json │
  │                         │
  │  {"status":"ok"}        │
  │                         │
  │                         │    ┌────────────────────────────────────┐
  │                         │    │ Connection::handleRead() cont'd    │
  │                         │    │                                    │
  │                         │    │  return reactor->removeHandler()   │
  │                         │    │         │                          │
  │                         │    │         ▼                          │
  │                         │    │  Reactor::removeHandler()          │
  │                         │    │    1. close(fd=5)                  │
  │                         │    │    2. epoll_ctl(DEL, fd=5)         │
  │                         │    │    3. delete Connection            │
  │                         │    └────────────────────────────────────┘
  │                         │
  │  (connection closed)    │
  │                         │
```

## Timeline View

```
┌────────────────────────────────────────────────────────────────┐
│                            TIME →                              │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌─────────┐                                                   │
│  │ accept  │ Acceptor creates Connection                       │
│  └────┬────┘                                                   │
│       │                                                        │
│       ▼                                                        │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                         │
│  │  recv   │  │  recv   │  │  recv   │  (partial reads ok)     │
│  │ "GET /" │  │ "health"│  │ "\r\n\r"│                         │
│  └────┬────┘  └────┬────┘  └────┬────┘                         │
│       │            │            │                              │
│       ▼            ▼            ▼                              │
│  ┌────────────────────────────────────┐                        │
│  │    Request::parse() incremental    │                        │
│  │    (accumulates in _readBuffer)    │                        │
│  └────────────────┬───────────────────┘                        │
│                   │                                            │
│                   ▼  done() == true                            │
│  ┌──────────┐  ┌─────────┐  ┌─────────┐                        │
│  │middleware│─▶│  route  │─▶│ handler │                        │
│  │  chain   │  │  match  │  │  exec   │                        │
│  └──────────┘  └─────────┘  └────┬────┘                        │
│                                  │                             │
│                                  ▼                             │
│  ┌──────────────────────────────────────┐                      │
│  │  Response::Writer writes to socket   │                      │
│  │  (status line → headers → body)      │                      │
│  └───────────────────┬──────────────────┘                      │
│                      │                                         │
│                      ▼                                         │
│  ┌─────────┐                                                   │
│  │ cleanup │ close fd, delete handler                          │
│  └─────────┘                                                   │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

## Parser State Machine

```
                    ┌───────────────────┐
                    │       Init        │
                    │  (parse request   │
                    │      line)        │
                    └─────────┬─────────┘
                              │ found CRLF
                              ▼
                    ┌───────────────────┐
                    │     Headers       │
                    │  (parse header    │
                    │     fields)       │
                    └─────────┬─────────┘
                              │ empty line (CRLF)
                              ▼
              ┌───────────────┴───────────────┐
              │                               │
              ▼                               ▼
    ┌───────────────────┐           ┌───────────────────┐
    │       Body        │           │   ChunkedSize     │
    │  (Content-Length  │           │  (Transfer-       │
    │     present)      │           │   Encoding:       │
    └─────────┬─────────┘           │   chunked)        │
              │                     └─────────┬─────────┘
              │ all bytes read                │
              │                               ▼
              │                     ┌───────────────────┐
              │                     │   ChunkedData     │
              │                     └─────────┬─────────┘
              │                               │
              │                               ▼
              │                     ┌───────────────────┐
              │                     │  ChunkedTrailer   │
              │                     └─────────┬─────────┘
              │                               │ size == 0
              ▼                               ▼
    ┌─────────────────────────────────────────────────┐
    │                      Done                        │
    └──────────────────────────────────────────────────┘
```

## Key Points

1. **Non-blocking everywhere** - `accept()`, `recv()`, sockets all non-blocking
2. **Incremental parsing** - Request can span multiple `recv()` calls
3. **Single-threaded** - One epoll loop handles all connections
4. **Connection per request** - Handler removed after response (no keep-alive yet)
5. **Middleware first** - Runs before route matching
