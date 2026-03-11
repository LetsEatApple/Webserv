#!/usr/bin/env python3
"""Stress tests for CGI: concurrency, large bodies, backpressure, disconnects."""

import os
import socket
import tempfile
import threading
import time
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")
PORT = 8097


def write_config(max_body="10m"):
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


def test_concurrent_cgi():
    """Many concurrent CGI requests should all succeed."""
    conf = write_config()
    proc = start_server(conf)
    try:
        n = 50
        results = [None] * n
        errors = []

        def do_request(i):
            try:
                status, _, body = request(PORT, "/cgi_hello.py")
                results[i] = (status, body)
            except Exception as e:
                errors.append(f"Request {i}: {e}")

        threads = [threading.Thread(target=do_request, args=(i,)) for i in range(n)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=10)

        assert not errors, f"Errors: {errors}"
        ok = sum(1 for r in results if r and r[0] == 200 and "CGI OK" in r[1])
        assert ok == n, f"Expected {n} successes, got {ok}"
        print(f"PASS: {n}/{n} concurrent CGI requests succeeded")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_large_post_body():
    """Large POST body should be streamed to CGI without corruption."""
    conf = write_config("2m")
    proc = start_server(conf)
    try:
        body_size = 1 * 1024 * 1024  # 1MB
        body = "A" * body_size
        status, _, resp = request(PORT, "/cgi_sink.py", method="POST",
                                  body=body,
                                  headers={"Content-Type": "application/octet-stream"})
        assert status == 200, f"Expected 200, got {status}"
        assert f"bytes={body_size}" in resp, f"Expected bytes={body_size}, got: {resp}"
        print(f"PASS: 1MB POST body streamed correctly to CGI")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_large_cgi_output():
    """CGI producing large output should stream without error."""
    import http.client
    conf = write_config()
    proc = start_server(conf)
    try:
        conn = http.client.HTTPConnection("127.0.0.1", PORT, timeout=30)
        conn.request("GET", "/cgi_flood.py")
        resp = conn.getresponse()
        body = resp.read()
        conn.close()
        assert resp.status == 200, f"Expected 200, got {resp.status}"
        assert len(body) > 1000000, f"Expected >1MB, got {len(body)} bytes"
        print(f"PASS: Large CGI output ({len(body)} bytes) streamed successfully")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_client_disconnect_during_cgi():
    """Client disconnecting mid-CGI should not crash the server."""
    conf = write_config()
    proc = start_server(conf)
    try:
        # Send requests that trigger slow CGI, then disconnect immediately
        for i in range(20):
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", PORT))
                s.sendall(b"GET /cgi_slow.py HTTP/1.1\r\nHost: localhost\r\n\r\n")
                s.close()
            except Exception:
                pass

        # Give server a moment to clean up
        time.sleep(1)

        # Server should still be responsive
        status, _, body = request(PORT, "/cgi_hello.py")
        assert status == 200, f"Expected 200 after disconnects, got {status}"
        assert "CGI OK" in body
        print("PASS: Server survived 20 client disconnects during CGI")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_no_zombie_processes():
    """After many CGI requests, no zombie processes should remain."""
    conf = write_config()
    proc = start_server(conf)
    try:
        for i in range(30):
            request(PORT, "/cgi_hello.py")

        time.sleep(0.5)

        # Count zombies belonging to our server
        import subprocess
        result = subprocess.run(
            ["ps", "--ppid", str(proc.pid), "-o", "stat="],
            capture_output=True, text=True
        )
        zombies = sum(1 for line in result.stdout.strip().split("\n")
                      if line.strip().startswith("Z"))
        assert zombies == 0, f"Found {zombies} zombie processes"
        print(f"PASS: 30 sequential CGI requests, 0 zombies")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_no_fd_leak():
    """File descriptor count should not grow after many CGI requests."""
    conf = write_config()
    proc = start_server(conf)
    try:
        # Warm up
        request(PORT, "/cgi_hello.py")

        # Count initial fds
        fd_dir = f"/proc/{proc.pid}/fd"
        if not os.path.exists(fd_dir):
            print("SKIP: /proc/pid/fd not available")
            return
        initial_fds = len(os.listdir(fd_dir))

        # Run many requests
        for i in range(50):
            request(PORT, "/cgi_hello.py")

        time.sleep(0.5)
        final_fds = len(os.listdir(fd_dir))

        leaked = final_fds - initial_fds
        assert leaked <= 2, f"FD leak: started with {initial_fds}, ended with {final_fds} (+{leaked})"
        print(f"PASS: No FD leak ({initial_fds} -> {final_fds} fds after 50 requests)")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_rapid_post_disconnect():
    """Sending POST with body and disconnecting mid-stream should not crash."""
    conf = write_config()
    proc = start_server(conf)
    try:
        for i in range(20):
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", PORT))
                # Send headers promising a large body, then disconnect
                s.sendall(
                    b"POST /cgi_sink.py HTTP/1.1\r\n"
                    b"Host: localhost\r\n"
                    b"Content-Type: application/octet-stream\r\n"
                    b"Content-Length: 1000000\r\n"
                    b"\r\n"
                    b"partial data here"
                )
                s.close()
            except Exception:
                pass

        time.sleep(1)

        # Server should still work
        status, _, body = request(PORT, "/cgi_hello.py")
        assert status == 200, f"Expected 200 after POST disconnects, got {status}"
        assert "CGI OK" in body
        print("PASS: Server survived 20 POST disconnects mid-body")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_concurrent_mixed():
    """Mix of GET and POST CGI requests concurrently."""
    conf = write_config()
    proc = start_server(conf)
    try:
        n = 40
        results = [None] * n
        errors = []

        def do_get(i):
            try:
                status, _, body = request(PORT, "/cgi_hello.py")
                results[i] = status
            except Exception as e:
                errors.append(f"GET {i}: {e}")

        def do_post(i):
            try:
                payload = f"data-{i}" * 100
                status, _, body = request(PORT, "/cgi_post.py", method="POST",
                                          body=payload,
                                          headers={"Content-Type": "text/plain"})
                results[i] = status
            except Exception as e:
                errors.append(f"POST {i}: {e}")

        threads = []
        for i in range(n):
            if i % 2 == 0:
                threads.append(threading.Thread(target=do_get, args=(i,)))
            else:
                threads.append(threading.Thread(target=do_post, args=(i,)))

        for t in threads:
            t.start()
        for t in threads:
            t.join(timeout=15)

        assert not errors, f"Errors: {errors}"
        ok = sum(1 for r in results if r == 200)
        assert ok == n, f"Expected {n} successes, got {ok}"
        print(f"PASS: {n} mixed GET/POST concurrent CGI requests all returned 200")
    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([
        test_concurrent_cgi,
        test_large_post_body,
        test_large_cgi_output,
        test_client_disconnect_during_cgi,
        test_no_zombie_processes,
        test_no_fd_leak,
        test_rapid_post_disconnect,
        test_concurrent_mixed,
    ])
