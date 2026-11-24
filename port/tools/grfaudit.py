#!/usr/bin/env python3
"""Reports how much of a GRF archive this client can read.

  grfaudit.py ARCHIVE.grf [ARCHIVE.grf ...] [--folder DIR] [--list-unsupported]
              [--maps FILE] [--write-maps FILE]
              [--rathena-maps maps_athena.conf [--rathena-cache map_cache.dat ...]
               --write-server-conf FILE]

For every world, ground, attribute, model, sprite and action file it reads
the format version from the first bytes and compares it with the highest
version the client's loaders accept (src/resource/*.cpp):

  .rsw 2.1   .gnd 1.7   .gat 1.2   .rsm 1.5   .spr 2.1   .act 2.5

Files stored with the archive's old DES scheme are decoded as the client
does (src/base/CDec.cpp, src/base/GPak.cpp), so their versions are read too.

It also reports which of the text tables the client loads are present.
Archives are searched in the order given; the first one that has a file
wins, as in the client. A loose folder given with --folder (the game's
data/ folder, read first when clientinfo.xml has <readfolder/>) is searched
before them. Needs Python 3.6 and nothing else.

Maps. A map loads if its world, ground and attribute files are readable.
For each such map the world file's model list is followed: every model it
places must exist as model\\NAME (or under its ResNameTable.txt name, as
the resource manager retries) and be a .rsm of version 1.5 or lower. The
client skips a model it cannot load (GameWorld.cpp traces "model not found"
and goes on), so missing models do not stop a map from loading; they leave
holes. The rule for "enterable" is therefore:

  world, ground and attribute readable, and at most MAX_BAD_MODELS (10%)
  of the model instances the world file places are missing or unreadable.

  --list-unsupported   print each file that is too new
  --maps FILE          write one line per map: name, rsw/gnd/gat status,
                       model instances placed / missing / too new, verdict
  --write-maps FILE    write the maps the client can enter, one per line
                       (name, then the model counts as a comment)
  --rathena-maps FILE  rAthena's conf/maps_athena.conf (the server's map list)
  --write-server-conf FILE
                       write an rAthena map_conf import that removes every
                       map of that list the client cannot enter (delmap:)
  --rathena-cache FILE rAthena's map_cache.dat (repeat, pre-re first): also
                       remove maps whose cells differ from the server's by
                       more than MAX_LAYOUT_DIFF (1%); the client would walk
                       through walls the server knows, or the reverse
"""
import argparse, collections, os, re, struct, sys, zlib

LIMITS = {'rsw': (2, 1), 'gnd': (1, 7), 'gat': (1, 2), 'rsm': (1, 5), 'spr': (2, 1), 'act': (2, 5)}
MAGIC = {'rsw': b'GRSW', 'gnd': b'GRGN', 'gat': b'GRAT', 'rsm': b'GRSM', 'spr': b'SP', 'act': b'AC'}
TABLES = '''msgstringtable.txt mapnametable.txt mp3nametable.txt indoorrswtable.txt fogparametertable.txt
  idnum2itemdisplaynametable.txt idnum2itemdesctable.txt idnum2itemresnametable.txt itemslottable.txt
  itemslotcounttable.txt num2itemdisplaynametable.txt num2itemdesctable.txt num2itemresnametable.txt
  carditemnametable.txt num2cardillustnametable.txt cardprefixnametable.txt cardpostfixnametable.txt
  bookitemnametable.txt metalprocessitemlist.txt skillnametable.txt skilldesctable.txt
  leveluseskillspamount.txt manner.txt mapobjlighttable.txt viewpointtable.txt mappostable.txt
  resnametable.txt clientinfo.xml sclientinfo.xml'''.split()
MAX_BAD_MODELS = 0.10      # share of a map's model instances that may be missing
MAX_LAYOUT_DIFF = 0.01     # share of cells whose walkability may differ from the server's

DATA_COMPRESSED, DATA_ENCRYPTED, DATA_HEADER_ENCRYPTED = 1, 2, 4


