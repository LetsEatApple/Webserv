#!/usr/bin/env python3
"""Tests for concurrent CGI POST with large bodies.

Covers the three bugs fixed in the cgi-refactor branch:
  1. Reactor dispatched handleRead OR handleWrite, never both
  2. Event flag stomping between CGI handlers (pauseClientRead vs onReadable)
  3. EPOLLHUP force-removed CGI fd handlers before EOF was read
"""

import os
import select
import socket
import subprocess
import tempfile
import threading
import time
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")
PORT = 8098


def write_config(max_body="200m"):
    content = f"""server {{
    listen {PORT};

    location / {{
        alias {DATA_DIR};
        limit_except GET POST;
        client_max_body_size {max_body};
        cgi .py /usr/bin/python3;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path


def raw_post(port, path, body_size, timeout=30):
    """Send a POST and receive the response using non-blocking IO
    so send and recv happen concurrently (avoids TCP deadlock)."""
    s = socket.socket()
    s.settimeout(timeout)
    s.connect(("127.0.0.1", port))
    s.setblocking(False)

    header = (f"POST {path} HTTP/1.1\r\n"
              f"Host: localhost\r\n"
              f"Content-Type: application/octet-stream\r\n"
              f"Content-Length: {body_size}\r\n"
              f"\r\n").encode()
    to_send = header + (b"y" * body_size)
    send_off = 0
    resp = b""
    done_send = False

    deadline = time.time() + timeout
    while time.time() < deadline:
        r = [s] if b"0\r\n\r\n" not in resp else []
        w = [s] if not done_send else []
        if not r and not w:
            break
        rl, wl, _ = select.select(r, w, [], max(0, deadline - time.time()))
        if s in wl:
            try:
                n = s.send(to_send[send_off:send_off + 65536])
                send_off += n
                if send_off >= len(to_send):
                    done_send = True
            except BlockingIOError:
                pass
        if s in rl:
            try:
                d = s.recv(65536)
                if not d:
                    break
                resp += d
            except BlockingIOError:
                pass

    s.close()
    return resp


# ── Tests ────────────────────────────────────────────────────────────────────


def test_single_large_post():
    """Single 1MB POST to CGI echo script returns correct data."""
    conf = write_config()
    proc = start_server(conf)
    try:
        sz = 1024 * 1024
        resp = raw_post(PORT, "/cgi_echo.py", sz)
        assert b"HTTP/1.1 200" in resp, f"Expected 200, got: {resp[:80]}"
        # Check that the uppercased body is present
        assert b"YYYYY" in resp, "Echo body not found in response"
        print(f"PASS: single 1MB POST echoed correctly")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_concurrent_large_post():
    """Multiple concurrent large POST requests must all succeed.
    Regression: event flag stomping between CGI handlers caused deadlock."""
    conf = write_config()
    proc = start_server(conf)
    try:
        n = 10
        sz = 500 * 1024  # 500KB each
        results = [None] * n
        errors = []

        def worker(i):
            try:
                resp = raw_post(PORT, "/cgi_echo.py", sz, timeout=30)
                results[i] = b"HTTP/1.1 200" in resp
            except Exception as e:
                errors.append(f"W{i}: {e}")

        threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=60)

        assert not errors, f"Errors: {errors}"
        ok = sum(1 for r in results if r)
        assert ok == n, f"Expected {n} successes, got {ok}"
        print(f"PASS: {n} concurrent 500KB POST requests all succeeded")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_concurrent_large_post_10mb():
    """20 concurrent 1MB POST requests.
    Regression: EPOLLHUP removed CGI fd before EOF was read."""
    conf = write_config()
    proc = start_server(conf)
    try:
        n = 20
        sz = 1024 * 1024  # 1MB
        results = [None] * n
        errors = []

        def worker(i):
            try:
                resp = raw_post(PORT, "/cgi_echo.py", sz, timeout=60)
                results[i] = b"HTTP/1.1 200" in resp
            except Exception as e:
                errors.append(f"W{i}: {e}")

        threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=90)

        assert not errors, f"Errors: {errors}"
        ok = sum(1 for r in results if r)
        assert ok == n, f"Expected {n} successes, got {ok}"
        print(f"PASS: {n} concurrent 1MB POST requests all succeeded")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_no_zombies_after_concurrent_post():
    """No zombie CGI processes after concurrent large POSTs.
    Regression: EPOLLHUP handler removed outFd before stop() could reap."""
    conf = write_config()
    proc = start_server(conf)
    try:
        n = 10
        sz = 200 * 1024
        threads = []

        def worker(i):
            try:
                raw_post(PORT, "/cgi_echo.py", sz, timeout=30)
            except Exception:
                pass

        threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=60)

        time.sleep(1)

        result = subprocess.run(
            ["ps", "--ppid", str(proc.pid), "-o", "stat="],
            capture_output=True, text=True
        )
        zombies = sum(1 for line in result.stdout.strip().split("\n")
                      if line.strip().startswith("Z"))
        assert zombies == 0, f"Found {zombies} zombie processes"
        print(f"PASS: {n} concurrent large POSTs, 0 zombies")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_keepalive_after_large_cgi_post():
    """Keep-alive must work after a large CGI POST response.
    Regression: updateEvents could leave connection with wrong event mask."""
    conf = write_config()
    proc = start_server(conf)
    try:
        # First: large POST via raw socket, then reuse connection for GET
        s = socket.socket()
        s.settimeout(10)
        s.connect(("127.0.0.1", PORT))

        # POST 100KB
        sz = 100 * 1024
        body = b"y" * sz
        req = (f"POST /cgi_echo.py HTTP/1.1\r\n"
               f"Host: localhost\r\n"
               f"Content-Type: application/octet-stream\r\n"
               f"Content-Length: {sz}\r\n"
               f"\r\n").encode() + body
        s.sendall(req)

        # Read chunked response until terminal chunk
        resp = b""
        while b"0\r\n\r\n" not in resp:
            d = s.recv(65536)
            if not d:
                break
            resp += d
        assert b"HTTP/1.1 200" in resp, f"POST failed: {resp[:80]}"

        # Now send GET on same connection
        s.sendall(b"GET /cgi_hello.py HTTP/1.1\r\nHost: localhost\r\n\r\n")
        resp2 = b""
        while b"0\r\n\r\n" not in resp2:
            d = s.recv(65536)
            if not d:
                break
            resp2 += d
        s.close()

        assert b"HTTP/1.1 200" in resp2, f"GET after POST failed: {resp2[:80]}"
        assert b"CGI OK" in resp2, f"GET body unexpected: {resp2[:200]}"
        print("PASS: keep-alive works after 100KB CGI POST")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_chunked_post_to_cgi():
    """Chunked Transfer-Encoding POST to CGI must work.
    This is what the 42 tester sends."""
    conf = write_config()
    proc = start_server(conf)
    try:
        s = socket.socket()
        s.settimeout(10)
        s.connect(("127.0.0.1", PORT))

        # Send chunked POST
        header = (b"POST /cgi_echo.py HTTP/1.1\r\n"
                  b"Host: localhost\r\n"
                  b"Transfer-Encoding: chunked\r\n"
                  b"Content-Type: application/octet-stream\r\n"
                  b"\r\n")
        s.sendall(header)

        # Send 3 chunks of 10KB each
        for _ in range(3):
            chunk_data = b"y" * 10240
            s.sendall(f"{len(chunk_data):x}\r\n".encode() + chunk_data + b"\r\n")
        # Terminal chunk
        s.sendall(b"0\r\n\r\n")

        resp = b""
        while b"0\r\n\r\n" not in resp:
            d = s.recv(65536)
            if not d:
                break
            resp += d
        s.close()

        assert b"HTTP/1.1 200" in resp, f"Expected 200, got: {resp[:80]}"
        assert b"YYYYY" in resp, "Echo body not found"
        print("PASS: chunked POST (3x10KB) to CGI works")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_sequential_cgi_keepalive():
    """Multiple sequential CGI POST requests on one keep-alive connection."""
    conf = write_config()
    proc = start_server(conf)
    try:
        s = socket.socket()
        s.settimeout(10)
        s.connect(("127.0.0.1", PORT))

        for i in range(5):
            body = f"request-{i}-".encode() + b"x" * 1000
            req = (f"POST /cgi_echo.py HTTP/1.1\r\n"
                   f"Host: localhost\r\n"
                   f"Content-Type: application/octet-stream\r\n"
                   f"Content-Length: {len(body)}\r\n"
                   f"\r\n").encode() + body
            s.sendall(req)

            resp = b""
            while b"0\r\n\r\n" not in resp:
                d = s.recv(65536)
                if not d:
                    break
                resp += d
            assert b"HTTP/1.1 200" in resp, f"Request {i} failed: {resp[:80]}"

        s.close()
        print("PASS: 5 sequential CGI POSTs on one keep-alive connection")
    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([
        test_single_large_post,
        test_concurrent_large_post,
        test_concurrent_large_post_10mb,
        test_no_zombies_after_concurrent_post,
        test_keepalive_after_large_cgi_post,
        test_chunked_post_to_cgi,
        test_sequential_cgi_keepalive,
    ])
