"""M8.6 the one-hour soak: a scripted session that plays the M8 features
round after round for an hour ends with no crash, no unknown packet and
stable memory.

Done when (implementation plan, M8 exit): "A one-hour scripted session ends
with no crash, no unknown packet and stable memory." `--param seconds=3600`
is the default; a shorter run plays the same rounds.

Set-up: the script's own group-99 account and character (bfhsoak /
BfhSoaker, made by bifrost-bot on first use; `--param user= password=
char=` for another), made a Swordman in SQL while offline (base level 50,
job level 40, Str 60, Basic Skill 9, Bash 10, Provoke 5, Endure 10; the
server drops Endure without Provoke 5), with 1,000,000
zeny, 400 Jellopy and 50 Red Potions, an empty storage, and the shortcut
bar's F1 = Endure, F2 = Bash, F3 = Red Potion (the `hotkey` table, sent by
the server at login). A second player, bifrost-bot on its own GM account
(bfhsoakgm / BfhSoakGM), warps beside the character in Prontera each round,
says a line and whispers one. Moves between the scenes of a round are
atcommands the client types (`@warp`, in atcommandlog); `@monster 1002` is
typed beside the character on the field, and `@item 909 200` when the
Jellopy run low.

A round, every action played through the control socket:
1. town (prontera 172,184): walk to picked cells and back, say a line,
   open and close one of the Alt+E/Q/S/A/O windows, turn and zoom the
   camera, rest the pointer on NPCs (session.py's steps); the bot's public
   line and whisper arrive;
2. through the portal prt06 into prt_in and back by prt06-1;
3. through prt04 into prt_in, buy a Red Potion and sell a Jellopy at the
   Tool Dealer (deal windows, amounts typed), back by prt04-1;
4. the Merchant#pron dialogue (Next, menus, the number 1): 10 Jellopy for
   a Red Potion (picklog N);
5. the Kafra: Use Storage (fee, zenylog N), 2 Jellopy put in, 1 taken out
   (picklog R), storage closed;
6. through the south gate prt001 onto prt_fild08; Endure by F1; two
   Porings spawned beside the character, the first hit with Bash (F2 and a
   click on it), both Ctrl-clicked until dead, a drop clicked as it lands
   (picklog P); a Red Potion used by double-click in the item window and
   one by F3 (picklog C); `@warp` back in sight of prtf004 (the fights
   wander off) and through it to Prontera.
An action that fails is recorded with its error, the windows it left open
are closed (a character killed by another script's monsters restarts at its
save point), and the scene goes on, or, after a failed warp or portal, the
round goes on with the next scene (each begins with its own `@warp`). The
run stops early only if the client exits.

Checked at the end, each criterion as a step of its own, recorded side by
side (one failing does not hide the others):
- no crash: the client was running at every step and exits with status 0
  when asked to quit (the runner's last step), no "crash:" line in its
  trace;
- no unknown packet: no "unknown packet header" line in the trace (the
  port's trace of CGameMode::PollNetworkStatus dropping a header it does not
  handle) and no received zone header without a name in the client's
  packet table (packet log); no failed assertion in the trace;
- the session really played: every activity succeeded at least once and in
  at least 80% of its tries (a drop lost to a looting Poring is not a
  failure of the pick-up, it is counted apart);
- stable memory, defined here before the first hour was run and not
  changed after it: the client's resident set size (`ps -o rss=`) is
  sampled every 30 s from the start of play. The first sixth of the play
  time (10 minutes of the hour) is warm-up: every scene and map of the
  round has been loaded at least once by then. Memory is stable when, over
  the samples after the warm-up, (a) the least-squares slope of RSS is
  below 5 MB per 10 minutes, and (b) no sample is more than 50 MB above the
  highest sample of the warm-up. The series goes to `rss.csv` in the run
  directory and into the report, with the physical footprint (macOS
  `footprint`) beside it, recorded and not judged; `leaks <pid>` (macOS) is run on the
  client once at the end and its summary recorded (not judged: the debug
  build keeps no allocation history).
"""

import os
import re
import subprocess
import threading
import time

from bfharness import npcplay, pktlog
from bfharness.bot import Bot
from bfharness.field import find, grab_drop, own_monster, walk_into_portal
from bfharness.play import enter_game, fresh_character, login, own_home, token
from bfharness.runner import Step, load_script
from bfharness.sql import wait_rows

NAME = "soak"
MILESTONE = "M8"
DESCRIPTION = "one hour of play round after round (town, portals, shop, NPC, storage, combat, skills, items); " \
              "no crash, no unknown packet, stable memory"
MANIFEST = "port/harness/manifests/m8-soak.sha256"
NEEDS_SERVER = True

