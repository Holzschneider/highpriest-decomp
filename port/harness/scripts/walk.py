"""M6 walk script.

Done when (implementation plan, M6): the walk script enters Prontera, clicks a
destination, and both the client's reported position and the server's agree
on arrival; captured frames show the map, the character, NPCs and other
actors.

Set-up: the character stands at prontera 155,183 (SQL, offline). A second
player, bifrost-bot on a group-99 account (bfharness.bot), stands at
158,186 and spawns a Poring beside itself (@warp, @monster: set-up by
atcommand, in atcommandlog), so the frames hold a player and a monster as
well as the NPCs around the fountain.
Played: the pointer rests on a cell, the client's picking names it
(`pointer.cell`), a click there walks the character to it; at 1024x768 and
again after a resize to 1600x900.
Compared with: chatlog's src_map_x/y of a line said on arrival (the map
server's own position of the speaker), and char.last_x/last_y after quitting.
Frames under <game folder>/test/frames/walk/, checksums in the manifest.
"""

from bfharness.bot import Bot
from bfharness.play import enter_game, login, say_and_confirm, token, walk_click
from bfharness.sql import wait_rows

NAME = "walk"
MILESTONE = "M6"
DESCRIPTION = "walks in Prontera at two window sizes; client and server agree on the cell"
MANIFEST = "port/harness/manifests/m6-walk.sha256"
NEEDS_SERVER = True

BOT_CELL = (158, 186)


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")

    with t.step("set-up: place character", "SQL, offline: prontera 155,183") as s:
        c = t.accounts.set_char(name, map="prontera", x=155, y=183)
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})
        char_id = c["char_id"]

    with t.step("set-up: second player and a monster", "bifrost-bot by atcommand: @warp, @monster") as s:
        bot = Bot.start(t, "walk", "bfhwalkgm", "bfbot", "BfhWalkGM", gm=True)
        if bot is None:
            s.note("mock: no second player")
        else:
            s.state("bot entered", bot.entered)
            mark = t.logs.mark()
            s.state("bot warped", bot.warp("prontera", *BOT_CELL))
            bot.gm("@monster 1002 1")   # a Poring beside the bot
            rows = wait_rows(lambda: [r for r in t.logs.atcommands(contains="@monster", after=mark)], timeout=10)
            s.rows("atcommandlog: @warp and @monster of the bot",
                   t.logs.atcommands(account_id=bot.entered.get("aid"), after=mark))

    ctl = t.launch()
    login(t, ctl, user, password)
    st = enter_game(t, ctl)
    with t.step("in Prontera", "M6: map, character, NPCs and other actors drawn") as s:
        s.equal(st["map"]["name"], "prontera", "map")
        ctl.wait({"frames": 60}, timeout=20)     # the fade-in of the map is over
        actors = ctl.state("actors")["actors"]
        s.state("actors", actors[:30])
        kinds = set(a["type"] for a in actors if a["screen"])
        s.note("on screen: %s" % ", ".join(sorted(kinds)))
        s.check("npc" in kinds, "NPCs on screen")
        if not t.mock:
            s.check(any(a["type"] == "pc" and (a["x"], a["y"]) == BOT_CELL and a["screen"] for a in actors),
                    "the second player on screen at %d,%d" % BOT_CELL)
            s.check("monster" in kinds, "a monster on screen")
        s.frame("prontera")

    cell = walk_click(t, ctl, 80, -48, "1024x768 window")
    with t.step("server agrees (first size)", "M6: server position = client position") as s:
        say_and_confirm(t, ctl, s, "bfh walk %s" % token(), char_id,
                        expect_pos=(cell["x"], cell["y"]), map="prontera")
        s.frame("prontera-arrived")

    with t.step("resize", "M6: in-game resize") as s:
        v = ctl.resize(width=1600, height=900)["view"]
        s.state("view", v)
        s.equal((v["width"], v["height"]), (1600, 900), "window size")
        s.frame("prontera-1600x900")

    cell = walk_click(t, ctl, -96, 64, "1600x900 window")
    with t.step("server agrees (second size)", "M6: server position = client position") as s:
        say_and_confirm(t, ctl, s, "bfh walk %s" % token(), char_id,
                        expect_pos=(cell["x"], cell["y"]), map="prontera")
        s.frame("prontera-1600x900-arrived")

    if bot is not None:
        bot.gm("@killmonster2")
    t.stop_client()
    with t.step("saved position", "the server saved the last cell on logout") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)]
                                  if r and not r["online"] and (r["last_x"], r["last_y"]) == (cell["x"], cell["y"])])
        c = t.logs.char(name)
        s.state("char row", {k: c[k] for k in ("last_map", "last_x", "last_y", "online")})
        s.rows("char %s saved at %d,%d" % (name, cell["x"], cell["y"]), rows)
