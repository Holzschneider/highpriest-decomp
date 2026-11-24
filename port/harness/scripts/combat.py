"""M8.1c combat and damage numbers; items on the ground.

Done when (checklist M8.1c): the client kills a monster by clicking it, its
damage numbers are drawn, and the server agrees that the monster died and
the character gained experience; the monster's drop lies on the ground as an
item actor, the client picks it up by clicking it, and the server logs the
pickup and holds the item in the inventory.

Set-up: a fresh group-99 account and its Novice (bifrost-bot creates the
character and logs out again; SQL places it on prt_fild08). In game, the
client types `@monster 1002 1` (a Poring beside the character; set-up by
atcommand, in atcommandlog).

Played: the Poring is clicked with Ctrl held. The 2008 client attacks while
the button is held (ATTM_REPEAT_TILL_BUTTONUP) and goes on attacking after
the button is up only with Ctrl or the /noctrl option (ATTM_REPEAT,
GameMode.cpp ProcessObjClick); so one Ctrl-click attacks until the Poring
dies (re-clicked if it walks off while the character is not attacking).
The `damage` event (one per number the client draws, CGameActor::
Am_Make_Number) names the target and the number; the frames show them.

Server: the drop is logged in picklog (type M, char_id 1002 = the mob id,
the map) when the Poring dies; the experience is the `char` row's base_exp
and job_exp after logout against the row before login. The ground item is
the actor of type `item` at the Poring's cell; a click on it sends
CZ_ITEM_PICKUP and the server logs picklog type P for the character, and the
`inventory` table holds the item after logout.
"""

import re
import time

from bfharness.accounts import unique_name
from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home
from bfharness.sql import wait_rows

NAME = "combat"
MILESTONE = "M8"
DESCRIPTION = "kill a Poring by clicking, damage numbers, exp; pick up its drop"
MANIFEST = "port/harness/manifests/m8-combat.sha256"
NEEDS_SERVER = True

PORING = 1002
START = ("prt_fild08", 170, 375)


def ok(s, cond, message):
    s.check(cond, message, passed=bool(cond))


def setup_character(t, user, password, name, sex="M"):
    """A fresh group-99 account with a Novice, placed at START."""
    with t.step("set-up: account and character", "group 99; the character created by bifrost-bot, placed by SQL") as s:
        aid = t.accounts.create_account(user, password, sex=sex, group_id=99)
        s.note("account %s (%d)" % (user, aid))
        if not t.mock:
            bot = Bot.start(t, "create", user, password, name, gm=True)
            s.state("created by the bot", bot.entered)
            bot.quit()
            wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]], timeout=20)
        else:
            if not t.logs.char(name):
                t.mockdb.put("char", char_id=max([r["char_id"] for r in t.mockdb.rows("char")] or [150000]) + 1,
                             account_id=aid, char_num=0, name=name, **{"class": 0}, base_level=1, job_level=1,
                             base_exp=0, job_exp=0, zeny=0, last_map="prontera", last_x=155, last_y=183,
                             str=5, agi=5, vit=5, int=5, dex=5, luk=5, status_point=0,
                             hp=40, max_hp=40, sp=11, max_sp=11, online=0)
        c = t.accounts.set_char(name, map=START[0], x=START[1], y=START[2])
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "base_exp", "job_exp")})
        return c


def own_poring(t, ctl, s, me):
    """Spawn a Poring by atcommand (set-up) and return the nearest one."""
    before = set(a["id"] for a in ctl.state("actors")["actors"])
    mark = t.logs.mark()
    t.gm.run(t.gm.monster(PORING, 1))
    s.rows("atcommandlog @monster", wait_rows(lambda: t.logs.atcommands(contains="@monster", after=mark), timeout=10))
    deadline = time.monotonic() + 10
    new = []
    while time.monotonic() < deadline and not new:
        ctl.wait({"frames": 5}, timeout=10)
        new = [a for a in ctl.state("actors")["actors"]
               if a["type"] == "monster" and a["job"] == PORING and a["id"] not in before]
    mons = new or [a for a in ctl.state("actors")["actors"] if a["type"] == "monster" and a["job"] == PORING]
    mons.sort(key=lambda a: abs(a["x"] - me["x"]) + abs(a["y"] - me["y"]))
    return mons[0] if mons else None


def find(ctl, aid):
    a = [x for x in ctl.state("actors")["actors"] if x["id"] == aid]
    return a[0] if a else None


