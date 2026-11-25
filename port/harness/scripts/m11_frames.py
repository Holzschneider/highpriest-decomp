"""M11.0c reference frames: one replayed session, captured before M11 changes src/.

Done when (checklist M11.0c): a reproducible set of frames is captured at the
base commit of M11, a comparison script with its tolerance is written down
(port/harness/framecmp.py, whose docstring is the definition of "match"), two
captures at the base commit match each other, and a planted renderer fault
makes the right frames fail. After every M11 step this script is run again
and its last step compares the new capture with the reference.

    BF_CLIENT=<client> python3 port/harness/run.py m11_frames            # replay, capture, compare
    BF_CLIENT=<client> python3 port/harness/run.py m11_frames --param record=1   # re-record the session (server)
    python3 port/harness/run.py m11_frames --mock

How the frames are made reproducible. The session is recorded once against
the test server (record=1: a fresh account and character, SQL set-up, the
same input as below, BF_PKT_LOG kept as the fixture
port/harness/fixtures/m11-session.pktlog with m11-session.json naming the
account). Every capture run replays that log (BF_PKT_REPLAY, the client
opens no socket): no server is needed, no other player walks into the
picture, and every scene gets the same packets. The input is the same
socket input every time, the window sizes are set by `resize`, and the
pointer rests on fixed logical points. The run's settings fix the camera
on the character (/camera: an easing camera ends on a float that depends
on the step at which the character's position arrived) and turn the
weather off (/effect off at map entry: random particles; /effect turns the
effects on again for the potion); the scenes stand away from Prontera's
chimney smoke where they can, and nothing attacks (a walk cut short by an
attack ends on a position that depends on timing). What still moves by itself (the edit
caret on timeGetTime()/500, the game cursor's animation and the cell marker,
animated sprites, water, effects, floating damage numbers) is found, not
guessed: every frame is captured three times, 400 ms apart (<name>.png,
<name>.s1.png, <name>.s2.png), and framecmp's reference masks every pixel
that differs among the captures of two runs (framecmp.py, points 2-4).

The frames (logical coordinates; 2048x1536 device pixels at UI scale 2,
i.e. 1024x768 logical, unless named otherwise):

| Frame | Scene | Checks in M11 |
|---|---|---|
| login-s2 | login screen, UI scale 2 | UI pass: point sampling, half-pixel offset, colour key, wallpaper tiles |
| login-s1 | login screen, 1024x768 px at UI scale 1 | the 2008 look; scale handling |
| login-typed | account typed, password masked | GDI text, caret (masked), edit fields |
| servers | character server list | list window, text |
| charselect | character selection, slot 0 holding the character | character sprite with equipment, window art |
| makechar, makechar-typed | creation window, a Latin-1 name typed (never sent) | stat hexagon, text input in Windows-1252 |
| prontera | game view on prontera 123,98, pointer over the ground | ground, models, lightmaps, sprites, shadows, NPCs, cursor over the world |
| chat-latin1 | a Latin-1 line typed into the chat line (the English 2008 client refuses to send it: MSI_ENGLISHONLY) | glyphs of code page 1252 |
| chat | the refusal, then an ASCII line said (chat window, balloon, the character's Latin-1 name) | chat window alpha, text |
| status, inventory, equipment, options | Alt+A, Alt+E, Alt+Q, Alt+O | window art, item icons, text, equipment sprite |
| npc-dialogue, npc-menu | Merchant#pron (prontera 123,102): text, Next, the menu | dialogue window, menu |
| indoor | @warp prt_in 126,72 (a map change) | an indoor map, the Tool Dealer |
| shop-dealtype, shop-buy | Tool Dealer#prt1 (prt_in 126,76): Buy/Sell, the shop list | deal windows, item icons |
| world-wide | 3200x1400 px at UI scale 2 | window and projection resize, wide aspect |
| world-s1 | 2048x1536 px at UI scale 1 | UI scale 1 in game |
| skills | Alt+S | skill window, skill icons |
| potion | a Red Potion double-clicked in the item window (the character does not move) | the item-use effect |
| field | @warp to FIELD | another map's ground, models and water |
| readback | Ctrl+F13 (Ctrl+Print Screen): the client's own BMP of the screen, converted to PNG | the render-target read-back (CMode::ScreenShot) |

Not captured: the map-loading screen (the 2008 client picks its picture
with rand() seeded from timeGetTime(), Session.cpp) and the fade of a map
change (it lasts a few frames after the load; a replay delivers the map's
packets at a moment that depends on timing, so its frame cannot be hit
reproducibly). Their code paths are the ones the frames above draw through.

Steps after the capture: "compare with the reference" runs framecmp against
<game folder>/test/reference/m11/ (--param reference=<dir>; --param
compare=0 skips it). The reference is built from two capture runs at the
base commit:

    python3 port/harness/framecmp.py reference --commit <sha> \
        --out <game folder>/test/reference/m11 <run 1 frames dir> <run 2 frames dir>
"""

