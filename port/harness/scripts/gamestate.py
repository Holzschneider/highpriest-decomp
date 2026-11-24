"""M6.4 game state: what the client reports about the world agrees with the server.

Done when (implementation plan, M6 step 4): in game mode the control socket's
`state` reports the map with its size, the own character with the position,
HP and SP the server has, and the actors around it; the wait conditions
`map`, `position` and `moving` hold on them. Every later scripted test reads
these parts (PROTOCOL.md section 3).

Set-up (SQL, offline): bftest's character BFProbe stands at prontera 155,183.
Played: log in, enter the world, let the pointer rest on NPCs (the 2008
client learns an actor's name only then), say a line, quit.
Compared with:
  map      the server's map cache (db/map_cache.dat in the rathena container)
  self     the char row (ids, name, job, levels, zeny, HP/SP read before
           login, and last_map/last_x/last_y and hp/sp saved on logout), and
           chatlog's src_map_x/y, the server's own position of the speaker
  actors   the NPC and warp definitions under npc/ in the rathena container:
           every NPC or portal the client lists stands on a defined cell
           of prontera, and the names learnt by hovering are the defined ones
With --mock the references are the mock's own tables.
"""

import re
import struct
import subprocess

from bfharness.play import enter_game, login, say_and_confirm, token
from bfharness.sql import wait_rows

NAME = "gamestate"
MILESTONE = "M6"
DESCRIPTION = "state map, self and actors in Prontera agree with the server"
MANIFEST = None
NEEDS_SERVER = True

START = ("prontera", 155, 183)


def rathena(t, *cmd):
    """Bytes of a command run in the server's rathena container (read-only)."""
    r = subprocess.run(["docker", "compose", "-f", str(t.server.dir / "docker-compose.yml"),
                        "exec", "-T", "rathena"] + list(cmd),
                       cwd=str(t.server.dir), capture_output=True, timeout=120)
    if r.returncode != 0:
        raise AssertionError("%s: exit %d: %s" % (" ".join(cmd), r.returncode, r.stderr[-500:]))
    return r.stdout


def server_map_size(t, name):
    if t.mock:
        import mock_client
        return mock_client.MAPS[name]
    # The renewal server reads db/re/ before db/ (the maps it changed).
    for path in ("db/re/map_cache.dat", "db/map_cache.dat"):
        d = rathena(t, "cat", path)
        _, count = struct.unpack_from("<IH", d, 0)
        at = 8
        for _ in range(count):
            n, xs, ys, size = struct.unpack_from("<12shhi", d, at)
            if n.split(b"\0", 1)[0].decode("latin-1").lower() == name:
                return xs, ys
            at += 20 + size
    raise AssertionError("%s not in the server's map cache" % name)


