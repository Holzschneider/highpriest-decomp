#!/usr/bin/env python3
"""Finds the Korean string literals of src/ and sorts them (step M9.2).

  port/tools/strings.py [--src DIR] [--out DIR] [--summary] [--review] [--check]

A literal is a C string literal (adjacent literals are joined, as the
compiler does) whose bytes include one >= 0x80 and that decodes as CP949.
Each one is classified from its call site:

  displayed  text the player can see: drawn, put in chat, a message box, a
             window caption, a name balloon, a format string that ends up on
             screen. These are the entries of translation.tsv.
  path       a file or folder name inside the archives, a resource name, a
             texture or sprite name. They stay as they are.
  other      not shown and not a path: the built-in Korean message table (the
             English service type loads msgStringTable.txt instead), debug
             traces, registry keys, protocol and crypto bytes, anything that
             does not decode as CP949.

The call site is the evidence: the function the literal is passed to, the
variable it is assigned to, the shape of the text itself. The reason column
of literals.tsv says which. The decisions can be reviewed in plain text with
--review, and corrected without touching the rules by editing OVERRIDES
(file:function or file:line prefixes) below.

Outputs (default port/data/strings/):
  literals.tsv     every literal: file, line, function, class, reason,
                   original bytes (hex), CP949 text. Committed.
  translation.tsv  the displayed ones, one per distinct byte string:
                   original bytes (hex), English (CP1252; empty until M9.3),
                   Korean text as a comment column for the translator.
                   An English column that is already filled in is kept when
                   the table is regenerated.
  callsites.tsv    file:line of every displayed literal, which bf_tr() will
                   wrap when the step that wires the lookup runs.

Only the client's own literals are written. Translations of ROenglishRE or
any other table never go into these files.
"""
import argparse, os, re, sys, collections

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
SRC_EXT = ('.c', '.cpp', '.h', '.hpp', '.inl')


# ---------------------------------------------------------------- tokenizer

class Tok:
    __slots__ = ('kind', 'text', 'line', 'val')

    def __init__(self, kind, text, line, val=None):
        self.kind, self.text, self.line, self.val = kind, text, line, val

    def __repr__(self):
        return f'{self.kind}:{self.text!r}@{self.line}'


ESC = {'n': 10, 't': 9, 'r': 13, 'a': 7, 'b': 8, 'f': 12, 'v': 11, '\\': 92, "'": 39, '"': 34, '?': 63}


