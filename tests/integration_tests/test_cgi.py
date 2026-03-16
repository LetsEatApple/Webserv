#!/usr/bin/env python3
"""Integration tests for CGI execution (Python and Perl)."""

import os
import tempfile
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")


def write_config():
    content = f"""server {{
    listen 8094;

    location / {{
        alias {DATA_DIR};
        limit_except GET POST;
        cgi .py /usr/bin/python3;
        cgi .pl /usr/bin/perl;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path


def test_cgi_python():
    conf = write_config()
    proc = start_server(conf)
    try:
        # 1. Basic Python CGI
        status, headers, body = request(8094, "/cgi_hello.py")
        assert status == 200, f"Expected 200, got {status}"
        assert "CGI OK" in body, f"Expected 'CGI OK' in body, got '{body}'"
        print("PASS: Python CGI -> 200 with correct body")

        # 2. Python CGI with query string
        status, headers, body = request(8094, "/cgi_env.py?foo=bar")
        assert status == 200, f"Expected 200, got {status}"
        assert "QUERY_STRING=foo=bar" in body, f"Missing QUERY_STRING in:\n{body}"
        assert "REQUEST_METHOD=GET" in body, f"Missing REQUEST_METHOD in:\n{body}"
        assert "GATEWAY_INTERFACE=CGI/1.1" in body, f"Missing GATEWAY_INTERFACE in:\n{body}"
        print("PASS: Python CGI env vars correct (QUERY_STRING, REQUEST_METHOD, GATEWAY_INTERFACE)")

        # 3. POST with body
        status, headers, body = request(8094, "/cgi_post.py", method="POST",
                                        body="hello world",
                                        headers={"Content-Type": "text/plain"})
        assert status == 200, f"Expected 200, got {status}"
        assert "received: hello world" in body, f"Expected echo, got '{body}'"
        print("PASS: Python CGI POST with body echoed back")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_cgi_perl():
    conf = write_config()
    proc = start_server(conf)
    try:
        status, headers, body = request(8094, "/cgi_hello.pl")
        assert status == 200, f"Expected 200, got {status}"
        assert "CGI OK" in body, f"Expected 'CGI OK' in body, got '{body}'"
        print("PASS: Perl CGI -> 200 with correct body")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_cgi_nonexistent_script():
    conf = write_config()
    proc = start_server(conf)
    try:
        status, _, _ = request(8094, "/nonexistent.py")
        assert status == 502, f"Expected 502, got {status}"
        print("PASS: Nonexistent CGI script -> 502")
    finally:
        stop_server(proc)
        os.unlink(conf)


def test_cgi_path_info():
    conf = write_config()
    proc = start_server(conf)
    try:
        # PATH_INFO should be the full request URI path (42 tester convention)
        status, headers, body = request(8094, "/cgi_env.py/extra/path?foo=bar")
        assert status == 200, f"Expected 200, got {status}"
        assert "PATH_INFO=/cgi_env.py/extra/path" in body, f"Expected PATH_INFO=/cgi_env.py/extra/path in:\n{body}"
        assert "SCRIPT_NAME=/cgi_env.py" in body, f"Expected SCRIPT_NAME=/cgi_env.py in:\n{body}"
        print("PASS: PATH_INFO is full request URI path")

        # No extra path -> PATH_INFO should be the script path
        status, headers, body = request(8094, "/cgi_env.py?foo=bar")
        assert "PATH_INFO=/cgi_env.py" in body, f"Expected PATH_INFO=/cgi_env.py in:\n{body}"
        print("PASS: PATH_INFO equals script path when no extra path")
    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([test_cgi_python, test_cgi_perl, test_cgi_nonexistent_script, test_cgi_path_info])
