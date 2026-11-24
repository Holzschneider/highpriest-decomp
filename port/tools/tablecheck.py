#!/usr/bin/env python3
"""Checks the client's text tables against what its parsers expect.

  tablecheck.py [--game DIR] [--folder DIR] [--archive A.grf ...] [--clientinfo NAME] [-v]

Every table is looked up the way the client looks it up: the name goes
through CLanguage::LanguageFile (data\\english\\NAME if that exists, else
data\\NAME) and then through CFileMgr::GetData, which reads the loose data
folder first when the clientinfo file has <readfolder>, the archives first
otherwise. The clientinfo file itself is read before that option is known,
so it is always looked up archives first.

Each file is then cut into tokens exactly as CommonObject::TokenFileToPCharList
(ItemInfo.cpp) or the table's own reader (Session.cpp, Res.cpp) does it, and
the token stream is checked against how the loader walks it:

  message table   at least as many entries as the client has message ids
                  (MSGSTRINGID in src/MsgStrings.h), one entry per line, so
                  that line N is message N
  id/value tables tokens come in groups (id#value#); every id is a number
  description     an id line (NNN#) followed by text lines; text lines are
    tables        only taken when they end in CR, and a text line that is
                  only digits would be read as a new id
  skill tables    every skill name must be one the client knows
                  (src/SkillInfo.cpp): the loaders append the lines of an
                  unknown skill to the skill before it
  map tables      groups of the size the reader expects, names end in .rsw
  clientinfo      parses as the client's XML reader needs (no <x/>), names
                  servicetype, servertype and one connection

Faults are printed as FILE:LINE: expectation, and the exit status is 1 if
there is any. Warnings (oddities that do not misplace an entry) do not fail.
"""
import argparse, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import grfaudit                                       # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(HERE, '..', '..', 'src'))


def tokens(buf, kind):
    """TokenFileToPCharList: [(token, line, ended by '#')]; token None for kind 2 markers."""
    out = []
    base = 0
    line = base_line = 1
    for i, c in enumerate(buf):
        if c == 0x23:                                  # '#'
            if buf[base:base + 2] != b'//':
                out.append((buf[base:i].split(b'\0', 1)[0], base_line, True))
                if kind == 2:
                    out.append((None, base_line, True))
                base, base_line = i + 1, line
        elif c == 10:
            line += 1
            base, base_line = i + 1, line
        elif c == 13:
            if kind and buf[base:base + 2] != b'//' and base < i and buf[base] != 0:
                out.append((buf[base:i].split(b'\0', 1)[0], base_line, False))
            base, base_line = i + 1, line
    return out


def hash_fields(buf):
    """Readers that only split at '#' and reset at CR/LF (Session.cpp, Res.cpp)."""
    out, base, line, base_line = [], 0, 1, 1
    for i, c in enumerate(buf):
        if c == 0x23:
            out.append((buf[base:i], base_line, True))
            base, base_line = i + 1, line
        elif c in (10, 13):
            line += c == 10
            base, base_line = i + 1, line
    return out


def is_number(t):
    return t is not None and t != b'' and all(48 <= b <= 57 for b in t)


def client_msg_ids():
    text = open(os.path.join(SRC, 'MsgStrings.h'), encoding='latin-1').read()
    ids = [int(v) for n, v in re.findall(r'^\s*(MSI_\w+)\s*=\s*(\d+)', text, re.M) if n != 'MSI_LAST']
    m = re.search(r'g_idMsgStrings\[(\d+)\]', open(os.path.join(SRC, 'MsgStrings.cpp'), encoding='latin-1').read())
    return int(m.group(1)) if m else max(ids) + 1


def client_skill_names():
    text = open(os.path.join(SRC, 'SkillInfo.cpp'), encoding='latin-1').read()
    return {n.encode() for n in re.findall(r'IdNameList\[[^\]]+\]\s*=\s*"([^"]+)"', text)}


class Check:
    def __init__(self, verbose):
        self.faults = 0
        self.warnings = 0
        self.verbose = verbose

    def fault(self, where, line, msg):
        self.faults += 1
        print(f'FAULT {where}:{line}: {msg}')

    def warn(self, where, line, msg):
        self.warnings += 1
        if self.verbose or self.warnings <= 40:
            print(f'warn  {where}:{line}: {msg}')


