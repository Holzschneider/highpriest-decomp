"""M6 walk script (skeleton of the real test; passes against the mock).

Done when (implementation plan, M6): the walk script enters Prontera, clicks a
destination, and both the client's reported position and the server's agree
on arrival; frames show the map and actors. Resizing the window in game keeps
picking correct: a click lands on the cell under the pointer at two sizes.

Set-up (SQL, offline): the character stands at prontera 155,183 before login.
Played: click the ground at two window sizes. The server's position comes
from chatlog (a line said on arrival is stored with the speaker's map, x and
y as the map server holds them) and, after quitting, from char.last_x/last_y.
"""

from bfharness.play import enter_game, login, say_and_confirm, token, walk_click
from bfharness.sql import wait_rows

NAME = "walk"
MILESTONE = "M6"
DESCRIPTION = "walks in Prontera at two window sizes; client and server agree on the cell"
MANIFEST = "port/harness/manifests/m6-walk.sha256"
NEEDS_SERVER = True


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")

    with t.step("set-up: place character", "SQL, offline: prontera 155,183") as s:
        c = t.accounts.set_char(name, map="prontera", x=155, y=183)
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})
        char_id = c["char_id"]

    ctl = t.launch()
    login(t, ctl, user, password)
    st = enter_game(t, ctl)
    with t.step("in Prontera", "M6: map, character, NPCs drawn") as s:
        s.equal(st["map"]["name"], "prontera", "map")
        actors = ctl.state("actors")["actors"]
        s.state("actors", actors[:20])
        s.check(any(a["type"] == "npc" for a in actors), "NPCs visible")
        s.frame("prontera")

    cell = walk_click(t, ctl, 80, -48, "1024x768 window")
    with t.step("server agrees (first size)", "M6: server position = client position") as s:
        say_and_confirm(t, ctl, s, "bfh walk %s" % token(), char_id,
                        expect_pos=(cell["x"], cell["y"]), map="prontera")

    with t.step("resize", "M6: in-game resize") as s:
        v = ctl.resize(width=1600, height=900)["view"]
        s.state("view", v)
        s.equal((v["width"], v["height"]), (1600, 900), "window size")
        s.frame("prontera-1600x900")

    cell = walk_click(t, ctl, -96, 64, "1600x900 window")
    with t.step("server agrees (second size)", "M6: server position = client position") as s:
        say_and_confirm(t, ctl, s, "bfh walk %s" % token(), char_id,
                        expect_pos=(cell["x"], cell["y"]), map="prontera")

    t.stop_client()
    with t.step("saved position", "the server saved the last cell on logout") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)]
                                  if r and not r["online"] and (r["last_x"], r["last_y"]) == (cell["x"], cell["y"])])
        c = t.logs.char(name)
        s.state("char row", {k: c[k] for k in ("last_map", "last_x", "last_y", "online")})
        s.rows("char %s saved at %d,%d" % (name, cell["x"], cell["y"]), rows)
