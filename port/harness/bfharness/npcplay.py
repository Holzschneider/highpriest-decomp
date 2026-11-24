"""Shared steps of the M8.1e scripts (npc.py, shop.py, storage.py): a fresh
account with a character made in SQL at a chosen cell, with items, zeny and
skills (set-up only, offline); an NPC's definition as the server loads it;
and the NPC windows driven through the client: talk to an NPC, Next, a menu
choice, a number typed into the input window, the shop and storage windows.

The NPC windows of the 2008 client and the control ids the scripts use
(PROTOCOL.md `controls`, reported by port/compat/stubs/bf_client_state.cpp):

| Window | Opened by | Controls |
|---|---|---|
| SAYDIALOGWND | ZC_SAY_DIALOG | `text` (the dialogue so far) |
| (the button under it) | ZC_WAIT_DIALOG / ZC_CLOSE_DIALOG | `next` / `close` in SAYDIALOGWND |
| CHOOSEWND | ZC_MENU_LIST | `choice:<label>` per entry |
| NPCEDITDIALOGWND | ZC_OPENEDITDLG (number), ZC_OPENEDITDLGSTR (text) | `edit`, `ok`, `cancel` |
| CHOOSESELLBUYWND | ZC_SELECT_DEALTYPE | `buy`, `sell` |
| ITEMSHOPWND | ZC_PC_PURCHASE_ITEMLIST / ZC_PC_SELL_ITEMLIST | `item:<id>` |
| ITEMPURCHASEWND / ITEMSELLWND | with the shop or sell list | `item:<id>`, `ok`, `cancel` |
| ITEMDROPCNTWND | an item dragged into a deal, storage or item window | `edit`, `ok` |
| ITEMSTOREWND | ZC_STORE_NORMAL_ITEMLIST3, ZC_NOTIFY_STOREITEM_COUNTINFO | `item:<id>`, `tab:item/equip/etc`, `close` |
| ITEMWND (M8.1d) | Alt+E | `item:<inventory index>`, `tab:item/equip/etc` |

The deal and storage windows name an item by its id; the item window by the
server's inventory index (the `items` state part maps one to the other).
The helpers below take an item id for every window.
"""

import re
import subprocess
import time

from .accounts import unique_name
from .sql import SqlError, quote


def new_player(t, tag, map, x, y, zeny=0, items=(), skills=(), job=0, job_level=1, base_level=1,
               storage=()):
    """A fresh account (group 99, so atcommands may set scenes up) and one
    character in slot 0 at map x,y. items: (nameid, amount) in the
    inventory; skills: (skill id, level); storage: (nameid, amount) in the
    account's storage. Returns (userid, password, char row)."""
    userid = t.param("user") or unique_name(tag=tag)
    password = "bfh-npc"
    name = t.param("char") or unique_name(tag=tag + "c")
    aid = t.accounts.create_account(userid, password, group_id=99)
    c = t.accounts.make_char(aid, name, 0, map, x, y, **{
        "class": job, "base_level": base_level, "job_level": job_level, "zeny": zeny, "weapon": 1,
        "sex": "F", "save_map": map, "save_x": x, "save_y": y})
    mock = getattr(t.db, "db", None)
    if mock is not None:
        cid = c["char_id"]
        mock.update("char", cid, skills=dict(skills))
        for nameid, amount in items:
            iid = max([r["id"] for r in mock.rows("inventory")] or [0]) + 1
            mock.put("inventory", id=iid, char_id=cid, nameid=nameid, amount=amount, equip=0)
        for nameid, amount in storage:
            sid = max([r["id"] for r in mock.rows("storage")] or [0]) + 1
            mock.put("storage", id=sid, account_id=aid, nameid=nameid, amount=amount)
        return userid, password, t.logs.char(name)
    for nameid, amount in items:
        t.db.execute("INSERT INTO `inventory` (`char_id`, `nameid`, `amount`, `identify`) VALUES (%d, %d, %d, 1)"
                     % (c["char_id"], nameid, amount))
    for sid, lv in skills:
        t.db.execute("INSERT INTO `skill` (`char_id`, `id`, `lv`, `flag`) VALUES (%d, %d, %d, 0)"
                     % (c["char_id"], sid, lv))
    for nameid, amount in storage:
        t.db.execute("INSERT INTO `storage` (`account_id`, `nameid`, `amount`, `identify`) VALUES (%d, %d, %d, 1)"
                     % (aid, nameid, amount))
    return userid, password, c


def storage_rows(t, account_id, nameid=None):
    rows = t.db.select("storage", where={"account_id": account_id})
    return [r for r in rows if nameid is None or r["nameid"] == nameid]


