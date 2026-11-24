"""M8.4 mail: the 2008 mail windows work against stock rAthena (PACKETVER 20080401).

Done when (implementation plan M8.4, mail): the mailbox opens from a
Mailbox NPC, the inbox lists a letter another character sent, the letter
opens and shows its text, and a letter written in the client reaches the
other character; each confirmed in the server's `mail` table.

Set-up (SQL, offline): two fresh accounts, each with one character; the
player at prontera 146,89 beside the Mailbox at 146,86 with 1000 zeny (the
NPC charges 130); one letter from the second character to the player in
`mail` (an inbox with something in it).
Played: click the Mailbox, take the dialog ("Yes."), which opens the mail
list (rAthena `openmail` -> ZC_MAIL_WINDOWS); click the letter's title
(CZ_MAIL_OPEN -> ZC_MAIL_REQ_OPEN) and read its text in the view window;
switch to the write tab, type recipient, title and text, click send
(CZ_MAIL_SEND -> ZC_MAIL_REQ_SEND).
Evidence: the letter's `mail.status` turns to MAIL_READ (2); a new `mail` row
from the player to the second character with the typed title and text,
byte for byte (the client puts a length byte before the text, rAthena
reads it); the zeny the NPC took in `zenylog`; the client's packet log
holds the mail packets in both directions with no unknown header.
"""

import time

from bfharness.accounts import unique_name
from bfharness.play import enter_game, login, own_home, token
from bfharness.sql import quote, wait_rows

NAME = "mail"
MILESTONE = "M8"
DESCRIPTION = "mail: open the mailbox at a Mailbox NPC, read a letter, write one; mail table"
MANIFEST = "port/harness/manifests/m8-mail.sha256"
NEEDS_SERVER = True

MAILBOX = (146, 86)          # npc/other/mail.txt: Mailbox#prt
START = (146, 89)
MAIL_READ = 2                # rAthena mmo.hpp mail_status: MAIL_NEW, MAIL_UNREAD, MAIL_READ
MAIL_HEADERS = {"0x0260": "ZC_MAIL_WINDOWS", "0x023f": "CZ_MAIL_GET_LIST", "0x0240": "ZC_MAIL_REQ_GET_LIST",
                "0x0241": "CZ_MAIL_OPEN", "0x0242": "ZC_MAIL_REQ_OPEN", "0x0248": "CZ_MAIL_SEND",
                "0x0249": "ZC_MAIL_REQ_SEND"}


def make_char(t, account_id, name, map, x, y, zeny=0, sex="F"):
    """A level-1 Novice in the account's first slot (SQL, offline set-up),
    stats 1 and 40/11 HP/SP, saved where it stands."""
    return t.accounts.make_char(account_id, name, 0, map, x, y, zeny=zeny, sex=sex, max_hp=40, hp=40,
                                str=1, agi=1, vit=1, int=1, dex=1, luk=1,
                                save_map=map, save_x=x, save_y=y)


def put_mail(t, sender, dest, title, message):
    """A letter in the inbox (SQL, offline set-up), as rAthena's char server writes one."""
    now = int(time.time())
    mock = getattr(t.db, "db", None)
    if mock is not None:
        mid = max([r["id"] for r in mock.rows("mail")] or [0]) + 1
        mock.put("mail", id=mid, send_name=sender["name"], send_id=sender["char_id"], dest_name=dest["name"],
                 dest_id=dest["char_id"], title=title, message=message, time=now, status=0, zeny=0, type=0)
        return mid
    t.db.execute(
        "INSERT INTO `mail` (`send_name`, `send_id`, `dest_name`, `dest_id`, `title`, `message`, `time`, `status`) "
        "VALUES (%s, %d, %s, %d, %s, %s, %d, 0)"
        % (quote(sender["name"]), sender["char_id"], quote(dest["name"]), dest["char_id"], quote(title),
           quote(message), now))
    return max(r["id"] for r in t.logs.db.select("mail", {"dest_id": dest["char_id"]}))


def mails(t, **where):
    return t.logs.db.select("mail", where, order="id")


def controls(w):
    return [{k: c.get(k) for k in ("id", "kind", "text", "rect", "items") if k in c}
            for c in (w or {}).get("controls", [])]


