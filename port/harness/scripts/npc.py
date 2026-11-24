"""M8.1e NPC dialogue: Next, Close, a menu choice and a number input, played
through the client's NPC windows against stock rAthena NPCs in Prontera.

Done when: talking to an NPC opens its dialogue (ZC_SAY_DIALOG into
SAYDIALOGWND), Next asks for the next page (CZ_REQ_NEXT_SCRIPT), a menu
(ZC_MENU_LIST into CHOOSEWND) is answered by clicking the entry and OK
(CZ_CHOOSE_MENU), a number typed into the input window (ZC_OPENEDITDLG into
NPCEDITDIALOGWND) reaches the script (CZ_INPUT_EDITDLG), Close ends the
conversation (CZ_CLOSE_DIALOG), and what the script did with the answers is
in the server's records.

Scenes (NPC definitions read from the server's npc/ at run time):
- Job Master (npc/custom/jobmaster.txt, prontera 153,193): menu " ~ Swordsman" (rAthena's jobname),
  Next, menu " ~ Change into ^0055FFSwordsman^000000 class", Next, Close. Evidence: the
  client's `self.job` becomes 1; `char.class` = 1 after logout.
- Merchant#pron (npc/merchants/novice_exchange.txt, prontera 123,102):
  Next x2, "Sure!", Next, "Jellopies", Next x3, "Deal.", Next x2, the number
  2, Close. Evidence: picklog type N: Jellopy -20 and Red Potion +2.
npclog stays empty: log_npc logs only the scripts' `logmes`, which neither
NPC calls.

Set-up (SQL, offline): a fresh group-99 account, a Novice of job level 10
(the Job Master's requirement) at prontera 153,189 with 30 Jellopy; the
move to the Merchant is `@warp prontera 123 98`, typed by the client
(atcommandlog). The conversations are played by clicks and typed text only.
"""

from bfharness import npcplay
from bfharness.play import enter_game, login, own_home
from bfharness.sql import wait_rows

NAME = "npc"
MILESTONE = "M8"
DESCRIPTION = "NPC dialogue: next, close, menu, number input (Job Master, Merchant#pron)"
MANIFEST = "port/harness/manifests/m8-npc.sha256"
NEEDS_SERVER = True

JELLOPY, RED_POTION, SWORDMAN = 909, 501, 1
JOBMASTER = ("prontera", 153, 193)
MERCHANT = ("prontera", 123, 102)
START = ("prontera", 153, 189)
MERCHANT_START = ("prontera", 123, 98)

DIALOGUE_PACKETS = {"0x0090": "CZ_CONTACTNPC", "0x00b9": "CZ_REQ_NEXT_SCRIPT", "0x00b8": "CZ_CHOOSE_MENU",
                    "0x0143": "CZ_INPUT_EDITDLG", "0x0146": "CZ_CLOSE_DIALOG"}


def at(x, y):
    return lambda a: a["type"] == "npc" and a["x"] == x and a["y"] == y


def run(t):
    with t.step("set-up: account and character", "SQL, offline: Novice job level 10, 30 Jellopy") as s:
        user, password, c = npcplay.new_player(t, "n", *START, job_level=10, items=[(JELLOPY, 30)])
        char_id, name = c["char_id"], c["name"]
        s.state("char row", {k: c[k] for k in ("char_id", "name", "class", "job_level", "last_map",
                                               "last_x", "last_y")})
        s.state("Job Master on the server", npcplay.server_npc(
            t, *JOBMASTER, "script", r"\sJob Master\s", "prontera,153,193,6\tscript\tJob Master\t123,{"))
        s.state("Merchant on the server", npcplay.server_npc(
            t, *MERCHANT, "script", r"\sMerchant#pron\s", "prontera,123,102,5\tscript\tMerchant#pron\t85,{"))

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 60}, timeout=20)

    with t.step("Job Master: menu, next, close", "M8.1e: a menu choice changes the job") as s:
        mark = t.logs.mark()
        npcplay.click_npc(ctl, at(*JOBMASTER[1:]), "Job Master")
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("jobmaster-dialogue")
        log = npcplay.run_dialogue(ctl, s, answers=["Swordsman", "Change into"],
                                   frames={"menu": "jobmaster-menu"})
        s.check([k for k, _ in log].count("menu") == 2, "two menus answered")
        me = None
        for _ in range(30):
            me = ctl.state("self")["self"]
            if me["job"] == SWORDMAN:
                break
            ctl.wait({"frames": 10}, timeout=10)
        s.equal(me["job"], SWORDMAN, "job the client shows after the change")
        ctl.wait({"frames": 30}, timeout=10)
        s.frame("swordman")
        s.rows("npclog (log_npc: only `logmes`; may be empty)", t.logs.npc(char_id=char_id, after=mark), min_rows=0)

    with t.step("set-up: to the Merchant", "atcommand typed by the client: @warp") as s:
        mark = t.logs.mark()
        npcplay.focus_chat(ctl)
        t.gm.run(t.gm.warp(*MERCHANT_START))
        s.rows("atcommandlog @warp", wait_rows(lambda: t.logs.atcommands(contains="@warp", after=mark)))
        ctl.wait({"position": {"x": MERCHANT_START[1], "y": MERCHANT_START[2]}}, timeout=30)
        ctl.wait({"frames": 60}, timeout=20)

    with t.step("Merchant: next, menus, number input, close",
                "M8.1e: the number typed reaches the script; picklog N") as s:
        mark = t.logs.mark()
        npcplay.click_npc(ctl, at(*MERCHANT[1:]), "Merchant")
        ctl.wait_window("SAYDIALOGWND", timeout=15)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("merchant-dialogue")
        log = npcplay.run_dialogue(ctl, s, answers=["Sure!", "Jellopies", "Deal."], numbers=[2],
                                   frames={"number": "merchant-input"})
        kinds = [k for k, _ in log]
        s.check(kinds.count("next") >= 8, "Next pressed through the pages", nexts=kinds.count("next"))
        s.check("number input" in kinds and kinds[-1] == "close", "number input, then Close")
        rows = wait_rows(lambda: t.logs.items(char_id=char_id, type="N", after=mark)
                         if len(t.logs.items(char_id=char_id, type="N", after=mark)) >= 2 else [])
        s.rows("picklog N for %s" % name, rows, min_rows=2)
        got = {(r["nameid"], r["amount"]) for r in rows}
        s.check((JELLOPY, -20) in got, "20 Jellopy taken (2 potions x 10)", rows=sorted(got))
        s.check((RED_POTION, 2) in got, "2 Red Potions given (the number typed)", rows=sorted(got))
        npcplay.check_sent(t, s, DIALOGUE_PACKETS)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("merchant-done")

    t.stop_client()
    with t.step("job saved", "char.class after logout") as s:
        rows = wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]
                                  and r["class"] == SWORDMAN])
        s.rows("char %s class %d" % (name, SWORDMAN), rows)
