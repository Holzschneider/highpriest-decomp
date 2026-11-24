"""M9.5: the coverage report of an English session.

Done when (implementation plan, M9): login, character creation, the gate test
of M8 and the option windows show no Korean in captured frames, and the
coverage report for that session is empty. The report (BF_COVERAGE, the
control socket's `coverage` part; port/native/strings/include/bf_coverage.h)
lists every displayed literal that fell back to Korean (`bf_tr`), every table
lookup that fell back to a placeholder name, and every text the client drew
that is Korean or outside Windows-1252, with where it came from. This script
passes only when the list is empty; when it is not, the last step lists every
entry, which is the point of running it.

Set-up: a fresh group-99 account in SQL, no character. Played through the
client: login; character creation (the name typed, confirmed in charlog);
entering Prontera; the main windows by their shortcuts (Alt+O options, Alt+E
items, Alt+Q equipment, Alt+S skills, Alt+A status) and the party window
(`/organize`, the sharing rules, the party window it opens; @allskill as set-up gives the
Novice the Basic Skill a party needs); the Job Master's dialogue to
Swordsman (job level 10 by @jlvl as set-up, before @allskill uses the points); a
Poring killed and its drop picked up at the south gate (as in the gate test,
reached by @warp); the Tool Dealer's deal type and
purchase windows in prt_in (reached by @warp as set-up); a chat line in
chatlog. Frames of every window go under the game folder.
"""

import json
import time

from bfharness import npcplay
from bfharness.accounts import unique_name
from bfharness.play import enter_game, login, own_home, say_and_confirm, token
from bfharness.sql import wait_rows

NAME = "english_session"
MILESTONE = "M9"
DESCRIPTION = "English session: login, creation, windows, NPC, shop, chat; the coverage report is empty"
MANIFEST = "port/harness/manifests/m9-english-session.sha256"
NEEDS_SERVER = True

SWORDMAN = 1
PORING = 1002
SOUTH_GATE = ("prt_fild08", 170, 372)
JOBMASTER = ("prontera", 153, 193)
DEALER = ("prt_in", 126, 76)
DEALER_START = ("prt_in", 126, 72)
WINDOWS = [("o", "OPTIONWND"), ("e", "ITEMWND"), ("q", "EQUIPWND"), ("s", "SKILLLISTWND"), ("a", "STATUSWND")]


def at(x, y):
    return lambda a: a["type"] == "npc" and (a["x"], a["y"]) == (x, y)


def gm(t, s, ctl, command):
    """Set-up by atcommand typed into the chat line, confirmed in atcommandlog."""
    mark = t.logs.mark()
    npcplay.focus_chat(ctl)
    t.gm.run(command)
    s.rows("atcommandlog %r" % command, wait_rows(lambda: t.logs.atcommands(contains=command, after=mark)))


def toggle(ctl, s, key, name, frame):
    ctl.press(key, "alt")
    ctl.wait_window(name, timeout=10)
    ctl.wait({"frames": 10}, timeout=10)
    s.frame(frame)
    ctl.press(key, "alt")
    ctl.wait_window(name, open=False, timeout=10)


def describe(e):
    out = "%s: %s" % (e["kind"], e["text"])
    if e.get("drawn"):
        out += " (drawn as %s)" % e["drawn"]
    if e.get("table"):
        out += " [table %s]" % e["table"]
    out += " x%d" % e["count"]
    if e.get("where"):
        out += " at " + e["where"]
    return out