def lex(data):
    """Splits C source (bytes) into tokens. Strings carry their decoded bytes
    in .val; comments are dropped. A #define line is lexed like any other, with
    a 'pp' token carrying the directive and macro name first."""
    toks, i, n, line = [], 0, len(data), 1
    at_bol = True
    while i < n:
        c = data[i]
        if c == 10:
            line += 1; i += 1; at_bol = True; continue
        if c in b' \t\r\f\v':
            i += 1; continue
        if c == 92 and i + 1 < n and data[i + 1] in b'\r\n':      # line continuation
            i += 2 if data[i + 1] == 10 else 3
            line += 1; continue
        if data.startswith(b'//', i):
            while i < n and data[i] != 10:
                if data[i] == 92 and i + 1 < n and data[i + 1] in b'\r\n':
                    i += 2; line += 1
                i += 1
            continue
        if data.startswith(b'/*', i):
            j = data.find(b'*/', i + 2)
            j = n if j < 0 else j + 2
            line += data.count(b'\n', i, j); i = j; continue
        if c == 35 and at_bol:                                   # preprocessor
            m = re.compile(rb'#[ \t]*(\w+)[ \t]*(\w*)').match(data, i)
            directive = m.group(1).decode() if m else ''
            if directive == 'include':
                while i < n and data[i] != 10:
                    i += 1
                continue
            toks.append(Tok('pp', directive + ' ' + (m.group(2).decode() if m else ''), line))
            i = m.end() if m else i + 1
            at_bol = False
            continue
        at_bol = False
        if c == 34 or (c in b'LuU' and data[i + 1:i + 2] == b'"') or (c == 117 and data[i + 1:i + 3] == b'8"'):
            wide = c != 34
            j = i + (data.index(b'"', i) - i) + 1
            out, start = bytearray(), line
            while j < n and data[j] != 34:
                b = data[j]
                if b == 10:
                    break                                       # unterminated; give up on it
                if b == 92:
                    j += 1
                    e = data[j]
                    if e in b'xX':
                        k = j + 1
                        while k < n and k < j + 3 and chr(data[k]) in '0123456789abcdefABCDEF':
                            k += 1
                        out.append(int(data[j + 1:k], 16) if k > j + 1 else 0); j = k
                    elif 48 <= e <= 55:
                        k = j
                        while k < n and k < j + 3 and 48 <= data[k] <= 55:
                            k += 1
                        out.append(int(data[j:k], 8) & 255); j = k
                    elif e in (10, 13):                          # continuation inside a string
                        j += 1 + (data[j] == 13 and data[j + 1:j + 2] == b'\n')
                        line += 1
                    else:
                        out.append(ESC.get(chr(e), e)); j += 1
                else:
                    out.append(b); j += 1
            toks.append(Tok('wstr' if wide else 'str', '', start, bytes(out)))
            i = j + 1
            continue
        if c == 39:                                              # character literal
            j = i + 1
            while j < n and data[j] != 39 and data[j] != 10:
                j += 2 if data[j] == 92 else 1
            toks.append(Tok('chr', data[i:j + 1].decode('latin-1'), line)); i = j + 1; continue
        m = re.compile(rb'[A-Za-z_]\w*').match(data, i)
        if m:
            toks.append(Tok('id', m.group().decode(), line)); i = m.end(); continue
        m = re.compile(rb'0[xX][0-9a-fA-F]+[uUlL]*|\d[\w.]*').match(data, i)
        if m:
            toks.append(Tok('num', m.group().decode(), line)); i = m.end(); continue
        two = data[i:i + 2]
        if two in (b'::', b'->', b'==', b'!=', b'<=', b'>=', b'&&', b'||', b'<<', b'>>', b'+=', b'-=', b'*=', b'/=', b'|=', b'&=', b'++', b'--'):
            toks.append(Tok('op', two.decode(), line)); i += 2; continue
        toks.append(Tok('op', chr(c), line)); i += 1
    return toks


# ------------------------------------------------------------ call-site model

class Lit:
    """One literal group: adjacent string tokens joined into one byte string."""
    __slots__ = ('file', 'line', 'func', 'data', 'callee', 'argn', 'lhs', 'stmt', 'cls', 'reason', 'text', 'wide',
                 'callers')

    def key(self):
        return self.data.hex()


def chain_before(toks, k):
    """The identifier chain ending at toks[k] (A :: B . c -> d), as one string."""
    parts = []
    while k >= 0:
        t = toks[k]
        if t.kind == 'id':
            parts.append(t.text)
            if k >= 1 and toks[k - 1].kind == 'op' and toks[k - 1].text in ('::', '.', '->'):
                parts.append(toks[k - 1].text); k -= 2; continue
        elif t.kind == 'op' and t.text == ']':          # a[...]: skip to the name
            depth = 0
            while k >= 0:
                if toks[k].text == ']': depth += 1
                elif toks[k].text == '[':
                    depth -= 1
                    if depth == 0: break
                k -= 1
            k -= 1; continue
        break
    return ''.join(reversed(parts))


def func_name(toks, start, end):
    """Name of the function whose definition is toks[start:end] (up to its '{')."""
    for k in range(start, end):
        if toks[k].kind == 'op' and toks[k].text == '(' and k > start and toks[k - 1].kind == 'id':
            return chain_before(toks, k - 1)
        if toks[k].kind == 'op' and toks[k].text == '=':
            return ''
    return ''


def stmt_text(toks, a, b, limit=160):
    out = []
    for t in toks[a:b]:
        out.append('"..."' if t.kind in ('str', 'wstr') else t.text)
    s = ' '.join(out)
    return s if len(s) <= limit else s[:limit - 3] + '...'


