"""F4 the equipment window's "show my equipment to others" check box.

Done when (checklist F4): the equipment window (Alt+Q) has the check box the
2008 client's UIEquipWnd::OnCreate (0x473840) makes for the own window
(80x12 at 10, h - 15, checkbox_1/checkbox_0.bmp, MSI_OPEN_EQUIPED_ITEM, the
state of g_session.m_bOpenEquipedItem); a click sends CZ_CONFIG (0x2d8, type
0 CONFIG_OPEN_EQUIPMENT_WINDOW, the new value; UIEquipWnd::SendMsg ID_CHECK
-> MM_OPEN_EQUIPEDITEM_CHANGE), the server answers ZC_CONFIG (0x2d9) and
stores `char.show_equip` (saved when the character leaves the map server);
on the next map entry ZC_CONFIG_NOTIFY (0x2da) sets the box again. Clicked
back, the value returns to 0. Then character selection and back into the map
with the window open (the crash of 41a50c8 stays fixed).

Set-up: a fresh group-99 account and Novice (play.fresh_character) at
prontera 155,183, show_equip 0 (rAthena's default).
"""

from bfharness.accounts import unique_name
from bfharness.play import enter_game, fresh_character, login, own_home
from bfharness.sql import wait_rows

NAME = "equip_open"
MILESTONE = "F4"
DESCRIPTION = "equipment window: the show-equipment check box, CZ_CONFIG, ZC_CONFIG(_NOTIFY), char.show_equip"
MANIFEST = "port/harness/manifests/f4-equip-open.sha256"
NEEDS_SERVER = True

START = ("prontera", 155, 183)
CZ_CONFIG = "0x02d8"            # HEADER_CZ_CONFIG 728
ZC_CONFIG = "0x02d9"            # HEADER_ZC_CONFIG 729
ZC_CONFIG_NOTIFY = "0x02da"     # HEADER_ZC_CONFIG_NOTIFY 730


def packets(t, direction, header):
    """(time, hex) of the packet log lines with that header."""
    path = t.run_dir / "client-packets.txt"
    out = []
    if not path.exists():
        return out
    for line in path.read_text(errors="replace").splitlines():
        f = line.split()
        if len(f) >= 6 and f[1] == direction and f[2].lower() == header:
            out.append((int(f[0]), f[5].lower()))
    return out


def box(ctl):
    """The open_equip control of EQUIPWND, or None."""
    w = ctl.window("EQUIPWND")
    for c in (w or {}).get("controls", []):
        if c["id"] == "open_equip":
            return c
    return None


def show_equip(t, name):
    return int(t.logs.char(name)["show_equip"])


def online(t, name):
    return int(t.logs.char(name)["online"])


def wait_box(ctl, text, timeout=10):
    """Polls until the check box reads `text`; returns the control."""
    rows = wait_rows(lambda: [c for c in [box(ctl)] if c and c["text"] == text], timeout=timeout)
    return rows[0] if rows else box(ctl)


def to_charselect(t, ctl, s, label):
    """Escape, Character select (UIQuitWnd), as logout.py does."""
    after = ctl.last_seq
    ctl.press("escape")
    ctl.wait_event("window", lambda e: e.get("action") == "open" and e.get("name") == "QUITWND",
                   after=after, timeout=10)
    after = ctl.last_seq
    ctl.click_control("QUITWND", "restart")
    ctl.wait_event("mode", lambda e: e.get("mode") == "charselect", after=after, timeout=30)
    ctl.wait_window("SELECTCHARWND", timeout=10)
    s.frame("charselect-" + label)


def reenter(t, ctl, s, name):
    ctl.press("enter")
    ctl.wait_mode("game", 120)
    ctl.wait_event("map", timeout=120)
    ctl.wait({"frames": 30}, timeout=20)
    s.check(t.client.proc.poll() is None, "the client still runs")
    s.equal(ctl.state("self")["self"]["name"], name, "the character in the world")
    s.check(bool(ctl.window("EQUIPWND")), "EQUIPWND reopened")


def click(t, ctl, s, name, want):
    """Click the check box; CZ_CONFIG out, ZC_CONFIG back, the box shows it."""
    sent0, got0 = len(packets(t, "S", CZ_CONFIG)), len(packets(t, "R", ZC_CONFIG))
    ctl.click_control("EQUIPWND", "open_equip")
    c = wait_box(ctl, "on" if want else "off")
    s.state("open_equip after the click", c)
    s.equal(c and c["text"], "on" if want else "off", "the check box shows the new state")
    ctl.wait({"frames": 10}, timeout=10)
    sent = packets(t, "S", CZ_CONFIG)[sent0:]
    got = packets(t, "R", ZC_CONFIG)[got0:]
    s.state("CZ_CONFIG sent", sent)
    s.state("ZC_CONFIG received", got)
    # d802 | type 00000000 | value 0000000x
    s.check(len(sent) == 1 and sent[0][1] == "d80200000000%02x000000" % want,
            "one CZ_CONFIG type 0 (open equipment window), value %d" % want)
    s.check(len(got) >= 1 and got[-1][1] == "d90200000000%02x000000" % want,
            "ZC_CONFIG type 0, value %d" % want)


