"""M8.1g (second half): screenshots; and the chat log /savechat writes.

Done when (implementation plan, M8.1; spec section 5): the 2008 client's
screenshot key writes the picture to ~/Pictures/<name>/, a JPEG (Ctrl with
the key: a BMP), at the size of the game's screen, showing the game (not
black), and the chat window says where it was saved.

The key: the 2008 client takes VK_SNAPSHOT (Print Screen) and VK_SCROLL
(Scroll Lock) (UIWindowMgr::ProcessPushButton; CMode::ScreenShot). A Mac
has neither; a PC keyboard's Print Screen arrives as F13, which the input
adapter turns into VK_SNAPSHOT (port/compat/input README, "Keys"), and the
control socket names it `f13`. Apple keyboards without F13 have the
adapter's substitute, Cmd+Shift+S, played here as well.

The client runs with BF_HOME in the run directory (play.own_home), which
the files module puts in front of every write location: the pictures land in
<run dir>/home/Pictures/<name>/, and the user's own Pictures folder is not
touched. The script compares that folder before and after each key: exactly
one new file per press. The picture is decoded with macOS's `sips` (to BMP,
read here): its size is the client's screen (the `view` part), its pixels
are not black and resemble the frame the control socket captures at the same
moment (mean brightness within a tolerance).

The same for the chat log: `/savechat` (TT_REQ_SAVE_CHAT, UINewChatWnd::
WriteChatToFile) writes Chat\Chat.txt below g_baseDir3, which lands in
<run dir>/home/Documents/<name>/Chat/, and the chat line names it there.
With `BF_APP_FOLDER` set to a long path (110, 300, 1000 bytes), the run also
shows that the game folder's length does not matter (g_baseDir3 is "C:").

Set-up: a fresh account whose character bifrost-bot creates, at
prt_fild08 230,250 (SQL).
"""

import os
import struct
import subprocess
import time
from pathlib import Path

from bfharness.accounts import unique_name
from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home

NAME = "screenshot"
MILESTONE = "M8"
DESCRIPTION = "the screenshot key writes a JPEG (Ctrl: BMP) of the screen to ~/Pictures/<name>/, /savechat the chat log to ~/Documents/<name>/Chat/, and each says so"
MANIFEST = "port/harness/manifests/m8-screenshot.sha256"
NEEDS_SERVER = True

START = ("prt_fild08", 230, 250)
APP = "Bifrost"   # only to find the folder under the run's BF_HOME; the client takes it from port/name.cmake


def ok(s, cond, message):
    s.check(cond, message, passed=bool(cond))


def pictures(t):
    if t.mock:
        return t.run_dir / "mock-pictures"
    return t.run_dir / "home" / "Pictures" / app_name()


def app_name():
    from bfharness import paths
    text = (paths.REPO / "port" / "name.cmake").read_text()
    import re
    m = re.search(r'set\(BF_APP_NAME "([^"]+)"\)', text)
    return m.group(1) if m else APP


def documents(t):
    if t.mock:
        return t.run_dir / "mock-documents"
    return t.run_dir / "home" / "Documents" / app_name()


def listing(d):
    d = Path(d)
    return {p.name: p.stat().st_size for p in d.iterdir()} if d.exists() else {}


def bmp_pixels(path):
    """(width, height, rows of (r, g, b)) of an uncompressed 24- or 32-bit BMP."""
    b = Path(path).read_bytes()
    if b[:2] != b"BM":
        raise AssertionError("%s is not a BMP" % path)
    off = struct.unpack_from("<I", b, 10)[0]
    w, h = struct.unpack_from("<ii", b, 18)
    bpp = struct.unpack_from("<H", b, 28)[0]
    comp = struct.unpack_from("<I", b, 30)[0]
    if bpp not in (24, 32) or comp not in (0, 3):
        raise AssertionError("%s: %d bits, compression %d" % (path, bpp, comp))
    step = bpp // 8
    stride = (w * step + 3) & ~3
    return w, abs(h), b, off, stride, step


def brightness(path):
    """Mean and spread of the luminance over a 64x48 grid of samples, and
    the image's size."""
    w, h, b, off, stride, step = bmp_pixels(path)
    vals = []
    for gy in range(48):
        for gx in range(64):
            x, y = (gx * w) // 64, (gy * h) // 48
            i = off + y * stride + x * step
            bl, g, r = b[i], b[i + 1], b[i + 2]
            vals.append(0.299 * r + 0.587 * g + 0.114 * bl)
    mean = sum(vals) / len(vals)
    spread = (sum((v - mean) ** 2 for v in vals) / len(vals)) ** 0.5
    return {"width": w, "height": h, "mean": round(mean, 1), "spread": round(spread, 1)}


def decode(t, src):
    """The picture as a BMP in the run directory, converted by sips."""
    out = t.run_dir / ("decoded-" + Path(src).stem + ".bmp")
    if Path(src).suffix.lower() == ".bmp":
        return Path(src)
    r = subprocess.run(["sips", "-s", "format", "bmp", str(src), "--out", str(out)],
                       capture_output=True, text=True, timeout=60)
    if r.returncode != 0 or not out.exists():
        raise AssertionError("sips could not decode %s: %s" % (src, r.stderr[-300:]))
    return out


def png_brightness(t, png):
    out = t.run_dir / ("decoded-" + Path(png).stem + ".bmp")
    subprocess.run(["sips", "-s", "format", "bmp", str(png), "--out", str(out)], capture_output=True, timeout=60)
    return brightness(out)


