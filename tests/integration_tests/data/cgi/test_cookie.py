#!/usr/bin/env python3
import os

cookie_header = os.environ.get("HTTP_COOKIE", "")

cookies = {}
if cookie_header:
    for pair in cookie_header.split(";"):
        pair = pair.strip()
        if "=" in pair:
            k, v = pair.split("=", 1)
            cookies[k.strip()] = v.strip()

visit_count = int(cookies.get("visit_count", "0")) + 1

print(f"Set-Cookie: visit_count={visit_count}; Path=/; Max-Age=3600")
print(f"Set-Cookie: last_cgi=cookie_test; Path=/; HttpOnly")
print("Content-Type: text/html")
print()
print("<html><body>")
print(f"<h1>Visit count: {visit_count}</h1>")
if cookie_header:
    print(f"<p>Received cookies: {cookie_header}</p>")
else:
    print("<p>No cookies received (first visit)</p>")
print("</body></html>")