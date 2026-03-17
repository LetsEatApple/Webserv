#!/usr/bin/env python3
"""CGI that prints lines slowly to test streaming."""
import sys
import time

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()

for i in range(3):
    sys.stdout.write(f"line {i}\n")
    sys.stdout.flush()
    time.sleep(0.2)
