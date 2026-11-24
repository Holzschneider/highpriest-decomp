"""PROTOCOL.md against the mock: framing, errors, every command, events."""

import pathlib  # noqa: E402
import sys  # noqa: E402
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))  # port/harness

import hashlib
import json
import os
import socket
import time
import unittest

from bfharness import paths
from bfharness.client import Client
from bfharness.control import ControlError
from bfharness.frames import png_size
from bfharness.mockdb import MockDb, seed

RUN = paths.build_dir() / "selftest" / "protocol"


class ProtocolTest(unittest.TestCase):
    def setUp(self):
        RUN.mkdir(parents=True, exist_ok=True)
        db = RUN / "mock-db.jsonl"
        if db.exists():
            db.unlink()
        seed(MockDb(db))
        self.client = Client(RUN, mock=True, env={"BF_MOCK_DB": str(db)}, timeout=60).start()
        self.ctl = self.client.connect(timeout=20)

    def tearDown(self):
        self.client.stop()

    def test_hello(self):
        info = self.ctl.info
        self.assertEqual(info["protocol"], 0)
        self.assertEqual(info["client"], paths.app_name())
        self.assertEqual(info["pid"], self.client.proc.pid)
        for c in ("hello", "key", "pointer", "capture", "state", "wait", "resize", "quit"):
            self.assertIn(c, info["commands"])

    def test_bad_lines(self):
        self.ctl.send_raw("this is not json")
        self.ctl.send_raw('{"id": 99}')
        deadline = time.time() + 5
        while time.time() < deadline and None not in self.ctl._replies:
            time.sleep(0.02)
        self.assertEqual(self.ctl._replies[None]["error"]["code"], "bad_request")
        while time.time() < deadline and 99 not in self.ctl._replies:
            time.sleep(0.02)
        self.assertEqual(self.ctl._replies[99]["error"]["code"], "bad_request")

    def assertCode(self, code, fn, *a, **kw):
        with self.assertRaises(ControlError) as cm:
            fn(*a, **kw)
        self.assertEqual(cm.exception.code, code)
        self.assertFalse(cm.exception.reply.get("ok"))
        self.assertIn("seq", cm.exception.reply)

    def test_errors(self):
        self.assertCode("unknown_command", self.ctl.request, "fly")
        self.assertCode("unsupported", self.ctl.request, "step", n=10)
        self.assertCode("bad_key", self.ctl.press, "hyper")
        self.assertCode("bad_text", self.ctl.type_text, "안녕")
        self.assertCode("bad_request", self.ctl.request, "pointer", action="click", x=1)
        self.assertCode("bad_request", self.ctl.wait, {"weather": "rain"})
        self.assertCode("bad_request", self.ctl.resize, scale=4)

    def test_typing_reaches_focused_field(self):
        self.ctl.wait_mode("login")
        self.ctl.type_text("bftest\xe9")      # Windows-1252 letter
        self.ctl.press("tab")
        self.ctl.type_text("pw")
        w = self.ctl.window("LOGINWND")
        texts = {c["id"]: c["text"] for c in w["controls"]}
        self.assertEqual(texts["id_edit"], "bftest\xe9")
        self.assertEqual(texts["pw_edit"], "**")
        self.ctl.press("backspace")
        w = self.ctl.window("LOGINWND")
        self.assertEqual({c["id"]: c["text"] for c in w["controls"]}["pw_edit"], "*")

    def test_pointer(self):
        r = self.ctl.click(100, 120)
        self.assertEqual((r["x"], r["y"]), (100, 120))
        r = self.ctl.pointer("down", button="right")
        self.assertEqual((r["x"], r["y"]), (100, 120))
        self.assertEqual(self.ctl.state("pointer")["pointer"]["buttons"], ["right"])
        self.ctl.pointer("up", 140, 120, "right")
        self.ctl.wheel(-2)
        p = self.ctl.state("pointer")["pointer"]
        self.assertEqual((p["x"], p["y"], p["buttons"]), (140, 120, []))
        self.assertIsNone(p["cell"])        # no map outside game mode

    def test_state_parts(self):
        st = self.ctl.state()
        for part in ("mode", "detail", "frame", "view", "pointer", "map", "self", "actors", "windows"):
            self.assertIn(part, st)
        self.assertIsNone(st["self"])
        self.assertEqual(st["actors"], [])
        only = self.ctl.state("mode")
        self.assertEqual(set(only) - {"id", "ok", "seq"}, {"mode"})

    def test_wait(self):
        r = self.ctl.wait({"frames": 3})
        self.assertEqual(r["met"], {"frames": 3})
        with self.assertRaises(ControlError) as cm:
            self.ctl.wait({"mode": "game"}, timeout=0.3)
        self.assertEqual(cm.exception.code, "timeout")
        self.assertEqual(cm.exception.reply["mode"], "login")

    def test_events_and_seq(self):
        self.ctl.wait_mode("login")
        before = self.ctl.press("escape")["seq"]
        self.ctl.type_text("bftest")
        self.ctl.press("tab")
        self.ctl.type_text("bftest")
        r = self.ctl.press("enter")
        # the client-side wait sees the mode event that follows the reply's seq
        w = self.ctl.wait({"event": "mode", "match": {"mode": "charselect"}}, timeout=10, after=r["seq"])
        self.assertEqual(w["event"]["mode"], "charselect")
        self.assertGreater(w["event"]["seq"], r["seq"])
        # and the harness saw the same events, in order, with increasing seq
        seqs = [e["seq"] for e in self.ctl.events]
        self.assertEqual(seqs, sorted(seqs))
        names = [(e["event"], e.get("name") or e.get("mode")) for e in self.ctl.events if e["seq"] > before]
        self.assertIn(("window", "LOGINWND"), names)
        self.assertIn(("window", "SELECTCHARWND"), names)
        self.assertEqual(self.ctl.wait_event("mode", lambda e: e["mode"] == "charselect")["previous"],
                         "connecting")

    def test_capture_and_resize(self):
        frames = RUN / "frames"
        frames.mkdir(exist_ok=True)
        p = frames / "a.png"
        r = self.ctl.capture(p)
        self.assertEqual(png_size(p), (r["width"], r["height"]))
        self.assertEqual(hashlib.sha256(p.read_bytes()).hexdigest(), r["sha256"])
        self.assertEqual(r["logical"], {"w": 1024, "h": 768})
        v = self.ctl.resize(width=1600, height=1000, scale=2)["view"]
        self.assertEqual((v["width"], v["height"], v["scale"]), (1600, 1000, 2))
        self.assertEqual(v["logical"], {"w": 800, "h": 500})
        w = self.ctl.window("LOGINWND")
        x, y, ww, hh = w["rect"]
        self.assertTrue(0 <= x and x + ww <= 800 and 0 <= y and y + hh <= 500, "window clamped into view")
        r = self.ctl.capture(p)
        self.assertEqual(png_size(p), (1600, 1000))
        self.assertEqual(r["scale"], 2)
        self.assertCode("io", self.ctl.capture, RUN / "missing-dir" / "x.png")
        self.assertCode("bad_request", self.ctl.request, "capture", path="relative.png")

    def test_busy(self):
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect(str(self.client.control_path))
        line = s.makefile().readline()
        s.close()
        self.assertEqual(json.loads(line), {"event": "error", "code": "busy"})
        self.assertEqual(self.ctl.state("mode")["mode"], "login")   # first connection unaffected

    def test_quit(self):
        self.ctl.quit()
        bye = self.ctl.wait_event("bye", timeout=10)
        self.assertEqual(bye["reason"], "quit")
        self.assertEqual(self.client.stop(), 0)
        self.assertFalse(os.path.exists(self.client.control_path))
        self.assertIn("exit", self.client.trace())


if __name__ == "__main__":
    unittest.main()