import json
import os
import shutil
import subprocess
import time
from pathlib import Path

from bfharness import npcplay, paths
from bfharness.frames import Frames, sha256
from bfharness.play import own_home

NAME = "m11"
MILESTONE = "M11"
DESCRIPTION = "M11 reference frames: login, selection, creation, world, windows, NPC, shop, field, wide, read-back"
MANIFEST = "port/harness/manifests/m11-frames.sha256"
NEEDS_SERVER = False

FIXTURE = paths.HARNESS_DIR / "fixtures" / "m11-session.pktlog"
SESSION = paths.HARNESS_DIR / "fixtures" / "m11-session.json"
MULTI = {"BF_MULTI_INSTANCE": "1"}

SIZE = (2048, 1536, 2)              # device pixels, UI scale: 1024 x 768 logical
SAMPLES, SAMPLE_GAP = 2, 0.4        # captures after the first, seconds apart
REST = (48, 40)                     # logical: where the pointer rests for UI frames
GROUND = (660, 330)                 # logical: over the ground, away from windows and actors
# No map effect (chimney smoke, rsw object type 44) within sight of START; isEffectOn 0 at start keeps
# the weather off (CWeather::ScriptProcess at map entry); /effect turns effects on for the potion.
START = ("prontera", 123, 98)
MERCHANT = (123, 102)                    # Merchant#pron, npc/merchants/novice_exchange.txt
INDOOR = ("prt_in", 126, 72)
DEALER = (126, 76)                       # Tool Dealer#prt1, a shop
FIELD = ("prt_fild05", 114, 156)        # a walkable cell south of the river (water level 14 in the .rsw)
FIRST_AID, RED_POTION = 142, 501
ITEMS = [(RED_POTION, 10), (909, 7), (512, 3)]
WORN = [(1201, 2), (2301, 16), (2220, 256)]   # Knife (right hand), Cotton Shirt (armour), Hat (upper head)
ZENY = 5000
CHAT_LINE = "Grüße aus Prontera – naïve café, 5 € ¿sí?"
CHAT_ASCII = "Hello from Prontera, 2008 style!"
NEW_NAME = "Jürgen"            # typed into the creation window, never sent

WINDOWS = [("a", "STATUSWND", "status"), ("e", "ITEMWND", "inventory"),
           ("q", "EQUIPWND", "equipment"), ("o", "OPTIONWND", "options")]


def reference_dir(t):
    return Path(t.param("reference") or paths.client_dir() / "test" / "reference" / "m11")


def regions_of(ctl, scale=None):
    """What framecmp needs to tell the strict region from the world: the
    mode, the UI scale and the open windows' rectangles (logical)."""
    st = ctl.state("mode", "view", "windows")
    v = st.get("view") or {}
    return {"mode": st.get("mode"), "scale": scale or v.get("scale"), "logical": v.get("logical"),
            "windows": [w["rect"] for w in st.get("windows", [])]}


def write_regions(t, name, info):
    t.frames.path(name).with_suffix(".json").write_text(json.dumps(info) + "\n")