class Files:
    def __init__(self, folder, archives):
        self.loose = grfaudit.read_folder(folder) if folder and os.path.isdir(folder) else {}
        self.packed = {}
        for path in reversed(archives):
            self.packed.update(grfaudit.read_table(path)[2])
        self.folder_first = False

    def exists(self, key):
        return key in self.loose or key in self.packed

    def get(self, key, folder_first=None):
        """(source, bytes) or (None, None)."""
        ff = self.folder_first if folder_first is None else folder_first
        order = (('folder', self.loose), ('archive', self.packed)) if ff else (('archive', self.packed), ('folder', self.loose))
        for src, table in order:
            if key in table:
                e = table[key]
                data = grfaudit.read_file(e)
                where = e.path if e.archive is None else f'{os.path.basename(e.archive.name)}:{key.decode("cp949", "replace")}'
                return where, data
        return None, None

    def table(self, name, english=True):
        """The file a table name resolves to, through LanguageFile and GetData."""
        n = name.lower().encode()
        if english and self.exists(b'data\\english\\' + n):
            n = b'english\\' + n
        return self.get(b'data\\' + n)


def check_clientinfo(ck, where, data):
    if data is None:
        ck.fault(where, 0, 'clientinfo file missing: the client falls back to the Korean service type')
        return False
    text = data.decode('latin-1')
    for m in re.finditer(r'<(\w+)[^>]*/>', text):
        ck.fault(where, text.count('\n', 0, m.start()) + 1,
                 f'<{m.group(1)}/>: the client\'s XML reader asserts on self-closing tags (xml.cpp ParseTag)')
    for m in re.finditer(r'<!--', text):
        end = text.find('-->', m.start())
        if '>' in text[m.start() + 4:end]:
            ck.fault(where, text.count('\n', 0, m.start()) + 1, 'comment contains ">": the reader ends it there')
    def element(name, scope=text):
        m = re.search(r'<' + name + r'>(.*?)</' + name + r'>', scope, re.S)
        return m.group(1) if m else None
    root = element('clientinfo')
    if root is None:
        ck.fault(where, 0, 'no <clientinfo> element')
        return False
    top = re.sub(r'<connection>.*?</connection>', '', root, flags=re.S)
    st = element('servicetype', top)
    if st != 'america':
        ck.fault(where, 0, f'servicetype is {st!r}; the English client needs exactly "america" (no spaces)')
    if element('servertype', top) not in ('primary', 'sakray', 'local', 'pk'):
        ck.fault(where, 0, 'servertype must be primary, sakray, local or pk')
    conn = element('connection', root)
    if conn is None:
        ck.fault(where, 0, 'no <connection>: SelectClientInfo(0) finds no address')
        return False
    for key in ('address', 'port', 'version'):
        v = element(key, conn)
        if v is None or v != v.strip() or not v:
            ck.fault(where, 0, f'<connection><{key}> missing or padded with spaces (contents are taken as they are)')
    if element('langtype', conn) not in (None, '1'):
        ck.fault(where, 0, '<langtype> overrides servicetype; it must be absent or 1 (america)')
    if '<aid>' in conn and '<admin>' not in conn:
        ck.fault(where, 0, '<aid> without <admin>: SelectClientInfo dereferences the first admin')
    readfolder = re.search(r'<readfolder>\s*</readfolder>', top) is not None
    print(f'  clientinfo {where}: servicetype={st} address={element("address", conn)}:{element("port", conn)} '
          f'readfolder={"yes" if readfolder else "no"}')
    return readfolder


def check_msg(ck, where, data, need):
    toks = tokens(data, 0)
    per_line = {}
    for t, ln, _ in toks:
        per_line[ln] = per_line.get(ln, 0) + 1
    for ln, c in sorted(per_line.items()):
        if c > 1:
            ck.fault(where, ln, f'{c} entries on one line: every message after it moves to the wrong id')
    lines = data.split(b'\n')
    for ln, l in enumerate(lines, 1):
        s = l.rstrip(b'\r')
        if s and b'#' not in s and not s.startswith(b'//'):
            ck.fault(where, ln, 'text without a closing "#": the message is dropped and the ids after it shift')
    if len(toks) < need:
        ck.fault(where, len(lines), f'{len(toks)} messages, the client indexes {need} (ids 0..{need - 1})')
    return f'{len(toks)} messages, client needs {need}'


