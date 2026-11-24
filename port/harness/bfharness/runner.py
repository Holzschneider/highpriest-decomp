"""Runs a test script and writes its report.

A script is a Python module with

    NAME = "login"                  # frames go to <frames root>/<NAME>/
    MILESTONE = "M5"
    DESCRIPTION = "..."
    MANIFEST = "port/harness/manifests/m5-login.sha256"   # checksum list in git (real runs)
    NEEDS_SERVER = True             # real runs: the test server must be online
    def run(t): ...                 # t is a Test

and does its work in steps:

    with t.step("log in", "M5: reaches character selection") as s:
        ...
        s.rows("loginlog: login ok", t.logs.logins(user, after=mark))

A step passes when its block finishes; an exception or a failed check fails
it, and the script stops there (later steps depend on earlier ones). Every
step carries its evidence: SQL rows, frames, trace lines, state excerpts.
After the script, the client is stopped and its exit code is a step of its
own. The report goes to build/harness/<NAME>-<time>/report.{json,md} and is
copied to build/harness/<NAME>[-mock]-latest.{json,md}.
"""

import datetime
import importlib.util
import json
import shutil
import time
import traceback
from pathlib import Path

from . import paths
from .accounts import Accounts, GM
from .client import Client
from .frames import Frames
from .sql import Logs, MockSql, Sql
from .mockdb import MockDb, seed

MAX_ROWS = 20


class StepFailed(AssertionError):
    pass


class Step:
    def __init__(self, test, name, criterion):
        self.test = test
        self.name = name
        self.criterion = criterion
        self.evidence = []
        self.status = "running"
        self.error = None
        self.t0 = time.monotonic()
        self.seconds = None

    # evidence

    def note(self, text):
        self.evidence.append({"kind": "note", "text": str(text)})

    def rows(self, label, rows, min_rows=1):
        """Record SQL rows; fail when there are fewer than min_rows."""
        rows = list(rows or [])
        self.evidence.append({"kind": "sql", "label": label, "count": len(rows),
                              "rows": rows[:MAX_ROWS]})
        if len(rows) < min_rows:
            raise StepFailed("%s: %d rows, expected at least %d" % (label, len(rows), min_rows))
        return rows

    def frame(self, name):
        rec = self.test.frames.capture(self.test.ctl, name)
        self.evidence.append(dict(rec, kind="frame"))
        return rec

    def state(self, label, data):
        self.evidence.append({"kind": "state", "label": label, "data": data})

    def trace(self, label, lines, max_lines=30):
        lines = list(lines)
        self.evidence.append({"kind": "trace", "label": label, "count": len(lines),
                              "lines": lines[-max_lines:]})

    def check(self, cond, message, **data):
        if data:
            self.evidence.append({"kind": "check", "text": message, "ok": bool(cond), "data": data})
        if not cond:
            raise StepFailed(message)

    def equal(self, actual, expected, message):
        self.check(actual == expected, "%s: got %r, expected %r" % (message, actual, expected),
                   actual=actual, expected=expected)

    # context manager

    def __enter__(self):
        return self

    def __exit__(self, kind, value, tb):
        self.seconds = round(time.monotonic() - self.t0, 3)
        if kind is None:
            self.status = "pass"
            return False
        self.status = "fail"
        self.error = "%s: %s" % (kind.__name__, value)
        if not isinstance(value, StepFailed):
            self.evidence.append({"kind": "exception",
                                  "text": "".join(traceback.format_exception(kind, value, tb))[-4000:]})
        raise _Stop()


class _Stop(Exception):
    pass


