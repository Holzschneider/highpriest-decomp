"""M8.1g (first half): the option window, two display toggles, the free
camera, and their persistence across a restart.

Done when (implementation plan, M8.1): the option window (Alt+O) turns
sound and music off and changes their volumes; the 2008 client's display
toggles typed into the chat line (/effect, /camera) answer with their
messages; the free camera (spec section 2, option FreeCamera, on by default)
lets the wheel zoom out beyond the 2008 limit and is held to it when the
option is off; and every setting is the same after the client is started
again: in the option window, in the toggles' answers, and in the settings
file (Settings.plist under the run's BF_HOME, PROTOCOL.md does not read it).

Set-up: a fresh account whose character bifrost-bot creates, placed at
prt_fild08 250,200 (SQL). The client runs with BF_HOME in the run directory
(play.own_home), so it starts from the defaults. Between the two runs the
settings file's FreeCamera is set to 0, as the settings window would
(port/native/prefs): the Mac settings are not the in-game option window.

Played, first run: zoom in with the wheel (10 notches), then out (40 notches away from the user) and read how far
from the character the cell 300 logical pixels right of the centre is; Alt+O;
click `sound_on` and `bgm_on` off, click each volume bar's track left of its
thumb (a page, -10); the `noctrl` check on; type /effect and /camera; quit.
Second run: the window shows the same; /effect and /camera answer with the
opposite messages (they were off and on); the same zoom reaches less far.
"""

import math
import plistlib
import time

from bfharness.accounts import unique_name
from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home

NAME = "options"
MILESTONE = "M8"
DESCRIPTION = "option window toggles and volumes, /effect and /camera, free camera; kept across a restart"
MANIFEST = "port/harness/manifests/m8-options.sha256"
NEEDS_SERVER = True

START = ("prt_fild08", 250, 200)
NOTCHES = 40
PROBE_DX = 300


def ok(s, cond, message):
    s.check(cond, message, passed=bool(cond))


def controls(ctl, window):
    w = ctl.window(window)
    return {c["id"]: c for c in (w or {}).get("controls", [])}


def centre(c):
    x, y, w, h = c["rect"]
    return x + w // 2, y + h // 2


def settings_path(t):
    if t.mock:
        return t.run_dir / "mock-settings.plist"
    return t.run_dir / "home" / "Library" / "Application Support" / "Bifrost" / "Settings.plist"


def read_settings(t):
    p = settings_path(t)
    if not p.exists():
        return {}
    with open(p, "rb") as f:
        return plistlib.load(f)


def option_values(t):
    d = read_settings(t)
    client = next((v for k, v in d.items() if k.lower() == "client"), {})
    opt = next((v for k, v in client.items() if k.lower() == "option"), {})
    return {k: opt.get(k) for k in ("isSoundOn", "bgmIsPaused", "streamVolume", "soundVolume", "isEffectOn",
                                    "g_isFixedCamera", "m_isNoCtrl")}


def setup(t):
    user = t.param("user") or unique_name(tag="op")
    password = "bfh-test"
    name = "Bfhop" + user[-8:]
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
    return user, password, name


def start(t, user, password):
    env = own_home(t)
    env["BF_MULTI_INSTANCE"] = "1"     # other clients may be running on this machine
    ctl = t.launch(env=env)
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 30}, timeout=20)
    return ctl


def reach(ctl):
    """How many cells from the character the cell under the pointer is, PROBE_DX
    logical pixels right of the window's centre: grows with the camera's distance."""
    v = ctl.state("view")["view"]
    x, y = v["logical"]["w"] // 2 + PROBE_DX, v["logical"]["h"] // 2
    ctl.move(x, y)
    ctl.wait({"frames": 3}, timeout=10)
    st = ctl.state("pointer", "self")
    cell, me = st["pointer"]["cell"], st["self"]
    if not cell:
        return None
    return round(math.hypot(cell["x"] - me["x"], cell["y"] - me["y"]), 1)


def zoom(t, s, ctl, label):
    """The probe's reach at the start, after 10 wheel notches towards the user
    (closer) and after 40 away (CGameMode::ProcessWheel: away moves the
    camera away), each after the view has eased to its new distance."""
    v = ctl.state("view")["view"]
    cx, cy = v["logical"]["w"] // 2, v["logical"]["h"] // 2 + 60
    out = [reach(ctl)]
    for dy, n in ((-1, 10), (1, NOTCHES)):
        for _ in range(n):
            ctl.wheel(dy, cx, cy)
            ctl.wait({"frames": 2}, timeout=10)
        ctl.wait({"frames": 60}, timeout=20)
        out.append(reach(ctl))
    s.state("reach of the probe point (cells): start, 10 notches in, then %d out" % NOTCHES, out)
    s.frame("zoomed-out-%s" % label)
    return out


