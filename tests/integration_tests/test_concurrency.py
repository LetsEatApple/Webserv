#!/usr/bin/env python3
"""Integration test for concurrent connections.

- 5 workers send 5 GET requests each simultaneously, all expect 200
"""

import threading
from helpers import start_server, stop_server, request, conf_path, run_tests


def test_concurrent():
    proc = start_server(conf_path("method.conf"))
    try:
        num_workers = 5
        num_requests = 5
        results = []
        errors = []

        def worker(wid):
            for i in range(num_requests):
                try:
                    status, _, _ = request(8093, "/")
                    results.append(status == 200)
                except Exception as e:
                    errors.append(f"worker {wid} req {i}: {e}")

        threads = []
        for w in range(num_workers):
            t = threading.Thread(target=worker, args=(w,))
            threads.append(t)
            t.start()
        for t in threads:
            t.join()

        total = num_workers * num_requests
        ok = sum(1 for r in results if r)
        assert ok == total and not errors, \
            f"{ok}/{total} succeeded, errors: {errors}"
        print(f"PASS: {ok}/{total} concurrent requests succeeded")

    finally:
        stop_server(proc)


if __name__ == "__main__":
    run_tests([test_concurrent])
