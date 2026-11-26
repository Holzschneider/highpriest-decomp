#!/usr/bin/env python3
"""The headless start on folders that are not usable game folders (ctest
`foldercheck`, label headless).

    foldercheck_test.py <client executable> <scratch directory> <application name>

Each folder is made under the scratch directory from generated files (no game
data): empty, missing, relative, only rdata.grf, an empty valid GRF, a GRF
without the files the start needs, and an empty valid GRF with those files
loose. `bifrost --headless` must refuse each of the first ones with exit
status 1, the line `start: game folder not usable: <reason>` naming what is
missing and `exit: start refused, status 1`, and never crash; the last one
must start (exit 0, `exit: WinMain returned 0`) with its missing data counted.
With game data in ~/Games/<name> (or BF_CLIENT_DIR) that folder must start too.
Exit 0 when every case holds, 1 otherwise, 77 (skipped) without the client.
"""

import os
import shutil
import struct
import subprocess
import sys
import zlib

MONSTER_CP949 = b"\xb8\xf3\xbd\xba\xc5\xcd"     # 몬스터, as the archive stores it
MONSTER = "몬스터"
NEEDED = [b"data\\sclientinfo.xml", b"data\\msgstringtable.txt",
          b"data\\sprite\\" + MONSTER_CP949 + b"\\poring.spr",
          b"data\\sprite\\" + MONSTER_CP949 + b"\\poring.act"]
NEEDED_LOOSE = ["data/sclientinfo.xml", "data/msgstringtable.txt",
                "data/sprite/%s/poring.spr" % MONSTER, "data/sprite/%s/poring.act" % MONSTER]
CLIENTINFO = (b'<?xml version="1.0" encoding="windows-1252" ?>\n<clientinfo>\n'
              b'<servicetype>america</servicetype>\n<servertype>primary</servertype>\n'
              b'<connection>\n<display>Local</display>\n<address>127.0.0.1</address>\n'
              b'<port>6900</port>\n<version>20</version>\n</connection>\n</clientinfo>\n')


def grf(path, names=()):
    """A GRF 0x200 whose table names `names`, each a file of no bytes."""
    table = b"".join(n + b"\0" + struct.pack("<IIIBI", 0, 0, 0, 1, 0) for n in names)
    z = zlib.compress(table)
    head = b"Master of Magic\0" + bytes(14) + struct.pack("<IIII", 0, 0, len(names) + 7, 0x200)
    with open(path, "wb") as f:
        f.write(head + struct.pack("<II", len(z), len(table)) + z)


def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def make_folders(base):
    shutil.rmtree(base, ignore_errors=True)
    os.makedirs(base)
    f = {}
    f["empty"] = os.path.join(base, "empty")
    os.makedirs(f["empty"])
    f["missing"] = os.path.join(base, "missing")
    f["relative"] = "relative/game"
    f["rdata-only"] = os.path.join(base, "rdata only")
    os.makedirs(f["rdata-only"])
    grf(os.path.join(f["rdata-only"], "rdata.grf"), NEEDED)
    f["empty-grf"] = os.path.join(base, "empty grf")
    os.makedirs(f["empty-grf"])
    grf(os.path.join(f["empty-grf"], "data.grf"))
    f["no-tables"] = os.path.join(base, "no tables")
    os.makedirs(f["no-tables"])
    grf(os.path.join(f["no-tables"], "data.grf"), NEEDED[2:])
    f["minimum"] = os.path.join(base, "minimum")
    os.makedirs(f["minimum"])
    grf(os.path.join(f["minimum"], "data.grf"))
    write(os.path.join(f["minimum"], NEEDED_LOOSE[0]), CLIENTINFO)
    write(os.path.join(f["minimum"], NEEDED_LOOSE[1]), b"Bifrost#\n")
    for loose in NEEDED_LOOSE[2:]:
        write(os.path.join(f["minimum"], loose), b"")
    return f


# name: (exit status, words the refusal must hold; None: the start must succeed)
CASES = [
    ("empty", 1, ["holds no data.grf"]),
    ("missing", 1, ["does not exist"]),
    ("relative", 1, ["full path", "is relative"]),
    ("rdata-only", 1, ["holds no data.grf", "DATA.INI"]),
    ("empty-grf", 1, ["data/sclientinfo.xml", "data/msgstringtable.txt", "poring.spr", "holds no files"]),
    ("no-tables", 1, ["data/sclientinfo.xml", "data/msgstringtable.txt"]),
    ("minimum", 0, None),
]


def run(client, base, name, folder):
    trace = os.path.join(base, name + ".trace")
    home = os.path.join(base, "home")
    os.makedirs(home, exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith("BF_")}
    env.update(BF_HOME=home, BF_CLIENT_DIR=folder, BF_MULTI_INSTANCE="1", BF_NO_ALERTS="1",
               BF_TRACE="err,mode", BF_TRACE_FILE=trace)
    try:
        r = subprocess.run([client, "--headless"], env=env, cwd=base, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=120)
        status = r.returncode
    except subprocess.TimeoutExpired:
        status = "timeout"
    try:
        with open(trace, "rb") as fp:
            lines = fp.read().decode("utf-8", "replace").splitlines()
    except OSError:
        lines = []
    return status, lines


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    client, base, app = sys.argv[1], os.path.abspath(sys.argv[2]), sys.argv[3]
    if not os.access(client, os.X_OK):
        print("skip: no client at %s" % client)
        return 77
    folders = make_folders(base)
    cases = list(CASES)
    real = os.environ.get("BF_CLIENT_DIR") or os.path.expanduser("~/Games/" + app)
    if os.path.exists(os.path.join(real, "data.grf")):
        folders["game-folder"] = real
        cases.append(("game-folder", 0, None))
    failures = 0
    for name, want, words in cases:
        status, lines = run(client, base, name, folders[name])
        crashed = [l for l in lines if "crash:" in l]
        refusal = next((l.split("start: game folder not usable: ", 1)[1] for l in lines
                        if "start: game folder not usable: " in l), "")
        if words is None:
            ok = status == 0 and not crashed and any("exit: WinMain returned 0" in l for l in lines) and not refusal
            detail = "started" if ok else "status %s, refusal %r, last line %r" % (status, refusal, lines[-1:] or "")
        else:
            missing = [w for w in words if w not in refusal]
            ok = (status == want and not crashed and not missing
                  and any("exit: start refused, status 1" in l for l in lines))
            detail = "status %s: %s" % (status, refusal) if ok else "status %s, refusal %r, lacks %s, crash %d" % (
                status, refusal, missing, len(crashed))
        print("%-4s %s: %s" % ("ok" if ok else "FAIL", name, detail))
        failures += not ok
    shutil.rmtree(base, ignore_errors=True)
    print("%s: %d failure(s)" % ("FAILED" if failures else "passed", failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
