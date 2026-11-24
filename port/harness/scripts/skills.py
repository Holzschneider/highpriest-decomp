"""M8.1f: skills and the skill tree.

Done when (implementation plan, M8.1): a first-class character raises a
skill with the skill window's level-up button, puts a self skill on the
shortcut bar and uses it by its key, and uses a targeted skill on a monster
through the target cursor; the server confirms each (the `skill` table and
char.skill_point, the SP spent in the char row, the monster's death, the
skill packets in the packet log, the `hotkey` table) and the effects are
drawn (frames).

Set-up (SQL, offline): a fresh account whose character bifrost-bot creates,
then made a Swordman (base level 30, job level 20) with 3 skill points,
Basic Skill 9, Provoke 5 and Endure 1 (Endure needs Provoke 5) and no Bash,
standing at prt_fild08 230,240. A GM bot (group 99) stands beside it and
spawns a Poring (@monster, set-up).

Played, through the control socket only:
- Alt+S opens the skill window; Bash's row shows level 0 and its level-up
  button; a click on it sends CZ_UPGRADE_SKILLLEVEL (0x0112), the row reads
  level 1 and the free points drop by one.
- F12 shows the shortcut bar if it is hidden; Endure is dragged from its row
  to slot 0 (CZ_SHORTCUT_KEY_CHANGE 0x02ba); F1 uses it: ZC_USE_SKILL
  (0x011a) for skill 8 on the own character, the SP drop in `self`.
- Bash's row is selected and double-clicked (the skill window's use), the
  client shows the target cursor, a click on the Poring sends CZ_USE_SKILL
  (0x0113) and the server answers with ZC_NOTIFY_SKILL (0x01de) for skill 5;
  repeated until the Poring is gone.
After quitting: `skill` has Bash 1, char.skill_point is 2, char.sp is below
what the client showed before the first skill, and `hotkey` has slot 0 =
skill 8 level 1.
"""

import time

from bfharness.accounts import unique_name
from bfharness.bot import Bot
from bfharness.play import enter_game, login, own_home
from bfharness.sql import wait_rows
from bfharness import pktlog

NAME = "skills"
MILESTONE = "M8"
DESCRIPTION = "skill window: raise Bash, Endure on the shortcut bar by F1, Bash on a Poring by the target cursor"
MANIFEST = "port/harness/manifests/m8-skills.sha256"
NEEDS_SERVER = True

START = ("prt_fild08", 230, 240)
PORING_CELL = (234, 240)
SWORDMAN = 1
SKILLS = {1: 9, 6: 5, 8: 1}       # NV_BASIC 9, SM_PROVOKE 5, SM_ENDURE 1
BASH, ENDURE = 5, 8
POINTS = 3
PORING = 1002

CZ_UPGRADE_SKILLLEVEL = "0x0112"
CZ_USE_SKILL = "0x0072"          # the 2008 client's padded CZ_USE_SKILL: level at 6, SKID at 10, target at 21
ZC_USE_SKILL = "0x011a"
ZC_NOTIFY_SKILL = "0x01de"
CZ_SHORTCUT_KEY_CHANGE = "0x02ba"


def ok(s, cond, message):
    s.check(cond, message, passed=bool(cond))


def packets(t, dir, header, after=0):
    """Lines of the client's packet log (BF_PKT_LOG; the mock writes one too)."""
    out = []
    path = t.run_dir / "client-packets.txt"
    if not path.exists():
        return out
    for line in path.read_text(errors="replace").splitlines():
        f = line.split()
        if len(f) >= 6 and f[1] == dir and f[2].lower() == header and int(f[0]) >= after:
            out.append({"time": int(f[0]), "dir": f[1], "header": f[2].lower(), "hex": f[5]})
    return out


def u16(hexs, at):
    b = bytes.fromhex(hexs)
    return int.from_bytes(b[at:at + 2], "little")


def now_ms(t):
    """A time the packet log's first field can be compared with."""
    ps = pktlog.read(t.run_dir / "client-packets.txt")
    return ps[-1].time + 1 if ps else 0


def controls(ctl, window):
    w = ctl.window(window)
    return {c["id"]: c for c in (w or {}).get("controls", [])}


