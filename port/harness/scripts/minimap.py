"""M8: the minimap, its map button and Ctrl+Tab.

Done when: on an outdoor map the minimap opens with the basic window
(MakeWindow 0x4EFB20 for WID_BASICINFOWND, at 0x4EFC74), translucent by
default, and the basic window's map button and Ctrl+Tab (both command 0x99
of UIBasicInfoWnd::SendMsg 0x472EA0) cycle it as the 2008 client does:
translucent -> closed -> opaque -> translucent (state `windows`: the
MINIMAPZOOMWND window and its `minimap` control, PROTOCOL.md; and the
minimap's rectangle in the frames: closed and open differ, opaque and
translucent differ). On an indoor map (data\\indoorrswtable.txt; both
places test CGameMode::m_canRotateView, +0x270) there is no minimap and the
button and Ctrl+Tab do nothing.

Set-up: a fresh group-99 account whose Novice bifrost-bot creates, at
prontera 156,191 (outdoor); the client gets its own settings home, so the
minimap starts translucent (UIWindowMgr's default). The move indoors is
`@warp prt_in 126 72`, typed into the chat line (atcommandlog). The minimap
is the client's own: what the server confirms is the character's entry
(char.online), the warp (atcommandlog) and, on logout, the indoor map the
character was saved on.
"""

import struct
import zlib

from bfharness import npcplay
from bfharness.accounts import unique_name
from bfharness.play import enter_game, fresh_character, login, own_home
from bfharness.sql import wait_rows

NAME = "minimap"
MILESTONE = "M8"
DESCRIPTION = "the basic window's map button and Ctrl+Tab cycle the minimap outdoors (reference 0x472EA0 case 0x99)"
MANIFEST = "port/harness/manifests/m8-minimap.sha256"
NEEDS_SERVER = True

START = ("prontera", 156, 191)
INDOOR = ("prt_in", 126, 72)
CYCLE = ["closed", "opaque", "translucent"]     # after one, two, three commands from translucent


def read_png(path):
    """(width, height, channels, rows) of an 8-bit RGB or RGBA PNG."""
    data = open(path, "rb").read()
    pos, idat = 8, b""
    w = h = ctype = 0
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h, depth, ctype = struct.unpack(">IIBB", body[:10])
            if depth != 8 or ctype not in (2, 6):
                raise ValueError("PNG kind not read here: depth %d colour type %d" % (depth, ctype))
        elif kind == b"IDAT":
            idat += body
        pos += 12 + n
    ch = 4 if ctype == 6 else 3
    raw = zlib.decompress(idat)
    stride = w * ch
    rows, prev = [], bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - ch] if i >= ch else 0
            b = prev[i]
            c = prev[i - ch] if i >= ch else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(line)
        prev = line
    return w, h, ch, rows


def region_difference(rec_a, rec_b, rect):
    """Share of the pixels of rect (logical [x, y, w, h]) whose colour differs
    by more than 24 in a channel between two frames."""
    k = rec_a.get("scale") or 1
    x0, y0 = int(rect[0] * k), int(rect[1] * k)
    x1, y1 = int((rect[0] + rect[2]) * k), int((rect[1] + rect[3]) * k)
    wa, ha, ca, ra = read_png(rec_a["path"])
    wb, hb, cb, rb = read_png(rec_b["path"])
    changed = total = 0
    for y in range(max(0, y0), min(y1, ha, hb)):
        la, lb = ra[y], rb[y]
        for x in range(max(0, x0), min(x1, wa, wb)):
            total += 1
            if max(abs(la[x * ca + j] - lb[x * cb + j]) for j in range(3)) > 24:
                changed += 1
    return changed / total if total else 0.0


def minimap(ctl):
    """'closed', or the minimap control's opacity, and its rectangle."""
    w = ctl.window("MINIMAPZOOMWND")
    if not w:
        return "closed", None
    c = next((c for c in w["controls"] if c["id"] == "minimap"), None)
    return (c["text"], c["rect"]) if c else ("no minimap control", None)


def command(ctl, how):
    if how == "button":
        ctl.click_control("BASICINFOWND", "map")
    else:
        ctl.press("tab", "ctrl")
    ctl.wait({"frames": 10}, timeout=10)


