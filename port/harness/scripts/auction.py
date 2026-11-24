"""M8.4 auction: the 2008 auction window works live against stock rAthena (PACKETVER 20080401).

Done when (implementation plan M8.4, auction): with `feature.auction: on`
on the test server, a seller registers an item through the client's
auction window, a buyer on another account finds it by a name search and
bids, the seller ends the auction (CZ_AUCTION_REQ_MY_SELL_STOP, 0x25d,
answered by rAthena with a 4-byte 0x25d), and the buyer takes the item out
of the letter it arrives in; each step confirmed in the server's tables.

How the window opens: the 2008 client opens UIAuctionWnd only on the
server's ZC_AUCTION_WINDOWS (CGameMode::Zc_Auction_Windows); there is no
key or slash command for it. rAthena sends it for the script command
`openauction`, which no NPC of its npc/ uses, and for the atcommand
`@auction`; the player types `@auction` (set-up, like the scene's warps).
Everything after it is clicks, drags and typing in the auction windows.

Set-up (SQL, offline): two group-99 accounts with one Novice each. The
seller holds a Poring Card (identified) and 50000 zeny; the buyer 20000
zeny and stands beside Prontera's Mailbox (146,86).
Played, four client sessions, one character at a time:
1. seller: `@auction`, the register tab, the card dragged from the item
   window (Alt+E, etc tab) into the window (CZ_AUCTION_ADD_ITEM), start
   price, buy-now price and hours typed, Add (CZ_AUCTION_ADD). Evidence:
   the `auction` row (seller, prices, nameid 4001), picklog I -1 for the
   card, zenylog I -12000 (the fee, 12000 a hour), the client's inventory
   without it; the sell tab lists the auction.
2. buyer: `@auction`, the name search for "Poring Card" (CZ_AUCTION_ITEM_SEARCH
   type 4) lists the seller's auction; a right-click on its name opens the
   menu, its first entry the bid window, the bid typed and OK
   (CZ_AUCTION_BUY). Evidence: zenylog I -bid, `auction.buyer_id`/`price`.
3. seller: the sell tab, the auction selected, Finish, OK in the message
   window (CZ_AUCTION_REQ_MY_SELL_STOP). Evidence: the client's "You have
   ended the auction" line from the 4-byte 0x25d and no unknown header
   after it (the 0x25d fix of M8.4, live), the `auction` row gone, a letter
   with the bid to the seller and one with the card to the buyer (`mail`,
   `mail_attachments`).
4. buyer: the Mailbox (130 zeny, as mail.py), the letter from the auction
   opened, its item taken (CZ_MAIL_GET_ITEM). Evidence: picklog E +1 card,
   the attachment row gone, the card in the client's `items`.
rAthena logs the auction's item and zeny moves as type I (LOG_TYPE_AUCTION),
the letter's item as E (LOG_TYPE_MAIL). The client's packet log must hold
every auction packet of the play and the trace no unknown header.
"""

import time

from bfharness import npcplay
from bfharness.play import enter_game, login, own_home
from bfharness.sql import wait_rows

NAME = "auction"
MILESTONE = "M8"
DESCRIPTION = "auction window live: register, search, bid, end (0x25d), item by mail; auction, mail tables"
MANIFEST = "port/harness/manifests/m8-auction.sha256"
NEEDS_SERVER = True

CARD, CARD_NAME = 4001, "Poring Card"
SELLER_ZENY, BUYER_ZENY = 50000, 20000
START_PRICE, BUY_NOW, HOURS, BID = 1000, 5000, 1, 2000
FEE_PER_HOUR = 12000                      # conf/battle/misc.conf auction_feeperhour
MAILBOX = (146, 86)
SELLER_START = ("prontera", 156, 180)
BUYER_START = ("prontera", 146, 89)
AUCTION_HEADERS = {"0x025f": "ZC_AUCTION_WINDOWS", "0x024b": "CZ_AUCTION_CREATE",
                   "0x024c": "CZ_AUCTION_ADD_ITEM", "0x0256": "ZC_ACK_AUCTION_ADD_ITEM",
                   "0x024d": "CZ_AUCTION_ADD", "0x0250": "ZC_AUCTION_RESULT",
                   "0x0251": "CZ_AUCTION_ITEM_SEARCH", "0x0252": "ZC_AUCTION_ITEM_REQ_SEARCH",
                   "0x024f": "CZ_AUCTION_BUY", "0x025c": "CZ_AUCTION_REQ_MY_INFO",
                   "0x025d": "CZ_AUCTION_REQ_MY_SELL_STOP / its answer"}


