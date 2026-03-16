#!/usr/bin/env python3
"""Integration tests for keep-alive connections after CGI responses,
non-CGI POST handling, directory-without-index behavior, and chunked
body size enforcement.

These cover regressions found during 42 tester debugging.
"""

import os
import tempfile
import http.client
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")
PORT = 8095


def write_config():
    content = f"""server {{
    listen {PORT};

    location / {{
        alias {DATA_DIR};
        limit_except GET POST;
        cgi .py /usr/bin/python3;
    }}

    location /noindex {{
        alias {DATA_DIR}/sub;
        limit_except GET;
    }}

    location /post_only {{
        alias {DATA_DIR};
        limit_except POST;
        client_max_body_size 100;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path


def setup():
    """Ensure test directories exist."""
    sub = os.path.join(DATA_DIR, "sub")
    os.makedirs(sub, exist_ok=True)
    # sub/ has no index.html — used to test directory-without-index


def test_keepalive_after_cgi():
    """After a CGI response, the next request on the same connection must work."""
    conf = write_config()
    proc = start_server(conf)
    try:
        conn = http.client.HTTPConnection("127.0.0.1", PORT, timeout=5)

        # 1st request: CGI
        conn.request("GET", "/cgi_hello.py")
        r1 = conn.getresponse()
        body1 = r1.read().decode()
        assert r1.status == 200, f"CGI request: expected 200, got {r1.status}"
        assert "CGI OK" in body1, f"CGI body missing 'CGI OK': {body1}"

        # 2nd request: static file on same connection
        conn.request("GET", "/hello.txt")
        r2 = conn.getresponse()
        body2 = r2.read().decode()
        assert r2.status == 200, f"Static after CGI: expected 200, got {r2.status}"
        assert "hello" in body2.lower(), f"Static body unexpected: {body2}"

        # 3rd request: another CGI to confirm full cycle
        conn.request("GET", "/cgi_hello.py")
        r3 = conn.getresponse()
        body3 = r3.read().decode()
        assert r3.status == 200, f"2nd CGI request: expected 200, got {r3.status}"
        assert "CGI OK" in body3

        conn.close()
        print("PASS: keep-alive works across CGI -> static -> CGI")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_keepalive_cgi_post_then_get():
    """POST to CGI followed by GET on the same connection."""
    conf = write_config()
    proc = start_server(conf)
    try:
        conn = http.client.HTTPConnection("127.0.0.1", PORT, timeout=5)

        # POST to CGI
        conn.request("POST", "/cgi_post.py", body="test data",
                     headers={"Content-Type": "text/plain"})
        r1 = conn.getresponse()
        body1 = r1.read().decode()
        assert r1.status == 200, f"POST CGI: expected 200, got {r1.status}"
        assert "received: test data" in body1, f"POST echo unexpected: {body1}"

        # GET static on same connection
        conn.request("GET", "/hello.txt")
        r2 = conn.getresponse()
        body2 = r2.read().decode()
        assert r2.status == 200, f"GET after POST CGI: expected 200, got {r2.status}"

        conn.close()
        print("PASS: keep-alive works POST CGI -> GET static")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_directory_without_index():
    """GET on a directory with no index file and autoindex off.
    - With trailing slash (resolved by alias): 403 Forbidden
    - Without trailing slash and no index: 404 Not Found (avoids useless 301)
    """
    conf = write_config()
    proc = start_server(conf)
    try:
        # /noindex resolves to data/sub/ (trailing slash) -> 403
        status, _, _ = request(PORT, "/noindex")
        assert status == 403, f"Expected 403 for dir without index, got {status}"
        print("PASS: directory without index (trailing slash) -> 403")

        # Create a subdir inside data/sub so we can test without trailing slash
        subdir = os.path.join(DATA_DIR, "sub", "empty")
        os.makedirs(subdir, exist_ok=True)
        try:
            status, _, _ = request(PORT, "/noindex/empty")
            assert status == 404, f"Expected 404 for dir without trailing slash and no index, got {status}"
            print("PASS: directory without index (no trailing slash) -> 404")
        finally:
            os.rmdir(subdir)

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_non_cgi_post_returns_204():
    """POST to a location without CGI match returns 204 No Content."""
    conf = write_config()
    proc = start_server(conf)
    try:
        status, _, body = request(PORT, "/post_only", method="POST",
                                  body="hello",
                                  headers={"Content-Length": "5"})
        assert status == 204, f"Expected 204 for non-CGI POST, got {status}"
        print("PASS: non-CGI POST -> 204")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_post_body_too_large_413():
    """POST with Content-Length exceeding client_max_body_size -> 413."""
    conf = write_config()
    proc = start_server(conf)
    try:
        body = "A" * 200
        status, _, _ = request(PORT, "/post_only", method="POST",
                               body=body,
                               headers={"Content-Length": str(len(body))})
        assert status == 413, f"Expected 413, got {status}"
        print("PASS: POST body too large -> 413")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_chunked_body_too_large_413():
    """POST with chunked body exceeding client_max_body_size -> 413."""
    conf = write_config()
    proc = start_server(conf)
    try:
        import socket

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", PORT))
        s.settimeout(5)

        # Send chunked POST with 200 bytes (limit is 100)
        payload = b"A" * 200
        chunk = (hex(len(payload))[2:].encode() + b"\r\n"
                 + payload + b"\r\n"
                 + b"0\r\n\r\n")
        req = (b"POST /post_only HTTP/1.1\r\n"
               b"Host: 127.0.0.1\r\n"
               b"Transfer-Encoding: chunked\r\n"
               b"\r\n" + chunk)
        s.sendall(req)

        resp = b""
        while True:
            try:
                data = s.recv(4096)
                if not data:
                    break
                resp += data
                if b"\r\n\r\n" in resp:
                    break
            except socket.timeout:
                break
        s.close()

        status_line = resp.split(b"\r\n")[0].decode()
        assert "413" in status_line, f"Expected 413 in status, got: {status_line}"
        print("PASS: chunked POST body too large -> 413")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_large_post_body_not_rejected_as_header():
    """POST with body > 8KB must not be rejected by the header size limit.

    Regression: when recv buffer (MAX) > MAX_HEADER_SIZE, a single recv
    could contain both headers and body data. The _headerBytes counter
    included body bytes, falsely triggering a 400 for any POST > 8KB.
    """
    conf = write_config()
    proc = start_server(conf)
    try:
        # 16KB body — well over the 8KB header limit but valid
        body = "X" * 16384
        status, _, resp_body = request(PORT, "/cgi_post.py", method="POST",
                                       body=body,
                                       headers={"Content-Type": "text/plain"})
        assert status == 200, f"Expected 200 for 16KB POST body, got {status}"
        assert "received: " in resp_body, f"CGI didn't echo body: {resp_body[:100]}"
        print("PASS: 16KB POST body not rejected by header limit")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_large_post_body_keepalive():
    """POST with body > 8KB followed by GET on the same connection.

    Regression: same _headerBytes bug would close the connection with 400,
    breaking keep-alive for all subsequent requests.
    """
    conf = write_config()
    proc = start_server(conf)
    try:
        conn = http.client.HTTPConnection("127.0.0.1", PORT, timeout=5)

        # POST 16KB to CGI
        body = "Y" * 16384
        conn.request("POST", "/cgi_post.py", body=body,
                     headers={"Content-Type": "text/plain"})
        r1 = conn.getresponse()
        r1.read()
        assert r1.status == 200, f"16KB POST: expected 200, got {r1.status}"

        # GET on same connection should work
        conn.request("GET", "/hello.txt")
        r2 = conn.getresponse()
        r2.read()
        assert r2.status == 200, f"GET after 16KB POST: expected 200, got {r2.status}"

        conn.close()
        print("PASS: 16KB POST then GET on same connection works")

    finally:
        stop_server(proc)
        os.unlink(conf)


setup()

if __name__ == "__main__":
    run_tests([
        test_keepalive_after_cgi,
        test_keepalive_cgi_post_then_get,
        test_directory_without_index,
        test_non_cgi_post_returns_204,
        test_post_body_too_large_413,
        test_chunked_body_too_large_413,
        test_large_post_body_not_rejected_as_header,
        test_large_post_body_keepalive,
    ])
