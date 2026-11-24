"""M6.5 packet replay: BF_PKT_REPLAY feeds a recorded packet log to the client.

Done when (implementation plan, M6 step 5): a recorded log replayed into the
client brings it to the state the recording reached, without a server: the
client opens no network socket (lsof), and its own BF_PKT_LOG of the replay
has the recording's packet sequence.

Steps:
  record    the real client logs in (account, password, server list) to
            character selection against the test server, with BF_PKT_LOG
            (skipped with --param record=0)
  replay    the same input against that log: same modes, no socket, the same
            sequence of (direction, header, size, connection)
  zone      (--param zone=1) the committed fixture (port/harness/fixtures/replay-zone.pktlog,
            a bifrost-netprobe session through zone entry on the test server)
            replayed into the client: login, server list, then character
            selection with no key pressed: the log's CH_SELECT_CHAR is passed
            over after BF_PKT_REPLAY_WAIT and the client follows the log to
            the zone server and into game mode. Opt-in while the world
            loader cannot load the map (M6.1): the client reaches game mode
            and then crashes in C3dGround::CalcLmIntensityFloatRGBAll, which
            fails the harness's clean-exit step.

    port/harness/run.py replay                               # record and replay; the server must be up
    port/harness/run.py replay --param zone=1                # and the zone step
    port/harness/run.py replay --param record=0 --param zone=1   # the fixture only; no server needed
    port/harness/run.py replay --param zone=1 --param fixture=<log>

The packet log format: port/instrument/bf_trace.h; the replay rules:
port/compat/include/bf_replay.h.
"""

import os
import subprocess
import time
from pathlib import Path

from bfharness import paths

NAME = "replay"
MILESTONE = "M6"
DESCRIPTION = "a recorded packet log replayed into the client reaches the recorded state without a server"
MANIFEST = None
NEEDS_SERVER = False      # only the record step needs it; it brings it up itself

FIXTURE = paths.HARNESS_DIR / "fixtures" / "replay-zone.pktlog"
MULTI = {"BF_MULTI_INSTANCE": "1"}   # other clients may be running on this machine


def packet_lines(path):
    """(direction, header, size, connection) per packet line of a BF_PKT_LOG file."""
    out = []
    if not Path(path).exists():
        return out
    for line in Path(path).read_text(errors="replace").splitlines():
        f = line.split()
        if len(f) >= 6 and f[1] in ("S", "R"):
            out.append((f[1], f[2], int(f[3]), f[6] if len(f) > 6 else "-", f[4]))
    return out


def without_keepalive(seq):
    # CZ_PING / CH ping (0x0187) is sent on a timer: how many fall into a run
    # depends on how long it takes, not on the protocol.
    return [p for p in seq if not (p[0] == "S" and p[1] == "0x0187")]


def tcp_sockets(pid):
    r = subprocess.run(["lsof", "-a", "-n", "-P", "-p", str(pid), "-i"], capture_output=True, text=True)
    return [l for l in r.stdout.splitlines()[1:] if l.strip()]


def launch(t, label, env):
    """Start the client with its own trace and packet log in the run directory."""
    pkt = t.run_dir / ("%s-packets.txt" % label)
    trace = t.run_dir / ("%s-trace.txt" % label)
    for p in (pkt, trace):
        if p.exists():
            p.unlink()
    e = dict(MULTI, BF_PKT_LOG=str(pkt), BF_TRACE_FILE=str(trace))
    os.environ.setdefault("BF_TRACE", "err,net,mode,stub")
    e.update(env)
    ctl = t.launch(env=e)
    return ctl, pkt, trace


def stop(t, ctl):
    t.stop_client()   # asks the client to quit, waits, records the exit status


def type_login(ctl, user, password):
    ctl.wait_mode("login", 60)
    ctl.type_text(user)
    ctl.press("tab")
    ctl.type_text(password)
    ctl.press("enter")


def to_charselect(t, s, ctl, user, password):
    type_login(ctl, user, password)
    st = ctl.wait_mode("servers", 30)
    s.state("after login", st)
    time.sleep(0.5)
    ctl.press("enter")
    st = ctl.wait_mode("charselect", 30)
    s.state("after the server list", st)
    return st


def replay_lines(trace):
    return [l for l in Path(trace).read_text(errors="replace").splitlines() if "replay:" in l] \
        if Path(trace).exists() else []


