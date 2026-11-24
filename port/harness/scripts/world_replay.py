"""M6.1 world loading, checked on a replayed session (no server needed).

Done when (implementation plan, M6 step 1, and the world package's brief):
the committed fixture (port/harness/fixtures/replay-zone.pktlog, a
bifrost-netprobe session through zone entry on prontera 155,183) replayed
into the client reaches game mode, the world loads and draws, and the client
is still alive 60 seconds later with no failed assertion in the trace. Frames
are captured through the control socket into
<game folder>/test/frames/world/ (outside git); their checksums go to
MANIFEST.

    BF_CLIENT=<absolute path> port/harness/run.py world_replay
    port/harness/run.py world_replay --param seconds=60 --param fixture=<log>
    port/harness/run.py world_replay --param map=alberta  # another readable
        map at the same cell (the fixture's map names replaced)
    port/harness/run.py world_replay --param refuse=grademk  # M6.6: the
        fixture cut and edited (under the run directory) so that the zone
        server sends the client to a map with a world file of version 2.4;
        the client refuses to enter and returns to character selection

The trace categories default to err,mode,res (BF_TRACE overrides); the
step "trace" lists every missing resource and failed assertion of the run.
"""

import os
import re
import time
from pathlib import Path

from bfharness import paths

NAME = "world"
MILESTONE = "M6"
DESCRIPTION = "a replayed zone entry loads Prontera, draws it and stays alive for 60 s"
MANIFEST = "port/harness/manifests/m6-world.sha256"
NEEDS_SERVER = False

FIXTURE = paths.HARNESS_DIR / "fixtures" / "replay-zone.pktlog"
MULTI = {"BF_MULTI_INSTANCE": "1"}


def _field(name):
    b = name.encode() + b".gat"
    return (b + b"\0" * (16 - len(b))).hex()


def renamed_fixture(t, src, new):
    """The whole fixture with prontera replaced by another readable map in
    HC_NOTIFY_ZONESVR and ZC_NPCACK_MAPMOVE (the position stays 155,183)."""
    out = []
    for line in Path(src).read_text().splitlines():
        f = line.split(" ")
        if len(f) > 5 and f[2] in ("0x0071", "0x0091"):
            f[5] = f[5].replace(_field("prontera"), _field(new))
        out.append(" ".join(f))
    dst = t.run_dir / ("replay-%s.pktlog" % new)
    dst.write_text("\n".join(out) + "\n")
    return dst


def refusal_fixture(t, src, new):
    """The fixture cut down to the session the client has when it refuses a
    map (M6.6): HC_NOTIFY_ZONESVR names `new` instead of prontera (the map
    name is a zero-padded 16-byte field), the zone connection ends after
    CZ_NOTIFY_ACTORINIT with a few server packets the client must drop,
    then CZ_RESTART type 1 and the server's ZC_RESTART_ACK, and a fourth
    connection repeats the character server's answer to CH_ENTER, as
    rAthena does after chrif_charselectreq."""
    field = _field
    lines = [l.split(" ") for l in Path(src).read_text().splitlines() if l.strip()]
    out, char_lines, t_ms = [], [], 0
    for f in lines:
        conn = f[6] if len(f) > 6 else "-"
        t_ms = int(f[0])
        if f[2] == "0x0071":
            f[5] = f[5].replace(field("prontera"), field(new))
        if conn.startswith("char/") and f[2] in ("0x0065", "0x84e5", "0x006b", "0x020d") or \
                (conn.startswith("char/") and f[4] == "raw"):
            char_lines.append(list(f))
        out.append(f)
        if f[2] == "0x007d":          # CZ_NOTIFY_ACTORINIT
            break
    out.append([str(t_ms), "S", "0x00b2", "3", "CZ_RESTART", "b20001", "zone/3"])
    out += [l for l in lines if (l[6] if len(l) > 6 else "").startswith("zone/")
            and l[1] == "R" and l[2] in ("0x0201", "0x008e")]
    out.append([str(t_ms + 1), "R", "0x00b3", "3", "ZC_RESTART_ACK", "b30001", "zone/3"])
    for f in char_lines:
        out.append([str(t_ms + 2)] + f[1:6] + ["char/4"])
    dst = t.run_dir / ("replay-refuse-%s.pktlog" % new)
    dst.write_text("".join(" ".join(f) + "\n" for f in out))
    return dst


