#!/usr/bin/env python3
"""Reads entire POST body from stdin and reports how many bytes were received."""
import os
import sys

content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
total = 0
while total < content_length:
    chunk = sys.stdin.buffer.read(min(8192, content_length - total))
    if not chunk:
        break
    total += len(chunk)

body = f"bytes={total}\n"
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
