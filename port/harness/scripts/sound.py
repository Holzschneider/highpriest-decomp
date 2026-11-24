"""M7 sound: login music that loops, map music per map, attack and UI effects
with distance-scaled volume.

Done when (implementation plan, M7): the login music loops, map music changes
on map change, and attack and UI effects play with distance-scaled volume.
The check is the sound module's own counters of started voices and the
stream's file name; how it sounds is left for the owner's play test.

What is read: the control socket's `sound` part (the counters of
port/native/audio: mode, voices started, the stream's file, status, loops,
duration and position) and its `sound` event, one per voice the client
starts, with the file name and the distance PlayWave gave it and the volume
PlayDigital computed (0..1, Miles' 0..127 divided by 127).

Login music: the login screen plays bgm\\01.mp3 (LoginMode.cpp). Looping is
shown by waiting for one whole pass: `stream.loops` goes from 0 to at least 1
while the status stays `playing` and the same file stays open, and the time
waited is consistent with the file's duration (01.mp3 is about 75 s). The
loop count is the module's: passes rendered by the output, so this needs the
real device (in SILENT mode nothing is rendered and the step fails, saying so).

Map music: CGameMode::PlayBGM looks the map up in mp3nametable.txt (read here
from the game's archives, rdata.grf before data.grf as the client searches
them). prontera and prt_in share bgm\\08.mp3, so the map change is the
south gate, prontera 156,22 (prt001) to prt_fild08 170,375, whose music is
bgm\\12.mp3. Set-up (SQL, offline): the character stands at prontera 156,29.

Attack: a GM bot stands on the arrival cell and spawns a Poring there
(@monster, set-up). The client clicks it, once per attack as the 2008 client
wants, until it dies. Voices: the weapon's hit sound (CSession's
m_weaponHitWaveNameTable, _hit_sword.wav for the Novice's knife) and the
Poring's damage and die sounds from its ACT events (CRenderObject::
ProcessSound).

UI effect: the 2008 client plays the button sound (wav\\버튼소리.wav,
"button sound") from the login, server-list and character-selection windows'
OK commands (UIFrameWnd.cpp); the login above presses Enter in each.

Distance: PlayDigital gives a sound within 40 world units (8 cells) the full
effects volume, and v * 40 / d between 40 and 250, none beyond. The bot also
spawns Porings 15 cells away (75 units); they wander, and their move sounds
and the near Poring's sounds are the same files at different distances. A
file heard both near (d <= 40) and far (40 < d < 250) is lower far away, and
every Poring voice follows the rule: the full effects volume within 40 units,
trunc(full * 40 / d) beyond, in Miles' units.
"""

import os
import re
import time

from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home

NAME = "sound"
MILESTONE = "M7"
DESCRIPTION = "login music loops; map music follows mp3nametable; attack and UI sounds with distance-scaled volume"
MANIFEST = "port/harness/manifests/m7-sound.sha256"
NEEDS_SERVER = True

START = ("prontera", 156, 29)
# the warp as the server's npc/ defines it: name, map, x, y, span x, span y, to map, x, y
WARP = ("prt001", "prontera", 156, 22, 3, 2, "prt_fild08", 170, 375)
ARRIVAL = (170, 375)        # prt_fild08 from prt001; the bot spawns the Poring to attack here
FAR_CELL = (170, 360)       # the far Porings, 15 cells south
PORING = 1002
BUTTON = "버튼소리.wav"
MOCK_TABLE = {"prontera": "bgm\\08.mp3", "prt_in": "bgm\\08.mp3", "prt_fild08": "bgm\\12.mp3"}


def base(path):
    return re.split(r"[\\/]", path or "")[-1].lower()


def mp3_table(t):
    """mp3nametable.txt as the client reads it: map.rsw -> bgm\\NN.mp3."""
    if t.mock:
        return dict(MOCK_TABLE)
    import sys
    from bfharness import paths
    sys.path.insert(0, str(paths.REPO / "port" / "tools"))
    import grfaudit
    game = os.environ.get("BF_CLIENT_DIR") or os.path.expanduser("~/Games/Bifrost")
    for arc in ("rdata.grf", "data.grf"):
        path = os.path.join(game, arc)
        if not os.path.exists(path):
            continue
        files = grfaudit.read_table(path)[2]
        for k, e in files.items():
            if k.lower() == b"data\\mp3nametable.txt":
                text = grfaudit.read_file(e).decode("cp949", "replace")
                out = {}
                for line in text.splitlines():
                    parts = line.split("#")
                    if len(parts) >= 2 and parts[0].lower().endswith(".rsw"):
                        out[parts[0].lower()[:-4]] = parts[1].replace("\\\\", "\\")
                return out
    raise AssertionError("no mp3nametable.txt in %s" % game)


