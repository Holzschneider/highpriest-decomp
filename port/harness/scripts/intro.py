"""M12.4a intro movie: CBink plays a Bink movie through the port's reader.

    BF_CLIENT=<client> python3 port/harness/run.py intro

The 2008 client plays openning.bik from the game folder before the login
screen (WinMain: g_Bink.OpenningMain(1)), unless the registry's BINKMODE is 1
(a key stopped it once); any key stops it. The port skips it by default
(option SkipIntroScreens, spec section 2), so every launch here that wants the
movie has its own settings (BF_HOME) with the option off. openning.bik is in
none of the port's data, so:

  A  BF_INTRO_MOVIE=data\\video\\ad\\01.bik (from data.grf; 450 frames at 30
     fps, no sound) plays to its end; frames are captured at three points
     and must be neither black nor alike; the measured rate is about 30 fps;
     the login screen follows.
  E  a game folder of its own under <game folder>/test/intro/folder (links to
     the archives, data/ and BGM/, and openning.bik linked to the public
     sample test/bink/samples/original.bik, 280 frames at 25 fps with sound):
     the 2008 lookup of openning.bik. The sound plays as a voice of the sound
     module tagged with the movie; Esc stops the movie (BINKMODE becomes 1)
     and the login screen follows.
  C  E's settings again: BINKMODE 1, the movie is not played again.
  G  E's folder with default settings: skipped by the option.
  D  the normal game folder, option off, no BF_INTRO_MOVIE: openning.bik is
     absent, the client says so and goes on to the login screen.

In every launch the login window has no intro button: UILoginWnd offers it
only when ..\\openning.bik exists and only for the services 0, 3, 10, 11, 13
and 14 (the 2008 executable's loop, checked against the reference
disassembly); the game folder's service is america.

Evidence: the control socket's `movie` part and event, the `sound` part, the
trace lines of CBink ("intro: ...") and of the adapter ("bink: ..."), and
frames under <game folder>/test/frames/intro/ (game artwork, outside git;
checksums in the manifest).
"""

import os
import plistlib
import struct
import subprocess
import time
import zlib
from pathlib import Path

from bfharness import paths

NAME = "intro"
MILESTONE = "M12"
DESCRIPTION = "the intro movie plays through the Bink adapter, with sound, ends or is stopped by a key; login follows"
MANIFEST = "port/harness/manifests/m12-intro.sha256"
NEEDS_SERVER = False

TRACE = "err,mode,stub,ui,gfx,snd"
ARCHIVE_MOVIE = "data\\video\\ad\\01.bik"
SAMPLE = "test/bink/samples/original.bik"
FOLDER_LINKS = ["data.grf", "rdata.grf", "data", "BGM", "DATA.INI"]


# -- settings and folders ------------------------------------------------------

def make_home(t, name, skip_intro=None):
    """A BF_HOME with Settings.plist; skip_intro None keeps the default."""
    home = t.run_dir / ("home-" + name)
    folder = home / "Library" / "Application Support" / paths.app_name()
    folder.mkdir(parents=True, exist_ok=True)
    values = {"FormatVersion": 1}        # fresh: a run directory may be reused
    if skip_intro is not None:
        values["Options"] = {"SkipIntroScreens": int(skip_intro)}
    with open(folder / "Settings.plist", "wb") as f:
        plistlib.dump(values, f)
    return home


def bink_mode(home):
    plist = home / "Library" / "Application Support" / paths.app_name() / "Settings.plist"
    try:
        with open(plist, "rb") as f:
            values = plistlib.load(f)
    except (OSError, plistlib.InvalidFileException):
        return None
    for k, v in values.items():
        if k.lower() == "client" and isinstance(v, dict):
            for kk, vv in v.items():
                if kk.lower() == "binkmode":
                    return vv
    return None


def intro_folder(t):
    """A game folder whose openning.bik is the public sample movie; None
    without the sample. Real runs make it under the game folder's test/,
    mock runs in the run directory."""
    game = paths.client_dir()
    sample = game / SAMPLE
    if t.mock:
        folder = t.run_dir / "intro-folder"
        folder.mkdir(exist_ok=True)
        (folder / "openning.bik").write_bytes(b"BIKi mock")
        return folder
    if not sample.exists():
        return None
    folder = game / "test" / "intro" / "folder"
    folder.mkdir(parents=True, exist_ok=True)
    for name in FOLDER_LINKS + ["openning.bik"]:
        link = folder / name
        target = sample if name == "openning.bik" else game / name
        if not target.exists():
            continue
        if link.is_symlink() or link.exists():
            if link.is_symlink() and Path(os.readlink(link)) == target:
                continue
            link.unlink()
        link.symlink_to(target)
    return folder


