#!/usr/bin/env python3
"""Echoes back the POST body."""
import os
import sys

content_length = int(os.environ.get("CONTENT_LENGTH", "0"))
body_in = sys.stdin.read(content_length) if content_length > 0 else ""

body = f"received: {body_in}\n"
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
