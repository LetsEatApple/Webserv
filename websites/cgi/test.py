#!/usr/bin/env python3
import sys, time
time.sleep(10)          # sleep BEFORE reading stdin
data = sys.stdin.read()
print("Content-Type: text/plain\r\n\r\n")
print(f"Got {len(data)} bytes")
