#!/usr/bin/env python3
"""Reads POST body and echoes it back uppercased. Mimics the 42 cgi_tester."""
import os
import sys

content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
if content_length > 0:
    data = b""
    while len(data) < content_length:
        chunk = sys.stdin.buffer.read(min(65536, content_length - len(data)))
        if not chunk:
            break
        data += chunk
else:
    data = sys.stdin.buffer.read()

out = data.upper()
sys.stdout.buffer.write(b"Content-Type: text/plain\r\n")
sys.stdout.buffer.write(b"\r\n")
sys.stdout.buffer.write(out)
sys.stdout.buffer.flush()
