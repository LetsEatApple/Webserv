#!/usr/bin/env python3
import sys
body = "CGI OK\n"
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
