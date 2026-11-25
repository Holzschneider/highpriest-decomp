"""Steps on the field that several scripts play the same way: walking into a
portal, a monster spawned beside the character (set-up by atcommand),
attacking it with one Ctrl-click until it dies, and picking its drop up from
the ground (combat.py, sound.py, gate.py).

The 2008 client attacks while the button is held (ATTM_REPEAT_TILL_BUTTONUP)
and goes on attacking after the button is up only with Ctrl or the /noctrl
option (ATTM_REPEAT, GameMode.cpp ProcessObjClick); so one Ctrl-click attacks
until the monster dies (re-clicked if it walks off while the character is
not attacking). The `damage` event (one per number the client draws,
CGameActor::Am_Make_Number) names the target and the number.
"""

import time

from .sql import wait_rows


def ok(s, cond, message):
    s.check(cond, message, passed=bool(cond))


def find(ctl, aid):
    a = [x for x in ctl.state("actors")["actors"] if x["id"] == aid]
    return a[0] if a else None


def walk_into_portal(t, ctl, s, warp, slack=4):
    """Click the portal actor nearest the warp's cell and wait for the map
    change. warp: (name, map, x, y, span x, span y, to map, x, y) as the
    server's npc/ defines it; the portal actor may stand anywhere in the span
    (and `slack` cells beyond: the mock's portals are single cells). Returns
    the `map` event."""
    name, mp, wx, wy, sx, sy, dm, dx, dy = warp
    portals = [a for a in ctl.state("actors")["actors"] if a["type"] == "portal" and a["screen"]
               and abs(a["x"] - wx) <= sx + slack and abs(a["y"] - wy) <= sy + slack]
    portals.sort(key=lambda a: abs(a["x"] - wx) + abs(a["y"] - wy))
    s.state("portal", portals[:1])
    ok(s, portals, "the portal %s on screen" % name)
    p = portals[0]
    after = ctl.last_seq
    ctl.move(p["screen"]["x"], p["screen"]["y"])
    ctl.wait({"frames": 3}, timeout=10)
    ctl.click(p["screen"]["x"], p["screen"]["y"])
    ev = ctl.wait_event("map", lambda e: e.get("map") == dm, after=after, timeout=90)
    s.state("map event", ev)
    ctl.wait({"map": dm, "moving": False}, timeout=30)
    return ev


def near_portal(ctl, a, cells=3):
    """The portal actor within `cells` (each axis) of actor a, or None."""
    for p in ctl.state("actors")["actors"]:
        if p["type"] == "portal" and abs(p["x"] - a["x"]) <= cells and abs(p["y"] - a["y"]) <= cells:
            return p
    return None


def own_monster(t, ctl, s, mob, me, n=1, tries=4):
    """Spawn a monster by atcommand typed by the client (set-up, in
    atcommandlog) and return the new one nearest the character. The n-th
    call alternates `@monster <mob> 1` and `@monster <mob>` (the same
    amount): the client refuses a line said the third time in a row
    (CGameMode m_sameChatRepeatCnt).

    A monster that the server put within 3 cells of a portal is passed
    over and another spawned (up to `tries` in all): the client walks next
    to a monster out of reach before attacking it, and at prt_fild08
    170,375 (the south gate's arrival) a Poring in the corridor north of
    the character has its neighbour cells inside the warp prtf004 (167-173,
    376-380), so the attack would carry the character back to prontera."""
    target = None
    for k in range(tries):
        target = _spawn_monster(t, ctl, s, mob, me, n + k)
        p = target and near_portal(ctl, target)
        if not p:
            return target
        s.note("the new monster at %d,%d stands within 3 cells of the portal at %d,%d; spawning another"
               % (target["x"], target["y"], p["x"], p["y"]))
    return target


def _spawn_monster(t, ctl, s, mob, me, n):
    before = set(a["id"] for a in ctl.state("actors")["actors"])
    mark = t.logs.mark()
    t.gm.run(t.gm.monster(mob, 1) if n % 2 else "@monster %s" % mob)
    s.rows("atcommandlog @monster", wait_rows(lambda: t.logs.atcommands(contains="@monster", after=mark), timeout=10))
    deadline = time.monotonic() + 10
    new = []
    while time.monotonic() < deadline and not new:
        ctl.wait({"frames": 5}, timeout=10)
        new = [a for a in ctl.state("actors")["actors"]
               if a["type"] == "monster" and a["job"] == mob and a["id"] not in before]
    mons = new or [a for a in ctl.state("actors")["actors"] if a["type"] == "monster" and a["job"] == mob]
    mons.sort(key=lambda a: abs(a["x"] - me["x"]) + abs(a["y"] - me["y"]))
    return mons[0] if mons else None