def check_groups(ck, where, data, size, kind=0, id_fields=(0,), numeric=True, rsw=False, reader=None):
    toks = reader(data) if reader else tokens(data, kind)
    if len(toks) % size:
        t, ln, _ = toks[-1] if toks else (b'', 0, True)
        ck.fault(where, ln, f'{len(toks)} fields, not a multiple of {size}: the last entry is cut short')
    shown = 0
    for g in range(0, len(toks) - len(toks) % size, size):
        for f in id_fields:
            t, ln, _ = toks[g + f]
            bad = (numeric and not is_number(t.strip())) or (rsw and not t.lower().endswith(b'.rsw'))
            if bad:
                ck.fault(where, ln, f'field {f + 1} of entry {g // size + 1} is {t[:30]!r}; expected '
                                    f'{"a number" if numeric else "a .rsw name"} (fields out of step from here)')
                shown += 1
                if shown >= 5:
                    ck.fault(where, ln, 'further faults in this file not shown')
                    return f'{len(toks) // size} entries'
                break
    return f'{len(toks) // size} entries'


def check_desc(ck, where, data, item=True, skills=None):
    if data.count(b'\r') == 0 and data.count(b'\n'):
        ck.fault(where, 1, 'lines end in LF only: the reader takes text lines at CR, so every description is lost')
    toks = tokens(data, 1)
    ids = unknown = 0
    for t, ln, hashed in toks:
        if not t:
            continue
        if item:
            if is_number(t):
                ids += 1
                if not hashed:
                    ck.fault(where, ln, f'text line {t!r} is only digits: the reader takes it as item id {int(t)}')
            elif hashed:
                ck.warn(where, ln, f'"#" inside a description line splits it: {t[:40]!r}')
        elif hashed:
            if t in skills:
                ids += 1
            else:
                unknown += 1
                ck.warn(where, ln, f'skill {t[:40]!r} is not one the client knows; its lines join the skill before it')
    if unknown:
        ck.fault(where, 0, f'{unknown} skills unknown to the client: their text is appended to the skill before each')
    return f'{ids} {"items" if item else "skills"}' + (f', {unknown} unknown skills' if unknown else '')


def check_skill_names(ck, where, data, skills):
    toks = tokens(data, 0)
    res = check_groups(ck, where, data, 2, numeric=False)
    unknown = [t for t, ln, _ in toks[0::2] if t not in skills]
    if unknown:
        ck.warn(where, 0, f'{len(unknown)} skill names unknown to the client (harmless: they land on id 0, '
                          f'which is reset afterwards), e.g. {b" ".join(unknown[:4]).decode()}')
    return res + f', {len(toks) // 2 - len(unknown)} known to the client'


def check_sp(ck, where, data, skills):
    toks = tokens(data, 0)
    known = unknown = 0
    for t, ln, _ in toks:
        if is_number(t) or t == b'@':
            continue
        if t in skills:
            known += 1
        else:
            unknown += 1
            ck.fault(where, ln, f'{t[:30]!r} is neither a number nor a skill the client knows: '
                                f'the numbers after it are added to the previous skill\'s SP list')
    return f'{known} skills' + (f', {unknown} unknown' if unknown else '')


# name, how the client loads it, which check
SPECS = [
    ('msgStringTable.txt', 'msg'),
    ('Num2ItemDisplayNameTable.txt', ('groups', 2)),
    ('Num2itemDescTable.txt', 'itemdesc'),
    ('IdNum2ItemDisplayNameTable.txt', ('groups', 2)),
    ('IdNum2itemDescTable.txt', 'itemdesc'),
    ('Num2ItemResNameTable.txt', ('groups', 2)),
    ('IdNum2ItemResNameTable.txt', ('groups', 2)),
    ('itemSlotTable.txt', ('groups', 2)),
    ('itemParamTable.txt', ('groups', 3)),
    ('itemSlotCountTable.txt', ('groups', 2)),
    ('CardItemNameTable.txt', ('groups', 1)),
    ('Num2CardIllustNameTable.txt', ('groups', 2)),
    ('CardPrefixNameTable.txt', ('groups', 2)),
    ('CardPostfixNameTable.txt', ('groups', 1)),
    ('BookItemNameTable.txt', ('groups', 1)),
    ('MetalProcessItemList.txt', 'present'),
    ('skillnametable.txt', 'skillname'),
    ('skilldesctable.txt', 'skilldesc'),
    ('leveluseskillspamount.txt', 'sp'),
    ('mapNameTable.txt', 'mapname'),
    ('mp3NameTable.txt', 'mp3'),
    ('indoorRswTable.txt', 'indoor'),
    ('mapObjLightTable.txt', 'objlight'),
    ('ViewPointTable.txt', 'viewpoint'),
    ('fogParameterTable.txt', 'fog'),
    ('ResNameTable.txt', 'resname'),
    ('mapPosTable.txt', 'present'),
    ('manner.txt', 'present'),
]
# Loaded by CFile::Open directly, not through LanguageFile.
DIRECT = {'ViewPointTable.txt', 'fogParameterTable.txt', 'ResNameTable.txt', 'mapPosTable.txt'}