def cycle(t, ctl, how, label):
    """Three commands from translucent; a frame after each."""
    with t.step(label, "M8: command 0x99 (%s) cycles the minimap translucent -> closed -> opaque -> translucent"
                % ("the basic window's map button" if how == "button" else "Ctrl+Tab")) as s:
        state, rect = minimap(ctl)
        s.equal(state, "translucent", "before: open, translucent")
        frames = {"translucent": s.frame("%s-0-translucent" % how)}
        seen = []
        for i, want in enumerate(CYCLE, 1):
            command(ctl, how)
            state, r = minimap(ctl)
            rect = r or rect
            seen.append(state)
            s.equal(state, want, "after %d: %s" % (i, want))
            frames.setdefault(state, s.frame("%s-%d-%s" % (how, i, state)))
        s.state("states", seen)
        if rect and all(k in frames for k in ("closed", "opaque", "translucent")):
            closed = region_difference(frames["closed"], frames["opaque"], rect)
            alpha = region_difference(frames["opaque"], frames["translucent"], rect)
            s.note("minimap rectangle %s: closed/opaque differ in %.0f%%, opaque/translucent in %.0f%%"
                   % (rect, closed * 100, alpha * 100))
            s.check(closed > 0.3, "the frames show the minimap open and closed", share=closed)
            if t.mock:
                s.note("mock frames draw no opacity; opaque/translucent compared live only")
            else:
                s.check(alpha > 0.1, "the frames show it opaque and translucent", share=alpha)


def run(t):
    user = t.param("user") or unique_name(tag="mm")
    password = "bfh-test"
    name = t.param("char") or ("Bfhmm" + user[-8:])
    c = fresh_character(t, user, password, name, place=START)

    env = own_home(t)
    env["BF_MULTI_INSTANCE"] = "1"     # other clients may be running on this machine
    ctl = t.launch(env=env)
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait_window("BASICINFOWND", timeout=10)
    ctl.wait({"frames": 30}, timeout=20)

    with t.step("entered outdoors", "the character is in prontera (char.online); the minimap opened with the "
                "basic window (MakeWindow 0x4EFB20 at 0x4EFC74), translucent") as s:
        row = t.logs.char(name)
        s.state("char row", {k: row.get(k) for k in ("char_id", "online", "last_map")})
        s.check(t.mock or row.get("online"), "char.online", passed=bool(t.mock or row.get("online")))
        state, rect = minimap(ctl)
        s.state("minimap", {"state": state, "rect": rect})
        s.equal(state, "translucent", "minimap open, translucent")
        basic = ctl.window("BASICINFOWND")
        s.check(basic and any(k["id"] == "map" for k in basic["controls"]), "the basic window shows its map button")

    cycle(t, ctl, "button", "map button")
    cycle(t, ctl, "ctrl-tab", "Ctrl+Tab")

    with t.step("indoors", "M8: on an indoor map (prt_in) there is no minimap; the button and Ctrl+Tab do "
                "nothing (CGameMode +0x270, m_canRotateView, is 0)") as s:
        mark = t.logs.mark()
        npcplay.focus_chat(ctl)
        t.gm.run(t.gm.warp(*INDOOR))
        s.rows("atcommandlog @warp", wait_rows(lambda: t.logs.atcommands(contains="@warp prt_in", after=mark),
                                               timeout=15))
        ctl.wait({"map": INDOOR[0]}, timeout=60)
        ctl.wait_window("BASICINFOWND", timeout=20)
        ctl.wait({"frames": 30}, timeout=20)
        seen = [minimap(ctl)[0]]
        for how in ("button", "ctrl-tab", "button"):
            command(ctl, how)
            seen.append(minimap(ctl)[0])
        s.state("minimap: on entry, after button, Ctrl+Tab, button", seen)
        s.check(all(x == "closed" for x in seen), "no minimap indoors, whatever is pressed", seen=seen)
        s.frame("prt_in")

    t.stop_client()
    with t.step("saved", "the server saved the character on the indoor map on logout") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]])
        r = rows[-1]
        s.state("char row after logout", {k: r[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})
        s.equal(r["last_map"], INDOOR[0], "saved map")
