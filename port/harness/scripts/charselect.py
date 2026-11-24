"""M5 character selection: select a character and enter the zone.

Done when (implementation plan, M5): character selection shows the account's
characters, and selecting one enters the zone; the packet log of the session
matches the sequence bifrost-netprobe produces.

Played: log in as bftest, select the character's slot (a click on it, or
--param via=enter: Enter on the slot the selection starts on), then OK (a
click, or Enter). Evidence: the client sends CZ_ENTER (0x009b) on the zone
connection (packet log); charlog 'char select' for the character; the login
sequence of the packet log equals bifrost-netprobe's (bfharness/pktlog.py),
which is run after the client has quit. The map load that follows is M6's:
this script quits once CZ_ENTER is sent.

    BF_CLIENT=<binary> port/harness/run.py charselect [--param via=enter]
"""

from bfharness.play import CZ_ENTER, compare_with_netprobe, login, stop_after_zone_entry, wait_sent
from bfharness.sql import wait_rows

NAME = "charselect"
MILESTONE = "M5"
DESCRIPTION = "selects a character and enters the zone; the login's packets match bifrost-netprobe's"
MANIFEST = "port/harness/manifests/m5-charselect.sha256"
NEEDS_SERVER = True

def run(t):
    if t.mock:
        with t.step("real client only", "the mock does not model this screen's details") as s:
            s.note("%s plays the real client against the test server; nothing to check against the mock" % NAME)
        return
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    via = t.param("via", "click")
    slot = int(t.param("slot", "0"))

    acc = t.logs.account(user)
    chars = {c["char_num"]: c for c in t.logs.chars(acc["account_id"])} if acc else {}

    ctl = t.launch()
    login(t, ctl, user, password)

    with t.step("characters drawn", "M5: the slots show the account's characters") as s:
        w = ctl.window("SELECTCHARWND")
        s.check(w is not None, "SELECTCHARWND is open")
        slots = {c["id"]: c for c in w.get("controls", []) if c["id"].startswith("slot:")}
        s.state("slots", {k: v["text"] for k, v in slots.items()})
        for n, c in chars.items():
            if "slot:%d" % n in slots:
                s.equal(slots["slot:%d" % n]["text"], c["name"], "slot %d" % n)
        s.check(slot in chars, "a character in slot %d" % slot)
        charname = chars[slot]["name"]

    with t.step("select", "M5: %s selects the character" % ("Enter" if via == "enter" else "a click on the slot")) as s:
        if via == "click":
            ctl.click_control("SELECTCHARWND", "slot:%d" % slot)
        w = ctl.window("SELECTCHARWND")
        texts = {c["id"]: c for c in w.get("controls", [])}
        s.state("controls", {k: v["text"] for k, v in texts.items()})
        s.check(texts.get("slot:%d" % slot, {}).get("focus"), "slot %d is the selected one" % slot)
        s.equal(texts.get("name", {}).get("text"), charname, "name shown")
        s.check("ok" in texts, "OK button shown (not make)")
        s.frame("selected")

    with t.step("enter", "M5: OK sends CH_SELECT_CHAR, the client goes to the zone and sends CZ_ENTER") as s:
        mark = t.logs.mark()
        if via == "click":
            ctl.click_control("SELECTCHARWND", "ok")
        else:
            ctl.press("enter")
        sent = wait_sent(t, CZ_ENTER, "zone", 60)
        s.check(bool(sent), "CZ_ENTER sent on the zone connection")
        rows = wait_rows(lambda: t.logs.charlog(name=charname, msg="char select", after=mark), timeout=10)
        s.rows("charlog 'char select' %s" % charname, rows)
        s.state("mode", ctl.state("mode", "detail"))

    stop_after_zone_entry(t)
    with t.step("packets", "M5: the login's packets match bifrost-netprobe's") as s:
        compare_with_netprobe(t, s, t.run_dir / "client-packets.txt", user, password, charname)