session = load_script("session")     # its walk, say, window, camera, hover and Tally
# windows that come and go by themselves: the HP gauge under a hurt
# character, the shortcut bar the soak keeps open
skills = load_script("skills")       # its move_window
session.BASE_WINDOWS = tuple(session.BASE_WINDOWS) + ("PCGAGE", "SHORTCUTWND")

PORING, JELLOPY, RED_POTION = 1002, 909, 501
NV_BASIC, BASH, PROVOKE, ENDURE, SWORDMAN = 1, 5, 6, 8, 1
SKILLS = {NV_BASIC: 9, BASH: 10, PROVOKE: 5, ENDURE: 10}
HOTKEYS = [(0, 1, ENDURE, 10), (1, 1, BASH, 10), (2, 0, RED_POTION, 0)]   # slot, type (1 skill), id, level
ZENY, JELLOPIES, POTIONS = 1000000, 400, 50

TOWN = ("prontera", 172, 184)
# session.py's walks that end on cells the server walks to from TOWN (its
# (110, 70) and (60, 120) pick cells the server's map refuses, so the walk
# stops one cell short every time)
TOWN_WALKS = [(-110, 90), (-70, 100)]
BEFORE_PRT04 = ("prontera", 134, 216)
AT_MERCHANT = ("prontera", 123, 98)
AT_KAFRA = ("prontera", 149, 86)
NORTH_OF_GATE = ("prontera", 156, 29)
SOUTH_OF_FIELD_EXIT = ("prt_fild08", 170, 372)   # the fights wander off; back in sight of prtf004
MERCHANT = (123, 102)
KAFRA = (146, 89)
DEALER = (126, 76)
# the warps as the server's npc/ defines them: name, map, x, y, span x, span y, to map, x, y
PRT06 = ("prt06", "prontera", 179, 184, 2, 2, "prt_in", 60, 73)
PRT06_1 = ("prt06-1", "prt_in", 60, 77, 2, 1, "prontera", 175, 188)
PRT04 = ("prt04", "prontera", 134, 221, 1, 1, "prt_in", 131, 71)
PRT04_1 = ("prt04-1", "prt_in", 135, 71, 1, 2, "prontera", 136, 219)
SOUTH_GATE = ("prt001", "prontera", 156, 22, 3, 2, "prt_fild08", 170, 375)
FIELD_NORTH = ("prtf004", "prt_fild08", 170, 378, 3, 2, "prontera", 156, 26)

CZ_USE_SKILL, ZC_USE_SKILL, ZC_NOTIFY_SKILL = "0x0072", "0x011a", "0x01de"
SAMPLE_SECONDS = 30
SLOPE_MB_PER_10MIN = 5.0
MARGIN_MB = 50.0
MIN_RATE = 0.8


def at(xy):
    return lambda a: a["type"] == "npc" and (a["x"], a["y"]) == tuple(xy)


class Packets:
    """The client's packet log read as it grows (an hour's log is large)."""

    def __init__(self, path):
        self.path, self.offset, self.seen = path, 0, []

    def new(self):
        if not os.path.exists(self.path):
            return []
        with open(self.path, "rb") as f:
            f.seek(self.offset)
            data = f.read()
        cut = data.rfind(b"\n") + 1
        self.offset += cut
        out = []
        for line in data[:cut].decode("utf-8", "replace").splitlines():
            p = line.split()
            if len(p) >= 6 and p[1] in ("S", "R"):
                out.append((p[1], p[2].lower(), p[5]))
        return out

    def since(self, mark, dir, header):
        """Packets (dir, header) logged after mark = self.mark()."""
        self.seen += self.new()
        return [h for d, hd, h in self.seen[mark:] if d == dir and hd == header]

    def mark(self):
        self.seen = self.seen[-200:] if len(self.seen) > 5000 else self.seen
        self.seen += self.new()
        return len(self.seen)


def u16(hexs, at):
    return int.from_bytes(bytes.fromhex(hexs)[at:at + 2], "little")


class Memory(threading.Thread):
    """RSS of the client every SAMPLE_SECONDS: [(seconds since play began, KB, activity,
    physical footprint in MB or None)]. The footprint (macOS `footprint`,
    dirty memory without the allocator's reclaimable free pages) is recorded
    beside RSS, not judged."""

    def __init__(self, pid, mock):
        super().__init__(daemon=True)
        self.pid, self.mock = pid, mock
        self.samples, self.activity = [], "start"
        self.t0 = time.monotonic()
        self.stop = threading.Event()

    def rss(self):
        r = subprocess.run(["ps", "-o", "rss=", "-p", str(self.pid)], capture_output=True, text=True)
        return int(r.stdout.strip()) if r.stdout.strip().isdigit() else None

    def footprint(self):
        if self.mock:
            return None
        try:
            r = subprocess.run(["footprint", str(self.pid)], capture_output=True, text=True, timeout=20)
        except (OSError, subprocess.TimeoutExpired):
            return None
        m = re.search(r"Footprint: ([\d.]+) (KB|MB|GB)", r.stdout)
        return round(float(m.group(1)) * {"KB": 1 / 1024.0, "MB": 1, "GB": 1024}[m.group(2)], 1) if m else None

    def run(self):
        while not self.stop.is_set():
            kb = self.rss()
            if kb is not None:
                self.samples.append((round(time.monotonic() - self.t0, 1), kb, self.activity, self.footprint()))
            self.stop.wait(SAMPLE_SECONDS if not self.mock else 2)


