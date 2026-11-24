#!/usr/bin/env python3
"""Builds the game folder's loose data/ folder from the archives' gaps.

  port/data/assemble.py [--game DIR] [--roe DIR] [--check]

The client reads data/ before the archives (clientinfo.xml has <readfolder>).
What goes there, and why:

  sclientinfo.xml   generated: the file WinMain.cpp opens. English service
                    (servicetype america), the local test server, readfolder.
                    The archive's own clientinfo.xml (kRO, Korean) is left
                    alone; the client does not open it.
  12 text tables    the tables the client loads that the 2021 archive no
                    longer has (kRO moved them to Lua), copied unchanged from
                    ROenglishRE: the pre-renewal tables for pre-2012 clients
                    where it has them, else its base (Renewal) tables, which
                    the pre-renewal set overlays.
  8 maps            prontera, prt_fild05, prt_fild08, prt_in, prt_church,
                    izlude, alberta, morocc: the 2021 archive holds their
                    renewal layouts, but the test server (rAthena pre-renewal)
                    walks them on its db/pre-re/map_cache.dat, which is the
                    older layout. ROenglishRE's Pre-Renewal/data holds exactly
                    those eight maps in that layout (their cells match the
                    server's cache), so the client draws what the server walks.

Every file is copied as it is; translation tables never go into git. A table
fault that tablecheck.py finds is repaired here, never in ROenglishRE, and
each repair is listed in REPAIRS below with its reason. Missing message ids
get the placeholder "?msg N" (spec: missing data shows an obvious
placeholder).

The script is idempotent: a file whose content is already right is left
alone, and it removes nothing it did not write. --check only reports.
"""
import argparse, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))

TRANS = 'Translation'
PRE_COMPAT = os.path.join(TRANS, 'Compatibility', '2012-04-10', 'Pre-Renewal', 'data')
PRE = os.path.join(TRANS, 'Pre-Renewal', 'data')
BASE = os.path.join(TRANS, 'Renewal', 'data')
ADD = os.path.join('Additions', 'data')

# file in data/  <-  file in ROenglishRE
TABLES = [
    ('msgstringtable.txt', os.path.join(BASE, 'msgstringtable.txt')),
    ('num2itemdisplaynametable.txt', os.path.join(PRE_COMPAT, 'num2itemdisplaynametable.txt')),
    ('num2itemdesctable.txt', os.path.join(PRE_COMPAT, 'num2itemdesctable.txt')),
    ('num2itemresnametable.txt', os.path.join(PRE_COMPAT, 'num2itemresnametable.txt')),
    ('idnum2itemdisplaynametable.txt', os.path.join(PRE_COMPAT, 'idnum2itemdisplaynametable.txt')),
    ('idnum2itemdesctable.txt', os.path.join(PRE_COMPAT, 'idnum2itemdesctable.txt')),
    ('idnum2itemresnametable.txt', os.path.join(PRE_COMPAT, 'idnum2itemresnametable.txt')),
    ('itemslotcounttable.txt', os.path.join(PRE_COMPAT, 'itemslotcounttable.txt')),
    ('skillnametable.txt', os.path.join(PRE, 'skillnametable.txt')),
    ('skilldesctable.txt', os.path.join(PRE, 'skilldesctable.txt')),
    ('leveluseskillspamount.txt', os.path.join(ADD, 'leveluseskillspamount.txt')),
]
PRE_MAPS = ['alberta', 'izlude', 'morocc', 'prontera', 'prt_church', 'prt_fild05', 'prt_fild08', 'prt_in']
MAPS = [(m + ext, os.path.join(PRE, m + ext)) for m in PRE_MAPS for ext in ('.rsw', '.gnd', '.gat')]




# Repairs: data/ name -> (function bytes -> bytes, reason). Each answers a
# fault tablecheck.py reports on the file as ROenglishRE has it.
REPAIRS = {}

CLIENTINFO = '''<?xml version="1.0" encoding="windows-1252" ?>
<clientinfo>
<desc>Bifrost: local test server</desc>
<servicetype>america</servicetype>
<servertype>primary</servertype>
<readfolder></readfolder>
<connection>
<display>Local</display>
<address>{address}</address>
<port>{port}</port>
<version>20</version>
<registrationweb>http://127.0.0.1/</registrationweb>
</connection>
</clientinfo>
'''


def sha(data):
    return hashlib.sha256(data).hexdigest()[:16]


def place(dst, data, check, report, source):
    old = open(dst, 'rb').read() if os.path.exists(dst) else None
    if old == data:
        state = 'same'
    elif check:
        state = 'MISSING' if old is None else 'DIFFERS'
    else:
        tmp = dst + '.part'
        with open(tmp, 'wb') as f:
            f.write(data)
        os.replace(tmp, dst)
        state = 'written' if old is None else 'updated'
    report.append((state, os.path.basename(dst), len(data), sha(data), source))
    return state


def main():
    game = os.environ.get('BF_CLIENT_DIR') or os.path.expanduser('~/Games/Bifrost')
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game', default=game, help='game folder (default $BF_CLIENT_DIR or ~/Games/Bifrost)')
    ap.add_argument('--roe', help='ROenglishRE checkout (default GAME/sources/ROenglishRE)')
    ap.add_argument('--address', default=os.environ.get('BF_SERVER_ADDRESS', '127.0.0.1'))
    ap.add_argument('--port', default=os.environ.get('BF_LOGIN_PORT', '6900'))
    ap.add_argument('--check', action='store_true', help='report what would change, write nothing')
    args = ap.parse_args()
    roe = args.roe or os.path.join(args.game, 'sources', 'ROenglishRE')
    out = os.path.join(args.game, 'data')
    if not os.path.isdir(roe):
        sys.exit(f'{roe}: no ROenglishRE checkout (port/data/fetch.sh copies it)')
    if not args.check:
        os.makedirs(out, exist_ok=True)

    try:
        rev = subprocess.run(['git', '-C', roe, 'rev-parse', '--short=12', 'HEAD'],
                             capture_output=True, text=True).stdout.strip() or 'unknown'
    except OSError:
        rev = 'unknown'
    print(f'ROenglishRE {roe} at {rev}; writing {out}{" (check only)" if args.check else ""}')

    report, missing = [], []
    for name, src in TABLES + MAPS:
        path = os.path.join(roe, src)
        if not os.path.exists(path):
            missing.append(src)
            continue
        data = open(path, 'rb').read()
        source = 'ROenglishRE/' + src
        if name in REPAIRS:
            fn, why = REPAIRS[name]
            fixed = fn(data)
            if fixed != data:
                source += f' (repaired: {why})'
            data = fixed
        place(os.path.join(out, name), data, args.check, report, source)
    info = CLIENTINFO.format(address=args.address, port=args.port).replace('\n', '\r\n').encode('latin-1')
    place(os.path.join(out, 'sclientinfo.xml'), info, args.check, report, 'generated by port/data/assemble.py')

    for state, name, size, digest, source in report:
        print(f'  {state:8} {name:32} {size:9} {digest}  {source}')
    for src in missing:
        print(f'  MISSING SOURCE {src}')
    bad = missing or (args.check and any(r[0] != 'same' for r in report))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
