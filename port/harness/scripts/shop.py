"""M8.1e shops: buy and sell at Prontera's Tool Dealer through the client's
deal windows, the amounts typed into the client's count window.

Done when: talking to the dealer opens the deal type window
(ZC_SELECT_DEALTYPE into CHOOSESELLBUYWND); Buy opens the shop list and the
purchase window (ZC_PC_PURCHASE_ITEMLIST into ITEMSHOPWND and
ITEMPURCHASEWND); a Red Potion dragged from the list into the purchase
window asks for the amount (ITEMDROPCNTWND), 2 typed, and Buy sends the deal
(CZ_PC_PURCHASE_ITEMLIST): picklog S +2 Red Potion, zenylog S -100 (2 x 50
zeny). Sell opens the player's sellable items and the sell window
(ZC_PC_SELL_ITEMLIST into ITEMSHOPWND in sell mode and ITEMSELLWND); 5
Jellopy dragged across, 5 typed, Sell sends the deal (CZ_PC_SELL_ITEMLIST):
picklog S -5 Jellopy, zenylog S +15 (5 x 3 zeny). After logout the
inventory holds 2 Red Potions and 5 Jellopy, and char.zeny is 1000 - 100 +
15.

The dealer: Tool Dealer#prt1, prt_in 126,76, a `shop` in the server's
npc/merchants/shops.txt (read at run time). Set-up (SQL, offline): a fresh
account, a Novice at prt_in 126,72 with 1000 zeny and 10 Jellopy. Played:
clicks, drags and typed amounts only.
"""

from bfharness import npcplay
from bfharness.play import enter_game, login, own_home
from bfharness.sql import wait_rows

NAME = "shop"
MILESTONE = "M8"
DESCRIPTION = "Tool Dealer in prt_in: buy 2 Red Potions, sell 5 Jellopy; amounts typed"
MANIFEST = "port/harness/manifests/m8-shop.sha256"
NEEDS_SERVER = True

JELLOPY, RED_POTION = 909, 501
DEALER = ("prt_in", 126, 76)
START = ("prt_in", 126, 72)
ZENY, JELLOPIES = 1000, 10
BUY, SELL = 2, 5
POTION_PRICE, JELLOPY_SELL = 50, 3     # item_db: Red Potion buy 50; Jellopy sells for 6 / 2


def dealer(a):
    return a["type"] == "npc" and (a["x"], a["y"]) == DEALER[1:]


def deal_type(ctl, button):
    npcplay.click_npc(ctl, dealer, "Tool Dealer")
    ctl.wait_window("CHOOSESELLBUYWND", timeout=15)
    ctl.wait({"frames": 5}, timeout=10)
    ctl.click_control("CHOOSESELLBUYWND", button)
    ctl.wait_window("CHOOSESELLBUYWND", open=False, timeout=10)