class Test:
    """What a script sees: server, logs, accounts, client, control, frames."""

    def __init__(self, module, mock=False, params=None, run_dir=None, client_args=(),
                 client_timeout=600.0):
        self.module = module
        self.name = getattr(module, "NAME", Path(module.__file__).stem)
        self.mock = mock
        self.params = dict(params or {})
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d-%H%M%S")
        self.run_dir = Path(run_dir) if run_dir else \
            paths.build_dir() / ("%s%s-%s" % (self.name, "-mock" if mock else "", stamp))
        self.run_dir.mkdir(parents=True, exist_ok=True)
        self.client_args = list(client_args)
        self.client_timeout = client_timeout
        self.steps = []
        self.client = None
        self.ctl = None
        self.started = datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")
        if mock:
            self.server = None
            self.mockdb = MockDb(self.run_dir / "mock-db.jsonl")
            seed(self.mockdb)
            self.db = MockSql(self.mockdb)
            self.frames = Frames(self.name, root=paths.build_dir() / "frames-mock", allow_in_repo=True)
        else:
            from .server import Server
            self.server = Server()
            self.mockdb = None
            self.db = Sql(self.server)
            self.frames = Frames(self.name)
        self.logs = Logs(self.db)
        self.accounts = Accounts(self.db, self.logs)
        self.gm = GM()

    def param(self, key, default=None):
        return self.params.get(key, default)

    def step(self, name, criterion=None):
        s = Step(self, name, criterion)
        self.steps.append(s)
        return s

    def launch(self, args=(), env=None):
        """Start the client (the mock in mock mode) and connect to its socket."""
        extra = dict(env or {})
        if self.mock:
            extra["BF_MOCK_DB"] = str(self.mockdb.path)
        self.client = Client(self.run_dir, args=list(args) + self.client_args, env=extra,
                             mock=self.mock, timeout=self.client_timeout)
        self.client.start()
        self.ctl = self.client.connect()
        self.gm = GM(self.ctl)
        return self.ctl

    def stop_client(self):
        if not self.client:
            return None
        code = self.client.stop()
        c = self.client
        s = Step(self, "client exits cleanly", "exit status 0, no failed assertion in the trace")
        self.steps.append(s)
        s.evidence.append({"kind": "note", "text": "exit status %s%s" % (code, " (killed)" if c.killed else "")})
        bad = c.trace_lines("assert", "ASSERT")
        s.trace("assertions in trace", bad)
        s.seconds = 0
        s.status = "pass" if code == 0 and not bad else "fail"
        if s.status == "fail":
            s.error = "exit status %s, %d assertion lines" % (code, len(bad))
        self.client = None
        return code

    # -- running -------------------------------------------------------------

    def execute(self):
        if not self.mock and getattr(self.module, "NEEDS_SERVER", True):
            with_step = self.step("test server online", "rAthena from port/server is up")
            try:
                with with_step as s:
                    s.note("server %s: %s" % (self.server.dir, self.server.ensure_up()))
            except _Stop:
                return self.finish()
        try:
            self.module.run(self)
        except _Stop:
            pass
        except Exception as e:  # outside any step
            s = Step(self, "script", None)
            s.status, s.error = "fail", "%s: %s" % (type(e).__name__, e)
            s.evidence.append({"kind": "exception", "text": traceback.format_exc()[-4000:]})
            self.steps.append(s)
        finally:
            if self.client:
                self.stop_client()
        return self.finish()

    def finish(self):
        manifest = None
        if self.frames.records:
            target = self.run_dir / "frames.sha256" if self.mock else getattr(self.module, "MANIFEST", None)
            if target:
                manifest = str(self.frames.write_manifest(target))
        report = {
            "script": self.name,
            "file": _rel(self.module.__file__),
            "milestone": getattr(self.module, "MILESTONE", None),
            "description": getattr(self.module, "DESCRIPTION", ""),
            "mock": self.mock,
            "started": self.started,
            "run_dir": _rel(self.run_dir),
            "passed": bool(self.steps) and all(s.status == "pass" for s in self.steps),
            "steps": [{"name": s.name, "criterion": s.criterion, "status": s.status,
                       "error": s.error, "seconds": s.seconds, "evidence": s.evidence}
                      for s in self.steps],
            "frames": self.frames.records,
            "manifest": _rel(manifest) if manifest else None,
            "sql_statements": len(getattr(self.db, "log", [])),
        }
        jpath = self.run_dir / "report.json"
        jpath.write_text(json.dumps(report, indent=2, default=str) + "\n")
        mpath = self.run_dir / "report.md"
        mpath.write_text(markdown(report))
        latest = paths.build_dir() / ("%s%s-latest" % (self.name, "-mock" if self.mock else ""))
        shutil.copyfile(jpath, str(latest) + ".json")
        shutil.copyfile(mpath, str(latest) + ".md")
        report["report_json"] = str(jpath)
        report["report_md"] = str(mpath)
        return report


def _rel(p):
    try:
        return str(Path(p).resolve().relative_to(paths.REPO.resolve()))
    except ValueError:
        return str(p)


def markdown(r):
    out = ["# %s: %s" % (r["script"], "PASS" if r["passed"] else "FAIL"), ""]
    out.append("%s%s. Started %s%s." % (
        r["milestone"] + ", " if r["milestone"] else "", r["description"] or r["file"],
        r["started"], ", against the mock client" if r["mock"] else ""))
    out.append("")
    out += ["| Step | Criterion | Result | Seconds |", "|---|---|---|---:|"]
    for s in r["steps"]:
        out.append("| %s | %s | %s | %s |" % (s["name"], s["criterion"] or "",
                                              s["status"] + (": " + s["error"] if s["error"] else ""),
                                              s["seconds"] if s["seconds"] is not None else ""))
    out.append("")
    for s in r["steps"]:
        if not s["evidence"]:
            continue
        out += ["## %s" % s["name"], ""]
        for e in s["evidence"]:
            k = e["kind"]
            if k == "note":
                out.append("- %s" % e["text"])
            elif k == "sql":
                out.append("- SQL, %s: %d row(s)" % (e["label"], e["count"]))
                for row in e["rows"][:5]:
                    out.append("  - `%s`" % json.dumps(row, default=str))
            elif k == "frame":
                out.append("- frame `%s.png` %dx%d sha256 `%s`" % (e["name"], e["width"], e["height"],
                                                                 e["sha256"][:16]))
            elif k == "state":
                out.append("- state, %s: `%s`" % (e["label"], json.dumps(e["data"], default=str)[:300]))
            elif k == "trace":
                out.append("- trace, %s: %d line(s)" % (e["label"], e["count"]))
                for line in e["lines"][-5:]:
                    out.append("  - `%s`" % line[:200])
            elif k == "check":
                out.append("- check %s: %s" % ("ok" if e["ok"] else "FAILED", e["text"]))
            elif k == "exception":
                out += ["", "```", e["text"].rstrip(), "```"]
        out.append("")
    if r["manifest"]:
        out.append("Frame checksums: `%s`." % r["manifest"])
    return "\n".join(out) + "\n"


def load_script(path):
    p = Path(path)
    if not p.exists():
        alt = paths.HARNESS_DIR / "scripts" / (p.name if p.suffix else p.name + ".py")
        if alt.exists():
            p = alt
        else:
            raise FileNotFoundError(path)
    spec = importlib.util.spec_from_file_location("bfscript_" + p.stem, str(p))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod
