"""The example scripts pass against the mock, and the runner writes reports."""

import pathlib  # noqa: E402
import sys  # noqa: E402
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))  # port/harness

import json
import unittest

from bfharness import paths
from bfharness.runner import Test, load_script


class ScriptsAgainstMock(unittest.TestCase):
    def run_script(self, name):
        t = Test(load_script(paths.HARNESS_DIR / "scripts" / (name + ".py")), mock=True,
                 run_dir=paths.build_dir() / "selftest" / ("script-" + name), client_timeout=120)
        r = t.execute()
        failed = [(s["name"], s["error"]) for s in r["steps"] if s["status"] != "pass"]
        self.assertTrue(r["passed"], failed)
        rep = json.loads((t.run_dir / "report.json").read_text())
        self.assertTrue(rep["passed"])
        self.assertTrue((t.run_dir / "report.md").read_text().startswith("# %s: PASS" % name))
        self.assertTrue(rep["frames"], "frames captured")
        self.assertTrue((t.run_dir / "frames.sha256").exists(), "mock manifest stays in the run dir")
        return rep

    def test_login(self):
        rep = self.run_script("login")
        kinds = {e["kind"] for s in rep["steps"] for e in s["evidence"]}
        self.assertTrue({"sql", "frame", "state"} <= kinds)

    def test_create_char(self):
        self.run_script("create_char")

    def test_walk(self):
        self.run_script("walk")

    def test_gate(self):
        self.run_script("gate")

    def test_chat(self):
        self.run_script("chat")

    def test_sound(self):
        rep = self.run_script("sound")
        names = [s["name"] for s in rep["steps"]]
        self.assertIn("login music loops", names)
        self.assertIn("distance-scaled volume", names)

    def test_failure_is_reported(self):
        """A step that fails stops the script and the report says FAIL."""
        import types
        mod = types.ModuleType("bfscript_failing")
        mod.__file__ = str(paths.HARNESS_DIR / "scripts" / "failing.py")
        mod.NAME = "failing"

        def run(t):
            ctl = t.launch()
            with t.step("wrong mode expected") as s:
                s.equal(ctl.state("mode")["mode"], "game", "mode")
            with t.step("never reached"):
                pass
        mod.run = run
        t = Test(mod, mock=True, run_dir=paths.build_dir() / "selftest" / "script-failing")
        r = t.execute()
        self.assertFalse(r["passed"])
        self.assertEqual([s["name"] for s in r["steps"]], ["wrong mode expected", "client exits cleanly"])
        self.assertIn("got 'login', expected 'game'", r["steps"][0]["error"])


if __name__ == "__main__":
    unittest.main()
