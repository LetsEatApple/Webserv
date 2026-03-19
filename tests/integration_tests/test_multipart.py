#!/usr/bin/env python3
"""Tests for file upload via HTML form with the upload directive."""

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

def test_upload_multipart_and_retrieve():
    """POST multipart/form-data uploads a file, GET retrieves it."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    name = "one.txt"
    content = "Hello one\n"
    try:
        boundary = "----WebServBoundary1234"
        body = (
            f"--{boundary}\r\n"
            f"Content-Disposition: form-data; name=\"file1\"; filename=\"{name}\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            f"{content}"
            f"--{boundary}--\r\n"
        )

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        status, _, resp = request(PORT, "/", method="POST", body=body, headers=headers)
        assert status == 200, f"Multipart upload: expected 200, got {status}"
        assert f"{name}" in resp, f"Response missing filenames: {resp}"

        path = os.path.join(upload_dir, name)
        assert os.path.exists(path), f"{name} not found"
        with open(path) as f:
            assert f.read() == content, f"{name} content mismatch"

        status, _, resp = request(PORT, f"/{name}")
        assert status == 200, f"GET: expected 200, got {status}"
        assert resp == content, f"GET body doesn't match: {resp}"

        print("PASS: upload and retrieve")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

def test_upload_multipart_delete_cycle():
    """POST multipart/form-data uploads, DELETE removes, GET returns 404."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    name = "one.txt"
    content = "Hello one\n"
    try:
        boundary = "----WebServBoundary1234"
        body = (
            f"--{boundary}\r\n"
            f"Content-Disposition: form-data; name=\"file1\"; filename=\"{name}\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            f"{content}"
            f"--{boundary}--\r\n"
        )

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        status, _, _ = request(PORT, "/", method="POST", body=body, headers=headers)
        assert status == 200, f"Multipart upload: expected 200, got {status}"

        status, _, _ = request(PORT, f"/{name}", method="DELETE", body=body, headers=headers)
        assert status == 204, f"Delete: expected 204, got {status}"

        status, _, _ = request(PORT, f"/{name}")
        assert status == 404, f"After delete: expected 404, got {status}"
        
        print("PASS: upload → delete → 404")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

def test_upload_multipart_overwrite():
    """Uploading to the same filename overwrites the file."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    name = "one.txt"
    content = "Hello one\n"
    content2 = "Hello two\n"
    try:
        boundary = "----WebServBoundary1234"
        body = (
            f"--{boundary}\r\n"
            f"Content-Disposition: form-data; name=\"file1\"; filename=\"{name}\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            f"{content}"
            f"--{boundary}--\r\n"
        )

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        boundary2 = "----WebServBoundary4321"
        body2 = (
            f"--{boundary2}\r\n"
            f"Content-Disposition: form-data; name=\"file1\"; filename=\"{name}\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            f"{content2}"
            f"--{boundary2}--\r\n"
        )

        headers2 = {
            "Content-Type": f"multipart/form-data; boundary={boundary2}"
        }

        request(PORT, "/", method="POST", body=body,
                headers=headers)
        request(PORT, "/", method="POST", body=body2,
                headers=headers2)

        status, _, resp = request(PORT, f"/{name}")
        assert status == 200
        assert resp == content2, f"Expected '{content2}', got '{resp}'"

        print("PASS: upload overwrite")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

def test_upload_multipart():
    """POST multipart/form-data with multiple files."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        boundary = "----WebServBoundary1234"
        body = (
            f"--{boundary}\r\n"
            "Content-Disposition: form-data; name=\"file1\"; filename=\"one.txt\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Hello One\n"
            f"--{boundary}\r\n"
            "Content-Disposition: form-data; name=\"file2\"; filename=\"two.txt\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Hello Two\n"
            f"--{boundary}--\r\n"
        )

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        status, _, resp = request(PORT, "/", method="POST", body=body, headers=headers)
        assert status == 200, f"Multipart upload: expected 200, got {status}"
        assert "one.txt" in resp and "two.txt" in resp, f"Response missing filenames: {resp}"

        for fname, content in [("one.txt", "Hello One\n"), ("two.txt", "Hello Two\n")]:
            path = os.path.join(upload_dir, fname)
            assert os.path.exists(path), f"{fname} not found in {resp}"
            with open(path) as f:
                assert f.read() == content, f"{fname} content mismatch"

        print("PASS: multipart upload with multiple files")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

