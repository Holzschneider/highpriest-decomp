"""M8.1h logout and character switch: the client's own ways out of the game.

Done when (checklist M8.1h): from the game the player returns to character
selection by the client's own menu (Escape opens the quit window; its
"Character select" button sends CZ_RESTART type 1 and the client goes back
on ZC_RESTART_ACK, as the 2008 client's UIQuitWnd 0x483B70), chooses the
account's other character and enters the world with it; returns to
selection again and cancels it to the login screen; logs in again and
enters; then leaves by the quit window's "Exit" (CZ_REQ_DISCONNECT, the
client quits on ZC_ACK_REQ_DISCONNECT). Each step is confirmed in SQL:
`charlog` 'char select' for each character entered, `char.online` of both
characters (only the one in the world is online; none after the exit), and
`loginlog` 'login ok' for both logins.

Set-up: the script's own account (bfhlogout) with two characters,
BfhLogoutA in slot 0 and BfhLogoutB in slot 1, made in SQL on first use
(`--param user= password=` for another account with two characters,
`--param char_a= char_b=` for their names), both at prontera 155,183.
"""

import subprocess

from bfharness.control import ControlError
from bfharness.play import enter_game, login, own_home
from bfharness.sql import wait_rows

NAME = "logout"
MILESTONE = "M8"
DESCRIPTION = "Escape menu: character select, the other character, back to login, log in again, exit; charlog, char.online, loginlog"
MANIFEST = "port/harness/manifests/m8-logout.sha256"
NEEDS_SERVER = True

CZ_RESTART = "0x00b2"           # HEADER_CZ_RESTART 178
CZ_REQ_DISCONNECT = "0x018a"    # HEADER_CZ_REQ_DISCONNECT 394


def packets(t, direction, header=None, name=None):
    """(time, hex) of the packet log lines with that header or name."""
    path = t.run_dir / "client-packets.txt"
    out = []
    if not path.exists():
        return out
    for line in path.read_text(errors="replace").splitlines():
        f = line.split()
        if len(f) >= 6 and f[1] == direction and (f[2].lower() == header or f[4] == name):
            out.append((int(f[0]), f[5]))
    return out


def online(t, *names):
    return {n: int(t.logs.char(n)["online"]) for n in names}


def quit_window(t, ctl, s, label):
    """Escape in game opens the quit window (UIWindowMgr::ProcessPushButton,
    VK_ESCAPE: WID_QUITWND for a living character)."""
    after = ctl.last_seq
    ctl.press("escape")
    ev = ctl.wait_event("window", lambda e: e.get("action") == "open" and e.get("name") == "QUITWND",
                        after=after, timeout=10)
    w = ctl.window("QUITWND")
    ids = [c["id"] for c in w.get("controls", [])]
    s.state("QUITWND controls", ids)
    s.check(all(i in ids for i in ("restart", "exit", "cancel")), "quit window: character select, exit, cancel")
    s.state("window event", ev)
    s.frame("quit-window-" + label)
    return w


def to_charselect(t, ctl, s, label):
    sent_before = len(packets(t, "S", CZ_RESTART))
    quit_window(t, ctl, s, label)
    after = ctl.last_seq
    ctl.click_control("QUITWND", "restart")
    ev = ctl.wait_event("mode", lambda e: e.get("mode") == "charselect", after=after, timeout=30)
    s.state("mode event", ev)
    ctl.wait_window("SELECTCHARWND", timeout=10)
    if not t.mock:
        restart = packets(t, "S", CZ_RESTART)[sent_before:]
        s.state("CZ_RESTART sent", restart)
        s.check(len(restart) == 1 and restart[0][1].lower().endswith("01"), "one CZ_RESTART type 1 sent")
        ack = packets(t, "R", name="ZC_RESTART_ACK")
        s.check(ack and ack[-1][1].lower().endswith("01"), "ZC_RESTART_ACK type 1 received")
    s.frame("charselect-" + label)


def enter_slot(t, ctl, slot, name):
    """Character selection: click the slot, then Enter (UISelectCharWnd)."""
    with t.step("enter the world as %s" % name, "M8.1h: the other character of the account enters") as s:
        mark = t.logs.mark()
        w = ctl.wait_window("SELECTCHARWND", timeout=10)
        slots = {c["id"]: c["text"] for c in ctl.window("SELECTCHARWND").get("controls", [])
                 if c["id"].startswith("slot:")}
        s.state("slots", slots)
        s.equal(slots.get("slot:%d" % slot), name, "slot %d holds %s" % (slot, name))
        ctl.click_control("SELECTCHARWND", "slot:%d" % slot)
        ctl.wait({"frames": 10}, timeout=10)
        ctl.press("enter")
        ctl.wait_mode("game", 120)
        ctl.wait_event("map", timeout=120)
        st = ctl.state("self")
        s.state("self", st["self"])
        s.equal(st["self"]["name"], name, "the character in the world")
        rows = wait_rows(lambda: t.logs.charlog(name=name, msg="char select", after=mark), timeout=10)
        s.rows("charlog 'char select' for %s" % name, rows)
        s.frame("game-" + name)


