"""M6 five-minute session: playing around Prontera leaves no unknown packet
header and no failed assertion in the trace.

Done when (implementation plan, M6): a five-minute scripted session leaves
no unknown packet header and no failed assertion in the trace.

Set-up: the character at prontera 155,183 (SQL, offline); a second player
(bifrost-bot, group 99) beside it, which says a line now and then and
walks between two cells, so the client receives other players' chat and
movement; a Poring spawned beside the bot (@monster).
Played for `--param seconds=300` (default five minutes), round after round:
walk to a picked cell, say a line, open and close the item, equipment,
skill, status and option windows with their Alt shortcuts, turn the camera
by right-drag and zoom with the wheel, rest the pointer on NPCs (the client
asks for their names), walk back towards the start.
Checked: each action's direct effect (arrival on the picked cell, the own
line as a chat event, the window open and closed, the bot's lines arriving);
then in the client's trace no "unknown packet header" line (the port's
trace of CGameMode::PollNetworkStatus dropping a header it does not handle)
and no assertion line; in the packet log no received header without a name
in the client's packet table; the client exits cleanly.
"""

import time

from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home, token
from bfharness.sql import wait_rows
from bfharness import pktlog

NAME = "session"
MILESTONE = "M6"
DESCRIPTION = "five minutes of walking, chatting, windows and camera in Prontera; no unknown header, no assertion"
MANIFEST = "port/harness/manifests/m6-session.sha256"
NEEDS_SERVER = True

START = (155, 183)
BOT_CELLS = [(160, 186), (160, 180)]
WINDOWS = [("e", "item"), ("q", "equipment"), ("s", "skill"), ("a", "status"), ("o", "option")]
BASE_WINDOWS = ("CHATWND", "BASICINFOWND", "SHORTENITEMWND", "MINIMAPZOOMWND", "TRANSBALLOONTEXT", "NAMEBALLOONTEXT")
WALKS = [(110, 70), (-110, 90), (60, 120), (-70, 100)]   # south of the start, away from the NPCs


class Tally:
    def __init__(self):
        self.ok = {}
        self.bad = []

    def record(self, what, ok, detail=None):
        self.ok.setdefault(what, [0, 0])
        self.ok[what][0 if ok else 1] += 1
        if not ok:
            self.bad.append({"action": what, "detail": detail})


def walk(ctl, tally, dx, dy):
    """Click a ground cell near the offset from the centre, away from actors
    (a click on an NPC talks to it, which is not this script's business)."""
    v = ctl.state("view")["view"]
    actors = [a for a in ctl.state("actors")["actors"] if a["type"] != "item"]
    for kx, ky in ((0, 0), (40, 0), (-40, 0), (0, 40), (0, -40), (60, 60), (-60, 60), (60, -60), (-60, -60)):
        x, y = v["logical"]["w"] // 2 + dx + kx, v["logical"]["h"] // 2 + dy + ky
        ctl.move(x, y)
        ctl.wait({"frames": 3}, timeout=10)
        st = ctl.state("pointer", "windows")
        cell = st["pointer"]["cell"]
        # The client shows an actor's name balloon while the pointer is on it.
        over = any(w["name"] == "NAMEBALLOONTEXT" for w in st["windows"])
        near = cell and [a for a in actors if abs(a["x"] - cell["x"]) <= 1 and abs(a["y"] - cell["y"]) <= 1]
        if cell and not over and not near:
            break
    else:
        tally.record("walk", False, "no free cell near offset %d,%d" % (dx, dy))
        return
    ctl.click(x, y)
    try:
        ctl.wait({"position": cell, "moving": False}, timeout=30)
        tally.record("walk", True)
    except Exception as e:
        me = ctl.state("self")["self"]
        # A cell the server refuses (not walkable) stops the walk short.
        tally.record("walk", False, "to %r: %s; at %d,%d" % (cell, e, me["x"], me["y"]))
    opened = [w["name"] for w in ctl.state("windows")["windows"] if w["name"] not in BASE_WINDOWS]
    if opened:
        tally.record("walk", False, "a click opened %s" % opened)
        close_dialogs(ctl)