class Entry:
    __slots__ = ('archive', 'packed', 'aligned', 'size', 'flags', 'offset', 'path')


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
        e.path = None
        e.packed, e.aligned, e.size, e.flags, e.offset = struct.unpack_from('<IIIBI', raw, end + 1)
        if e.flags & 1:
            entries[raw[at:end].lower()] = e
        at = end + 18
    return version, count - seed - 7, entries


def read_folder(root):
    """Loose files under root, keyed like archive names (data\\..., lower case)."""
    entries = {}
    base = os.path.dirname(os.path.abspath(root.rstrip('/')))
    for dirpath, _, names in os.walk(root):
        for n in names:
            full = os.path.join(dirpath, n)
            rel = os.path.relpath(full, base).replace(os.sep, '\\')
            e = Entry()
            e.archive = None
            e.path = full
            e.size = e.packed = e.aligned = os.path.getsize(full)
            e.flags = 1
            e.offset = 0
            entries[rel.encode('cp949', 'replace').lower()] = e
    return entries


# --- The archive's DES variant (src/base/CDec.cpp) ---------------------------
# CDec::MakeKey clears every key bit (it ANDs into a zeroed buffer), so all
# round keys are zero, and DataDecrypt runs a single round. A block is then
# IP, L ^= P(S(E(R))), FP. Bits are numbered from the top bit of byte 0.

_IP = (58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4, 62, 54, 46, 38, 30, 22, 14, 6,
       64, 56, 48, 40, 32, 24, 16, 8, 57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
       61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7)
_FP = (40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31, 38, 6, 46, 14, 54, 22, 62, 30,
       37, 5, 45, 13, 53, 21, 61, 29, 36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
       34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25)
_E = (32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
      16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1)
_P = (16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10,
      2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11, 4, 25)
_S = (
    (14, 0, 4, 15, 13, 7, 1, 4, 2, 14, 15, 2, 11, 13, 8, 1, 3, 10, 10, 6, 6, 12, 12, 11, 5, 9, 9, 5, 0, 3, 7, 8,
     4, 15, 1, 12, 14, 8, 8, 2, 13, 4, 6, 9, 2, 1, 11, 7, 15, 5, 12, 11, 9, 3, 7, 14, 3, 10, 10, 0, 5, 6, 0, 13),
    (15, 3, 1, 13, 8, 4, 14, 7, 6, 15, 11, 2, 3, 8, 4, 14, 9, 12, 7, 0, 2, 1, 13, 10, 12, 6, 0, 9, 5, 11, 10, 5,
     0, 13, 14, 8, 7, 10, 11, 1, 10, 3, 4, 15, 13, 4, 1, 2, 5, 11, 8, 6, 12, 7, 6, 12, 9, 0, 3, 5, 2, 14, 15, 9),
    (10, 13, 0, 7, 9, 0, 14, 9, 6, 3, 3, 4, 15, 6, 5, 10, 1, 2, 13, 8, 12, 5, 7, 14, 11, 12, 4, 11, 2, 15, 8, 1,
     13, 1, 6, 10, 4, 13, 9, 0, 8, 6, 15, 9, 3, 8, 0, 7, 11, 4, 1, 15, 2, 14, 12, 3, 5, 11, 10, 5, 14, 2, 7, 12),
    (7, 13, 13, 8, 14, 11, 3, 5, 0, 6, 6, 15, 9, 0, 10, 3, 1, 4, 2, 7, 8, 2, 5, 12, 11, 1, 12, 10, 4, 14, 15, 9,
     10, 3, 6, 15, 9, 0, 0, 6, 12, 10, 11, 1, 7, 13, 13, 8, 15, 9, 1, 4, 3, 5, 14, 11, 5, 12, 2, 7, 8, 2, 4, 14),
    (2, 14, 12, 11, 4, 2, 1, 12, 7, 4, 10, 7, 11, 13, 6, 1, 8, 5, 5, 0, 3, 15, 15, 10, 13, 3, 0, 9, 14, 8, 9, 6,
     4, 11, 2, 8, 1, 12, 11, 7, 10, 1, 13, 14, 7, 2, 8, 13, 15, 6, 9, 15, 12, 0, 5, 9, 6, 10, 3, 4, 0, 5, 14, 3),
    (12, 10, 1, 15, 10, 4, 15, 2, 9, 7, 2, 12, 6, 9, 8, 5, 0, 6, 13, 1, 3, 13, 4, 14, 14, 0, 7, 11, 5, 3, 11, 8,
     9, 4, 14, 3, 15, 2, 5, 12, 2, 9, 8, 5, 12, 15, 3, 10, 7, 11, 0, 14, 4, 1, 10, 7, 1, 6, 13, 0, 11, 8, 6, 13),
    (4, 13, 11, 0, 2, 11, 14, 7, 15, 4, 0, 9, 8, 1, 13, 10, 3, 14, 12, 3, 9, 5, 7, 12, 5, 2, 10, 15, 6, 8, 1, 6,
     1, 6, 4, 11, 11, 13, 13, 8, 12, 1, 3, 4, 7, 10, 14, 7, 10, 9, 15, 5, 6, 0, 8, 15, 0, 14, 5, 2, 9, 3, 2, 12),
    (13, 1, 2, 15, 8, 13, 4, 8, 6, 10, 15, 3, 11, 7, 1, 4, 10, 12, 9, 5, 3, 6, 14, 11, 5, 0, 0, 14, 12, 9, 7, 2,
     7, 2, 11, 1, 4, 14, 1, 7, 9, 4, 12, 10, 14, 8, 2, 13, 0, 15, 6, 12, 10, 9, 13, 0, 15, 3, 3, 5, 5, 6, 8, 11))


