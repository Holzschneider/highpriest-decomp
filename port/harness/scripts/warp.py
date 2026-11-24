"""M6 map change by warp portal, between two readable maps.

Done when (implementation plan, M6): a map change by warp portal works
between two readable maps.

The maps: prontera and prt_in are both among the pre-renewal maps the game
folder holds (port/data/assemble.py). The portal: rAthena's prt06,
prontera 179,184 (2x2 cells) to prt_in 60,73, and back by prt06-1, prt_in
60,77 (2x1) to prontera 175,188 (npc/ of the server, read at run time).

Set-up (SQL, offline): the character stands at prontera 172,184, seven cells
west of the portal, which is on screen.
Played: the pointer rests on the portal as the client draws it, a click
walks the character into it; the client loads prt_in. There it walks into
the exit portal the same way and is back in Prontera.
Compared with: the `map` event and `state.map`/`self` against the warp's
destination as the server defines it; chatlog's src_map and src_map_x/y of a
line said after each change (the map server's own map and cell);
char.last_map/last_x/last_y after quitting.
"""

import re
import subprocess

from bfharness.play import enter_game, login, say_and_confirm, token, own_home
from bfharness.sql import wait_rows

NAME = "warp"
MILESTONE = "M6"
DESCRIPTION = "walks through a portal from prontera into prt_in and back; client and server agree"
MANIFEST = "port/harness/manifests/m6-warp.sha256"
NEEDS_SERVER = True

START = ("prontera", 172, 184)
# name, map, x, y, span x, span y, destination map, x, y (as in the server's npc/)
WARPS = [("prt06", "prontera", 179, 184, 2, 2, "prt_in", 60, 73),
         ("prt06-1", "prt_in", 60, 77, 2, 1, "prontera", 175, 188)]


def server_warp(t, name):
    """The warp's definition as the server loads it (the mock: WARPS as written)."""
    w = [x for x in WARPS if x[0] == name][0]
    if t.mock:
        return w
    r = subprocess.run(["docker", "compose", "-f", str(t.server.dir / "docker-compose.yml"), "exec", "-T",
                        "rathena", "sh", "-c", "grep -rhE '^%s,%d,%d,[0-9]+\\s+warp\\s+%s\\s' npc/ || true"
                        % (w[1], w[2], w[3], re.escape(name))],
                       cwd=str(t.server.dir), capture_output=True, text=True, timeout=60)
    line = r.stdout.strip().splitlines()
    if not line:
        raise AssertionError("warp %s not defined at %s %d,%d on the server" % (name, w[1], w[2], w[3]))
    m = re.match(r"(\w+),(\d+),(\d+),\d+\s+warp\s+\S+\s+(\d+),(\d+),(\w+),(\d+),(\d+)", line[0])
    return (name, m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5)),
            m.group(6), int(m.group(7)), int(m.group(8)))


def through(t, ctl, warp, char_id):
    name, mp, wx, wy, sx, sy, dm, dx, dy = warp
    with t.step("the portal %s on %s" % (name, mp), "the client shows the portal where the server has it") as s:
        s.state("warp as the server defines it", {"map": mp, "x": wx, "y": wy, "span": [sx, sy],
                                                  "to": [dm, dx, dy]})
        st = ctl.state("map", "self", "actors")
        s.equal(st["map"]["name"], mp, "map")
        portals = [a for a in st["actors"] if a["type"] == "portal"]
        s.state("portals reported", portals)
        p = [a for a in portals if abs(a["x"] - wx) <= sx and abs(a["y"] - wy) <= sy]
        s.check(p and p[0]["screen"], "a portal on screen at %d,%d" % (wx, wy))
        portal = p[0]
    with t.step("walk into %s" % name, "M6: a map change by warp portal: %s to %s" % (mp, dm)) as s:
        after = ctl.last_seq
        ctl.move(portal["screen"]["x"], portal["screen"]["y"])
        ctl.wait({"frames": 3}, timeout=10)
        s.state("pointer on the portal", ctl.state("pointer")["pointer"])
        ctl.click(portal["screen"]["x"], portal["screen"]["y"])
        ev = ctl.wait_event("map", lambda e: e.get("map") == dm, after=after, timeout=90)
        s.state("map event", ev)
        st = ctl.wait({"map": dm, "moving": False}, timeout=30)
        me = ctl.state("map", "self")
        s.state("map and self", me)
        s.equal(me["map"]["name"], dm, "map after the portal")
        s.equal((me["self"]["x"], me["self"]["y"]), (dx, dy), "cell: the warp's destination")
        ctl.wait({"frames": 60}, timeout=20)
        s.frame("%s-from-%s" % (dm, mp))
    with t.step("server agrees on %s" % dm, "M6: the map server has the character on %s" % dm) as s:
        say_and_confirm(t, ctl, s, "bfh warp %s" % token(), char_id, expect_pos=(dx, dy), map=dm)


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")

    with t.step("set-up: place character", "SQL, offline: %s %d,%d" % START) as s:
        c = t.accounts.set_char(name, map=START[0], x=START[1], y=START[2])
        s.state("char row", {k: c[k] for k in ("char_id", "last_map", "last_x", "last_y", "online")})
        char_id = c["char_id"]
        warps = [server_warp(t, w[0]) for w in WARPS]
        s.state("warps", warps)

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)
    for w in warps:
        through(t, ctl, w, char_id)

    t.stop_client()
    last = warps[-1]
    with t.step("saved on logout", "the server saved the map and cell after the portals") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]
                                  and (r["last_map"], r["last_x"], r["last_y"]) == (last[6], last[7], last[8])])
        s.rows("char %s saved at %s %d,%d" % (name, last[6], last[7], last[8]), rows)
