"""M8 gate test (skeleton of the real test; passes against the mock).

Done when (implementation plan, M8): create a character, kill a Poring at the
south gate, pick up the drop, buy a potion and sell the loot, change job at
the Job Master, chat. Each step is confirmed in the server's SQL logs.

Set-up, by atcommands typed through the client (logged in atcommandlog) on a
fresh group-99 account: job level 10, 1000 zeny, and the moves between the
scenes (@warp to prt_fild08, prt_in, prontera). What is under test is played:
creating the character, attacking, picking up, the shop windows, the Job
Master's menu, the chat line. The NPC and deal windows are played as in
npc.py and shop.py (bfharness/npcplay.py): menus by entry and OK, items
dragged into the deal window with the amount typed.

| Step | SQL evidence |
|---|---|
| create | charlog 'make new char', char row |
| kill | picklog type M, char_id 1002 (the Poring's mob id), map prt_fild08 |
| pick up | picklog type P, char_id, nameid 909 (Jellopy), amount > 0 |
| buy | picklog type S, nameid 501, amount > 0; zenylog type S, amount < 0 |
| sell | picklog type S, nameid 909, amount < 0; zenylog type S, amount > 0 |
| job change | char.class = 1 after logout (Swordman) |
| chat | chatlog type O with the line |

Server requirement: the Job Master is rAthena's npc/custom/jobmaster.txt,
which the stock configuration does not load; port/server/import must add it
(`npc: npc/custom/jobmaster.txt` in map_conf.txt). Shop: Tool Dealer at
prt_in 126,76. Control ids used (PROTOCOL.md `controls`): CHOOSESELLBUYWND
`buy`/`sell`, ITEMSHOPWND `item:<id>`, ITEMPURCHASEWND and ITEMSELLWND
`item:<id>` and `ok`, ITEMDROPCNTWND `edit`, CHOOSEWND `choice:<label>` and
`ok`, SAYDIALOGWND `next`/`close`.
"""

import time

from bfharness import npcplay
from bfharness.accounts import unique_name
from bfharness.play import enter_game, login, say_and_confirm, token
from bfharness.sql import wait_rows

NAME = "gate"
MILESTONE = "M8"
DESCRIPTION = "the gate test: create, kill, pick up, buy, sell, job change, chat"
MANIFEST = "port/harness/manifests/m8-gate.sha256"
NEEDS_SERVER = True

PORING, JELLOPY, RED_POTION, SWORDMAN = 1002, 909, 501, 1


def gm(t, s, ctl, command, after_mark):
    """Set-up by atcommand, confirmed in atcommandlog."""
    t.gm.run(command)
    rows = wait_rows(lambda: t.logs.atcommands(contains=command, after=after_mark))
    s.rows("atcommandlog %r" % command, rows)


def click_actor(ctl, pred, what):
    a = ctl.actor(pred)
    if not a or not a.get("screen"):
        raise AssertionError("no %s on screen" % what)
    ctl.click(a["screen"]["x"], a["screen"]["y"])
    return a


