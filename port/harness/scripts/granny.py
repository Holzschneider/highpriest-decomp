"""M12.3a Granny models in the client: the guild flags around the Prontera
fountain, the Emperium and the castle guardians drawn as their 3D models.

Done when (checklist M12.3a; implementation plan M12: "a Granny model is
drawn in a War of Emperium castle frame"): with the script's own account in
Prontera at the fountain, the five guild flags (job 722, `Guildflag90_1.gr2`;
the server's InsideFlagsPR1..5, e.g. "Kriemhild" at 155,190) are drawn as
flags, not as the placeholder Poring: frames, and no `res.missing` trace line
for a `.gr2` name; the three guardians (1285-1287) spawned by a GM bot
(set-up) are drawn; the Emperium is drawn and animated: two frames of its
region taken seconds apart differ. Killing the spawned monsters (set-up,
@killmonster2) leaves the Granny corpses (CGrannyCorpse3d), framed too. At
the end the client's counters say the adapter read the files
(`granny.files`) and refused or failed nothing.

The Emperium. rAthena refuses `@monster 1288` ("Monster 'Emperium' cannot
be spawned", atcommand.cpp), so it is shown twice: in Prontera the bot
spawns the Dream Metal (1846), whose client job name is the same
`Empelium90_0.gr2` (Session.cpp, JT_DREAMMETAL), and then the real Emperium
in its castle: the bot starts the War of Emperium (`@agitstart`; the castle
script spawns 1288 at prtg_cas01 197,197), goes there and calls the
character in (`@recall`); `@agitend` ends it again. That is a minute of War
of Emperium on the shared server (a broadcast to every player, castles in
GvG); `--param woe=0` leaves the castle part out.

The 2008 client draws the guild flag in its rest pose: its model gets bone
type 1 from its name (`_1`), and C3dGrannyBoneRes::GetAnimation loads no
animation for type 1 (reference executable 0x436490: `if (param_1 != 1)
AddBone(...)`; WinMain preloads the flag's animation as type 5, which no
model uses). The port keeps that, so the flags' two frames are recorded,
not judged. The Emperium (bone type 0) plays its own 9.9 s idle animation,
the guardians (7, 8, 9) their 3dmob_bone files.

Set-up: the account bfhgranny / character BfhGranny (created on first use by
bifrost-bot, which then logs out), placed at prontera 155,184 by SQL. The GM
bot bfhgrannygm / BfhGrannyGM spawns the monsters a few cells west of the
character. Frames under <game folder>/test/frames/granny/, checksums in the
manifest. Compare by eye with any 2008-era screenshot under
<game folder>/test/reference/ that shows a guild flag.
"""
import re
import struct
import time
import zlib

from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home

NAME = "granny"
MILESTONE = "M12"
DESCRIPTION = "guild flags, Emperium and guardians drawn as Granny models"
MANIFEST = "port/harness/manifests/m12-granny.sha256"
NEEDS_SERVER = True

USER, PASSWORD, CHAR = "bfhgranny", "bfgranny", "BfhGranny"
GM_USER, GM_CHAR = "bfhgrannygm", "BfhGrannyGM"
START = ("prontera", 155, 184)
FLAG_JOB = 722
FLAGS = [(155, 190), (146, 194), (165, 194), (143, 203), (167, 203)]   # the server's npc/guild/prtg_cas0*.txt
EMPERIUM = 1288
DREAM_METAL = 1846          # the Emperium's model (Empelium90_0.gr2); @monster refuses 1288
CASTLE = ("prtg_cas01", 197, 197)   # agit_main.txt: where OnAgitStart spawns the Emperium
CASTLE_CELL = (197, 190)            # where the bot stands and calls the character
GUARDIANS = {1285: "archer guardian", 1286: "knight guardian", 1287: "soldier guardian"}
SPAWN_CELL = (149, 184)     # where the GM bot stands to spawn: 6 cells west


# ---- frames: a PNG reader (standard library only) and region differences

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


def region(rec, screen, up=150, side=50, down=20):
    """The device-pixel box above an actor's feet (logical screen position)."""
    k = rec.get("scale") or 1
    x0, x1 = int((screen["x"] - side) * k), int((screen["x"] + side) * k)
    y0, y1 = int((screen["y"] - up) * k), int((screen["y"] + down) * k)
    return max(0, x0), max(0, y0), min(rec["width"], x1), min(rec["height"], y1)


def region_difference(rec_a, rec_b, box):
    """Share of pixels in box whose colour differs by more than 24 in a channel."""
    wa, ha, ca, ra = read_png(rec_a["path"])
    wb, hb, cb, rb = read_png(rec_b["path"])
    x0, y0, x1, y1 = box
    changed = total = 0
    for y in range(y0, min(y1, ha, hb)):
        la, lb = ra[y], rb[y]
        for x in range(x0, min(x1, wa, wb)):
            total += 1
            if max(abs(la[x * ca + j] - lb[x * cb + j]) for j in range(3)) > 24:
                changed += 1
    return changed / total if total else 0.0


