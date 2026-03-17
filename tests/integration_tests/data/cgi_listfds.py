#!/usr/bin/env python3
import os, sys

fds = []
for fd in range(100):
    try:
        os.fstat(fd)
        fds.append(fd)
    except:
        pass

body = f"FDS: {fds}\n"
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
sys.stdout.flush()
