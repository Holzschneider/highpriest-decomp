#!/usr/bin/env python3
"""Run a harness script, or the harness's own tests.

    port/harness/run.py --self-test                     unit tests against the mock (+ one server check)
    port/harness/run.py port/harness/scripts/login.py --mock
    BF_CLIENT=build/bin/bifrost port/harness/run.py login
    port/harness/run.py --server-status

Options for a script: --mock (the mock client and its fake logs instead of the
client and the server), --param key=value (read by the script with
t.param(key)), --client-arg ARG (repeatable, appended to the client's
arguments), --timeout SECONDS (kill the client after that long).
Exit status 0 when every step passed.
"""

import argparse
import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from bfharness import paths                    # noqa: E402
from bfharness.runner import Test, load_script  # noqa: E402


def self_test(verbosity):
    suite = unittest.defaultTestLoader.discover(str(paths.HARNESS_DIR / "tests"),
                                                top_level_dir=str(paths.HARNESS_DIR))
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)
    return 0 if result.wasSuccessful() else 1


def server_status():
    from bfharness.server import Server
    s = Server()
    print("compose project:", s.dir)
    print(json.dumps(s.status(), indent=2))
    print("online:", s.online())
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("script", nargs="?")
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--server-status", action="store_true")
    ap.add_argument("--mock", action="store_true")
    ap.add_argument("--param", action="append", default=[], metavar="KEY=VALUE")
    ap.add_argument("--client-arg", action="append", default=[], metavar="ARG")
    ap.add_argument("--timeout", type=float, default=600.0)
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args(argv)

    if a.self_test:
        return self_test(2 if a.verbose else 1)
    if a.server_status:
        return server_status()
    if not a.script:
        ap.error("name a script, or --self-test")
    params = {}
    for kv in a.param:
        k, _, v = kv.partition("=")
        params[k] = v
    t = Test(load_script(a.script), mock=a.mock, params=params, client_args=a.client_arg,
             client_timeout=a.timeout)
    r = t.execute()
    for s in r["steps"]:
        print("%-4s %s%s" % (s["status"].upper(), s["name"], (": " + s["error"]) if s["error"] else ""))
    print("%s: %s" % (r["script"], "PASS" if r["passed"] else "FAIL"))
    print("report:", r["report_md"])
    return 0 if r["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