def slope(points):
    """Least-squares slope of (x, y) points."""
    n = len(points)
    if n < 2:
        return 0.0
    mx = sum(x for x, _ in points) / n
    my = sum(y for _, y in points) / n
    sxx = sum((x - mx) ** 2 for x, _ in points)
    return sum((x - mx) * (y - my) for x, y in points) / sxx if sxx else 0.0


def judge_memory(samples, warmup):
    """The docstring's definition. Returns (stable, numbers)."""
    warm = [kb for s, kb, *_ in samples if s < warmup]
    rest = [(s, kb) for s, kb, *_ in samples if s >= warmup]
    if not warm or len(rest) < 2:
        return False, {"error": "too few samples (%d in warm-up, %d after)" % (len(warm), len(rest))}
    per_10min = slope([(s / 600.0, kb / 1024.0) for s, kb in rest])
    peak = max(warm) / 1024.0
    top = max(kb for _, kb in rest) / 1024.0
    n = {"samples": len(samples), "warmup_seconds": warmup,
         "rss_first_mb": round(samples[0][1] / 1024.0, 1),
         "rss_end_of_warmup_mb": round(warm[-1] / 1024.0, 1),
         "rss_last_mb": round(samples[-1][1] / 1024.0, 1),
         "warmup_peak_mb": round(peak, 1), "after_warmup_max_mb": round(top, 1),
         "after_warmup_min_mb": round(min(kb for _, kb in rest) / 1024.0, 1),
         "slope_mb_per_10min": round(per_10min, 3),
         "bound_slope_mb_per_10min": SLOPE_MB_PER_10MIN, "bound_margin_mb": MARGIN_MB}
    return per_10min < SLOPE_MB_PER_10MIN and top <= peak + MARGIN_MB, n


