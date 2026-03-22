#!/usr/bin/env python3
"""Integration test for client idle timeout.

With the default 60s timeout, the idle test is skipped (too slow).
To run the full test with a 3s timeout:

    cmake -S . -B cmake-build-debug -DCLIENT_TIMEOUT_SEC=3
    cmake --build cmake-build-debug
    ctest --test-dir cmake-build-debug -R idle_timeout -V
"""

import socket
import time
import os
from helpers import start_server, stop_server, request, conf_path, run_tests

TIMEOUT_SEC = int(os.environ.get("CLIENT_TIMEOUT_SEC") or "60")


def test_active_client_not_timed_out():
    """A client that sends a request promptly should not be timed out."""
    proc = start_server(conf_path("method.conf"))
    try:
        status, _, _ = request(8093, "/")
        assert status == 200, f"Expected 200, got {status}"
        print("PASS: Active client not timed out -> 200")
    finally:
        stop_server(proc)


def test_idle_connection_closed():
    """A client that connects but sends nothing should get 408 and be closed."""
    if TIMEOUT_SEC > 10:
        print(f"SKIP: CLIENT_TIMEOUT_SEC={TIMEOUT_SEC}s too long for test"
              " (set CLIENT_TIMEOUT_SEC=3 in constants.hpp and env to run)")
        return
    proc = start_server(conf_path("method.conf"))
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(TIMEOUT_SEC + 5)
        s.connect(("127.0.0.1", 8093))
        # Send nothing, wait for server to time us out
        start = time.monotonic()
        resp = b""
        try:
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                resp += chunk
        except socket.timeout:
            pass
        elapsed = time.monotonic() - start
        s.close()

        assert resp, "Expected a response before connection close"
        status_line = resp.split(b"\r\n")[0]
        status = int(status_line.split(b" ")[1])
        assert status == 408, f"Expected 408, got {status}"
        assert elapsed >= TIMEOUT_SEC - 1, \
            f"Closed too early: {elapsed:.1f}s (expected ~{TIMEOUT_SEC}s)"
        print(f"PASS: Idle connection got 408 after {elapsed:.1f}s")

        # Server should still work after timing out idle clients
        status, _, _ = request(8093, "/")
        assert status == 200, f"Expected 200 after timeout, got {status}"
        print("PASS: Server still responsive after timeout")
    finally:
        stop_server(proc)


if __name__ == "__main__":
    run_tests([test_active_client_not_timed_out, test_idle_connection_closed])
