"""M8.1i the scripted gate test.

Done when (implementation plan, M8 exit): create a character, kill a Poring
at the south gate, pick up the drop, buy a potion and sell the loot, change
job at the Job Master, chat. Each step is confirmed in the server's SQL logs.
The same script, run with the address sanitizer build as BF_CLIENT, is the
second exit criterion (completes with no sanitizer report).

Set-up: a fresh group-99 account without characters (SQL); in game, by
atcommands typed through the client's chat line (atcommandlog): job level +9
and skill points -9 (rAthena's Job Master wants job level 10 and no unspent
points: `.SkillPointCheck`), 1000 zeny, and the moves between the scenes
(@warp prontera 156 29 north of the south gate, prt_in beside the Tool
Dealer, prontera beside the Job Master). The Poring is spawned beside the
character by `@monster 1002 1`, typed by the client, so that the kill does
not wait for the map's own Porings to wander by.

Played, through the control socket only: the character is created in the
client's creation window (Enter on the empty slot, the name typed, Enter);
the character walks through the south gate (the portal prt001, prontera
156,22, clicked) onto prt_fild08; the Poring is Ctrl-clicked until it dies
(bfharness/field.py, as combat.py); its drop is clicked the moment it lands
(Porings are looters: one standing by takes a drop within a second; if one
does, or nothing drops, another Poring is spawned, up to six); the
Tool Dealer's deal windows buy one Red Potion and sell the loot, the items
dragged and the amounts typed (npcplay.deal, as shop.py); the Job Master's
menus are answered (npcplay.run_dialogue, as npc.py); a line is said.

| Step | SQL evidence |
|---|---|
| create | charlog 'make new char' with the name; the char row |
| kill | picklog type M, char_id 1002 (the Poring's mob id), map prt_fild08; base exp after logout |
| pick up | picklog type P, the character, the item dropped, amount > 0 |
| buy | picklog type S, nameid 501, amount 1; zenylog type S, -50 |
| sell | picklog type S, the picked-up item, amount < 0; zenylog type S, > 0 |
| job change | the client's job 1; char.class = 1 after logout (Swordman) |
| chat | chatlog type O with the line, on prontera |

Server requirement: the Job Master is rAthena's npc/custom/jobmaster.txt
(prontera 153,193), loaded by port/server/import/map_conf.txt. Shop: Tool
Dealer#prt1 at prt_in 126,76.
"""

import time

from bfharness import npcplay
from bfharness.accounts import unique_name
from bfharness.field import find, grab_drop, kill_until_drop, ok, walk_into_portal
from bfharness.play import enter_game, login, own_home, say_and_confirm, token
from bfharness.sql import wait_rows

NAME = "gate"
MILESTONE = "M8"
DESCRIPTION = "the gate test: create, kill at the south gate, pick up, buy, sell, job change, chat"
MANIFEST = "port/harness/manifests/m8-gate.sha256"
NEEDS_SERVER = True

PORING, JELLOPY, RED_POTION, SWORDMAN = 1002, 909, 501, 1
NORTH_OF_GATE = ("prontera", 156, 29)
# the warp as the server's npc/ defines it: name, map, x, y, span x, span y, to map, x, y
SOUTH_GATE = ("prt001", "prontera", 156, 22, 3, 2, "prt_fild08", 170, 375)
DEALER = ("prt_in", 126, 76)
AT_DEALER = ("prt_in", 126, 72)
JOBMASTER = ("prontera", 153, 193)
AT_JOBMASTER = ("prontera", 153, 189)


def at(x, y):
    return lambda a: a["type"] == "npc" and a["x"] == x and a["y"] == y


def gm(t, s, ctl, command):
    """Set-up by atcommand typed through the client, confirmed in atcommandlog."""
    mark = t.logs.mark()
    npcplay.focus_chat(ctl)
    t.gm.run(command)
    return s.rows("atcommandlog %r" % command,
                  wait_rows(lambda: t.logs.atcommands(contains=command, after=mark), timeout=15))


def warp(t, s, ctl, place):
    gm(t, s, ctl, t.gm.warp(*place))
    ctl.wait({"map": place[0], "position": {"x": place[1], "y": place[2]}}, timeout=60)
    ctl.wait({"frames": 60}, timeout=20)


