#!/usr/bin/env python3
"""Integration tests for static file serving."""

import os
import tempfile
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")


def write_config():
    """Generate config with absolute path for alias."""
    content = f"""server {{
    listen 8091;

    location / {{
        alias {DATA_DIR};
        limit_except GET;
        index index.html;
    }}

    location /files {{
        alias {DATA_DIR};
        limit_except GET;
        index index.html;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path


def test_static_files():
    conf = write_config()
    proc = start_server(conf)
    try:
        # 1. Serve a regular text file
        status, headers, body = request(8091, "/hello.txt")
        assert status == 200, f"Expected 200, got {status}"
        assert body == "hello world", f"Expected 'hello world', got '{body}'"
        assert "text/plain" in headers.get("content-type", ""), \
            f"Expected text/plain, got {headers.get('content-type')}"
        print("PASS: GET /hello.txt -> 200 with correct body and content-type")

        # 2. Serve an HTML file
        status, headers, body = request(8091, "/page.html")
        assert status == 200, f"Expected 200, got {status}"
        assert "text/html" in headers.get("content-type", ""), \
            f"Expected text/html, got {headers.get('content-type')}"
        print("PASS: GET /page.html -> 200 with text/html")

        # 3. Nonexistent file returns 404
        status, _, _ = request(8091, "/nope.txt")
        assert status == 404, f"Expected 404, got {status}"
        print("PASS: GET /nope.txt -> 404")

        # 4. Nested path
        status, _, body = request(8091, "/sub/deep.txt")
        assert status == 200, f"Expected 200, got {status}"
        assert body == "deep content", f"Expected 'deep content', got '{body}'"
        print("PASS: GET /sub/deep.txt -> 200 with correct body")

        # 5. Non-root location strips prefix
        status, _, body = request(8091, "/files/hello.txt")
        assert status == 200, f"Expected 200, got {status}"
        assert body == "hello world", f"Expected 'hello world', got '{body}'"
        print("PASS: GET /files/hello.txt -> 200 (non-root location)")

        # 6. HEAD is not in limit_except -> 405
        status, headers, body = request(8091, "/hello.txt", method="HEAD")
        assert status == 405, f"Expected 405, got {status}"
        print("PASS: HEAD /hello.txt -> 405 (not in limit_except)")

        # 7. Nonexistent path under non-root location returns 404
        status, _, _ = request(8091, "/files/nonexistent.txt")
        assert status == 404, f"Expected 404, got {status}"
        print("PASS: GET /files/nonexistent.txt -> 404")

    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([test_static_files])
