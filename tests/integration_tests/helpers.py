"""Shared helpers for integration tests."""

import argparse
import subprocess
import time
import signal
import sys
import os
import http.client

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = None
TARGET = "webserv"


def build():
    print(f"Building {TARGET}...")
    result = subprocess.run(
        ["cmake", "--build", BUILD_DIR, "--target", TARGET],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(result.stderr)
        sys.exit(1)
    print("Build OK")


def start_server(config):
    executable = os.path.join(BUILD_DIR, TARGET)
    proc = subprocess.Popen(
        [executable, config],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    time.sleep(1)
    return proc


def stop_server(proc):
    proc.send_signal(signal.SIGINT)
    proc.wait(timeout=5)


def request(port, path, method="GET", body=None, headers=None):
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=5)
    conn.request(method, path, body=body, headers=headers or {})
    resp = conn.getresponse()
    resp_body = resp.read().decode()
    resp_headers = dict(resp.getheaders())
    conn.close()
    return resp.status, resp_headers, resp_body


def conf_path(name):
    return os.path.join(SCRIPT_DIR, "conf", name)


def run_tests(tests):
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True,
                        help="Path to CMake build directory")
    args = parser.parse_args()

    global BUILD_DIR
    BUILD_DIR = args.build_dir

    build()
    failed = False
    for test in tests:
        try:
            print(f"\n--- {test.__name__} ---")
            test()
        except Exception as e:
            print(f"FAIL: {e}")
            failed = True
    sys.exit(1 if failed else 0)