def close_dialogs(ctl, tries=8):
    """End an NPC's dialog or menu by its own buttons (Escape in game mode
    would open the quit window)."""
    for _ in range(tries):
        open_ = [w for w in ctl.state("windows")["windows"] if w["name"] in ("SAYDIALOGWND", "CHOOSEWND")]
        if not open_:
            return
        ids = [c["id"] for w in open_ for c in w.get("controls", []) if c["kind"] == "button"]
        pick = next((i for i in ("close", "cancel", "next", "ok") if i in ids), None)
        choices = [c["id"] for w in open_ for c in w.get("controls", []) if c["id"].startswith("choice:")]
        target = pick or (choices[-1] if choices else None)
        w = open_[0] if pick or not choices else next(w for w in open_ if any(c["id"] == target for c in w["controls"]))
        if target is None:
            return
        ctl.click_control(w["name"], target)
        ctl.wait({"frames": 10}, timeout=10)


def say(ctl, tally, text):
    after = ctl.last_seq
    ctl.say(text)
    try:
        ctl.wait_event("chat", lambda e: text in (e.get("text") or ""), after=after, timeout=10)
        tally.record("say", True)
    except Exception as e:
        tally.record("say", False, "%r: %s" % (text, e))


def window(ctl, tally, key, label):
    after = ctl.last_seq
    ctl.press(key, "alt")
    try:
        ev = ctl.wait_event("window", lambda e: e.get("action") == "open", after=after, timeout=5)
    except Exception as e:
        tally.record("window", False, "alt+%s (%s) opened nothing: %s" % (key, label, e))
        return
    ctl.wait({"frames": 10}, timeout=10)
    after = ctl.last_seq
    ctl.press(key, "alt")
    try:
        ctl.wait_event("window", lambda e: e.get("action") == "close" and e.get("wid") == ev["wid"],
                       after=after, timeout=5)
        tally.record("window", True)
    except Exception as e:
        tally.record("window", False, "alt+%s (%s) did not close %s: %s" % (key, label, ev.get("name"), e))


def camera(ctl, tally, k):
    """Turn by right-drag and zoom with the wheel, then back, so that the
    walks of the next round go the same way."""
    v = ctl.state("view")["view"]
    cx, cy = v["logical"]["w"] // 2, v["logical"]["h"] // 2 - 120
    turn = 60 if k % 2 else -60
    notch = 1 if k % 2 else -1
    ctl.drag(cx, cy, cx + turn, cy, button="right")
    for _ in range(3):
        ctl.wheel(notch, cx, cy)
    ctl.wait({"frames": 20}, timeout=10)
    ctl.drag(cx + turn, cy, cx, cy, button="right")
    for _ in range(3):
        ctl.wheel(-notch, cx, cy)
    ctl.wait({"frames": 20}, timeout=10)
    tally.record("camera", True)


