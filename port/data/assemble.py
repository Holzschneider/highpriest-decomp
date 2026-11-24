#!/usr/bin/env python3
"""Builds the game folder's loose data/ folder from the archives' gaps.

  port/data/assemble.py [--game DIR] [--roe DIR] [--check] [--list]

The client reads data/ before the archives (clientinfo.xml has <readfolder>).
What goes there, and why:

  sclientinfo.xml   generated: the file WinMain.cpp opens. English service
                    (servicetype america), the local test server, readfolder.
                    The archive's own clientinfo.xml (kRO, Korean) is left
                    alone; the client does not open it.
  17 text tables    the tables the client loads that the 2021 archive no
                    longer has (kRO moved them to Lua), and the displayed
                    tables it holds only in Korean (map names, card name
                    prefixes and postfixes, forge materials, skill shouts),
                    copied unchanged from ROenglishRE: the pre-renewal tables
                    for pre-2012 clients where it has them, else its base
                    (Renewal) tables, which the pre-renewal set overlays.
  bf_strings.tsv    the client's own translation table for displayed literals
                    (port/data/strings/translation.tsv, written by
                    port/tools/strings.py), copied unchanged; the lookup in
                    port/native/strings reads it, so a translation changes
                    without a rebuild.
  textures          ROenglishRE's English interface bitmaps, under the
                    client's own folder names (data/texture/유저인터페이스/...),
                    each in the version whose image size equals the
                    archive file's (see TEXTURE_SETS); --list names the
                    source of every file and why others were left out.
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
import argparse, hashlib, os, re, subprocess, sys

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
    # Displayed tables the archive holds in Korean (M9.5, found by the coverage
    # report): map names (party window, minimap, warp lists), the card name
    # prefixes and postfixes of item names, the forge window's materials, the
    # Dancer's and Bard's skill shouts. ROenglishRE has them only in its base
    # set; the files are plain ASCII in the 2008 formats (tablecheck.py).
    ('mapnametable.txt', os.path.join(BASE, 'mapnametable.txt')),
    ('cardprefixnametable.txt', os.path.join(BASE, 'cardprefixnametable.txt')),
    ('cardpostfixnametable.txt', os.path.join(ADD, 'cardpostfixnametable.txt')),
    ('metalprocessitemlist.txt', os.path.join(BASE, 'metalprocessitemlist.txt')),
    ('dc_scream.txt', os.path.join(BASE, 'dc_scream.txt')),
    ('ba_frostjoke.txt', os.path.join(BASE, 'ba_frostjoke.txt')),
]
PRE_MAPS = ['alberta', 'izlude', 'morocc', 'prontera', 'prt_church', 'prt_fild05', 'prt_fild08', 'prt_in']
MAPS = [(m + ext, os.path.join(PRE, m + ext)) for m in PRE_MAPS for ext in ('.rsw', '.gnd', '.gat')]

# --- English interface textures ----------------------------------------------
# ROenglishRE's translated bitmaps (data/texture/...) exist in several sets.
# Its own generator builds a pre-renewal client of 2012-04-10 or older from
# Renewal, overlaid by Pre-Renewal, overlaid by Compatibility/2012-04-10; the
# later Compatibility/<date> sets are for newer clients. A file is taken from
# the first set in TEXTURE_SETS whose version has the image size of the file
# the client reads from the archives today (rdata.grf over data.grf): the 2008
# UI code places buttons and labels on those sizes, so a version of another
# size has another layout and is never used. A file the archives lack is not
# copied (the client never reads it), except the ones in TEXTURE_ABSENT_OK.
# The files are copied unchanged; their folders get the client's names
# (ROenglishRE stores the CP949 folder names as Latin-1 characters; the game
# folder holds them as Unicode, which the sys module's CP949 lookup finds).
TEXTURE_SETS = ['Pre-Renewal', 'Compatibility/2012-04-10', 'Renewal']   # then every other Compatibility set, oldest first
# Referenced by the 2008 client (LoginMode.cpp, the licence window's "disagree"
# button) but no longer in the 2021 archives; the English version has the size
# of its sibling btn_agree, which is in them.
TEXTURE_ABSENT_OK = {
    b'data\\texture\\\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\btn_disagree' + s + b'.bmp'
    for s in (b'', b'_a', b'_b')
}


def blank_tga(width, height):
    """A 32-bit TGA, every pixel transparent."""
    import struct
    return struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, width, height, 32, 8) + bytes(width * height * 4)


# Textures with Korean text that ROenglishRE has no English version of and that
# the English services drew from their own data: client path -> (maker, reason).
# The title screen draws them for every service (UIWindowMgr::RenderTitleGraphic,
# as the reference does); a transparent file of the archive's size leaves the
# picture as it is without the Korean.
TEXTURE_GENERATED = {
    b'data\\texture\\\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba\\t_12\xbc\xbc.tga':
        (blank_tga, 'transparent in place of the Korean age rating of the title screen'),
}


def image_size(data):
    """(width, height) of a BMP, TGA, JPEG or PNG, or None."""
    import struct
    if data[:2] == b'BM' and len(data) >= 26:
        w, h = struct.unpack_from('<ii', data, 18)
        return w, abs(h)
    if data[:8] == b'\x89PNG\r\n\x1a\n' and len(data) >= 24:
        return struct.unpack_from('>II', data, 16)
    if data[:2] == b'\xff\xd8':
        at = 2
        while at + 9 < len(data):
            if data[at] != 0xff:
                at += 1
                continue
            if data[at + 1] in (0xc0, 0xc1, 0xc2):
                h, w = struct.unpack_from('>HH', data, at + 5)
                return w, h
            at += 2 + struct.unpack_from('>H', data, at + 2)[0]
        return None
    if len(data) >= 18 and data[2] in (1, 2, 3, 9, 10, 11):
        return struct.unpack_from('<HH', data, 12)
    return None


def client_name(component):
    """A ROenglishRE path component as the client's bytes: CP949 names are
    stored as Latin-1 characters (À¯ÀúÀÎÅÍÆäÀÌ½º is 유저인터페이스)."""
    import unicodedata
    component = unicodedata.normalize('NFC', component)
    try:
        return component.encode('latin-1')
    except UnicodeEncodeError:
        return component.encode('cp949')


def archive_tables(game):
    import grfaudit
    files = {}
    for name in ('data.grf', 'rdata.grf'):          # rdata.grf is searched first
        path = os.path.join(game, name)
        if os.path.exists(path):
            files.update(grfaudit.read_table(path)[2])
    return files


def texture_candidates(roe):
    """client path (lower-case CP949 bytes, data\\texture\\...) -> [(set, file)] in preference order."""
    comp = os.path.join(roe, TRANS, 'Compatibility')
    sets = TEXTURE_SETS + ['Compatibility/' + d for d in sorted(os.listdir(comp))
                           if 'Compatibility/' + d not in TEXTURE_SETS]
    found = {}
    for s in sets:
        data = os.path.join(roe, TRANS, *s.split('/'), 'data')
        for dirpath, _, names in os.walk(os.path.join(data, 'texture')):
            for n in sorted(names):
                full = os.path.join(dirpath, n)
                rel = os.path.relpath(full, data).split(os.sep)
                key = b'data\\' + b'\\'.join(client_name(c) for c in rel).lower()
                found.setdefault(key, []).append((s, full))
    return found


def choose_textures(game, roe):
    """[(client path, content, source)] and the files left out with why."""
    import grfaudit
    archives = archive_tables(game)
    chosen, left = [], []
    for key, cands in sorted(texture_candidates(roe).items()):
        entry = archives.get(key)
        if entry is None:
            if key in TEXTURE_ABSENT_OK:
                s, full = cands[0]
                chosen.append((key, open(full, 'rb').read(), 'ROenglishRE/%s/%s (absent from the archives)' % (TRANS, s)))
            else:
                left.append((key, 'absent from the archives'))
            continue
        want = image_size(grfaudit.read_file(entry) or b'')
        for s, full in cands:
            with open(full, 'rb') as f:
                data = f.read()
            if image_size(data) == want:
                chosen.append((key, data, 'ROenglishRE/%s/%s' % (TRANS, s)))
                break
        else:
            sizes = ', '.join('%s %s' % (s, image_size(open(f, 'rb').read())) for s, f in cands)
            left.append((key, 'archive %s, ROenglishRE %s' % (want, sizes)))
    for key, (make, why) in sorted(TEXTURE_GENERATED.items()):
        entry = archives.get(key)
        size = entry and image_size(grfaudit.read_file(entry) or b'')
        if size:
            chosen.append((key, make(*size), 'generated: ' + why))
    return chosen, left


def texture_destination(out, key):
    """The game folder path of a client path: CP949 decoded to Unicode, as the
    sys module's lookup decodes it."""
    parts = key.split(b'\\')[1:]
    return os.path.join(out, *(p.decode('cp949') for p in parts))



