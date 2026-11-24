"""M9.1: the login packet and the code page of the English service type.

Done when (implementation plan, M9.1): the English service type (clientinfo
<servicetype>america</servicetype>, written by port/data/assemble.py) sends
CA_LOGIN (0x0064), not the Korean CA_LOGIN_HAN (0x02b0); chat and names are
Windows-1252 on the wire.

Set-up: a fresh account in SQL. Played: log in, open character creation,
type a name with a letter outside ASCII (default "Renée" and a number), confirm.
Evidence: the packet log (CA_LOGIN sent, no CA_LOGIN_HAN; the name in
CH_MAKE_CHAR is the Windows-1252 bytes, é = 0xE9) and, when the server makes
the character, the char row's name and the selection slot.

The server's side needs two settings of port/server (configuration only):
char_name_option 0 (stock rAthena allows only ASCII letters and digits in
names when it is 1) and default_codepage latin1 (MariaDB's latin1 is
Windows-1252; the tables are utf8mb4, so the connection must say what the
bytes are). Without them the server refuses the name (HC_REFUSE_MAKECHAR);
the step then fails and says so, the wire checks stand on their own.
"""

from bfharness.accounts import unique_name
from bfharness.play import login
from bfharness.sql import wait_rows

NAME = "english"
MILESTONE = "M9"
DESCRIPTION = "English service: CA_LOGIN and Windows-1252 names on the wire"
NEEDS_SERVER = True

CA_LOGIN = "0x0064"
CA_LOGIN_HAN = "0x02b0"


def sent(t, header):
    """The sent packets with this header: (name, hex bytes) per line of the packet log."""
    path = t.run_dir / "client-packets.txt"
    out = []
    if not path.exists():
        return out
    for line in path.read_text(errors="replace").splitlines():
        f = line.split()
        if len(f) >= 6 and f[1] == "S" and f[2].lower() == header:
            out.append((f[4], f[5]))
    return out


def received(t, header):
    path = t.run_dir / "client-packets.txt"
    if not path.exists():
        return []
    return [line.split()[5] for line in path.read_text(errors="replace").splitlines()
            if len(line.split()) >= 6 and line.split()[1] == "R" and line.split()[2].lower() == header]


def run(t):
    userid = t.param("user") or unique_name(tag="e")
    password = t.param("password", "bfh-test")
    charname = t.param("name") or ("Renée" + unique_name(tag="")[-6:])
    wire = charname.encode("cp1252")

    with t.step("set-up: fresh account", "account without characters") as s:
        aid = t.accounts.create_account(userid, password)
        s.note("created %s (account %d) in SQL" % (userid, aid))

    ctl = t.launch()
    login(t, ctl, userid, password)

    with t.step("login packet", "M9.1: the English service sends CA_LOGIN, not CA_LOGIN_HAN") as s:
        if t.mock:
            s.note("mock: no packet log")
        else:
            logins = sent(t, CA_LOGIN)
            s.state("CA_LOGIN sent", logins)
            s.check(len(logins) == 1, "one CA_LOGIN (0x0064) sent")
            s.equal(len(sent(t, CA_LOGIN_HAN)), 0, "CA_LOGIN_HAN (0x02b0) sent")
            if logins:
                raw = bytes.fromhex(logins[0][1])
                s.equal(raw[6:30].split(b"\0")[0], userid.encode("ascii"), "account name in CA_LOGIN")

    with t.step("name on the wire", "M9.1: a typed name is Windows-1252 in CH_MAKE_CHAR") as s:
        mark = t.logs.mark()
        ctl.press("enter")                 # Enter on an empty slot opens creation
        ctl.wait_mode("makechar", 30)
        ctl.wait_window("MAKECHARWND")
        ctl.click_control("MAKECHARWND", "name_edit")
        ctl.type_text(charname)
        w = ctl.window("MAKECHARWND")
        texts = {c["id"]: c["text"] for c in (w or {}).get("controls", [])}
        if texts:
            s.equal(texts.get("name_edit"), charname, "name field")
        s.frame("makechar-name")
        ctl.press("enter")
        if t.mock:
            s.note("mock: no packet log")
        else:
            made = wait_rows(lambda: sent(t, "0x0067"), timeout=10)
            s.state("CH_MAKE_CHAR sent", made)
            s.check(bool(made), "CH_MAKE_CHAR (0x0067) sent")
            if made:
                raw = bytes.fromhex(made[-1][1])
                s.equal(raw[2:26].split(b"\0")[0].hex(), wire.hex(), "name bytes (Windows-1252)")

    with t.step("name in the char table", "M9.1: the server stores the name; the slot shows it") as s:
        if t.mock:
            s.note("mock: no server")
            return
        wait_rows(lambda: received(t, "0x006e") or received(t, "0x006d"), timeout=15)
        if received(t, "0x006e") and not received(t, "0x006d"):
            raise AssertionError("the server refused the name (HC_REFUSE_MAKECHAR %s); port/server needs "
                                 "char_name_option: 0 and default_codepage: latin1" % received(t, "0x006e"))
        ctl.wait_mode("charselect", 30)
        rows = wait_rows(lambda: t.logs.charlog(name=charname, msg="make new char", after=mark))
        s.rows("charlog 'make new char' %s" % charname, rows)
        c = t.logs.char(charname)
        s.check(c is not None, "char row named %r" % charname)
        w = ctl.window("SELECTCHARWND")
        slots = [x["text"] for x in (w or {}).get("controls", []) if x["id"].startswith("slot:")]
        s.state("slots", slots)
        if slots:
            s.check(charname in slots, "%s shown in a slot" % charname)
        s.frame("charselect-name")
