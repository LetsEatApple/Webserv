#!/usr/bin/env python3
"""Integration tests for HTTP method handling.

- POST and DELETE on GET-only location return 405
- POST with small body returns 200
- POST exceeding client_max_body_size returns 413
"""

from helpers import start_server, stop_server, request, conf_path, run_tests


def test_method_not_allowed():
    proc = start_server(conf_path("method.conf"))
    try:
        status, _, _ = request(8093, "/", method="POST", body="hello",
                               headers={"Content-Length": "5"})
        assert status == 405, f"Expected 405, got {status}"
        print("PASS: POST / -> 405")

        status, _, _ = request(8093, "/", method="DELETE")
        assert status == 405, f"Expected 405, got {status}"
        print("PASS: DELETE / -> 405")

    finally:
        stop_server(proc)


def test_post_body():
    proc = start_server(conf_path("method.conf"))
    try:
        body = "A" * 50
        status, _, resp_body = request(8093, "/post_body", method="POST",
                                       body=body,
                                       headers={"Content-Length": str(len(body))})
        assert status == 200, f"Expected 200, got {status}"
        assert resp_body == "ok", f"Expected 'ok', got '{resp_body}'"
        print("PASS: POST /post_body with 50 bytes -> 200")

    finally:
        stop_server(proc)


def test_post_body_too_large():
    proc = start_server(conf_path("method.conf"))
    try:
        body = "A" * 200
        status, _, _ = request(8093, "/post_body", method="POST",
                               body=body,
                               headers={"Content-Length": str(len(body))})
        assert status == 413, f"Expected 413, got {status}"
        print("PASS: POST /post_body with 200 bytes -> 413")

    finally:
        stop_server(proc)


if __name__ == "__main__":
    run_tests([test_method_not_allowed, test_post_body, test_post_body_too_large])
