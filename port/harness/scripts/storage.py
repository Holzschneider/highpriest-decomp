"""M8.1e storage: Kafra storage in Prontera, an item put in and taken out
through the client's storage and inventory windows.

Done when: the Kafra's dialogue (Next, the menu entry "Use Storage", Close)
opens the storage (ZC_STORE_NORMAL_ITEMLIST3 and
ZC_NOTIFY_STOREITEM_COUNTINFO into ITEMSTOREWND) and takes the 40 zeny fee
(zenylog N -40); 4 Jellopy dragged from the inventory (Alt+E, etc tab) into
the storage window (the item window names it `item:<inventory index>`, the
storage `item:<item id>`; PROTOCOL.md), the amount typed into the count
window, go in (the
2008 client's body-to-storage packet; picklog R -4); 1 dragged back from
the storage's etc tab into the inventory comes out (picklog R +1); Close
ends the session (ZC_CLOSE_STORE) and the account's `storage` row holds 3
Jellopy; after logout the inventory holds 7.

The Kafra: kaf_prontera5, prontera 146,89 (npc/kafras/kafras.txt, read at
run time); the Mailbox stands on 146,86 in front of her, so the character
starts at 149,86. Storage needs the Novice's Basic Skill level 6 under the
server's `basic_skill_check: yes` (conf/battle/player.conf): set-up gives
level 9 in the `skill` table. Set-up (SQL, offline): a fresh account, 1000
zeny, 10 Jellopy, an empty storage. Played: clicks, Alt+E, drags and typed
amounts only.
"""

from bfharness import npcplay
from bfharness.play import enter_game, login, own_home
from bfharness.sql import wait_rows

NAME = "storage"
MILESTONE = "M8"
DESCRIPTION = "Kafra storage in Prontera: put 4 Jellopy in, take 1 out; picklog R, storage table"
MANIFEST = "port/harness/manifests/m8-storage.sha256"
NEEDS_SERVER = True

JELLOPY, NV_BASIC = 909, 1
KAFRA = ("prontera", 146, 89)
START = ("prontera", 149, 86)
ZENY, JELLOPIES, PUT, TAKE, FEE = 1000, 10, 4, 1, 40
ETC_TAB = "etc"
# Sent by the 2008 client for storage (PACKETVER 20080401's zone packet ids)
STORE_PACKETS = {"0x0094": "CZ_MOVE_ITEM_FROM_BODY_TO_STORE", "0x00f7": "CZ_MOVE_ITEM_FROM_STORE_TO_BODY",
                 "0x0193": "CZ_CLOSE_STORE"}


def kafra(a):
    return a["type"] == "npc" and (a["x"], a["y"]) == KAFRA[1:]


def run(t):
    with t.step("set-up: account and character", "SQL, offline: Basic Skill 9, 1000 zeny, 10 Jellopy") as s:
        user, password, c = npcplay.new_player(t, "k", *START, zeny=ZENY, items=[(JELLOPY, JELLOPIES)],
                                               skills=[(NV_BASIC, 9)])
        char_id, name, aid = c["char_id"], c["name"], c["account_id"]
        s.state("char row", {k: c[k] for k in ("char_id", "account_id", "name", "zeny", "last_map",
                                               "last_x", "last_y")})
        s.state("Kafra on the server", npcplay.server_npc(
            t, *KAFRA, "script", r"Kafra Employee::kaf_prontera5",
            "prontera,146,89,6\tscript\tKafra Employee::kaf_prontera5\t117,{"))
        s.equal(npcplay.storage_rows(t, aid), [], "storage of the new account")

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    with t.step("Kafra: Use Storage", "M8.1e storage: the Kafra's dialogue opens the storage; zenylog N") as s:
        mark = t.logs.mark()
        npcplay.click_npc(ctl, kafra, "Kafra Employee", dy=-50)
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("kafra")
        npcplay.run_dialogue(ctl, s, answers=["Use Storage"])
        ctl.wait_window("ITEMSTOREWND", timeout=15)
        z = s.rows("zenylog N (fee)", wait_rows(lambda: t.logs.zeny(char_id, "N", after=mark)))
        s.equal(z[-1]["amount"], -FEE, "storage fee")

    with t.step("put 4 Jellopy in", "M8.1e storage: inventory to storage; picklog R -4") as s:
        mark = t.logs.mark()
        ctl.press("e", "alt")
        ctl.wait_window("ITEMWND", timeout=10)
        npcplay.click_tab(ctl, "ITEMWND", ETC_TAB)
        npcplay.wait_item(ctl, "ITEMWND", JELLOPY)
        npcplay.drag_item(ctl, "ITEMWND", JELLOPY, "ITEMSTOREWND")
        ctl.wait_window("ITEMDROPCNTWND", timeout=10)
        s.frame("put-amount")
        npcplay.type_into(ctl, "ITEMDROPCNTWND", PUT, s, "the amount")
        rows = s.rows("picklog R Jellopy -", wait_rows(lambda: t.logs.items(char_id, JELLOPY, "R", after=mark)))
        s.equal(rows[-1]["amount"], -PUT, "picklog amount")
        ctl.wait({"frames": 10}, timeout=10)
        s.equal(npcplay.item_count(ctl, "ITEMWND", JELLOPY), JELLOPIES - PUT, "Jellopy in the inventory window")
        npcplay.click_tab(ctl, "ITEMSTOREWND", ETC_TAB)
        npcplay.wait_item(ctl, "ITEMSTOREWND", JELLOPY)
        s.equal(npcplay.item_count(ctl, "ITEMSTOREWND", JELLOPY), PUT, "Jellopy in the storage window")
        s.frame("stored")

    with t.step("take 1 Jellopy out", "M8.1e storage: storage to inventory; picklog R +1") as s:
        mark = t.logs.mark()
        npcplay.drag_item(ctl, "ITEMSTOREWND", JELLOPY, "ITEMWND")
        ctl.wait_window("ITEMDROPCNTWND", timeout=10)
        npcplay.type_into(ctl, "ITEMDROPCNTWND", TAKE, s, "the amount")
        rows = s.rows("picklog R Jellopy +", wait_rows(lambda: t.logs.items(char_id, JELLOPY, "R", after=mark)))
        s.equal(rows[-1]["amount"], TAKE, "picklog amount")
        ctl.wait({"frames": 10}, timeout=10)
        s.equal(npcplay.item_count(ctl, "ITEMSTOREWND", JELLOPY), PUT - TAKE, "Jellopy in the storage window")
        s.equal(npcplay.item_count(ctl, "ITEMWND", JELLOPY), JELLOPIES - PUT + TAKE,
                "Jellopy in the inventory window")

    with t.step("close the storage", "M8.1e storage: the storage table holds what is left") as s:
        ctl.click_control("ITEMSTOREWND", "close")
        ctl.wait_window("ITEMSTOREWND", open=False, timeout=15)
        rows = wait_rows(lambda: [r for r in npcplay.storage_rows(t, aid, JELLOPY) if r["amount"] == PUT - TAKE])
        s.rows("storage of account %d: %d Jellopy" % (aid, PUT - TAKE), rows)
        npcplay.check_sent(t, s, STORE_PACKETS)

    t.stop_client()
    with t.step("saved on logout", "inventory after logout") as s:
        wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]])
        s.rows("inventory: %d Jellopy" % (JELLOPIES - PUT + TAKE),
               [r for r in t.logs.inventory(char_id, JELLOPY) if r["amount"] == JELLOPIES - PUT + TAKE])