def drop_unknown_skills(data):
    """Removes the blocks of skills this client does not know (third jobs and
    later). SKILL_INFO::InitSkillInfoTables files every line after an unknown
    skill name under the skill before it: a description would gain another
    skill's text, an SP list another skill's numbers. A block runs from its
    "NAME#" line to the next one; the text of the blocks kept is untouched."""
    import tablecheck
    known = tablecheck.client_skill_names()
    header = re.compile(rb'^([A-Z][A-Z0-9_]*)#\s*$')
    out, keep = [], True
    for line in data.splitlines(keepends=True):
        m = header.match(line)
        if m:
            keep = m.group(1) in known
        if keep or line.startswith(b'//'):
            out.append(line)
    return b''.join(out)


def drop_non_rsw_maps(data):
    """Removes the lines of mapnametable.txt whose map is not a world file
    (ROenglishRE lists `alberta_d.gat`). CSession::GetMapName looks maps up
    by their .rsw name, so such a line is never found; tablecheck.py takes it
    for fields out of step. Comments, empty lines and every other entry stay
    as they are."""
    out = []
    for line in data.splitlines(keepends=True):
        head = line.split(b'#', 1)[0].strip()
        if b'#' in line and head and not head.startswith(b'//') and not head.lower().endswith(b'.rsw'):
            continue
        out.append(line)
    return b''.join(out)


