"""M5 character-creation script (skeleton of the real test; passes against the mock).

Done when (implementation plan, M5): a second script creates a character.
Set-up: a fresh account, created in SQL (default) or registered by logging in
as <userid>_F (--param register=1), so the account has no characters.
Played: log in, open character creation on the empty slot, type a name,
confirm. Evidence: charlog 'make new char' with that name and a char row on
the account; the new name in the selection slot.
"""

from bfharness.accounts import registration_userid, unique_name
from bfharness.play import login
from bfharness.sql import wait_rows

NAME = "create_char"
MILESTONE = "M5"
DESCRIPTION = "creates a character on a fresh account"
MANIFEST = "port/harness/manifests/m5-create-char.sha256"
NEEDS_SERVER = True


def run(t):
    userid = t.param("user") or unique_name(tag="a")
    password = t.param("password", "bfh-test")
    charname = t.param("name") or unique_name(tag="c")
    register = t.param("register") == "1"

    with t.step("set-up: fresh account", "account without characters") as s:
        if register:
            typed = registration_userid(userid, "F")
            s.note("registers %s by logging in as %s" % (userid, typed))
        else:
            typed = userid
            aid = t.accounts.create_account(userid, password)
            s.note("created %s (account %d) in SQL" % (userid, aid))
            s.equal(len(t.logs.chars(aid)), 0, "characters on the new account")

    ctl = t.launch()
    login(t, ctl, typed, password)

    with t.step("open character creation", "M5: an empty slot opens MAKECHARWND") as s:
        mark = t.logs.mark()
        ctl.press("enter")                 # Enter on an empty slot opens creation
        ctl.wait_mode("makechar", 30)
        ctl.wait_window("MAKECHARWND")
        s.frame("makechar")

    with t.step("hair and stats", "M5: hair style and colour, stats change by their buttons") as s:
        def texts():
            w = ctl.window("MAKECHARWND")
            return {c["id"]: c["text"] for c in w.get("controls", [])}
        before = texts()
        s.state("MAKECHARWND controls", before)
        clicks = [("right", 2), ("up", 1), ("str", 1), ("agi", 2)]
        if all(c in before for c, _ in clicks):
            for control, n in clicks:
                for _ in range(n):
                    ctl.click_control("MAKECHARWND", control)
            after = texts()
            s.state("after", after)
            # STR takes from INT, AGI from LUK (UIMakeCharWnd::SendMsg)
            s.equal((after.get("str"), after.get("int")), ("6", "4"), "STR up, INT down")
            s.equal((after.get("agi"), after.get("luk")), ("7", "3"), "AGI up, LUK down")
            expect = {"hair": 3, "hair_color": 1, "str": 6, "int": 4, "agi": 7, "luk": 3}
        else:
            s.note("controls not exposed (mock): stats and hair left at their defaults")
            expect = {}
        s.frame("makechar-set")

    with t.step("create", "M5: the server makes the character") as s:
        if expect:
            ctl.click_control("MAKECHARWND", "name_edit")   # the buttons may have taken the focus
        ctl.type_text(charname)
        ctl.press("enter")
        ctl.wait_mode("charselect", 30)
        rows = wait_rows(lambda: t.logs.charlog(name=charname, msg="make new char", after=mark))
        s.rows("charlog 'make new char' %s" % charname, rows)
        for k, v in expect.items():
            s.equal(rows[-1].get(k), v, "charlog %s" % k)
        c = t.logs.char(charname)
        s.check(c is not None, "char row %s exists" % charname)
        acc = t.logs.account(userid)
        s.equal(c["account_id"], acc["account_id"], "account of the new character")
        w = ctl.window("SELECTCHARWND")
        slots = [x["text"] for x in w.get("controls", []) if x["id"].startswith("slot:")]
        s.state("slots", slots)
        if slots:
            s.check(charname in slots, "%s shown in a slot" % charname)
        s.frame("charselect-new")
