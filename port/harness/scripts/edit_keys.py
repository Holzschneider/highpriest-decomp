"""Cursor and editing keys in the client's edit fields (M5.3, M6 chat line).

The 2008 client passes WM_KEYDOWN to the focused edit field and then to the
language object (CLanguage::OnKeyDown: Left, Right, Home, End, Delete,
Shift+ and Ctrl+ variants), WndProc at 0x696D1A in the reference executable.

Played, in the login window's account field: "abcdef", Left twice, "X"
gives "abcdXef"; Home, Delete gives "bcdXef"; End, Backspace gives "bcdXe";
Right at the end and Left at the start move nothing; Cmd+A selects the
whole text (F10), so "Z" typed after it leaves "Z". Then, in game as
bftest/BFProbe: "hello world" typed into the chat line, Home, "X", Enter;
chatlog has "Xhello world"; then "hello world", Left x4, Delete, End and a
token gives "hello wrld <token>".

Evidence: the `text` of LOGINWND's `id_edit` (state windows), the chat
event and the chatlog row.

    BF_CLIENT=<binary> port/harness/run.py edit_keys
"""

from bfharness.play import enter_game, login, own_home, token
from bfharness.sql import wait_rows

NAME = "edit_keys"
MILESTONE = "M5"
DESCRIPTION = "Left, Right, Home, End, Delete, Backspace, Cmd+A in the login field and the chat line"
MANIFEST = None
NEEDS_SERVER = True


def id_text(ctl):
    w = ctl.window("LOGINWND") or {}
    return {c["id"]: c for c in w.get("controls", [])}["id_edit"]["text"]


def keys(ctl, *names):
    for n in names:
        ctl.press(n)


def chat_line(t, ctl, s, char_id, typed, edit, expect):
    """Type `typed`, play `edit` (key names, or ("text", str)), Enter; the
    line the server logs is `expect`."""
    mark = t.logs.mark()
    after = ctl.last_seq
    ctl.type_text(typed)
    for k in edit:
        if isinstance(k, tuple):
            ctl.type_text(k[1])
        else:
            ctl.press(k)
    ctl.press("enter")
    ev = ctl.wait_event("chat", lambda e: expect in (e.get("text") or ""), after=after, timeout=10)
    s.state("chat event", ev)
    rows = s.rows("chatlog line %r" % expect,
                  wait_rows(lambda: t.logs.chat(char_id=char_id, contains=expect, after=mark), timeout=10))
    msg = rows[-1]["message"]
    s.check(msg.endswith(" : " + expect) or msg == expect, "chatlog message is the edited line", chatlog=msg)


def run(t):
    if t.mock:
        with t.step("real client only", "the mock does not model the edit controls' caret") as s:
            s.note("%s plays the real client against the test server; nothing to check against the mock" % NAME)
        return
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    name = t.param("char", "BFProbe")

    with t.step("set-up: place character", "SQL, offline: prontera 155,183") as s:
        c = t.accounts.set_char(name, map="prontera", x=155, y=183)
        char_id = c["char_id"]

    ctl = t.launch(env=own_home(t))
    with t.step("start", "the login window, focus in the account field") as s:
        ctl.wait_mode("login", 60)
        ctl.wait_window("LOGINWND", timeout=30)
        left = id_text(ctl)
        if left:
            s.note("account field holds %r (a saved account); cleared with End and Backspace" % left)
            ctl.press("end")
            for _ in left:
                ctl.press("backspace")
        s.equal(id_text(ctl), "", "account field empty")

    with t.step("Left", "Left moves the caret; typing inserts there") as s:
        ctl.type_text("abcdef")
        s.equal(id_text(ctl), "abcdef", "typed")
        keys(ctl, "left", "left")
        ctl.type_text("X")
        s.equal(id_text(ctl), "abcdXef", "Left x2, X")

    with t.step("Home and Delete", "Home goes to the start; Delete removes the character after the caret") as s:
        keys(ctl, "home", "delete")
        s.equal(id_text(ctl), "bcdXef", "Home, Delete")

    with t.step("End and Backspace", "End goes to the end; Backspace removes the last character") as s:
        keys(ctl, "end", "backspace")
        s.equal(id_text(ctl), "bcdXe", "End, Backspace")

    with t.step("edges", "Right and Delete at the end, Left at the start change nothing") as s:
        keys(ctl, "right", "delete")
        ctl.type_text("!")
        s.equal(id_text(ctl), "bcdXe!", "Right, Delete at the end, !")
        keys(ctl, "home", "left")
        ctl.type_text("<")
        s.equal(id_text(ctl), "<bcdXe!", "Home, Left at the start, <")
        keys(ctl, "delete", "backspace")
        s.equal(id_text(ctl), "cdXe!", "Delete, Backspace around the caret")
        keys(ctl, "right", "right", "right")
        ctl.type_text("b")
        s.equal(id_text(ctl), "cdXbe!", "Right x3, b")

    with t.step("Cmd+A", "F10: Cmd+A selects the whole text; typing replaces it, Backspace deletes it") as s:
        keys(ctl, "home", "right")
        ctl.press("a", "cmd")
        ctl.type_text("Z")
        s.equal(id_text(ctl), "Z", "Cmd+A, Z")
        ctl.type_text("yx")
        ctl.press("a", "cmd")
        ctl.press("backspace")
        s.equal(id_text(ctl), "", "Cmd+A, Backspace")
        ctl.type_text("cdXbe!")
        ctl.press("end")
        for _ in range(len(id_text(ctl))):
            ctl.press("backspace")
        s.equal(id_text(ctl), "", "cleared")

    login(t, ctl, user, password)
    enter_game(t, ctl)

    with t.step("chat: Home", "the chat line: Home, then typing inserts at the start") as s:
        chat_line(t, ctl, s, char_id, "hello world", ["home", ("text", "X")], "Xhello world")

    with t.step("chat: Left and Delete", "the chat line: Left x4, Delete; End, a token") as s:
        tok = token()
        chat_line(t, ctl, s, char_id, "hello world", ["left", "left", "left", "left", "delete", "end", ("text", " " + tok)],
                  "hello wrld " + tok)
    t.stop_client()
