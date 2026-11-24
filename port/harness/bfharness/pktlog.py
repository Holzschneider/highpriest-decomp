"""Packet logs (BF_PKT_LOG) and the comparison of a login with bifrost-netprobe's.

A line of the log is: time, direction (S sent, R received), header, size,
name, hex bytes, connection (account/1, char/2, zone/3). The login sequence
of a session is what the client sends and receives on the account and
character connections, and what it sends on the zone connection up to and
including its first packet there (CZ_ENTER): the part both programs play the
same way, before map loading and the server's stream of zone packets.

    python3 port/harness/bfharness/pktlog.py <client log> <netprobe log>

prints both sequences side by side and exits 1 when they differ (compare()
says what counts as the same).
"""

import subprocess
import sys
from pathlib import Path


class Packet:
    __slots__ = ("time", "dir", "header", "size", "name", "conn")

    def __init__(self, time, dir, header, size, name, conn):
        self.time, self.dir, self.header, self.size, self.name, self.conn = time, dir, header, size, name, conn

    def key(self):
        return (self.dir, self.header, self.conn.split("/")[0])

    def __repr__(self):
        return "%s %s %s %s" % (self.dir, self.header, self.name, self.conn)


def parse(lines):
    out = []
    for line in lines:
        f = line.split()
        if len(f) < 6 or f[1] not in ("S", "R"):
            continue
        conn = f[6] if len(f) > 6 else "?"
        try:
            out.append(Packet(int(f[0]), f[1], f[2].lower(), int(f[3]), f[4], conn))
        except ValueError:
            continue
    return out


def read(path):
    p = Path(path)
    return parse(p.read_text(errors="replace").splitlines()) if p.exists() else []


# Sent on a timer, not by the login's steps: left out of the comparison.
KEEPALIVE = {"0x0187", "0x7e", "0x007e", "0x0089"}


def login_sequence(packets):
    """The packets of the login, from CA_LOGIN to the first zone packet sent."""
    seq = []
    for p in packets:
        kind = p.conn.split("/")[0]
        if kind == "zone":
            if p.dir == "S":
                seq.append(p)
                break
            continue
        if p.header in KEEPALIVE:
            continue
        seq.append(p)
    return seq


def compare(client, probe):
    """(equal, lines). Equal when the packets sent are the same, in the same
    order, on the same connections, and each connection received the same
    packets. The order of what is received may differ: the character server
    sends HC_BLOCK_CHARACTER on its own after HC_ACCEPT_ENTER, before or after
    the program's CH_SELECT_CHAR depending on how fast that comes."""
    a, b = login_sequence(client), login_sequence(probe)
    sent_a = [p.key() for p in a if p.dir == "S"]
    sent_b = [p.key() for p in b if p.dir == "S"]
    recv_a = sorted(p.key() for p in a if p.dir == "R")
    recv_b = sorted(p.key() for p in b if p.dir == "R")
    equal = bool(a) and sent_a == sent_b and recv_a == recv_b
    lines = []
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else None
        y = b[i] if i < len(b) else None
        same = x is not None and y is not None and x.key() == y.key()
        lines.append("%s %-40s %s" % ("  " if same else "~ ", x if x else "-", y if y else "-"))
    lines.append("sent: %s" % ("same order" if sent_a == sent_b else "DIFFERENT"))
    lines.append("received per connection: %s" % ("same packets" if recv_a == recv_b else "DIFFERENT"))
    return equal, lines


def has_sent(packets, header, conn_kind=None):
    return [p for p in packets if p.dir == "S" and p.header == header and
            (conn_kind is None or p.conn.startswith(conn_kind))]


def run_netprobe(binary, log_path, args=(), timeout=120):
    """Runs bifrost-netprobe with BF_PKT_LOG=log_path. Returns (exit code, last output line)."""
    import os
    env = dict(os.environ)
    env["BF_PKT_LOG"] = str(log_path)
    Path(log_path).unlink(missing_ok=True)
    r = subprocess.run([str(binary)] + list(args), env=env, capture_output=True, text=True, timeout=timeout)
    out = (r.stdout + r.stderr).strip().splitlines()
    ok = [l for l in out if "netprobe:" in l]
    return r.returncode, (ok or out or [""])[-1]


def netprobe_next_to_client():
    """bifrost-netprobe in the build folder of BF_CLIENT, or None."""
    import os
    exe = (os.environ.get("BF_CLIENT") or "").split()
    if exe:
        cand = Path(exe[0]).parent / "bifrost-netprobe"
        if cand.exists():
            return cand
    return None


def main(argv):
    if len(argv) != 3:
        print(__doc__.strip())
        return 2
    equal, lines = compare(read(argv[1]), read(argv[2]))
    print("\n".join(lines))
    print("same login sequence" if equal else "sequences differ")
    return 0 if equal else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