def analyze(path, rel):
    data = open(path, 'rb').read()
    toks = lex(data)
    lits = []
    braces = []          # (kind, name, lhs)  kind: 'func' | 'agg' | 'block'
    parens = []          # (callee, argn, tokindex)
    stmt_start = 0
    cur_func = ''
    pp_name = ''
    i, n = 0, len(toks)
    while i < n:
        t = toks[i]
        if t.kind == 'pp':
            pp_name = t.text
            if t.text.startswith('define'):
                # the rest of the logical line: tokens with the same line number, or following the continuation
                j = i + 1
                body_line = t.line
                while j < n and toks[j].line <= body_line + 20 and toks[j].kind != 'pp' and toks[j].line == t.line:
                    j += 1
                # strings in a macro body: context is the macro
                k = i + 1
                while k < j:
                    if toks[k].kind == 'str':
                        l2 = k
                        buf = bytearray()
                        while l2 < j and toks[l2].kind == 'str':
                            buf += toks[l2].val; l2 += 1
                        lit = Lit(); lit.file, lit.line, lit.func = rel, toks[k].line, '#define ' + t.text.split()[-1]
                        lit.data, lit.callee, lit.argn, lit.lhs = bytes(buf), '', 0, t.text.split()[-1]
                        lit.stmt = stmt_text(toks, i, j); lit.wide = False
                        lits.append(lit)
                        k = l2
                    else:
                        k += 1
                i = j; stmt_start = i; continue
            i += 1; stmt_start = i; continue
        if t.kind in ('str', 'wstr'):
            j = i
            buf = bytearray()
            while j < n and toks[j].kind == t.kind:
                buf += toks[j].val; j += 1
            lit = Lit(); lit.file, lit.line, lit.wide = rel, t.line, t.kind == 'wstr'
            lit.data = bytes(buf)
            lit.func = cur_func if (braces and any(b[0] == 'func' for b in braces)) else ''
            callee, argn = '', 0
            for p in reversed(parens):
                if p[0]:
                    callee, argn = p[0], p[1]; break
            lit.callee, lit.argn = callee, argn
            # the variable assigned to, or the array being initialised
            lhs = ''
            depth = 0
            for k in range(i - 1, stmt_start - 1, -1):
                tk = toks[k]
                if tk.kind == 'op':
                    if tk.text in (')', ']'): depth += 1
                    elif tk.text in ('(', '['): depth -= 1
                    elif tk.text == '=' and depth <= 0:
                        lhs = chain_before(toks, k - 1); break
                    elif tk.text in ('==', '!=') and depth <= 0 and not callee:
                        lhs = '(cmp)' + chain_before(toks, k - 1); break
            if not lhs and braces and braces[-1][0] == 'agg':
                lhs = braces[-1][2]
            if not lhs and stmt_start < n and toks[stmt_start].kind == 'id' and toks[stmt_start].text == 'return':
                lhs = 'return'
            lit.lhs = lhs
            end = j
            while end < n and not (toks[end].kind == 'op' and toks[end].text in (';', '{', '}')):
                end += 1
            lit.stmt = stmt_text(toks, stmt_start, min(end + 1, n))
            lits.append(lit)
            i = j; continue
        if t.kind == 'op':
            x = t.text
            if x == '(':
                callee = toks[i - 1].text if i > 0 and toks[i - 1].kind == 'id' and toks[i - 1].text not in (
                    'if', 'while', 'for', 'switch', 'return', 'sizeof') else ''
                if i > 1 and toks[i - 1].kind == 'op' and toks[i - 1].text == ']':
                    callee = ''
                parens.append([callee, 0, i])
            elif x == ')':
                if parens: parens.pop()
            elif x == ',':
                if parens: parens[-1][1] += 1
            elif x == '{':
                kind, name, lhs = 'block', '', ''
                if not braces and not parens:
                    nm = func_name(toks, stmt_start, i)
                    prev = toks[i - 1]
                    if nm and (prev.text == ')' or prev.text == 'const' or prev.text == 'throw'):
                        kind, name = 'func', nm
                        cur_func = nm
                    else:
                        kind = 'agg'
                        lhs = chain_before(toks, i - 2) if prev.text == '=' else ''
                        if not lhs:
                            for k in range(stmt_start, i):
                                if toks[k].text == '=':
                                    lhs = chain_before(toks, k - 1); break
                        cur_func = ''
                elif braces and braces[-1][0] == 'agg':
                    kind, lhs = 'agg', braces[-1][2]
                elif braces and toks[i - 1].text in ('=', ',') :
                    kind = 'agg'
                    lhs = chain_before(toks, i - 2) if toks[i - 1].text == '=' else ''
                braces.append((kind, name, lhs))
                stmt_start = i + 1
            elif x == '}':
                if braces:
                    k = braces.pop()
                    if k[0] == 'func': cur_func = ''
                stmt_start = i + 1
            elif x == ';' and not parens:
                stmt_start = i + 1
        i += 1
    return lits


