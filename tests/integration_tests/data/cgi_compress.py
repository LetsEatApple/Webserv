#!/usr/bin/env python3
"""Compresses POST body on the fly using gzip.

Demonstrates streaming: reads stdin in chunks, writes compressed
output incrementally. With body streaming, the CGI starts compressing
before the full upload arrives. Without it, the entire body must be
buffered in the server before compression can begin.
"""
import os
import sys
import zlib
import time

content_length = int(os.environ.get("CONTENT_LENGTH", "0"))

sys.stdout.write("Content-Type: application/gzip\r\n")
sys.stdout.write("X-Original-Size: %d\r\n" % content_length)
sys.stdout.write("\r\n")
sys.stdout.flush()

compressor = zlib.compressobj(6, zlib.DEFLATED, 31)  # 31 = gzip format
total_in = 0
total_out = 0
start = time.time()

while total_in < content_length:
    to_read = min(8192, content_length - total_in)
    chunk = sys.stdin.buffer.read(to_read)
    if not chunk:
        break
    total_in += len(chunk)
    compressed = compressor.compress(chunk)
    if compressed:
        sys.stdout.buffer.write(compressed)
        sys.stdout.buffer.flush()
        total_out += len(compressed)

tail = compressor.flush()
if tail:
    sys.stdout.buffer.write(tail)
    sys.stdout.buffer.flush()
    total_out += len(tail)

elapsed = time.time() - start
sys.stderr.write("compress: %d -> %d bytes (%.1f%%) in %.3fs\n" % (
    total_in, total_out, 100.0 * total_out / max(total_in, 1), elapsed))