def shot(t, s, name, rest=None):
    """The frame and its samples, SAMPLE_GAP apart; <name>.json beside it
    names the mode and the windows (framecmp's regions)."""
    ctl = t.ctl
    if rest:
        ctl.move(*rest)
    ctl.wait({"frames": 10}, timeout=20)
    info = regions_of(ctl)
    rec = s.frame(name)
    write_regions(t, name, info)
    for k in range(1, SAMPLES + 1):
        time.sleep(SAMPLE_GAP)
        t.frames.capture(ctl, "%s.s%d" % (name, k))
    return rec


def at(cell):
    return lambda a: (a["x"], a["y"]) == tuple(cell) and a["type"] in ("npc", "monster")


def wait_actor(ctl, pred, what, timeout=20):
    deadline = time.monotonic() + timeout
    while True:
        a = ctl.actor(pred, learn_names=False)
        if a and a.get("screen"):
            return a
        if time.monotonic() > deadline:
            raise AssertionError("no %s on screen" % what)
        ctl.wait({"frames": 10}, timeout=10)


def setup_record(t, s):
    """A fresh account and character for the recording (SQL, offline)."""
    from bfharness.accounts import unique_name
    t.params.setdefault("char", "Zo\u00eb" + unique_name(tag="")[-6:])   # Windows-1252 in a name
    user, password, c = npcplay.new_player(t, "m11", *START, zeny=ZENY, items=ITEMS, skills=[(FIRST_AID, 1)])
    mock = getattr(t.db, "db", None)
    for nameid, equip in WORN:
        if mock is not None:
            mock.put("inventory", id=max([r["id"] for r in mock.rows("inventory")] or [0]) + 1,
                     char_id=c["char_id"], nameid=nameid, amount=1, equip=equip)
            continue
        t.db.execute("INSERT INTO `inventory` (`char_id`, `nameid`, `amount`, `identify`, `equip`) "
                     "VALUES (%d, %d, 1, 1, %d)" % (c["char_id"], nameid, equip))
    if not t.mock:
        # hp below max_hp: the potion heals
        t.db.execute("UPDATE `char` SET `weapon` = 1, `head_top` = 16, `hp` = 20 WHERE `char_id` = %d" % c["char_id"])
    s.state("char row", {k: c.get(k) for k in ("char_id", "name", "zeny", "last_map", "last_x", "last_y")})
    return {"user": user, "password": password, "char": c["name"], "char_id": c["char_id"]}


def client_options(home):
    """Client/Option in the run's settings: g_isFixedCamera 1 (the 2008
    client's /camera): the view sits on the character instead of easing
    after it (CView::InterpolateViewInfo), so the view does not depend on
    the step at which the character's position arrived; isEffectOn 0 (the
    client's /effect off): no weather particles, which are random
    (CWeather::ScriptProcess runs at map entry only, so /effect later turns
    effects on without them)."""
    import plistlib
    folder = home / "Library" / "Application Support" / paths.app_name()
    plist = folder / "Settings.plist"
    with open(plist, "rb") as f:
        values = plistlib.load(f)
    client = values.setdefault(next((k for k in values if k.lower() == "client"), "Client"), {})
    option = client.setdefault(next((k for k in client if k.lower() == "option"), "Option"), {})
    option["g_isFixedCamera"] = 1
    option["isEffectOn"] = 0
    with open(plist, "wb") as f:
        plistlib.dump(values, f)


