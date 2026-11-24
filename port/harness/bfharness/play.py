"""Steps that several scripts share: logging in, entering the game, walking to a
clicked cell, saying a line and finding it in chatlog.

These drive the client only through the control socket, with the keyboard
where the 2008 client accepts it (the account field has the focus at start,
Tab moves to the password, Enter submits, Enter on the character selection
starts the selected slot), so they do not depend on the control module
exposing every window's controls. Where a script must click a control, it
names it by the ids in PROTOCOL.md's `controls`.
"""

import time

from . import pktlog
from .sql import wait_rows


def login(t, ctl, user, password, timeout=60):
    """From the login screen to character selection. Returns the loginlog rows."""
    mark = t.logs.mark()
    with t.step("type account and password", "M5: the edit control takes account and password") as s:
        st = ctl.wait_mode("login", timeout)
        s.state("mode", {"mode": st["mode"], "frame": st["frame"]})
        ctl.type_text(user)
        ctl.press("tab")
        ctl.type_text(password)
        w = ctl.window("LOGINWND")
        s.check(w is not None, "LOGINWND is open")
        texts = {c["id"]: c["text"] for c in w.get("controls", [])}
        s.state("LOGINWND controls", texts)
        if texts:
            s.equal(texts.get("id_edit"), user, "account field")
            s.equal(texts.get("pw_edit"), "*" * len(password), "password field (masked)")
        s.frame("login-typed")
    with t.step("connect", "M5: reaches character selection") as s:
        after = ctl.last_seq
        ctl.press("enter")
        st = through_server_list(ctl, after, timeout)
        s.state("mode", st)
        rows = wait_rows(lambda: t.logs.logins(user, after=mark), timeout=10)
        s.rows("loginlog 'login ok' for %s after %s" % (user, mark.time), rows)
    return rows


def through_server_list(ctl, after, timeout=60):
    """After the login request: the 2008 client shows the character servers
    even when there is one; Enter takes the one selected. A client (or the
    mock) that goes straight to character selection is fine too."""
    ev = ctl.wait_event("mode", lambda e: e.get("mode") in ("servers", "charselect", "login"),
                        after=after, timeout=timeout)
    if ev["mode"] == "servers":
        ctl.wait_window("SELECTSERVERWND", timeout=10)
        ctl.press("enter")
    elif ev["mode"] == "login":
        raise AssertionError("back at the login screen (refused?): %r" % ev)
    return ctl.wait_mode("charselect", timeout)


def enter_game(t, ctl, slot=0, timeout=120):
    """From character selection into the world. Returns state."""
    with t.step("enter the world", "M6: map loaded, own character placed") as s:
        mark = t.logs.mark()
        for _ in range(slot):
            ctl.press("right")
        ctl.press("enter")
        st = ctl.wait_mode("game", timeout)
        s.state("mode", st)
        ev = ctl.wait_event("map", timeout=timeout)
        s.state("map event", ev)
        rows = wait_rows(lambda: t.logs.charlog(msg="char select", after=mark), timeout=10)
        s.rows("charlog 'char select'", rows)
        st = ctl.state("map", "self")
        s.state("self", st["self"])
        s.check(st["self"] is not None and st["map"] is not None, "self and map reported")
        return st


def walk_click(t, ctl, dx, dy, label, timeout=60):
    """Point at the logical offset (dx, dy) from the window centre, read the cell
    picking finds there, click, wait for arrival, and check that the reported
    position is that cell. Returns the cell."""
    with t.step("walk: %s" % label, "M6: click-to-move lands on the cell under the pointer") as s:
        v = ctl.state("view")["view"]
        x, y = v["logical"]["w"] // 2 + dx, v["logical"]["h"] // 2 + dy
        ctl.move(x, y)
        p = ctl.state("pointer")["pointer"]
        s.state("pointer", p)
        s.check(p["cell"] is not None, "a cell under the pointer at %d,%d" % (x, y))
        cell = p["cell"]
        ctl.click(x, y)
        try:
            ctl.wait({"position": {"x": cell["x"], "y": cell["y"]}, "moving": False}, timeout)
        except Exception as e:
            s.note("no arrival at %r: %s" % (cell, e))
        me = ctl.state("self")["self"]
        s.state("self after arrival", me)
        s.equal((me["x"], me["y"]), (cell["x"], cell["y"]), "position after arrival")
        return cell


