"""M5 login script (skeleton of the real test; passes against the mock).

Done when (implementation plan, M5): the login script types an account and
password, clicks through and reaches character selection with characters
drawn. Evidence: loginlog 'login ok' after the marker, the account's
characters in SQL matching the slots the client shows, frames of both screens.

    port/harness/run.py port/harness/scripts/login.py --mock
    BF_CLIENT=<binary> port/harness/run.py login [--param user=bftest --param password=bftest]
"""

from bfharness.play import login

NAME = "login"
MILESTONE = "M5"
DESCRIPTION = "types account and password, reaches character selection with characters drawn"
MANIFEST = "port/harness/manifests/m5-login.sha256"
NEEDS_SERVER = True


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")

    with t.step("launch", "client opens its control socket and shows the login screen") as s:
        ctl = t.launch()
        s.state("hello", ctl.info)
        st = ctl.wait_mode("login", 60)
        s.state("mode", st)
        s.frame("login")

    login(t, ctl, user, password)

    with t.step("characters drawn", "M5: character selection shows the account's characters") as s:
        acc = t.logs.account(user)
        s.check(acc is not None, "account %s exists" % user)
        chars = t.logs.chars(acc["account_id"])
        s.rows("char rows of account %d" % acc["account_id"], chars, min_rows=0)
        w = ctl.window("SELECTCHARWND")
        s.check(w is not None, "SELECTCHARWND is open")
        slots = {c["id"]: c["text"] for c in w.get("controls", []) if c["id"].startswith("slot:")}
        s.state("slots", slots)
        for c in chars:
            s.equal(slots.get("slot:%d" % c["char_num"]), c["name"], "slot %d" % c["char_num"])
        s.frame("charselect")