class Soak:
    def __init__(self, t, ctl, char_id, aid, bot):
        self.t, self.ctl, self.char_id, self.aid, self.bot = t, ctl, char_id, aid, bot
        self.tally = session.Tally()
        self.notes = {}
        self.pk = Packets(str(t.client.pkt_log)) if not t.mock else Packets(str(t.run_dir / "client-packets.txt"))
        self.monsters = 0
        self.k = 0
        self.memory = None
        self.dead = None

    # -- bookkeeping ---------------------------------------------------------

    def note(self, what):
        self.notes[what] = self.notes.get(what, 0) + 1

    def alive(self):
        c = self.t.client
        if c and c.proc and c.proc.poll() is not None:
            self.dead = "the client exited with %s" % c.proc.returncode
        elif self.ctl.closed:
            self.dead = "the control connection closed"
        return self.dead is None

    def act(self, what, fn, *args):
        """Run one action; record it; on failure tidy up. Returns its success."""
        if self.memory:
            self.memory.activity = what
        s = Step(self.t, what, None)  # scratch evidence, not part of the report
        try:
            fn(s, *args)
        except Exception as e:
            self.tally.record(what, False, "round %d: %s: %s" % (self.k, type(e).__name__, str(e)[:300]))
            if self.alive():
                try:
                    self.tidy()
                except Exception as e2:
                    self.tally.record("tidy", False, "round %d: %s" % (self.k, e2))
            return False
        self.tally.record(what, True)
        return True

    def leg(self, *actions):
        """Play the actions in order; a failed warp or portal ends the leg
        (the next ones need its place), a failed other action does not."""
        for a in actions:
            if not self.alive():
                return False
            if not self.act(*a) and a[0].startswith(("warp", "portal")):
                return False
        return True

    def tidy(self):
        """Close what a failed action left open: NPC dialogs by their buttons,
        deal and storage windows, the count window, the Alt windows."""
        ctl = self.ctl
        if ctl.window("RESTARTWND"):
            # killed (another script's monsters on the shared server): back
            # to the save point, as a player would; the next scene warps
            c = npcplay.controls(ctl.window("RESTARTWND"))
            if ctl.state("self")["self"]["hp"] == 0 and "restart" in c:
                self.note("killed, restarted at the save point")
                ctl.click_control("RESTARTWND", "restart")
                ctl.wait({"frames": 60}, timeout=30)
            elif "cancel" in c:
                ctl.click_control("RESTARTWND", "cancel")
        session.close_dialogs(ctl)
        for _ in range(6):
            names = [w["name"] for w in ctl.state("windows")["windows"]]
            done = True
            for wname, ids in (("ITEMDROPCNTWND", ("cancel", "ok")), ("ITEMPURCHASEWND", ("cancel",)),
                               ("ITEMSELLWND", ("cancel",)), ("CHOOSESELLBUYWND", ("cancel",)),
                               ("ITEMSTOREWND", ("close",)), ("NPCEDITDIALOGWND", ("cancel", "ok"))):
                if wname in names:
                    c = npcplay.controls(ctl.window(wname))
                    pick = next((i for i in ids if i in c), None)
                    if pick:
                        ctl.click_control(wname, pick)
                        ctl.wait({"frames": 10}, timeout=10)
                        done = False
            if done:
                break
        for key, wname in (("e", "ITEMWND"), ("q", "EQUIPWND"), ("s", "SKILLLISTWND"), ("a", "STATUSWND"),
                           ("o", "OPTIONWND")):
            if ctl.window(wname):
                ctl.press(key, "alt")
                ctl.wait({"frames": 5}, timeout=10)
        session.close_dialogs(ctl)
        time.sleep(0.3)

    # -- scene set-up ----------------------------------------------------------

    def gm(self, s, command):
        mark = self.t.logs.mark()
        npcplay.focus_chat(self.ctl)
        self.t.gm.run(command)
        s.rows("atcommandlog %r" % command,
               wait_rows(lambda: self.t.logs.atcommands(contains=command, after=mark), timeout=15))

    def warp(self, s, place):
        self.gm(s, self.t.gm.warp(*place))
        self.ctl.wait({"map": place[0], "position": {"x": place[1], "y": place[2]}}, timeout=60)
        self.ctl.wait({"frames": 30}, timeout=20)

    # -- the actions -------------------------------------------------------------

    def town(self, s):
        ctl, tally, k = self.ctl, self.tally, self.k
        for i in (0, 1):
            dx, dy = TOWN_WALKS[(2 * k + i) % len(TOWN_WALKS)]
            session.walk(ctl, tally, dx, dy)
            session.walk(ctl, tally, -dx, -dy)
        session.say(ctl, tally, "bfh soak %d %s" % (k, token()))
        key, label = session.WINDOWS[k % len(session.WINDOWS)]
        session.window(ctl, tally, key, label)
        session.camera(ctl, tally, k)
        session.hover(ctl, tally)

    def bot_lines(self, s):
        if self.bot is None:
            s.note("mock: no second player")
            return
        me = self.ctl.state("map", "self")
        self.bot.warp(me["map"]["name"], me["self"]["x"] + 2, me["self"]["y"] - 1)
        line = "bfh soak bot %d %s" % (self.k, token())
        after = self.ctl.last_seq
        self.bot.say(line)
        self.ctl.wait_event("chat", lambda e: line in (e.get("text") or ""), after=after, timeout=15)
        line = "bfh soak whisper %d %s" % (self.k, token())
        after = self.ctl.last_seq
        self.bot.send("whisper", name=self.name, text=line)
        self.ctl.wait_event("chat", lambda e: e.get("channel") == "whisper" and line in (e.get("text") or ""),
                            after=after, timeout=15)

    def portal(self, s, warp):
        # a ground click within 225 ms of the last button press is not a
        # walk (ProcessGroundPick 0x588510, m_leftBtnClickTick); the deal and
        # dialog windows end with one
        time.sleep(0.3)
        walk_into_portal(self.t, self.ctl, s, warp)
        me = self.ctl.state("map", "self")
        s.equal(me["map"]["name"], warp[6], "map after the portal")
        if not self.t.mock:     # the mock's field portal lands on its own cell
            s.equal((me["self"]["x"], me["self"]["y"]), (warp[7], warp[8]), "cell after the portal")
        self.ctl.wait({"frames": 30}, timeout=20)

    def shop(self, s):
        t, ctl = self.t, self.ctl
        mark = t.logs.mark()
        npcplay.deal(ctl, at(DEALER), "buy", RED_POTION, 1, s)
        s.rows("picklog S Red Potion +1", wait_rows(lambda: [r for r in t.logs.bought(self.char_id, RED_POTION,
                                                                                         after=mark)
                                                               if r["amount"] == 1]))
        time.sleep(npcplay.THROTTLE)
        mark = t.logs.mark()
        npcplay.deal(ctl, at(DEALER), "sell", JELLOPY, 1, s)
        s.rows("picklog S Jellopy -1", wait_rows(lambda: [r for r in t.logs.sold(self.char_id, JELLOPY, after=mark)
                                                            if r["amount"] == -1]))

    def merchant(self, s):
        t, ctl = self.t, self.ctl
        mark = t.logs.mark()
        npcplay.click_npc(ctl, at(MERCHANT), "Merchant")
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        npcplay.run_dialogue(ctl, s, answers=["Sure!", "Jellopies", "Deal."], numbers=[1])
        rows = wait_rows(lambda: [r for r in t.logs.items(char_id=self.char_id, type="N", after=mark)
                                  if (r["nameid"], r["amount"]) in ((JELLOPY, -10), (RED_POTION, 1))]
                         if len(t.logs.items(char_id=self.char_id, type="N", after=mark)) >= 2 else [])
        s.rows("picklog N: 10 Jellopy for a Red Potion", rows, min_rows=2)

    def storage(self, s):
        t, ctl = self.t, self.ctl
        mark = t.logs.mark()
        npcplay.click_npc(ctl, at(KAFRA), "Kafra Employee", dy=-50)
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        npcplay.run_dialogue(ctl, s, answers=["Use Storage"])
        ctl.wait_window("ITEMSTOREWND", timeout=15)
        s.rows("zenylog N (storage fee)", wait_rows(lambda: t.logs.zeny(self.char_id, "N", after=mark)))
        if not ctl.window("ITEMWND"):
            ctl.press("e", "alt")
            ctl.wait_window("ITEMWND", timeout=10)
        npcplay.click_tab(ctl, "ITEMWND", "etc")
        npcplay.wait_item(ctl, "ITEMWND", JELLOPY)
        mark = t.logs.mark()
        npcplay.drag_item(ctl, "ITEMWND", JELLOPY, "ITEMSTOREWND")
        npcplay.type_into(ctl, "ITEMDROPCNTWND", 2)
        s.rows("picklog R Jellopy -2", wait_rows(lambda: [r for r in t.logs.items(self.char_id, JELLOPY, "R",
                                                                                   after=mark)
                                                            if r["amount"] == -2]))
        npcplay.click_tab(ctl, "ITEMSTOREWND", "etc")
        npcplay.wait_item(ctl, "ITEMSTOREWND", JELLOPY)
        mark = t.logs.mark()
        npcplay.drag_item(ctl, "ITEMSTOREWND", JELLOPY, "ITEMWND")
        npcplay.type_into(ctl, "ITEMDROPCNTWND", 1)
        s.rows("picklog R Jellopy +1", wait_rows(lambda: [r for r in t.logs.items(self.char_id, JELLOPY, "R",
                                                                                   after=mark)
                                                            if r["amount"] == 1]))
        ctl.click_control("ITEMSTOREWND", "close")
        ctl.wait_window("ITEMSTOREWND", open=False, timeout=15)
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", open=False, timeout=10)

    def shortcut_bar(self, s):
        """F12 shows the shortcut bar and turns its pages: to page 1, where
        F1-F9 use hotkeys 0-8; then dragged to the top edge, out of the way
        of the ground clicks (as skills.py)."""
        ctl = self.ctl
        for _ in range(4):
            if npcplay.controls(ctl.window("SHORTCUTWND")).get("page", {}).get("text") == "1":
                break
            ctl.press("f12")
            ctl.wait({"frames": 3}, timeout=10)
        s.check(npcplay.controls(ctl.window("SHORTCUTWND")).get("page", {}).get("text") == "1",
                "the shortcut bar at page 1")
        skills.move_window(ctl, "SHORTCUTWND", (272, 16), (300, 0))

    def endure(self, s):
        mark = self.pk.mark()
        self.ctl.press("f1")
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if [h for h in self.pk.since(mark, "R", ZC_USE_SKILL) if u16(h, 2) == ENDURE]:
                return
            time.sleep(0.3)
        raise AssertionError("no ZC_USE_SKILL for Endure after F1")

    def spawn(self, s):
        self.monsters += 1
        me = self.ctl.state("self")["self"]
        target = own_monster(self.t, self.ctl, s, PORING, me, self.monsters)
        s.check(target is not None and target["screen"], "a Poring on screen")
        return target

    def fight(self, s, bash):
        """Spawn a Poring; with bash, F2 and a click on it first; then
        Ctrl-click until it dies; click its drop as it lands."""
        t, ctl = self.t, self.ctl
        target = self.spawn(s)
        if bash:
            # F2, then a click on the Poring (the target cursor); once more
            # if it walked off before the click landed
            mark = self.pk.mark()
            hit = []
            for _ in range(2):
                a = find(ctl, target["id"])
                if not a or not a["screen"]:
                    break
                ctl.press("f2")
                ctl.wait({"frames": 3}, timeout=10)
                a = find(ctl, target["id"]) or a
                if a["screen"]:
                    ctl.move(a["screen"]["x"], a["screen"]["y"])
                    ctl.wait({"frames": 3}, timeout=10)
                    ctl.click(a["screen"]["x"], a["screen"]["y"])
                deadline = time.monotonic() + 8
                while time.monotonic() < deadline and not hit:
                    hit = [h for h in self.pk.since(mark, "R", ZC_NOTIFY_SKILL) if u16(h, 2) == BASH]
                    time.sleep(0.3)
                if hit:
                    break
            self.tally.record("bash", bool(hit), None if hit else "round %d: no ZC_NOTIFY_SKILL for Bash" % self.k)
        mark = t.logs.mark()
        deadline = time.monotonic() + 60
        clicks, gone, cell, last, seen = 0, False, (target["x"], target["y"]), 0, 0
        after = ctl.last_seq
        while time.monotonic() < deadline:
            a = find(ctl, target["id"])
            if not a:
                gone = True
                break
            cell = (a["x"], a["y"])
            hits = [e for e in ctl.events_named("damage", after) if e["target"] == target["id"]]
            if len(hits) != seen:
                seen, last = len(hits), time.monotonic()
            if a["screen"] and (clicks == 0 or time.monotonic() - last > 6):
                ctl.move(a["screen"]["x"], a["screen"]["y"])
                ctl.wait({"frames": 3}, timeout=10)
                ctl.key("down", "ctrl")
                ctl.click(a["screen"]["x"], a["screen"]["y"])
                ctl.key("up", "ctrl")
                clicks += 1
                last = time.monotonic()
            time.sleep(0.3 if not t.mock else 0.1)
        s.check(gone, "the Poring died within 60 s")
        item = grab_drop(ctl, s, cell)
        if not item:
            self.note("no drop to pick up (nothing dropped, or a Poring looted it)")
            return
        rows = wait_rows(lambda: t.logs.picked_up(self.char_id, item["job"], after=mark), timeout=15)
        self.tally.record("pick up", bool(rows), None if rows else "round %d: no picklog P for %d" % (self.k,
                                                                                                    item["job"]))

    def potions(self, s):
        t, ctl = self.t, self.ctl
        if not ctl.window("ITEMWND"):
            ctl.press("e", "alt")
            ctl.wait_window("ITEMWND", timeout=10)
        npcplay.click_tab(ctl, "ITEMWND", "item")
        pot = npcplay.inventory_items(ctl, RED_POTION)
        s.check(pot, "a Red Potion in the inventory")
        c = npcplay.controls(ctl.window("ITEMWND")).get("item:%d" % pot[0]["index"])
        s.check(c, "the potion shown in the item window")
        mark = t.logs.mark()
        time.sleep(0.6)
        ctl.move(*npcplay.center(c["rect"]))
        ctl.wait({"frames": 10}, timeout=10)
        ctl.double_click(*npcplay.center(c["rect"]))
        s.rows("picklog C Red Potion (double-click)",
               wait_rows(lambda: t.logs.items(self.char_id, RED_POTION, "C", after=mark, sign=-1)))
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", open=False, timeout=10)
        mark = t.logs.mark()
        ctl.press("f3")
        s.rows("picklog C Red Potion (F3)",
               wait_rows(lambda: t.logs.items(self.char_id, RED_POTION, "C", after=mark, sign=-1)))

    def restock(self, s):
        jel = sum(i["amount"] for i in npcplay.inventory_items(self.ctl, JELLOPY))
        if jel < 60:
            self.gm(s, self.t.gm.item(JELLOPY, 200))
            self.note("Jellopy restocked by @item")

    # -- a round ---------------------------------------------------------------

    def round(self):
        if self.ctl.window("RESTARTWND"):
            self.tidy()
        if not self.ctl.window("SHORTCUTWND"):
            self.act("shortcut bar", self.shortcut_bar)
        self.leg(("warp to town", self.warp, TOWN), ("town", self.town), ("bot lines", self.bot_lines))
        self.leg(("warp to town", self.warp, TOWN), ("portal prt06", self.portal, PRT06),
                 ("portal prt06-1", self.portal, PRT06_1))
        self.leg(("warp to prt04", self.warp, BEFORE_PRT04), ("portal prt04", self.portal, PRT04),
                 ("shop", self.shop), ("portal prt04-1", self.portal, PRT04_1))
        self.leg(("warp to the merchant", self.warp, AT_MERCHANT), ("restock", self.restock),
                 ("npc dialogue", self.merchant))
        self.leg(("warp to the kafra", self.warp, AT_KAFRA), ("storage", self.storage))
        self.leg(("warp to the gate", self.warp, NORTH_OF_GATE), ("portal prt001", self.portal, SOUTH_GATE),
                 ("endure", self.endure), ("fight with bash", self.fight, True), ("fight", self.fight, False),
                 ("potions", self.potions), ("warp to the field exit", self.warp, SOUTH_OF_FIELD_EXIT),
                 ("portal prtf004", self.portal, FIELD_NORTH))
        with self.ctl._cond:                  # an hour of events: keep the recent ones
            if len(self.ctl.events) > 4000:
                self.ctl.events = self.ctl.events[-2000:]