def _permutation_tables(table, width):
    """Per input byte position and value, the output bits it sets."""
    out = [[0] * 256 for _ in range(8)]
    for i, src in enumerate(table):
        src -= 1
        bit = 1 << (width - 1 - i)
        pos, m = src >> 3, 0x80 >> (src & 7)
        for v in range(256):
            if v & m:
                out[pos][v] |= bit
    return out


_IPT = _permutation_tables(_IP, 64)
_FPT = _permutation_tables(_FP, 64)
# S-box output g (4 bits at a-bit 4g) through P, as a 32-bit word.
_SP = []
for _g in range(8):
    row = []
    for _v in range(64):
        a = _S[_g][_v] << (28 - 4 * _g)
        t = 0
        for _i, _src in enumerate(_P):
            if a >> (31 - (_src - 1)) & 1:
                t |= 1 << (31 - _i)
        row.append(t)
    _SP.append(row)
_EBITS = [[_E[6 * g + j] - 1 for j in range(6)] for g in range(8)]   # bit of R, 0 = top


def _des_block(b):
    x = 0
    for p in range(8):
        x |= _IPT[p][b[p]]
    r = x & 0xffffffff
    f = 0
    for g in range(8):
        six = 0
        for k in _EBITS[g]:
            six = (six << 1) | (r >> (31 - k) & 1)
        f |= _SP[g][six]
    x ^= f << 32
    y = 0
    for p in range(8):
        y |= _FPT[p][(x >> (56 - 8 * p)) & 0xff]
    return y.to_bytes(8, 'big')


_SWAP = {0: 43, 43: 0, 185: 192, 192: 185, 254: 235, 235: 254, 108: 128, 128: 108,
         1: 104, 104: 1, 119: 72, 72: 119, 96: 255, 255: 96}


def _decrypt(data, packed, header_only):
    out = bytearray(data)
    n = len(data) // 8
    if header_only:
        for i in range(min(20, n)):
            out[8 * i:8 * i + 8] = _des_block(data[8 * i:8 * i + 8])
        return bytes(out)
    mod = len(str(packed)) if packed else 1
    mod = 1 if mod < 3 else mod + 1 if mod < 5 else mod + 9 if mod < 7 else mod + 15
    k = 0
    for i in range(n):
        s = data[8 * i:8 * i + 8]
        if i < 20 or i % mod == 0:
            out[8 * i:8 * i + 8] = _des_block(s)
        else:
            if k == 7:
                o = bytearray(8)
                o[0:2] = s[3:5]
                o[3:6] = s[0:3]
                o[2] = s[6]
                o[6] = s[5]
                o[7] = _SWAP.get(s[7], s[7])
                out[8 * i:8 * i + 8] = o
                k = 0
            k += 1
    return bytes(out)


