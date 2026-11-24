"""M4 frames: the login screen at UI scale 1, 2 and 3, at 4:3 and in a wide window.

Done when (implementation plan, M4): the login screen appears with wallpaper,
window frames, buttons and text, at one frame per display refresh; frames
captured through the socket at UI scale 1, 2 and 3 and at a 4:3 and a wide
window size are kept under ~/Games/<name>/test/frames/ with their checksums
and this script in the repository; UI pixels are clean blocks at every scale.

    BF_CLIENT=build/cmake/debug/bifrost python3 port/harness/run.py m4_frames

Needs no server: the login screen is drawn before anything is sent. Frames go
to <game folder>/test/frames/m4/ (game artwork: outside git); their
checksums to port/harness/manifests/m4-frames.sha256. At a given commit the
frames are reproducible (the M4 verifier got byte-identical frames from two
runs), so a changed checksum means a changed picture; the checks below remain
the criterion.

Per frame, after `resize` (window content in device pixels, UI scale):
  - the reply's view and the capture have the size, scale and logical size asked;
  - the login window's pixels (but its rounded corners, which show the
    wallpaper), read back at logical resolution, are clean s x s blocks and equal those of the first frame (same art at every scale
    and size: nothing stale, nothing filtered); it sits where the client
    places it at that size (UILoginWnd: UIX(185), UICY(300), 280 x 120);
  - the cursor (the blocks that change when the pointer moves away) is
    clean blocks;
  - the wallpaper covers the screen (no black where the title tiles are).
Pacing: frames counted over 3 s against the display's refresh rate.
"""

import os
import shutil
import struct
import subprocess
import tempfile
import time
import zlib

NAME = "m4"
MILESTONE = "M4"
DESCRIPTION = "login screen at UI scale 1, 2, 3, at 4:3 and wide; clean blocks; pacing"
MANIFEST = "port/harness/manifests/m4-frames.sha256"
NEEDS_SERVER = False

# (frame name, width px, height px, UI scale): the default window of the
# client is 1024 x 768 points, 2048 x 1536 pixels on a 2x screen.
FRAMES = [
    ("login-scale2", 2048, 1536, 2),
    ("login-scale1", 2048, 1536, 1),
    ("login-scale3", 2048, 1536, 3),
    ("login-4x3", 1024, 768, 1),      # 1024 x 768 logical at scale 1
    ("login-wide", 3200, 1400, 2),    # 1600 x 700 points on a 2x screen
]
POINTER = (48, 40)                    # logical; over the wallpaper, away from the login window
LOGIN_W, LOGIN_H = 280, 120
CARET_BOX = (92, 30, 100, 16)         # the ID field, where the caret blinks (UILoginWnd::OnCreate)


# ---- PNG reading (standard library only) ----------------------------------

