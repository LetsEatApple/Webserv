#!/usr/bin/env python3
"""Integration test: CGI requests must not leave zombie processes.

Sends several CGI requests and checks that no child of the webserv
process is in zombie (Z) state after each response is fully received.
"""

import os
import subprocess
import tempfile
import time
import http.client
from helpers import start_server, stop_server, request, run_tests, SCRIPT_DIR

DATA_DIR = os.path.join(SCRIPT_DIR, "data")
PYTHON3 = "/usr/bin/python3"


def write_config():
    content = f"""server {{
    listen 8096;

    cgi .py {PYTHON3};

    location / {{
        alias {DATA_DIR};
        index index.html;
    }}
}}
"""
    fd, path = tempfile.mkstemp(suffix=".conf")
    os.write(fd, content.encode())
    os.close(fd)
    return path


def get_zombies(server_pid):
    """Return list of zombie children of the given PID."""
    try:
        result = subprocess.run(
            ["ps", "--ppid", str(server_pid), "-o", "pid=,stat=,comm="],
            capture_output=True, text=True, timeout=5,
        )
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []
    zombies = []
    for line in result.stdout.strip().splitlines():
        parts = line.split()
        if len(parts) >= 2 and "Z" in parts[1]:
            zombies.append(line.strip())
    return zombies


def test_no_zombies_after_cgi():
    conf = write_config()
    proc = start_server(conf)
    try:
        server_pid = proc.pid

        # Fire requests rapidly on a keep-alive connection to minimize
        # the gap between CGI exit and reaping.
        conn = http.client.HTTPConnection("127.0.0.1", 8096, timeout=5)
        for i in range(10):
            conn.request("GET", "/cgi_hello.py")
            resp = conn.getresponse()
            body = resp.read().decode()
            assert resp.status == 200, f"Request {i+1}: expected 200, got {resp.status}"
            assert "CGI OK" in body, f"Request {i+1}: unexpected body '{body}'"
        conn.close()

        # Give the server a moment to reap children
        time.sleep(1)

        zombies = get_zombies(server_pid)
        assert len(zombies) == 0, (
            f"Found {len(zombies)} zombie process(es) after 10 CGI requests:\n"
            + "\n".join(zombies)
        )
        print(f"PASS: 10 CGI requests, 0 zombies (server pid {server_pid})")

    finally:
        stop_server(proc)
        os.unlink(conf)


def test_no_zombies_after_slow_cgi():
    conf = write_config()
    proc = start_server(conf)
    try:
        server_pid = proc.pid

        # The slow CGI script writes its response, closes ALL fds (so the
        # server sees EOF immediately), then sleeps 3s before exiting.
        # The server calls stopCgi → waitpid(WNOHANG) which returns 0
        # (child still alive) → sets _cgiPid = -1 → nobody ever reaps it.
        status, _, body = request(8096, "/cgi_slow_exit.py")
        assert status == 200, f"Expected 200, got {status}"
        assert "SLOW CGI OK" in body, f"Unexpected body '{body}'"

        # Wait for the child to exit (3s sleep in the script) + margin
        time.sleep(5)

        # The child has exited.  If the server didn't reap it, it's a zombie.
        zombies = get_zombies(server_pid)
        assert len(zombies) == 0, (
            f"Found {len(zombies)} zombie process(es) after slow CGI exited:\n"
            + "\n".join(zombies)
        )
        print(f"PASS: slow CGI request, 0 zombies (server pid {server_pid})")

    finally:
        stop_server(proc)
        os.unlink(conf)


if __name__ == "__main__":
    run_tests([test_no_zombies_after_cgi, test_no_zombies_after_slow_cgi])
