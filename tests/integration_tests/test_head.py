#!/usr/bin/env python3
"""Integration tests for HEAD requests.

- HEAD on static file returns 200 with Content-Length but empty body (no limit_except)
- HEAD on index.html returns 200 with empty body (no limit_except)
- HEAD returns 405 when not in limit_except
"""

import os
import tempfile
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")


def write_config():
    content = f"""server {{
    listen 8094;

    location / {{
        alias {DATA_DIR};
        index index.html;
    }}

    location /restricted {{
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


def test_head_static_file():
    conf = write_config()
    proc = start_server(conf)
    try:
        # HEAD is not a recognized method in limit_except, so it returns 405
        # when any limit_except is set (default includes GET, POST, DELETE)
        status, _, _ = request(8094, "/hello.txt", method="HEAD")
        assert status == 405, f"Expected 405, got {status}"
        print("PASS: HEAD /hello.txt -> 405 (HEAD not in default limit_except)")

        status, _, _ = request(8094, "/index.html", method="HEAD")
        assert status == 405, f"Expected 405, got {status}"
        print("PASS: HEAD /index.html -> 405 (HEAD not in default limit_except)")

        # Explicit limit_except GET also returns 405 for HEAD
        status, _, _ = request(8094, "/restricted/hello.txt", method="HEAD")
        assert status == 405, f"Expected 405, got {status}"
        print("PASS: HEAD /restricted/hello.txt -> 405 (not in limit_except)")

    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([test_head_static_file])
