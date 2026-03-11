#!/usr/bin/env python3
"""Integration tests for streaming CGI with chunked transfer encoding."""

import os
import socket
import tempfile
import time
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")


def write_config():
    content = f"""server {{
    listen 8095;

    location / {{
        alias {DATA_DIR};
        limit_except GET POST;
        cgi .py /usr/bin/python3;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path


def test_chunked_response():
    """CGI response should use Transfer-Encoding: chunked."""
    conf = write_config()
    proc = start_server(conf)
    try:
        status, headers, body = request(8095, "/cgi_hello.py")
        assert status == 200, f"Expected 200, got {status}"
        te = headers.get("transfer-encoding", "")
        assert te == "chunked", f"Expected chunked, got '{te}'"
        assert "CGI OK" in body, f"Expected 'CGI OK' in body, got '{body}'"
        print("PASS: CGI response uses Transfer-Encoding: chunked")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_streaming_slow_cgi():
    """Slow CGI should stream chunks as they arrive."""
    conf = write_config()
    proc = start_server(conf)
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("127.0.0.1", 8095))
        sock.sendall(b"GET /cgi_slow.py HTTP/1.1\r\nHost: localhost\r\n\r\n")
        sock.settimeout(5)

        data = b""
        start = time.time()
        while time.time() - start < 5:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                data += chunk
                if b"line 2" in data:
                    break
            except socket.timeout:
                break

        text = data.decode(errors="replace")
        assert "transfer-encoding: chunked" in text.lower(), \
            f"Expected chunked header in:\n{text}"
        assert "line 0" in text, f"Missing 'line 0' in:\n{text}"
        assert "line 2" in text, f"Missing 'line 2' in:\n{text}"
        elapsed = time.time() - start
        assert elapsed >= 0.3, f"Expected >= 0.3s (streaming), got {elapsed:.2f}s"
        print(f"PASS: Slow CGI streamed in {elapsed:.2f}s with chunked encoding")
        sock.close()
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_post_chunked():
    """POST CGI should also use chunked response."""
    conf = write_config()
    proc = start_server(conf)
    try:
        status, headers, body = request(8095, "/cgi_post.py", method="POST",
                                        body="hello stream",
                                        headers={"Content-Type": "text/plain"})
        assert status == 200, f"Expected 200, got {status}"
        te = headers.get("transfer-encoding", "")
        assert te == "chunked", f"Expected chunked, got '{te}'"
        assert "received: hello stream" in body, f"Unexpected body: '{body}'"
        print("PASS: POST CGI uses chunked response")
    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([test_chunked_response, test_streaming_slow_cgi, test_post_chunked])