# ------------------------------------------------------------ classification

PATH_EXT = re.compile(r'\.(spr|act|bmp|wav|rsw|gat|gnd|tga|jpg|jpeg|png|gif|imf|pal|ebm|str|mp3|rsm2?|rma|w3d|gr2|bik|'
                      r'lub|lua|ini|dat|lst|x|rsx|txt|xml)(?![A-Za-z])', re.I)

# The call is what decides. Callees whose argument is a file or resource name.
PATH_CALLEES = {
    'UIBmp', 'SetSprName', 'SetActName', 'SetSpriteName', 'SetActFileName', 'SetBitmap', 'SetBitmap2',
    'SetBitmapName', 'PlayWave', 'Get', 'GetTexture', 'EffectTextureSet', 'Map_MagicZone', 'RG_COIN2',
    'PRESSURE', 'ThrowItem', 'ShieldBoomerang', 'STIN', 'STIN2', 'STIN4', 'STIN5', 'Create', 'pathName',
    'pName', 'pathUI',
}
# Variables and tables that hold file or folder name parts (sprite folders, palette names, bitmap names).
PATH_LHS = re.compile(
    r'^(m_(new)?(Accessory|PalJob|PcJob|PcJobImf|PcWeapon|Shield|Job|PcSex|NewPcSex|NewPcJob|WeaponHitWave)NameTable'
    r'|m_(new)?(pc|pal)?(job|sex|weapon|shield|accessory)(imf)?nametable|m_weaponHitWaveNameTable|m_newPcSexNameTable'
    r'|pathName|pathUI|pName|preBmpName\d?|bmpName|bmp\d?|bitmap|m_bitmap|m_bitmapName|m_wallPaperBmpName|m_itemBmp|m_skillBmp'
    r'|minimapBmpName|serviceBmpName|sprActName\.(spr|act)Name|m_sprResList|m_actResList|rank_(act|spr)|cursor(Act|Spr)'
    r'|number(Act|Spr)|itemSpriteFileName|g_prim->m_texture|mid|midBmp|left|right|btnLeft|btnRight|buttonName|bg'
    r'|CONTAINER_NAME|PATH_HUMAN|TITLE_FILE)$', re.I)
# Functions that only build file names; every literal in them that is not otherwise decided is a name part.
PATH_FUNCS = {'CSession::GetStateFileName', 'CSession::GetPetAccActName', 'CSession::GetJobDyeSprName'}
# Calls that show their argument to the player (or a message box, whose text the player reads).
DISPLAY_CALLEES = {'ErrorMsg': 'error box: text of a MessageBox (ErrorMsg) the player reads',
                   'MessageBoxA': 'message box text',
                   'TextOutWithOutline': 'drawn on the map',
                   'GetTextWidth': 'measures text that is drawn',
                   'SendMsg': 'pushed into the chat history (UIM_PUSHINTOCHATHISTORY)'}
# Calls that only compare or search with their literal, or write diagnostics.
OTHER_CALLEES = {
    'Trace': 'debug trace (a BFT_GAME line in the port)',
    'OutputDebugStringA': 'debug output',
    'fwrite': 'file content written by the client, never displayed',
    'strcmp': 'compared with text, not shown',
    'strstr': 'searched for in text, not shown',
    'find': 'searched for in text, not shown',
    'TTPair': 'typed chat command, matched against the player\'s input; not shown',
}
# Whole modules or functions that do not run for the English (america) service, or never show their literals.
SECURITY_FILES = ('cSecureGamePack.cpp', 'cKeyProtector.cpp', 'base/FindHack.cpp', 'NPGameLib.cpp', 'PacketProtect.cpp',
                  'cSecureGamePack.h', 'CommPipe.cpp')
