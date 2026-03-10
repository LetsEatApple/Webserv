#!/usr/bin/env python3
"""Integration tests for max header size limit (8KB)."""

import socket
import time
from helpers import start_server, stop_server, request, conf_path, run_tests


def raw_request(port, raw):
    """Send raw bytes in chunks and return the response status code."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    s.connect(("127.0.0.1", port))
    try:
        for i in range(0, len(raw), 512):
            s.send(raw[i:i+512])
            time.sleep(0.005)
    except BrokenPipeError:
        pass
    try:
        s.shutdown(socket.SHUT_WR)
    except OSError:
        pass
    resp = b""
    try:
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            resp += chunk
    except (socket.timeout, ConnectionReset, OSError):
        pass
    s.close()
    if resp:
        status_line = resp.split(b"\r\n")[0]
        return int(status_line.split(b" ")[1])
    return 0


def test_header_limit():
    proc = start_server(conf_path("method.conf"))
    try:
        # 1. Normal request with small headers should succeed
        status, _, _ = request(8093, "/")
        assert status == 200, f"Expected 200, got {status}"
        print("PASS: Normal request with small headers -> 200")

        # 2. Single header larger than 8KB should be rejected
        big_value = "X" * 9000
        raw = (
            "GET / HTTP/1.1\r\n"
            "Host: localhost\r\n"
            f"X-Big: {big_value}\r\n"
            "\r\n"
        ).encode()
        status = raw_request(8093, raw)
        assert status == 400, f"Expected 400, got {status}"
        print("PASS: Oversized header (9KB) -> 400")

        # 3. Many small headers totalling > 8KB should be rejected
        headers = "".join(f"X-H{i}: {'A' * 80}\r\n" for i in range(120))
        raw = (
            "GET / HTTP/1.1\r\n"
            "Host: localhost\r\n"
            + headers
            + "\r\n"
        ).encode()
        assert len(raw) > 8192, f"Request too small: {len(raw)}"
        status = raw_request(8093, raw)
        assert status == 400, f"Expected 400, got {status}"
        print("PASS: Many small headers (>8KB total) -> 400")

        # 4. Server still works after rejecting oversized requests
        status, _, _ = request(8093, "/")
        assert status == 200, f"Expected 200, got {status}"
        print("PASS: Server still responsive after rejections -> 200")

        # 5. Request just under 8KB should succeed
        big_value = "X" * 7900
        raw = (
            "GET / HTTP/1.1\r\n"
            "Host: localhost\r\n"
            f"X-Big: {big_value}\r\n"
            "\r\n"
        ).encode()
        assert len(raw) <= 8192, f"Request too large: {len(raw)}"
        status = raw_request(8093, raw)
        assert status == 200, f"Expected 200, got {status}"
        print("PASS: Request just under 8KB limit -> 200")

    finally:
        stop_server(proc)


if __name__ == "__main__":
    run_tests([test_header_limit])
