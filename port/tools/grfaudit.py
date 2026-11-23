#!/usr/bin/env python3
"""Reports how much of a GRF archive this client can read.

  grfaudit.py ARCHIVE.grf [ARCHIVE.grf ...] [--list-unsupported] [--maps FILE]

For every world, ground, attribute, model, sprite and action file it reads
the format version from the first bytes and compares it with the highest
version the client's loaders accept (src/resource/*.cpp):

  .rsw 2.1   .gnd 1.7   .gat 1.2   .rsm 1.5   .spr 2.1   .act 2.5

It also reports which of the text tables the client loads are present.
Archives are searched in the order given; the first one that has a file
wins, as in the client. Needs Python 3.6 and nothing else.

  --list-unsupported   print each file that is too new
  --maps FILE          write one line per map: name, rsw/gnd/gat versions,
                       number of its models that are too new or missing
"""
import argparse, collections, re, struct, sys, zlib

LIMITS = {'rsw': (2, 1), 'gnd': (1, 7), 'gat': (1, 2), 'rsm': (1, 5), 'spr': (2, 1), 'act': (2, 5)}
MAGIC = {'rsw': b'GRSW', 'gnd': b'GRGN', 'gat': b'GRAT', 'rsm': b'GRSM', 'spr': b'SP', 'act': b'AC'}
TABLES = '''msgstringtable.txt mapnametable.txt mp3nametable.txt indoorrswtable.txt fogparametertable.txt
  idnum2itemdisplaynametable.txt idnum2itemdesctable.txt idnum2itemresnametable.txt itemslottable.txt
  itemslotcounttable.txt num2itemdisplaynametable.txt num2itemdesctable.txt num2itemresnametable.txt
  carditemnametable.txt num2cardillustnametable.txt cardprefixnametable.txt cardpostfixnametable.txt
  bookitemnametable.txt metalprocessitemlist.txt skillnametable.txt skilldesctable.txt
  leveluseskillspamount.txt manner.txt mapobjlighttable.txt viewpointtable.txt mappostable.txt
  resnametable.txt clientinfo.xml sclientinfo.xml'''.split()

class Entry:
    __slots__ = ('archive', 'packed', 'aligned', 'size', 'flags', 'offset')

def read_table(path):
    f = open(path, 'rb')
    head = f.read(46)
    if head[:15] != b'Master of Magic':
        sys.exit(f'{path}: not a GRF archive')
    table_offset, seed, count, version = struct.unpack('<IIII', head[30:46])
    if version >> 8 != 2:
        sys.exit(f'{path}: archive version 0x{version:x}; this tool reads 0x200 only')
    f.seek(46 + table_offset)
    packed, _ = struct.unpack('<II', f.read(8))
    raw = zlib.decompress(f.read(packed))
    entries, at = {}, 0
    while at < len(raw):
        end = raw.index(b'\0', at)
        e = Entry()
        e.archive = f
        e.packed, e.aligned, e.size, e.flags, e.offset = struct.unpack_from('<IIIBI', raw, end + 1)
        if e.flags & 1:
            entries[raw[at:end].lower()] = e
        at = end + 18
    return version, count - seed - 7, entries

def head(e, want=16):
    """The first bytes of a file, or None if it is DES-encrypted."""
    if e.flags & 6:
        return None
    e.archive.seek(46 + e.offset)
    data = e.archive.read(min(e.aligned, 512))
    try:
        return zlib.decompressobj().decompress(data, want)
    except zlib.error:
        return b''

def version_of(kind, first):
    magic = MAGIC[kind]
    if not first.startswith(magic) or len(first) < len(magic) + 2:
        return None
    a, b = first[len(magic)], first[len(magic) + 1]
    return (b, a) if kind in ('spr', 'act') else (a, b)   # sprites store minor first

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('archives', nargs='+')
    ap.add_argument('--list-unsupported', action='store_true')
    ap.add_argument('--maps')
    args = ap.parse_args()

    files = {}
    for path in reversed(args.archives):          # earlier archives override later ones
        version, count, entries = read_table(path)
        print(f'{path}: version 0x{version:x}, {len(entries)} files')
        files.update(entries)
    print(f'combined: {len(files)} files')

    by_ext = collections.Counter(name.rsplit(b'.', 1)[-1].decode('latin-1') if b'.' in name else '' for name in files)
    print('by extension:', ', '.join(f'{ext or "(none)"} {n}' for ext, n in by_ext.most_common(18)))

    versions = {kind: collections.Counter() for kind in LIMITS}
    status = {}          # name -> 'ok' | 'new' | 'encrypted' | 'unreadable'
    for name, e in files.items():
        kind = name.rsplit(b'.', 1)[-1].decode('latin-1')
        if kind not in LIMITS:
            continue
        first = head(e)
        if first is None:
            versions[kind]['encrypted'] += 1; status[name] = 'encrypted'; continue
        v = version_of(kind, first)
        if v is None:
            versions[kind]['unreadable'] += 1; status[name] = 'unreadable'; continue
        versions[kind][v] += 1
        status[name] = 'ok' if v <= LIMITS[kind] else 'new'
        if status[name] == 'new' and args.list_unsupported:
            print(f'  too new: {name.decode("cp949", "replace")} {v[0]}.{v[1]}')

    print()
    print(f'{"kind":5} {"limit":6} {"files":>7} {"readable":>9} {"too new":>8} {"encrypted":>10}  versions')
    for kind, limit in LIMITS.items():
        c = versions[kind]
        total = sum(c.values())
        new = sum(n for v, n in c.items() if isinstance(v, tuple) and v > limit)
        ok = sum(n for v, n in c.items() if isinstance(v, tuple) and v <= limit)
        spread = ' '.join(f'{v[0]}.{v[1]}:{n}' for v, n in sorted((v, n) for v, n in c.items() if isinstance(v, tuple)))
        print(f'{kind:5} {limit[0]}.{limit[1]:<4} {total:7} {ok:9} {new:8} {c["encrypted"]:10}  {spread}')
    print('(encrypted files use the archive\'s old DES scheme; they predate every format change)')

    # Maps: a map loads if its world, ground and attribute files are readable.
    worlds = sorted(n for n in files if n.endswith(b'.rsw'))
    good = bad = 0
    lines = []
    for w in worlds:
        base = w[:-4]
        parts = [status.get(base + ext, 'missing') for ext in (b'.rsw', b'.gnd', b'.gat')]
        loads = all(p in ('ok', 'encrypted') for p in parts)
        good += loads; bad += not loads
        lines.append(f'{w.decode("cp949", "replace"):40} rsw={parts[0]:9} gnd={parts[1]:9} gat={parts[2]:9} {"loads" if loads else "FAILS"}')
    print(f'\nmaps: {len(worlds)} world files; {good} have readable world, ground and attribute files, {bad} do not')
    if args.maps:
        open(args.maps, 'w').write('\n'.join(lines) + '\n')

    present = [t for t in TABLES if (b'data\\' + t.encode()) in files]
    missing = [t for t in TABLES if t not in present]
    print(f'\ntext tables the client loads: {len(present)} of {len(TABLES)} present')
    if missing:
        print('  missing:', ' '.join(missing))

if __name__ == '__main__':
    main()