def server_npc(t, map, x, y, kind, name_re, fallback):
    """The NPC's definition line as the server loads it, from the server's
    npc/ (pre-renewal files; `npc/re/` is not loaded). The mock: fallback."""
    if t.mock:
        return fallback
    r = subprocess.run(["docker", "compose", "-f", str(t.server.dir / "docker-compose.yml"), "exec", "-T",
                        "rathena", "sh", "-c",
                        "grep -rhE '^%s,%d,%d,[0-9]+\\s+%s\\s' npc/ --exclude-dir=re || true" % (map, x, y, kind)],
                       cwd=str(t.server.dir), capture_output=True, text=True, timeout=60)
    lines = [ln for ln in r.stdout.splitlines() if re.search(name_re, ln)]
    if not lines:
        raise AssertionError("no %s %r at %s %d,%d in the server's npc/" % (kind, name_re, map, x, y))
    return lines[0].strip()


# ---- the NPC windows, through the client ------------------------------------

def click_npc(ctl, pred, what, timeout=10, dy=-30):
    """Rest the pointer on the NPC (so its name is known), then click it,
    dy logical pixels above its feet (on its body: a short NPC in front, such
    as Prontera's mailbox before the Kafra, would take a click at the feet)."""
    deadline = time.monotonic() + timeout
    while True:
        a = ctl.actor(pred)
        if a and a.get("screen"):
            ctl.move(a["screen"]["x"], a["screen"]["y"] + dy)
            ctl.wait({"frames": 3}, timeout=10)
            ctl.click(a["screen"]["x"], a["screen"]["y"] + dy)
            return a
        if time.monotonic() > deadline:
            raise AssertionError("no %s on screen" % what)
        ctl.wait({"frames": 20}, timeout=10)


def controls(w):
    return {c["id"]: c for c in (w or {}).get("controls", [])}


def wait_control(ctl, wname, cid, timeout=15):
    """Wait until the window shows the control; returns the window."""
    deadline = time.monotonic() + timeout
    while True:
        w = ctl.window(wname)
        if w and cid in controls(w):
            return w
        if time.monotonic() > deadline:
            raise AssertionError("no control %s in %s within %ds (window: %r)" % (cid, wname, timeout, w))
        ctl.wait({"frames": 5}, timeout=10)


def center(rect):
    return rect[0] + rect[2] // 2, rect[1] + rect[3] // 2


THROTTLE = 0.35   # UISayDialogWnd/UIChooseWnd drop a Next or OK within 300 ms of the last (g_session.m_dwOldTime)


def choose(ctl, label, s=None):
    """Select the menu entry `label` (the entry equal to it, else the first
    that contains it: rAthena menus carry decorations such as " ~ ") in
    CHOOSEWND (ZC_MENU_LIST), scrolling the list if needed, and confirm with
    OK (CZ_CHOOSE_MENU). Returns (entries seen, entry chosen)."""
    ctl.wait_window("CHOOSEWND", timeout=15)
    seen = []
    for _ in range(12):
        w = ctl.window("CHOOSEWND")
        entries = [c for c in w.get("controls", []) if c["id"].startswith("choice:")]
        seen += [c["text"] for c in entries if c["text"] not in seen]
        hit = [c for c in entries if c["text"] == label] or [c for c in entries if label in c["text"]]
        if hit:
            break
        lst = controls(w).get("list")
        if not lst:
            break
        ctl.wheel(-1, *center(lst["rect"]))     # down one notch
        ctl.wait({"frames": 3}, timeout=10)
    if s is not None:
        s.state("menu", seen)
    if not hit:
        raise AssertionError("no menu entry %r among %r" % (label, seen))
    ctl.click(*center(hit[0]["rect"]))
    w = ctl.window("CHOOSEWND")
    if w and "ok" in controls(w):
        time.sleep(THROTTLE)
        ctl.click_control("CHOOSEWND", "ok")
    ctl.wait_window("CHOOSEWND", open=False, timeout=10)
    return seen, hit[0]["text"]


def type_into(ctl, wname, value, s=None, what="the number"):
    """Type value into the edit field of an input window (the NPC's number or
    text input, or a deal window's amount), which opens with its text
    selected, and confirm with Enter."""
    w = wait_control(ctl, wname, "edit")
    before = controls(w)["edit"]["text"]
    ctl.type_text(str(value))
    w = ctl.window(wname)
    if s is not None:
        s.state("%s: edit field before and after typing" % wname, [before, controls(w)["edit"]["text"]])
        s.equal(controls(w)["edit"]["text"], str(value), "%s in %s" % (what, wname))
    ctl.press("enter")
    ctl.wait_window(wname, open=False, timeout=10)


