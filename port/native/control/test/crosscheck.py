#!/usr/bin/env python3
"""The harness against the native control module.

Runs the harness's own protocol test (port/harness/tests/test_protocol.py,
written against the mock) with the client replaced by bifrost-control-echo,
a headless stand-in that serves the protocol through the real module, plus a
few checks of this module's own. The harness's control client (bfharness)
does the talking, so both sides of PROTOCOL.md meet here.

    python3 port/native/control/test/crosscheck.py [build/native/control/bifrost-control-echo]
"""

import hashlib
import pathlib
import sys
import unittest

REPO = pathlib.Path(__file__).resolve().parents[4]
sys.path.insert(0, str(REPO / "port" / "harness"))

from bfharness import client as client_mod  # noqa: E402
from bfharness import paths  # noqa: E402
from bfharness.control import ControlError  # noqa: E402
from bfharness.frames import png_size  # noqa: E402
import tests.test_protocol as test_protocol  # noqa: E402

ECHO = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else REPO / "build" / "native" / "control" / "bifrost-control-echo").resolve()


class EchoClient(client_mod.Client):
    """The harness's launcher, starting the echo binary instead of the mock."""

    def __init__(self, run_dir, mock=False, **kw):
        kw.pop("command", None)
        client_mod.Client.__init__(self, run_dir, command=[str(ECHO)], **kw)


test_protocol.Client = EchoClient
RUN = paths.build_dir() / "selftest" / "control-echo"


class NativeExtra(unittest.TestCase):
    """What the protocol test does not touch: drag, modifiers, waits on events."""

    def setUp(self):
        RUN.mkdir(parents=True, exist_ok=True)
        self.client = EchoClient(RUN, timeout=60).start()
        self.ctl = self.client.connect(timeout=20)

    def tearDown(self):
        self.client.stop()

    def test_drag_and_buttons(self):
        self.ctl.drag(100, 100, 180, 140, button="right", steps=4)
        p = self.ctl.state("pointer")["pointer"]
        self.assertEqual((p["x"], p["y"], p["buttons"]), (180, 140, []))
        self.ctl.pointer("down", 10, 10, "middle")
        self.assertEqual(self.ctl.state("pointer")["pointer"]["buttons"], ["middle"])
        self.ctl.pointer("up", button="middle")

    def test_modifiers_and_cmd_q(self):
        self.ctl.wait_mode("login")
        self.ctl.type_text("x")
        self.ctl.press("tab", "shift")      # Shift+Tab moves the focus like Tab in the stand-in
        self.ctl.type_text("y")
        texts = {c["id"]: c["text"] for c in self.ctl.window("LOGINWND")["controls"]}
        self.assertEqual((texts["id_edit"], texts["pw_edit"]), ("x", "*"))
        # Through the app queue, as AppKit's Cmd+Q. The client quits on the
        # key-down, before the rest of the press is taken: no reply, then bye.
        with self.assertRaises(ControlError) as cm:
            self.ctl.press("q", "cmd")
        self.assertEqual(cm.exception.code, "closed")
        bye = self.ctl.wait_event("bye", timeout=10)
        self.assertEqual(bye["reason"], "quit")
        self.assertEqual(self.client.stop(), 0)

    def test_wait_on_window_event_and_capture(self):
        self.ctl.wait_mode("login")
        r = self.ctl.press("enter")
        w = self.ctl.wait({"event": "window", "match": {"action": "open", "name": "SELECTCHARWND"}},
                          timeout=10, after=r["seq"])
        self.assertEqual(w["event"]["wid"], 5)
        self.ctl.wait_window("SELECTCHARWND")
        p = RUN / "charselect.png"
        c = self.ctl.capture(p)
        self.assertEqual(png_size(p), (1024, 768))
        self.assertEqual(hashlib.sha256(p.read_bytes()).hexdigest(), c["sha256"])
        self.assertGreater(c["frame"], 0)
        with self.assertRaises(ControlError) as cm:
            self.ctl.wait({"mode": "game"}, timeout=0.2)
        self.assertEqual(cm.exception.reply["mode"], "charselect")


def main():
    if not ECHO.exists():
        print("no %s: build port/native/control first" % ECHO, file=sys.stderr)
        return 2
    print("client under test: %s" % ECHO)
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTests(loader.loadTestsFromTestCase(test_protocol.ProtocolTest))
    suite.addTests(loader.loadTestsFromTestCase(NativeExtra))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
