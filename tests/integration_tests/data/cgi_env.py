#!/usr/bin/env python3
"""Prints CGI environment variables as key=value pairs."""
import os
import sys

env_vars = [
    "REQUEST_METHOD", "SCRIPT_NAME", "QUERY_STRING",
    "CONTENT_TYPE", "CONTENT_LENGTH", "PATH_INFO",
    "SERVER_PROTOCOL", "GATEWAY_INTERFACE",
]

lines = []
for var in env_vars:
    val = os.environ.get(var, "")
    lines.append(f"{var}={val}")

body = "\n".join(lines) + "\n"
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