def read_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("%s is not a PNG" % path)
    pos, idat = 8, b""
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        pos += 12 + n
        if kind == b"IHDR":
            w, h, depth, ctype = struct.unpack(">IIBB", body[:10])
        elif kind == b"IDAT":
            idat += body
    if depth != 8 or ctype not in (2, 6):
        raise ValueError("%s: depth %d colour type %d" % (path, depth, ctype))
    bpp = 4 if ctype == 6 else 3
    raw = zlib.decompress(idat)
    stride = w * bpp
    rows, prev, p = [], bytearray(stride), 0
    for _ in range(h):
        f = raw[p]
        line = bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        if f == 1:
            for x in range(bpp, stride):
                line[x] = (line[x] + line[x - bpp]) & 255
        elif f == 2:
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 255
        elif f == 3:
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                line[x] = (line[x] + (a + prev[x]) // 2) & 255
        elif f == 4:
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                b = prev[x]
                c = prev[x - bpp] if x >= bpp else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(bytes(line))
        prev = line
    return w, h, bpp, rows


class Image:
    def __init__(self, path):
        self.w, self.h, self.bpp, self.rows = read_png(path)

    def px(self, x, y):
        o = x * self.bpp
        return self.rows[y][o:o + 3]

    def blocks(self, s, lx, ly, lw, lh, skip=None, corners=0):
        """The logical pixels of a rectangle, each read from its s x s block;
        returns (pixels, number of blocks that are not uniform). `skip` is a
        box inside the rectangle and `corners` the size of the corner squares
        to leave out (where a window's rounded corners show the wallpaper)."""
        out, bad = {}, 0
        for y in range(ly, ly + lh):
            for x in range(lx, lx + lw):
                if skip and skip[0] <= x - lx < skip[0] + skip[2] and skip[1] <= y - ly < skip[1] + skip[3]:
                    continue
                if (x - lx < corners or x - lx >= lw - corners) and (y - ly < corners or y - ly >= lh - corners):
                    continue
                p = self.px(x * s, y * s)
                if any(self.px(x * s + i, y * s + j) != p for j in range(s) for i in range(s)):
                    bad += 1
                out[(x - lx, y - ly)] = p
        return out, bad


def cursor_blocks(ctl, img, s):
    """Captures the frame again with the pointer away, to a temporary file,
    and checks the logical blocks around POINTER that differ: returns
    (blocks not uniform within a spread of 6, blocks that differ)."""
    tmp = tempfile.mkdtemp(prefix="bf-m4-")
    try:
        ctl.move(POINTER[0] + 200, POINTER[1] + 200)
        ctl.wait({"frames": 5}, 20)
        away = Image(ctl.capture(os.path.join(tmp, "away.png"))["path"])
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
        ctl.move(*POINTER)
    bad = blocks = 0
    for y in range(POINTER[1], POINTER[1] + 40):
        for x in range(POINTER[0], POINTER[0] + 32):
            cells = [(x * s + i, y * s + j) for j in range(s) for i in range(s)]
            if all(img.px(a, b) == away.px(a, b) for a, b in cells):
                continue
            blocks += 1
            pix = [img.px(a, b) for a, b in cells]
            if max(max(p[c] for p in pix) - min(p[c] for p in pix) for c in range(3)) > 6:
                bad += 1
    return bad, blocks


def login_rect(lw, lh):
    """UIWindowMgr::MakeWindow(WID_LOGINWND): Move(UIX(185), UICY(300))."""
    return (lw - 640) // 2 + 185, lh * 300 // 480, LOGIN_W, LOGIN_H


def display_refresh_hz():
    """The highest refresh rate of the screens (NSScreen.maximumFramesPerSecond), or None."""
    js = ('ObjC.import("AppKit"); '
          'Math.max.apply(null, $.NSScreen.screens.js.map(function (s) { return s.maximumFramesPerSecond; }))')
    try:
        out = subprocess.run(["osascript", "-l", "JavaScript", "-e", js], capture_output=True,
                             text=True, timeout=30).stdout.strip()
        return float(out) if out else None
    except (OSError, subprocess.SubprocessError, ValueError):
        return None


def screen_locked():
    """True while the login session's screen is locked (CGSSessionScreenIsLocked)."""
    try:
        out = subprocess.run(["ioreg", "-n", "Root", "-d1", "-a"], capture_output=True, text=True,
                             timeout=30).stdout
    except (OSError, subprocess.SubprocessError):
        return False
    i = out.find("CGSSessionScreenIsLocked")
    return i >= 0 and "<true/>" in out[i:i + 80]


def run(t):
    with t.step("launch", "client opens its control socket and shows the login screen") as s:
        ctl = t.launch()
        s.state("hello", ctl.info)
        st = ctl.wait_mode("login", 60)
        s.state("mode", st)
        ctl.wait({"frames": 10}, 20)
        ctl.move(*POINTER)
        ctl.wait({"frames": 5}, 20)

    with t.step("pacing", "one frame per display refresh (spec: up to 120 Hz)") as s:
        f0 = ctl.state("frame")["frame"]
        t0 = time.monotonic()
        time.sleep(3.0)
        f1 = ctl.state("frame")["frame"]
        t1 = time.monotonic()
        fps = (f1 - f0) / (t1 - t0)
        hz = display_refresh_hz()
        locked = screen_locked()
        s.note("%d frames in %.2f s: %.1f frames per second; screens refresh at up to %s Hz; screen %s"
               % (f1 - f0, t1 - t0, fps, hz, "locked" if locked else "unlocked"))
        s.check(fps > 20, "the client draws continuously", fps=round(fps, 1))
        s.check(fps <= 125, "presents are paced, not free-running (at most 120 Hz)", fps=round(fps, 1))
        if hz and not locked:
            s.check(abs(fps - hz) <= hz * 0.1, "a frame per display refresh", fps=round(fps, 1), hz=hz)
        elif hz:
            # A locked screen does not show the window; the window server then
            # paces Core Animation at its own rate, not the display's.
            s.note("screen locked: the pacing to the display's refresh is not observable here")

    reference = None
    for name, width, height, scale in FRAMES:
        with t.step(name, "%dx%d px at UI scale %d: login screen whole, clean blocks" % (width, height, scale)) as s:
            r = ctl.resize(width=width, height=height, scale=scale)
            view = r.get("view") or {}
            s.state("view", view)
            lw, lh = width // scale, height // scale
            s.equal((view.get("width"), view.get("height"), view.get("scale")), (width, height, scale), "view")
            s.equal(view.get("logical"), {"w": lw, "h": lh}, "logical size")
            ctl.move(*POINTER)
            ctl.wait({"frames": 10}, 20)
            rec = s.frame(name)
            # The capture is the logical screen (logical x scale); the window's
            # leftover strip (under one logical pixel) is black and left out.
            s.equal((rec["width"], rec["height"], rec["scale"]), (lw * scale, lh * scale, scale), "capture")
            if t.mock:
                continue
            img = Image(rec["path"])
            x, y, w, h = login_rect(lw, lh)
            pixels, bad = img.blocks(scale, x, y, w, h, skip=CARET_BOX, corners=2)
            s.check(bad == 0, "login window at (%d, %d): every logical pixel a clean %dx%d block" % (x, y, scale, scale),
                    nonuniform=bad)
            if reference is None:
                reference = pixels
            else:
                diff = sum(1 for k, v in reference.items() if pixels.get(k) != v)
                s.check(diff == 0, "login window identical to %s's, logical pixel for pixel" % FRAMES[0][0],
                        differing=diff)
            # The cursor: the blocks that change when the pointer goes away.
            # It is drawn with alpha 250/255, so a stretched (filtered)
            # wallpaper shows through by up to 2 %: a spread of 6 is allowed.
            cbad, cblocks = cursor_blocks(ctl, img, scale)
            s.check(cblocks > 20 and cbad == 0, "cursor at %s: %d blocks, each a clean %dx%d block"
                    % (POINTER, cblocks, scale, scale), nonuniform=cbad, blocks=cblocks)
            # The title tiles cover the screen above the tapes (5/6 of the height).
            dark = total = 0
            for gy in range(4, lh * 5 // 6 - 4, 16):
                for gx in range(4, lw - 4, 16):
                    if x - 4 <= gx < x + w + 4 and y - 4 <= gy < y + h + 4:
                        continue
                    p = img.px(gx * scale, gy * scale)
                    total += 1
                    dark += max(p) < 8
            s.check(dark * 20 < total, "wallpaper covers the screen", black=dark, sampled=total)
            s.note("login window at (%d, %d) of %dx%d logical; %d sample points, %d black" % (x, y, lw, lh, total, dark))

    with t.step("quit", "the client quits on request") as s:
        s.state("quit", ctl.quit())