def run(t):
    userid = t.param("user") or unique_name(tag="cv")
    password = t.param("password", "bfh-test")
    charname = t.param("name") or unique_name(tag="cvn")
    report_path = t.run_dir / "coverage.json"

    with t.step("set-up: GM account", "group 99 (atcommands set the scenes up); no character yet") as s:
        aid = t.accounts.create_account(userid, password, group_id=99)
        s.note("account %s (%d)" % (userid, aid))

    env = own_home(t)
    env["BF_COVERAGE"] = str(report_path)
    ctl = t.launch(env=env)
    login(t, ctl, userid, password)

    with t.step("character creation", "made through the client; charlog 'make new char'") as s:
        mark = t.logs.mark()
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("charselect-empty")
        ctl.press("enter")                 # Enter on an empty slot opens creation
        ctl.wait_mode("makechar", 30)
        ctl.wait_window("MAKECHARWND")
        ctl.click_control("MAKECHARWND", "name_edit")
        ctl.type_text(charname)
        ctl.wait({"frames": 5}, timeout=10)
        s.frame("makechar")
        ctl.press("enter")
        ctl.wait_mode("charselect", 30)
        s.rows("charlog 'make new char' %s" % charname,
               wait_rows(lambda: t.logs.charlog(name=charname, msg="make new char", after=mark)))
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("charselect")
        char_id = t.logs.char(charname)["char_id"]

    enter_game(t, ctl)
    with t.step("in the world", "the basic windows of game mode; @allskill as set-up (a party needs Basic Skill 7)") as s:
        ctl.wait({"frames": 60}, timeout=20)
        s.frame("world")
        gm(t, s, ctl, t.gm.job_level(9))   # the Job Master's requirement, before the skills use up the points
        gm(t, s, ctl, "@allskill")
        ctl.wait({"frames": 10}, timeout=10)

    with t.step("main windows", "M9: Alt+O options, Alt+E items, Alt+Q equipment, Alt+S skills, Alt+A status") as s:
        for key, name in WINDOWS:
            toggle(ctl, s, key, name, "window-" + name.lower())

    with t.step("party window", "/organize, the sharing rules, the party in SQL; the party window") as s:
        party = unique_name("BfP")[:20]
        npcplay.focus_chat(ctl)
        ctl.say("/organize %s" % party)
        ctl.wait_window("PARTYSETTINGWND", timeout=15)
        ctl.wait({"frames": 5}, timeout=10)
        s.frame("party-settings")
        ctl.click_control("PARTYSETTINGWND", "ok")
        s.rows("party row %s" % party, wait_rows(lambda: t.db.select("party", {"name": party}), timeout=15))
        # The party window opens by itself when the party is made
        # (ZC_ACK_MAKE_GROUP); the 2008 client's Alt+Z, its button's tooltip,
        # is not in the reconstruction's UIWindowMgr::ProcessPushButton.
        ctl.wait_window("MESSENGERGROUPWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("party-window")
        # Closed by its close button (UIMessengerGroupWnd::OnCreate: at (w - 14, 3)),
        # so that it does not cover the NPCs clicked next.
        x, y, w, h = ctl.window("MESSENGERGROUPWND")["rect"]
        ctl.click(x + w - 14 + 4, y + 3 + 4)
        ctl.wait_window("MESSENGERGROUPWND", open=False, timeout=10)

    with t.step("NPC dialogue", "the Job Master's dialogue and menus to Swordsman (job level set up by @jlvl before)") as s:
        npcplay.click_npc(ctl, at(*JOBMASTER[1:]), "Job Master")
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("jobmaster-dialogue")
        npcplay.run_dialogue(ctl, s, answers=["Swordsman", "Change into"], frames={"menu": "jobmaster-menu"})
        deadline = time.monotonic() + 15
        while ctl.state("self")["self"]["job"] != SWORDMAN and time.monotonic() < deadline:
            ctl.wait({"frames": 10}, timeout=10)
        s.equal(ctl.state("self")["self"]["job"], SWORDMAN, "job reported by the client")

    with t.step("kill and pick up", "the gate's Poring at the south gate (@warp as set-up): picklog M, then P") as s:
        gm(t, s, ctl, t.gm.warp(*SOUTH_GATE))
        ctl.wait_map(SOUTH_GATE[0], 60)
        ctl.wait({"frames": 60}, timeout=20)
        mark = t.logs.mark()
        deadline = time.monotonic() + 120
        drops = []
        while not drops and time.monotonic() < deadline:
            a = ctl.actor(lambda a: a["type"] == "monster" and a["job"] == PORING)
            if a and a.get("screen"):
                ctl.click(a["screen"]["x"], a["screen"]["y"])
            drops = wait_rows(lambda: t.logs.monster_drops(PORING, map=SOUTH_GATE[0], after=mark), timeout=3)
        s.rows("picklog M from mob %d on %s" % (PORING, SOUTH_GATE[0]), drops)
        s.frame("poring-killed")
        mark = t.logs.mark()
        deadline = time.monotonic() + 30
        rows = []
        while not rows and time.monotonic() < deadline:
            a = ctl.actor(lambda a: a["type"] == "item")
            if a and a.get("screen"):
                ctl.click(a["screen"]["x"], a["screen"]["y"])
            rows = wait_rows(lambda: t.logs.picked_up(char_id, after=mark), timeout=3)
        s.rows("picklog P by %d" % char_id, rows)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("picked-up")
        toggle(ctl, s, "e", "ITEMWND", "window-itemwnd-loot")

    with t.step("shop windows", "the Tool Dealer's deal type and purchase windows (prt_in by @warp)") as s:
        gm(t, s, ctl, t.gm.warp(*DEALER_START))
        ctl.wait({"position": {"x": DEALER_START[1], "y": DEALER_START[2]}}, timeout=60)
        ctl.wait({"frames": 60}, timeout=20)
        npcplay.click_npc(ctl, at(*DEALER[1:]), "Tool Dealer")
        ctl.wait_window("CHOOSESELLBUYWND", timeout=15)
        ctl.wait({"frames": 5}, timeout=10)
        s.frame("shop-dealtype")
        ctl.click_control("CHOOSESELLBUYWND", "buy")
        ctl.wait_window("ITEMPURCHASEWND", timeout=15)
        npcplay.wait_control(ctl, "ITEMSHOPWND", "item:501")
        ctl.wait({"frames": 5}, timeout=10)
        s.frame("shop-buy")
        ctl.click_control("ITEMPURCHASEWND", "cancel")
        ctl.wait_window("ITEMPURCHASEWND", open=False, timeout=10)

    with t.step("chat", "a public line, in chatlog") as s:
        npcplay.focus_chat(ctl)
        say_and_confirm(t, ctl, s, "bfh english %s" % token(), char_id)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("chat")

    with t.step("coverage part", "the control socket's `coverage` part during the session") as s:
        live = ctl.state("coverage")["coverage"]
        s.state("coverage counts", {k: live[k] for k in ("entries", "untranslated", "placeholder", "korean",
                                                          "foreign")})

    t.stop_client()

    with t.step("coverage report", "M9.5: the report written at exit (BF_COVERAGE) is empty") as s:
        s.check(report_path.exists(), "report written to %s" % report_path)
        report = json.loads(report_path.read_text())
        s.state("counts", {k: report[k] for k in ("entries", "untranslated", "placeholder", "korean", "foreign",
                                                   "strings")})
        s.state("entries", [describe(e) for e in report["list"]])
        s.equal(report["entries"], len(report["list"]), "entries listed")
        s.equal(report["entries"], 0, "entries in the coverage report")