def set_up(t, user, password, name):
    c = fresh_character(t, user, password, name)
    cid, aid = c["char_id"], c["account_id"]
    with t.step("set-up: Swordman, items, hotkeys", "SQL, offline: job, levels, skills, inventory, storage, "
                                                     "shortcut bar") as s:
        c = t.accounts.set_char(name, map=TOWN[0], x=TOWN[1], y=TOWN[2], job=SWORDMAN, base_level=50,
                                job_level=40, zeny=ZENY, hp=9999)
        if t.mock:
            t.mockdb.update("char", cid, sp=200, max_sp=200, hp=900, max_hp=900, str=60)
            for k, v in SKILLS.items():
                t.mockdb.put("skill", char_id=cid, id=k, lv=v, flag=0)
            for slot, kind, sid, lv in HOTKEYS:
                t.mockdb.put("hotkey", char_id=cid, hotkey=slot, type=str(kind), itemskill_id=sid, skill_lvl=lv)
            for nameid, amount in ((JELLOPY, JELLOPIES), (RED_POTION, POTIONS)):
                iid = max([r["id"] for r in t.mockdb.rows("inventory")] or [0]) + 1
                t.mockdb.put("inventory", id=iid, char_id=cid, nameid=nameid, amount=amount, equip=0)
        else:
            t.db.execute("UPDATE `char` SET `str` = 60, `dex` = 40, `int` = 20, `sp` = 9999, `status_point` = 0, "
                         "`skill_point` = 0 WHERE `char_id` = %d AND `online` = 0" % cid)
            for table, where in (("skill", "char_id"), ("hotkey", "char_id"), ("inventory", "char_id")):
                t.db.execute("DELETE FROM `%s` WHERE `%s` = %d" % (table, where, cid))
            t.db.execute("DELETE FROM `storage` WHERE `account_id` = %d" % aid)
            t.db.execute("INSERT INTO `skill` (`char_id`, `id`, `lv`, `flag`) VALUES %s"
                         % ", ".join("(%d, %d, %d, 0)" % (cid, k, v) for k, v in SKILLS.items()))
            t.db.execute("INSERT INTO `hotkey` (`char_id`, `hotkey`, `type`, `itemskill_id`, `skill_lvl`) VALUES %s"
                         % ", ".join("(%d, %d, %d, %d, %d)" % (cid, h, k, i, l) for h, k, i, l in HOTKEYS))
            for nameid, amount in ((JELLOPY, JELLOPIES), (RED_POTION, POTIONS)):
                t.db.execute("INSERT INTO `inventory` (`char_id`, `nameid`, `amount`, `identify`) "
                             "VALUES (%d, %d, %d, 1)" % (cid, nameid, amount))
        c = t.logs.char(name)
        s.state("char row", {k: c.get(k) for k in ("char_id", "class", "base_level", "job_level", "zeny",
                                                    "last_map", "last_x", "last_y")})
        s.rows("hotkey rows", t.db.select("hotkey", {"char_id": cid}), min_rows=len(HOTKEYS))
    return cid, aid


