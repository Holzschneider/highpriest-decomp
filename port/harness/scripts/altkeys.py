"""M8: the 2008 client's Alt shortcuts and the tip of the day.

Done when: every keyboard shortcut of UIWindowMgr::ProcessPushButton (the
reference's 0x4f58c0) that opens a window for a character without party,
guild, pet, homunculus or cart opens it (state `windows`), and a second press
closes it, as the 2008 client toggles them: Alt+A status, Alt+E items, Alt+Q
equipment, Alt+S skills, Alt+O options, Alt+C chat room, Alt+D detail level,
Alt+I friend options, Alt+L emotion list, Alt+M emotions, Alt+Y command list,
Alt+X quit, Ctrl+` world map; Alt+H opens the friend
and party window on its friend tab and Alt+Z on its party tab (Alt+Z while the
friend tab is open switches to the party tab); Alt+U, the quest window, is the
Korean service's only and opens nothing for the English one. And the tip of
the day (tipOfTheday_e.txt in the game folder, placed by port/data/assemble.py)
opens on the first entry into the world (the `window` event; against
rAthena the server's map move right after the entry removes it again, as it
would the 2008 client's), and /tip opens it with English text; its Next
button turns to another tip and its Close button closes it. A frame per
window.

Set-up: a fresh group-99 account whose Novice bifrost-bot creates, at
prontera 156,191; the client gets its own settings home with the tip of the
day on (own_home(t, tips=True); the harness keeps it off for other scripts).
The shortcuts act in the client only; the server's part is the account and
the character row, and the character's entry (charlog), which the script
confirms.

Not covered (each needs a scene the script does not set up): Alt+G guild,
Alt+P party settings, Alt+J pet, Alt+R and Alt+T homunculus, Ctrl+R and
Ctrl+T mercenary, Alt+W cart, Alt+B memorial dungeon (a reservation); Alt+V (basic window size), Alt+End (HP bar),
Alt+Home, Alt+Insert, Alt+F10 and Alt+0-9 open no window.
"""

import time

from bfharness import npcplay
from bfharness.accounts import unique_name
from bfharness.runner import StepFailed
from bfharness.play import enter_game, fresh_character, login, own_home

NAME = "altkeys"
MILESTONE = "M8"
DESCRIPTION = "Alt shortcuts open and close their windows (reference 0x4f58c0); the tip of the day in English"
MANIFEST = "port/harness/manifests/m8-altkeys.sha256"
NEEDS_SERVER = True

START = ("prontera", 156, 191)

# key, modifier, window, reference case in ProcessPushButton (0x4f58c0)
TOGGLES = [
    ("a", "alt", "STATUSWND", "0x41"),
    ("e", "alt", "ITEMWND", "0x45"),
    ("q", "alt", "EQUIPWND", "0x51"),
    ("s", "alt", "SKILLLISTWND", "0x53"),
    ("o", "alt", "OPTIONWND", "0x4f"),
    ("c", "alt", "CHATROOMMAKEWND", "0x43"),
    ("d", "alt", "DETAILLEVELWND", "0x44"),
    ("i", "alt", "FRIENDOPTIONWND", "0x49"),
    ("l", "alt", "EMOTIONLISTWND", "0x4c"),
    ("m", "alt", "EMOTIONWND", "0x4d"),
    ("y", "alt", "COMMANDLISTWND", "0x59"),
    ("x", "alt", "QUITWND", "0x58"),
    ("grave", "ctrl", "ROMAPWND", "0xc0"),
]
MESSENGER = "MESSENGERGROUPWND"


def packets(t, dir, header):
    """Lines of the client's packet log (BF_PKT_LOG) with this header."""
    path = t.run_dir / "client-packets.txt"
    if not path.exists():
        return []
    out = []
    for line in path.read_text(errors="replace").splitlines():
        f = line.split()
        if len(f) >= 4 and f[1] == dir and f[2].lower() == header:
            out.append({"time": int(f[0]), "header": f[2]})
    return out