def controls(ctl, wname):
    return npcplay.controls(ctl.window(wname))


def inside(w, c):
    wx, wy, ww, wh = w["rect"]
    x, y, cw, ch = c["rect"]
    return wx <= x and x + cw <= wx + ww and wy <= y and y + ch <= wy + wh


def shown(ctl, wname):
    """The window's controls that lie inside it (the 2008 windows park
    unused controls at -100,-100)."""
    w = ctl.window(wname)
    return {c["id"]: c for c in (w or {}).get("controls", []) if inside(w, c)} if w else {}


def click(ctl, c, button="left"):
    x, y, w, h = c["rect"]
    ctl.click(x + min(w, 40) // 2, y + h // 2, button=button)


def frames(ctl, n=10):
    ctl.wait({"frames": n}, timeout=20)


def wait_shown(ctl, wname, pred, what, timeout=20):
    deadline = time.monotonic() + timeout
    while True:
        cs = shown(ctl, wname)
        hit = [c for c in cs.values() if pred(c)]
        if hit:
            return hit[0]
        if time.monotonic() > deadline:
            raise AssertionError("%s: not shown in %s (%r)" % (what, wname, sorted(cs)))
        frames(ctl, 5)


def type_into(ctl, wname, cid, text):
    """Click an edit field, clear it and type."""
    c = controls(ctl, wname)[cid]
    click(ctl, c)
    frames(ctl, 3)
    ctl.press("end")
    for _ in range(len(c.get("text") or "") + 2):
        ctl.press("backspace")
    ctl.type_text(text)
    frames(ctl, 3)
    return controls(ctl, wname)[cid].get("text")


def open_auction(t, ctl, s):
    """`@auction` in the chat line; ZC_AUCTION_WINDOWS opens the window."""
    ctl.say("@auction")
    ctl.wait_window("AUCTIONWND", timeout=15)
    frames(ctl, 20)
    w = ctl.window("AUCTIONWND")
    s.state("AUCTIONWND", {"rect": w["rect"], "controls": sorted(shown(ctl, "AUCTIONWND"))})


def tab(ctl, name):
    ctl.click_control("AUCTIONWND", "auction_" + name)
    frames(ctl, 15)


def session(t, user, password):
    # login() captures "login-typed" in every session; the file holds the
    # last one, so the manifest keeps only that record.
    t.frames.records = [r for r in t.frames.records if r.get("name") != "login-typed"]
    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    frames(ctl, 40)
    return ctl


def auctions(t, **where):
    return t.db.select("auction", where=where)


def mails(t, **where):
    return t.db.select("mail", where=where, order="id")


def attachments(t, mail_id):
    return t.db.select("mail_attachments", where={"id": mail_id})


def letters(t, after_id, *dest_ids):
    m = [r for r in mails(t) if r["id"] > after_id and r["dest_id"] in dest_ids]
    return m if len(m) >= len(dest_ids) else []


def mailbox(ctl, s):
    """Prontera's Mailbox (npc/other/mail.txt): click, its dialogue with
    "Yes." in the menu (130 zeny), then `openmail` opens the mail list."""
    npcplay.click_npc(ctl, lambda a: a["type"] == "npc" and a["name"].startswith("Mailbox"), "Mailbox", dy=-10)
    ctl.wait_window("SAYDIALOGWND", timeout=15)
    npcplay.run_dialogue(ctl, s, answers=["Yes."])
    ctl.wait_window("MAILLISTWND", timeout=15)
    frames(ctl, 20)


def run(t):
    with t.step("set-up: seller and buyer", "SQL, offline: two accounts; a Poring Card and zeny") as s:
        s_user, _, seller = npcplay.new_player(t, "as", *SELLER_START, zeny=SELLER_ZENY, items=[(CARD, 1)])
        b_user, _, buyer = npcplay.new_player(t, "ab", *BUYER_START, zeny=BUYER_ZENY)
        s_pw = b_pw = "bfh-npc"
        for who, c in (("seller", seller), ("buyer", buyer)):
            s.state(who, {k: c[k] for k in ("char_id", "account_id", "name", "zeny", "last_map", "last_x", "last_y")})
        s.rows("seller's inventory", t.logs.inventory(seller["char_id"], CARD))
        s.state("open auctions before", [{k: r[k] for k in ("auction_id", "seller_name", "item_name", "price")}
                                         for r in auctions(t)])
    sid, bid = seller["char_id"], buyer["char_id"]
    packets = {}
    unknown = []
    sessions = []

    def stop(t):
        client = t.client
        unknown.extend(client.trace_lines("unknown packet header"))
        t.stop_client()
        path = client.pkt_log
        if path and path.exists():
            for line in path.read_text(errors="replace").splitlines():
                f = line.split()
                if len(f) >= 4 and f[2].lower() in AUCTION_HEADERS:
                    key = "%s %s %s" % (f[1], f[2].lower(), f[3])
                    packets[key] = packets.get(key, 0) + 1
            # each session's log kept apart (the next client writes the same path)
            path.rename(path.with_name("client-packets-%d.txt" % (len(sessions) + 1)))
        sessions.append(client)

    # -- 1. the seller registers the card -----------------------------------
    ctl = session(t, s_user, s_pw)
    with t.step("seller: open the auction window", "M8.4 auction: @auction -> ZC_AUCTION_WINDOWS opens AUCTIONWND") as s:
        open_auction(t, ctl, s)
        s.frame("auction-view")
        c = shown(ctl, "AUCTIONWND")
        for cid in ("auction_view", "auction_add", "auction_sell", "auction_buy", "close"):
            s.check(cid in c, "the window shows its control %s" % cid)

    with t.step("seller: register the card", "M8.4 auction: drag, prices, Add; auction row, picklog I, zenylog I") as s:
        mark = t.logs.mark()
        tab(ctl, "add")
        c = shown(ctl, "AUCTIONWND")
        s.state("register tab", {k: v.get("text") for k, v in c.items()})
        for cid in ("now_money", "max_money", "delete_hour", "add_auction", "cancel"):
            s.check(cid in c, "the register tab shows %s" % cid)
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", timeout=10)
        npcplay.click_tab(ctl, "ITEMWND", "etc")
        item = npcplay.item_control(ctl, "ITEMWND", CARD)
        w = ctl.window("AUCTIONWND")
        x0, y0 = npcplay.center(item["rect"])
        ctl.drag(x0, y0, w["rect"][0] + 36, w["rect"][1] + 160, button="left")
        added = wait_shown(ctl, "AUCTIONWND", lambda c: c["id"] == "add_item" and c.get("text"), "the card's name")
        s.state("item in the window", added)
        s.check(CARD_NAME in added["text"], "the window names the card", text=added["text"])
        s.check(not npcplay.inventory_items(ctl, CARD), "the client took the card out of its inventory")
        ctl.press("e", "alt")                 # close the item window
        frames(ctl, 10)
        got = [type_into(ctl, "AUCTIONWND", cid, str(v))
               for cid, v in (("now_money", START_PRICE), ("max_money", BUY_NOW), ("delete_hour", HOURS))]
        s.state("typed (now, max, hours)", got)
        s.frame("auction-register")
        after = ctl.last_seq
        ctl.click_control("AUCTIONWND", "add_auction")
        rows = s.rows("auction row of %s" % seller["name"],
                      wait_rows(lambda: auctions(t, seller_id=sid), timeout=20))
        a = rows[-1]
        s.equal((a["nameid"], a["price"], a["buynow"], a["hours"], a["buyer_id"]),
                (CARD, START_PRICE, BUY_NOW, HOURS, 0), "nameid, price, buy-now, hours, no buyer")
        s.rows("picklog I -1 (the card to the auction)",
               wait_rows(lambda: [r for r in t.logs.items(sid, CARD, "I", after=mark) if r["amount"] == -1]))
        z = s.rows("zenylog I (the fee)", wait_rows(lambda: t.logs.zeny(sid, "I", after=mark)))
        s.equal(z[-1]["amount"], -HOURS * FEE_PER_HOUR, "fee")
        try:
            ev = ctl.wait_event("chat", lambda e: e.get("channel") == "system", after=after, timeout=10)
            s.state("client's message", ev)
        except Exception as e:
            s.note("no system line after Add: %s" % e)
    auction_id = a["auction_id"]

    with t.step("seller: the sell tab lists it", "M8.4 auction: CZ_AUCTION_REQ_MY_INFO 0 -> ZC_AUCTION_ITEM_REQ_SEARCH") as s:
        tab(ctl, "sell")
        row = wait_shown(ctl, "AUCTIONWND", lambda c: c["id"] == "auction:%d" % auction_id, "the auction row")
        s.state("row", row)
        s.check(CARD_NAME in (row.get("text") or ""), "the row names the card")
        s.frame("auction-sell")
    stop(t)

    # -- 2. the buyer finds it and bids -------------------------------------
    ctl = session(t, b_user, b_pw)
    with t.step("buyer: search by name", "M8.4 auction: CZ_AUCTION_ITEM_SEARCH type 4 lists the seller's card") as s:
        open_auction(t, ctl, s)
        c = shown(ctl, "AUCTIONWND")
        s.state("view tab", {k: v.get("text") for k, v in c.items()})
        if c.get("search_by:name", {}).get("text") != "on":
            click(ctl, c["search_by:name"])
            frames(ctl, 5)
        type_into(ctl, "AUCTIONWND", "search_edit", CARD_NAME)
        ctl.click_control("AUCTIONWND", "search")
        row = wait_shown(ctl, "AUCTIONWND", lambda c: c["id"] == "auction:%d" % auction_id, "the auction row")
        cs = shown(ctl, "AUCTIONWND")
        s.state("result rows", {k: v.get("text") for k, v in cs.items() if ":" in k and k.split(":")[0]
                                in ("auction", "seller", "buyer")})
        s.equal(cs.get("seller:%d" % auction_id, {}).get("text"), seller["name"], "seller shown")
        s.frame("auction-search")

    with t.step("buyer: bid", "M8.4 auction: right-click, the bid entry, the price, OK (CZ_AUCTION_BUY); zenylog I") as s:
        mark = t.logs.mark()
        click(ctl, row, button="right")
        ctl.wait_window("IMGMENUWND", timeout=10)
        frames(ctl, 5)
        s.frame("auction-menu")
        ctl.click_control("IMGMENUWND", "menu:0")
        ctl.wait_window("AUCTIONMSGWND", timeout=10)
        frames(ctl, 5)
        got = type_into(ctl, "AUCTIONMSGWND", "money", str(BID))
        s.state("bid typed", got)
        s.frame("auction-bid")
        ctl.click_control("AUCTIONMSGWND", "ok")
        z = s.rows("zenylog I (the bid)", wait_rows(lambda: t.logs.zeny(bid, "I", after=mark), timeout=20))
        s.equal(z[-1]["amount"], -BID, "bid paid")
        rows = s.rows("auction %d with its bidder" % auction_id,
                      wait_rows(lambda: [r for r in auctions(t, auction_id=auction_id) if r["buyer_id"] == bid],
                                timeout=20))
        s.equal(rows[-1]["price"], BID, "price is the bid")
    stop(t)

    # -- 3. the seller ends the auction (0x25d) -----------------------------
    ctl = session(t, s_user, s_pw)
    with t.step("seller: end the auction", "M8.4 auction: Finish, OK -> CZ_AUCTION_REQ_MY_SELL_STOP; 4-byte 0x25d") as s:
        mark_mail = max([r["id"] for r in mails(t)] or [0])
        open_auction(t, ctl, s)
        tab(ctl, "sell")
        row = wait_shown(ctl, "AUCTIONWND", lambda c: c["id"] == "auction:%d" % auction_id, "the auction row")
        click(ctl, row)
        finish = wait_shown(ctl, "AUCTIONWND", lambda c: c["id"] == "finish_auction", "the Finish button")
        s.frame("auction-selected")
        click(ctl, finish)
        ctl.wait_window("AUCTIONMSGWND", timeout=10)
        frames(ctl, 5)
        s.frame("auction-finish")
        after = ctl.last_seq
        ctl.click_control("AUCTIONMSGWND", "ok")
        ev = ctl.wait_event("chat", lambda e: "ended the auction" in (e.get("text") or ""), after=after, timeout=20)
        s.state("client's message (0x25d result 0)", ev)
        gone = wait_rows(lambda: [1] if not auctions(t, auction_id=auction_id) else [], timeout=20)
        s.check(gone, "the auction row is gone")
        new = s.rows("letters from the auction", wait_rows(lambda: letters(t, mark_mail, sid, bid), timeout=20),
                     min_rows=2)
        to_seller = [r for r in new if r["dest_id"] == sid]
        to_buyer = [r for r in new if r["dest_id"] == bid]
        s.check(to_seller and to_seller[-1]["zeny"] == BID, "the seller's letter carries the bid", rows=to_seller)
        s.check(to_buyer, "a letter to the buyer")
        att = s.rows("the buyer's letter's attachment", attachments(t, to_buyer[-1]["id"]))
        s.equal(att[0]["nameid"], CARD, "attached item")
        frames(ctl, 20)
        s.frame("auction-ended")
    buyer_mail = to_buyer[-1]
    stop(t)
    with t.step("0x25d read as 4 bytes", "M8.4: the client's packet log has the 4-byte answer; no unknown header") as s:
        s.state("auction packets of the sessions so far", packets)
        s.check(any(k.startswith("R 0x025d 4") for k in packets), "0x25d received with 4 bytes")
        s.check(any(k.startswith("S 0x025d") for k in packets), "0x25d sent")

    # -- 4. the buyer takes the card out of the letter ----------------------
    ctl = session(t, b_user, b_pw)
    with t.step("buyer: the letter's item", "M8.4 auction: Mailbox, the letter, its item taken (CZ_MAIL_GET_ITEM); picklog E") as s:
        mark = t.logs.mark()
        mailbox(ctl, s)
        w = ctl.window("MAILLISTWND")
        s.state("MAILLISTWND controls", [{k: c.get(k) for k in ("id", "text")} for c in w.get("controls", [])])
        header = [c for c in w.get("controls", []) if c["id"].startswith("header")
                  and (c.get("text") or "") == buyer_mail["title"] and inside(w, c)]
        s.check(header, "the auction's letter is listed", title=buyer_mail["title"])
        click(ctl, header[-1])
        ctl.wait_window("MAILVIEWWND", timeout=15)
        frames(ctl, 20)
        s.frame("auction-letter")
        v = ctl.window("MAILVIEWWND")
        x, y = v["rect"][0], v["rect"][1]
        ctl.click(x + 260, y + 345)            # UIMailViewWnd::OnLBtnDown: the attachment box
        rows = s.rows("picklog E +1 (the card from the letter)",
                      wait_rows(lambda: [r for r in t.logs.items(bid, CARD, "E", after=mark) if r["amount"] == 1],
                                timeout=20))
        gone = wait_rows(lambda: [1] if not attachments(t, buyer_mail["id"]) else [], timeout=20)
        s.check(gone, "the letter's attachment row is gone")
        have = wait_rows(lambda: npcplay.inventory_items(ctl, CARD), timeout=10)
        s.state("the card in the client's items", have)
        s.check(have, "the client holds the card")
        frames(ctl, 10)
        s.frame("auction-taken")

    stop(t)
    with t.step("packets", "the auction packets in both directions, no unknown header") as s:
        s.state("auction packets (direction, header, length: count)", packets)
        seen = {AUCTION_HEADERS[k.split()[1]] for k in packets}
        if not t.mock:
            s.check(set(AUCTION_HEADERS.values()) <= seen, "every auction packet of the play was seen",
                    missing=sorted(set(AUCTION_HEADERS.values()) - seen))
        s.trace("unknown packet header lines", unknown)
        s.check(not unknown, "no unknown packet header")
