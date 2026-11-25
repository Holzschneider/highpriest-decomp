"""The example scripts pass against the mock, and the runner writes reports."""

import pathlib  # noqa: E402
import sys  # noqa: E402
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))  # port/harness

import json
import subprocess
import unittest
from concurrent.futures import ThreadPoolExecutor

from bfharness import paths
from bfharness.runner import Test, load_script


class ScriptsAgainstMock(unittest.TestCase):
    def run_script(self, name, title=None):
        t = Test(load_script(paths.HARNESS_DIR / "scripts" / (name + ".py")), mock=True,
                 run_dir=paths.build_dir() / "selftest" / ("script-" + name), client_timeout=120)
        r = t.execute()
        failed = [(s["name"], s["error"]) for s in r["steps"] if s["status"] != "pass"]
        self.assertTrue(r["passed"], failed)
        rep = json.loads((t.run_dir / "report.json").read_text())
        self.assertTrue(rep["passed"])
        self.assertTrue((t.run_dir / "report.md").read_text().startswith("# %s: PASS" % (title or name)))
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

    def test_npc(self):
        rep = self.run_script("npc")
        self.assertIn("Merchant: next, menus, number input, close", [s["name"] for s in rep["steps"]])

    def test_shop(self):
        self.run_script("shop")

    def test_storage(self):
        self.run_script("storage")

    def test_sound(self):
        rep = self.run_script("sound")
        names = [s["name"] for s in rep["steps"]]
        self.assertIn("login music loops", names)
        self.assertIn("distance-scaled volume", names)

    def test_intro(self):
        rep = self.run_script("intro")
        names = [s["name"] for s in rep["steps"]]
        self.assertIn("A: the movie ends and the login screen follows", names)
        self.assertIn("E: Esc stops the movie and the login screen follows", names)

    def test_english_session(self):
        rep = self.run_script("english_session")
        self.assertIn("coverage report", [s["name"] for s in rep["steps"]])

    def test_m11_frames(self):
        rep = self.run_script("m11_frames", title="m11")
        names = {f["name"] for f in rep["frames"]}
        self.assertTrue({"login-s2", "login-s1", "charselect", "makechar", "prontera", "chat-latin1",
                         "inventory", "world-wide", "login-s2.s1", "login-s2.s2"} <= names)

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


class EveryScriptAgainstMock(unittest.TestCase):
    """Every script in scripts/ passes with --mock (scripts that need the real
    client say so in a step and pass), so that a change to the mock or to a
    script cannot break another script unnoticed. Run in parallel: each
    script has its own run directory and mock client."""

    def run_one(self, name):
        r = subprocess.run([sys.executable, str(paths.HARNESS_DIR / "run.py"), name, "--mock"],
                           capture_output=True, text=True, timeout=300)
        return name, r.returncode, r.stdout[-1500:]

    def test_every_script(self):
        names = sorted(p.stem for p in (paths.HARNESS_DIR / "scripts").glob("*.py") if p.stem != "__init__")
        self.assertGreater(len(names), 20)
        with ThreadPoolExecutor(max_workers=6) as pool:
            results = list(pool.map(self.run_one, names))
        failed = ["%s:\n%s" % (n, out) for n, rc, out in results if rc != 0]
        self.assertFalse(failed, "scripts failing with --mock:\n" + "\n".join(failed))


if __name__ == "__main__":
    unittest.main()
