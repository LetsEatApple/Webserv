# Code Review

## Summary

| Severity | Count | Description |
|----------|-------|-------------|
| Critical | 2 | Architecture coupling, logic bug |
| Medium | 5 | Memory leak, missing features, no validation |
| Low | 5 | Dead code, style issues |

---

## Critical Issues

### 1. `error()` defined in main.cpp but called from library

**Location:** `main.cpp:20`, called from `server.cpp:47`, `reactor.cpp:46`

**Problem:**
```cpp
// main.cpp:20
int error(int err) { ... }

// server.cpp:47
return error(1);  // Library calls function defined in application!
```

Library code should not depend on application code. This creates a circular dependency and makes the library unusable without main.cpp.

**Fix:** Move `error()` to a shared utility in the Reactor module, or use dependency injection (pass error handler to Server/Reactor constructors).

---

### 2. Stdin quit logic is inverted

**Location:** `handler.cpp:117`

**Problem:**
```cpp
if (input.compare("quit")){  // compare() returns 0 on MATCH!
    std::cout << "Quit called..." << std::endl;
    reactor->stop(SET);
}
```

`std::string::compare()` returns `0` when strings are equal. This code triggers shutdown on ANY input EXCEPT "quit".

**Fix:**
```cpp
if (input.compare("quit") == 0) {  // Or: if (input == "quit")
```

---

## Medium Issues

### 3. `removeHandler()` closes fd before epoll_ctl

**Location:** `reactor.cpp:99-102`

**Problem:**
```cpp
if (handler->get_fd() != -1)
    close(handler->get_fd());              // fd closed HERE

if (epoll_ctl(_epfd, EPOLL_CTL_DEL, handler->get_fd(), NULL) == -1)
    return false;  // Using CLOSED fd!
```

Works by accident because the kernel auto-removes closed fds from epoll, but logically incorrect.

**Fix:**
```cpp
int fd = handler->get_fd();
epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);  // Remove first
close(fd);                                    // Then close
```

---

### 4. Memory leak when registerHandler fails

**Location:** `reactor.cpp:53-54`

**Problem:**
```cpp
if (!registerHandler(new Signal(sfd), EPOLLIN))
    return false;  // Signal object leaked!
```

If `registerHandler()` fails (e.g., epoll_ctl fails), the dynamically allocated handler is never deleted.

**Fix:**
```cpp
Signal* sig = new Signal(sfd);
if (!registerHandler(sig, EPOLLIN)) {
    delete sig;
    return false;
}
```

---

### 5. No SO_REUSEADDR on listening sockets

**Location:** `server.cpp:45`

**Problem:**
```cpp
_s_list[i].sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
// Missing SO_REUSEADDR
```

Server cannot restart quickly after crash due to TIME_WAIT state.

**Fix:**
```cpp
int opt = 1;
setsockopt(_s_list[i].sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

---

### 6. Listen backlog too small

**Location:** `server.cpp:55`

**Problem:**
```cpp
int listen_val = listen(_s_list[i].sock, 1);  // Backlog of 1!
```

Under load, connections will be refused.

**Fix:**
```cpp
listen(_s_list[i].sock, SOMAXCONN);  // Or at least 128
```

---

### 7. No input validation on ports

**Location:** `server.cpp:19`

**Problem:**
```cpp
_s_list[i].port = atoi(ports[i+1]);  // No validation!
```

- `atoi("abc")` returns 0 (silent failure)
- `atoi("99999")` returns invalid port (> 65535)
- `atoi("-1")` returns negative port

**Fix:**
```cpp
long port = strtol(ports[i+1], &endptr, 10);
if (*endptr != '\0' || port < 1 || port > 65535) {
    // Handle error
}
```

---

## Low Issues

### 8. Global `g_signal_fd`

**Location:** `reactor.cpp:16`

**Problem:**
```cpp
static int g_signal_fd = -1;
```

Prevents multiple Reactor instances. Should be a member variable with alternative signal handling approach.

---

### 9. Unused member `_ev`

**Location:** `reactor.hpp:22`, `reactor.cpp:47`

**Problem:**
```cpp
struct epoll_event _ev;  // Declared

_ev.events = EPOLLIN;    // Set but never used
```

`registerHandler()` creates a local `ev` instead.

**Fix:** Remove `_ev` member.

---

### 10. Unused members in Server

**Location:** `server.hpp:30-31`

**Problem:**
```cpp
int _s_sock;  // Never used
int _c_sock;  // Never used
```

**Fix:** Remove unused members.

---

### 11. Handler return values ignored

**Location:** `reactor.cpp:122`

**Problem:**
```cpp
_handlers[_evlist[i].data.fd]->handleRead();  // Return value ignored
```

If handler returns an error, it's silently dropped.

**Fix:** Check return value and handle errors appropriately.

---

### 12. Inconsistent return types

**Location:** Various

**Problem:**
```cpp
int  init();           // returns int (0/1)
int  registerHandler();  // returns int (0/1)
bool stop();           // returns bool
```

Mix of `int` and `bool` for boolean success/failure.

**Fix:** Use `bool` consistently for boolean operations.

---

### 13. Magic numbers SET/GET

**Location:** `reactor.cpp:132`, `lib.h:34-35`

**Problem:**
```cpp
#define SET 1
#define GET 0

if (mode == SET)
    _sig = true;
```

Confusing API design.

**Fix:** Use separate methods:
```cpp
void requestStop() { _sig = true; }
bool shouldStop() const { return _sig; }
```

---

### 14. Missing Rule of Three

**Location:** `Server`, `Reactor` classes

**Problem:** Classes manage resources (file descriptors, dynamic memory) but don't define copy constructor or assignment operator.

**Fix:** Either implement copy semantics or delete them:
```cpp
Server(const Server&);             // = delete in C++11
Server& operator=(const Server&);  // = delete in C++11
```

---

### 15. Poor naming

| Current | Suggested |
|---------|-----------|
| `struct server_list` | `ListenSocket` |
| `s_list` | `_listenSockets` |
| `nports` | `_portCount` |

---

## Good Practices Observed

- Clean separation of HTTP parsing from I/O handling
- Proper use of Reactor pattern
- Incremental parsing handles partial reads correctly
- Signal handling via self-pipe trick is correct
- Non-blocking I/O throughout
- Well-structured class hierarchy for event handlers
