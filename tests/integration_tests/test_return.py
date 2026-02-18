#!/usr/bin/env python3
"""Integration test for the return directive."""

import subprocess
from helpers import start_server, stop_server, request, conf_path, run_tests


def test_return200():
    proc = start_server(conf_path("return_200.conf"))
    try:
        status, _, body = request(8081, "/")
        assert status == 200, f"Expected 200, got {status}"
        assert "8081" in body, f"Expected '8081' in body, got '{body}'"
        print("PASS: GET :8081/ -> 200 with 8081")

        status, _, body = request(8082, "/")
        assert status == 200, f"Expected 200, got {status}"
        assert "8082" in body, f"Expected '8082' in body, got '{body}'"
        print("PASS: GET :8082/ -> 200 with 8082")

    finally:
        stop_server(proc)


def test_redirect():
    proc = start_server(conf_path("return_redirect.conf"))
    try:
        status, headers, body = request(8090, "/old")
        assert status == 301, f"Expected 301, got {status}"
        assert headers.get("location") == "/new", \
            f"Expected location /new, got {headers.get('location')}"
        print("PASS: GET /old -> 301 /new")

        status, _, body = request(8090, "/new")
        assert status == 200, f"Expected 200, got {status}"
        assert body == "you made it", f"Expected 'you made it', got '{body}'"
        print("PASS: GET /new -> 200 'you made it'")

        result = subprocess.run(
            ["curl", "-s", "-L", "http://127.0.0.1:8090/old"],
            capture_output=True, text=True, timeout=5,
        )
        assert result.stdout == "you made it", \
            f"Expected 'you made it', got '{result.stdout}'"
        print("PASS: curl -L /old follows redirect to /new")

    finally:
        stop_server(proc)


if __name__ == "__main__":
    run_tests([test_return200, test_redirect])