def run(t):
    if t.mock:
        with t.step("real client only", "the mock has no packet-log replay") as s:
            s.note("%s replays a packet log into the real client (BF_PKT_REPLAY); nothing to check against the mock" % NAME)
        return
    fixture = Path(t.param("fixture", str(FIXTURE)))
    seconds = float(t.param("seconds", "60"))
    global MANIFEST
    target = t.param("refuse")
    mapname = t.param("map", "prontera")
    if target or mapname != "prontera":   # one checksum list per scene
        MANIFEST = "port/harness/manifests/m6-world-%s.sha256" % (("refuse-" + target) if target else mapname)
    os.environ.setdefault("BF_TRACE", "err,mode,res")
    trace = t.run_dir / "world-trace.txt"

    with t.step("fixture", "the recorded zone entry") as s:
        s.check(fixture.exists(), "fixture %s" % fixture)
        if target or mapname != "prontera":
            if target:
                fixture = refusal_fixture(t, fixture, target)
            else:
                fixture = renamed_fixture(t, fixture, mapname)
            s.check("%s.gat" % (target or mapname) in bytes.fromhex(
                [l.split(" ")[5] for l in fixture.read_text().splitlines() if " 0x0071 " in l][0]).decode("latin-1"),
                "map name replaced in HC_NOTIFY_ZONESVR")
        s.note("fixture: %s" % fixture)

    env = dict(MULTI, BF_PKT_REPLAY=str(fixture), BF_PKT_REPLAY_WAIT=t.param("wait", "3000"),
               BF_PKT_REPLAY_HOLD="-1", BF_TRACE_FILE=str(trace),
               BF_PKT_LOG=str(t.run_dir / "world-packets.txt"))
    ctl = t.launch(env=env)

    with t.step("to character selection", "login and server list against the log") as s:
        ctl.wait_mode("login", 60)
        ctl.type_text(t.param("user", "bftest"))
        ctl.press("tab")
        ctl.type_text(t.param("password", "bftest"))
        ctl.press("enter")
        ctl.wait_mode("servers", 30)
        time.sleep(0.5)
        ctl.press("enter")
        s.state("mode", ctl.wait_mode("charselect", 30))
        if t.param("select", "1") != "0":
            # Pick the character with the window's OK button, as a player
            # does (keyboard selection is the login package's, M5): the
            # client then sends the log's CH_SELECT_CHAR itself and knows its
            # character (name, job, level). Without it the replay passes over
            # the request after BF_PKT_REPLAY_WAIT and the session holds no
            # character. The button sits at (+213, +149) from the centre of
            # the logical screen; SELECTCHARWND's "ok" control once reported.
            time.sleep(1)
            try:
                x, y = ctl.control_rect("SELECTCHARWND", "ok")[:2]
                x, y = x + 4, y + 4
            except Exception:
                v = (ctl.state("view").get("view") or {}).get("logical") or {"w": 1024, "h": 768}
                x, y = v["w"] // 2 + 213, v["h"] // 2 + 149
            s.note("click OK at %d,%d" % (x, y))
            ctl.click(x, y)

    if target:
        with t.step("refusal", "M6.6: an unreadable world is a traced error and a return to character selection") as s:
            # no key at character selection: the replay passes over
            # CH_SELECT_CHAR and delivers HC_NOTIFY_ZONESVR naming the map
            deadline = time.monotonic() + 90
            seen = ["charselect"]
            while time.monotonic() < deadline:
                mode = ctl.state("mode")["mode"]
                if seen[-1] != mode:
                    seen.append(mode)
                if mode == "charselect" and len(seen) > 2:
                    break
                time.sleep(0.05)
            s.state("modes", seen)
            s.check(len(seen) > 2 and seen[-1] == "charselect", "left character selection and came back to it")
            time.sleep(1)
            s.frame("refused-%s" % target)
            lines = [l for l in trace.read_text(errors="replace").splitlines()
                     if "world:" in l or "res.unusable" in l or "ErrorMsg" in l]
            s.trace("refusal trace lines", lines)
            s.check(any("refusing to enter" in l for l in lines), "a traced error refuses the map")
            s.check(any("back to character selection" in l for l in lines), "the server acknowledged CZ_RESTART")
            s.check(not any("ErrorMsg" in l for l in lines), "no error box")
            unexpected = [l for l in trace.read_text(errors="replace").splitlines()
                          if "replay:" in l and ("does not have" in l or "passed over" in l) and "CH_SELECT_CHAR" not in l]
            s.trace("replay mismatches after character selection", unexpected)
            s.check(not unexpected, "the client sent CZ_NOTIFY_ACTORINIT and CZ_RESTART as the server expects")
        t.stop_client()
        return

    with t.step("game mode", "the replay brings the client into game mode on %s" % mapname) as s:
        st = ctl.wait_mode("game", 90)
        s.state("mode", st)
        m = ctl.state("map").get("map")
        if m is not None:      # the state part comes with M6.4
            s.equal(m.get("name"), mapname, "map (state)")
        world = [l for l in trace.read_text(errors="replace").splitlines() if 'mode game "' in l]
        s.trace("mode change", world)
        s.check(any('"%s.rsw"' % mapname in l for l in world), "game mode on %s.rsw (trace)" % mapname)

    t0 = time.monotonic()
    with t.step("world drawn", "frames of the loaded world") as s:
        time.sleep(3)
        s.frame("%s-3s" % mapname)
        st = ctl.state("self", "actors")
        if "self" in st or "actors" in st:      # the state parts come with M6.4
            s.state("self", st.get("self"))
            s.state("actors", (st.get("actors") or [])[:30])

    with t.step("alive", "the client is alive %d s after entering game mode" % seconds) as s:
        mid = t0 + seconds / 2
        while time.monotonic() < mid:
            time.sleep(1)
            s.check(t.client.proc.poll() is None, "client running")
        s.frame("%s-%ds" % (mapname, int(seconds / 2)))
        while time.monotonic() < t0 + seconds:
            time.sleep(1)
            s.check(t.client.proc.poll() is None, "client running")
        st = ctl.state("mode")
        s.equal(st["mode"], "game", "still in game mode")
        s.frame("%s-%ds" % (mapname, int(seconds)))

    t.stop_client()
    with t.step("trace", "missing resources and failed assertions of the session") as s:
        lines = trace.read_text(errors="replace").splitlines()
        missing = sorted({re.sub(r"^\[[^\]]*\]\s*", "", l) for l in lines
                          if "missing" in l and "[counter]" not in l})
        asserts = [l for l in lines if "assertion failed" in l]
        counters = [l for l in lines if l.startswith("[counter]")]
        s.trace("missing", missing, max_lines=200)
        s.trace("assertions", asserts)
        s.trace("counters", counters, max_lines=60)
        s.check(not asserts, "no failed assertion")