OTHER_FUNCS = {
    'CRenderer::ScreenEffect': 'dead code: a message buffer that is filled and never used',
    'CSession::InitVariables': 'initial value of a name field, overwritten before it is shown',
    'CSession::RerecordProhibitLog': 'line of a log file the client writes',
    'CSession::InitTalkTypeTable': 'typed chat command, matched against the player\'s input; not shown',
    'CSession::GetTalkType': 'typed chat command, matched against the player\'s input; not shown',
    'CSession::InitShortCutTextFromReg': 'saved shortcut text compared with the Korean default; not shown',
    'CLanguageTC::CLanguageTC': 'code-page conversion table',
}

# Corrections made by hand after reading the call site: 'file:line' -> (class, reason).
# Empty unless the rules above are wrong for one literal; every entry says why.
OVERRIDES = {}


def decode(data):
    try:
        return data.decode('cp949')
    except UnicodeDecodeError:
        return None


def classify(l):
    """Returns (class, reason) from the call site of literal l."""
    text = decode(l.data)
    f, fn, callee, lhs = l.file, l.func, l.callee, l.lhs
    key = f'{f}:{l.line}'
    if key in OVERRIDES:
        return OVERRIDES[key]
    if f.endswith('MsgStrings.cpp') and lhs == 'g_idMsgStrings':
        return 'other', 'built-in Korean message table: the English service type loads msgStringTable.txt instead'
    if text is None:
        return 'other', 'not CP949 text (binary or obfuscated bytes)'
    if l.data[:1] and l.data[0] < 0x20 and l.data[0] not in (9, 10, 13) or callee.startswith('_Dect'):
        return 'other', 'obfuscated blob (leading control byte, decoded at run time); anti-cheat code'
    if any(f.endswith(s) for s in SECURITY_FILES):
        return 'other', 'anti-cheat or security module; not run for the english service type'
    if 'FindHack' in f or fn.startswith('CGameGuard') or fn.startswith('InitToolhelp32') or fn == 'ReadMemCrc':
        return 'other', 'anti-cheat code; not run for the english service type'
    if l.wide:
        return 'other', 'wide literal'
    # a Korea-only branch
    if fn == 'CGameMode::SendMsg' and callee == 'strcat':
        return 'other', 'appended only for ServiceKorea'
    # the callee decides first: ErrorMsg('item.txt ...') names a file and is still a message
    if callee in DISPLAY_CALLEES and not ('\\' in text and callee == 'SendMsg'):
        return 'displayed', DISPLAY_CALLEES[callee]
    if callee in OTHER_CALLEES:
        return 'other', OTHER_CALLEES[callee]
    if callee == 'strcat' and 'Shortcut' in fn or callee == 'strcat' and 'Shorcut' in fn:
        return 'other', 'registry value text, not shown'
    if fn in OTHER_FUNCS and OTHER_FUNCS[fn]:
        return 'other', OTHER_FUNCS[fn]
    if lhs == 'strApple':
        return 'path', 'resource name of the placeholder item (identifiedResourceName)'
    if lhs == 'szCharNameEcryption':
        return 'other', 'key text for a name encryption routine'
    if lhs in ('fontFace9x',):
        return 'other', 'font face name (mapped to a macOS font in M9.4, not translated)'
    if lhs == 'sexTable' or lhs.startswith('(cmp)'):
        return 'other', 'kinship words searched and replaced in pet-talk text (GetPetTalkString); the English pettalk table never contains them'
    if callee == 'WinPair':
        return 'other', 'language table entry for another service'
    if fn in ('UIChangeNameWnd::SendMsg',) or (fn == 'CPlayer::ReLaunchBlurEffects') or fn == 'CBitmapRes::LoadBMPData':
        return 'other', 'name compared with a literal'
    if callee in PATH_CALLEES:
        return 'path', f'argument of {callee}(), a file or resource name'
    if lhs and PATH_LHS.match(lhs):
        return 'path', f'assigned to {lhs}, a file or folder name part'
    if fn in PATH_FUNCS:
        return 'path', f'{fn} only returns file names'
    if callee == 'sprintf' and fn in ('CPc::RenderBodyLayer', 'UIEquipWnd::OnDraw', 'UISelectCharWnd::OnDraw'):
        return 'path', 'name part of a sprite file name built with sprintf'
    if callee == 'resize' and fn == 'CSession::InitPcNameTable':
        return 'path', 'initial entry of a sprite folder name table'
    # the shape of the text
    if '\\' in text or PATH_EXT.search(text):
        return 'path', 'file name shape (folder separator or resource extension)'
    if fn == 'CSession::GetItemName' and lhs == 'return':
        return 'displayed', 'returned as an item name shown to the player'
    if callee in ('sprintf', 'strcpy'):
        return 'displayed', f'text built with {callee}() for display'
    if fn.endswith('::SendMsg') or lhs in ('pet',):
        return 'displayed', 'text of a window message handler, drawn by the window'
    return 'displayed', 'default: not a path, not a diagnostic (review)'


