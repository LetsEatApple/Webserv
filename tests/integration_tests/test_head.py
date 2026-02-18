#!/usr/bin/env python3
"""Integration tests for HEAD requests.

- HEAD on static file returns 200 with Content-Length but empty body
- HEAD on index.html returns 200 with empty body
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
        status, headers, body = request(8094, "/hello.txt", method="HEAD")
        assert status == 200, f"Expected 200, got {status}"
        assert body == "", f"Expected empty body, got '{body}'"
        assert int(headers.get("content-length", 0)) > 0, \
            "Expected non-zero Content-Length"
        print("PASS: HEAD /hello.txt -> 200 with empty body")

        status, headers, body = request(8094, "/index.html", method="HEAD")
        assert status == 200, f"Expected 200, got {status}"
        assert body == "", f"Expected empty body, got '{body}'"
        print("PASS: HEAD /index.html -> 200 with empty body")

    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([test_head_static_file])
