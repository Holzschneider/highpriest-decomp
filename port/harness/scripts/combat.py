"""M8.1c combat and damage numbers; items on the ground.

Done when (checklist M8.1c): the client kills a monster by clicking it, its
damage numbers are drawn, and the server agrees that the monster died and
the character gained experience; the monster's drop lies on the ground as an
item actor, the client picks it up by clicking it, and the server logs the
pickup and holds the item in the inventory.

Set-up: a fresh group-99 account and its male Novice (bifrost-bot creates
the character and logs out again; SQL places it on prt_fild08 at 200,340,
open ground away from the south gate's arrival, where gate, sound, soak and
english_session spawn their Porings; play.fresh_character). In game, the
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

Porings are looters, and the field has its own besides those other scripts
spawn: one standing by takes a drop within a second (picklog type L). So the
drop is clicked the moment it lands (field.grab_drop, as gate.py does), and
if it was lost to a looter another Poring is fought, up to six; the pick-up
itself is always the client's click, confirmed by picklog P.
"""

import re
import time

from bfharness.accounts import unique_name
from bfharness.field import find, grab_drop, kill_until_drop, ok
from bfharness.play import enter_game, fresh_character, login, own_home
from bfharness.sql import wait_rows

NAME = "combat"
MILESTONE = "M8"
DESCRIPTION = "kill a Poring by clicking, damage numbers, exp; pick up its drop"
MANIFEST = "port/harness/manifests/m8-combat.sha256"
NEEDS_SERVER = True

PORING = 1002
START = ("prt_fild08", 200, 340)


def run(t):
    user = t.param("user") or unique_name(tag="c")
    password = t.param("password", "bfh-test")
    name = t.param("char") or unique_name(tag="C")
    sex = t.param("sex", "M")

    c0 = fresh_character(t, user, password, name, sex, place=START)
    char_id = c0["char_id"]

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)
    t.gm.ctl = ctl
    first = ctl.last_seq

    mark = t.logs.mark()
    grabbed = {}

    def on_death(s, cell):
        grabbed["item"] = grab_drop(ctl, s, cell)

    def picked(drops):
        it = grabbed.pop("item", None)
        if not drops or not it or it["job"] not in set(r["nameid"] for r in drops):
            return False
        rows = wait_rows(lambda: t.logs.picked_up(char_id, it["job"], after=mark), timeout=20)
        if rows:
            grabbed.update(item=it, rows=rows)
        return bool(rows)

    _, cell, drops = kill_until_drop(t, ctl, PORING, START[0], tries=6, on_death=on_death, enough=picked)

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
        s.rows("picklog M from mob %d on %s" % (PORING, START[0]), drops, min_rows=0)
        it = grabbed.get("item")
        s.state("the item actor clicked as it landed beside %r" % (cell,), it)
        ok(s, it and it["job"] in set(r["nameid"] for r in drops),
           "an item actor of the Poring's drop on the ground beside its last cell, within 6 kills "
           "(Porings loot drops)")
        s.frame("drop")

    with t.step("pick it up", "M8.1c: a click on the item picks it up (picklog P)") as s:
        s.rows("picklog P item %s by %d" % (it["job"], char_id), grabbed["rows"])
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
