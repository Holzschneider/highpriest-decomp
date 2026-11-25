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

The session covers the criterion exactly, in one client launch: login (the
login window, the server list, character selection); character creation
(the name typed, confirmed in charlog); the gate test of M8 as gate.py plays
it (through the south gate prt001 on foot, a Poring killed and its drop
picked up, a Red Potion bought and the loot sold at the Tool Dealer, the Job
Master's dialogue to Swordman, a chat line), each step confirmed in the
server's SQL logs; and every option window and tab: the option window (Alt+O)
with its base button and "info." tab (which change nothing, as in the 2008
client: UIOptionWnd::SendMsg, reference 0x478cd0, has no case for the base
button and nothing sets the page that draws optwin1_bg), its skin list and
its mini state; the chat window's battle message options, scrolled; Arrange
Detail (Alt+D); Friend Setup (Alt+I); the Escape menu (QUITWND); the item
window's three tabs with the loot; equipment, skills and status (Alt+Q/S/A),
the status and skills again as a Swordman; the party settings and party
window (`/organize`). Frames of every window go under the game folder.

Set-up: a fresh group-99 account in SQL, no character; in game, by
atcommands typed through the client's chat line (atcommandlog): job level +9,
@allskill (Basic Skill for the party; it leaves no unspent point for the Job
Master's check), 1000 zeny, and the moves between the scenes (@warp north of
the south gate, beside the Tool Dealer, beside the Job Master), as gate.py.
"""

import json
import time

from bfharness import npcplay
from bfharness.accounts import unique_name
from bfharness.field import find, grab_drop, kill_until_drop, ok, walk_into_portal
from bfharness.play import enter_game, login, own_home, say_and_confirm, token
from bfharness.sql import wait_rows

NAME = "english_session"
MILESTONE = "M9"
DESCRIPTION = "English session: login, creation, the gate test, every option window; the coverage report is empty"
MANIFEST = "port/harness/manifests/m9-english-session.sha256"
NEEDS_SERVER = True

PORING, RED_POTION, SWORDMAN = 1002, 501, 1
NORTH_OF_GATE = ("prontera", 156, 29)
# the warp as the server's npc/ defines it: name, map, x, y, span x, span y, to map, x, y
SOUTH_GATE = ("prt001", "prontera", 156, 22, 3, 2, "prt_fild08", 170, 375)
DEALER = ("prt_in", 126, 76)
AT_DEALER = ("prt_in", 126, 72)
JOBMASTER = ("prontera", 153, 193)
AT_JOBMASTER = ("prontera", 153, 189)
MAIN_WINDOWS = [("e", "ITEMWND"), ("q", "EQUIPWND"), ("s", "SKILLLISTWND"), ("a", "STATUSWND")]


def at(x, y):
    return lambda a: a["type"] == "npc" and (a["x"], a["y"]) == (x, y)


def gm(t, s, ctl, command):
    """Set-up by atcommand typed into the chat line, confirmed in atcommandlog."""
    mark = t.logs.mark()
    npcplay.focus_chat(ctl)
    t.gm.run(command)
    return s.rows("atcommandlog %r" % command,
                  wait_rows(lambda: t.logs.atcommands(contains=command, after=mark), timeout=15))


def warp(t, s, ctl, place):
    gm(t, s, ctl, t.gm.warp(*place))
    ctl.wait({"map": place[0], "position": {"x": place[1], "y": place[2]}}, timeout=60)
    ctl.wait({"frames": 60}, timeout=20)


def snap(ctl, s, name, n=10):
    ctl.wait({"frames": n}, timeout=10)
    s.frame(name)


def names(ctl):
    return sorted(w["name"] for w in ctl.state("windows")["windows"])


def toggle(ctl, s, key, name, frame):
    ctl.press(key, "alt")
    ctl.wait_window(name, timeout=10)
    snap(ctl, s, frame)
    s.state("%s controls" % name, [c["id"] for c in ctl.window(name).get("controls", [])])
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


def option_window(t, ctl, s):
    """Alt+O: the window, its base button and info. tab, the skin list, mini."""
    ctl.press("o", "alt")
    ctl.wait_window("OPTIONWND", timeout=10)
    snap(ctl, s, "opt-optionwnd")
    before = ctl.window("OPTIONWND")
    s.state("OPTIONWND controls", [c["id"] for c in before["controls"]])
    open_before = names(ctl)

    # the base button and the info. tab change nothing, as in 2008
    ctl.click_control("OPTIONWND", "base")
    snap(ctl, s, "opt-optionwnd-base")
    x, y, w, h = before["rect"]
    ctl.click(x + 8, y + 50)                  # the "info." tab drawn on optwin0_bg
    snap(ctl, s, "opt-optionwnd-info-tab")
    after = ctl.window("OPTIONWND")
    s.equal(after["rect"], before["rect"], "OPTIONWND after the base button and the info. tab (2008: no page change)")
    s.equal(names(ctl), open_before, "windows after the base button and the info. tab")

    # the skin list
    ctl.click_control("OPTIONWND", "skinselectcombo")
    ctl.wait_window("COMBOBOXWND", timeout=10)
    snap(ctl, s, "opt-skin-list")
    cx, cy, cw, ch = ctl.window("COMBOBOXWND")["rect"]
    ctl.click(cx + 20, cy + 6)                # the first entry, <Basic Skin>, the current one
    ctl.wait_window("COMBOBOXWND", open=False, timeout=10)
    snap(ctl, s, "opt-after-skin")

    # mini and back
    ctl.click_control("OPTIONWND", "mini")
    ctl.wait({"frames": 10}, timeout=10)
    s.equal(ctl.window("OPTIONWND")["rect"][3], 17, "OPTIONWND height when mini")
    snap(ctl, s, "opt-optionwnd-mini")
    ctl.click_control("OPTIONWND", "mini")
    ctl.wait({"frames": 10}, timeout=10)
    s.equal(ctl.window("OPTIONWND")["rect"][3], before["rect"][3], "OPTIONWND height restored")
    ctl.press("o", "alt")
    ctl.wait_window("OPTIONWND", open=False, timeout=10)


def battle_options(ctl, s):
    """The chat window's option button: the battle message options, scrolled."""
    ctl.click_control("CHATWND", "option")
    ctl.wait_window("BATTLEMSG_OPTIONWND", timeout=10)
    snap(ctl, s, "opt-battlemsg")
    x, y, w, h = ctl.window("BATTLEMSG_OPTIONWND")["rect"]
    for _ in range(8):                        # the scroll bar's down arrow, to the last entries
        ctl.click(x + w - 12, y + 165)
        ctl.wait({"frames": 3}, timeout=10)
    snap(ctl, s, "opt-battlemsg-scrolled")
    ctl.click_control("CHATWND", "option")
    ctl.wait_window("BATTLEMSG_OPTIONWND", open=False, timeout=10)


def escape_menu(ctl, s):
    ctl.press("escape")
    ctl.wait_window("QUITWND", timeout=10)
    snap(ctl, s, "opt-escape-menu")
    ids = [c["id"] for c in ctl.window("QUITWND")["controls"]]
    s.check(all(i in ids for i in ("restart", "exit", "cancel")), "QUITWND: character select, exit, cancel", ids=ids)
    ctl.click_control("QUITWND", "cancel")
    ctl.wait_window("QUITWND", open=False, timeout=10)


def run(t):
    userid = t.param("user") or unique_name(tag="cv")
    password = t.param("password", "bfh-test")
    charname = t.param("name") or unique_name(tag="cvn")
    report_path = t.run_dir / "coverage.json"

    with t.step("set-up: GM account", "group 99 (atcommands set the scenes up); no character yet") as s:
        aid = t.accounts.create_account(userid, password, group_id=99)
        s.note("account %s (%d)" % (userid, aid))
        s.equal(len(t.logs.chars(aid)), 0, "characters on the new account")

    env = own_home(t)
    env["BF_COVERAGE"] = str(report_path)
    ctl = t.launch(env=env)
    with t.step("login window", "M9: the login screen in English") as s:
        ctl.wait_mode("login", 60)
        snap(ctl, s, "login", 30)
    login(t, ctl, userid, password)

    with t.step("character creation", "made through the client; charlog 'make new char'") as s:
        mark = t.logs.mark()
        snap(ctl, s, "charselect-empty")
        ctl.press("enter")                 # Enter on an empty slot opens creation
        ctl.wait_mode("makechar", 30)
        ctl.wait_window("MAKECHARWND")
        snap(ctl, s, "makechar-empty")
        ctl.click_control("MAKECHARWND", "name_edit")
        ctl.type_text(charname)
        snap(ctl, s, "makechar", 5)
        ctl.press("enter")
        ctl.wait_mode("charselect", 30)
        s.rows("charlog 'make new char' %s" % charname,
               wait_rows(lambda: t.logs.charlog(name=charname, msg="make new char", after=mark)))
        c0 = t.logs.char(charname)
        s.check(c0 is not None and c0["account_id"] == aid, "char row %s on account %d" % (charname, aid))
        char_id = c0["char_id"]
        snap(ctl, s, "charselect")

    enter_game(t, ctl)
    with t.step("in the world", "set-up: @jlvl 9 (the Job Master's requirement), @allskill (Basic Skill for a "
                                "party; no unspent points), @zeny 1000") as s:
        snap(ctl, s, "world", 60)
        for cmd in (t.gm.job_level(9), "@allskill", t.gm.zeny(1000)):
            gm(t, s, ctl, cmd)
        ctl.wait({"frames": 10}, timeout=10)

    with t.step("option window", "Alt+O: base button, info. tab, skin list, mini") as s:
        option_window(t, ctl, s)

    with t.step("other option windows", "battle message options, Arrange Detail (Alt+D), Friend Setup (Alt+I), "
                                        "the Escape menu") as s:
        battle_options(ctl, s)
        toggle(ctl, s, "d", "DETAILLEVELWND", "opt-detaillevel")
        toggle(ctl, s, "i", "FRIENDOPTIONWND", "opt-friendoption")
        escape_menu(ctl, s)

    with t.step("main windows", "Alt+E items, Alt+Q equipment, Alt+S skills, Alt+A status") as s:
        for key, name in MAIN_WINDOWS:
            toggle(ctl, s, key, name, "window-" + name.lower())

    with t.step("party window", "/organize, the sharing rules, the party in SQL; the party window") as s:
        party = unique_name("BfP")[:20]
        npcplay.focus_chat(ctl)
        ctl.say("/organize %s" % party)
        ctl.wait_window("PARTYSETTINGWND", timeout=15)
        snap(ctl, s, "party-settings", 5)
        ctl.click_control("PARTYSETTINGWND", "ok")
        s.rows("party row %s" % party, wait_rows(lambda: t.db.select("party", {"name": party}), timeout=15))
        # the party window opens by itself when the party is made (ZC_ACK_MAKE_GROUP)
        ctl.wait_window("MESSENGERGROUPWND", timeout=15)
        snap(ctl, s, "party-window")
        # closed by its close button (UIMessengerGroupWnd::OnCreate: at (w - 14, 3))
        x, y, w, h = ctl.window("MESSENGERGROUPWND")["rect"]
        ctl.click(x + w - 14 + 4, y + 3 + 4)
        ctl.wait_window("MESSENGERGROUPWND", open=False, timeout=10)

    with t.step("walk through the south gate", "gate: @warp north of it, the portal prt001 to prt_fild08") as s:
        warp(t, s, ctl, NORTH_OF_GATE)
        walk_into_portal(t, ctl, s, SOUTH_GATE)
        s.equal(ctl.state("map")["map"]["name"], SOUTH_GATE[6], "map after the gate")
        snap(ctl, s, "south-gate", 60)

    # Porings are looters: the drop is clicked the moment the Poring dies
    # (field.grab_drop), and another Poring is fought if a looter took it
    mark = t.logs.mark()
    grabbed = {}

    def on_death(s, cell):
        grabbed["item"] = grab_drop(ctl, s, cell)

    def picked(drops):
        it = grabbed.pop("item", None)
        if not drops or not it:
            return False
        rows = wait_rows(lambda: t.logs.picked_up(char_id, it["job"], after=mark), timeout=20)
        if rows:
            grabbed.update(item=it, rows=rows, drops=drops)
        return bool(rows)

    kill_until_drop(t, ctl, PORING, SOUTH_GATE[6], tries=6, on_death=on_death, enough=picked)

    with t.step("kill and pick up", "gate: picklog M from the Poring on prt_fild08, then P by the character") as s:
        s.check(grabbed.get("drops"), "a Poring's drop picked up within 6 kills (Porings loot drops)")
        s.rows("picklog M from mob %d on %s" % (PORING, SOUTH_GATE[6]), grabbed["drops"])
        loot = grabbed["item"]["job"]
        s.rows("picklog P item %d by %d" % (loot, char_id), grabbed["rows"])
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and find(ctl, grabbed["item"]["id"]):
            time.sleep(0.5)
        ok(s, not find(ctl, grabbed["item"]["id"]), "the item is gone from the ground")
        snap(ctl, s, "picked-up")
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", timeout=10)
        for tab in ("item", "equip", "etc"):
            npcplay.click_tab(ctl, "ITEMWND", tab)
            snap(ctl, s, "itemwnd-" + tab)
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", open=False, timeout=10)

    with t.step("buy a potion", "gate: the Tool Dealer (picklog S +, zenylog S -)") as s:
        warp(t, s, ctl, AT_DEALER)
        mark = t.logs.mark()
        npcplay.deal(ctl, at(*DEALER[1:]), "buy", RED_POTION, 1, s, frames={"list": "shop-buy", "amount": "shop-amount"})
        rows = s.rows("picklog S Red Potion +", wait_rows(lambda: t.logs.bought(char_id, RED_POTION, after=mark)))
        s.equal(rows[-1]["amount"], 1, "picklog amount")
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.check(z[-1]["amount"] < 0, "zeny paid", amount=z[-1]["amount"])

    with t.step("sell the loot", "gate: the Poring's drop (picklog S -, zenylog S +)") as s:
        mark = t.logs.mark()
        npcplay.deal(ctl, at(*DEALER[1:]), "sell", loot, 1, s, frames={"list": "shop-sell"})
        rows = s.rows("picklog S item %d -" % loot, wait_rows(lambda: t.logs.sold(char_id, loot, after=mark)))
        s.equal(rows[-1]["amount"], -1, "picklog amount")
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.check(z[-1]["amount"] > 0, "zeny received", amount=z[-1]["amount"])

    with t.step("change job", "gate: the Job Master's dialogue and menus to Swordman") as s:
        warp(t, s, ctl, AT_JOBMASTER)
        npcplay.click_npc(ctl, at(*JOBMASTER[1:]), "Job Master")
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        snap(ctl, s, "jobmaster-dialogue")
        npcplay.run_dialogue(ctl, s, answers=["Swordsman", "Change into"], frames={"menu": "jobmaster-menu"})
        me = None
        for _ in range(30):
            me = ctl.state("self")["self"]
            if me["job"] == SWORDMAN:
                break
            ctl.wait({"frames": 10}, timeout=10)
        s.equal(me["job"], SWORDMAN, "job the client shows after the change")
        snap(ctl, s, "swordman", 30)
        toggle(ctl, s, "a", "STATUSWND", "swordman-status")
        toggle(ctl, s, "s", "SKILLLISTWND", "swordman-skills")

    with t.step("chat", "gate: a public line, in chatlog") as s:
        npcplay.focus_chat(ctl)
        say_and_confirm(t, ctl, s, "bfh english %s" % token(), char_id, map=JOBMASTER[0])
        snap(ctl, s, "chat")

    with t.step("coverage part", "the control socket's `coverage` part during the session") as s:
        live = ctl.state("coverage")["coverage"]
        s.state("coverage counts", {k: live[k] for k in ("entries", "untranslated", "placeholder", "korean",
                                                          "foreign")})

    t.stop_client()

    with t.step("saved on logout", "gate: char.class = Swordman after logout") as s:
        rows = s.rows("char %s class %d" % (charname, SWORDMAN),
                      wait_rows(lambda: [r for r in [t.logs.char(charname)] if r and not r["online"]
                                         and r["class"] == SWORDMAN]))
        s.state("char before and after", {k: (c0[k], rows[-1][k]) for k in ("class", "base_level", "base_exp",
                                                                             "job_level", "zeny")})

    with t.step("coverage report", "M9.5: the report written at exit (BF_COVERAGE) is empty") as s:
        s.check(report_path.exists(), "report written to %s" % report_path)
        report = json.loads(report_path.read_text())
        s.state("counts", {k: report[k] for k in ("entries", "untranslated", "placeholder", "korean", "foreign",
                                                   "strings")})
        s.state("entries", [describe(e) for e in report["list"]])
        s.equal(report["entries"], len(report["list"]), "entries listed")
        s.equal(report["entries"], 0, "entries in the coverage report")