def run_dialogue(ctl, s, answers=(), numbers=(), texts=(), timeout=60, frames=None):
    """Play an NPC conversation through its windows until it ends with Close:
    Next on SAYDIALOGWND, the next of `answers` in each menu, the next of
    `numbers` in each number input (NPCEDITDIALOGWND), the next of `texts`
    in each text input (NPCTEXTEDITDIALOGWND). frames: {"number"/"text"/
    "menu": frame name}, captured the first time that window is answered.
    Returns the transcript, a list of (kind, detail), which also goes into
    the step's evidence."""
    frames = dict(frames or {})

    def frame(kind):
        if kind in frames:
            ctl.wait({"frames": 5}, timeout=10)
            s.frame(frames.pop(kind))

    answers, numbers, texts = list(answers), list(numbers), list(texts)
    log = []
    deadline = time.monotonic() + timeout
    while True:
        if time.monotonic() > deadline:
            s.state("dialogue so far", log)
            raise AssertionError("the conversation did not end within %ds" % timeout)
        names = {w["name"]: w for w in ctl.state("windows")["windows"]}
        if "NPCTEXTEDITDIALOGWND" in names and texts:
            frame("text")
            v = texts.pop(0)
            type_into(ctl, "NPCTEXTEDITDIALOGWND", v, s, "the text")
            log.append(("text input", v))
        elif "NPCEDITDIALOGWND" in names and numbers:
            frame("number")
            v = numbers.pop(0)
            type_into(ctl, "NPCEDITDIALOGWND", v, s, "the number")
            log.append(("number input", v))
        elif "CHOOSEWND" in names and answers:
            frame("menu")
            seen, chosen = choose(ctl, answers.pop(0))
            log.append(("menu", {"entries": seen, "chosen": chosen}))
        elif "SAYDIALOGWND" in names and ("next" in controls(names["SAYDIALOGWND"])
                                           or "close" in controls(names["SAYDIALOGWND"])):
            c = controls(names["SAYDIALOGWND"])
            button = "next" if "next" in c else "close"
            log.append(("say", c.get("text", {}).get("text")))
            time.sleep(THROTTLE)
            ctl.click_control("SAYDIALOGWND", button)
            log.append((button, None))
            if button == "close":
                ctl.wait_window("SAYDIALOGWND", open=False, timeout=10)
                s.state("dialogue", log)
                if answers or numbers or texts:
                    raise AssertionError("closed with answers left: %r %r %r" % (answers, numbers, texts))
                return log
            # the button goes; the server's answer brings the next one
            ctl.wait({"frames": 5}, timeout=10)
        else:
            ctl.wait({"frames": 5}, timeout=10)


INDEXED = ("ITEMWND", "EQUIPWND")   # windows that name items by inventory index (M8.1d)


def inventory_items(ctl, nameid, worn=False):
    """The `items` part's entries for an item id, in the inventory (or worn)."""
    return [i for i in (ctl.state("items").get("items") or [])
            if i["id"] == nameid and bool(i.get("equipped")) == worn]


def item_control(ctl, wname, nameid):
    """The control showing item nameid in window wname: `item:<id>` in the
    deal and storage windows, `item:<inventory index>` in the item window."""
    w = ctl.window(wname)
    if not w:
        raise AssertionError("%s is not open" % wname)
    c = controls(w)
    keys = ["item:%d" % i["index"] for i in inventory_items(ctl, nameid, wname == "EQUIPWND")] \
        if wname in INDEXED else ["item:%d" % nameid]
    for k in keys:
        if k in c:
            return c[k]
    raise AssertionError("no item %d in %s (looked for %r): %r" % (nameid, wname, keys,
                         [k for k in c if k.startswith("item:")]))


def wait_item(ctl, wname, nameid, timeout=15):
    deadline = time.monotonic() + timeout
    while True:
        try:
            return item_control(ctl, wname, nameid)
        except AssertionError:
            if time.monotonic() > deadline:
                raise
        ctl.wait({"frames": 5}, timeout=10)


def drag_item(ctl, src, nameid, dst):
    """Drag item nameid from window src onto window dst with the left
    button (down on the item, moves, up over dst), as a player moves items
    between the shop, deal, inventory and storage windows."""
    c = item_control(ctl, src, nameid)
    d = ctl.window(dst)
    if not d:
        raise AssertionError("%s is not open" % dst)
    x0, y0 = center(c["rect"])
    x1, y1 = center(d["rect"])
    ctl.drag(x0, y0, x1, y1, button="left")
    ctl.wait({"frames": 3}, timeout=10)
    return c