def kill(t, ctl, n, target):
    """Ctrl-click the Poring once; click again only when no number has been
    drawn on it for 6 s (it wandered off). Returns (died, last cell, drops)."""
    with t.step("attack Poring %d until it dies" % n,
                "M8.1c: one Ctrl-click attacks until the monster dies; damage numbers drawn; the server logs the death") as s:
        mark = t.logs.mark()
        after = ctl.last_seq
        deadline = time.monotonic() + 90
        clicks, gone, shot, seen = 0, False, False, 0
        cell = (target["x"], target["y"])
        last = 0
        while time.monotonic() < deadline:
            a = find(ctl, target["id"])
            if not a:
                gone = True
                break
            cell = (a["x"], a["y"])
            hits = [e for e in ctl.events_named("damage", after) if e["target"] == target["id"]]
            if hits and not shot:
                s.frame("damage-number-%d" % n)
                shot = True
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
        dmg = ctl.events_named("damage", after)
        on_poring = [e for e in dmg if e.get("target") == target["id"]]
        s.state("damage events (target, number, kind)", [(e["target"], e["number"], e["kind"]) for e in dmg][:40])
        s.note("%d Ctrl-click(s); %d numbers on the Poring; it %s at %r"
               % (clicks, len(on_poring), "died" if gone else "was still there after 90 s", cell))
        ok(s, gone, "the Poring is gone from the client's actors")
        ok(s, on_poring, "damage numbers drawn on the Poring: %s" % [e.get("number") for e in on_poring])
        ok(s, len(on_poring) > clicks, "attacking went on after the click: %d numbers for %d click(s)"
           % (len(on_poring), clicks))
        drops = wait_rows(lambda: t.logs.monster_drops(PORING, map=START[0], after=mark), timeout=5)
        s.rows("picklog M: what mob %d dropped on %s (a Poring that drops nothing leaves no row)"
               % (PORING, START[0]), drops, min_rows=0)
        s.frame("killed-%d" % n)
        return after, cell, drops


def run(t):
    user = t.param("user") or unique_name(tag="c")
    password = t.param("password", "bfh-test")
    name = t.param("char") or unique_name(tag="C")
    sex = t.param("sex", "M")

    c0 = setup_character(t, user, password, name, sex)
    char_id = c0["char_id"]

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)
    t.gm.ctl = ctl
    first = ctl.last_seq

    # a Poring drops nothing about a fifth of the time: up to four of them
    drops, cell = [], None
    for n in range(1, 5):
        with t.step("set-up: Poring %d" % n, "atcommand typed by the client: @monster %d 1" % PORING) as s:
            me = ctl.state("self")["self"]
            target = own_poring(t, ctl, s, me)
            s.state("self", me)
            s.state("the Poring", target)
            ok(s, target is not None and target["screen"], "a Poring on screen")
        after, cell, drops = kill(t, ctl, n, target)
        if drops:
            break

    with t.step("attack sounds", "M8.1c/M7: the attack's voices: the own character's act sound, the hit, the Poring's") as s:
        vs = [e for e in ctl.events_named("sound", first) if e.get("started")]
        files = sorted(set(re.split(r"[\\/]", e["file"] or "")[-1].lower() for e in vs))
        s.state("voices started since entering the map", files)
        own = [f for f in files if f.startswith("attack_")]
        if sex == "M":
            # the male Novice's body act (초보자_남.act) has attack_short_sword.wav
            # and attack_sword.wav on its attack actions; the female's (rdata.grf)
            # has no sound events at all
            ok(s, own, "the own attack sound from the body's act: %s" % own)
        else:
            s.note("female Novice: her act has no sound events; own attack sounds %s" % own)
        ok(s, [f for f in files if f.startswith("_hit_")], "the weapon's hit sound")
        ok(s, [f for f in files if re.match(r"poring_(damage|die)", f)], "the Poring's damage or die sound")

    with t.step("the drop on the ground", "M8.1c: the monster's drop appears as an item actor") as s:
        ok(s, drops, "a Poring dropped something (picklog M)")
        dropped = set(r["nameid"] for r in drops)
        deadline = time.monotonic() + 10
        items = []
        while time.monotonic() < deadline and not items:
            items = [a for a in ctl.state("actors")["actors"] if a["type"] == "item" and a["job"] in dropped
                     and abs(a["x"] - cell[0]) <= 3 and abs(a["y"] - cell[1]) <= 3 and a["screen"]]
            time.sleep(0.5)
        s.state("items near %r" % (cell,), items)
        ok(s, items, "an item of %s on the ground beside the Poring's last cell" % sorted(dropped))
        s.frame("drop")

    with t.step("pick it up", "M8.1c: a click on the item picks it up (picklog P)") as s:
        it = items[0]
        mark = t.logs.mark()
        ctl.move(it["screen"]["x"], it["screen"]["y"])
        ctl.wait({"frames": 3}, timeout=10)
        ctl.click(it["screen"]["x"], it["screen"]["y"])
        s.rows("picklog P item %s by %d" % (it["job"], char_id),
               wait_rows(lambda: t.logs.picked_up(char_id, it["job"], after=mark), timeout=20))
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline and find(ctl, it["id"]):
            time.sleep(0.5)
        ok(s, not find(ctl, it["id"]), "the item is gone from the ground")
        picked = it["job"]

    t.stop_client()
    with t.step("server: experience and inventory", "the char row after logout: exp gained; inventory holds the item") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]], timeout=20)
        c1 = rows[0]
        s.state("char before and after", {k: (c0[k], c1[k]) for k in ("base_level", "base_exp", "job_level", "job_exp")})
        ok(s, (c1["base_level"], c1["base_exp"]) > (c0["base_level"], c0["base_exp"]), "base experience gained")
        s.rows("inventory item %d" % picked, t.logs.inventory(char_id, picked))