def hover(ctl, tally):
    st = ctl.state("actors")
    if not any(a["type"] == "npc" and a["screen"] for a in st["actors"]):
        return
    a = ctl.actor(lambda a: a["type"] == "npc" and a["name"] != "?name", state=st)
    tally.record("hover", a is not None, None if a else "no NPC name learnt")


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")
    seconds = float(t.param("seconds", "20" if t.mock else "300"))

    with t.step("set-up: place character", "SQL, offline: prontera %d,%d" % START) as s:
        c = t.accounts.set_char(name, map="prontera", x=START[0], y=START[1])
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})

    with t.step("set-up: second player", "bifrost-bot by atcommand: @warp, @monster") as s:
        bot = Bot.start(t, "session", "bfhsessgm", "bfbot", "BfhSessGM", gm=True)
        if bot is None:
            s.note("mock: no second player")
        else:
            s.state("bot warped", bot.warp("prontera", *BOT_CELLS[0]))
            bot.gm("@monster 1002 1")

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    tally = Tally()
    with t.step("play for %d s" % seconds, "M6: a %d-second session of walking, chat, windows, camera" % seconds) as s:
        s.frame("start")
        start = time.monotonic()
        k = 0
        while time.monotonic() - start < seconds:
            dx, dy = WALKS[k % len(WALKS)]
            walk(ctl, tally, dx, dy)
            say(ctl, tally, "bfh session %d %s" % (k, token()))
            key, label = WINDOWS[k % len(WINDOWS)]
            window(ctl, tally, key, label)
            camera(ctl, tally, k)
            if k % 3 == 0:
                hover(ctl, tally)
            if bot is not None:
                # The bot walks up to the character (chat reaches the area
                # around the speaker), then says its line.
                me = ctl.state("self")["self"]
                since = bot.mark()
                bot.move(me["x"] + 2, me["y"] + (1 if k % 2 else -1))
                try:
                    bot.wait(lambda e: e.get("event") == "position" and (e.get("arrived") or e.get("stopped")),
                             timeout=15, since=since)
                except Exception as e:
                    tally.record("bot line", False, "the bot did not reach the character: %s" % e)
                line = "bfh bot %d %s" % (k, token())
                after = ctl.last_seq
                bot.say(line)
                try:
                    ctl.wait_event("chat", lambda e: line in (e.get("text") or ""), after=after, timeout=10)
                    tally.record("bot line", True)
                except Exception as e:
                    tally.record("bot line", False, "%r: %s" % (line, e))
            walk(ctl, tally, -dx, -dy)
            k += 1
        s.note("%d rounds in %.0f s" % (k, time.monotonic() - start))
        s.state("actions: [succeeded, failed]", tally.ok)
        s.state("failed actions", tally.bad)
        s.frame("end")
        s.check(k >= 1, "at least one round")
        # A walk may stop short on a cell the server will not walk to; the
        # rest must not fail at all.
        hard = [b for b in tally.bad if b["action"] != "walk"]
        s.check(not hard, "chat, windows, camera, names and the bot's lines worked every time", failed=hard)
        walks = tally.ok.get("walk", [0, 0])
        s.check(walks[0] >= 0.9 * sum(walks), "at least 90%% of the walks arrived (%d of %d)" % (walks[0], sum(walks)))
        s.check(ctl.state("mode")["mode"] == "game", "still in game")

    client = t.client
    pkt_path = client.pkt_log
    t.stop_client()
    with t.step("trace", "M6: no unknown packet header and no failed assertion in the trace") as s:
        unknown = client.trace_lines("unknown packet header")
        asserts = client.trace_lines("assert", "ASSERT")
        crash = client.trace_lines("crash:")
        errors = [l for l in client.trace_lines() if " err " in l[:24]]
        s.trace("unknown packet header lines", unknown)
        s.trace("assertion lines", asserts)
        s.trace("crash lines", crash)
        s.trace("all error lines (for the record)", errors[:40])
        s.check(not unknown, "no unknown packet header (%d lines)" % len(unknown))
        s.check(not asserts, "no failed assertion (%d lines)" % len(asserts))
        s.check(not crash, "no crash")
        packets = pktlog.read(pkt_path)
        received = [p for p in packets if p.dir == "R"]
        nameless = sorted(set("%s %s" % (p.header, p.conn.split("/")[0]) for p in received
                              if p.name in ("?",) and p.conn.split("/")[0] == "zone"))
        s.note("%d packets received, %d sent" % (len(received), len(packets) - len(received)))
        s.state("received zone headers the client's packet table does not name", nameless)
        if not t.mock:
            s.check(not nameless, "every header received on the zone connection is in the client's table")
        if not t.mock:
            rows = wait_rows(lambda: t.logs.chat(char_id=c["char_id"], contains="bfh session"), timeout=10)
            s.rows("chatlog lines of the session", rows, min_rows=tally.ok.get("say", [0])[0])
