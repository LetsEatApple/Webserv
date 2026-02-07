#!/usr/bin/env python3
"""Integration test for the return directive using return_redirect.conf"""

import subprocess
import time
import signal
import sys
import os
import http.client

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_ROOT, "cmake-build-debug")
TARGET = "webserv"


def build():
    print(f"Building {TARGET}...")
    result = subprocess.run(
        ["cmake", "--build", BUILD_DIR, "--target", TARGET],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(result.stderr)
        sys.exit(1)
    print("Build OK")


def start_server(config):
    executable = os.path.join(BUILD_DIR, TARGET)
    proc = subprocess.Popen(
        [executable, config],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    time.sleep(1)
    return proc


def stop_server(proc):
    proc.send_signal(signal.SIGINT)
    proc.wait(timeout=5)


def get(port, path):
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    conn.request("GET", path)
    resp = conn.getresponse()
    body = resp.read().decode()
    headers = dict(resp.getheaders())
    conn.close()
    return resp.status, resp.reason, headers, body

def test_return200():
    proc = start_server(os.path.join(PROJECT_ROOT, "tests_python/conf/return_200.conf"))
    try:
        status, reason, headers, body = get(8081, "/")
        assert status == 200, f"Expected 200, got {status}"
        assert "8081" in body, f"Expected '8081' in body, got '{body}'"
        print("PASS: GET :8081/ -> 200 with 8081")

        status, reason, headers, body = get(8082, "/")
        assert status == 200, f"Expected 200, got {status}"
        assert "8082" in body, f"Expected '8082' in body, got '{body}'"
        print("PASS: GET :8082/ -> 200 with 8082")

    finally:
        stop_server(proc)

def test_redirect():
    proc = start_server(os.path.join(PROJECT_ROOT, "tests_python/conf/return_redirect.conf"))
    try:
        # GET /old -> 301 redirect to /new
        status, reason, headers, body = get(8090, "/old")
        assert status == 301, f"Expected 301, got {status}"
        assert headers.get("location") == "/new", f"Expected location /new, got {headers.get('location')}"
        print("PASS: GET /old -> 301 /new")

        # GET /new -> 200 with body
        status, reason, headers, body = get(8090, "/new")
        assert status == 200, f"Expected 200, got {status}"
        assert body == "you made it", f"Expected 'you made it', got '{body}'"
        print("PASS: GET /new -> 200 'you made it'")

        # curl -L behavior: follow redirect from /old to /new
        result = subprocess.run(
            ["curl", "-s", "-L", "http://127.0.0.1:8090/old"],
            capture_output=True, text=True, timeout=5,
        )
        assert result.stdout == "you made it", f"Expected 'you made it', got '{result.stdout}'"
        print("PASS: curl -L /old follows redirect to /new")

    finally:
        stop_server(proc)




if __name__ == "__main__":
    build()
    failed = False
    for test in [test_return200, test_redirect]:
        try:
            print(f"\n--- {test.__name__} ---")
            test()
        except Exception as e:
            print(f"FAIL: {e}")
            failed = True
    sys.exit(1 if failed else 0)