def say(t, ctl, text, expect):
    after = ctl.last_seq
    ctl.say(text)
    ev = ctl.wait_event("chat", lambda e: e.get("from") is None and any(x in (e.get("text") or "") for x in expect),
                        after=after, timeout=10)
    return ev["text"]


def option_window(ctl):
    if not ctl.window("OPTIONWND"):
        ctl.press("o", "alt")
        ctl.wait_window("OPTIONWND", timeout=10)
    ctl.wait({"frames": 3}, timeout=10)
    return controls(ctl, "OPTIONWND")


def page_left(ctl, bar):
    """A click on a horizontal scroll bar's track just right of its left arrow:
    UIScrollBar's page-up, -10 on the option window's volume bars."""
    x, y, w, h = bar["rect"]
    ctl.click(x + 16, y + h // 2)
    ctl.wait({"frames": 3}, timeout=10)


def run(t):
    user, password, name = setup(t)

    # ---- first run: defaults, then change everything
    ctl = start(t, user, password)
    with t.step("free camera on (default)", "spec 2: the wheel zooms out beyond the 2008 limit") as s:
        free = zoom(t, s, ctl, "free")
        ok(s, None not in free and free[1] < free[0] < free[2],
           "the wheel zooms in and out, beyond the start: %r" % (free,))
    with t.step("option window", "Alt+O opens OPTIONWND with its checks and volume bars") as s:
        cs = option_window(ctl)
        s.state("OPTIONWND controls", {k: (c["kind"], c["text"]) for k, c in cs.items()})
        for cid in ("bgm_on", "sound_on", "noctrl", "bgm_volume", "sound_volume"):
            ok(s, cid in cs, "control %s" % cid)
        s.equal(cs["sound_on"]["text"], "on", "sound on by default")
        s.equal(cs["bgm_on"]["text"], "on", "music on by default")
        s.equal(cs["noctrl"]["text"], "off", "NoCtrl off by default")
        snd = ctl.state("sound")["sound"]
        s.state("sound part", {k: snd.get(k) for k in ("mode", "effects_volume", "music_volume")} |
                {"stream": snd.get("stream")})
        s.frame("options-default")
        defaults = {"bgm": int(cs["bgm_volume"]["text"]), "sound": int(cs["sound_volume"]["text"]),
                    "stream": snd["stream"]["status"], "stream_volume": snd["stream"]["volume"]}
    with t.step("volumes", "a page left on each volume bar: -10; the music stream's volume follows") as s:
        page_left(ctl, controls(ctl, "OPTIONWND")["bgm_volume"])
        page_left(ctl, controls(ctl, "OPTIONWND")["sound_volume"])
        cs = controls(ctl, "OPTIONWND")
        s.equal(int(cs["bgm_volume"]["text"]), defaults["bgm"] - 10, "music volume")
        s.equal(int(cs["sound_volume"]["text"]), defaults["sound"] - 10, "effects volume")
        snd = ctl.state("sound")["sound"]
        s.state("sound part volumes", {k: snd.get(k) for k in ("effects_volume", "music_volume")})
        s.state("stream", snd["stream"])
        # SetStreamVolume sets the stream's own volume (Miles' 0..127 / 127);
        # the masters (effects_volume, music_volume) are the settings window's
        ok(s, abs(snd["stream"]["volume"] - (defaults["bgm"] - 10) / 127.0) < 0.01,
           "the stream's volume %.4f -> %.4f (%d/127)" % (defaults["stream_volume"], snd["stream"]["volume"],
                                                       defaults["bgm"] - 10))
    with t.step("sound and music off", "the option window's checks: sound_on, bgm_on off; the music stops") as s:
        ctl.click_control("OPTIONWND", "sound_on")
        ctl.click_control("OPTIONWND", "bgm_on")
        ctl.wait({"frames": 10}, timeout=10)
        cs = controls(ctl, "OPTIONWND")
        s.equal(cs["sound_on"]["text"], "off", "sound_on")
        s.equal(cs["bgm_on"]["text"], "off", "bgm_on")
        snd = ctl.state("sound")["sound"]
        s.state("stream", snd["stream"])
        ok(s, snd["stream"]["status"] != "playing", "music %s -> %s" % (defaults["stream"], snd["stream"]["status"]))
    with t.step("NoCtrl on", "the option window's noctrl check (/noctrl)") as s:
        ctl.click_control("OPTIONWND", "noctrl")
        ctl.wait({"frames": 3}, timeout=10)
        s.equal(controls(ctl, "OPTIONWND")["noctrl"]["text"], "on", "noctrl")
        s.frame("options-changed")
    with t.step("display toggles", "/effect and /camera answer in the chat window") as s:
        s.state("/effect", say(t, ctl, "/effect", ("Effects Off", "Effects On")))
        s.state("/camera", say(t, ctl, "/camera", ("amera",)))
        lines = [e["text"] for e in ctl.events_named("chat") if e.get("from") is None][-2:]
        ok(s, "Effects Off" in lines[0], "effects turned off: %r" % lines[0])
        first_camera = lines[1]
    t.stop_client()
    with t.step("settings file", "Settings.plist holds the options the client wrote on quitting") as s:
        vals = option_values(t)
        s.state("Client/Option", vals)
        s.equal(vals["isSoundOn"], 0, "isSoundOn")
        s.equal(vals["bgmIsPaused"], 1, "bgmIsPaused")
        s.equal(vals["streamVolume"], defaults["bgm"] - 10, "streamVolume")
        s.equal(vals["soundVolume"], defaults["sound"] - 10, "soundVolume")
        s.equal(vals["isEffectOn"], 0, "isEffectOn")
        s.equal(vals["g_isFixedCamera"], 1, "g_isFixedCamera")
        s.equal(vals["m_isNoCtrl"], 1, "m_isNoCtrl")
    with t.step("set-up: free camera off", "Settings.plist Options/FreeCamera = 0, as the settings window writes it") as s:
        d = read_settings(t)
        d.setdefault("Options", {})["FreeCamera"] = 0
        with open(settings_path(t), "wb") as f:
            plistlib.dump(d, f)
        s.state("Options", read_settings(t).get("Options"))

    # ---- second run: everything as left. play.login captures login-typed
    # again, over the first run's file: only the second one stays recorded.
    t.frames.records = [r for r in t.frames.records if r["name"] != "login-typed"]
    ctl = start(t, user, password)
    with t.step("options after a restart", "the option window shows what the first run set") as s:
        cs = option_window(ctl)
        s.state("OPTIONWND controls", {k: (c["kind"], c["text"]) for k, c in cs.items()})
        s.equal(cs["sound_on"]["text"], "off", "sound_on")
        s.equal(cs["bgm_on"]["text"], "off", "bgm_on")
        s.equal(int(cs["bgm_volume"]["text"]), defaults["bgm"] - 10, "music volume")
        s.equal(int(cs["sound_volume"]["text"]), defaults["sound"] - 10, "effects volume")
        s.equal(cs["noctrl"]["text"], "on", "noctrl")
        snd = ctl.state("sound")["sound"]
        s.state("sound part", {k: snd.get(k) for k in ("effects_volume", "music_volume")} | {"stream": snd["stream"]})
        ok(s, snd["stream"]["status"] != "playing", "no map music: %s" % snd["stream"]["status"])
        s.frame("options-restarted")
        ctl.press("o", "alt")
    with t.step("display toggles after a restart", "/effect and /camera now answer the other way") as s:
        s.state("/effect", say(t, ctl, "/effect", ("Effects Off", "Effects On")))
        s.state("/camera", say(t, ctl, "/camera", ("amera",)))
        lines = [e["text"] for e in ctl.events_named("chat") if e.get("from") is None][-2:]
        ok(s, "Effects On" in lines[0], "effects were off, now on: %r" % lines[0])
        ok(s, lines[1] != first_camera, "/camera: %r after %r" % (lines[1], first_camera))
    with t.step("free camera off", "spec 2: with FreeCamera 0 the wheel stops at the 2008 limit") as s:
        held = zoom(t, s, ctl, "2008")
        ok(s, None not in held and held[1] < held[0], "the wheel zooms in: %r" % (held,))
        ok(s, held[2] < free[2], "zoomed out as far as it goes: %s cells with the option off, %s with it on"
           % (held[2], free[2]))
    t.stop_client()