def server_npcs(t, name):
    """(x, y) -> [display names] of the NPCs, shops and warps defined on the map."""
    out = {}
    if t.mock:
        import mock_client
        for (m, x, y, nm, _job, _role) in mock_client.NPCS:
            if m == name:
                out.setdefault((x, y), []).append(nm)
        for (m, x, y, *_rest) in mock_client.PORTALS:
            if m == name:
                out.setdefault((x, y), []).append("")
        return out
    text = rathena(t, "sh", "-c", "grep -rhaE '^%s,[0-9]+,[0-9]+,' npc/ || true" % name).decode("latin-1")
    for line in text.splitlines():
        f = line.split("\t")
        if len(f) < 3:
            continue
        m = re.match(r"%s,(\d+),(\d+)" % re.escape(name), f[0])
        if not m:
            continue
        shown = f[2].split("::")[0].split("#")[0]
        out.setdefault((int(m.group(1)), int(m.group(2))), []).append(shown)
    return out


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")
    mapname, sx, sy = START

    with t.step("set-up: place character", "SQL, offline: %s %d,%d" % START) as s:
        c = t.accounts.set_char(name, map=mapname, x=sx, y=sy)
        before = {k: c[k] for k in ("char_id", "account_id", "class", "base_level", "job_level", "zeny",
                                    "hp", "max_hp", "sp", "max_sp", "last_map", "last_x", "last_y", "online")}
        s.state("char row before login", before)

    ctl = t.launch()
    login(t, ctl, user, password)
    st = enter_game(t, ctl)

    with t.step("map", "M6.4: `map` names the map and its size in cells, as the server has it") as s:
        m = st["map"]
        s.state("map", m)
        s.equal(m["name"], mapname, "map name")
        size = server_map_size(t, mapname)
        s.note("server's map cache: %s is %dx%d" % (mapname, size[0], size[1]))
        s.equal((m["width"], m["height"]), tuple(size), "map size (cells)")
        ev = ctl.events_named("map")
        s.state("map events", ev)
        s.check(ev and ev[-1]["map"] == mapname and (ev[-1]["x"], ev[-1]["y"]) == (sx, sy),
                "map event with the start cell")

    with t.step("self", "M6.4: `self` has the character's ids, name, levels, position, HP and SP") as s:
        me = ctl.state("self")["self"]
        s.state("self", me)
        s.equal(me["id"], before["account_id"], "account id")
        s.equal(me["char_id"], before["char_id"], "char id")
        s.equal(me["name"], name, "name")
        s.equal(me["job"], before["class"], "job")
        s.equal((me["base_level"], me["job_level"]), (before["base_level"], before["job_level"]), "levels")
        s.equal(me["zeny"], before["zeny"], "zeny")
        s.equal((me["x"], me["y"]), (sx, sy), "cell")
        s.equal((me["hp"], me["max_hp"], me["sp"], me["max_sp"]),
                (before["hp"], before["max_hp"], before["sp"], before["max_sp"]), "HP and SP (char row)")
        s.check(me["moving"] is False and me["dest"] is None, "standing")
        s.check(0 <= me["dir"] <= 7, "dir 0-7")
        s.check(me["max_weight"] > 0, "max weight reported")

    with t.step("server's position", "M6.4: chatlog has the server's position of the speaker; it is the reported cell") as s:
        say_and_confirm(t, ctl, s, "bfh state %s" % token(), before["char_id"],
                        expect_pos=(me["x"], me["y"]), map=mapname)
        ev = [e for e in ctl.events_named("chat") if e.get("from") == name]
        s.check(ev and ev[-1]["channel"] == "normal", "own line: chat event, channel normal, from the character")

    with t.step("waits", "M6.4: wait map, position, moving hold on the reported state") as s:
        for until in ({"map": mapname}, {"position": {"x": me["x"], "y": me["y"]}}, {"moving": False},
                      {"map": mapname, "position": {"x": me["x"], "y": me["y"]}, "moving": False}):
            r = ctl.wait(until, timeout=5)
            s.state("wait %s" % until, r)
            s.equal(r["met"], until, "met")

    with t.step("actors", "M6.4: `actors` lists the NPCs around the start, on the cells the server defines") as s:
        defined = server_npcs(t, mapname)
        actors = ctl.state("actors")["actors"]
        near = [a for a in actors if a["type"] in ("npc", "portal")]
        s.state("NPCs and portals reported", near)
        s.note("%d actors, %d NPCs/portals; %d cells of %s with NPC or warp definitions"
               % (len(actors), len(near), len(defined), mapname))
        s.check(len(near) >= 3, "at least three NPCs around %d,%d" % (sx, sy))
        stray = [a for a in near if (a["x"], a["y"]) not in defined]
        s.check(not stray, "every NPC stands on a defined cell", stray=stray)
        s.check(all(isinstance(a["id"], int) and a["id"] > 0 and isinstance(a["job"], int) for a in near),
                "ids and jobs are numbers")
        s.check(any(a["screen"] for a in near), "some NPCs on screen")
        s.frame("prontera-npcs")

    with t.step("names", "M6.4: an actor's name is learnt when the pointer rests on it, and is the server's") as s:
        named = ctl.actor(lambda a: a["type"] == "npc" and a["name"] != "?name")
        actors = ctl.state("actors")["actors"]
        known = [a for a in actors if a["type"] == "npc" and a["name"] != "?name"]
        s.state("NPCs with names", known)
        s.check(named is not None, "a name learnt by hovering")
        wrong = [a for a in known if a["name"] not in defined.get((a["x"], a["y"]), [])]
        s.check(not wrong, "names as defined for their cells", wrong=wrong,
                defined={"%d,%d" % (a["x"], a["y"]): defined.get((a["x"], a["y"])) for a in known})

    with t.step("pointer", "M6.4: `pointer.cell` is the cell the client's picking finds") as s:
        v = ctl.state("view")["view"]
        cx, cy = v["logical"]["w"] // 2, v["logical"]["h"] // 2
        ctl.move(cx, cy)
        p = ctl.state("pointer")["pointer"]
        s.state("pointer at the window centre", p)
        s.equal((p["x"], p["y"]), (cx, cy), "pointer position (logical)")
        s.check(p["cell"] is not None and abs(p["cell"]["x"] - sx) <= 1 and abs(p["cell"]["y"] - sy) <= 1,
                "the cell under the centre is the character's or next to it")

    t.stop_client()
    with t.step("saved on logout", "M6.4: the server saved the reported cell, HP and SP") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]])
        c = rows[-1]
        s.state("char row after logout", {k: c[k] for k in ("last_map", "last_x", "last_y", "hp", "sp", "online")})
        s.equal((c["last_map"], c["last_x"], c["last_y"]), (mapname, me["x"], me["y"]), "saved position")
        s.equal((c["hp"], c["sp"]), (me["hp"], me["sp"]), "saved HP and SP")