def test_upload_multipart_binary():
    """POST multipart/form-data with multiple binary files."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        boundary = "----WebServBinaryBoundary1234"

        binary1 = bytes(range(128))
        binary2 = bytes(range(128, 256))
        body = (
                f"--{boundary}\r\n"
                + "Content-Disposition: form-data; name=\"bin1\"; filename=\"b1.bin\"\r\n"
                + "Content-Type: application/octet-stream\r\n\r\n"
                + binary1.decode('latin1') + "\r\n"
                + f"--{boundary}\r\n"
                + "Content-Disposition: form-data; name=\"bin2\"; filename=\"b2.bin\"\r\n"
                + "Content-Type: application/octet-stream\r\n\r\n"
                + binary2.decode('latin1') + "\r\n"
                + f"--{boundary}--\r\n"
        )

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        status, _, resp = request(PORT, "/", method="POST", body=body, headers=headers)
        assert status == 200, f"Multipart binary upload: expected 200, got {status}"
        assert "b1.bin" in resp and "b2.bin" in resp, f"Response missing filenames: {resp}"

        for fname, data in [("b1.bin", binary1), ("b2.bin", binary2)]:
            path = os.path.join(upload_dir, fname)
            assert os.path.exists(path), f"{fname} not found"
            with open(path, "rb") as f:
                saved = f.read()
            assert saved == data, f"{fname} binary content mismatch"

        print("PASS: multipart binary upload with multiple files")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

def test_upload_multipart_no_files_mixed_data():
    """POST multipart without filename returns 200."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        boundary = "----WebServBoundary1234"
        body = (
            f"--{boundary}\r\n"
            "Content-Disposition: form-data; name=\"username\"\r\n\r\n"
            "test_user\r\n"
            
            f"--{boundary}\r\n"
            "Content-Disposition: form-data; name=\"config\"\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"debug\":true}\r\n"
            f"--{boundary}--\r\n"
        )

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        status, _, resp = request(PORT, "/", method="POST", body=body, headers=headers)
        assert status == 200, f"Expected status 200, got {status}"
        assert "Fields received: 2" in resp, f"Wrong receive message"
        assert "Files uploaded successfully: 0" in resp, f"Wrong upload message"
        print("PASS: no filename, mixed content")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

def test_upload_multipart_no_fields():
    """POST multipart without filename returns 200."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        boundary = "----WebServBoundary1234"
        body = f"--{boundary}--\r\n"

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        status, _, resp = request(PORT, "/", method="POST", body=body, headers=headers)
        assert status == 200, f"Expected status 200, got {status}"
        assert "Fields received: 0" in resp, f"Wrong receive message"
        assert "Files uploaded successfully: 0" in resp, f"Wrong upload message"
        print("PASS: no fields")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

def test_upload_multipart_bad_directory():
    """Upload to nonexistent directory returns 500."""
    upload_dir = make_dirs()
    conf = write_config(upload_dir)
    proc = start_server(conf)
    try:
        boundary = "----WebServBoundary1234"
        body = (
            f"--{boundary}\r\n"
            "Content-Disposition: form-data; name=\"file1\"; filename=\"one.txt\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Hello One\n"
            f"--{boundary}\r\n"
            "Content-Disposition: form-data; name=\"file2\"; filename=\"two.txt\"\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Hello Two\n"
            f"--{boundary}--\r\n"
        )

        headers = {
            "Content-Type": f"multipart/form-data; boundary={boundary}"
        }

        status, _, _ = request(PORT, "/readonly", method="POST",
                               body=body,
                               headers=headers)
        assert status == 500, f"Bad directory: expected 500, got {status}"

        print("PASS: bad upload directory → 500")
    finally:
        stop_server(proc)
        os.unlink(conf)
        shutil.rmtree(upload_dir)

if __name__ == "__main__":
    run_tests([
        test_upload_multipart,
        test_upload_multipart_binary,
        test_upload_multipart_and_retrieve,
        test_upload_multipart_delete_cycle,
        test_upload_multipart_overwrite,
        test_upload_multipart_no_files_mixed_data,
        test_upload_multipart_no_fields,
        test_upload_multipart_bad_directory,
    ])
