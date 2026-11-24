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