def take(t, s, ctl, label, mods, suffix, key=("f13",)):
    folder = pictures(t)
    before = listing(folder)
    after_seq = ctl.last_seq
    # CMode::ScreenShot reads Ctrl when it draws the picture, the frame after
    # the key: Ctrl is held down across it, as a player's finger would be
    for m in mods:
        ctl.key("down", m)
    ctl.press(key[0], *key[1:])
    ev = ctl.wait_event("chat", lambda e: "is Saved" in (e.get("text") or ""), after=after_seq, timeout=15)
    for m in mods:
        ctl.key("up", m)
    s.state("chat line", ev["text"])
    time.sleep(1.0)
    new = {k: v for k, v in listing(folder).items() if k not in before}
    s.state("new files in %s" % folder, new)
    ok(s, len(new) == 1, "one new file for one press (%d)" % len(new))
    name = sorted(new)[0]
    ok(s, name.lower().startswith("screen") and name.lower().endswith(suffix), "%s: screen...%s" % (name, suffix))
    where = name if t.mock else str(folder / name)
    ok(s, where in ev["text"], "the chat line names the file where it is: %s" % where)
    frame = s.frame(label)
    path = folder / name
    if t.mock:
        return path
    info = brightness(decode(t, path))
    ref = png_brightness(t, frame["path"])
    v = ctl.state("view")["view"]
    s.state("picture", info)
    s.state("control capture (same moment)", ref)
    s.state("view", v)
    ok(s, (info["width"], info["height"]) in ((v["width"], v["height"]), (v["logical"]["w"], v["logical"]["h"])),
       "size %dx%d: the screen (%dx%d device pixels, %dx%d logical)"
       % (info["width"], info["height"], v["width"], v["height"], v["logical"]["w"], v["logical"]["h"]))
    ok(s, info["mean"] > 20 and info["spread"] > 10, "not black or flat: mean %.1f, spread %.1f"
       % (info["mean"], info["spread"]))
    ok(s, abs(info["mean"] - ref["mean"]) < 20, "brightness like the captured frame's: %.1f against %.1f"
       % (info["mean"], ref["mean"]))
    return path


def run(t):
    user = t.param("user") or unique_name(tag="ss")
    password = "bfh-test"
    name = "Bfhss" + user[-8:]
    with t.step("set-up: character", "SQL, offline: a fresh character at %s %d,%d" % START) as s:
        aid = t.accounts.create_account(user, password)
        if not t.logs.char(name):
            if t.mock:
                t.mockdb.put("char", char_id=max([r["char_id"] for r in t.mockdb.rows("char")] or [150000]) + 1,
                             account_id=aid, char_num=0, name=name, **{"class": 0}, base_level=1, job_level=1,
                             zeny=0, last_map="prontera", last_x=155, last_y=183, online=0,
                             hp=40, max_hp=40, sp=11, max_sp=11)
            else:
                bot = Bot.start(t, "maker", user, password, name)
                bot.quit()
                t.bots.remove(bot)
        c = t.accounts.set_char(name, map=START[0], x=START[1], y=START[2])
        s.state("char row", {k: c.get(k) for k in ("char_id", "last_map", "last_x", "last_y")})
    env = own_home(t)
    env["BF_MULTI_INSTANCE"] = "1"     # other clients may be running on this machine
    if t.mock:
        env["BF_MOCK_PICTURES"] = str(pictures(t))
        env["BF_MOCK_DOCUMENTS"] = str(documents(t))
    ctl = t.launch(env=env)
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)
    with t.step("screenshot key", "F13 (a PC keyboard's Print Screen): a JPEG in Pictures/<name>/") as s:
        take(t, s, ctl, "screenshot-jpeg", (), ".jpg")
    with t.step("Ctrl + screenshot key", "Ctrl+F13: a BMP in Pictures/<name>/") as s:
        take(t, s, ctl, "screenshot-bmp", ("ctrl",), ".bmp")
    with t.step("Cmd+Shift+S", "the Print Screen substitute for Apple keyboards: a JPEG in Pictures/<name>/") as s:
        take(t, s, ctl, "screenshot-cmd-shift-s", (), ".jpg", key=("s", "cmd", "shift"))
    with t.step("/savechat", "the chat log: Chat\\Chat.txt in Documents/<name>/Chat/, the chat line names it") as s:
        folder = documents(t) / "Chat"
        before = listing(folder)
        real_chat = Path(os.path.expanduser("~/Documents")) / app_name() / "Chat"
        real_before = listing(real_chat)
        after_seq = ctl.last_seq
        ctl.say("/savechat")
        ev = ctl.wait_event("chat", lambda e: "is Saved" in (e.get("text") or ""), after=after_seq, timeout=15)
        s.state("chat line", ev["text"])
        time.sleep(0.5)
        new = {k: v for k, v in listing(folder).items() if k not in before}
        s.state("new files in %s" % folder, new)
        ok(s, len(new) == 1, "one new file (%d)" % len(new))
        name = sorted(new)[0] if new else ""
        ok(s, name.lower().startswith("chat") and name.lower().endswith(".txt"), "%s: Chat...txt" % name)
        ok(s, str(folder / name) in ev["text"], "the chat line names the file where it is: %s" % (folder / name))
        ok(s, new.get(name, 0) > 0, "not empty: %d bytes" % new.get(name, 0))
    with t.step("the user's own folder", "nothing written to ~/Pictures/<name>/ or ~/Documents/<name>/Chat/ by this run") as s:
        real = Path(os.path.expanduser("~/Pictures")) / app_name()
        s.state("pictures folder of this run", str(pictures(t)))
        mine = [n for n in listing(real) if n in listing(pictures(t))]
        ok(s, not mine or t.mock, "none of this run's files in %s" % real)
        ok(s, listing(real_chat) == real_before, "%s unchanged by /savechat" % real_chat)
    t.stop_client()