def ok(s, cond, message):
    """A check that is kept in the report whether it passes or not."""
    s.check(cond, message, passed=bool(cond))


def sound(ctl):
    return ctl.state("sound")["sound"]


def miles(v):
    """A voice's volume in Miles' 0..127, as PlayDigital set it."""
    return int(round(v * 127))


def voices(ctl, after, pred=None):
    return [e for e in ctl.events_named("sound", after) if pred is None or pred(e)]


def login_music(t, ctl):
    with t.step("login music", "M7: the login screen plays bgm\\01.mp3") as s:
        ctl.wait_mode("login", 60)
        st = sound(ctl)
        s.state("sound at the login screen", st)
        s.note("audio mode: %s" % st["mode"])
        s.equal(base(st["stream"]["file"]), "01.mp3", "stream file")
        s.equal(st["stream"]["status"], "playing", "stream status")
        ok(s, st["stream"]["duration"] > 0, "the stream has a duration")
    with t.step("login music loops", "M7: the login music loops (stream_loops past one pass)") as s:
        st = sound(ctl)
        dur = st["stream"]["duration"]
        first = st["stream"]["position"]
        t0 = time.monotonic()
        samples = [(0.0, st["stream"]["position"], st["stream"]["loops"])]
        limit = (dur - first) + 20 if not t.mock else 30
        while time.monotonic() - t0 < limit:
            time.sleep(1.0 if t.mock else 3.0)
            st = sound(ctl)
            samples.append((round(time.monotonic() - t0, 1), round(st["stream"]["position"], 2), st["stream"]["loops"]))
            if st["stream"]["loops"] >= 1 and st["stream"]["position"] > 1.0:
                break
        waited = time.monotonic() - t0
        s.state("(seconds waited, position, loops)", samples)
        s.state("sound after the wait", st)
        if st["mode"] == "silent":
            s.note("SILENT mode: the module renders nothing, so it counts no pass")
        ok(s, st["stream"]["loops"] >= 1, "loops %d after %.0f s (one pass is %.1f s, %.1f s of it played at the start)"
                % (st["stream"]["loops"], waited, dur, first))
        s.equal(base(st["stream"]["file"]), "01.mp3", "still the same file")
        s.equal(st["stream"]["status"], "playing", "still playing after the end of the file (loop count 0: forever)")
        ok(s, waited >= (dur - first) - 3, "the pass ended no earlier than the file's length allows (waited %.1f s, %.1f s left at the start)"
                % (waited, dur - first))
        ok(s, st["stream"]["position"] < dur, "playing the next pass: %.1f s into it" % st["stream"]["position"])


def button_sounds(t, s, ctl, after):
    vs = voices(ctl, after, lambda e: base(e.get("file")) == BUTTON)
    s.state("button sounds", vs)
    ok(s, vs and all(v["started"] for v in vs), "the button sound started (%d)" % len(vs))
    if vs:
        v = vs[0]
        s.equal(v["distance"], 0, "distance (a UI sound is at the listener)")
    return vs


