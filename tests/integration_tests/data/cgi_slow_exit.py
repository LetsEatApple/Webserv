#!/usr/bin/env python3
"""CGI script that closes all fds then sleeps before exiting."""
import os
import sys

body = "SLOW CGI OK\n"
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("Content-Length: %d\r\n" % len(body))
sys.stdout.write("\r\n")
sys.stdout.write(body)
sys.stdout.flush()

# Close ALL file descriptors so the server sees EOF
for fd in range(256):
    try:
        os.close(fd)
    except OSError:
        pass

# Child is still alive with no open fds – sleep long enough that
# the server's WNOHANG waitpid returns 0, then exit to become a zombie.
import time
time.sleep(3)