def scan(src):
    """All Korean literals of the tree, classified, in file and line order."""
    out = []
    for dp, dn, fn in os.walk(src):
        dn.sort()
        for f in sorted(fn):
            if not f.endswith(SRC_EXT):
                continue
            p = os.path.join(dp, f)
            rel = os.path.relpath(p, os.path.dirname(os.path.abspath(src))).replace(os.sep, '/')
            for l in analyze(p, rel):
                if not any(b >= 0x80 for b in l.data):
                    continue
                l.text = decode(l.data)
                l.cls, l.reason = classify(l)
                out.append(l)
    out.sort(key=lambda l: (l.file, l.line))
    return out


# ------------------------------------------------------------------- output

def esc(s):
    """One field of a .tsv: backslash, newline, tab and carriage return escaped."""
    return s.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t').replace('\r', '\\r')


def unesc(s):
    out, i = [], 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            out.append({'n': '\n', 't': '\t', 'r': '\r', '\\': '\\'}.get(s[i + 1], '\\' + s[i + 1])); i += 2
        else:
            out.append(s[i]); i += 1
    return ''.join(out)


def shape(l):
    if l.func.startswith('#define'):
        return 'macro'
    if l.lhs == 'return':
        return 'return'
    if l.callee:
        return 'argument'
    if l.lhs and not l.lhs.startswith('(cmp)'):
        return 'initializer' if l.stmt.rstrip().endswith(('}', ',')) or l.stmt.lstrip().startswith('"') else 'assignment'
    return 'other'


FORMAT = re.compile(r'%[-+ #0]*(\*|\d+)?(\.(\*|\d+))?(hh|h|ll|l|L|z|I64)?[diouxXeEfgGcspn]')


def specs(s):
    return [m.group(0) for m in FORMAT.finditer(s.replace('%%', ''))]


LITERALS_HEAD = '# file\tline\tfunction\tclass\treason\toriginal bytes (hex)\tcp949 text (backslash, newline, tab escaped)\n'
TRANSLATION_HEAD = '''# Translation table for the client's displayed Korean literals (port/tools/strings.py, M9.2).
# One line per distinct original: <original bytes, hex> TAB <English> TAB <Korean, for reference>.
# The file is UTF-8. The English column holds only characters Windows-1252 has: bf_strings converts it
# to Windows-1252 when it loads. Backslash, newline and tab are written \\\\, \\n, \\t. An empty English
# column means "not translated yet": the client then shows the original and counts the miss.
# A translation keeps the original's printf conversions (%d, %s, ...) in the same order.
'''
CALLSITES_HEAD = '# file\tline\tfunction\tshape\toriginal bytes (hex)\n'


def load_translations(path):
    old = {}
    if os.path.exists(path):
        for line in open(path, encoding='utf-8'):
            if line.startswith('#') or not line.strip():
                continue
            f = line.rstrip('\n').split('\t')
            if len(f) >= 2:
                old[f[0]] = f[1]
    return old


def render(lits, old):
    """Returns {filename: text}."""
    out = {}
    out['literals.tsv'] = LITERALS_HEAD + ''.join(
        f'{l.file}\t{l.line}\t{l.func}\t{l.cls}\t{l.reason}\t{l.key()}\t{esc(l.text or "")}\n' for l in lits)
    seen, rows = set(), []
    for l in lits:
        if l.cls == 'displayed' and l.key() not in seen:
            seen.add(l.key())
            rows.append(f'{l.key()}\t{old.get(l.key(), "")}\t{esc(l.text)}\n')
    out['translation.tsv'] = TRANSLATION_HEAD + ''.join(rows)
    out['callsites.tsv'] = CALLSITES_HEAD + ''.join(
        f'{l.file}\t{l.line}\t{l.func}\t{shape(l)}\t{l.key()}\n' for l in lits if l.cls == 'displayed')
    out['review.txt'] = review(lits)
    return out