def run(t):
    if t.mock:
        with t.step("the check box", "real client only") as s:
            s.note("mock: the mock client has no equipment window check box")
        return
    user, name = unique_name(tag="q"), unique_name(tag="Q")
    fresh_character(t, user, "bfh-test", name, place=START)
    with t.step("set-up: show_equip", "rAthena's default for a new character") as s:
        s.equal(show_equip(t, name), 0, "char.show_equip")

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, "bfh-test")
    enter_game(t, ctl)

    with t.step("the equipment window and its check box", "F4: UIEquipWnd::OnCreate makes it (0x473840)") as s:
        ctl.press("q", "alt")
        ctl.wait({"frames": 6}, timeout=10)
        w = ctl.window("EQUIPWND")
        s.check(bool(w), "EQUIPWND open")
        s.state("EQUIPWND rect", w and w["rect"])
        c = box(ctl)
        s.state("open_equip", c)
        s.check(c is not None and c["kind"] == "check", "open_equip is a check control")
        if w and c:
            (wx, wy, ww, wh), (cx, cy, cw, ch) = w["rect"], c["rect"]
            s.equal((cx - wx, cy - wy, ch), (10, wh - 15, 12), "at 10, h - 15, 12 high as the reference")
            s.check(80 <= cw <= ww // 2 - 28, "80 wide as the reference, or the English label's width "
                    "short of the cart button (%d)" % cw)
        s.equal(c and c["text"], "off", "off, as the server's ZC_CONFIG_NOTIFY said on entry")
        s.check(packets(t, "R", ZC_CONFIG_NOTIFY), "ZC_CONFIG_NOTIFY received on entry")
        s.frame("equip-off")

    with t.step("click: show the equipment", "F4: CZ_CONFIG 0x2d8 type 0 value 1, ZC_CONFIG back") as s:
        click(t, ctl, s, name, 1)
        s.frame("equip-on")

    with t.step("character selection: the server saves show_equip 1", "F4: char.show_equip in SQL") as s:
        to_charselect(t, ctl, s, "on")
        rows = wait_rows(lambda: [1] if online(t, name) == 0 and show_equip(t, name) == 1 else [], timeout=15)
        s.equal(show_equip(t, name), 1, "char.show_equip after leaving the map server")
        s.check(bool(rows), "saved")

    with t.step("the map again: ZC_CONFIG_NOTIFY 1 sets the box", "F4: 0x2da on map entry; the window reopened") as s:
        n0 = len(packets(t, "R", ZC_CONFIG_NOTIFY))
        reenter(t, ctl, s, name)
        notes = packets(t, "R", ZC_CONFIG_NOTIFY)[n0:]
        s.state("ZC_CONFIG_NOTIFY", notes)
        s.check(notes and notes[-1][1] == "da0201", "ZC_CONFIG_NOTIFY 1")
        c = box(ctl)
        s.state("open_equip", c)
        s.equal(c and c["text"], "on", "the check box shows on")
        s.frame("equip-on-again")

    with t.step("click back: hide the equipment", "F4: CZ_CONFIG type 0 value 0, ZC_CONFIG back") as s:
        click(t, ctl, s, name, 0)

    with t.step("character selection: show_equip 0", "F4: char.show_equip in SQL") as s:
        to_charselect(t, ctl, s, "off")
        wait_rows(lambda: [1] if online(t, name) == 0 and show_equip(t, name) == 0 else [], timeout=15)
        s.equal(show_equip(t, name), 0, "char.show_equip after leaving the map server")

    with t.step("the map again with the window open", "41a50c8: ZC_CONFIG_NOTIFY reaches the reopened window") as s:
        reenter(t, ctl, s, name)
        c = box(ctl)
        s.state("open_equip", c)
        s.equal(c and c["text"], "off", "the check box shows off")
        s.frame("equip-off-again")

    client = t.client
    t.stop_client()
    with t.step("no assertion or crash", "trace") as s:
        s.trace("assertion and crash lines", client.trace_lines("assert", "crash:"))
        s.check(not client.trace_lines("assert", "crash:"), "no assertion or crash line")