def run(t):
    user = t.param("user", "bfhlogout")
    password = t.param("password", "bfh-test")
    a = t.param("char_a", "BfhLogoutA")
    b = t.param("char_b", "BfhLogoutB")

    with t.step("set-up: account with two characters", "SQL, offline: slot 0 %s, slot 1 %s at prontera 155,183" % (a, b)) as s:
        aid = t.accounts.create_account(user, password)
        for slot, n in ((0, a), (1, b)):
            t.accounts.make_char(user, n, slot=slot)
            t.accounts.set_char(n, map="prontera", x=155, y=183)
        rows = t.logs.chars(aid)
        s.state("char rows", [{k: r[k] for k in ("char_id", "char_num", "name", "online")} for r in rows])
        s.check([r["name"] for r in rows][:2] == [a, b], "slots 0 and 1 hold %s and %s" % (a, b))
        mark0 = t.logs.mark()

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl, slot=0)

    with t.step("first character online", "M8.1h: char.online") as s:
        st = ctl.state("self")["self"]
        s.equal(st["name"], a, "the character in the world")
        s.equal(wait_rows(lambda: [1] if online(t, a, b) == {a: 1, b: 0} else [], timeout=10) and online(t, a, b),
                {a: 1, b: 0}, "char.online")

    with t.step("back to character selection", "M8.1h: Escape, Character select; CZ_RESTART 1, ZC_RESTART_ACK") as s:
        to_charselect(t, ctl, s, a)
        s.equal(wait_rows(lambda: [1] if online(t, a, b)[a] == 0 else [], timeout=15) and online(t, a, b),
                {a: 0, b: 0}, "char.online after leaving the map server")

    enter_slot(t, ctl, 1, b)

    with t.step("second character online", "M8.1h: char.online") as s:
        s.equal(wait_rows(lambda: [1] if online(t, a, b) == {a: 0, b: 1} else [], timeout=10) and online(t, a, b),
                {a: 0, b: 1}, "char.online")

    with t.step("back to the login screen", "M8.1h: Character select, then Cancel on the selection") as s:
        to_charselect(t, ctl, s, b)
        after = ctl.last_seq
        ctl.click_control("SELECTCHARWND", "cancel")
        ev = ctl.wait_event("mode", lambda e: e.get("mode") == "login", after=after, timeout=30)
        s.state("mode event", ev)
        ctl.wait_window("LOGINWND", timeout=10)
        s.frame("login-again")
        s.equal(wait_rows(lambda: [1] if online(t, a, b)[b] == 0 else [], timeout=15) and online(t, a, b),
                {a: 0, b: 0}, "char.online")

    login(t, ctl, user, password)
    enter_slot(t, ctl, 0, a)

    with t.step("exit by the quit window", "M8.1h: Escape, Exit; CZ_REQ_DISCONNECT, the client quits") as s:
        quit_window(t, ctl, s, "exit")
        try:
            ctl.click_control("QUITWND", "exit")
        except ControlError as e:
            s.note("the control connection closed with the click: %s" % e)
        try:
            code = t.client.proc.wait(timeout=30)
        except subprocess.TimeoutExpired:
            code = None
        s.note("client exit status %r" % (code,))
        s.check(code is not None, "the client quit by itself within 30 s")

    client = t.client
    t.stop_client()
    with t.step("evidence after the exit", "M8.1h: charlog, char.online, loginlog") as s:
        s.equal(client.exit_code, 0, "exit status")
        s.check(not client.killed, "not killed by the harness")
        if not t.mock:
            s.check(len(packets(t, "S", CZ_REQ_DISCONNECT)) == 1, "one CZ_REQ_DISCONNECT sent")
            s.check(packets(t, "R", name="ZC_ACK_REQ_DISCONNECT"), "ZC_ACK_REQ_DISCONNECT received")
        s.equal(wait_rows(lambda: [1] if online(t, a, b) == {a: 0, b: 0} else [], timeout=15) and online(t, a, b),
                {a: 0, b: 0}, "char.online after the exit")
        sel = t.logs.charlog(account_id=aid, msg="char select", after=mark0)
        s.rows("charlog 'char select' of the account", sel, min_rows=3)
        s.equal([r["name"] for r in sel], [a, b, a], "characters entered, in order")
        s.rows("loginlog 'login ok'", t.logs.logins(user, after=mark0), min_rows=2)
        s.trace("assertion and crash lines", client.trace_lines("assert", "crash:"))
        s.check(not client.trace_lines("assert", "crash:"), "no assertion or crash line")
