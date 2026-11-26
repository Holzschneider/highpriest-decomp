"""F11: Cmd+Plus, Cmd+Minus and Cmd+0 change the UI scale.

    BF_CLIENT=<client> python3 port/harness/run.py scale_keys

On the login screen, no server needed. Cmd+Plus steps the UI scale in
effect up and Cmd+Minus down, within 1..3; Cmd+0 sets it back to auto (the
view's rule: the screen's backing scale). Each change is kept as the
settings window keeps it: Display/UIScale in Settings.plist, absent for
auto. The keys from the control socket carry no key code, so they are taken
by position (`equal`, `minus`, `0` of a US keyboard); on a real keyboard the
keys that type "+" ("=" unshifted on a US layout), "-" and "0" count.

Evidence: the `view` state (`scale` in effect, `scale_setting`, logical size) and the
settings file of the run's own home.
"""

import plistlib

from bfharness import paths
from bfharness.play import own_home

NAME = "scale_keys"
MILESTONE = "F11"
DESCRIPTION = "Cmd+Plus, Cmd+Minus, Cmd+0 step the UI scale and go back to auto; kept in the settings"
NEEDS_SERVER = False


def view(ctl):
    return ctl.state("view")["view"]


def saved(home):
    """Display/UIScale of the run's Settings.plist, or None."""
    plist = home / "Library" / "Application Support" / paths.app_name() / "Settings.plist"
    try:
        with open(plist, "rb") as f:
            values = plistlib.load(f)
    except (OSError, plistlib.InvalidFileException):
        return None
    group = next((v for k, v in values.items() if k.lower() == "display" and isinstance(v, dict)), {})
    return next((v for k, v in group.items() if k.lower() == "uiscale"), None)


def press(ctl, key):
    ctl.press(key, "cmd")
    ctl.wait({"frames": 3}, 20)
    return view(ctl)


def run(t):
    env = own_home(t)
    ctl = t.launch(env=env)
    home = t.run_dir / "home"
    with t.step("start", "the login screen at the auto scale") as s:
        ctl.wait_mode("login", 60)
        ctl.wait_window("LOGINWND", timeout=30)
        v = view(ctl)
        s.state("view", v)
        if t.mock:
            s.note("the mock does not model the view's scale")
            return
        s.equal(v["scale_setting"], 0, "setting auto")
        auto = v["scale"]

    with t.step("Cmd+Minus", "steps down to 1, not below") as s:
        for want in range(auto - 1, 0, -1):
            v = press(ctl, "minus")
            s.equal((v["scale_setting"], v["scale"]), (want, want), "Cmd+Minus")
        v = press(ctl, "minus")
        s.state("view", v)
        s.equal((v["scale_setting"], v["scale"]), (1, 1), "Cmd+Minus at 1")
        s.equal(saved(home), 1, "Display/UIScale")

    with t.step("Cmd+Plus", "steps up to 3, not beyond") as s:
        for want in (2, 3):
            v = press(ctl, "equal")
            s.state("view %d" % want, v)
            s.equal((v["scale_setting"], v["scale"]), (want, want), "Cmd+Plus")
        v = press(ctl, "equal")
        s.equal(v["scale"], 3, "Cmd+Plus at 3")
        s.equal(saved(home), 3, "Display/UIScale")
        v = press(ctl, "kpminus")
        s.equal(v["scale"], 2, "Cmd+keypad Minus")

    with t.step("Cmd+0", "back to auto; the setting removed") as s:
        v = press(ctl, "0")
        s.state("view", v)
        s.equal((v["scale_setting"], v["scale"]), (0, auto), "Cmd+0")
        s.equal(saved(home), None, "Display/UIScale absent")
        ctl.wait_window("LOGINWND", timeout=10)