def centre(c):
    x, y, w, h = c["rect"]
    return x + w // 2, y + h // 2


def skill_row(ctl, skid, tries=12):
    """The skill window's row of skid, scrolling the list with the wheel
    (UISkillListWnd::OnWheel) until it is shown; None if it never is."""
    for i in range(tries):
        cs = controls(ctl, "SKILLLISTWND")
        row = cs.get("skill:%d" % skid)
        if row:
            return row
        ids = sorted(int(k[6:]) for k in cs if k.startswith("skill:"))
        w = ctl.window("SKILLLISTWND")
        if not w or not ids:
            return None
        x, y, ww, hh = w["rect"]
        ctl.wheel(-1 if skid > ids[-1] else 1, x + ww // 2, y + hh // 2)
        ctl.wait({"frames": 3}, timeout=10)
    return None


def move_window(ctl, name, grab, to):
    """Drag a window by the point `grab` (relative to its corner) so that its
    corner lands on `to`: out of the way of the character and the monster in
    the middle of the screen."""
    x, y, w, h = ctl.window(name)["rect"]
    gx, gy = x + grab[0], y + grab[1]
    ctl.move(gx, gy)
    ctl.wait({"frames": 2}, timeout=10)
    ctl.drag(gx, gy, to[0] + grab[0], to[1] + grab[1], button="left")
    ctl.wait({"frames": 3}, timeout=10)
    return ctl.window(name)["rect"]


def covered(ctl, x, y):
    """The windows over a logical point."""
    return [w["name"] for w in ctl.state("windows")["windows"]
            if w["rect"][0] <= x < w["rect"][0] + w["rect"][2] and w["rect"][1] <= y < w["rect"][1] + w["rect"][3]]


def setup(t):
    user = t.param("user") or unique_name(tag="sk")
    password = "bfh-test"
    name = t.param("char") or ("Bfhsk" + user[-8:])
    with t.step("set-up: Swordman with skill points", "SQL, offline: class, levels, points, skills, place") as s:
        # the mock has no bot: its account is a GM's, to spawn the Poring by @monster
        aid = t.accounts.create_account(user, password, group_id=99 if t.mock else 0)
        if not t.logs.char(name):
            if t.mock:
                t.mockdb.put("char", char_id=max([r["char_id"] for r in t.mockdb.rows("char")] or [150000]) + 1,
                             account_id=aid, char_num=0, name=name, **{"class": 0}, base_level=1, job_level=1,
                             zeny=0, last_map="prontera", last_x=155, last_y=183, online=0, hp=40, max_hp=40,
                             sp=11, max_sp=11, skill_point=0)
            else:
                bot = Bot.start(t, "maker", user, password, name)
                bot.quit()
                t.bots.remove(bot)
        c = t.accounts.set_char(name, map=START[0], x=START[1], y=START[2], job=SWORDMAN,
                                base_level=30, job_level=20)
        cid = c["char_id"]
        if t.mock:
            t.mockdb.update("char", cid, skill_point=POINTS, sp=60, max_sp=60, hp=400, max_hp=400)
            for k, v in SKILLS.items():
                t.mockdb.put("skill", char_id=cid, id=k, lv=v, flag=0)
        else:
            t.db.execute("UPDATE `char` SET `skill_point` = %d, `hp` = 9999, `sp` = 9999 "
                         "WHERE `char_id` = %d AND `online` = 0" % (POINTS, cid))
            t.db.execute("DELETE FROM `skill` WHERE `char_id` = %d" % cid)
            t.db.execute("DELETE FROM `hotkey` WHERE `char_id` = %d" % cid)
            t.db.execute("INSERT INTO `skill` (`char_id`, `id`, `lv`, `flag`) VALUES %s"
                         % ", ".join("(%d, %d, %d, 0)" % (cid, k, v) for k, v in SKILLS.items()))
        c = t.logs.char(name)
        s.state("char row", {k: c.get(k) for k in ("char_id", "class", "base_level", "job_level", "skill_point",
                                                    "last_map", "last_x", "last_y")})
        s.rows("skill rows", t.db.select("skill", {"char_id": cid}))
    return user, password, name, cid


def skill_window(t, ctl):
    with t.step("open the skill window", "Alt+S opens SKILLLISTWND with the character's skills") as s:
        if not ctl.window("SKILLLISTWND"):
            ctl.press("s", "alt")
            ctl.wait_window("SKILLLISTWND", timeout=10)
        ctl.wait({"frames": 5}, timeout=10)
        s.state("skill window moved to the top left", move_window(ctl, "SKILLLISTWND", (120, 6), (0, 0)))
        cs = controls(ctl, "SKILLLISTWND")
        s.state("SKILLLISTWND controls", {k: (c["kind"], c["text"], c["rect"]) for k, c in cs.items()})
        ok(s, skill_row(ctl, ENDURE), "Endure's row (scrolled to)")
        ok(s, skill_row(ctl, BASH), "Bash's row (scrolled to)")
        cs = controls(ctl, "SKILLLISTWND")
        s.equal(cs.get("points", {}).get("text"), str(POINTS), "free skill points")
        s.equal(cs.get("skill:%d" % BASH, {}).get("text"), "SM_BASH 0", "Bash before")
        s.frame("skill-window")
        return cs


def raise_bash(t, ctl, cid):
    with t.step("raise Bash", "the level-up button sends CZ_UPGRADE_SKILLLEVEL; the row reads 1, points 2") as s:
        skill_row(ctl, BASH)
        cs = controls(ctl, "SKILLLISTWND")
        up = cs.get("up:%d" % BASH)
        ok(s, up, "Bash's level-up button shown")
        t0 = now_ms(t)
        ctl.move(*centre(up))
        ctl.wait({"frames": 3}, timeout=10)
        ctl.click(*centre(up))
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            cs = controls(ctl, "SKILLLISTWND")
            if cs.get("skill:%d" % BASH, {}).get("text") == "SM_BASH 1":
                break
            time.sleep(0.3)
        s.state("SKILLLISTWND controls", {k: c["text"] for k, c in cs.items()})
        s.equal(cs.get("skill:%d" % BASH, {}).get("text"), "SM_BASH 1", "Bash after")
        s.equal(cs.get("points", {}).get("text"), str(POINTS - 1), "free skill points after")
        sent = packets(t, "S", CZ_UPGRADE_SKILLLEVEL, t0)
        s.state("CZ_UPGRADE_SKILLLEVEL sent", sent)
        ok(s, sent and u16(sent[0]["hex"], 2) == BASH, "CZ_UPGRADE_SKILLLEVEL for skill %d" % BASH)
        s.frame("bash-raised")


def hotkey(t, ctl):
    with t.step("Endure to the shortcut bar", "drag from the skill window to slot 0: CZ_SHORTCUT_KEY_CHANGE") as s:
        # F12 shows the bar and turns it to its next page of three
        # (ProcessPushButton, the reference's case 0x7b): to page 1, hotkeys 0-8
        for _ in range(4):
            if controls(ctl, "SHORTCUTWND").get("page", {}).get("text") == "1":
                break
            ctl.press("f12")
            ctl.wait({"frames": 3}, timeout=10)
        s.equal(controls(ctl, "SHORTCUTWND").get("page", {}).get("text"), "1", "shortcut bar shown at page 1")
        s.state("shortcut bar moved to the top", move_window(ctl, "SHORTCUTWND", (272, 16), (300, 0)))
        sc = controls(ctl, "SHORTCUTWND")
        s.state("SHORTCUTWND before", {k: c["text"] for k, c in sc.items()})
        src = skill_row(ctl, ENDURE)
        dst = sc["slot:0"]
        t0 = now_ms(t)
        x0, y0 = centre(src)
        x1, y1 = centre(dst)
        ctl.move(x0, y0)
        ctl.wait({"frames": 3}, timeout=10)
        ctl.pointer("down", x0, y0)
        ctl.wait({"frames": 3}, timeout=10)
        for i in range(1, 9):
            ctl.move(x0 + (x1 - x0) * i // 8, y0 + (y1 - y0) * i // 8)
            ctl.wait({"frames": 2}, timeout=10)
        ctl.pointer("up", x1, y1)
        ctl.wait({"frames": 5}, timeout=10)
        sc = controls(ctl, "SHORTCUTWND")
        s.state("SHORTCUTWND after", {k: c["text"] for k, c in sc.items()})
        # The bar keeps the skill's use level, which the 2008 client sets for
        # the skills of leveluseskillspamount.txt only; Endure is not one, so
        # its level is 0 (the skill's own level is used, MM_HANG_USE_SKILL).
        ok(s, sc["slot:0"]["text"].startswith("skill:%d:" % ENDURE), "slot 0: %r" % sc["slot:0"]["text"])
        sent = wait_rows(lambda: packets(t, "S", CZ_SHORTCUT_KEY_CHANGE, t0), timeout=5)
        s.state("CZ_SHORTCUT_KEY_CHANGE sent", sent)
        ok(s, [p for p in sent if u16(p["hex"], 2) == 0], "CZ_SHORTCUT_KEY_CHANGE for slot 0")
        s.frame("shortcut-bar")


def self_skill(t, ctl):
    with t.step("Endure by F1", "F1 uses slot 0: ZC_USE_SKILL for Endure, SP spent") as s:
        me = ctl.state("self")["self"]
        sp0 = me["sp"]
        t0 = now_ms(t)
        ctl.press("f1")
        got = wait_rows(lambda: packets(t, "R", ZC_USE_SKILL, t0), timeout=10)
        s.state("ZC_USE_SKILL received", got)
        ok(s, [p for p in got if u16(p["hex"], 2) == ENDURE], "ZC_USE_SKILL for skill %d" % ENDURE)
        sent = packets(t, "S", CZ_USE_SKILL, t0)
        s.state("CZ_USE_SKILL sent", sent)
        ok(s, [p for p in sent if u16(p["hex"], 10) == ENDURE], "CZ_USE_SKILL for skill %d" % ENDURE)
        ctl.wait({"frames": 10}, timeout=10)
        s.frame("endure")
        me = ctl.state("self")["self"]
        s.state("self", {k: me[k] for k in ("hp", "max_hp", "sp", "max_sp")})
        ok(s, me["sp"] < sp0, "SP %d -> %d" % (sp0, me["sp"]))
        return sp0


def poring(t, ctl, bot):
    with t.step("set-up: a Poring", "GM bot beside the character: @warp, @monster") as s:
        if bot is None:
            mark = t.logs.mark()
            t.gm.run("@monster %d 1" % PORING)
            ctl.wait({"frames": 5}, timeout=10)
            s.rows("atcommandlog (mock: the character's own @monster)", t.logs.atcommands(after=mark))
        else:
            mark = t.logs.mark()
            bot.warp(START[0], *PORING_CELL)
            bot.gm("@monster %d 1" % PORING)
            time.sleep(1.0)
            s.rows("atcommandlog", t.logs.atcommands(account_id=bot.entered.get("aid"), after=mark))
        me = ctl.state("self")["self"]
        deadline = time.monotonic() + 20
        mons = []
        while time.monotonic() < deadline:
            mons = [a for a in ctl.state("actors")["actors"] if a["type"] == "monster" and a["job"] == PORING
                    and a["screen"] and abs(a["x"] - me["x"]) + abs(a["y"] - me["y"]) <= 10]
            if mons:
                break
            time.sleep(0.5)
        s.state("Porings near", mons)
        ok(s, mons, "a Poring within 10 cells")
        return mons[0]


def target_skill(t, ctl, target):
    with t.step("Bash on the Poring", "skill window use, target cursor, click: CZ_USE_SKILL, ZC_NOTIFY_SKILL") as s:
        t0 = now_ms(t)
        uses = 0
        gone = False
        bashed = False
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            a = [x for x in ctl.state("actors")["actors"] if x["id"] == target["id"]]
            if not a:
                gone = True
                break
            row = skill_row(ctl, BASH) if ctl.window("SKILLLISTWND") else None
            if not row:
                ctl.press("s", "alt")
                ctl.wait_window("SKILLLISTWND", timeout=10)
                continue
            # select the row, then the window's use button (UISkillListWnd
            # ID_OK: UM_USESKILL, the target cursor for a skill on a character)
            ctl.click(*centre(row))
            ctl.wait({"frames": 3}, timeout=10)
            ctl.click_control("SKILLLISTWND", "ok")
            ctl.wait({"frames": 5}, timeout=10)
            a = [x for x in ctl.state("actors")["actors"] if x["id"] == target["id"]]
            if not a or not a[0]["screen"]:
                continue
            over = covered(ctl, a[0]["screen"]["x"], a[0]["screen"]["y"])
            if over:
                s.note("the Poring at %r is under %s" % (a[0]["screen"], over))
                time.sleep(1.0)
                continue
            ctl.move(a[0]["screen"]["x"], a[0]["screen"]["y"])
            ctl.wait({"frames": 3}, timeout=10)
            if uses == 0:
                s.frame("target-cursor")
            t1 = now_ms(t)
            ctl.click(a[0]["screen"]["x"], a[0]["screen"]["y"])
            uses += 1
            # the client walks into Bash's range first; the frame is taken
            # a few frames after the server's ZC_NOTIFY_SKILL (hit, damage)
            hit = wait_rows(lambda: [p for p in packets(t, "R", ZC_NOTIFY_SKILL, t1) if u16(p["hex"], 2) == BASH],
                            timeout=8)
            if hit and not bashed:
                ctl.wait({"frames": 8}, timeout=10)
                s.frame("bash")
                bashed = True
            time.sleep(1.0 if not t.mock else 0.2)
        sent = packets(t, "S", CZ_USE_SKILL, t0)
        got = packets(t, "R", ZC_NOTIFY_SKILL, t0)
        s.state("CZ_USE_SKILL sent", sent)
        s.state("ZC_NOTIFY_SKILL received", got[:10])
        s.note("%d uses, %d CZ_USE_SKILL sent, %d ZC_NOTIFY_SKILL received; the Poring %s"
               % (uses, len(sent), len(got), "is gone" if gone else "was still there after 90 s"))
        ok(s, [p for p in sent if u16(p["hex"], 10) == BASH], "CZ_USE_SKILL for skill %d" % BASH)
        ok(s, [p for p in got if u16(p["hex"], 2) == BASH], "ZC_NOTIFY_SKILL for skill %d" % BASH)
        ok(s, gone, "the Poring died")


def run(t):
    user, password, name, cid = setup(t)
    with t.step("set-up: GM bot", "bifrost-bot on a group-99 account") as s:
        bot = Bot.start(t, "skillsgm", "bfhskillgm", "bfbot", "BfhSkillGM", gm=True)
        s.note("mock: no bot" if bot is None else "bot entered: %r" % bot.entered)
    env = own_home(t)
    env["BF_MULTI_INSTANCE"] = "1"     # other clients may be running on this machine
    ctl = t.launch(env=env)
    login(t, ctl, user, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 30}, timeout=20)
    skill_window(t, ctl)
    raise_bash(t, ctl, cid)
    hotkey(t, ctl)
    sp0 = self_skill(t, ctl)
    target = poring(t, ctl, bot)
    target_skill(t, ctl, target)
    if bot is not None:
        bot.gm("@killmonster2")
    t.stop_client()
    with t.step("saved by the server", "after quitting: skill, char.skill_point, char.sp, hotkey") as s:
        deadline = time.monotonic() + 15
        c = t.logs.char(name)
        while c.get("online") and time.monotonic() < deadline:
            time.sleep(0.5)
            c = t.logs.char(name)
        s.state("char row", {k: c.get(k) for k in ("online", "class", "skill_point", "sp", "max_sp", "hp")})
        skills = {r["id"]: r["lv"] for r in t.db.select("skill", {"char_id": cid})}
        s.state("skill rows", skills)
        s.equal(skills.get(BASH), 1, "Bash in the skill table")
        s.equal(c.get("skill_point"), POINTS - 1, "char.skill_point")
        ok(s, c.get("sp") is not None and c["sp"] < sp0, "char.sp %s below %d (before the first skill)"
           % (c.get("sp"), sp0))
        keys = t.db.select("hotkey", {"char_id": cid})
        s.rows("hotkey rows", keys)
        k0 = [k for k in keys if k["hotkey"] == 0][-1:]
        ok(s, k0 and int(k0[0]["type"]) == 1 and k0[0]["itemskill_id"] == ENDURE,
           "hotkey 0 = skill %d" % ENDURE)