def run(t):
    user = t.param("user", "bftest")
    password = t.param("password", "bftest")
    record = t.param("record", "1") != "0"
    fixture = Path(t.param("fixture", str(FIXTURE)))
    zone = t.param("zone", "0") != "0"

    if record:
        with t.step("record", "the real client logs in to character selection with BF_PKT_LOG") as s:
            s.note("server: %s" % t.server.ensure_up())
            ctl, rec_pkt, _ = launch(t, "record", {})
            st = to_charselect(t, s, ctl, user, password)
            s.equal(st["mode"], "charselect", "mode")
            time.sleep(1)
            recorded_state = ctl.state("mode", "detail")
            s.state("recorded state", recorded_state)
            stop(t, ctl)
            rec = packet_lines(rec_pkt)
            s.state("recorded packets", [" ".join(map(str, p)) for p in rec])
            conns = sorted({p[3] for p in rec}, key=lambda c: c.split("/")[-1])
            s.equal(conns[:2], ["account/1", "char/2"], "connections in the log")
            s.check(any(p[4] == "raw" for p in rec), "the character server's account id is recorded (raw)")

        with t.step("replay", "the same input against the log: same state, no socket, same packets") as s:
            ctl, rep_pkt, rep_trace = launch(t, "replay", {"BF_PKT_REPLAY": str(rec_pkt),
                                                         "BF_PKT_REPLAY_HOLD": "-1"})
            st = to_charselect(t, s, ctl, user, password)
            time.sleep(1)
            replayed_state = ctl.state("mode", "detail")
            s.state("replayed state", replayed_state)
            s.equal(replayed_state["mode"], recorded_state["mode"], "mode")
            s.equal(replayed_state["detail"], recorded_state["detail"], "sub-mode")
            sockets = tcp_sockets(t.client.proc.pid)
            s.state("lsof -i of the client", sockets)
            s.check(not sockets, "the client has no network socket")
            stop(t, ctl)
            rep = packet_lines(rep_pkt)
            s.trace("replay trace lines", replay_lines(rep_trace))
            a, b = without_keepalive(rec), without_keepalive(rep)
            s.state("replayed packets", [" ".join(map(str, p)) for p in rep])
            s.equal([p[:4] for p in b], [p[:4] for p in a], "packet sequence (direction, header, size, connection)")

    if not zone:
        return
    with t.step("zone", "a recording through zone entry, replayed: the client reaches game mode without a server") as s:
        s.check(fixture.exists(), "fixture %s" % fixture)
        log = packet_lines(fixture)
        s.note("fixture: %d packets, connections %s" % (len(log), sorted({p[3] for p in log})))
        ctl, zone_pkt, zone_trace = launch(t, "zone", {"BF_PKT_REPLAY": str(fixture),
                                                    "BF_PKT_REPLAY_WAIT": t.param("wait", "3000"),
                                                    "BF_PKT_REPLAY_HOLD": "-1"})
        to_charselect(t, s, ctl, user, password)
        sockets = tcp_sockets(t.client.proc.pid)
        s.state("lsof -i of the client at character selection", sockets)
        s.check(not sockets, "the client has no network socket")
        # No key: the client cannot pick the character yet (M5); the replay
        # passes over the log's CH_SELECT_CHAR and delivers HC_NOTIFY_ZONESVR.
        st = ctl.wait_mode("game", 90)
        s.state("mode", st)
        sockets = tcp_sockets(t.client.proc.pid)
        s.state("lsof -i of the client in game mode", sockets)
        s.check(not sockets, "the client has no network socket")
        time.sleep(5)
        stop(t, ctl)
        lines = replay_lines(zone_trace)
        s.trace("replay trace lines", lines)
        got = packet_lines(zone_pkt)
        recv_log = [p[:4] for p in log if p[0] == "R"]
        recv_got = [p[:4] for p in got if p[0] == "R"]
        s.state("client packets", [" ".join(map(str, p)) for p in got])
        s.note("received %d of the fixture's %d packets" % (len(recv_got), len(recv_log)))
        s.equal(recv_got, recv_log[:len(recv_got)], "received packets: the fixture's, in order")
        s.check(any(p[1] in ("0x0073", "0x02eb") for p in recv_got), "ZC_ACCEPT_ENTER(2) delivered")
        sent = [p for p in got if p[0] == "S"]
        s.check(any(p[3].startswith("zone/") and p[1] == "0x009b" for p in sent), "the client sent CZ_ENTER (0x009b) on the zone connection")
        s.check(any(p[1] == "0x007d" for p in sent), "the client sent CZ_NOTIFY_ACTORINIT once the map was loaded")
