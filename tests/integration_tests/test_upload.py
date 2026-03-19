#!/usr/bin/env python3
"""Tests for file upload via POST with the upload directive."""

import os
import shutil
import tempfile
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")
PORT = 8099


def make_dirs():
    upload_dir = tempfile.mkdtemp(prefix="webserv_upload_")
    return upload_dir


def write_config(upload_dir):
    content = f"""server {{
    listen {PORT};

    location / {{
        alias {upload_dir};
        limit_except GET POST DELETE;
        upload {upload_dir};
    }}

    location /noupload {{
        alias {upload_dir};
        limit_except GET POST;
    }}

    location /readonly {{
        alias {upload_dir};
        limit_except GET POST;
        upload /nonexistent_dir_xyz;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path


def test_upload_and_retrieve():
    """POST uploads a file, GET retrieves it."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        body = "hello world"
        status, _, resp = request(PORT, "/test.txt", method="POST", body=body,
                                  headers={"Content-Type": "text/plain"})
        assert status == 200, f"Upload: expected 200, got {status}"
        assert "test.txt" in resp, f"Upload response missing filename: {resp}"

        # File should exist on disk
        filepath = os.path.join(upload_dir, "test.txt")
        assert os.path.exists(filepath), "Uploaded file not found on disk"
        with open(filepath) as f:
            assert f.read() == body, "File content doesn't match"

        # GET it back
        status, _, resp = request(PORT, "/test.txt")
        assert status == 200, f"GET: expected 200, got {status}"
        assert resp == body, f"GET body doesn't match: {resp}"

        print("PASS: upload and retrieve")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)


def test_upload_delete_cycle():
    """POST uploads, DELETE removes, GET returns 404."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        status, _, _ = request(PORT, "/cycle.txt", method="POST", body="data",
                               headers={"Content-Type": "text/plain"})
        assert status == 200, f"Upload: expected 200, got {status}"

        status, _, _ = request(PORT, "/cycle.txt", method="DELETE")
        assert status == 204, f"Delete: expected 204, got {status}"

        status, _, _ = request(PORT, "/cycle.txt")
        assert status == 404, f"After delete: expected 404, got {status}"

        print("PASS: upload → delete → 404")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)


def test_upload_overwrite():
    """Uploading to the same filename overwrites the file."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        request(PORT, "/over.txt", method="POST", body="first",
                headers={"Content-Type": "text/plain"})
        request(PORT, "/over.txt", method="POST", body="second",
                headers={"Content-Type": "text/plain"})

        status, _, resp = request(PORT, "/over.txt")
        assert status == 200
        assert resp == "second", f"Expected 'second', got '{resp}'"

        print("PASS: upload overwrite")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)


def test_upload_binary():
    """Binary content is preserved exactly."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        import http.client
        binary = bytes(range(256))
        conn = http.client.HTTPConnection("127.0.0.1", PORT, timeout=5)
        conn.request("POST", "/binary.bin", body=binary,
                     headers={"Content-Type": "application/octet-stream"})
        resp = conn.getresponse()
        resp.read()
        assert resp.status == 200

        filepath = os.path.join(upload_dir, "binary.bin")
        with open(filepath, "rb") as f:
            saved = f.read()
        assert saved == binary, f"Binary mismatch: {len(saved)} vs {len(binary)}"
        conn.close()

        print("PASS: binary upload preserved")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)


def test_upload_no_filename():
    """POST to / (no filename) returns 400."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        status, _, _ = request(PORT, "/", method="POST", body="data",
                               headers={"Content-Type": "text/plain"})
        assert status == 400, f"No filename: expected 400, got {status}"

        print("PASS: no filename → 400")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)


def test_upload_path_traversal():
    """Path traversal is neutralized by routing normalization.
    /../../../etc/passwd is normalized to /etc/passwd, filename='passwd',
    saved safely inside upload_dir — not at /etc/passwd."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        status, _, _ = request(PORT, "/../../../etc/passwd", method="POST",
                               body="safe",
                               headers={"Content-Type": "text/plain"})
        # Routing normalizes the path; file is saved as upload_dir/passwd
        assert status == 200, f"Expected 200 (normalized), got {status}"
        saved = os.path.join(upload_dir, "passwd")
        assert os.path.exists(saved), "File not saved in upload dir"
        assert not os.path.exists("/tmp/evil_marker"), "Traversal escaped upload dir"

        print("PASS: path traversal neutralized by routing")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)


def test_upload_no_directive():
    """POST to location without upload directive returns 204."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        status, _, _ = request(PORT, "/noupload/file.txt", method="POST",
                               body="data",
                               headers={"Content-Type": "text/plain"})
        assert status == 204, f"No upload directive: expected 204, got {status}"

        print("PASS: no upload directive → 204")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)


def test_upload_bad_directory():
    """Upload to nonexistent directory returns 500."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        status, _, _ = request(PORT, "/readonly/file.txt", method="POST",
                               body="data",
                               headers={"Content-Type": "text/plain"})
        assert status == 500, f"Bad directory: expected 500, got {status}"

        print("PASS: bad upload directory → 500")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

if __name__ == "__main__":
    run_tests([
        test_upload_and_retrieve,
        test_upload_delete_cycle,
        test_upload_overwrite,
        test_upload_binary,
        test_upload_no_filename,
        test_upload_path_traversal,
        test_upload_no_directive,
        test_upload_bad_directory,
    ])