def say_and_confirm(t, ctl, s, text, char_id, expect_pos=None, map=None):
    """Say a line in public chat; find it in chatlog. chatlog stores the
    server's own position of the speaker, so expect_pos checks the server's
    view of where the character stands."""
    mark = t.logs.mark()
    after = ctl.last_seq
    ctl.say(text)
    ev = ctl.wait_event("chat", lambda e: text in (e.get("text") or ""), after=after, timeout=10)
    s.state("chat event", ev)
    rows = s.rows("chatlog line %r" % text,
                  wait_rows(lambda: t.logs.chat(char_id=char_id, contains=text, after=mark), timeout=10))
    row = rows[-1]
    if map is not None:
        s.equal(row["src_map"], map, "server's map in chatlog")
    if expect_pos is not None:
        s.equal((row["src_map_x"], row["src_map_y"]), tuple(expect_pos), "server's position in chatlog")
    return row


def token():
    return "%x" % int(time.time() * 1000 % 0xffffffff)


# ---- zone entry (M5): CZ_ENTER, the packet comparison, the exit after it

CZ_ENTER = "0x009b"


def wait_sent(t, header, conn, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        sent = pktlog.has_sent(pktlog.read(t.client.pkt_log), header, conn)
        if sent:
            return sent
        time.sleep(0.2)
    return []


def compare_with_netprobe(t, s, client_log, user, password, charname):
    probe = pktlog.netprobe_next_to_client()
    s.check(probe is not None, "bifrost-netprobe next to BF_CLIENT")
    log = t.run_dir / "netprobe-packets.txt"
    code, last = pktlog.run_netprobe(probe, log, ["--user", user, "--pass", password,
                                                  "--char-name", charname, "--listen-ms", "1000"])
    s.note("bifrost-netprobe exit %d: %s" % (code, last))
    equal, lines = pktlog.compare(pktlog.read(client_log), pktlog.read(log))
    s.state("login sequence: client | netprobe", lines)
    s.check(equal, "the client's login sequence equals bifrost-netprobe's")


def stop_after_zone_entry(t):
    """The map load after CZ_ENTER is M6.1's (it fails on prontera.rsw at the
    time of writing): its exit is recorded, not judged, unless the script is
    run with --param strict_exit=1, which keeps the runner's check."""
    if t.param("strict_exit") == "1":
        t.stop_client()
        return
    c = t.client
    code = c.stop(grace=5)
    with t.step("client stopped after zone entry", "recorded only: the map load is M6.1") as s:
        s.note("exit status %s%s" % (code, " (killed)" if c.killed else ""))
        s.trace("assertions and crash lines in trace", c.trace_lines("assert", "crash:")[:6])
    t.client = None


def own_home(t):
    """Environment for t.launch: the client keeps its settings (window
    positions, camera, the registry) under the run directory (BF_HOME), so a
    script starts from the defaults and leaves the user's profile alone. The
    game folder stays BF_CLIENT_DIR, which the harness always passes."""
    if t.mock:
        return {}
    home = t.run_dir / "home"
    home.mkdir(exist_ok=True)
    return {"BF_HOME": str(home)}


def fresh_character(t, user, password, name, sex="M", place=None, **columns):
    """Set-up for the M8 scripts, which must not share a character: a group-99
    account `user` (atcommands for set-up) with a Novice `name`, created by
    bifrost-bot (which logs in once and out again), then placed and changed
    by SQL while offline (`place` = (map, x, y); `columns` as set_char
    takes them). With --mock the character row is written to the mock's
    database. Returns the char row."""
    from .bot import Bot
    with t.step("set-up: account and character",
                "group 99; the character created by bifrost-bot, set by SQL while offline") as s:
        aid = t.accounts.create_account(user, password, sex=sex, group_id=99)
        s.note("account %s (%d), sex %s" % (user, aid, sex))
        if not t.mock:
            bot = Bot.start(t, "create", user, password, name, gm=True)
            s.state("created by the bot", bot.entered)
            bot.quit()
            wait_rows(lambda: [r for r in [t.logs.char(name)] if r and not r["online"]], timeout=20)
        elif not t.logs.char(name):
            rows = t.mockdb.rows("char")
            t.mockdb.put("char", char_id=max([r["char_id"] for r in rows] or [150000]) + 1,
                         account_id=aid, char_num=0, name=name, **{"class": 0}, base_level=1, job_level=1,
                         base_exp=0, job_exp=0, zeny=0, last_map="prontera", last_x=155, last_y=183,
                         str=5, agi=5, vit=5, int=5, dex=5, luk=5, status_point=0,
                         hp=40, max_hp=40, sp=11, max_sp=11, online=0)
        if place:
            columns.update(map=place[0], x=place[1], y=place[2])
        c = t.accounts.set_char(name, **columns)
        s.state("char row", {k: c.get(k) for k in ("char_id", "last_map", "last_x", "last_y", "hp", "max_hp",
                                                    "str", "status_point", "base_exp", "job_exp")})
        return c
