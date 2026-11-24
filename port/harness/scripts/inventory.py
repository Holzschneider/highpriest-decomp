"""M8.1d inventory and equipment; status window.

Done when (checklist M8.1d): through the item window (Alt+E) the client uses
a potion (HP rises, picklog C), equips a weapon and an armour that the
equipment window (Alt+Q) then shows and the status window (Alt+A) reflects
in ATK and DEF, takes the armour off again, drops an item on the ground
(picklog P, negative amount), and raises a stat with the status window's
button; the server agrees: picklog rows while playing, and after logout the
`inventory` table's `equip` column and the `char` row's str and
status_point.

Set-up: a fresh group-99 account and its male Novice (play.fresh_character),
offline at prontera 150,150 with 10 HP (SQL); in game the client types
`@item 501 5` (Red Potion), `@item 1207 1` (Main Gauche; the server's Sword
needs base level 2), `@item 2305 1` (Adventurer's Suit), `@item 2220 1`
(Hat), `@item 2201 1` (Sunglasses) and
`@statuspoint 10` (set-up by atcommand, in atcommandlog). The Novice starts
with a Knife and a Cotton Shirt worn (rAthena's start_items).

Played, all by pointer and keys through the control socket:
- Alt+E opens ITEMWND; its `tab:item`/`tab:equip`/`tab:etc` and the items
  it shows (`item:<index>`) are controls (PROTOCOL.md `controls`);
  double-click on the Red Potion uses one (UIItemWnd::OnLBtnDblClk ->
  CZ_USE_ITEM); HP from `self` before and after.
- the equip tab: double-click on the Main Gauche and the Suit wears them
  (CZ_REQ_WEAR_EQUIP); the `items` state part marks them worn.
- Alt+Q opens EQUIPWND, which shows both as `item:<index>`; Alt+A opens
  STATUSWND, whose `atk` and `def` texts ("24 + 0") change from before.
- double-click on the Suit in EQUIPWND takes it off (CZ_REQ_TAKEOFF_EQUIP).
- a Hat and Sunglasses (head top and middle) are worn too; after two map
  changes (@warp, set-up) the equipment window still lists every worn item
  and the trace has no assertion (the slots CSession::ClearEquipItem
  cleared only partly on arm64).
- the Red Potions are dragged from ITEMWND onto the ground; the count window
  (ITEMDROPCNTWND) takes the amount and Enter; the item lies on the ground.
- the `str` button of STATUSWND raises Str (CZ_STATUS_CHANGE).
"""

import time

from bfharness.accounts import unique_name
from bfharness.play import enter_game, fresh_character, login, own_home
from bfharness.sql import wait_rows

NAME = "inventory"
MILESTONE = "M8"
DESCRIPTION = "item window: use a potion, equip and unequip, drop; equipment and status windows"
MANIFEST = "port/harness/manifests/m8-inventory.sha256"
NEEDS_SERVER = True

START = ("prontera", 150, 150)
POTION, WEAPON, SUIT = 501, 1207, 2305
HAT, GLASSES = 2220, 2201         # head top and head middle, the last two equipment slots
KNIFE, SHIRT = 1201, 2301


def ok(s, cond, message):
    s.check(cond, message, passed=bool(cond))


def inv(ctl):
    return ctl.state("items")["items"] or []


def item(ctl, nameid):
    found = [i for i in inv(ctl) if i["id"] == nameid]
    return found[0] if found else None


def controls(ctl, window):
    w = ctl.window(window)
    return {c["id"]: c for c in w["controls"]} if w else {}


def centre(c):
    x, y, w, h = c["rect"]
    return x + w // 2, y + h // 2


def toggle(ctl, key, window, open=True):
    """Alt+<key> opens or closes a window; returns its controls."""
    if (ctl.window(window) is not None) != open:
        ctl.press(key, "alt")
        ctl.wait_window(window, open=open, timeout=10)
    ctl.wait({"frames": 3}, timeout=10)
    return controls(ctl, window)


