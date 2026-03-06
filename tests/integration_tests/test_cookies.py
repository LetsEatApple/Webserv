#!/usr/bin/env python3
"""
Test cookie support in webserv.

Usage:
    python3 tests/test_cookies.py [host] [port]

Defaults to localhost:8080
"""

import http.cookiejar
import urllib.request
import sys
import traceback

host = sys.argv[1] if len(sys.argv) > 1 else "localhost"
port = sys.argv[2] if len(sys.argv) > 2 else "8080"
base_url = f"http://{host}:{port}"
cgi_path = "/cgi/test_cookie.py"
url = base_url + cgi_path


def test_no_cookies():
    """First request — server should set cookies, no cookies sent."""
    print(f"=== Test 1: First request (no cookies) ===")
    print(f"  URL: {url}")
    req = urllib.request.Request(url)
    resp = urllib.request.urlopen(req, timeout=5)
    body = resp.read().decode()
    headers = resp.info()

    # Collect all Set-Cookie headers
    set_cookies = []
    for key, val in resp.getheaders():
        if key.lower() == "set-cookie":
            set_cookies.append(val)

    # Fallback: try get_all
    if not set_cookies:
        set_cookies = headers.get_all("Set-Cookie") or []

    print(f"  Status: {resp.status}")
    print(f"  All headers: {resp.getheaders()}")
    print(f"  Set-Cookie headers: {set_cookies}")
    print(f"  Body: {body.strip()[:200]}")

    assert resp.status == 200, f"Expected 200, got {resp.status}"
    assert set_cookies, "No Set-Cookie headers received"
    assert any("visit_count=1" in c for c in set_cookies), \
        f"visit_count not set to 1, got: {set_cookies}"
    print("  ✅ PASSED\n")


def test_cookies_sent_back():
    """Multiple requests — cookies should be sent back and visit_count should increment."""
    print("=== Test 2: Cookie persistence (3 requests) ===")
    jar = http.cookiejar.CookieJar()
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))

    for i in range(1, 4):
        resp = opener.open(url, timeout=5)
        body = resp.read().decode()
        cookies_sent = "; ".join(f"{c.name}={c.value}" for c in jar)
        print(f"  Request {i}: status={resp.status}")
        print(f"    Cookies in jar: {cookies_sent}")
        print(f"    Body snippet: {body.strip()[:150]}")
        assert resp.status == 200
        assert f"Visit count: {i}" in body, \
            f"Expected 'Visit count: {i}' in body, got: {body.strip()[:200]}"

    cookies = {c.name: c.value for c in jar}
    print(f"  Final cookie jar: {cookies}")
    assert cookies.get("visit_count") == "3", \
        f"Expected visit_count=3, got {cookies.get('visit_count')}"
    print("  ✅ PASSED\n")


def test_custom_cookie():
    """Send a custom cookie and verify the CGI script receives it."""
    print("=== Test 3: Custom cookie forwarded to CGI ===")
    req = urllib.request.Request(url)
    req.add_header("Cookie", "my_key=my_value; visit_count=41")
    resp = urllib.request.urlopen(req, timeout=5)
    body = resp.read().decode()

    set_cookies = []
    for key, val in resp.getheaders():
        if key.lower() == "set-cookie":
            set_cookies.append(val)
    if not set_cookies:
        set_cookies = resp.info().get_all("Set-Cookie") or []

    print(f"  Status: {resp.status}")
    print(f"  Set-Cookie: {set_cookies}")
    print(f"  Body: {body.strip()[:200]}")

    assert resp.status == 200
    assert "my_key=my_value" in body, \
        f"Custom cookie not forwarded to CGI, body: {body.strip()[:200]}"
    assert any("visit_count=42" in c for c in set_cookies), \
        f"visit_count not incremented to 42, got: {set_cookies}"
    print("  ✅ PASSED\n")


if __name__ == "__main__":
    print(f"Testing cookies against {url}\n")
    passed = 0
    failed = 0
    tests = [test_no_cookies, test_cookies_sent_back, test_custom_cookie]

    for test in tests:
        try:
            test()
            passed += 1
        except AssertionError as e:
            print(f"  ❌ FAILED: {e}\n")
            failed += 1
        except Exception as e:
            print(f"  ❌ ERROR: {e}")
            traceback.print_exc()
            failed += 1

    print(f"Results: {passed} passed, {failed} failed")
    sys.exit(1 if failed else 0)