def kill(t, ctl, n, target, mob, map, what="Poring", on_death=None):
    """Ctrl-click the monster once; click again only when no number has been
    drawn on it for 6 s (it wandered off). on_death(s, cell), if given, is
    called the moment the monster leaves the client's actors, before the
    evidence is gathered (grab_drop: Porings are looters, and one standing
    by takes a drop within a second). Returns (event seq before the attack,
    last cell, picklog M rows of its drop)."""
    with t.step("attack %s %d until it dies" % (what, n),
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
        if gone and on_death:
            on_death(s, cell)
        dmg = ctl.events_named("damage", after)
        on_it = [e for e in dmg if e.get("target") == target["id"]]
        s.state("damage events (target, number, kind)", [(e["target"], e["number"], e["kind"]) for e in dmg][:40])
        s.note("%d Ctrl-click(s); %d numbers on the %s; it %s at %r"
               % (clicks, len(on_it), what, "died" if gone else "was still there after 90 s", cell))
        ok(s, gone, "the %s is gone from the client's actors" % what)
        ok(s, on_it, "damage numbers drawn on the %s: %s" % (what, [e.get("number") for e in on_it]))
        ok(s, len(on_it) > clicks, "attacking went on after the click: %d numbers for %d click(s)"
           % (len(on_it), clicks))
        drops = wait_rows(lambda: t.logs.monster_drops(mob, map=map, after=mark), timeout=5)
        s.rows("picklog M: what mob %d dropped on %s (a %s that drops nothing leaves no row)"
               % (mob, map, what), drops, min_rows=0)
        s.frame("killed-%d" % n)
        return after, cell, drops


def kill_until_drop(t, ctl, mob, map, tries=4, what="Poring", on_death=None, enough=bool):
    """Spawn and kill monsters one at a time until one drops something (a
    Poring drops nothing about a fifth of the time), or until enough(drops)
    holds. Returns (event seq before the last attack, its last cell,
    picklog M rows)."""
    after, cell, drops = None, None, []
    for n in range(1, tries + 1):
        with t.step("set-up: %s %d" % (what, n), "atcommand typed by the client: @monster %d" % mob) as s:
            me = ctl.state("self")["self"]
            target = own_monster(t, ctl, s, mob, me, n)
            s.state("self", me)
            s.state("the %s" % what, target)
            ok(s, target is not None and target["screen"], "a %s on screen" % what)
        after, cell, drops = kill(t, ctl, n, target, mob, map, what, on_death)
        if enough(drops):
            break
    return after, cell, drops


def ground_items(ctl, nameids, cell, timeout=10, near=3):
    """Item actors of those ids on screen within `near` cells of cell,
    waited for up to timeout seconds."""
    deadline = time.monotonic() + timeout
    items = []
    while time.monotonic() < deadline and not items:
        items = [a for a in ctl.state("actors")["actors"] if a["type"] == "item" and a["job"] in nameids
                 and abs(a["x"] - cell[0]) <= near and abs(a["y"] - cell[1]) <= near and a["screen"]]
        if not items:
            time.sleep(0.5)
    return items


def grab_drop(ctl, s, cell, timeout=3, near=2):
    """Click the first item actor that lands within `near` cells of cell,
    as soon as it is there (polled without waiting for frames). Returns the
    item actor clicked, or None."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        items = [a for a in ctl.state("actors")["actors"] if a["type"] == "item" and a["screen"]
                 and abs(a["x"] - cell[0]) <= near and abs(a["y"] - cell[1]) <= near]
        if items:
            items.sort(key=lambda a: abs(a["x"] - cell[0]) + abs(a["y"] - cell[1]))
            it = items[0]
            ctl.move(it["screen"]["x"], it["screen"]["y"])
            ctl.click(it["screen"]["x"], it["screen"]["y"])
            s.state("the drop, clicked as it landed", it)
            return it
        time.sleep(0.05)
    s.note("no item landed within %d cells of %r in %ds" % (near, cell, timeout))
    return None


def pick_up(t, ctl, s, item, char_id):
    """Click the item actor (CZ_ITEM_PICKUP); the server logs picklog type
    P for the character, and the item leaves the ground. Returns the rows."""
    mark = t.logs.mark()
    ctl.move(item["screen"]["x"], item["screen"]["y"])
    ctl.wait({"frames": 3}, timeout=10)
    ctl.click(item["screen"]["x"], item["screen"]["y"])
    rows = s.rows("picklog P item %s by %d" % (item["job"], char_id),
                  wait_rows(lambda: t.logs.picked_up(char_id, item["job"], after=mark), timeout=20))
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline and find(ctl, item["id"]):
        time.sleep(0.5)
    ok(s, not find(ctl, item["id"]), "the item is gone from the ground")
    return rows
