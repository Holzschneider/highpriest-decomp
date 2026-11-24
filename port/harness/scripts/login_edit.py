"""M5.3 the login window's edit controls and "save".

Played: typing with a wrong letter and Backspace into the account field, Tab
to the password field, the password pasted with Cmd+V (masked), Tab around,
"save" checked, login to character selection. Then the client is started
again: the account is filled in from the settings (Account/Name) and the
password from the Keychain, and Enter alone logs in. At the end "save" is
unchecked again, which clears both, so the other scripts start from empty
fields.

Evidence: the `controls` of LOGINWND (text, focus), loginlog rows, the
settings file, `security find-generic-password` for the Keychain item. The
Keychain is often out of reach in an unattended, locked session
(errSecInteractionNotAllowed): the script then records that the password was
not restored, and passes on the rest.

    BF_CLIENT=<binary> port/harness/run.py login_edit
"""

import plistlib
import subprocess
from pathlib import Path

from bfharness import paths
from bfharness.play import through_server_list
from bfharness.sql import wait_rows

NAME = "login_edit"
MILESTONE = "M5"
DESCRIPTION = "edit controls of the login window: typing, Backspace, Tab, Cmd+V, masking, save"
MANIFEST = "port/harness/manifests/m5-login-edit.sha256"
NEEDS_SERVER = True


def settings_account():
    p = Path.home() / "Library" / "Application Support" / paths.app_name() / "Settings.plist"
    try:
        with open(p, "rb") as f:
            return plistlib.load(f).get("Account", {}).get("Name")
    except (OSError, plistlib.InvalidFileException):
        return None


def keychain_has(account):
    r = subprocess.run(["security", "find-generic-password", "-s", paths.app_name(), "-a", account],
                       capture_output=True, text=True)
    return r.returncode == 0, (r.stdout + r.stderr).strip().splitlines()[-1:] if r.returncode else ["found"]


def controls(ctl):
    w = ctl.window("LOGINWND")
    return {c["id"]: c for c in (w or {}).get("controls", [])}


def focused(ctl):
    return [k for k, c in controls(ctl).items() if c.get("focus")]


class Clipboard:
    """Sets the Mac clipboard for the paste and puts the previous text back."""

    def __init__(self, text):
        self.text = text

    def __enter__(self):
        self.saved = subprocess.run(["pbpaste"], capture_output=True).stdout
        subprocess.run(["pbcopy"], input=self.text.encode(), check=True)
        return self

    def __exit__(self, *exc):
        subprocess.run(["pbcopy"], input=self.saved)


def run(t):
    if t.mock:
        with t.step("real client only", "the mock does not model this screen's details") as s:
            s.note("%s plays the real client against the test server; nothing to check against the mock" % NAME)
        return
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")

    ctl = t.launch()
    with t.step("start", "the login window has the focus in the account field, both fields empty") as s:
        ctl.wait_mode("login", 60)
        ctl.wait_window("LOGINWND", timeout=30)
        c = controls(ctl)
        s.state("controls", {k: (v["text"], v["focus"]) for k, v in c.items()})
        s.equal(c["id_edit"]["text"], "", "account field empty (no saved account; a run before left one?)")
        s.equal(focused(ctl), ["id_edit"], "focus")

    with t.step("type and Backspace", "M5.3: characters typed, Backspace removes the last") as s:
        ctl.type_text(user[:-1] + "x")
        s.equal(controls(ctl)["id_edit"]["text"], user[:-1] + "x", "typed")
        ctl.press("backspace")
        ctl.type_text(user[-1])
        s.equal(controls(ctl)["id_edit"]["text"], user, "after Backspace")

    with t.step("Tab and Cmd+V", "M5.3: Tab moves between the fields; Cmd+V pastes; the password is masked") as s:
        ctl.press("tab")
        s.equal(focused(ctl), ["pw_edit"], "focus after Tab")
        with Clipboard(password):
            ctl.press("v", "cmd")
        c = controls(ctl)
        s.equal(c["pw_edit"]["text"], "*" * len(password), "pasted password, masked")
        ctl.press("tab")
        s.equal(focused(ctl), ["id_edit"], "focus after a second Tab")
        ctl.press("tab")
        s.equal(focused(ctl), ["pw_edit"], "focus after a third Tab")
        s.frame("typed")

    with t.step("save and log in", "M5.3: 'save' checked; the login reaches character selection") as s:
        ctl.click_control("LOGINWND", "save")
        s.equal(controls(ctl)["save"]["text"], "on", "save checked")
        mark = t.logs.mark()
        after = ctl.last_seq
        ctl.press("enter")
        through_server_list(ctl, after, 60)
        s.rows("loginlog for %s" % user, wait_rows(lambda: t.logs.logins(user, after=mark), timeout=10))
    t.stop_client()

    with t.step("saved", "M5.3: account name in the settings, password in the Keychain") as s:
        s.equal(settings_account(), user, "Account/Name in Settings.plist")
        found, out = keychain_has(user)
        s.note("Keychain item %s/%s: %s" % (paths.app_name(), user, out))
        keychain = found

    ctl = t.launch()
    with t.step("restored", "M5.3: the next start fills in the account (and the password from the Keychain)") as s:
        ctl.wait_mode("login", 60)
        ctl.wait_window("LOGINWND", timeout=30)
        c = controls(ctl)
        s.state("controls", {k: (v["text"], v["focus"]) for k, v in c.items()})
        s.equal(c["id_edit"]["text"], user, "account restored")
        s.equal(c["save"]["text"], "on", "save still checked")
        s.equal(focused(ctl), ["pw_edit"], "focus in the password field")
        if keychain:
            s.equal(c["pw_edit"]["text"], "*" * len(password), "password restored from the Keychain")
        else:
            s.note("UNTESTED: the Keychain item was not written (Keychain out of reach), "
                   "so the password cannot be restored; see the client's trace for the w32 line")
            s.equal(c["pw_edit"]["text"], "", "no password without the Keychain")
        s.frame("restored")
        if keychain:
            mark = t.logs.mark()
            after = ctl.last_seq
            ctl.press("enter")
            through_server_list(ctl, after, 60)
            s.rows("loginlog for %s with the saved password" % user,
                   wait_rows(lambda: t.logs.logins(user, after=mark), timeout=10))

    with t.step("unsave", "unchecking 'save' clears the account and the Keychain item") as s:
        if ctl.state("mode")["mode"] != "login":
            ctl.click_control("SELECTCHARWND", "cancel")
            ctl.wait_mode("login", 30)
            ctl.wait_window("LOGINWND", timeout=10)
        ctl.click_control("LOGINWND", "save")
        s.equal(controls(ctl)["save"]["text"], "off", "save unchecked")
    t.stop_client()
    with t.step("cleared", "nothing saved any more") as s:
        s.equal(settings_account() or "", "", "Account/Name in Settings.plist")
        found, out = keychain_has(user)
        s.check(not found, "no Keychain item %s/%s (%s)" % (paths.app_name(), user, out))
