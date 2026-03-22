_This project has been created as part of the 42 curriculum
by lhagemos, mkugan, pschneid._

## Description

A lightweight HTTP/1.1 server written in C++98, featuring an
epoll-based reactor pattern, CGI support with streaming I/O,
virtual host routing, directory listing, file uploads, and
chunked transfer encoding.

## Architecture

Single-threaded event loop using the Reactor pattern with Linux epoll. 
The Reactor owns a single epoll fd and a map of `fd → EventHandler*`. 
All I/O — client sockets, listening sockets, CGI pipes, and the signal self-pipe is multiplexed through one `epoll_wait` call. 
Handlers are polymorphic: the Reactor dispatches `handleRead()` and `handleWrite()` without knowing the concrete type.
The HTTP request parser is incremental, processing data as it arrives from `recv()` rather than waiting for a complete request.

```
main()
  ├─ config::buildConfig()    parse nginx-style config
  ├─ Server(listeners)        bind + listen on each host:port
  ├─ Reactor::init(sessions)  register fds with epoll
  └─ Reactor::handleEvents()  main loop
       ├─ epoll_wait()
       ├─ handler->handleRead()
       ├─ handler->handleWrite()
       └─ checkTimeouts()
```

### Handler Hierarchy

```
EventHandler (abstract)
  ├─ Acceptor       accept() new connections
  ├─ Connection     HTTP request/response lifecycle
  ├─ CgiFdHandler   CGI pipe I/O (delegates to CgiProcess)
  ├─ Signal          signal self-pipe (SIGINT/SIGTERM)
  └─ Stdin           terminal input ("quit" command)
```

### Connection States

```
StateReadingHeaders → StateCgiRunning → StateCgiStreaming → StateSendingResponse
                    → StateSendingResponse (non-CGI)
```

### CGI

- Uses `socketpair(AF_UNIX, SOCK_STREAM)` for I/O
- Streaming: starts at `headersDone()`, body sent to CGI as it arrives
- Output relayed as `Transfer-Encoding: chunked`
- Backpressure in both directions at 64KB threshold

### Configuration

Nginx-style config parsed into `map<host:port, vector<Server>>`. Each server has locations with: `alias`, `index`, `autoindex`, `limit_except`, `error_page`, `client_max_body_size`, `cgi`, `return`, `upload`.

## Features

- HTTP/1.1 with GET, POST, DELETE methods
- Keep-alive connections
- Chunked transfer encoding (request and response)
- CGI with streaming I/O (Python, Perl)
- Virtual host routing (`server_name`)
- Custom error pages
- Directory listing (autoindex)
- File uploads (raw body and multipart/form-data)
- HTTP redirections (`return` directive)
- Client body size limits

## Configuration Example

```nginx
server {
    listen 127.0.0.1:8080;
    server_name example.com;
    client_max_body_size 10M;
    error_page 404 /404.html;

    cgi .py /usr/bin/python3;

    location / {
        alias /var/www/html;
        index index.html;
        autoindex off;
        limit_except GET;
    }

    location /upload/ {
        alias /var/www/uploads;
        upload /var/www/uploads;
        limit_except POST;
    }
}
```

## Instructions

### Build
```bash
make        # build
make DEBUG=1  # AddressSanitizer + UBSan
```

### Run
```bash
./webserv [configuration_file.conf]
```

### Clean
```bash
make clean # Objects and dependencies
make fclean # Objects, dependencies, and executable
make re # Delete everything and rebuild
```

### Test
```bash
# curl
curl -v http://localhost:8000/ # GET request
curl -v http://localhost:8000/file.html # GET with specific file
curl -v -X POST http://localhost:8000/ # POST request
curl -v -X DELETE http://localhost:8000/resource # DELETE request

curl -v http://localhost:8080/ \
  -H "Host: localhost"  # test header

# siege (load testing)
siege -b http://localhost:8000/ # runs the test with NO DELAY for throughput benchmarking.
siege -c 50 -t 30s http://localhost:8000/ # 50 concurrent users for 30 sec
./tests/siege_test.sh  # automated suite: concurrency, stress, sustained load
```
## Resources
- [RFC 9110 - HTTP Semantics](https://datatracker.ietf.org/doc/html/rfc9110)
- [RFC 9112 - HTTP/1.1](https://datatracker.ietf.org/doc/html/rfc9112)
- [RFC 6265 - HTTP State Management Mechanism](https://datatracker.ietf.org/doc/html/rfc6265)
- [RFC 3875 - The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [RFC 7578 - Returning Values from Forms: multipart/form-data](https://datatracker.ietf.org/doc/html/rfc7578)
- [nginx documentation](https://nginx.org/en/docs/)
- [MDN Web Docs - HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [Reactor: An Object Behavioral Pattern for Demultiplexing and Dispatching Handles for Synchronous Events — Douglas C. Schmidt](https://www.dre.vanderbilt.edu/~schmidt/PDF/reactor-siemens.pdf)
- [Patterns in C – Part 5: REACTOR — Adam Petersen](https://www.adamtornhill.com/Patterns%20in%20C%205,%20REACTOR.pdf)
- HTTP: The Definitive Guide — David Gourley, Brian Totty, Marjorie Sayer, Anshu Aggarwal, Sailu Reddy (O'Reilly, 2002)
- HTTP Developer's Handbook — Chris Shiflett (Developer's Library, 2003)
- UNIX Network Programming, Volume 1: The Sockets Networking API — W. Richard Stevens, Bill Fenner, Andrew M. Rudoff (3rd ed., 2003)
- The Linux Programming Interface — Michael Kerrisk (2010, Ch. 56–61: Sockets, Ch. 63: Alternative I/O Models)
- [From TCP to HTTP | Full Course by ThePrimeagen](https://www.youtube.com/watch?v=FknTw9bJsXM)

### AI Usage
AI was used as a support tool for debugging, reviewing code, and generating documentation. All code was written, understood, and validated by the team.
