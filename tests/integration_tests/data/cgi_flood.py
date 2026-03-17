#!/usr/bin/env python3
"""Outputs a large amount of data to test output backpressure."""
import sys

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()

# Write 1MB in 1KB chunks
chunk = "X" * 1024 + "\n"
for _ in range(1024):
    sys.stdout.write(chunk)
    sys.stdout.flush()