def run(t):
    with t.step("set-up: account and character", "SQL, offline: 1000 zeny, 10 Jellopy at the dealer") as s:
        user, password, c = npcplay.new_player(t, "s", *START, zeny=ZENY, items=[(JELLOPY, JELLOPIES)])
        char_id, name = c["char_id"], c["name"]
        s.state("char row", {k: c[k] for k in ("char_id", "name", "zeny", "last_map", "last_x", "last_y")})
        line = npcplay.server_npc(t, *DEALER, "shop", r"\sTool Dealer",
                                  "prt_in,126,76,0\tshop\tTool Dealer#prt1\t53,611:-1,501:-1,502:-1")
        s.state("Tool Dealer on the server", line)
        s.check(",%d:" % RED_POTION in line, "the dealer sells Red Potions")

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    with t.step("buy 2 Red Potions", "M8.1e shop: buy; picklog S, zenylog S, amount typed") as s:
        mark = t.logs.mark()
        zeny0 = ctl.state("self")["self"]["zeny"]
        deal_type(ctl, "buy")
        ctl.wait_window("ITEMPURCHASEWND", timeout=15)
        npcplay.wait_control(ctl, "ITEMSHOPWND", "item:%d" % RED_POTION)
        s.state("shop list", [c["text"] for c in ctl.window("ITEMSHOPWND")["controls"]
                              if c["id"].startswith("item:")])
        npcplay.drag_item(ctl, "ITEMSHOPWND", RED_POTION, "ITEMPURCHASEWND")
        ctl.wait_window("ITEMDROPCNTWND", timeout=10)
        s.frame("buy-amount")
        npcplay.type_into(ctl, "ITEMDROPCNTWND", BUY, s, "the amount")
        npcplay.wait_control(ctl, "ITEMPURCHASEWND", "item:%d" % RED_POTION)
        s.equal(npcplay.item_count(ctl, "ITEMPURCHASEWND", RED_POTION), BUY, "Red Potions in the purchase window")
        s.state("purchase window", ctl.window("ITEMPURCHASEWND")["controls"])
        s.frame("buy-list")
        ctl.click_control("ITEMPURCHASEWND", "ok")
        ctl.wait_window("ITEMPURCHASEWND", open=False, timeout=15)
        rows = s.rows("picklog S Red Potion", wait_rows(lambda: t.logs.bought(char_id, RED_POTION, after=mark)))
        s.equal(rows[-1]["amount"], BUY, "picklog amount")
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.equal(z[-1]["amount"], -BUY * POTION_PRICE, "zeny paid")
        ctl.wait({"frames": 10}, timeout=10)
        s.equal(ctl.state("self")["self"]["zeny"], zeny0 - BUY * POTION_PRICE, "zeny the client shows")
        npcplay.check_sent(t, s, {"0x0090": "CZ_CONTACTNPC", "0x00c5": "CZ_ACK_SELECT_DEALTYPE",
                                  "0x00c8": "CZ_PC_PURCHASE_ITEMLIST"})

    with t.step("sell 5 Jellopy", "M8.1e shop: sell; picklog S, zenylog S, amount typed") as s:
        mark = t.logs.mark()
        zeny0 = ctl.state("self")["self"]["zeny"]
        deal_type(ctl, "sell")
        ctl.wait_window("ITEMSELLWND", timeout=15)
        npcplay.wait_control(ctl, "ITEMSHOPWND", "item:%d" % JELLOPY)
        s.state("items to sell", [c["text"] for c in ctl.window("ITEMSHOPWND")["controls"]
                                  if c["id"].startswith("item:")])
        npcplay.drag_item(ctl, "ITEMSHOPWND", JELLOPY, "ITEMSELLWND")
        ctl.wait_window("ITEMDROPCNTWND", timeout=10)
        npcplay.type_into(ctl, "ITEMDROPCNTWND", SELL, s, "the amount")
        npcplay.wait_control(ctl, "ITEMSELLWND", "item:%d" % JELLOPY)
        s.equal(npcplay.item_count(ctl, "ITEMSELLWND", JELLOPY), SELL, "Jellopy in the sell window")
        s.frame("sell-list")
        ctl.click_control("ITEMSELLWND", "ok")
        ctl.wait_window("ITEMSELLWND", open=False, timeout=15)
        rows = s.rows("picklog S Jellopy", wait_rows(lambda: t.logs.sold(char_id, JELLOPY, after=mark)))
        s.equal(rows[-1]["amount"], -SELL, "picklog amount")
        z = s.rows("zenylog S", wait_rows(lambda: t.logs.zeny(char_id, "S", after=mark)))
        s.equal(z[-1]["amount"], SELL * JELLOPY_SELL, "zeny received")
        ctl.wait({"frames": 10}, timeout=10)
        s.equal(ctl.state("self")["self"]["zeny"], zeny0 + SELL * JELLOPY_SELL, "zeny the client shows")
        npcplay.check_sent(t, s, {"0x00c9": "CZ_PC_SELL_ITEMLIST"})

    t.stop_client()
    with t.step("saved on logout", "inventory and zeny after logout") as s:
        zeny = ZENY - BUY * POTION_PRICE + SELL * JELLOPY_SELL
        s.rows("char %s zeny %d" % (name, zeny),
               wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"] and r["zeny"] == zeny]))
        s.rows("inventory: %d Red Potions" % BUY,
               [r for r in t.logs.inventory(char_id, RED_POTION) if r["amount"] == BUY])
        s.rows("inventory: %d Jellopy" % (JELLOPIES - SELL),
               [r for r in t.logs.inventory(char_id, JELLOPY) if r["amount"] == JELLOPIES - SELL])