def raw_bytes(e, limit=None):
    """The stored bytes of an entry, decrypted, still compressed. limit cuts them short."""
    if e.archive is None:
        return None
    n = e.aligned if limit is None else min(e.aligned, limit)
    e.archive.seek(46 + e.offset)
    data = e.archive.read(n)
    if e.flags & (DATA_ENCRYPTED | DATA_HEADER_ENCRYPTED):
        data = _decrypt(data[:len(data) // 8 * 8], e.packed, not (e.flags & DATA_ENCRYPTED))
    return data


def read_file(e):
    """The whole content of an entry, or None if it does not decompress."""
    if e.archive is None:
        return open(e.path, 'rb').read()
    try:
        return zlib.decompress(raw_bytes(e)[:e.packed])
    except zlib.error:
        return None


def head(e, want=16):
    """The first bytes of a file ('' if they do not decompress)."""
    if e.archive is None:
        with open(e.path, 'rb') as f:
            return f.read(want)
    data = raw_bytes(e, 512)
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


def parse_rsw(data):
    """What C3dWorldRes::Load reads from a world file that matters here.

    Returns (ground name, attribute name, model names placed, one per
    instance); attribute name is None before version 1.4, the model list None
    if the object list does not parse."""
    if not data.startswith(b'GRSW') or len(data) < 6:
        return None
    ma, mi = data[4], data[5]
    v = (ma, mi)
    if v > LIMITS['rsw']:
        return None
    name = lambda at: data[at:at + 40].split(b'\0', 1)[0]
    at = 6 + 40
    gnd, at = name(at), at + 40
    gat = None
    if v >= (1, 4):
        gat, at = name(at), at + 40
    at += 40
    if v >= (1, 3): at += 4
    if v >= (1, 8): at += 16
    if v >= (1, 9): at += 4
    if v >= (1, 5): at += 8 + 24
    if v >= (1, 7): at += 4
    if v >= (1, 6): at += 16
    try:
        count, = struct.unpack_from('<i', data, at)
        at += 4
        models = []
        for _ in range(count):
            kind, = struct.unpack_from('<i', data, at)
            at += 4
            if kind == 1:
                if v >= (1, 3):
                    at += 40 + 12
                models.append(data[at:at + 80].split(b'\0', 1)[0])
                at += 196
            elif kind == 2:
                at += 108
            elif kind == 3:
                at += 192 if ma >= 2 else 188
            elif kind == 4:
                at += 116
            else:
                models = None                   # the client asserts and misreads from here
                break
    except struct.error:
        models = None
    return gnd, gat, models


def resolve(name, folder, files, resname):
    """The archive key the resource manager ends up loading for name, or None.

    CResMgr::Get tries the name, then its ResNameTable.txt replacement."""
    for n in (name.lower(), resname.get(name.lower())):
        if n is not None and b'data\\' + folder + n in files:
            return b'data\\' + folder + n
    return None


def read_resnametable(files):
    e = files.get(b'data\\resnametable.txt')
    table = {}
    if not e:
        return table
    data = read_file(e) or b''
    # CResMgr::ReadResNameTable: '#' ends a field, pairs a -> b, CR/LF reset.
    a = None
    for line in data.split(b'\n'):
        fields = line.rstrip(b'\r').split(b'#')[:-1]
        for f in fields:
            if a is None:
                a = f
            else:
                table[a.lower()] = f.lower()
                a = None
    return table


def model_status(name, files, status, resname):
    """'ok', 'new' (too new), 'rsm2', 'unreadable' or 'missing' for one model reference."""
    key = resolve(name, b'model\\', files, resname)
    if key is None:
        if name.lower().endswith(b'.rsm') and resolve(name + b'2', b'model\\', files, resname):
            return 'rsm2'
        return 'missing'
    if key.endswith(b'.rsm2'):
        return 'rsm2'
    return {'ok': 'ok', 'new': 'new'}.get(status.get(key), 'unreadable')


def read_map_cache(path):
    """rAthena's db/.../map_cache.dat: name -> (width, height, cell types)."""
    d = open(path, 'rb').read()
    _, count = struct.unpack_from('<IH', d, 0)
    at, out = 8, {}
    for _ in range(count):
        name, xs, ys, n = struct.unpack_from('<12shhi', d, at)
        out.setdefault(name.split(b'\0', 1)[0].decode('latin-1').lower(), (xs, ys, d[at + 20:at + 20 + n]))
        at += 20 + n
    return out


def layout_differs(gat, cached):
    """Share of cells whose walkability differs between a .gat and the
    server's cache entry (1.0 if the sizes differ). Types 1 and 5 block
    walking; 0 and 3 (water, which rAthena derives from height) do not."""
    w, h = struct.unpack_from('<ii', gat, 6)
    xs, ys, packed = cached
    if (w, h) != (xs, ys):
        return 1.0
    cells = zlib.decompress(packed)
    types = gat[14 + 16:14 + 20 * w * h:20]
    blocked = lambda t: t in (1, 5)
    diff = sum(1 for a, b in zip(types, cells) if blocked(a) != blocked(b))
    return diff / (w * h)


def read_rathena_maps(path):
    maps = []
    for line in open(path, encoding='latin-1'):
        m = re.match(r'\s*map\s*:\s*(\S+)', line)
        if m:
            maps.append(m.group(1).lower())
    return maps


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('archives', nargs='+')
    ap.add_argument('--folder', help='loose data folder searched before the archives')
    ap.add_argument('--list-unsupported', action='store_true')
    ap.add_argument('--maps')
    ap.add_argument('--write-maps')
    ap.add_argument('--rathena-maps')
    ap.add_argument('--write-server-conf')
    ap.add_argument('--rathena-cache', action='append', default=[],
                    help='rAthena map_cache.dat, in the server\'s search order (pre-re first); '
                         'maps whose cells differ from it by more than 1%% are removed too')
    args = ap.parse_args()
    if args.write_server_conf and not args.rathena_maps:
        ap.error('--write-server-conf needs --rathena-maps')

    files = {}
    for path in reversed(args.archives):          # earlier archives override later ones
        version, count, entries = read_table(path)
        print(f'{path}: version 0x{version:x}, {len(entries)} files')
        files.update(entries)
    if args.folder:
        loose = read_folder(args.folder)
        print(f'{args.folder}: loose folder, {len(loose)} files, searched first')
        files.update(loose)
    print(f'combined: {len(files)} files')

    by_ext = collections.Counter(name.rsplit(b'.', 1)[-1].decode('latin-1') if b'.' in name else '' for name in files)
    print('by extension:', ', '.join(f'{ext or "(none)"} {n}' for ext, n in by_ext.most_common(18)))

    versions = {kind: collections.Counter() for kind in LIMITS}
    des = collections.Counter()
    status = {}          # name -> 'ok' | 'new' | 'unreadable'
    for name, e in files.items():
        kind = name.rsplit(b'.', 1)[-1].decode('latin-1')
        if kind not in LIMITS:
            continue
        if e.flags & (DATA_ENCRYPTED | DATA_HEADER_ENCRYPTED):
            des[kind] += 1
        v = version_of(kind, head(e))
        if v is None:
            versions[kind]['unreadable'] += 1; status[name] = 'unreadable'; continue
        versions[kind][v] += 1
        status[name] = 'ok' if v <= LIMITS[kind] else 'new'
        if status[name] == 'new' and args.list_unsupported:
            print(f'  too new: {name.decode("cp949", "replace")} {v[0]}.{v[1]}')

    print()
    print(f'{"kind":5} {"limit":6} {"files":>7} {"readable":>9} {"too new":>8} {"unreadable":>11} {"DES":>6}  versions')
    for kind, limit in LIMITS.items():
        c = versions[kind]
        total = sum(c.values())
        new = sum(n for v, n in c.items() if isinstance(v, tuple) and v > limit)
        ok = sum(n for v, n in c.items() if isinstance(v, tuple) and v <= limit)
        spread = ' '.join(f'{v[0]}.{v[1]}:{n}' for v, n in sorted((v, n) for v, n in c.items() if isinstance(v, tuple)))
        print(f'{kind:5} {limit[0]}.{limit[1]:<4} {total:7} {ok:9} {new:8} {c["unreadable"]:11} {des[kind]:6}  {spread}')
    print('(DES: stored with the archive\'s old encryption, decoded here as the client does)')

    # Maps: a map loads if its world file and the ground and attribute files
    # it names are readable (GameWorld.cpp); every name goes through the
    # resource manager, so ResNameTable.txt aliases count (pvp_y_1-1.rsw is
    # prontera.rsw). It is enterable if, in addition, few enough of its models
    # are missing.
    resname = read_resnametable(files)
    names = {n[5:-4] for n in files if n.endswith(b'.rsw') and n.count(b'\\') == 1}
    names |= {n[:-4] for n in resname if n.endswith(b'.rsw') and b'\\' not in n}
    server = read_rathena_maps(args.rathena_maps) if args.rathena_maps else []
    names |= {m.encode() for m in server}
    worlds = sum(1 for n in files if n.endswith(b'.rsw'))
    loads_n = enterable_n = 0
    lines, enterable, model_totals = [], [], collections.Counter()
    bad_refs = collections.Counter()
    verdicts = {}
    gat_key = {}
    for base in sorted(names):
        mapname = base.decode('cp949', 'replace')
        counts = collections.Counter()
        rsw = resolve(base + b'.rsw', b'', files, resname)
        parts = [status.get(rsw, 'missing') if rsw else 'missing', 'missing', 'missing']
        models = None
        if parts[0] == 'ok':
            parsed = parse_rsw(read_file(files[rsw]) or b'')
            if parsed is None:
                parts[0] = 'unreadable'
            else:
                gnd, gat, models = parsed
                for i, n in ((1, gnd), (2, gat)):
                    key = resolve(n, b'', files, resname) if n else None
                    parts[i] = status.get(key, 'missing') if key else 'missing'
                    if i == 2:
                        gat_key[mapname] = key
        if all(p == 'missing' for p in parts):
            verdicts[mapname] = 'absent'
            lines.append(f'{mapname:24} no world file {"":104} absent')
            continue
        loads = all(p == 'ok' for p in parts)
        if loads:
            if models is None:
                counts['objlist'] += 1
            else:
                for m in models:
                    s = model_status(m, files, status, resname)
                    counts[s] += 1
                    if s != 'ok':
                        bad_refs[(s, m)] += 1
        placed = sum(counts[k] for k in ('ok', 'new', 'rsm2', 'unreadable', 'missing'))
        bad = placed - counts['ok']
        ok = loads and not counts['objlist'] and bad <= MAX_BAD_MODELS * placed
        loads_n += loads
        enterable_n += ok
        model_totals.update(counts)
        verdict = 'ENTER' if ok else 'models' if loads else 'FAILS'
        verdicts[mapname] = verdict
        world = rsw[5:].decode('cp949', 'replace') if rsw else '-'
        detail = (f'models={placed} missing={counts["missing"]} new={counts["new"]} '
                  f'rsm2={counts["rsm2"]} unreadable={counts["unreadable"]}'
                  + (' objlist=bad' if counts['objlist'] else ''))
        lines.append(f'{mapname:24} {world:20} rsw={parts[0]:10} gnd={parts[1]:10} gat={parts[2]:10} {detail:50} {verdict}')
        if ok:
            enterable.append((mapname, world, placed, bad))

    print(f'\nmaps: {worlds} world files, {len(names)} map names (with ResNameTable aliases'
          f'{" and the server list" if server else ""}); {loads_n} have readable world, ground '
          f'and attribute files, {sum(v == "FAILS" for v in verdicts.values())} do not, '
          f'{sum(v == "absent" for v in verdicts.values())} have no world file')
    print(f'  model instances on those maps: {sum(model_totals[k] for k in ("ok", "new", "rsm2", "unreadable", "missing"))}, '
          f'readable {model_totals["ok"]}, missing {model_totals["missing"]}, too new {model_totals["new"]}, '
          f'.rsm2 {model_totals["rsm2"]}, unreadable {model_totals["unreadable"]}; '
          f'{model_totals["objlist"]} world files whose object list does not parse')
    print(f'  enterable (at most {MAX_BAD_MODELS:.0%} of model instances missing or unreadable): {enterable_n}')
    if bad_refs:
        print(f'  {len(bad_refs)} distinct model references not loadable; most placed:')
        for (s, m), n in bad_refs.most_common(8):
            print(f'    {n:5} {s:10} {m.decode("cp949", "replace")}')
    if args.maps:
        open(args.maps, 'w').write('\n'.join(lines) + '\n')
    if args.write_maps:
        with open(args.write_maps, 'w') as f:
            f.write(f'# Maps the client can enter: {enterable_n} of {len(names)} map names.\n'
                    f'# Written by port/tools/grfaudit.py {" ".join(os.path.basename(a) for a in args.archives)}'
                    f'{" --folder data" if args.folder else ""}.\n'
                    f'# Rule: world, ground, attribute readable; at most {MAX_BAD_MODELS:.0%} of the model\n'
                    f'# instances missing or unreadable (the client skips those models).\n')
            for name, world, placed, bad in enterable:
                alias = '' if world == name + '.rsw' else f', world file {world}'
                f.write(f'{name:24} # models {placed}, not loadable {bad}{alias}\n')

    if args.rathena_maps:
        can = {e[0] for e in enterable}
        if args.rathena_cache:
            caches = [read_map_cache(p) for p in args.rathena_cache]
            minor = []
            for m in server:
                cached = next((c[m] for c in caches if m in c), None)
                if m not in can or cached is None or not gat_key.get(m):
                    continue
                share = layout_differs(read_file(files[gat_key[m]]) or b'', cached)
                if share > MAX_LAYOUT_DIFF:
                    can.discard(m)
                    verdicts[m] = f'layout ({share:.1%} of cells differ from the server\'s map cache)' \
                        if share < 1 else 'layout (size differs from the server\'s map cache)'
                elif share > 0:
                    minor.append((m, share))
            print(f'server map caches {" ".join(args.rathena_cache)}: '
                  f'{sum(v.startswith("layout") for v in verdicts.values())} enterable maps removed for a different layout, '
                  f'{len(minor)} kept with up to {MAX_LAYOUT_DIFF:.0%} of cells different'
                  + (f' (largest: {max(minor, key=lambda x: x[1])[0]} {max(x[1] for x in minor):.2%})' if minor else ''))
        drop = [m for m in server if m not in can]
        keep = [m for m in server if m in can]
        print(f'\nserver map list {args.rathena_maps}: {len(server)} maps, {len(keep)} enterable, {len(drop)} removed')
        if args.write_server_conf:
            with open(args.write_server_conf, 'w') as f:
                f.write('// Generated by port/tools/grfaudit.py --write-server-conf; do not edit.\n'
                        '// Removes from the test server every map the client cannot enter, so that\n'
                        '// nothing warps a character there. Imported from map_conf.txt, which rAthena\n'
                        '// reads after conf/maps_athena.conf.\n'
                        f'// {len(server)} maps in the server list, {len(keep)} kept, {len(drop)} removed.\n'
                        '// Reason per line: FAILS = world, ground or attribute file unreadable or too new;\n'
                        '// models = too many models missing; absent = no world file in the archives;\n'
                        '// layout = the client\'s ground differs from the server\'s map cache.\n')
                for m in drop:
                    f.write(f'delmap: {m}\t// {verdicts.get(m, "absent")}\n')


if __name__ == '__main__':
    main()
