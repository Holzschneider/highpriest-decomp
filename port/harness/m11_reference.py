#!/usr/bin/env python3
"""Recapture the M11 reference frames: one command.

    BF_CLIENT=<client> python3 port/harness/m11_reference.py [--runs 2] [--check 1] [--out <dir>]

What it does, at the commit that is checked out (the client named by
BF_CLIENT must be built from it):

1. Moves an existing reference at <out> (default <game folder>/test/reference/m11)
   aside to <out>-<its commit>.
2. Runs port/harness/scripts/m11_frames.py --runs times (default 2), each
   replaying the recorded session (port/harness/fixtures/m11-session.pktlog;
   no server needed) with its frames under <out>/runs/run<n>/m11/.
3. Builds the reference from those runs with framecmp.py (masks from the
   pixels that differ among them; framecmp.py's docstring is the definition
   of "match") into <out>: the frames of run 1, a mask per frame, and
   reference.json naming the commit.
4. Writes the frames' checksums of run 1 to port/harness/manifests/m11-frames.sha256.
5. Runs the capture --check more times (default 1), under <out>/checks/check<n>/,
   each compared with the new reference by the script's last step; the
   command fails unless every check matches (the reproducibility proof).

After an M11 step the comparison alone is:

    BF_CLIENT=<client> python3 port/harness/run.py m11_frames
"""

import argparse
import datetime
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from bfharness import paths          # noqa: E402
from bfharness.frames import _display, png_size, sha256   # noqa: E402
import framecmp                      # noqa: E402

MANIFEST = paths.REPO / "port" / "harness" / "manifests" / "m11-frames.sha256"


def head():
    sha = subprocess.run(["git", "-C", str(paths.REPO), "rev-parse", "--short", "HEAD"],
                         capture_output=True, text=True, check=True).stdout.strip()
    dirty = subprocess.run(["git", "-C", str(paths.REPO), "status", "--porcelain", "--", "src", "port", ":!port/harness"],
                           capture_output=True, text=True).stdout.strip()
    return sha + ("-dirty" if dirty else "")


def capture(frames_root, params=()):
    """One run of the script; returns (exit status, report path)."""
    env = dict(os.environ, BF_FRAMES_DIR=str(frames_root))
    cmd = [sys.executable, str(HERE / "run.py"), "m11_frames"]
    for p in params:
        cmd += ["--param", p]
    print("$ BF_FRAMES_DIR=%s %s" % (frames_root, " ".join(cmd[1:])), flush=True)
    r = subprocess.run(cmd, env=env, capture_output=True, text=True)
    lines = (r.stdout + r.stderr).strip().splitlines()
    for line in lines[-6:]:
        print("  " + line)
    report = next((l.split("report:", 1)[1].strip() for l in lines if l.startswith("report:")), None)
    return r.returncode, report


def write_manifest(frames_dir, commit):
    lines = ["# M11 reference frames at %s: run 1 under %s/; regenerate with port/harness/m11_reference.py"
             % (commit, _display(frames_dir))]
    for p in sorted(Path(frames_dir).glob("*.png")):
        w, h = png_size(p)
        lines.append("%s  %dx%d  %s" % (sha256(p), w, h, p.name))
    MANIFEST.write_text("\n".join(lines) + "\n")


def main():
    ap = argparse.ArgumentParser(description="recapture the M11 reference frames")
    ap.add_argument("--out", default=str(paths.client_dir() / "test" / "reference" / "m11"))
    ap.add_argument("--runs", type=int, default=2)
    ap.add_argument("--check", type=int, default=1)
    a = ap.parse_args()
    if not os.environ.get("BF_CLIENT"):
        ap.error("BF_CLIENT must name the client built from this commit")
    out = Path(a.out)
    commit = head()
    print("reference for %s into %s" % (commit, out), flush=True)
    if out.exists():
        old = "unknown"
        try:
            old = json.loads((out / "reference.json").read_text()).get("commit") or old
        except (OSError, ValueError):
            pass
        aside = out.with_name("%s-%s-%s" % (out.name, old,
                                            datetime.datetime.now().strftime("%Y%m%d%H%M%S")))
        print("previous reference (%s) moved to %s" % (old, aside))
        shutil.move(str(out), str(aside))
    runs = []
    for n in range(1, a.runs + 1):
        root = out / "runs" / ("run%d" % n)
        code, _ = capture(root, ["compare=0"])
        if code != 0:
            print("FAIL: capture run %d failed" % n)
            return 1
        runs.append(root / "m11")
    framecmp.build_reference(runs, out, commit)
    write_manifest(runs[0], commit)
    ok = True
    for n in range(1, a.check + 1):
        code, report = capture(out / "checks" / ("check%d" % n), ["reference=%s" % out])
        if report:
            rep = json.loads(Path(report).with_suffix(".json").read_text())
            for s in rep["steps"]:
                if s["name"] == "compare with the reference":
                    for e in s["evidence"]:
                        if e["kind"] == "trace":
                            for line in e["lines"]:
                                print("  " + line)
        ok = ok and code == 0
    print("%s: reference at %s for %s%s" % ("PASS" if ok else "FAIL", out, commit,
                                          "" if a.check == 0 else ", %d check(s) %s" % (a.check, "match" if ok else "do not all match")))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
