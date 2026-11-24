"""M6 resize and picking: a click lands on the cell under the pointer at
several window sizes.

Done when (implementation plan, M6): resizing the window in game keeps
picking correct: a click lands on the cell under the pointer at two window
sizes.

Set-up (SQL, offline): the character stands at prontera 155,183, among the
NPCs around the fountain.
Played, at the window the client opens with, after a resize to 1600x900 and
after one to 2560x1080 at UI scale 2 (a wide window, logical 1280x540):
  1. Under the pointer: the pointer rests on each NPC's feet as the client
     projects them (`actors[].screen`, the renderer's projection) and the
     client's picking (`pointer.cell`, a ray through the pointer into the
     ground) must name that NPC's cell. Projection and picking are separate
     paths through the camera, so they agree only when both use the window's
     current size.
  2. The click: the pointer rests on a free cell off the centre, picking
     names it, a click there walks the character; on arrival the client's
     position is that cell, and so is the server's (chatlog src_map_x/y of a
     line said on arrival).
Frames: the pointer over the cell before each click (the ground cursor is
drawn on the picked cell), under <game folder>/test/frames/resize_pick/.
"""

from bfharness.play import enter_game, login, say_and_confirm, token, walk_click, own_home
from bfharness.sql import wait_rows

NAME = "resize_pick"
MILESTONE = "M6"
DESCRIPTION = "picking names the cell under the pointer, and a click walks there, at three window sizes"
MANIFEST = "port/harness/manifests/m6-resize-pick.sha256"
NEEDS_SERVER = True

SIZES = [None, (1600, 900, None), (2560, 1080, 2)]
CLICKS = [(80, -48), (-96, 64), (120, 40)]


def under_pointer(t, ctl, label):
    with t.step("under the pointer: %s" % label,
                "M6: picking names the cell the client draws an actor on, at this window size") as s:
        actors = ctl.state("actors")["actors"]
        on = [a for a in actors if a["type"] in ("npc", "pc", "monster") and a["screen"]]
        least = 1 if t.mock else 3   # the mock's Prontera has two
        s.check(len(on) >= least, "at least %d actors on screen (%d)" % (least, len(on)))
        seen = []
        for a in on:
            ctl.move(a["screen"]["x"], a["screen"]["y"])
            ctl.wait({"frames": 2}, timeout=10)
            p = ctl.state("pointer")["pointer"]
            seen.append({"actor": a["id"], "job": a["job"], "cell": [a["x"], a["y"]],
                         "screen": [a["screen"]["x"], a["screen"]["y"]],
                         "picked": [p["cell"]["x"], p["cell"]["y"]] if p["cell"] else None})
        s.state("actor cell, its screen position, the cell picked there", seen)
        wrong = [e for e in seen if e["picked"] != e["cell"]]
        s.check(not wrong, "every picked cell is the actor's cell", wrong=wrong)


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")

    with t.step("set-up: place character", "SQL, offline: prontera 155,183") as s:
        c = t.accounts.set_char(name, map="prontera", x=155, y=183)
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})
        char_id = c["char_id"]

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    cell = None
    for size, (dx, dy) in zip(SIZES, CLICKS):
        if size is None:
            v = ctl.state("view")["view"]
            label = "%dx%d (start)" % (v["width"], v["height"])
        else:
            w, h, scale = size
            label = "%dx%d%s" % (w, h, " scale %d" % scale if scale else "")
            with t.step("resize to %s" % label, "M6: the window is resized in game") as s:
                v = ctl.resize(width=w, height=h, scale=scale)["view"]
                s.state("view", v)
                s.equal((v["width"], v["height"]), (w, h), "window size (device pixels)")
                if scale:
                    s.equal(v["scale"], scale, "UI scale")
                ctl.wait({"frames": 10}, timeout=10)
        under_pointer(t, ctl, label)
        with t.step("pointer frame: %s" % label, "the ground cursor on the picked cell") as s:
            vv = ctl.state("view")["view"]
            ctl.move(vv["logical"]["w"] // 2 + dx, vv["logical"]["h"] // 2 + dy)
            ctl.wait({"frames": 3}, timeout=10)
            s.state("pointer", ctl.state("pointer")["pointer"])
            s.frame("pointer-%s" % label.split(" ")[0])
        cell = walk_click(t, ctl, dx, dy, label)
        with t.step("server agrees: %s" % label, "M6: the server's position is the clicked cell") as s:
            say_and_confirm(t, ctl, s, "bfh pick %s" % token(), char_id,
                            expect_pos=(cell["x"], cell["y"]), map="prontera")

    t.stop_client()
    with t.step("saved position", "the server saved the last clicked cell on logout") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)]
                                  if r and not r["online"] and (r["last_x"], r["last_y"]) == (cell["x"], cell["y"])])
        s.rows("char %s saved at %d,%d" % (name, cell["x"], cell["y"]), rows)