def run(t):
    userid = t.param("user") or unique_name(tag="g")
    password = t.param("password", "bfh-test")
    charname = t.param("name") or unique_name(tag="n")

    with t.step("set-up: GM account", "group 99, so atcommands can set scenes up") as s:
        aid = t.accounts.create_account(userid, password, group_id=99)
        s.note("account %s (%d)" % (userid, aid))

    ctl = t.launch()
    login(t, ctl, userid, password)

    with t.step("create a character", "gate: create") as s:
        mark = t.logs.mark()
        ctl.press("enter")
        ctl.wait_window("MAKECHARWND", timeout=30)
        ctl.type_text(charname)
        ctl.press("enter")
        ctl.wait_mode("charselect", 30)
        s.rows("charlog 'make new char'",
               wait_rows(lambda: t.logs.charlog(name=charname, msg="make new char", after=mark)))
        char_id = t.logs.char(charname)["char_id"]

    enter_game(t, ctl)

    with t.step("set-up: south gate", "atcommands: job level, zeny, warp") as s:
        mark = t.logs.mark()
        for cmd in (t.gm.job_level(9), t.gm.zeny(1000), t.gm.warp("prt_fild08", 170, 372)):
            gm(t, s, ctl, cmd, mark)
        ctl.wait_map("prt_fild08", 60)
        s.frame("south-gate")

    with t.step("kill a Poring", "gate: kill, confirmed by the drop in picklog") as s:
        mark = t.logs.mark()
        deadline = time.monotonic() + 120
        drops = []
        while not drops and time.monotonic() < deadline:
            try:
                click_actor(ctl, lambda a: a["type"] == "monster" and a["job"] == PORING, "Poring")
            except AssertionError:
                pass
            drops = wait_rows(lambda: t.logs.monster_drops(PORING, map="prt_fild08", after=mark), timeout=3)
        s.rows("picklog M from mob %d on prt_fild08" % PORING, drops)
        s.frame("poring-killed")

    with t.step("pick up the drop", "gate: pick up") as s:
        mark = t.logs.mark()
        click_actor(ctl, lambda a: a["type"] == "item" and a["job"] == JELLOPY, "Jellopy")
        rows = wait_rows(lambda: t.logs.picked_up(char_id, JELLOPY, after=mark), timeout=30)
        s.rows("picklog P Jellopy by %d" % char_id, rows)

    with t.step("buy a potion", "gate: buy") as s:
        mark = t.logs.mark()
        gm(t, s, ctl, t.gm.warp("prt_in", 126, 73), mark)
        ctl.wait_map("prt_in", 60)
        mark = t.logs.mark()
        click_actor(ctl, lambda a: a["type"] == "npc" and a["name"].startswith("Tool Dealer"), "Tool Dealer")
        ctl.wait_window("CHOOSESELLBUYWND")
        ctl.click_control("CHOOSESELLBUYWND", "buy")
        ctl.wait_window("ITEMPURCHASEWND")
        npcplay.wait_control(ctl, "ITEMSHOPWND", "item:%d" % RED_POTION)
        npcplay.drag_item(ctl, "ITEMSHOPWND", RED_POTION, "ITEMPURCHASEWND")
        npcplay.type_into(ctl, "ITEMDROPCNTWND", 1, s, "the amount")
        s.frame("shop")
        ctl.click_control("ITEMPURCHASEWND", "ok")
        s.rows("picklog S Red Potion +", wait_rows(lambda: t.logs.bought(char_id, RED_POTION, after=mark)))
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.check(z[-1]["amount"] < 0, "zeny paid", amount=z[-1]["amount"])

    with t.step("sell the loot", "gate: sell") as s:
        mark = t.logs.mark()
        click_actor(ctl, lambda a: a["type"] == "npc" and a["name"].startswith("Tool Dealer"), "Tool Dealer")
        ctl.wait_window("CHOOSESELLBUYWND")
        ctl.click_control("CHOOSESELLBUYWND", "sell")
        ctl.wait_window("ITEMSELLWND")
        npcplay.wait_control(ctl, "ITEMSHOPWND", "item:%d" % JELLOPY)
        npcplay.drag_item(ctl, "ITEMSHOPWND", JELLOPY, "ITEMSELLWND")
        npcplay.type_into(ctl, "ITEMDROPCNTWND", 1, s, "the amount")
        ctl.click_control("ITEMSELLWND", "ok")
        s.rows("picklog S Jellopy -", wait_rows(lambda: t.logs.sold(char_id, JELLOPY, after=mark)))
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.check(z[-1]["amount"] > 0, "zeny received", amount=z[-1]["amount"])

    with t.step("change job", "gate: Job Master makes a Swordman") as s:
        mark = t.logs.mark()
        gm(t, s, ctl, t.gm.warp("prontera", 153, 190), mark)
        ctl.wait_map("prontera", 60)
        click_actor(ctl, lambda a: a["type"] == "npc" and a["name"] == "Job Master", "Job Master")
        ctl.wait_window("CHOOSEWND")
        s.frame("job-master")
        npcplay.run_dialogue(ctl, s, answers=["Swordsman", "Change into"])
        deadline = time.monotonic() + 15
        while ctl.state("self")["self"]["job"] != SWORDMAN and time.monotonic() < deadline:
            ctl.wait({"frames": 10})
        s.equal(ctl.state("self")["self"]["job"], SWORDMAN, "job reported by the client")

    with t.step("chat", "gate: chat, in chatlog") as s:
        say_and_confirm(t, ctl, s, "bfh gate %s" % token(), char_id)

    t.stop_client()
    with t.step("job saved", "char.class after logout") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(charname)] if r and r["class"] == SWORDMAN])
        s.rows("char %s class %d" % (charname, SWORDMAN), rows)