def through_portal(t, ctl, table, char_id):
    name, mp, wx, wy, sx, sy, dm, dx, dy = WARP
    with t.step("map music in %s" % mp, "M7: map music from mp3nametable (%s.rsw)" % mp) as s:
        st = ctl.state("map", "sound")
        s.state("sound", st["sound"])
        s.equal(st["map"]["name"], mp, "map")
        s.equal(base(st["sound"]["stream"]["file"]), base(table[mp]), "stream file: %s" % table[mp])
        s.equal(st["sound"]["stream"]["status"], "playing", "stream status")
    with t.step("walk into %s" % name, "the map change by the south gate, %s to %s" % (mp, dm)) as s:
        # the server's warp is a 3x2 area around 156,22 (the mock's is a cell at 156,18)
        portals = [a for a in ctl.state("actors")["actors"] if a["type"] == "portal" and a["screen"]
                   and abs(a["x"] - wx) <= sx + 4 and abs(a["y"] - wy) <= sy + 4]
        portals.sort(key=lambda a: abs(a["x"] - wx) + abs(a["y"] - wy))
        s.state("portal", portals[:1])
        ok(s, portals, "the portal %s on screen" % name)
        p = portals[0]
        after = ctl.last_seq
        ctl.move(p["screen"]["x"], p["screen"]["y"])
        ctl.wait({"frames": 3}, timeout=10)
        ctl.click(p["screen"]["x"], p["screen"]["y"])
        ev = ctl.wait_event("map", lambda e: e.get("map") == dm, after=after, timeout=90)
        s.state("map event", ev)
        ctl.wait({"map": dm, "moving": False}, timeout=30)
    with t.step("map music in %s" % dm, "M7: map music changes on map change (%s.rsw)" % dm) as s:
        ok(s, base(table[dm]) != base(table[mp]), "the two maps have different music: %s, %s" % (table[mp], table[dm]))
        ctl.wait({"frames": 30}, timeout=20)
        st = sound(ctl)
        s.state("sound", st)
        s.equal(base(st["stream"]["file"]), base(table[dm]), "stream file: %s" % table[dm])
        s.equal(st["stream"]["status"], "playing", "stream status")
        s.frame("%s-arrived" % dm)


def attack(t, ctl, bot):
    with t.step("set-up: Porings near and far", "bot by atcommand: @warp, @monster at %d,%d and at %d,%d"
                % (ARRIVAL + FAR_CELL)) as s:
        if bot is None:
            s.note("mock: the Porings of the mock's prt_fild08")
        else:
            mark = t.logs.mark()
            s.state("bot at the far cell", bot.warp("prt_fild08", *FAR_CELL))
            bot.gm("@monster 1002 3")
            time.sleep(1.0)
            s.state("bot at the near cell", bot.warp("prt_fild08", *ARRIVAL))
            bot.gm("@monster 1002 1")
            time.sleep(1.0)
            s.rows("atcommandlog: @warp and @monster of the bot",
                   t.logs.atcommands(account_id=bot.entered.get("aid"), after=mark))
        ctl.wait({"frames": 30}, timeout=20)
    with t.step("attack a Poring", "M7: attack effects play: voices started for the attack's hit and the Poring") as s:
        me = ctl.state("self")["self"]
        near = lambda a: abs(a["x"] - me["x"]) + abs(a["y"] - me["y"])
        mons = sorted([a for a in ctl.state("actors")["actors"] if a["type"] == "monster" and a["job"] == PORING
                       and a["screen"]], key=near)
        s.state("Porings on screen, nearest first", mons[:6])
        ok(s, mons, "a Poring on screen")
        target = mons[0]
        after = ctl.last_seq
        before = sound(ctl)["voices_started"]
        # the 2008 client attacks once per click (continuous attack is
        # Ctrl or the /noctrl option), so it clicks until the Poring is gone
        deadline = time.monotonic() + 90
        clicks = 0
        gone = False
        while time.monotonic() < deadline:
            a = [x for x in ctl.state("actors")["actors"] if x["id"] == target["id"]]
            if not a:
                gone = True
                break
            if a[0]["screen"]:
                ctl.move(a[0]["screen"]["x"], a[0]["screen"]["y"])
                ctl.wait({"frames": 3}, timeout=10)
                ctl.click(a[0]["screen"]["x"], a[0]["screen"]["y"])
                clicks += 1
            time.sleep(1.0 if not t.mock else 0.2)
        st = sound(ctl)
        vs = voices(ctl, after)
        s.state("voices since the first click: (file, volume 0..127, distance, started)",
                [(v["file"], miles(v["volume"]), v["distance"], v["started"]) for v in vs][-60:])
        s.note("%d clicks; the Poring %s" % (clicks, "died" if gone else "was still there after 90 s"))
        ok(s, st["voices_started"] > before, "voices_started %d -> %d" % (before, st["voices_started"]))
        hit_v = [v for v in vs if v["started"] and re.match(r"(_hit_|attack_)", base(v["file"]))]
        mon_v = [v for v in vs if v["started"] and re.match(r"poring_(damage|die)", base(v["file"]))]
        ok(s, hit_v, "the attack's sound started: %s" % sorted(set(base(v["file"]) for v in hit_v)))
        ok(s, mon_v, "the Poring's damage or die sound started: %s" % sorted(set(base(v["file"]) for v in mon_v)))
        ok(s, gone, "the Poring died")
        s.frame("attacked")
        return after