def git_head():
    try:
        return subprocess.run(["git", "-C", str(paths.REPO), "rev-parse", "--short", "HEAD"],
                              capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def run(t):
    global MANIFEST
    record = t.param("record") == "1"
    replay = not t.mock and not record

    with t.step("session", "the recorded session (replay) or a fresh one (record, mock)") as s:
        if replay:
            s.check(FIXTURE.exists() and SESSION.exists(), "fixture %s and %s" % (FIXTURE.name, SESSION.name))
            session = json.loads(SESSION.read_text())
            env = dict(MULTI, BF_PKT_REPLAY=str(FIXTURE), BF_PKT_REPLAY_WAIT=t.param("wait", "60000"),
                       BF_PKT_REPLAY_HOLD="-1")
            s.note("replaying %s (recorded %s at %s)" % (FIXTURE.name, session.get("recorded"), session.get("commit")))
        else:
            if record:
                s.note("server: %s" % t.server.ensure_up())
                MANIFEST = None                       # the recording's frames are not the reference
                t.frames = Frames(NAME + "-record")
            session = setup_record(t, s)
            env = dict(MULTI)
        s.state("session", {k: session[k] for k in ("user", "char")})
    env.update(own_home(t))
    if "BF_HOME" in env:
        client_options(Path(env["BF_HOME"]))
    ctl = t.launch(env=env)

    with t.step("login screen", "UI scale 2 and 1") as s:
        ctl.wait_mode("login", 60)
        ctl.wait({"frames": 30}, timeout=30)
        for name, (w, h, sc) in (("login-s2", SIZE), ("login-s1", (1024, 768, 1))):
            v = ctl.resize(width=w, height=h, scale=sc).get("view") or {}
            s.equal((v.get("width"), v.get("height"), v.get("scale")), (w, h, sc), "view")
            shot(t, s, name, REST)
        ctl.resize(width=SIZE[0], height=SIZE[1], scale=SIZE[2])

    with t.step("login typed", "account and masked password in the edit fields") as s:
        ctl.type_text(session["user"])
        ctl.press("tab")
        ctl.type_text(session["password"])
        shot(t, s, "login-typed", REST)

    with t.step("server list and character selection", "the list window, then the slots") as s:
        after = ctl.last_seq
        ctl.press("enter")
        ev = ctl.wait_event("mode", lambda e: e.get("mode") in ("servers", "charselect", "login"),
                            after=after, timeout=60)
        s.check(ev["mode"] != "login", "not refused: %r" % ev)
        if ev["mode"] == "servers":
            ctl.wait_window("SELECTSERVERWND", timeout=10)
            shot(t, s, "servers", REST)
            ctl.press("enter")
        else:
            s.note("no server list (mock)")
        ctl.wait_mode("charselect", 60)
        ctl.wait_window("SELECTCHARWND", timeout=10)
        ctl.wait({"frames": 30}, timeout=20)
        shot(t, s, "charselect", REST)

    with t.step("character creation", "the empty slot's creation window, a Latin-1 name typed, cancelled") as s:
        ctl.press("right")
        ctl.wait({"frames": 5}, timeout=10)
        ctl.press("enter")
        ctl.wait_mode("makechar", 30)
        ctl.wait_window("MAKECHARWND")
        shot(t, s, "makechar", REST)
        ctl.type_text(NEW_NAME)
        texts = {c["id"]: c["text"] for c in ctl.window("MAKECHARWND").get("controls", [])}
        if "name_edit" in texts:
            s.equal(texts["name_edit"], NEW_NAME, "name field")
        shot(t, s, "makechar-typed", REST)
        try:
            ctl.click_control("MAKECHARWND", "cancel")
        except Exception:
            ctl.press("escape")
        ctl.wait_mode("charselect", 30)
        ctl.wait({"frames": 5}, timeout=10)
        ctl.press("left")
        ctl.wait({"frames": 5}, timeout=10)

    with t.step("prontera", "the world, the cursor over the ground") as s:
        after = ctl.last_seq
        ctl.press("enter")
        ctl.wait_mode("game", 120)
        ev = ctl.wait_event("map", after=after, timeout=60)
        s.state("map event", ev)
        ctl.wait({"frames": 90}, timeout=30)
        if not t.mock:
            wait_actor(ctl, at(MERCHANT), "Merchant")
        s.state("self", ctl.state("self")["self"])
        shot(t, s, "prontera", GROUND)
        s.state("camera", ctl.state("camera").get("camera"))
    if t.param("until") == "prontera":
        t.stop_client()
        return

    with t.step("chat", "a Latin-1 line typed (the 2008 English client refuses to send it), an ASCII line said") as s:
        npcplay.focus_chat(ctl)
        ctl.type_text(CHAT_LINE)
        shot(t, s, "chat-latin1", GROUND)
        after = ctl.last_seq
        ctl.press("enter")
        if not t.mock:
            # UIFrameWnd (ID_OK of the chat window): g_session.IsOnlyEnglish, MSI_ENGLISHONLY
            ev = ctl.wait_event("chat", lambda e: "English" in (e.get("text") or ""), after=after, timeout=15)
            s.state("refusal", ev)
        w = ctl.window("CHATWND")
        left = {c["id"]: c["text"] for c in (w or {}).get("controls", [])}.get("chat_edit") or ""
        for _ in range(len(left)):
            ctl.press("backspace")
        after = ctl.last_seq
        ctl.say(CHAT_ASCII)
        ev = ctl.wait_event("chat", lambda e: CHAT_ASCII in (e.get("text") or ""), after=after, timeout=15)
        s.state("chat event", ev)
        shot(t, s, "chat", GROUND)

    with t.step("windows", "status, item, equipment and option windows") as s:
        for key, wname, frame in WINDOWS:
            ctl.press(key, "alt")
            ctl.wait_window(wname, timeout=10)
            shot(t, s, frame, REST)
            ctl.press(key, "alt")
            ctl.wait_window(wname, open=False, timeout=10)

    if not t.mock:
        skills_and_potion(t)
        npc(t)

    with t.step("window sizes", "a wide window, UI scale 1 in game") as s:
        for name, (w, h, sc) in (("world-wide", (3200, 1400, 2)), ("world-s1", (2048, 1536, 1))):
            v = ctl.resize(width=w, height=h, scale=sc).get("view") or {}
            s.equal((v.get("width"), v.get("height"), v.get("scale")), (w, h, sc), "view")
            ctl.wait({"frames": 30}, timeout=20)
            shot(t, s, name, REST)
        ctl.resize(width=SIZE[0], height=SIZE[1], scale=SIZE[2])
        ctl.wait({"frames": 30}, timeout=20)

    if not t.mock:
        indoor_and_shop(t)
        field(t)
    readback(t)

    with t.step("quit", "the client quits on request") as s:
        s.state("quit", ctl.quit())
    t.stop_client()
    if record:
        with t.step("fixture", "the recorded packet log becomes the fixture") as s:
            dropped = clean_fixture(t.run_dir / "client-packets.txt", FIXTURE)
            s.note("keep-alives and name requests left out: %d" % dropped)
            session.update(recorded=time.strftime("%Y-%m-%d"), commit=git_head(),
                           start=list(START), field=list(FIELD))
            SESSION.write_text(json.dumps(session, indent=1) + "\n")
            lines = FIXTURE.read_text().splitlines()
            s.note("%s: %d packets" % (FIXTURE, len(lines)))
            others = [l for l in lines if " ZC_NOTIFY_" in l and ("MOVEENTRY" in l or "NEWENTRY" in l)]
            s.trace("actors entering or moving in during the recording", others, max_lines=20)
    elif replay and t.param("compare", "1") != "0":
        compare_step(t)


def clean_fixture(src, dst):
    """The recorded log without the packets the client sends on its own
    timing: PING (0x0187), a sent packet the server answers with
    ZC_NOTIFY_TIME (CZ_REQUEST_TIME, the keep-alive) and one it answers with
    ZC_ACK_REQNAME (CZ_REQNAME: the 2008 client asks for a name when the
    pointer has rested on an actor for a while). In a replay they never fall
    exactly where the recording had them, and a sent packet out of place
    makes the replay drop the ones after it; without them every packet left
    to send is one the script's input makes, and BF_PKT_REPLAY_WAIT can be
    long (60 s), so that no answer is handed over before the action it
    answers. The answers stay in the log (delivered with the packets
    before them)."""
    lines = Path(src).read_text().splitlines()
    fields = [l.split(" ") for l in lines]
    timed = {"0x0187"}          # PING; and every header the server answers as below
    for i, f in enumerate(fields[:-1]):
        nxt = fields[i + 1]
        if len(f) > 4 and f[1] == "S" and len(nxt) > 4 and nxt[1] == "R" \
                and nxt[4] in ("ZC_NOTIFY_TIME", "ZC_ACK_REQNAME"):
            timed.add(f[2])
    out, dropped = [], 0
    for line, f in zip(lines, fields):
        if len(f) > 4 and f[1] == "S" and (f[2] in timed or f[4] == "PING"):
            dropped += 1
            continue
        out.append(line)
    Path(dst).write_text("\n".join(out) + "\n")
    return dropped


def npc(t):
    ctl = t.ctl
    with t.step("npc", "Merchant#pron: dialogue, Next, the menu, No thanks") as s:
        npcplay.click_npc(ctl, at(MERCHANT), "Merchant")
        npcplay.wait_control(ctl, "SAYDIALOGWND", "next")
        shot(t, s, "npc-dialogue", REST)
        while not ctl.window("CHOOSEWND"):
            w = npcplay.wait_control(ctl, "SAYDIALOGWND", "next")
            time.sleep(npcplay.THROTTLE)
            ctl.click_control("SAYDIALOGWND", "next")
            ctl.wait({"frames": 10}, timeout=10)
        shot(t, s, "npc-menu", REST)
        npcplay.run_dialogue(ctl, s, answers=["No, thanks."])


def indoor_and_shop(t):
    ctl = t.ctl
    with t.step("map change", "@warp to %s %d,%d: an indoor map" % INDOOR) as s:
        after = ctl.last_seq
        npcplay.focus_chat(ctl)
        t.gm.run(t.gm.warp(*INDOOR))
        ev = ctl.wait_event("map", after=after, timeout=90)
        s.state("map event", ev)
        ctl.wait({"frames": 120}, timeout=30)
        wait_actor(ctl, at(DEALER), "Tool Dealer")
        shot(t, s, "indoor", GROUND)

    with t.step("shop", "Tool Dealer: deal type, the shop list") as s:
        npcplay.click_npc(ctl, at(DEALER), "Tool Dealer")
        ctl.wait_window("CHOOSESELLBUYWND", timeout=15)
        shot(t, s, "shop-dealtype", REST)
        ctl.click_control("CHOOSESELLBUYWND", "buy")
        ctl.wait_window("ITEMPURCHASEWND", timeout=15)
        npcplay.wait_control(ctl, "ITEMSHOPWND", "item:%d" % RED_POTION)
        shot(t, s, "shop-buy", REST)
        ctl.click_control("ITEMPURCHASEWND", "cancel")
        ctl.wait_window("ITEMPURCHASEWND", open=False, timeout=10)
        if ctl.window("ITEMSHOPWND"):
            ctl.press("escape")
        ctl.wait({"frames": 10}, timeout=10)
        s.state("windows", [w["name"] for w in ctl.state("windows")["windows"]])


def skills_and_potion(t):
    """The skill window, then a Red Potion double-clicked in the item window:
    the item-use effect over the character, which does not move (a walk,
    for instance to a monster, ends at a position the replay does not
    reproduce, and the camera follows it). The character starts below its
    max HP (set-up), so the potion heals."""
    ctl = t.ctl
    with t.step("skills", "Alt+S: the skill window") as s:
        ctl.press("s", "alt")
        ctl.wait_window("SKILLLISTWND", timeout=10)
        npcplay.wait_control(ctl, "SKILLLISTWND", "skill:%d" % FIRST_AID)
        shot(t, s, "skills", REST)
        ctl.press("s", "alt")
        ctl.wait_window("SKILLLISTWND", open=False, timeout=10)

    with t.step("potion", "/effect on, a Red Potion double-clicked in the item window: the item-use effect") as s:
        after = ctl.last_seq
        npcplay.focus_chat(ctl)
        ctl.say("/effect")
        s.state("/effect", ctl.wait_event("chat", lambda e: "ffect" in (e.get("text") or ""), after=after, timeout=10))
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", timeout=10)
        c = npcplay.wait_item(ctl, "ITEMWND", RED_POTION)
        x, y = npcplay.center(c["rect"])
        after = ctl.last_seq
        ctl.double_click(x, y)
        deadline = time.monotonic() + 20
        while npcplay.item_count(ctl, "ITEMWND", RED_POTION) == dict(ITEMS)[RED_POTION]:
            if time.monotonic() > deadline:
                raise AssertionError("the potion was not used")
            ctl.wait({"frames": 2}, timeout=10)
        ctl.wait({"frames": 4}, timeout=10)
        shot(t, s, "potion", None)
        s.state("number events", ctl.events_named("damage", after=after))
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", open=False, timeout=10)
        ctl.wait({"frames": 120}, timeout=30)


def field(t):
    ctl = t.ctl
    with t.step("field", "@warp to %s: another map's world" % FIELD[0]) as s:
        after = ctl.last_seq
        npcplay.focus_chat(ctl)
        t.gm.run(t.gm.warp(*FIELD))
        ev = ctl.wait_event("map", after=after, timeout=90)
        s.state("map event", ev)
        ctl.wait({"frames": 120}, timeout=30)
        shot(t, s, "field", GROUND)


def readback(t):
    """Ctrl+F13: the client reads its own screen back (CMode::ScreenShot) and
    writes a BMP under BF_HOME's Pictures; converted to PNG as a frame."""
    import framecmp as fc
    ctl = t.ctl
    with t.step("readback", "Ctrl+Print Screen: the client's BMP of its screen") as s:
        if t.mock:
            s.note("the mock writes no screenshot")
            return
        pics = t.run_dir / "home" / "Pictures" / paths.app_name()
        ctl.move(*GROUND)
        ctl.wait({"frames": 10}, timeout=20)
        info = regions_of(ctl, scale=1)      # the picture is the logical screen
        for k in range(SAMPLES + 1):
            name = "readback" if k == 0 else "readback.s%d" % k
            before = set(pics.glob("*.bmp")) if pics.exists() else set()
            # CMode::ScreenShot reads Ctrl when it draws the picture, the frame
            # after the key: Ctrl is held across it (as screenshot.py does)
            ctl.key("down", "ctrl")
            ctl.press("f13")
            ctl.wait({"frames": 10}, timeout=20)
            ctl.key("up", "ctrl")
            deadline = time.monotonic() + 20
            new = []
            while time.monotonic() < deadline and not new:
                time.sleep(0.2)
                new = [p for p in (set(pics.glob("*.bmp")) if pics.exists() else set()) - before]
            s.check(len(new) == 1, "one new BMP in %s: %r" % (pics, new))
            time.sleep(0.3)
            w, h, rows = fc.read_bmp(new[0])
            out = t.frames.path(name)
            fc.write_png(out, w, h, rows)
            rec = {"name": name, "path": str(out), "sha256": sha256(out), "width": w, "height": h,
                   "scale": 1, "logical": None, "frame": None}
            t.frames.records.append(rec)
            if k == 0:
                write_regions(t, name, info)
                s.evidence.append(dict(rec, kind="frame"))
                s.note("%s: %dx%d" % (new[0].name, w, h))


def compare_step(t):
    import framecmp as fc
    ref = reference_dir(t)
    with t.step("compare with the reference", "framecmp.py: every reference frame matches") as s:
        if not (ref / "reference.json").exists():
            s.note("no reference at %s: nothing to compare (build one with framecmp.py reference)" % ref)
            return
        out = t.run_dir / "framecmp"
        lines = []
        summary = fc.compare(ref, t.frames.dir, out=out, log=lines.append)
        s.trace("framecmp", lines, max_lines=60)
        s.note("diff images and verdict.json in %s" % out)
        s.check(summary["passed"], "%d of %d frames match the reference (commit %s)"
                % (summary["matched"], summary["frames"], summary["reference_commit"]))