def review(lits):
    out = ['Classification of the Korean string literals of src/ (generated by port/tools/strings.py; do not edit).',
           'Each group is one rule and the reason it gives. Displayed entries are listed in full; for the other',
           'classes the first examples of each group show the call site the rule looked at.', '']
    for cls in ('displayed', 'other', 'path'):
        sel = [l for l in lits if l.cls == cls]
        out.append(f'=== {cls}: {len(sel)} literals, {len({l.key() for l in sel})} distinct ===')
        groups = collections.OrderedDict()
        for l in sel:
            groups.setdefault(l.reason, []).append(l)
        for reason, g in sorted(groups.items(), key=lambda kv: -len(kv[1])):
            out.append('')
            out.append(f'-- {reason}  [{len(g)}]')
            show = g if cls == 'displayed' else g[:3]
            for l in show:
                t = (l.text or '').replace('\n', '\\n')
                out.append(f'   {l.file}:{l.line}  {l.func or "-"}')
                out.append(f'      {l.stmt}')
                out.append(f'      {t if len(t) < 100 else t[:97] + "..."}')
            if len(show) < len(g):
                out.append(f'   ... {len(g) - len(show)} more')
        out.append('')
    return '\n'.join(out)


def summary(lits):
    n = collections.Counter(l.cls for l in lits)
    d = {c: len({l.key() for l in lits if l.cls == c}) for c in ('displayed', 'path', 'other')}
    print(f'{len(lits)} literals with bytes >= 0x80 that are CP949 text or binary, in '
          f'{len({l.file for l in lits})} files')
    for c in ('displayed', 'path', 'other'):
        print(f'  {c:10} {n[c]:5} literals  {d[c]:5} distinct')
    todo = [l for l in lits if l.reason.startswith('default:')]
    print(f'  unreviewed (fell through every rule): {len(todo)}')
    print('other, by reason:')
    for r, c in collections.Counter(l.reason for l in lits if l.cls == 'other').most_common():
        print(f'  {c:5}  {r}')


def check(lits, out, old):
    want = render(lits, old)
    bad = 0
    for name, text in want.items():
        p = os.path.join(out, name)
        have = open(p, encoding='utf-8').read() if os.path.exists(p) else None
        if have != text:
            print(f'{name}: {"missing" if have is None else "out of date"} (run port/tools/strings.py)')
            bad += 1
    keys = {l.key() for l in lits if l.cls == 'displayed'}
    for k in old:
        if k not in keys:
            print(f'translation.tsv: stale key {k[:24]}...')
            bad += 1
    for l in lits:
        if l.cls == 'displayed' and old.get(l.key()):
            if specs(unesc(old[l.key()])) != specs(l.text):
                print(f'translation.tsv: {l.file}:{l.line}: printf conversions differ: '
                      f'{specs(l.text)} vs {specs(unesc(old[l.key()]))}')
                bad += 1
            try:
                unesc(old[l.key()]).encode('cp1252')
            except UnicodeEncodeError:
                print(f'translation.tsv: {l.file}:{l.line}: characters outside Windows-1252')
                bad += 1
    print('strings: in sync' if not bad else f'strings: {bad} problem(s)')
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--src', default=os.path.join(ROOT, 'src'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'port', 'data', 'strings'))
    ap.add_argument('--summary', action='store_true', help='print counts per class; write nothing')
    ap.add_argument('--check', action='store_true', help='compare the committed files with a fresh scan; write nothing')
    args = ap.parse_args()
    lits = scan(args.src)
    old = load_translations(os.path.join(args.out, 'translation.tsv'))
    if args.summary:
        summary(lits)
        return 0
    if args.check:
        return 1 if check(lits, args.out, old) else 0
    os.makedirs(args.out, exist_ok=True)
    for name, text in render(lits, old).items():
        with open(os.path.join(args.out, name), 'w', encoding='utf-8', newline='\n') as f:
            f.write(text)
    summary(lits)
    stale = [k for k in old if k not in {l.key() for l in lits if l.cls == 'displayed'}]
    if stale:
        print(f'warning: {len(stale)} translation key(s) no longer in the source were dropped')
    return 0


if __name__ == '__main__':
    sys.exit(main())