def ok(s, cond, message):
    """A check kept in the report whether it passes or not."""
    s.check(cond, message, passed=bool(cond))


def on_screen(ctl, job):
    return [a for a in ctl.state("actors")["actors"] if a["job"] == job and a["screen"]]


# ---- set-up

def own_character(t, s):
    """The script's account and character; the character is made by
    bifrost-bot on first use (it creates a missing character by name)."""
    if t.mock:
        s.note("mock: the mock's own account")
        return None
    t.accounts.create_account(USER, PASSWORD)
    if not t.logs.char(CHAR):
        maker = Bot.start(t, "granny-maker", USER, PASSWORD, CHAR)
        s.state("character made by bifrost-bot", maker.entered)
        maker.quit()
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and (t.logs.char(CHAR) or {}).get("online"):
            time.sleep(0.5)
    c = t.accounts.set_char(CHAR, map=START[0], x=START[1], y=START[2])
    s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})
    return c


def spawn(t, bot, s, mob, count=1):
    if bot is None:
        s.note("mock: no bot; the mock spawns nothing")
        return
    mark = t.logs.mark()
    bot.gm("@monster %d %d" % (mob, count))
    time.sleep(1.0)
    s.rows("atcommandlog: @monster %d" % mob,
           [r for r in t.logs.atcommands(account_id=bot.entered.get("aid"), after=mark)])


# ---- the steps

def flags(t, ctl):
    with t.step("guild flags drawn", "M12.3a: the guild flags at the fountain are Granny models, not the placeholder") as s:
        ctl.wait({"frames": 90}, timeout=30)
        fl = on_screen(ctl, FLAG_JOB)
        s.state("guild flags on screen (job %d)" % FLAG_JOB, fl)
        ok(s, len(fl) >= (1 if t.mock else 3), "%d guild flags on screen" % len(fl))
        rec_a = s.frame("flags")
        time.sleep(2.0)
        rec_b = s.frame("flags-2s")
        if t.mock:
            s.note("mock: synthetic frames, no models")
            return
        missing = [l for l in t.client.trace_lines("res.missing") if ".gr2" in l.lower()]
        s.trace("res.missing lines naming a .gr2", missing)
        ok(s, not missing, "no .gr2 reported missing (%d lines)" % len(missing))
        diffs = {"%s@%d,%d" % (a["name"], a["x"], a["y"]): round(region_difference(rec_a, rec_b, region(rec_a, a["screen"])), 4)
                 for a in fl}
        s.state("share of changed pixels above each flag, 2 s apart (recorded: the 2008 client does not animate the flag)", diffs)


def emperium_drawn(t, ctl, s, job, name):
    """The model on screen, framed twice 3 s apart; its region changed."""
    em = on_screen(ctl, job)
    s.state("%s on screen (job %d)" % (name, job), em)
    if t.mock:
        s.frame(name)
        s.note("mock: no %s, synthetic frames" % name)
        return
    ok(s, em, "the %s on screen" % name)
    rec_a = s.frame(name)
    time.sleep(3.0)
    rec_b = s.frame(name + "-3s")
    em2 = [a for a in on_screen(ctl, job) if a["id"] == em[0]["id"]]
    ok(s, em2 and em2[0]["screen"] == em[0]["screen"], "it did not move (an Emperium stands)")
    d = region_difference(rec_a, rec_b, region(rec_a, em[0]["screen"], up=110, side=45))
    s.state("share of changed pixels above the %s, 3 s apart" % name, round(d, 4))
    ok(s, d > 0.02, "the %s's region changed between the frames (%.1f %% of its pixels)" % (name, 100 * d))


def dream_metal(t, ctl, bot):
    with t.step("set-up: Dream Metal", "GM bot: @monster %d (the Emperium's model) near the character" % DREAM_METAL) as s:
        if bot is not None:
            s.state("bot", bot.warp(START[0], *SPAWN_CELL))
        spawn(t, bot, s, DREAM_METAL)
        ctl.wait({"frames": 60}, timeout=20)
    with t.step("Emperium model drawn and animated", "M12.3a: Empelium90_0.gr2 is drawn; two frames of it differ") as s:
        emperium_drawn(t, ctl, s, DREAM_METAL, "dreammetal")