def run(t):
    user = t.param("user", "bfhsoak")
    password = t.param("password", "bfh-test")
    name = t.param("char", "BfhSoaker")
    seconds = float(t.param("seconds", "40" if t.mock else "3600"))
    warmup = seconds / 6.0
    t.client_timeout = max(t.client_timeout, seconds + 1800)

    cid, aid = set_up(t, user, password, name)
    with t.step("set-up: second player", "bifrost-bot, GM, its own account") as s:
        bot = Bot.start(t, "soak", t.param("bot_user", "bfhsoakgm"), "bfbot", t.param("bot_char", "BfhSoakGM"),
                        gm=True)
        s.note("mock: no second player" if bot is None else "bot %s in" % bot.char)

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    soak = Soak(t, ctl, cid, aid, bot)
    soak.name = name
    pid = t.client.proc.pid
    with t.step("play for %d s" % seconds, "M8.6: rounds of town, portals, shop, NPC, storage, combat, skills, "
                                           "items until the time is up; the client stays up") as s:
        s.frame("start")
        soak.memory = Memory(pid, t.mock)
        soak.memory.start()
        start = time.monotonic()
        while time.monotonic() - start < seconds and soak.alive():
            soak.round()
            soak.k += 1
            if soak.k == 1:
                s.frame("first-round")
        played = time.monotonic() - start
        s.note("%d rounds in %.0f s" % (soak.k, played))
        s.state("actions: [succeeded, failed]", soak.tally.ok)
        s.state("failed actions", soak.tally.bad[:80])
        s.state("not counted as failures", soak.notes)
        alive, why = soak.alive(), soak.dead
        in_game = alive and ctl.state("mode")["mode"] == "game"
        if alive:
            s.frame("end")

    leaks = None
    if not t.mock and alive and t.param("leaks", "1") == "1":
        with t.step("leaks", "recorded, not judged: macOS `leaks` on the running client") as s:
            try:
                r = subprocess.run(["leaks", str(pid)], capture_output=True, text=True, timeout=900)
                (t.run_dir / "leaks.txt").write_text(r.stdout + r.stderr)
                leaks = [l for l in r.stdout.splitlines() if re.search(r"leaks? for \d+ total leaked bytes", l)]
                s.state("leaks summary", leaks or r.stdout.splitlines()[:5])
            except (OSError, subprocess.TimeoutExpired) as e:
                s.note("leaks did not run: %s" % e)
    soak.memory.stop.set()
    soak.memory.join(timeout=5)
    samples = list(soak.memory.samples)

    client = t.client
    pkt_path = client.pkt_log
    t.stop_client()

    # The three verdicts are recorded side by side: a failed one does not
    # hide the others (a failed step would stop the script).
    def verdict(name, criterion, checks, evidence=()):
        st = t.step(name, criterion)
        st.evidence.extend(evidence)
        for ok_, text, data in checks:
            st.evidence.append({"kind": "check", "text": text, "ok": bool(ok_), "data": data})
        bad = [text for ok_, text, _ in checks if not ok_]
        st.status, st.error, st.seconds = ("fail", "; ".join(bad), 0) if bad else ("pass", None, 0)

    unknown = client.trace_lines("unknown packet header")
    asserts = client.trace_lines("assert", "ASSERT")
    crash = client.trace_lines("crash:")
    nameless, received = [], []
    if not t.mock:
        received = [p for p in pktlog.read(pkt_path) if p.dir == "R"]
        nameless = sorted(set(p.header for p in received if p.name == "?" and p.conn.split("/")[0] == "zone"))
    verdict("no crash, no unknown packet",
            "M8.6: the client up for the whole time and exit 0; no unknown packet header; every zone header named; "
            "no failed assertion",
            [(alive and in_game, "the client up and in game after %.0f s" % played, {"why": why}),
             (played >= seconds, "played the whole time (%.0f of %.0f s)" % (played, seconds), {}),
             (client.exit_code == 0, "the client exited with 0 when asked to quit", {"code": client.exit_code}),
             (not crash, "no crash line (%d)" % len(crash), {}),
             (not unknown, "no unknown packet header (%d lines)" % len(unknown), {}),
             (not nameless, "every header received on the zone connection is in the client's table",
              {"nameless": nameless}),
             (not asserts, "no failed assertion (%d lines)" % len(asserts), {})],
            [{"kind": "note", "text": "%d packets received" % len(received)},
             {"kind": "trace", "label": "unknown packet header lines", "count": len(unknown), "lines": unknown[-30:]},
             {"kind": "trace", "label": "assertion lines", "count": len(asserts), "lines": asserts[-30:]},
             {"kind": "trace", "label": "crash lines", "count": len(crash), "lines": crash[-30:]}])

    with open(t.run_dir / "rss.csv", "w") as f:
        f.write("seconds,rss_kb,activity,footprint_mb\n")
        for sec, kb, what, fp in samples:
            f.write("%s,%d,%s,%s\n" % (sec, kb, what, "" if fp is None else fp))
    stable, numbers = judge_memory(samples, warmup)
    ev = [{"kind": "state", "label": "rss samples (seconds, MB; footprint MB, recorded only)",
           "data": [(sec, round(kb / 1024.0, 1), fp) for sec, kb, _, fp in samples]},
          {"kind": "state", "label": "memory", "data": numbers}]
    if leaks is not None:
        ev.append({"kind": "state", "label": "leaks summary", "data": leaks})
    if t.mock and numbers.get("error"):
        stable = True
        ev.append({"kind": "note", "text": "mock: the series is the mock's, too short to judge"})
    verdict("stable memory", "M8.6: after a warm-up of the first sixth, RSS slope < %g MB per 10 min and no sample "
                             "above the warm-up peak + %g MB" % (SLOPE_MB_PER_10MIN, MARGIN_MB),
            [(stable, "RSS stable after the warm-up", {k: v for k, v in numbers.items() if k != "samples"})], ev)

    rates = {k: v[0] / float(sum(v)) for k, v in soak.tally.ok.items() if sum(v)}
    never = sorted(k for k, v in soak.tally.ok.items() if v[0] == 0)
    low = sorted("%s %.0f%%" % (k, 100 * r) for k, r in rates.items() if r < MIN_RATE)
    verdict("the session played", "every activity succeeded at least once and in %d%% of its tries" % (100 * MIN_RATE),
            [(soak.k >= 1, "%d rounds" % soak.k, {}),
             (not never, "every activity succeeded at least once", {"never": never}),
             (not low, "every activity succeeded in at least %d%% of its tries" % (100 * MIN_RATE), {"low": low})],
            [{"kind": "state", "label": "actions: [succeeded, failed]", "data": soak.tally.ok}])