def run(t):
    userid = t.param("user") or unique_name(tag="g")
    password = t.param("password", "bfh-test")
    charname = t.param("name") or unique_name(tag="G")

    with t.step("set-up: GM account", "a fresh group-99 account without characters (SQL)") as s:
        aid = t.accounts.create_account(userid, password, group_id=99)
        s.note("account %s (%d)" % (userid, aid))
        s.equal(len(t.logs.chars(aid)), 0, "characters on the new account")

    ctl = t.launch(env=own_home(t))
    login(t, ctl, userid, password)

    with t.step("create a character", "gate: create, in the client's creation window") as s:
        mark = t.logs.mark()
        ctl.press("enter")                 # Enter on an empty slot opens creation
        ctl.wait_mode("makechar", 30)
        ctl.wait_window("MAKECHARWND")
        ctl.type_text(charname)
        s.frame("makechar")
        ctl.press("enter")
        ctl.wait_mode("charselect", 30)
        s.rows("charlog 'make new char' %s" % charname,
               wait_rows(lambda: t.logs.charlog(name=charname, msg="make new char", after=mark)))
        c0 = t.logs.char(charname)
        s.check(c0 is not None and c0["account_id"] == aid, "char row %s on account %d" % (charname, aid))
        s.state("char row", {k: c0[k] for k in ("char_id", "class", "base_level", "job_level", "base_exp",
                                                "zeny", "last_map", "last_x", "last_y")})
        char_id = c0["char_id"]

    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    with t.step("set-up: job level, zeny, north of the south gate",
                "atcommands typed by the client: @jlvl 9, @skpoint -9, @zeny 1000, @warp") as s:
        for cmd in (t.gm.job_level(9), "@skpoint -9", t.gm.zeny(1000)):
            gm(t, s, ctl, cmd)
        warp(t, s, ctl, NORTH_OF_GATE)
        s.state("self", ctl.state("self")["self"])

    with t.step("walk through the south gate", "gate: the kill is at the south gate (prt001 to prt_fild08)") as s:
        walk_into_portal(t, ctl, s, SOUTH_GATE)
        me = ctl.state("map", "self")
        s.state("map and self", me)
        s.equal(me["map"]["name"], SOUTH_GATE[6], "map after the gate")
        ctl.wait({"frames": 60}, timeout=20)
        s.frame("south-gate")

    # Porings are looters: one standing by takes a drop within a second, so
    # the drop is clicked the moment the Poring dies (field.grab_drop), and
    # another Poring is fought if the drop was lost to a looter
    mark = t.logs.mark()
    grabbed = {}

    def on_death(s, cell):
        grabbed["item"] = grab_drop(ctl, s, cell)

    def picked(drops):
        it = grabbed.pop("item", None)
        if not drops or not it:
            return False
        rows = wait_rows(lambda: t.logs.picked_up(char_id, it["job"], after=mark), timeout=10)
        if rows:
            grabbed.update(item=it, rows=rows, drops=drops)
        return bool(rows)

    kill_until_drop(t, ctl, PORING, SOUTH_GATE[6], tries=6, on_death=on_death, enough=picked)

    with t.step("the drop on the ground", "gate: kill; the Poring's drop lands beside it") as s:
        s.check(grabbed.get("drops"), "a Poring's drop picked up within 6 kills (Porings loot drops)")
        s.rows("picklog M from mob %d on %s" % (PORING, SOUTH_GATE[6]), grabbed["drops"])
        s.state("the item actor clicked", grabbed["item"])
        s.check(grabbed["item"]["job"] in set(r["nameid"] for r in grabbed["drops"]),
                "the item clicked is the Poring's drop")

    with t.step("pick up the drop", "gate: pick up (picklog P)") as s:
        loot = grabbed["item"]["job"]
        s.rows("picklog P item %d by %d" % (loot, char_id), grabbed["rows"])
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and find(ctl, grabbed["item"]["id"]):
            time.sleep(0.5)
        ok(s, not find(ctl, grabbed["item"]["id"]), "the item is gone from the ground")
        s.frame("picked-up")

    with t.step("buy a potion", "gate: buy at the Tool Dealer (picklog S +, zenylog S -)") as s:
        warp(t, s, ctl, AT_DEALER)
        mark = t.logs.mark()
        npcplay.deal(ctl, at(*DEALER[1:]), "buy", RED_POTION, 1, s, frames={"list": "buy"})
        rows = s.rows("picklog S Red Potion +", wait_rows(lambda: t.logs.bought(char_id, RED_POTION, after=mark)))
        s.equal(rows[-1]["amount"], 1, "picklog amount")
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.check(z[-1]["amount"] < 0, "zeny paid", amount=z[-1]["amount"])

    with t.step("sell the loot", "gate: sell the Poring's drop (picklog S -, zenylog S +)") as s:
        mark = t.logs.mark()
        npcplay.deal(ctl, at(*DEALER[1:]), "sell", loot, 1, s, frames={"list": "sell"})
        rows = s.rows("picklog S item %d -" % loot, wait_rows(lambda: t.logs.sold(char_id, loot, after=mark)))
        s.equal(rows[-1]["amount"], -1, "picklog amount")
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.check(z[-1]["amount"] > 0, "zeny received", amount=z[-1]["amount"])

    with t.step("change job", "gate: the Job Master makes a Swordman") as s:
        warp(t, s, ctl, AT_JOBMASTER)
        npcplay.click_npc(ctl, at(*JOBMASTER[1:]), "Job Master")
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        npcplay.run_dialogue(ctl, s, answers=["Swordsman", "Change into"], frames={"menu": "job-master"})
        me = None
        for _ in range(30):
            me = ctl.state("self")["self"]
            if me["job"] == SWORDMAN:
                break
            ctl.wait({"frames": 10}, timeout=10)
        s.equal(me["job"], SWORDMAN, "job the client shows after the change")
        ctl.wait({"frames": 30}, timeout=10)
        s.frame("swordman")

    with t.step("chat", "gate: a public line, in chatlog") as s:
        npcplay.focus_chat(ctl)
        say_and_confirm(t, ctl, s, "bfh gate %s" % token(), char_id, map=JOBMASTER[0])

    t.stop_client()
    with t.step("saved on logout", "char.class = Swordman; experience from the kill; the potion") as s:
        rows = s.rows("char %s class %d" % (charname, SWORDMAN),
                      wait_rows(lambda: [r for r in [t.logs.char(charname)] if r and not r["online"]
                                         and r["class"] == SWORDMAN]))
        c1 = rows[-1]
        s.state("char before and after", {k: (c0[k], c1[k]) for k in ("class", "base_level", "base_exp",
                                                                       "job_level", "zeny")})
        ok(s, (c1["base_level"], c1["base_exp"]) > (c0["base_level"], c0["base_exp"]),
           "base experience gained (the kill)")
        s.rows("inventory: Red Potion", t.logs.inventory(char_id, RED_POTION))