def item_count(ctl, wname, nameid):
    """How many of an item a window shows: the `<name> <count>` text of the
    deal and storage windows; for the item window, the `items` part's amount
    of the stacks it shows (its controls carry the name only)."""
    if wname in INDEXED:
        w = ctl.window(wname)
        shown = controls(w) if w else {}
        return sum(i["amount"] for i in inventory_items(ctl, nameid, wname == "EQUIPWND")
                   if "item:%d" % i["index"] in shown)
    w = ctl.window(wname)
    c = controls(w).get("item:%d" % nameid) if w else None
    return int(c["text"].rsplit(" ", 1)[1]) if c else 0


def click_tab(ctl, wname, name):
    """A vertical tab of the item or storage window: "item", "equip" or "etc"."""
    wait_control(ctl, wname, "tab:%s" % name)
    ctl.click_control(wname, "tab:%s" % name)
    ctl.wait({"frames": 3}, timeout=10)


DEAL_WINDOWS = {"buy": "ITEMPURCHASEWND", "sell": "ITEMSELLWND"}


def deal(ctl, dealer, kind, nameid, amount, s=None, frames=None, what="Tool Dealer"):
    """Buy or sell one item at a shop NPC through the client's deal windows:
    click the dealer (the actor predicate `dealer`), `kind` ("buy"/"sell")
    in CHOOSESELLBUYWND, drag the item from ITEMSHOPWND into the purchase or
    sell window, type the amount into ITEMDROPCNTWND, check the window's
    count and press its OK (CZ_PC_PURCHASE_ITEMLIST / CZ_PC_SELL_ITEMLIST).
    A stack of one (the sell list's `<name> 1`) moves without the count
    window, as every drag of a single item in the 2008 client.
    frames: {"amount"/"list": frame name}, the count window and the filled
    deal window. The server's records are the caller's to check."""
    frames = frames or {}
    target = DEAL_WINDOWS[kind]
    click_npc(ctl, dealer, what)
    ctl.wait_window("CHOOSESELLBUYWND", timeout=15)
    ctl.wait({"frames": 5}, timeout=10)
    ctl.click_control("CHOOSESELLBUYWND", kind)
    ctl.wait_window("CHOOSESELLBUYWND", open=False, timeout=10)
    ctl.wait_window(target, timeout=15)
    wait_control(ctl, "ITEMSHOPWND", "item:%d" % nameid)
    if s is not None:
        s.state("%s list" % kind, [c["text"] for c in ctl.window("ITEMSHOPWND")["controls"]
                                   if c["id"].startswith("item:")])
    have = item_count(ctl, "ITEMSHOPWND", nameid)
    drag_item(ctl, "ITEMSHOPWND", nameid, target)
    if have != 1:
        ctl.wait_window("ITEMDROPCNTWND", timeout=10)
        if s is not None and "amount" in frames:
            s.frame(frames["amount"])
        type_into(ctl, "ITEMDROPCNTWND", amount, s, "the amount")
    elif s is not None:
        # a single item moves without the count window (m_numDragItem == 1)
        s.note("one %d in the list: moved without the count window" % nameid)
    wait_control(ctl, target, "item:%d" % nameid)
    if s is not None:
        s.equal(item_count(ctl, target, nameid), amount, "items in %s" % target)
        s.state("%s controls" % target, ctl.window(target)["controls"])
        if "list" in frames:
            s.frame(frames["list"])
    ctl.click_control(target, "ok")
    ctl.wait_window(target, open=False, timeout=15)


def sent(t, header):
    """Packets with that header ("0x00b8") the client sent (BF_PKT_LOG), as
    "<header> <name> <size>" lines; the mock sends none."""
    from . import pktlog
    if t.mock or not t.client:
        return []
    return ["%s %s %d" % (p.header, p.name, p.size) for p in pktlog.read(t.client.pkt_log)
            if p.dir == "S" and p.header == header]


def check_sent(t, s, headers):
    """Each of headers ({header: what}) was sent at least once (live runs)."""
    if t.mock:
        s.note("packet log: not kept by the mock")
        return
    got = {h: sent(t, h) for h in headers}
    s.state("packets sent", {"%s (%s)" % (h, w): len(got[h]) for h, w in headers.items()})
    for h, w in headers.items():
        s.check(got[h], "%s (%s) in the packet log" % (w, h))


def focus_chat(ctl):
    """Give the chat line the keyboard before typing a line. Closing an input
    window (UINpcEditDialogWnd, deleted) leaves no edit field focused
    (UIWindowMgr::ProcessInput clears m_editWindow), so the first Enter only
    focuses the chat line again (UM_DEFPUSH to the frame windows), as in the
    2008 client."""
    w = ctl.window("CHATWND")
    if w and not w.get("focus"):
        ctl.press("enter")
        ctl.wait({"frames": 3}, timeout=10)