def pad_messages(data):
    """Appends "?msg N#" for every message id the table lacks (none for the
    current ROenglishRE table, which has 4,022 lines for the client's 1,387)."""
    import tablecheck
    have = len(tablecheck.tokens(data, 0))
    need = tablecheck.client_msg_ids()
    if have >= need:
        return data
    if not data.endswith(b'\n'):
        data += b'\r\n'
    return data + b''.join(b'?msg %d#\r\n' % i for i in range(have, need))


# Repairs: data/ name -> (function bytes -> bytes, reason). Each answers a
# fault tablecheck.py reports on the file as ROenglishRE has it.
REPAIRS = {
    'msgstringtable.txt': (pad_messages, 'placeholders for missing message ids'),
    'mapnametable.txt': (drop_non_rsw_maps, 'entries that name no .rsw removed'),
    'skilldesctable.txt': (drop_unknown_skills, 'blocks of skills the client does not know removed'),
    'leveluseskillspamount.txt': (drop_unknown_skills, 'blocks of skills the client does not know removed'),
}

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

DATA_INI = '''[Data]
1=rdata.grf
2=data.grf
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
    ap.add_argument('--list', action='store_true', help='also list every texture with its source, and the ones left out')
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
    # The client's own string table (not a ROenglishRE file; it lives in the repository).
    strings_src = os.path.join(HERE, 'strings', 'translation.tsv')
    if os.path.exists(strings_src):
        place(os.path.join(out, 'bf_strings.tsv'), open(strings_src, 'rb').read(), args.check, report,
              'port/data/strings/translation.tsv')
    else:
        missing.append('port/data/strings/translation.tsv')
    # The archive list the client reads from the game folder (WinMain.cpp,
    # AddPaksFromDataIni): rdata.grf is searched before data.grf.
    place(os.path.join(args.game, 'DATA.INI'), DATA_INI.replace('\n', '\r\n').encode('latin-1'),
          args.check, report, 'generated by port/data/assemble.py')

    # English interface textures (one summary line per source set; --list for every file).
    textures, left = choose_textures(args.game, roe)
    tex_report, per_set = [], {}
    for key, data, s in textures:
        dst = texture_destination(out, key)
        if not args.check:
            os.makedirs(os.path.dirname(dst), exist_ok=True)
        state = place(dst, data, args.check, tex_report, s)
        per_set.setdefault(s, {}).setdefault(state, 0)
        per_set[s][state] += 1

    for state, name, size, digest, source in report:
        print(f'  {state:8} {name:32} {size:9} {digest}  {source}')
    for s, states in per_set.items():
        print(f'  textures {s}: ' + ', '.join(f'{n} {st}' for st, n in sorted(states.items())))
    absent = sum(1 for _, why in left if why.startswith('absent'))
    print(f'  textures left out: {absent} absent from the archives, {len(left) - absent} of another size than the archive\'s')
    if args.list:
        for (key, _, s), (state, *_rest) in zip(textures, tex_report):
            print(f'    {state:8} {key.decode("cp949", "replace")}  <- {s}')
        for key, why in left:
            print(f'    left out {key.decode("cp949", "replace")}: {why}')
    report += tex_report
    for src in missing:
        print(f'  MISSING SOURCE {src}')
    bad = missing or (args.check and any(r[0] != 'same' for r in report))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