def castle(t, ctl, bot):
    """The real Emperium, in its castle during the War of Emperium."""
    mp, ex, ey = CASTLE
    with t.step("set-up: War of Emperium", "GM bot: @agitstart, @warp %s %d %d, @recall %s" % (mp, CASTLE_CELL[0], CASTLE_CELL[1], CHAR)) as s:
        mark = t.logs.mark()
        bot.gm("@agitstart")
        time.sleep(3.0)
        s.state("bot in the castle", bot.warp(mp, *CASTLE_CELL, timeout=30))
        after = ctl.last_seq
        bot.gm("@recall %s" % CHAR)
        ev = ctl.wait_event("map", lambda e: e.get("map") == mp, after=after, timeout=90)
        s.state("map event", ev)
        ctl.wait({"map": mp, "moving": False}, timeout=30)
        ctl.wait({"frames": 90}, timeout=30)
        s.rows("atcommandlog: @agitstart, @warp, @recall",
               [r for r in t.logs.atcommands(account_id=bot.entered.get("aid"), after=mark)], min_rows=3)
    with t.step("Emperium drawn and animated in its castle", "M12: a Granny model drawn in a War of Emperium castle frame; two frames of it differ") as s:
        emperium_drawn(t, ctl, s, EMPERIUM, "emperium-castle")


def guardians(t, ctl, bot):
    with t.step("set-up: guardians", "GM bot: @monster %s" % ", ".join(str(j) for j in GUARDIANS)) as s:
        for job in GUARDIANS:
            spawn(t, bot, s, job)
        ctl.wait({"frames": 30}, timeout=20)
    with t.step("guardians drawn", "M12.3a: the knight, archer and soldier guardians are drawn") as s:
        seen = {}
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and len(seen) < len(GUARDIANS):
            for job in GUARDIANS:
                g = on_screen(ctl, job)
                if g:
                    seen[job] = g[0]
            time.sleep(0.5)
        s.state("guardians on screen", seen)
        s.frame("guardians")
        time.sleep(1.0)
        s.frame("guardians-1s")
        for job, what in GUARDIANS.items():
            ok(s, t.mock or job in seen, "the %s (%d) on screen" % (what, job))


def corpses(t, ctl, bot):
    with t.step("corpses", "set-up: @killmonster2; the Granny actors leave their corpses (CGrannyCorpse3d)") as s:
        if bot is not None:
            bot.gm("@killmonster2")
        time.sleep(1.5)
        s.frame("corpses")
        left = [a for a in ctl.state("actors")["actors"] if a["job"] == DREAM_METAL or a["job"] in GUARDIANS]
        s.state("Granny monsters still listed", left)
        ok(s, t.mock or not left, "the monsters are gone (%d left)" % len(left))


def run(t):
    with t.step("set-up: own character", "account %s, character %s at %s %d,%d (SQL, offline)" % ((USER, CHAR) + START)) as s:
        own_character(t, s)
    with t.step("set-up: GM bot", "bifrost-bot on a group-99 account") as s:
        bot = Bot.start(t, "granny", GM_USER, "bfbot", GM_CHAR, gm=True)
        s.note("mock: no bot" if bot is None else "bot entered: %r" % bot.entered)
        if bot is not None:
            s.state("bot at the spawn cell", bot.warp(START[0], *SPAWN_CELL))

    env = own_home(t)
    if not t.mock:
        env["BF_TRACE"] = "err,net,mode,stub,res"   # res: the res.missing lines and the counters at quit
    ctl = t.launch(env=env)
    login(t, ctl, USER if not t.mock else "bftest", PASSWORD if not t.mock else "bftest")
    st = enter_game(t, ctl)
    with t.step("at the fountain", "prontera, the character placed by set-up") as s:
        s.equal(st["map"]["name"], "prontera", "map")

    flags(t, ctl)
    dream_metal(t, ctl, bot)
    guardians(t, ctl, bot)
    corpses(t, ctl, bot)
    if bot is not None and t.param("woe", "1") != "0":
        try:
            castle(t, ctl, bot)
        finally:
            bot.gm("@agitend")

    client = t.client
    t.stop_client()
    if not t.mock:
        with t.step("adapter counters", "the client read the .gr2 files through the adapter and refused none") as s:
            lines = client.trace_lines("[counter]")
            ctr = {}
            for l in lines:
                m = re.search(r"\[counter\]\s+(\S+)\s+(-?\d+)", l)
                if m:
                    ctr[m.group(1)] = int(m.group(2))
            gr = {k: v for k, v in ctr.items() if k.startswith("granny") or k.startswith("res.")}
            s.state("counters", gr)
            ok(s, ctr.get("granny.files", 0) > 0, "granny.files = %d" % ctr.get("granny.files", 0))
            bad = {k: v for k, v in gr.items() if k.startswith("granny.") and k != "granny.files"}
            ok(s, not bad, "no adapter failure counted: %r" % bad)