def controls(ctl, window):
    w = ctl.window(window)
    return {c["id"]: c for c in (w or {}).get("controls", [])}


def tab(ctl):
    cs = controls(ctl, MESSENGER)
    on = [k for k in ("tab:friend", "tab:party") if cs.get(k, {}).get("text") == "on"]
    return on[0][4:] if on else None


def press(ctl, key, mod):
    ctl.press(key, mod)
    ctl.wait({"frames": 4}, timeout=10)


def closed(ctl, name, timeout=5.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not ctl.window(name):
            return True
        time.sleep(0.1)
    return False


def opened(ctl, name, timeout=5.0):
    try:
        ctl.wait_window(name, timeout=timeout)
        return True
    except Exception:
        return False


def tip_of_the_day(t, ctl):
    with t.step("tip of the day at entry", "GameMode.cpp: the first entry into the world opens TIPOFTHEDAYWND") as s:
        ev = [e for e in ctl.events_named("window") if e.get("name") == "TIPOFTHEDAYWND"]
        s.state("TIPOFTHEDAYWND events", ev)
        first = [e for e in ev if e.get("action") == "open"]
        s.check(first, "opened on the first entry into the world", passed=bool(first))
        if [e for e in ev if e.get("action") == "close"]:
            # rAthena sends ZC_NPCACK_MAPMOVE to the same cell right after the
            # entry (its rewarp); CGameMode::Initialize then removes every
            # window but the chat window, the tip included, and the 2008
            # client opens it once per run only.
            moves = [p for p in packets(t, "R", "0x0091")][:1]
            s.note("closed again by the client: the server's map move right after the entry %r" % moves)

    with t.step("tip of the day", "English text; Next turns to another tip, Close closes it; /tip opens it") as s:
        if not ctl.window("TIPOFTHEDAYWND"):
            npcplay.focus_chat(ctl)
            ctl.say("/tip")           # TT_OPEN_TIP: the 2008 client's command for the window
        ok = opened(ctl, "TIPOFTHEDAYWND", timeout=10)
        s.check(ok, "TIPOFTHEDAYWND open", passed=ok)
        if not ok:
            return
        ctl.wait({"frames": 10}, timeout=10)
        text = controls(ctl, "TIPOFTHEDAYWND").get("text", {})
        lines = [x for x in text.get("items", []) if x.strip()]
        s.state("tip lines", lines)
        s.check(len(lines) >= 2, "a heading and at least one line of a tip", passed=len(lines) >= 2)
        body = " ".join(lines[1:])
        english = bool(body) and all(32 <= ord(ch) < 256 for ch in body) and \
            any(w in (" %s " % body.lower()) for w in (" the ", " to ", " and ", " you ", " a ", "click"))
        s.check(english, "English text: %r" % body[:100], passed=english)
        s.check("\r" not in text.get("text", ""), "no CR in the lines", passed="\r" not in text.get("text", ""))
        s.frame("tip-of-the-day")
        before = text.get("text")
        after = before
        for _ in range(3):
            ctl.click_control("TIPOFTHEDAYWND", "next")
            ctl.wait({"frames": 4}, timeout=10)
            after = controls(ctl, "TIPOFTHEDAYWND").get("text", {}).get("text")
            if after != before:
                break
        s.state("after Next", after)
        s.check(after and after != before, "Next shows another tip", passed=bool(after and after != before))
        s.frame("tip-next")
        ctl.click_control("TIPOFTHEDAYWND", "close")
        gone = closed(ctl, "TIPOFTHEDAYWND")
        s.check(gone, "Close closes it", passed=gone)


def toggles(t, ctl):
    table = []
    for key, mod, name, case in TOGGLES:
        label = "%s+%s" % (mod.capitalize(), "`" if key == "grave" else key.upper())
        result = {"opened": False, "closed": False}
        try:
            toggle(t, ctl, key, mod, name, case, label, result)
        except StepFailed:
            pass      # recorded as a failed step; the other shortcuts still run
        table.append((label, name, case, result["opened"], result["closed"]))
    return table


def toggle(t, ctl, key, mod, name, case, label, result):
    with t.step(label, "%s opens %s, a second press closes it (case %s)" % (label, name, case)) as s:
        if ctl.window(name):
            press(ctl, key, mod)
            closed(ctl, name)
        press(ctl, key, mod)
        result["opened"] = ok = opened(ctl, name)
        s.check(ok, "%s opens %s" % (label, name), passed=ok)
        ctl.wait({"frames": 6}, timeout=10)
        s.state(name, {k: v for k, v in ctl.window(name).items() if k in ("id", "rect")})
        s.frame("alt-" + key)
        press(ctl, key, mod)
        result["closed"] = gone = closed(ctl, name)
        s.check(gone, "a second %s closes it" % label, passed=gone)


def messenger(t, ctl):
    with t.step("Alt+H, Alt+Z", "the friend and party window: Alt+H friend tab, Alt+Z party tab (cases 0x48, 0x5a)") as s:
        if ctl.window(MESSENGER):
            press(ctl, "h", "alt")
            closed(ctl, MESSENGER)
        press(ctl, "h", "alt")
        ok = opened(ctl, MESSENGER)
        s.check(ok, "Alt+H opens %s" % MESSENGER, passed=ok)
        s.equal(tab(ctl), "friend", "tab after Alt+H")
        s.frame("alt-h")
        press(ctl, "z", "alt")
        ok = opened(ctl, MESSENGER)
        s.check(ok, "Alt+Z with the friend tab open keeps the window open", passed=ok)
        s.equal(tab(ctl), "party", "tab after Alt+Z")
        s.frame("alt-z")
        press(ctl, "z", "alt")
        gone = closed(ctl, MESSENGER)
        s.check(gone, "a second Alt+Z closes it", passed=gone)
        press(ctl, "h", "alt")
        ok = opened(ctl, MESSENGER)
        s.equal(tab(ctl), "friend", "Alt+H opens it on the friend tab again")
        press(ctl, "h", "alt")
        gone = closed(ctl, MESSENGER)
        s.check(gone, "a second Alt+H closes it", passed=gone)

    with t.step("Alt+U", "the quest window's shortcut is the Korean service's (case 0x55); English: nothing") as s:
        press(ctl, "u", "alt")
        ctl.wait({"frames": 10}, timeout=10)
        quest = ctl.window("QUESTWND")
        s.check(quest is None, "QUESTWND not opened", passed=quest is None)
        if quest:
            press(ctl, "u", "alt")


def run(t):
    user = t.param("user") or unique_name(tag="ak")
    password = "bfh-test"
    name = t.param("char") or ("Bfhak" + user[-8:])
    c = fresh_character(t, user, password, name, place=START)

    env = own_home(t, tips=True)
    env["BF_MULTI_INSTANCE"] = "1"     # other clients may be running on this machine
    mark = t.logs.mark()
    ctl = t.launch(env=env)
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 20}, timeout=20)

    with t.step("entered", "the character is in the world (charlog, char.online)") as s:
        rows = t.logs.charlog(name=name, after=mark) if hasattr(t.logs, "charlog") else []
        s.rows("charlog %s" % name, rows)
        row = t.logs.char(name)
        s.state("char row", {k: row.get(k) for k in ("char_id", "online", "last_map")})
        s.check(t.mock or row.get("online"), "char.online", passed=bool(t.mock or row.get("online")))

    tip_of_the_day(t, ctl)
    # The basic window answers the shortcuts (ProcessPushButton tests m_basicInfoWnd)
    ctl.wait_window("BASICINFOWND", timeout=10)
    table = toggles(t, ctl)
    messenger(t, ctl)

    with t.step("summary", "key -> window, reference case, opened, closed") as s:
        s.state("shortcuts", [{"key": k, "window": w, "case": c, "opened": o, "closed": g}
                              for k, w, c, o, g in table])
        bad = [k for k, _w, _c, o, g in table if not (o and g)]
        s.check(not bad, "every shortcut toggles its window%s" % (": not %s" % bad if bad else ""), passed=not bad)