# -- frames ---------------------------------------------------------------------

def read_png(path):
    """(width, height, rows of RGB bytes) of an 8-bit RGB or RGBA PNG."""
    data = Path(path).read_bytes()
    pos, idat, w = 8, b"", None
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
            if depth != 8 or ctype not in (2, 6) or interlace:
                raise ValueError("%s: PNG depth %d type %d not read here" % (path, depth, ctype))
            bpp = 3 if ctype == 2 else 4
        elif kind == b"IDAT":
            idat += body
        pos += 12 + n
    raw = zlib.decompress(idat)
    stride = w * bpp
    rows, prev = [], bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + ((a + b) >> 1)) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(bytes(line) if bpp == 3 else bytes(v for i, v in enumerate(line) if i % 4 != 3))
        prev = line
    return w, h, rows


def small_copy(path):
    """A copy at most 128 pixels wide (sips, macOS), so that Python decodes it
    quickly; the path itself where sips is missing."""
    out = Path(str(path) + ".small.png")
    try:
        subprocess.run(["sips", "-Z", "128", str(path), "--out", str(out)], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return out
    except (OSError, subprocess.CalledProcessError):
        return path


def signature(path, grid=16):
    """Mean brightness, share of non-black pixels, and a grid x grid thumbnail."""
    small = small_copy(path)
    w, h, rows = read_png(small)
    if small != path:
        small.unlink()
    cells = [[0, 0] for _ in range(grid * grid)]
    lit = total = bright = 0
    for y in range(h):
        row = rows[y]
        gy = y * grid // h
        for x in range(w):
            r, g, b = row[3 * x], row[3 * x + 1], row[3 * x + 2]
            v = (r * 299 + g * 587 + b * 114) // 1000
            total += 1
            bright += v
            lit += v > 16
            c = cells[gy * grid + x * grid // w]
            c[0] += v
            c[1] += 1
    return {"mean": bright / max(total, 1), "lit": lit / max(total, 1),
            "thumb": [c[0] / max(c[1], 1) for c in cells]}


def thumb_diff(a, b):
    return sum(abs(x - y) for x, y in zip(a["thumb"], b["thumb"])) / len(a["thumb"])


# -- the movie through the control socket ------------------------------------------

def movie(ctl):
    return ctl.state("movie")["movie"]


def wait_movie(ctl, pred, timeout, what):
    deadline = time.monotonic() + timeout
    while True:
        m = movie(ctl)
        if pred(m):
            return m
        if time.monotonic() > deadline:
            raise AssertionError("%s: not within %.0f s (movie %r)" % (what, timeout, m))
        time.sleep(0.02)


def wait_end(ctl, timeout):
    r = ctl.wait({"event": "movie", "match": {"what": "end"}}, timeout=timeout, after=0)
    return r["event"]


def to_login_window(s, ctl):
    """The login screen: with SkipIntroScreens 0 the service selection
    (SELECTSERVERWND) comes first, as in 2008; Enter takes the one service."""
    ctl.wait_mode("login", 30)
    for _ in range(50):
        names = [w["name"] for w in ctl.state("windows")["windows"]]
        if "LOGINWND" in names:
            break
        if "SELECTSERVERWND" in names:
            s.note("service selection shown (option off): Enter")
            ctl.press("enter")
            ctl.wait_window("SELECTSERVERWND", open=False, timeout=10)
        time.sleep(0.1)
    ctl.wait_window("LOGINWND", timeout=10)


def no_intro_button(s, ctl):
    w = ctl.window("LOGINWND")
    ids = [c["id"] for c in (w or {}).get("controls", [])]
    s.state("LOGINWND controls", ids)
    s.check(w is not None, "the login window is open")
    s.check("intro" not in ids, "the login window offers no intro button (2008: service america never shows it)")


def finish(t, s_name, criterion, checks):
    """Stops the client and checks its trace lines: checks is a list of
    (needle, message)."""
    c = t.client
    t.stop_client()
    with t.step(s_name, criterion) as s:
        lines = c.trace_lines("intro:", "bink:")
        s.trace("intro and bink lines", lines)
        for needle, message in checks:
            s.check(any(needle in l for l in lines), message, needle=needle)


# -- the script ------------------------------------------------------------------

def run(t):
    game = paths.client_dir()

    # A: a movie from the archives, played to its end.
    home_a = make_home(t, "a", skip_intro=0)
    ctl = t.launch(env={"BF_HOME": str(home_a), "BF_INTRO_MOVIE": ARCHIVE_MOVIE, "BF_TRACE": TRACE})
    with t.step("A: the named movie plays at the start",
                "BF_INTRO_MOVIE=%s with SkipIntroScreens 0: CBink opens it before the login mode" % ARCHIVE_MOVIE) as s:
        m = wait_movie(ctl, lambda m: m["playing"] and m["frame"] >= 1, 60, "the movie starts")
        s.state("movie", m)
        s.equal(m["file"], ARCHIVE_MOVIE, "file")
        s.equal((m["width"], m["height"], m["frames"]), (512, 256, 450), "size and frames")
        s.check(abs(m["fps"] - 30) < 0.01, "30 fps", fps=m["fps"])
        s.equal(ctl.state("mode")["mode"], "start", "mode while the intro plays")
    with t.step("A: the movie runs at its rate", "frames decoded over two seconds against 30 fps") as s:
        m0, t0 = movie(ctl), time.monotonic()
        time.sleep(2.0)
        m1, t1 = movie(ctl), time.monotonic()
        rate = (m1["frame"] - m0["frame"]) / (t1 - t0)
        s.note("frame %d -> %d in %.2f s" % (m0["frame"], m1["frame"], t1 - t0))
        s.check(m1["playing"] and 27 <= rate <= 33, "about 30 frames per second", measured=round(rate, 2))
    with t.step("A: frames change and are not black", "captures at movie frames 90, 200, 380") as s:
        shots = []
        for target in (90, 200, 380):
            m = wait_movie(ctl, lambda m, n=target: m["frame"] >= n or not m["playing"], 30, "frame %d" % target)
            s.check(m["playing"], "still playing at frame %d" % target)
            shots.append((m["frame"], s.frame("a-movie-%03d" % target)))
        sigs = []
        for f, rec in shots:
            sig = signature(rec["path"])
            sigs.append(sig)
            s.note("frame %d: mean %.1f, lit %.2f" % (f, sig["mean"], sig["lit"]))
            s.check(sig["lit"] > 0.2 and sig["mean"] > 12, "frame %d is not black" % f,
                    mean=sig["mean"], lit=sig["lit"])
        for i in range(len(sigs)):
            for j in range(i + 1, len(sigs)):
                d = thumb_diff(sigs[i], sigs[j])
                s.check(d > 3, "frames %d and %d differ" % (shots[i][0], shots[j][0]), mean_abs_diff=d)
    with t.step("A: the movie ends and the login screen follows",
                "event movie end with ended, frame 450; then LOGINWND, mode login") as s:
        e = wait_end(ctl, 30)
        s.state("end event", e)
        s.check(e["ended"] and e["frame"] == 450, "shown to its last frame", event=e)
        ctl.wait_mode("login", 30)
        sig = signature(s.frame("a-after")["path"])
        to_login_window(s, ctl)
        no_intro_button(s, ctl)
        s.check(thumb_diff(sig, sigs[-1]) > 3, "the login screen replaced the movie")
        s.frame("a-login")
    finish(t, "A: trace lines", "CBink's and the adapter's start and end lines",
           [("intro: start, 450 frames", "CBink: intro start"),
            ("bink: open %s: 512x256, 450 frames" % ARCHIVE_MOVIE, "adapter: open"),
            ("intro: end at frame 450 of 450 (ended)", "CBink: intro end"),
            ("bink: close %s after frame 450 of 450 (ended)" % ARCHIVE_MOVIE, "adapter: close")])

    # E: openning.bik in a game folder of its own, as the 2008 client finds it.
    folder = intro_folder(t)
    if folder is None:
        with t.step("E: openning.bik with sound, stopped by Esc", "needs %s" % SAMPLE) as s:
            s.note("skipped: %s is absent" % (game / SAMPLE))
    else:
        home_e = make_home(t, "e", skip_intro=0)
        ctl = t.launch(env={"BF_HOME": str(home_e), "BF_CLIENT_DIR": str(folder), "BF_TRACE": TRACE})
        with t.step("E: openning.bik plays with its sound",
                    "the game folder's openning.bik (the sample, 280 frames, 25 fps, one sound track); "
                    "its first track is a sound-module voice tagged with the movie") as s:
            m = wait_movie(ctl, lambda m: m["playing"] and m["frame"] >= 25, 60, "frame 25")
            s.state("movie", m)
            s.equal(m["file"], "openning.bik", "file")
            s.equal(m["frames"], 280, "frames")
            s.check(m["tracks"] == 1 and m["sound"] and m["voice"] != 0, "the sound track plays", movie=m)
            snd = ctl.state("sound")["sound"]
            voices = [v for v in snd["recent"] if v["file"] == "openning.bik"]
            s.state("sound", {"mode": snd["mode"], "voices": voices})
            s.check(len(voices) == 1 and voices[0]["started"], "one voice started for the movie")
            s.check(abs(voices[0]["duration"] - 280 / 25) < 0.05, "the voice lasts as long as the movie",
                    duration=voices[0]["duration"])
            sig = signature(s.frame("e-movie")["path"])
            s.check(sig["lit"] > 0.2, "the frame is not black", lit=sig["lit"])
        with t.step("E: Esc stops the movie and the login screen follows",
                    "a key stops the movie (WndProc: g_Bink.Break); event end with ended false") as s:
            m0 = movie(ctl)
            ctl.press("escape")
            e = wait_end(ctl, 10)
            s.state("end event", e)
            s.check(not e["ended"] and m0["frame"] <= e["frame"] < 280, "stopped before its end", event=e)
            to_login_window(s, ctl)
            no_intro_button(s, ctl)
            s.check(movie(ctl)["voice"] == 0, "the sound is stopped with the movie")
            s.frame("e-login")
        finish(t, "E: trace lines", "start and stop",
               [("intro: start, 280 frames", "CBink: intro start"),
                ("bink: open openning.bik: 512x384, 280 frames at 25.000 fps, 1 sound track(s), first track played",
                 "adapter: open with sound"),
                ("(stopped)", "CBink: stopped by the key")])
        with t.step("E: BINKMODE 1 is saved", "CBink::WriteBinkModeRegistry after a key") as s:
            s.equal(bink_mode(home_e), 1, "Client/BINKMODE in the settings")

        # C: the same settings: not played again.
        ctl = t.launch(env={"BF_HOME": str(home_e), "BF_CLIENT_DIR": str(folder), "BF_TRACE": TRACE})
        with t.step("C: after a key the movie is not played again", "BINKMODE 1 and flag 1: OpenningMain returns") as s:
            to_login_window(s, ctl)
            s.equal(movie(ctl)["opened"], 0, "movies opened")
        finish(t, "C: trace lines", "skipped by BINKMODE",
               [("intro: skipped (BINKMODE 1", "CBink: skipped")])

        # G: default settings: the option skips it.
        ctl = t.launch(env={"BF_HOME": str(make_home(t, "g")), "BF_CLIENT_DIR": str(folder), "BF_TRACE": TRACE})
        with t.step("G: by default the option skips the movie", "SkipIntroScreens defaults to 1") as s:
            ctl.wait_window("LOGINWND", timeout=60)
            s.equal(movie(ctl)["opened"], 0, "movies opened")
            no_intro_button(s, ctl)
        finish(t, "G: trace lines", "skipped by the option",
               [("intro: skipped (option SkipIntroScreens)", "CBink: skipped by the option")])

    # D: openning.bik absent: the client goes on.
    ctl = t.launch(env={"BF_HOME": str(make_home(t, "d", skip_intro=0)), "BF_TRACE": TRACE})
    with t.step("D: without openning.bik the client goes on to the login screen",
                "missing data never stops the client (spec section 5)") as s:
        ctl.wait_mode("login", 60)
        to_login_window(s, ctl)
        s.equal(movie(ctl)["opened"], 0, "movies opened")
        no_intro_button(s, ctl)
        s.frame("d-login")
    finish(t, "D: trace lines", "the missing movie is named",
           [("intro: no movie (openning.bik not found in the game folder)", "CBink: no movie")])