def main():
    game = os.environ.get('BF_CLIENT_DIR') or os.path.expanduser('~/Games/Bifrost')
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game', default=game, help='game folder (default $BF_CLIENT_DIR or ~/Games/Bifrost)')
    ap.add_argument('--folder', help='loose data folder (default GAME/data)')
    ap.add_argument('--archive', action='append', help='archives in search order (default GAME/rdata.grf, GAME/data.grf)')
    ap.add_argument('--clientinfo', default='sclientinfo.xml', help='the clientinfo file the client opens (WinMain.cpp)')
    ap.add_argument('-v', '--verbose', action='store_true', help='print every warning')
    args = ap.parse_args()
    folder = args.folder or os.path.join(args.game, 'data')
    archives = args.archive or [os.path.join(args.game, 'rdata.grf'), os.path.join(args.game, 'data.grf')]

    ck = Check(args.verbose)
    files = Files(folder, archives)
    print(f'loose folder {folder}: {len(files.loose)} files; archives {" ".join(archives)}')

    where, data = files.get(b'data\\' + args.clientinfo.lower().encode(), folder_first=False)
    files.folder_first = check_clientinfo(ck, where or args.clientinfo, data)
    if any(k.startswith(b'data\\english\\') for k in list(files.loose) + list(files.packed)):
        print('  data\\english\\ exists: LanguageFile prefers it for tables found there')

    need = client_msg_ids()
    skills = client_skill_names()
    for name, how in SPECS:
        before = ck.faults
        where, data = files.table(name, english=name not in DIRECT)
        if data is None:
            if how == 'present' and name == 'MetalProcessItemList.txt':
                ck.warn(name, 0, 'missing (optional list)')
                continue
            ck.fault(name, 0, f'missing: neither the folder nor the archives have data\\{name}')
            continue
        if data[:3] == b'\xef\xbb\xbf':
            ck.fault(where, 1, 'starts with a UTF-8 byte order mark: the first entry\'s id reads as 0')
        if how == 'msg':
            res = check_msg(ck, where, data, need)
        elif isinstance(how, tuple):
            res = check_groups(ck, where, data, how[1])
        elif how == 'itemdesc':
            res = check_desc(ck, where, data, item=True)
        elif how == 'skillname':
            res = check_skill_names(ck, where, data, skills)
        elif how == 'skilldesc':
            res = check_desc(ck, where, data, item=False, skills=skills)
        elif how == 'sp':
            res = check_sp(ck, where, data, skills)
        elif how in ('mapname', 'mp3'):
            res = check_groups(ck, where, data, 2, id_fields=(0,), numeric=False, rsw=True)
        elif how == 'objlight':
            res = check_groups(ck, where, data, 2, id_fields=(0,), numeric=False, rsw=True)
        elif how == 'indoor':
            res = check_groups(ck, where, data, 1, numeric=False, rsw=True)
        elif how == 'viewpoint':
            res = check_groups(ck, where, data, 10, numeric=False, rsw=True, reader=hash_fields)
        elif how == 'fog':
            res = check_groups(ck, where, data, 5, numeric=False, rsw=True, reader=hash_fields)
        elif how == 'resname':
            res = check_groups(ck, where, data, 2, numeric=False, reader=hash_fields)
        else:
            res = f'{len(data)} bytes'
        state = 'ok   ' if ck.faults == before else 'FAULT'
        print(f'  {state} {name:32} {res:48} {where}')

    print(f'\n{ck.faults} faults, {ck.warnings} warnings')
    sys.exit(1 if ck.faults else 0)


if __name__ == '__main__':
    main()