def distance(t, ctl, after):
    with t.step("distance-scaled volume", "M7: a farther sound plays at a lower volume (PlayDigital: v * 40 / d)") as s:
        deadline = time.monotonic() + (20 if t.mock else 90)
        pair = None
        while time.monotonic() < deadline:
            vs = voices(ctl, after, lambda e: e["started"] and e["distance"] is not None
                        and base(e["file"]).startswith("poring_"))
            near = [v for v in vs if v["distance"] <= 40]
            far = [v for v in vs if 40 < v["distance"] < 250]
            pairs = [(n, f) for n in near for f in far if base(n["file"]) == base(f["file"])]
            if pairs:
                pair = pairs[0]
                break
            time.sleep(2.0)
        s.state("Poring voices: (file, volume 0..127, distance)",
                [(base(v["file"]), miles(v["volume"]), round(v["distance"], 1)) for v in vs][-60:])
        ok(s, pair, "one Poring sound heard both within 40 units and between 40 and 250")
        n, f = pair
        full = miles(n["volume"])
        s.state("the pair", {"near": n, "far": f})
        ok(s, miles(f["volume"]) < full, "%s: far %d (at %.1f) < near %d (at %.1f)"
                % (base(f["file"]), miles(f["volume"]), f["distance"], full, n["distance"]))
        # every Poring voice against the rule, with the near volume as the full volume
        wrong = []
        for v in vs:
            d = v["distance"]
            expect = full if d <= 40 else int(full * 40.0 / d)
            if abs(miles(v["volume"]) - expect) > 1:
                wrong.append((base(v["file"]), miles(v["volume"]), expect, round(d, 1)))
        ok(s, not wrong, "all %d Poring voices: volume = %d within 40 units, trunc(%d * 40 / d) beyond "
                "(within 1: the client computes in float); off: %s" % (len(vs), full, full, wrong[:5]))


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")

    with t.step("set-up: the music table", "mp3nametable.txt from the game's archives") as s:
        table = mp3_table(t)
        s.state("mp3nametable", {m: table.get(m) for m in ("prontera", "prt_in", "prt_fild08")})

    ctl = t.launch(env=own_home(t))
    login_music(t, ctl)
    # placed only now, after the long wait at the login screen: other scripts
    # on the shared server place the same character
    with t.step("set-up: place character", "SQL, offline: %s %d,%d" % START) as s:
        c = t.accounts.set_char(name, map=START[0], x=START[1], y=START[2])
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})
        char_id = c["char_id"]
    after = ctl.last_seq
    login(t, ctl, user, password)
    enter_game(t, ctl)
    with t.step("UI effect", "M7: UI effects play: the button sound of the login windows") as s:
        button_sounds(t, s, ctl, after)
    ctl.wait({"frames": 60}, timeout=20)
    through_portal(t, ctl, table, char_id)

    with t.step("set-up: GM bot", "bifrost-bot on a group-99 account") as s:
        bot = Bot.start(t, "sound", "bfhsoundgm", "bfbot", "BfhSoundGM", gm=True)
        s.note("mock: no bot" if bot is None else "bot entered: %r" % bot.entered)
    after = attack(t, ctl, bot)
    distance(t, ctl, after)

    with t.step("counters at the end", "the module's counters") as s:
        st = sound(ctl)
        s.state("sound", st)
        s.note("audio mode %s: %d voices started, %d refused (a sound that cannot be decoded is refused), "
               "%d decode failures; %d streams opened, %d failed"
               % (st["mode"], st["voices_started"], st["voices_rejected"], st["decode_failures"],
                  st["stream"]["opened"], st["stream"]["failures"]))
        ok(s, st["stream"]["failures"] == 0, "no stream failed to open")
    if bot is not None:
        bot.gm("@killmonster2")
    t.stop_client()