def dialog(ctl, s, choice):
    """Take an NPC dialog through: `next`, the choice, `close`."""
    deadline = time.monotonic() + 30
    chosen = 0
    while time.monotonic() < deadline:
        st = ctl.state("windows")
        names = [w["name"] for w in st["windows"]]
        if "MAILLISTWND" in names:
            return True
        if "CHOOSEWND" in names and chosen < 3:
            # UIChooseWnd ignores ok within 300 ms of the last dialog button.
            time.sleep(0.4)
            cw = ctl.window("CHOOSEWND", st)
            ids = [c["id"] for c in cw.get("controls", [])]
            s.state("choices", ids)
            if "choice:" + choice in ids:
                ctl.click_control("CHOOSEWND", "choice:" + choice)
            else:
                # The menu as a list: the first line ("Yes.") is the one
                # selected when the menu opens; click it, then ok.
                lx, ly, lw, lh = next(c["rect"] for c in cw["controls"] if c["kind"] == "list")
                ctl.click(lx + lw // 3, ly + 7)
                ctl.wait({"frames": 5}, timeout=10)
                ctl.click_control("CHOOSEWND", "ok")
            chosen += 1
        elif "SAYDIALOGWND" in names:
            w = ctl.window("SAYDIALOGWND", st)
            # The dialog keeps hidden buttons outside its rectangle; take a
            # visible one (the next or the close button).
            wx, wy, ww, wh = w["rect"]
            shown = [c for c in w.get("controls", []) if c["kind"] == "button"
                     and wx <= c["rect"][0] and c["rect"][0] + c["rect"][2] <= wx + ww
                     and wy <= c["rect"][1] and c["rect"][1] + c["rect"][3] <= wy + wh]
            if shown:
                bx, by, bw, bh = shown[0]["rect"]
                ctl.click(bx + bw // 2, by + bh // 2)
        ctl.wait({"frames": 10}, timeout=10)
    return False


def run(t):
    password = "bfh-test"
    user_a = t.param("user") or unique_name(tag="ma")
    user_b = unique_name(tag="mb")
    name_a = t.param("char") or unique_name(tag="Ma")
    name_b = unique_name(tag="Mb")
    title_in = "bfh in %s" % token()
    text_in = "Hello from %s, letter %s." % (name_b, token())
    title_out = "bfh out %s" % token()
    text_out = "Reply %s from the Mac" % token()

    with t.step("set-up: accounts, characters, a letter", "SQL, offline") as s:
        aid_a = t.accounts.create_account(user_a, password)
        aid_b = t.accounts.create_account(user_b, password, sex="M")
        a = make_char(t, aid_a, name_a, "prontera", START[0], START[1], zeny=1000)
        b = make_char(t, aid_b, name_b, "prontera", 150, 150, sex="M")
        mid = put_mail(t, b, a, title_in, text_in)
        s.state("player", {k: a[k] for k in ("char_id", "name", "last_map", "last_x", "last_y", "zeny")})
        s.state("other", {k: b[k] for k in ("char_id", "name")})
        s.rows("letter in the inbox", mails(t, id=mid))

    ctl = t.launch(env=own_home(t))
    login(t, ctl, user_a, password)
    enter_game(t, ctl)
    ctl.wait({"frames": 30}, timeout=20)

    with t.step("open the mailbox", "M8.4 mail: the Mailbox NPC opens the mail list (ZC_MAIL_WINDOWS)") as s:
        mark = t.logs.mark()
        box = ctl.actor(lambda a: a["type"] == "npc" and a["name"].startswith("Mailbox"))
        s.check(box is not None and box.get("screen"), "the Mailbox is on screen", actor=box)
        ctl.click(box["screen"]["x"], box["screen"]["y"])
        s.check(dialog(ctl, s, "Yes."), "the dialog ends in the mail list")
        ctl.wait_window("MAILLISTWND", timeout=15)
        ctl.wait({"frames": 20}, timeout=10)
        z = wait_rows(lambda: t.logs.zeny(a["char_id"], after=mark), timeout=10)
        s.state("zenylog", z)
        s.frame("mail-list")
        w = ctl.window("MAILLISTWND")
        s.state("MAILLISTWND controls", controls(w))
        header = [c for c in w.get("controls", []) if title_in in (c.get("text") or "")]
        s.check(header, "the letter's title is listed")
        sender = [c for c in w.get("controls", []) if c.get("text") == name_b]
        s.check(sender, "the sender's name is listed")

    with t.step("read the letter", "M8.4 mail: CZ_MAIL_OPEN, the view shows the text; mail.status MAIL_READ") as s:
        c = header[0]
        x, y, cw, ch = c["rect"]
        ctl.click(x + min(cw, 60) // 2, y + ch // 2)
        ctl.wait_window("MAILVIEWWND", timeout=15)
        ctl.wait({"frames": 20}, timeout=10)
        s.frame("mail-view")
        v = ctl.window("MAILVIEWWND")
        s.state("MAILVIEWWND controls", controls(v))
        texts = " ".join([c.get("text") or "" for c in v.get("controls", [])] +
                         [i for c in v.get("controls", []) for i in c.get("items") or []])
        s.check(text_in in texts, "the view shows the letter's text", texts=texts)
        rows = wait_rows(lambda: [r for r in mails(t, id=mid) if r["status"] == MAIL_READ], timeout=15)
        s.rows("mail %d status %d (MAIL_READ)" % (mid, MAIL_READ), rows)
        ctl.click_control("MAILVIEWWND", "close")
        ctl.wait_window("MAILVIEWWND", open=False, timeout=10)

    with t.step("write a letter", "M8.4 mail: CZ_MAIL_SEND reaches the other character's inbox") as s:
        mark_id = max([r["id"] for r in mails(t)] or [0])
        w = ctl.window("MAILLISTWND")
        wx, wy = w["rect"][0], w["rect"][1]
        ctl.click(wx + 10, wy + 100)          # the write tab on the window's left edge
        ctl.wait({"frames": 10}, timeout=10)
        w = ctl.window("MAILLISTWND")
        s.state("write controls", controls(w))
        # Recipient, title and text, in the order UIMailListWnd::OnCreate
        # makes them; the zeny field is parked outside the window until
        # its button is pressed.
        wx, wy, ww, wh = w["rect"]
        edits = [c for c in w.get("controls", []) if c["kind"] == "edit"
                 and wx <= c["rect"][0] < wx + ww and wy <= c["rect"][1] < wy + wh]
        s.check(len(edits) >= 3, "recipient, title and text fields", edits=edits)
        for field, value in zip(edits[:3], (name_b, title_out, text_out)):
            fx, fy, fw, fh = field["rect"]
            ctl.click(fx + 4, fy + min(fh, 16) // 2)
            ctl.type_text(value)
        s.frame("mail-write")
        after = ctl.last_seq
        ctl.click_control("MAILLISTWND", "send_mail")
        rows = wait_rows(lambda: [r for r in mails(t, dest_id=b["char_id"]) if r["id"] > mark_id], timeout=20)
        s.rows("mail from %s to %s" % (name_a, name_b), rows)
        if rows:
            r = rows[-1]
            s.equal(r["send_name"], name_a, "sender")
            s.equal(r["title"], title_out, "title")
            s.equal(r["message"], text_out, "text, byte for byte")
        try:
            ev = ctl.wait_event("chat", lambda e: e.get("channel") == "system", after=after, timeout=10)
            s.state("client's message", ev)
        except Exception as e:
            s.note("no system line after sending: %s" % e)

    client = t.client
    t.stop_client()
    with t.step("packets", "the mail packets in both directions, no unknown header") as s:
        seen = {}
        path = client.pkt_log
        if path and path.exists():
            for line in path.read_text(errors="replace").splitlines():
                f = line.split()
                if len(f) >= 3 and f[2].lower() in MAIL_HEADERS:
                    seen.setdefault(MAIL_HEADERS[f[2].lower()], 0)
                    seen[MAIL_HEADERS[f[2].lower()]] += 1
        s.state("mail packets", seen)
        if not t.mock:
            s.check(set(MAIL_HEADERS.values()) <= set(seen), "every mail packet of the session was seen",
                    missing=sorted(set(MAIL_HEADERS.values()) - set(seen)))
        unknown = client.trace_lines("unknown packet header")
        s.trace("unknown packet header lines", unknown)
        s.check(not unknown, "no unknown packet header")
