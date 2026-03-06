#!/usr/bin/env python3
"""
Test cookie support in webserv.

Usage:
    python3 tests/test_cookies.py [host] [port]

Defaults to localhost:8080
"""

import os
from helpers import SCRIPT_DIR, start_server, stop_server, run_tests
import tempfile
import requests

DATA_DIR = os.path.join(SCRIPT_DIR, "data")

port = 8095
def write_config():
    content = f"""server {{
    listen {port};

    location / {{
        alias {DATA_DIR}/cgi;
        cgi .py /usr/bin/python3;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path

def test_no_cookies():
    conf = write_config()
    proc = start_server(conf)
    try:
        r = requests.get(f"http://127.0.0.1:{port}/test_cookie.py", allow_redirects=False)
        assert r.status_code == 200, f"Expected 200, got {r.status_code}"

        assert "visit_count" in r.cookies, f"No visit_count cookie, got {dict(r.cookies)}"
        assert r.cookies["visit_count"] == "1", f"Expected visit_count=1, got: {r.cookies['visit_count']}"

    finally:
        stop_server(proc)
        os.unlink(conf)

def test_cookies_sent_back():
    conf = write_config()
    proc = start_server(conf)
    try:
        session = requests.Session()
        for i in range(1, 4):
            r = session.get(f"http://127.0.0.1:{port}/test_cookie.py", allow_redirects=False)
            assert r.status_code == 200, f"Expected 200, got {r.status_code}"

            assert "visit_count" in r.cookies, f"No visit_count cookie, got {dict(r.cookies)}"
            assert r.cookies["visit_count"] == str(i), f"Expected visit_count={i}, got: {r.cookies['visit_count']}"
    finally:
        stop_server(proc)
        os.unlink(conf)

def test_custom_cookies():
    conf = write_config()
    proc = start_server(conf)

    try:
        cookies = dict(my_key="my_value; visit_count=41")
        r = requests.get(f"http://127.0.0.1:{port}/test_cookie.py", cookies=cookies, allow_redirects=False)
        assert r.status_code == 200, f"Expected 200, got {r.status_code}"
        assert "my_key=my_value" in r.text, \
            f"Custom cookie not forwarded to CGI, body: {r.text.strip()[:200]}"
        assert "visit_count" in r.cookies, f"No visit_count cookie, got {dict(r.cookies)}"
        assert r.cookies["visit_count"] == "42", f"Expected visit_count=42, got: {r.cookies['visit_count']}"
    finally:
        stop_server(proc)
        os.unlink(conf)

if __name__ == "__main__":
    run_tests([test_no_cookies, test_cookies_sent_back, test_custom_cookies])