def tab(ctl, name):
    c = controls(ctl, "ITEMWND")["tab:%s" % name]
    if c["text"] != "on":
        ctl.click(*centre(c))
        ctl.wait({"frames": 3}, timeout=10)
    return controls(ctl, "ITEMWND")


def double_click_item(ctl, window, it):
    c = controls(ctl, window).get("item:%d" % it["index"])
    if c is None:
        raise AssertionError("%s shows no item:%d (%s); controls %s"
                             % (window, it["index"], it["name"], sorted(controls(ctl, window))))
    # the previous double-click is past the double-click interval, so that
    # an item moved into the cell just clicked is not opened by one click
    time.sleep(0.6)
    ctl.move(*centre(c))
    ctl.wait({"frames": 40}, timeout=10)
    ctl.double_click(*centre(c))
    return c


def at_least(n, rows):
    return rows if len(rows) >= n else []


def wait_until(ctl, fn, timeout=10.0):
    deadline = time.monotonic() + timeout
    while True:
        v = fn()
        if v or time.monotonic() > deadline:
            return v
        ctl.wait({"frames": 5}, timeout=10)


def run(t):
    user = t.param("user") or unique_name(tag="i")
    password = t.param("password", "bfh-test")
    name = t.param("char") or unique_name(tag="I")

    c0 = fresh_character(t, user, password, name, t.param("sex", "M"), place=START, hp=10)
    char_id = c0["char_id"]

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)
    t.gm.ctl = ctl

    with t.step("set-up: items and status points", "atcommands typed by the client") as s:
        mark = t.logs.mark()
        cmds = (t.gm.item(POTION, 5), t.gm.item(WEAPON, 1), t.gm.item(SUIT, 1), t.gm.item(HAT, 1),
                t.gm.item(GLASSES, 1), "@statuspoint 10")
        for cmd in cmds:
            t.gm.run(cmd)
        s.rows("atcommandlog", wait_rows(lambda: at_least(len(cmds), t.logs.atcommands(char_id=char_id, after=mark)),
                                         timeout=10), min_rows=len(cmds))
        got = wait_until(ctl, lambda: all(item(ctl, i) for i in (POTION, WEAPON, SUIT, HAT, GLASSES)))
        s.state("inventory", inv(ctl))
        ok(s, got, "the items part lists the Red Potions, the Main Gauche and the Suit")

    with t.step("status window before", "Alt+A opens STATUSWND; its values as drawn") as s:
        st = toggle(ctl, "a", "STATUSWND")
        before = {k: st[k]["text"] for k in ("atk", "def", "stat:str", "need:str", "status_point") if k in st}
        s.state("STATUSWND values", before)
        ok(s, "atk" in before and "def" in before, "atk and def reported")
        s.frame("status-before")
        toggle(ctl, "a", "STATUSWND", open=False)

    with t.step("use a potion", "M8.1d: double-click in the item window uses a Red Potion; HP rises; picklog C") as s:
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", timeout=10)
        tab(ctl, "item")
        s.state("ITEMWND controls", controls(ctl, "ITEMWND"))
        s.frame("itemwnd")
        hp0 = ctl.state("self")["self"]["hp"]
        pot = item(ctl, POTION)
        mark = t.logs.mark()
        double_click_item(ctl, "ITEMWND", pot)
        hp1 = wait_until(ctl, lambda: ctl.state("self")["self"]["hp"] > hp0 and ctl.state("self")["self"]["hp"])
        after = item(ctl, POTION)
        s.note("HP %s -> %s; potions %d -> %s" % (hp0, hp1, pot["amount"], after and after["amount"]))
        ok(s, hp1 and hp1 > hp0, "HP rose")
        ok(s, after and after["amount"] == pot["amount"] - 1, "one potion fewer")
        s.rows("picklog C Red Potion", wait_rows(lambda: t.logs.items(char_id, POTION, "C", after=mark, sign=-1)))

    with t.step("equip weapon and armour", "M8.1d: double-click on equipment in the item window wears it") as s:
        tab(ctl, "equip")
        s.state("ITEMWND controls (equip tab)", controls(ctl, "ITEMWND"))
        for nameid in (WEAPON, SUIT, HAT, GLASSES):
            double_click_item(ctl, "ITEMWND", item(ctl, nameid))
            wait_until(ctl, lambda: (item(ctl, nameid) or {}).get("equipped"))
        s.state("inventory", inv(ctl))
        worn = {i["id"]: i["equipped"] for i in inv(ctl) if i["equipped"]}
        ok(s, all(worn.get(n) for n in (WEAPON, SUIT, HAT, GLASSES)),
           "weapon, Suit, Hat and Sunglasses worn: %r" % worn)
        ok(s, not worn.get(KNIFE) and not worn.get(SHIRT), "Knife and Cotton Shirt taken off by the swap")
        s.frame("equipped-itemwnd")
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", open=False, timeout=10)

    with t.step("equipment window", "M8.1d: Alt+Q opens EQUIPWND, which shows the Main Gauche and the Suit") as s:
        eq = toggle(ctl, "q", "EQUIPWND")
        s.state("EQUIPWND controls", eq)
        for nameid in (WEAPON, SUIT, HAT, GLASSES):
            it = item(ctl, nameid)
            ok(s, ("item:%d" % it["index"]) in eq, "EQUIPWND shows %s" % it["name"])
        s.frame("equipwnd")

    with t.step("status window after", "M8.1d: STATUSWND shows the changed ATK and DEF") as s:
        toggle(ctl, "q", "EQUIPWND", open=False)
        st = toggle(ctl, "a", "STATUSWND")
        now = {k: st[k]["text"] for k in ("atk", "def") if k in st}
        s.state("before, after", {"before": before, "after": now})
        ok(s, now["atk"] != before["atk"], "ATK changed: %s -> %s" % (before["atk"], now["atk"]))
        ok(s, now["def"] != before["def"], "DEF changed: %s -> %s" % (before["def"], now["def"]))
        s.frame("status-equipped")
        toggle(ctl, "a", "STATUSWND", open=False)

    with t.step("take the armour off", "M8.1d: double-click in EQUIPWND takes the Suit off") as s:
        toggle(ctl, "q", "EQUIPWND")
        double_click_item(ctl, "EQUIPWND", item(ctl, SUIT))
        off = wait_until(ctl, lambda: (item(ctl, SUIT) or {}).get("equipped") == 0)
        s.state("inventory", inv(ctl))
        ok(s, off, "the Suit is no longer worn")
        ok(s, ("item:%d" % item(ctl, SUIT)["index"]) not in controls(ctl, "EQUIPWND"), "EQUIPWND no longer shows it")
        ok(s, item(ctl, WEAPON)["equipped"], "the weapon is still worn")
        toggle(ctl, "q", "EQUIPWND", open=False)

    with t.step("equipment after a map change", "M8.1d: the worn items, head top and middle included, "
                "are listed again after a map change (ZC_EQUIPMENT_ITEMLIST), with no assertion") as s:
        mark = t.logs.mark()
        worn = sorted(i["index"] for i in inv(ctl) if i["equipped"])
        for cmd, m in ((t.gm.warp("izlude", 128, 110), "izlude"), (t.gm.warp(*START), START[0])):
            t.gm.run(cmd)
            ctl.wait_map(m, 60)
            ctl.wait({"frames": 30}, timeout=20)
        s.rows("atcommandlog @warp", t.logs.atcommands(char_id=char_id, contains="@warp", after=mark), min_rows=2)
        eq = toggle(ctl, "q", "EQUIPWND")
        s.state("EQUIPWND controls", sorted(eq))
        now = sorted(i["index"] for i in inv(ctl) if i["equipped"])
        s.equal(now, worn, "worn items (indices) after the map change")
        ok(s, all(("item:%d" % i) in eq for i in worn), "EQUIPWND shows them all")
        asserts = [] if t.mock else t.client.trace_lines("assertion failed")
        s.trace("assertion lines", asserts)
        ok(s, not asserts, "no assertion (CSession::AddEquipItem expects every slot cleared)")
        toggle(ctl, "q", "EQUIPWND", open=False)

    with t.step("drop an item", "M8.1d: drag from the item window onto the ground; picklog P with a negative amount") as s:
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", timeout=10)
        tab(ctl, "item")
        pot = item(ctl, POTION)
        c = controls(ctl, "ITEMWND")["item:%d" % pot["index"]]
        v = ctl.state("view")["view"]
        gx, gy = v["logical"]["w"] // 2 + 60, v["logical"]["h"] // 2 - 40
        mark = t.logs.mark()
        x0, y0 = centre(c)
        ctl.pointer("down", x0, y0)
        for k in range(1, 9):
            ctl.move(x0 + (gx - x0) * k // 8, y0 + (gy - y0) * k // 8)
        ctl.pointer("up", gx, gy)
        if pot["amount"] > 1:
            ctl.wait_window("ITEMDROPCNTWND", timeout=10)
            s.state("ITEMDROPCNTWND", ctl.window("ITEMDROPCNTWND"))
            s.frame("dropcount")
            ctl.press("enter")
        rows = s.rows("picklog P Red Potion dropped",
                      wait_rows(lambda: t.logs.items(char_id, POTION, "P", after=mark, sign=-1), timeout=10))
        ground = wait_until(ctl, lambda: [a for a in ctl.state("actors")["actors"]
                                          if a["type"] == "item" and a["job"] == POTION])
        s.state("on the ground", ground)
        ok(s, ground, "a Red Potion lies on the ground")
        ok(s, item(ctl, POTION) is None or item(ctl, POTION)["amount"] == pot["amount"] + rows[-1]["amount"],
           "the inventory lost what picklog says was dropped")
        dropped = -rows[-1]["amount"]
        s.frame("dropped")
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", open=False, timeout=10)

    with t.step("raise a stat", "M8.1d: the status window's Str button raises Str") as s:
        st = toggle(ctl, "a", "STATUSWND")
        before_str, before_pts = st["stat:str"]["text"], st["status_point"]["text"]
        ctl.click(*centre(st["str"]))
        st = wait_until(ctl, lambda: (lambda c: c if c["stat:str"]["text"] != before_str else None)(controls(ctl, "STATUSWND")))
        s.state("Str, status points", {"before": (before_str, before_pts),
                                       "after": st and (st["stat:str"]["text"], st["status_point"]["text"])})
        ok(s, st and int(st["stat:str"]["text"].split()[0]) == int(before_str.split()[0]) + 1, "Str one higher")
        ok(s, st and int(st["status_point"]["text"]) < int(before_pts), "status points spent")
        s.frame("status-raised")
        str_after, pts_after = int(st["stat:str"]["text"].split()[0]), int(st["status_point"]["text"])
        weapon_idx = item(ctl, WEAPON)["index"]

    t.stop_client()
    with t.step("server: inventory and stats", "after logout: inventory.equip and the char row agree") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]], timeout=20)
        c1 = rows[0]
        s.state("char str, status_point before and after", {k: (c0[k], c1[k]) for k in ("str", "status_point")})
        s.equal(c1["str"], str_after, "char.str")
        s.equal(c1["status_point"], pts_after, "char.status_point")
        invrows = t.logs.inventory(char_id)
        s.rows("inventory", invrows)
        by = {}
        for r in invrows:
            by.setdefault(r["nameid"], []).append(r)
        ok(s, WEAPON in by and by[WEAPON][0]["equip"] != 0, "Main Gauche worn (equip %s)" % [r["equip"] for r in by.get(WEAPON, [])])
        ok(s, SUIT in by and by[SUIT][0]["equip"] == 0, "Suit not worn (equip %s)" % [r["equip"] for r in by.get(SUIT, [])])
        ok(s, all(r["equip"] == 0 for r in by.get(KNIFE, []) + by.get(SHIRT, [])), "Knife and Cotton Shirt not worn")
        ok(s, all(by.get(n) and by[n][0]["equip"] != 0 for n in (HAT, GLASSES)), "Hat and Sunglasses worn")
        left = sum(r["amount"] for r in by.get(POTION, []))
        s.equal(left, 5 - 1 - dropped, "Red Potions left: 5 - 1 used - %d dropped" % dropped)
