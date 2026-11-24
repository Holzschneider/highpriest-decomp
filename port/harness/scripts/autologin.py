"""M5 the same login from launch arguments alone.

Done when (implementation plan, M5): the same login works from launch
arguments alone. The client is started with --server, --account, --password
and --character (or, with --param via=env, with BF_SERVER, BF_ACCOUNT,
BF_PASSWORD and BF_CHARACTER) and gets no input at all: it logs in, takes
the character server, selects the character and sends CZ_ENTER.

Evidence: the mode events (login -> connecting -> ... -> loading/game) with
no key or pointer request sent; loginlog 'login ok' and charlog 'char select'
after the marker; CZ_ENTER on the zone connection in the packet log; the
login sequence equals bifrost-netprobe's. The map load after CZ_ENTER is
M6.1's, recorded but not judged (charselect.py, stop_after_zone_entry).

    BF_CLIENT=<binary> port/harness/run.py autologin [--param via=env]
"""

from bfharness.play import CZ_ENTER, compare_with_netprobe, stop_after_zone_entry, wait_sent
from bfharness.sql import wait_rows

NAME = "autologin"
MILESTONE = "M5"
DESCRIPTION = "login, server choice and character selection from launch arguments, without input"
MANIFEST = "port/harness/manifests/m5-autologin.sha256"
NEEDS_SERVER = True


def run(t):
    if t.mock:
        with t.step("real client only", "the mock does not model this screen's details") as s:
            s.note("%s plays the real client against the test server; nothing to check against the mock" % NAME)
        return
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    server = t.param("server", "127.0.0.1:6900")
    via = t.param("via", "args")

    acc = t.logs.account(user)
    chars = sorted(t.logs.chars(acc["account_id"]), key=lambda c: c["char_num"]) if acc else []
    charname = t.param("character") or (chars[0]["name"] if chars else None)

    with t.step("launch", "M5.5: the client starts with server, account, password and character") as s:
        s.check(charname is not None, "a character on %s" % user)
        mark = t.logs.mark()
        if via == "env":
            env = {"BF_SERVER": server, "BF_ACCOUNT": user, "BF_PASSWORD": password, "BF_CHARACTER": charname}
            ctl = t.launch(env=env)
            s.note("environment BF_SERVER=%s BF_ACCOUNT=%s BF_PASSWORD=... BF_CHARACTER=%s" % (server, user, charname))
        else:
            args = ["--server", server, "--account", user, "--password", password, "--character", charname]
            ctl = t.launch(args=args)
            s.note("arguments --server %s --account %s --password ... --character %s" % (server, user, charname))

    with t.step("no input", "M5.5: login, server choice and selection happen by themselves") as s:
        sent = wait_sent(t, CZ_ENTER, "zone", 90)
        s.check(bool(sent), "CZ_ENTER sent on the zone connection")
        modes = [e["mode"] for e in ctl.events_named("mode")]
        s.state("modes", modes)
        s.check("connecting" in modes, "went through the login")
        s.rows("loginlog 'login ok' %s" % user, wait_rows(lambda: t.logs.logins(user, after=mark), timeout=10))
        s.rows("charlog 'char select' %s" % charname,
               wait_rows(lambda: t.logs.charlog(name=charname, msg="char select", after=mark), timeout=10))
        transcript = (t.run_dir / "control-transcript.txt")
        sent_cmds = []
        if transcript.exists():
            for line in transcript.read_text(errors="replace").splitlines():
                for cmd in ("\"key\"", "\"pointer\""):
                    if "\"cmd\": " + cmd in line or "\"cmd\":" + cmd in line:
                        sent_cmds.append(line[:120])
        s.equal(sent_cmds, [], "key or pointer requests sent")
        opened = sorted({e["name"] for e in ctl.events_named("window") if e.get("action") == "open"})
        s.state("windows opened", opened)
        for w in ("LOGINWND", "SELECTSERVERWND", "SELECTCHARWND"):
            s.check(w not in opened, "%s not shown" % w)

    stop_after_zone_entry(t)
    with t.step("packets", "M5: the login's packets match bifrost-netprobe's") as s:
        compare_with_netprobe(t, s, t.run_dir / "client-packets.txt", user, password, charname)
