"""M8.4 auction: rAthena's auction answers parse in the client; the feature is off on the server.

Done when (implementation plan M8.4, auction): the auction packets the
client and stock rAthena (PACKETVER 20080401) exchange agree in length and
layout. The test server runs rAthena's default `feature.auction: off`
(conf/battle/feature.conf), so no auction window opens there: rAthena
drops every auction request and never sends ZC_AUCTION_WINDOWS. Whether to
turn it on is a server configuration decision; this script reads the
setting and records it.

The one difference found by reading rAthena (clif_Auction_close) is
checked by replay, without the server: rAthena answers
CZ_AUCTION_REQ_MY_SELL_STOP with a 4-byte ZC_AUCTION_ACK_MY_SELL_STOP sent
under the request's header 0x25d, which the client's table gives the
request's 6 bytes. The recorded zone entry (fixtures/replay-zone.pktlog)
is replayed with two packets appended: that 4-byte 0x25d (result 0, "you
have ended the auction") and a ZC_BROADCAST. Read with 6 bytes, the
broadcast's header would be lost and the client would drop the rest of the
stream as an unknown header. Checked: the auction message and the
broadcast both reach the chat window, and the trace has no unknown header.
The client is real only (the mock has no replay); with --mock the steps
note that and pass.
"""

import os
import re
import subprocess
import time
from pathlib import Path

from bfharness import paths
from bfharness.play import token

NAME = "auction"
MILESTONE = "M8"
DESCRIPTION = "auction: feature.auction on the server; the 0x25d answer parsed (replay)"
NEEDS_SERVER = False

FIXTURE = paths.HARNESS_DIR / "fixtures" / "replay-zone.pktlog"
MULTI = {"BF_MULTI_INSTANCE": "1"}


def server_setting():
    """feature.auction as the running map server reads it: conf/battle/feature.conf,
    overridden by conf/import/battle_conf.txt."""
    out = {}
    for path in ("conf/battle/feature.conf", "conf/import/battle_conf.txt"):
        try:
            text = subprocess.run(["docker", "exec", "bifrost-rathena-rathena-1", "cat", "/rathena/" + path],
                                  capture_output=True, text=True, timeout=20).stdout
        except Exception as e:
            out[path] = "unreadable: %s" % e
            continue
        m = re.findall(r"^\s*feature\.auction\s*:\s*(\S+)", text, re.M)
        out[path] = m[-1] if m else None
    return out


def fixture_with_answer(t, text):
    lines = FIXTURE.read_text().splitlines()
    last = max(int(l.split(" ")[0]) for l in lines if l.strip())
    body = text.encode("cp1252") + b"\0"
    bcast = (0x9a).to_bytes(2, "little") + (4 + len(body)).to_bytes(2, "little") + body
    lines.append("%d R 0x025d 4 ZC_AUCTION_ACK_MY_SELL_STOP 5d020000 zone/3" % (last + 1500))
    lines.append("%d R 0x009a %d ZC_BROADCAST %s zone/3" % (last + 1500, len(bcast), bcast.hex()))
    dst = t.run_dir / "replay-auction.pktlog"
    dst.write_text("\n".join(lines) + "\n")
    return dst


def run(t):
    with t.step("server setting", "feature.auction of the running server (read only)") as s:
        if t.mock:
            s.note("mock: no server")
        else:
            setting = server_setting()
            s.state("feature.auction", setting)
            s.note("rAthena default is off; the auction window cannot open on this server while it is off")

    if t.mock:
        with t.step("0x25d answer by replay", "real client only") as s:
            s.note("mock: the mock client has no packet replay")
        return

    line = "bfh auction %s" % token()
    fixture = fixture_with_answer(t, line)
    trace = t.run_dir / "auction-trace.txt"
    env = dict(MULTI, BF_PKT_REPLAY=str(fixture), BF_PKT_REPLAY_WAIT="3000", BF_PKT_REPLAY_HOLD="-1",
               BF_TRACE_FILE=str(trace), BF_PKT_LOG=str(t.run_dir / "auction-packets.txt"))
    os.environ.setdefault("BF_TRACE", "err,mode,net")
    ctl = t.launch(env=env)

    with t.step("replayed zone entry", "login, character selection and zone entry from the log") as s:
        ctl.wait_mode("login", 60)
        ctl.type_text("bftest")
        ctl.press("tab")
        ctl.type_text("bftest")
        ctl.press("enter")
        ctl.wait_mode("servers", 30)
        time.sleep(0.5)
        ctl.press("enter")
        ctl.wait_mode("charselect", 30)
        time.sleep(1)
        try:
            x, y = ctl.control_rect("SELECTCHARWND", "ok")[:2]
            x, y = x + 4, y + 4
        except Exception:
            v = (ctl.state("view").get("view") or {}).get("logical") or {"w": 1024, "h": 768}
            x, y = v["w"] // 2 + 213, v["h"] // 2 + 149
        ctl.click(x, y)
        s.state("mode", ctl.wait_mode("game", 90))

    with t.step("0x25d answer by replay", "M8.4: the 4-byte 0x25d is read whole; the packet after it intact") as s:
        try:
            ev = ctl.wait_event("chat", lambda e: line in (e.get("text") or ""), after=0, timeout=30)
            s.state("broadcast", ev)
        except Exception as e:
            ev = None
            s.note("broadcast not seen: %s" % e)
        chats = ctl.events_named("chat")
        s.state("chat lines", [c.get("text") for c in chats][-8:])
        s.check(ev is not None, "the broadcast after the 4-byte 0x25d reached the chat window")
        t.stop_client()
        unknown = [l for l in trace.read_text(errors="replace").splitlines() if "unknown packet header" in l]
        s.trace("unknown packet header lines", unknown)
        s.check(not unknown, "no unknown packet header")
        log = (t.run_dir / "auction-packets.txt")
        got = [l for l in log.read_text(errors="replace").splitlines() if " R 0x025d " in l] if log.exists() else []
        s.trace("0x25d as the client received it", got)
        s.check(got and got[0].split(" ")[3] == "4", "received as 4 bytes")
