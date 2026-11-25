#!/usr/bin/env python3
"""Checks that the packet and file structures have, on arm64 macOS, the size
and field offsets they had in the 32-bit Windows build (step M2.8).

  layoutcheck.py [--tree DIR] [--entry FILE ...] [-v] [--discover]
  layoutcheck.py --live [--ref-tree DIR] [--write-reference] ...
  layoutcheck.py --self-test

  --tree      the source tree compiled for arm64 (default: src)
  --live      compile the 32-bit Windows side now, with the Wine headers
              (BF_WIN_HEADERS; port/README.md says how to get them back from
              history), instead of reading it from the reference
  --ref-tree  with --live: the source tree compiled for 32-bit Windows
              (default: --tree). Give the untouched tree here to check a
              rewritten one against the original text, not only against
              itself.
  --write-reference  with --live: also write the Windows side to the
              reference, REFERENCE below
  --entry     check only these entries (paths as in ENTRIES below)
  -v          list every differing field, not only the first of each record
  --discover  list the types the readers fill from file buffers and say which
              of them no entry covers; checks nothing

Each entry is compiled with -fsyntax-only as arm64 code with the flags of
the port's build (port/tools/flags.sh: BF_TARGET, BF_DEFINES, BF_INCLUDES),
and compared with the same entry compiled as 32-bit Windows code
(-target i386-pc-windows-msvc, which gives long 4 bytes, double and long long
8-byte alignment and the Visual C++ rules for #pragma pack and bit-fields).
The Windows side is read from REFERENCE (port/tools/layout-win32.ref), which
--live --write-reference wrote with the Wine headers before M11.11 removed
them, from the original text: --ref-tree build/port/layout/reftree/src, made
with `git archive 454ff09 src | tar -x -C build/port/layout/reftree`, the
last tree whose src/ compiled for 32-bit Windows without the native modules'
headers and the BSD socket headers that M11.1-M11.9 put in it. --live
compiles the Windows side again. A header entry is compiled as a generated
unit that includes common.h and the header; a .cpp entry is compiled as it
is. clang dumps the layout of every complete record (-Xclang
-fdump-record-layouts-complete); the two dumps are parsed and compared
record by record: size, alignment, the set of fields, and the offset (and
bit range) of every field, nested ones included.

The Windows compile defines what BF_DEFINES defines except BF_PORT, so it
sees the original code paths. It is given the Windows includes src/common.h
had before M11.11 (WIN_PRELUDE, force-included, and an empty bf_types.h),
reads the C headers of the same Wine tree (BF_WIN_CRT_HEADERS, default: the
msvcrt directory beside BF_WIN_HEADERS) and the SDK's libc++ with a
configuration for that target; tool-made shims for what that combination
lacks live in build/port/layout/shim. Compiler errors in system headers are
expected there and only counted.

Which records count. A record belongs to an entry when the entry's file
defines it (named records: a struct/class/union definition in the file's
text; unnamed ones: clang's location). Of those, the records "on the path"
are every record of the pure packet headers, and in the other entries the
records named in ENTRIES: the types the readers fill straight from a file or
packet buffer. Any difference in a record on the path fails the check (exit
1), and so does a path record that only one compile produced. A record of a
pure packet header that holds a pointer is not a wire record (the client
sends and parses raw bytes, so a pointer cannot travel); it is taken off the
path and listed in a section of its own. A named path record that holds a
pointer stays on the path and fails if it differs. All other records of
the entries are compared too and reported, split into those that hold
pointers, references, a vtable or library containers (they legitimately
differ) and plain ones (worth a look, but no failure).

How the path set was found: every call of Read, GetCurBuf, memcpy, fread and
fwrite with a sizeof(T) argument, and every cast of a reader's buffer to T*,
in the client's sources; --discover repeats that search. Covered formats:
packets (packet.h, PaddingPacket.h, device/GronPacket.h and the packet
structures that LoginMode, GameMode2, GameModePacket and GuildInfo.h define
outside those headers), GRF
(base/GPak.cpp), ACT (resource/Action.*), SPR (resource/Sprite.cpp), PAL and
BMP/TGA (resource/Bitmap.cpp and the Windows records BITMAPFILEHEADER,
BITMAPINFOHEADER, BITMAPINFO, RGBQUAD, PALETTEENTRY, RECT), GAT
(resource/Attr.h), GND (resource/Ground.h), RSW (resource/World.*), RSM
(3dlib/Prim.h, Volume.h, Anim.h), STR (resource/EZeffect.h) and the Thai
dictionary (framework/ftwbrk.h). IMF files are read one int at a time and
need no record. Granny and Bink files are read by their libraries. Left out
on purpose: GameGuard's _GG_AUTH_DATA (base/FindHack.cpp, removed in M2.5)
and the vertex records tlvertex3d and lmtlvertex3d, which are copied in
memory only (they are compared and reported as other records of Prim.h).

Exit status: 0 no difference on the path, 1 differences, 2 the tool could
not do its work (a compile stopped with a fatal error, a file is missing).
The dumps are kept in build/port/layout/{win32,arm64}/.
"""
import argparse, concurrent.futures, json, os, re, shutil, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = os.path.join(ROOT, 'build', 'port', 'layout')
REFERENCE = os.path.join(ROOT, 'port', 'tools', 'layout-win32.ref')

# What src/common.h included from the Windows headers before M11.11 put
# bf_types.h there: the Windows side of a --live run starts with it.
WIN_PRELUDE = """#define DIRECTINPUT_VERSION 0x700
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#define D3D_OVERLOADS
#include <d3d.h>
#include <dsound.h>
#include <dinput.h>
#include <shlwapi.h>
"""

ALL = None   # every record the entry defines is on the path

# Headers that the client only ever includes after another one, which
# defines what they use (PaddingPacket.h needs TALKBOX_CHAT_LENGTH).
PRELUDE = {'PaddingPacket.h': ['packet.h']}

# (file relative to the tree, group, records on the path)
ENTRIES = [
    ('packet.h', 'packet', ALL),
    ('PaddingPacket.h', 'packet', ALL),
    ('device/GronPacket.h', 'packet', ALL),
    ('LoginMode.h', 'packet', ['BILLING_INFO']),
    ('LoginMode.cpp', 'packet', ['PACKET_HC_ACCEPT_ENTER', 'PACKET_HC_BLOCK_CHARACTER']),
    ('GameMode2.cpp', 'packet', ALL),
    ('GameModePacket.cpp', 'packet', ['STRUCT_FRIEND']),
    ('GuildInfo.h', 'packet', ['MEMBER_POSITION_INFORMATION']),
    ('base/GPak.cpp', 'file', ['pak_file_header', 'pak_data_header', 'pak_info_header']),
    ('resource/Action.h', 'file', ['CSprClip', 'CAttachPointInfo']),
    ('resource/Action.cpp', 'file', ['CSprClip_v11', 'CSprOldClip', 'FileHeader']),
    ('resource/Sprite.cpp', 'file', ['FileHeader', 'SprChunk']),
    ('resource/Bitmap.cpp', 'file', ['TargaHeader']),
    ('resource/Attr.h', 'file', ['CAttrCell']),
    ('resource/Ground.h', 'file', ['CLMInfo', 'LMIndex', 'ColorChannel', 'GndSurfaceFmt', 'GndCellFmt17']),
    ('resource/World.h', 'file', ['C3dWorldRes::effectSrcInfo', 'C3dWorldRes::lightSrcInfo',
                                     'C3dWorldRes::soundSrcInfo', 'C3dWorldRes::soundSrcInfo_V19']),
    ('resource/World.cpp', 'file', ['tmpActorInfo']),
    ('resource/EZeffect.h', 'file', ['KAC_XFORMDATA', 'KAC_KEYFRAME']),
    ('3dlib/Prim.h', 'file', ['vector3d', 'texCoor', 'tvertex3d', 'face3d', 'face3d_v11', 'matrix']),
    ('3dlib/Volume.h', 'file', ['CVolumeBox_v11', 'CVolumeBox_v21', 'CVolumeBox']),
    ('3dlib/Anim.h', 'file', ['posKeyframe', 'rotKeyframe', 'scaleKeyframe']),
    ('framework/ftwbrk.h', 'file', ['_DICTHEADER', '_DICTVERSION']),
    # The rest of the resource headers: nothing in them is read from a file
    # as a whole record, but their records are compared and reported.
    ('resource/Sprite.h', 'file', []),
    ('resource/PaletteRes.h', 'file', []),
    ('resource/ImfRes.h', 'file', []),
    ('resource/Bitmap.h', 'file', []),
    ('resource/Emblem.h', 'file', []),
    ('resource/Model.h', 'file', []),
    ('resource/Wave.h', 'file', []),
    ('resource/Skybox.h', 'file', []),
    # Windows records that readers fill from files, found by name anywhere.
    ('<windows.h>', 'file', ['tagRECT', 'tagRGBQUAD', 'tagPALETTEENTRY', 'BITMAPFILEHEADER',
                             'BITMAPINFOHEADER', 'tagBITMAPINFO']),
]

# Readers searched by --discover.
READERS = r'(resource|base|3dlib|framework)/[^/]+\.cpp$|^(GameWorld|3dGround|3dActor|GameMode\w*|LoginMode|packet|Session\w*)\.cpp$'


# ---------------------------------------------------------------- flags

def port_flags():
    """The variables of flags.sh, split the way census.sh's unquoted
    expansion splits them."""
    script = ('. "$ROOT/port/tools/flags.sh"; printf "%s\\n" "${WIN_HEADERS:-}" '
              '"$BF_TARGET" "$BF_DEFINES" "$BF_INCLUDES"')
    env = dict(os.environ, ROOT=ROOT)
    out = subprocess.run(['bash', '-c', script], env=env, check=True,
                         capture_output=True, text=True).stdout.split('\n')
    return out[0], out[1].split(), out[2].split(), out[3].split()


def make_shims(sdk):
    """Fills what the SDK's libc++ and the Wine C headers lack for an
    i386 Visual C++ target. 'cxx' goes ahead of libc++, 'fill' after
    everything else, so a real header always wins."""
    cxx = os.path.join(OUT, 'shim', 'cxx'); fill = os.path.join(OUT, 'shim', 'fill')
    os.makedirs(os.path.join(OUT, 'shim', 'win'), exist_ok=True)
    os.makedirs(cxx, exist_ok=True); os.makedirs(os.path.join(fill, 'wine'), exist_ok=True)
    site = open(os.path.join(sdk, 'usr/include/c++/v1/__config_site')).read()
    site = re.sub(r'#define _LIBCPP_HAS_VENDOR_AVAILABILITY_ANNOTATIONS 1',
                  '#define _LIBCPP_HAS_VENDOR_AVAILABILITY_ANNOTATIONS 0', site)
    site = re.sub(r'#define _LIBCPP_HAS_THREADS 1', '#define _LIBCPP_HAS_THREADS 0', site)
    site = re.sub(r'#define _LIBCPP_PSTL_BACKEND_LIBDISPATCH', '#define _LIBCPP_PSTL_BACKEND_SERIAL', site)
    site = site.replace('/* #undef _LIBCPP_NO_VCRUNTIME */', '#define _LIBCPP_NO_VCRUNTIME')
    files = {
        os.path.join(cxx, '__config_site'): site,
        os.path.join(fill, 'wine', '32on64utils.h'): '#ifndef HOSTPTR\n#define HOSTPTR\n#endif\n',
        os.path.join(fill, 'wine', 'asm.h'): '/* layoutcheck: nothing the record layouts need */\n',
        os.path.join(OUT, 'shim', 'win', 'bf_types.h'): '/* layoutcheck: the Windows headers declare it all (win_prelude.h) */\n',
        os.path.join(fill, 'win_prelude.h'): WIN_PRELUDE,
    }
    for path, text in files.items():
        if not os.path.exists(path) or open(path).read() != text:
            open(path, 'w').write(text)
    return cxx, fill


def compilers(tree, ref_tree, mode):
    """The flags of each side. mode: 'reference' (arm64 only), 'live' (the
    Windows side with the Wine headers), 'bare' (the Windows side with no
    headers at all, for the self-test)."""
    build_win_headers, target, defines, includes = port_flags()
    dump = ['-fsyntax-only', '-ferror-limit=0', '-Xclang', '-fdump-record-layouts-complete']
    arm = target + dump + defines + includes + ['-I' + tree]
    if build_win_headers:
        arm += ['-isystem', build_win_headers]
    win_target = (['-target', 'i386-pc-windows-msvc', '-fms-extensions', '-fms-compatibility',
                   '-fshort-wchar', '-Wno-everything'] + dump
                  + [d for d in defines if d != '-DBF_PORT'])
    if mode == 'reference':
        return {'arm64': arm}
    if mode == 'bare':
        return {'win32': win_target + ['-nostdinc', '-nostdinc++', '-I' + ref_tree], 'arm64': arm}
    win_headers = os.environ.get('BF_WIN_HEADERS') or build_win_headers
    if not os.path.isfile(os.path.join(win_headers, 'windows.h')):
        sys.exit(f'layoutcheck: no windows.h in {win_headers}; set BF_WIN_HEADERS (port/README.md)')
    crt = os.environ.get('BF_WIN_CRT_HEADERS')
    if not crt:
        for cand in (os.path.join(os.path.dirname(win_headers), 'msvcrt'),
                     os.path.join(win_headers, 'msvcrt'), os.path.join(win_headers, '..', 'msvcrt')):
            if os.path.isfile(os.path.join(cand, 'stdio.h')):
                crt = cand; break
    if not crt or not os.path.isfile(os.path.join(crt, 'stdio.h')):
        sys.exit(f'layoutcheck: no Windows C headers beside {win_headers}; set BF_WIN_CRT_HEADERS')
    sdk = subprocess.run(['xcrun', '--show-sdk-path'], check=True, capture_output=True,
                         text=True).stdout.strip()
    cxx, fill = make_shims(sdk)
    win = (win_target
           # A name of the DirectX 7 SDK that the Wine headers lack (bf_prefix.h
           # supplied it to the arm64 build).
           + ['-D_D3DPRIMITIVETYPE=D3DPRIMITIVETYPE']
           + ['-nostdinc++', '-isystem', cxx, '-isystem', os.path.join(sdk, 'usr/include/c++/v1'),
              '-isystem', crt, '-isystem', win_headers, '-isystem', fill, '-I' + ref_tree,
              # The native modules' headers, which src/ includes outright since M11
              # (bf_sys.h, bf_render.h, ...); bf_types.h is the empty shim.
              '-I' + os.path.join(OUT, 'shim', 'win')]
           + [i for i in includes if i.startswith('-I') and not i.endswith('port/compat/include')]
           + ['-include', os.path.join(fill, 'win_prelude.h')])
    return {'win32': win, 'arm64': arm}


# ---------------------------------------------------------------- dumps

HEAD = re.compile(r'^\s*\d+ \| (?:(struct|class|union) )?(.*?)(?: \(empty\))?$')
LINE = re.compile(r'^\s*(\d+)(?::(\d+)-(\d+))? \|( +)(.*)$')
SIZE = re.compile(r'\[sizeof=(\d+),(?: dsize=\d+,)? align=(\d+)')
UNNAMED = re.compile(r'\((?:unnamed|anonymous)(?: (?:struct|union|class))? at (.*?):(\d+):(\d+)\)')
PTRTYPE = re.compile(r'[*&]|\bstd::|^(const )?(H[A-Z]+|LP[A-Z0-9_]+|P[A-Z][A-Z0-9_]*|HANDLE|BSTR)$')


def split_scope(name):
    """'A<x::y>::B::(unnamed at f:1:2)' -> ['A<x::y>', 'B', '(unnamed at f:1:2)']"""
    parts, depth, cur, i = [], 0, '', 0
    while i < len(name):
        c = name[i]
        if c in '<(': depth += 1
        elif c in '>)': depth -= 1
        if depth == 0 and name.startswith('::', i):
            parts.append(cur); cur = ''; i += 2; continue
        cur += c; i += 1
    return parts + [cur]


def parse_dump(text):
    """Returns {record name: {'size', 'align', 'fields': {path: (offset, bits, type)},
    'ptr': bool, 'children': [record type names of fields]}}."""
    records = {}
    for block in text.split('*** Dumping AST Record Layout')[1:]:
        lines = [l for l in block.split('\n') if l.strip()]
        m = HEAD.match(lines[0])
        if not m:
            continue
        name = m.group(2)
        fields, stack, ptr, children, count = {}, [], False, [], {}
        size = align = None
        for line in lines[1:]:
            s = SIZE.search(line)
            if s:
                size, align = int(s.group(1)), int(s.group(2)); break
            f = LINE.match(line)
            if not f:
                continue
            depth = (len(f.group(4)) - 1) // 2
            body = f.group(5).strip()
            if body.endswith('vtable pointer)') or body.endswith('vftable pointer)') \
                    or body.startswith('(vbtable pointer') or body.startswith('(vfptr'):
                ptr = True; ftype, fname = 'vptr', '(vptr)'
            elif re.search(r'\((primary |virtual )?base\)( \(empty\))?$', body):
                ftype = re.sub(r' \((primary |virtual )?base\).*$', '', body)
                fname = '(base %s)' % ftype
            else:
                body = re.sub(r' \(empty\)$', '', body)
                if body.endswith(')') and UNNAMED.search(body):
                    ftype, fname = body, '(anon)'
                else:
                    ftype, _, fname = body.rpartition(' ')
            # A record-typed field or base (struct X, or a base class) is a
            # record: whether it holds pointers comes from its own fields
            # below. Only plain types are matched against the pointer
            # typedefs, which would otherwise take PACKET_... for PVOID-like.
            is_record = fname.startswith('(base ') or re.match(r'^(const )?(struct|class|union) ', ftype)
            if '*' in ftype or '&' in ftype or (not is_record and PTRTYPE.search(ftype)):
                ptr = True
            del stack[depth - 1:]
            parent = '.'.join(stack)
            key = (parent, fname)
            count[key] = count.get(key, 0) + 1
            if count[key] > 1:
                fname = '%s#%d' % (fname, count[key])
            stack.append(fname)
            bits = (int(f.group(2)), int(f.group(3))) if f.group(2) else None
            fields['.'.join(stack)] = (int(f.group(1)), bits, ftype)
            children.append(re.sub(r'^(const )?(struct |class |union )?|(\[\d+\])+$', '', ftype))
        if size is None:
            continue
        records.setdefault(name, {'size': size, 'align': align, 'fields': fields,
                                  'ptr': ptr, 'children': children})
    # A record holds pointers when it or a record-typed field does.
    changed = True
    while changed:
        changed = False
        for r in records.values():
            if not r['ptr'] and any(records.get(c, {}).get('ptr') for c in r['children']):
                r['ptr'] = changed = True
    return records


# ---------------------------------------------------------------- attribution

def strip_comments(text):
    def keep_lines(m):
        return re.sub(r'[^\n]', ' ', m.group(0))
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'',
                  keep_lines, text, flags=re.S)


DEFN = re.compile(r'\b(?:struct|class|union)\s+(?:__declspec\s*\([^)]*\)\s*)?([A-Za-z_]\w*)\s*(?::[^;{}()]*)?\{')


def defined_names(path):
    return set(DEFN.findall(strip_comments(open(path, errors='replace').read())))


_lines = {}
def declarator(path, line, col):
    """The typedef or variable name after the closing brace of the unnamed
    record that starts at path:line:col, or None."""
    if path not in _lines:
        try:
            _lines[path] = strip_comments(open(path, errors='replace').read()).split('\n')
        except OSError:
            _lines[path] = []
    src = _lines[path]
    if line > len(src):
        return None
    text = '\n'.join([src[line - 1][col - 1:]] + src[line:line + 400])
    start = text.find('{')
    if start < 0:
        return None
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '{': depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                m = re.match(r'\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?\*?\s*([A-Za-z_]\w*)', text[i + 1:])
                return m.group(1) if m else None
    return None


def attribute(records, tree, entry, names):
    """The records of the dump that belong to the entry, under a name that is
    the same in both compiles: {name: record}."""
    out = {}
    if entry.startswith('<'):
        for name, r in records.items():
            scope = split_scope(name)
            u = UNNAMED.fullmatch(scope[-1])
            short = declarator(os.path.normpath(u.group(1)), int(u.group(2)), int(u.group(3))) if u else name
            if short in names and short not in out:
                out[short] = r
        return out
    path = os.path.normpath(os.path.join(tree, entry))
    defined = defined_names(path)
    for name, r in records.items():
        scope = split_scope(name)
        if scope[0].startswith('std'):
            continue
        u = UNNAMED.fullmatch(scope[-1])
        if u:
            if os.path.normpath(u.group(1)) != path:
                continue
            decl = declarator(path, int(u.group(2)), int(u.group(3)))
            scope[-1] = decl if decl else '(unnamed@%s)' % u.group(2)
        elif re.sub(r'<.*$', '', scope[0]) not in defined:
            continue
        key = '::'.join(scope)
        n = 2
        while key in out:
            key = '%s#%d' % ('::'.join(scope), n); n += 1
        out[key] = r
    return out


# ---------------------------------------------------------------- compare

def compare(a, b):
    """Differences between the Windows record a and the arm64 record b, as
    (field, Windows text, arm64 text), and the field that most likely caused
    them: the last leaf field before the first shift (or the last leaf field
    when only the size differs), as 'path (type)'."""
    diffs = []
    if a['size'] != b['size']:
        diffs.append(('sizeof', a['size'], b['size']))
    if a['align'] != b['align']:
        diffs.append(('alignof', a['align'], b['align']))
    def show(f):
        return '%d' % f[0] if not f[1] else '%d:%d-%d' % (f[0], f[1][0], f[1][1])
    cause, leaves, paths = None, [], list(a['fields'])
    for i, path in enumerate(paths):
        f, g = a['fields'][path], b['fields'].get(path)
        if g is None:
            diffs.append((path, show(f), 'missing'))
        elif (f[0], f[1]) != (g[0], g[1]):
            diffs.append((path, show(f), show(g)))
            if cause is None:
                cause = list(leaves)
        is_leaf = i + 1 == len(paths) or not paths[i + 1].startswith(path + '.')
        if is_leaf and g is not None:
            leaves.append(path)
    for path, g in b['fields'].items():
        if path not in a['fields']:
            diffs.append((path, 'missing', show(g)))
    if cause is None and diffs:
        cause = leaves
    if cause:
        # The last leaf, and the members of a union that share its offset.
        last = a['fields'][cause[-1]][0]
        same = [p for p in cause if a['fields'][p][0] == last and not a['fields'][p][1]] or cause[-1:]
        cause = ('one of ' if len(same) > 1 else '') + ', '.join(
            '%s (%s)' % (p, b['fields'][p][2]) for p in same[-4:])
    return diffs, cause


def compile_entry(job):
    side, flags, tree, entry = job
    unit_dir = os.path.join(OUT, side)
    os.makedirs(unit_dir, exist_ok=True)
    stem = entry.strip('<>').replace('/', '_')
    if entry.endswith('.cpp'):
        source = os.path.join(tree, entry)
    else:
        source = os.path.join(unit_dir, stem + '.unit.cpp')
        body = '#include "common.h"\n'
        if not entry.startswith('<'):
            for first in PRELUDE.get(entry, []):
                body += '#include "%s"\n' % first
            body += '#include "%s"\n' % entry
        open(source, 'w').write(body)
    if not os.path.isfile(source):
        return side, entry, None, ['missing source %s' % source]
    p = subprocess.run(['clang++', '-std=gnu++14'] + flags + [source],
                       capture_output=True, text=True, errors='replace')
    open(os.path.join(unit_dir, stem + '.layout.txt'), 'w').write(p.stdout)
    open(os.path.join(unit_dir, stem + '.log'), 'w').write(p.stderr)
    errors = [l for l in p.stderr.split('\n') if re.search(r' (fatal )?error: ', l)]
    return side, entry, p.stdout, errors


# ---------------------------------------------------------------- discover

def discover(tree):
    found = {}
    pat = re.compile(r'(?:Read|GetCurBuf|memcpy|fread|fwrite)\s*\([^;]*?sizeof\s*\(\s*([A-Za-z_]\w*)\s*\)'
                     r'|\(\s*(?:const\s+)?([A-Za-z_]\w*)\s*\*\s*\)\s*(?:m_fp\.|fp\.|fp->)?(?:GetCurBuf|pFileIter|bitmapiter|buf\b)')
    for base, _, files in os.walk(tree):
        for name in files:
            rel = os.path.relpath(os.path.join(base, name), tree)
            if not re.search(READERS, rel):
                continue
            for n, line in enumerate(strip_comments(open(os.path.join(base, name), errors='replace').read()).split('\n'), 1):
                for m in pat.finditer(line):
                    t = m.group(1) or m.group(2)
                    found.setdefault(t, []).append('%s:%d' % (rel, n))
    covered = {n.split('::')[-1] for _, _, names in ENTRIES for n in (names or [])}
    for entry, _, names in ENTRIES:
        if names is ALL:
            covered |= defined_names(os.path.join(tree, entry))
            # typedef names of the whole-file packet headers
            covered |= set(re.findall(r'\}\s*([A-Za-z_]\w*)\s*[,;]',
                                      strip_comments(open(os.path.join(tree, entry), errors='replace').read())))
    scalar = {'int', 'float', 'double', 'char', 'short', 'long', 'BYTE', 'WORD', 'DWORD', 'BOOL',
              'bool', 'unsigned', 'void', 'byte', 'UINT', 'USHORT', 'ULONG', 'LONG', 'INT', 'float32'}
    def is_covered(t):
        return any(c in covered for c in (t, 'tag' + t, '_' + t))
    shown = 0
    for t in sorted(found):
        if t in scalar or is_covered(t):
            continue
        shown += 1
        print('%-28s %s' % (t, ', '.join(found[t][:4]) + (' ...' if len(found[t]) > 4 else '')))
    print('\n%d types found, %d covered by ENTRIES or by the whole-file packet headers, %d listed above.\n'
          'A listed name is a variable, a buffer that is not a record, or a gap to add to ENTRIES.'
          % (len(found), len(found) - shown, shown))


# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--tree', default=os.path.join(ROOT, 'src'))
    ap.add_argument('--ref-tree')
    ap.add_argument('--entry', action='append')
    ap.add_argument('-v', '--verbose', action='store_true')
    ap.add_argument('--discover', action='store_true')
    ap.add_argument('--self-test', action='store_true')
    ap.add_argument('--live', action='store_true')
    ap.add_argument('--write-reference', action='store_true')
    args = ap.parse_args()
    if (args.ref_tree or args.write_reference) and not args.live:
        ap.error('--ref-tree and --write-reference need --live')
    tree = os.path.abspath(args.tree)
    ref_tree = os.path.abspath(args.ref_tree or args.tree)
    if args.discover:
        discover(tree); return 0
    if args.self_test:
        return self_test()
    entries = [e for e in ENTRIES if not args.entry or e[0] in args.entry]
    if not entries:
        sys.exit('layoutcheck: no such entry')
    if args.write_reference and args.entry:
        ap.error('--write-reference writes every entry; leave out --entry')
    mode = 'live' if args.live else 'reference'
    return check(tree, ref_tree, entries, args.verbose, mode, args.write_reference)[0]


# The self-test needs no Windows header on either side: common.h gives the
# Windows side its own DWORD and 32-bit time_t (it is compiled -nostdinc).
SELFTEST_COMMON = r'''
#ifdef _WIN32
typedef unsigned long DWORD;
typedef long time_t;
#else
#include "bf_types.h"
#include <time.h>
#endif
'''
SELFTEST = r'''
#pragma pack(push, 1)
struct ST_SAME    { short type; DWORD id; char name[24]; unsigned char b : 3, c : 5; };
struct ST_LONG    { short type; unsigned long id; short x; };       /* 8 -> 12 */
struct ST_TIME    { short type; time_t when; short x; };            /* 8 -> 12 */
struct ST_NESTED  { short type; struct ST_LONG inner; char tail; }; /* via ST_LONG */
struct ST_POINTER { short type; char *text; };                      /* not a wire record */
#pragma pack(pop)
struct ST_ALIGN   { char c; double d; };                            /* 16 both */
struct ST_BITS    { unsigned int a : 4; char b; };                  /* MSVC b at 4, arm64 at 1 */
struct ST_LLONG   { char c; long long q; };                         /* 16 both */
typedef struct { short type; long v; } ST_ANON;                     /* 8 -> 16 */
'''


def self_test():
    """Runs the check on a synthetic header whose answers are known."""
    tree = os.path.join(OUT, 'selftest')
    os.makedirs(tree, exist_ok=True)
    open(os.path.join(tree, 'common.h'), 'w').write(SELFTEST_COMMON)
    open(os.path.join(tree, 'selftest.h'), 'w').write(SELFTEST)
    code, failing, pointer = check(tree, tree, [('selftest.h', 'packet', ALL)], True, 'bare', False)
    want_fail = {'ST_LONG', 'ST_TIME', 'ST_NESTED', 'ST_ANON', 'ST_BITS'}
    ok = code == 1 and set(failing) == want_fail and set(pointer) == {'ST_POINTER'}
    print('\nself-test: failing %s, pointer records %s: %s'
          % (sorted(failing), sorted(pointer), 'OK' if ok else 'WRONG, expected %s' % sorted(want_fail)))
    return 0 if ok else 2


def read_reference():
    """{entry: (records, compiler errors)} of the Windows side."""
    if not os.path.isfile(REFERENCE):
        sys.exit('layoutcheck: no %s; write it with --live --write-reference' % REFERENCE)
    ref = {}
    for line in open(REFERENCE):
        if line.startswith('#') or not line.strip():
            continue
        r = json.loads(line)
        recs, _ = ref.setdefault(r['entry'], ({}, r.get('errors', '')))
        if 'name' in r:
            recs[r['name']] = {'size': r['size'], 'align': r['align'], 'ptr': r['ptr'],
                               'fields': {f[0]: (f[1], tuple(f[2]) if f[2] else None, f[3])
                                          for f in r['fields']}}
    return ref


def write_reference(sides, ref_tree):
    """sides: [(entry, records, compiler errors)], one JSON object per line."""
    with open(REFERENCE, 'w') as f:
        f.write('# The 32-bit Windows side of layoutcheck.py: every record of each entry as\n'
                '# the Visual C++ rules lay it out (clang, i386-pc-windows-msvc, the Wine\n'
                '# headers), written by layoutcheck.py --live --write-reference before\n'
                '# M11.11 removed the Wine headers. One line per entry and per record.\n'
                '# Windows side compiled from: %s\n' % (os.path.relpath(ref_tree, ROOT)
                                                       if ref_tree.startswith(ROOT) else ref_tree))
        for entry, recs, errors in sides:
            f.write(json.dumps({'entry': entry, 'errors': errors}) + '\n')
            for name in sorted(recs):
                r = recs[name]
                f.write(json.dumps({'entry': entry, 'name': name, 'size': r['size'], 'align': r['align'],
                                    'ptr': r['ptr'], 'fields': [[p, v[0], list(v[1]) if v[1] else None, v[2]]
                                                                for p, v in r['fields'].items()]}) + '\n')
    print('layoutcheck: wrote %s' % os.path.relpath(REFERENCE, ROOT))


def check(tree, ref_tree, entries, verbose, mode, write):
    """Returns (exit status, names failing, names of pointer records taken off the path)."""
    flags = compilers(tree, ref_tree, mode)
    reference = read_reference() if mode == 'reference' else None
    for side in flags:
        shutil.rmtree(os.path.join(OUT, side), ignore_errors=True)
    jobs = [(side, flags[side], ref_tree if side == 'win32' else tree, e[0])
            for e in entries for side in flags]
    results = {}
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as pool:
        for side, entry, dump, errors in pool.map(compile_entry, jobs):
            results[side, entry] = (dump, errors)

    fatal = False
    failing, pointer_packet, other_ptr, other_plain = [], [], [], []
    print('%-22s %6s %6s %6s %6s   %s' % ('entry', 'path', 'differ', 'other', 'differ', 'compiler errors win32/arm64 (in the tree)'))
    totals = [0, 0, 0, 0]
    written = []
    for entry, group, names in entries:
        recs, errs = {}, {}
        if reference is not None:
            if entry not in reference:
                print('%s: not in %s; write it with --live --write-reference'
                      % (entry, os.path.relpath(REFERENCE, ROOT)), file=sys.stderr)
                fatal = True
            recs['win32'], errs['win32'] = reference.get(entry, ({}, '-'))
        for side, tr in (('win32', ref_tree), ('arm64', tree)):
            if side not in flags:
                continue
            dump, errors = results[side, entry]
            fatal_lines = [l for l in errors if 'fatal error' in l or l.startswith('missing source')]
            if dump is None or fatal_lines:
                print('%s: %s compile stopped: %s' % (entry, side, (fatal_lines or errors)[0]), file=sys.stderr)
                fatal = True
            # An error inside a header entry itself means some of its records
            # were not compiled at all, so they would go unchecked.
            own = [l for l in errors if not entry.endswith('.cpp') and l.startswith(os.path.join(tr, entry) + ':')]
            if own:
                print('%s: %s errors in the header itself, its records are not all checked: %s' % (entry, side, own[0]), file=sys.stderr)
                fatal = True
            in_tree = sum(1 for l in errors if l.startswith(tr))
            errs[side] = '%d (%d)' % (len(errors), in_tree)
            recs[side] = attribute(parse_dump(dump or ''), tr, entry, names or [])
        win, arm = recs['win32'], recs['arm64']
        written.append((entry, win, errs['win32']))
        on_path = lambda n: names is ALL or n in names
        counts = [0, 0, 0, 0]
        for name in sorted(set(win) | set(arm)):
            # In the pure packet headers, a record that holds a pointer cannot
            # be a wire record: it is client bookkeeping and leaves the path.
            path = on_path(name) and not (names is ALL and (win.get(name) or arm.get(name))['ptr'])
            counts[0 if path else 2] += 1
            if name not in win or name not in arm:
                d = [('(record)', 'present' if name in win else 'missing', 'present' if name in arm else 'missing')]
                cause = None
            else:
                d, cause = compare(win[name], arm[name])
            if not d:
                continue
            counts[1 if path else 3] += 1
            item = (entry, name, d, (win.get(name) or arm.get(name))['ptr'], cause)
            if not path and on_path(name):
                pointer_packet.append(item)
            elif path:
                failing.append(item)
            elif item[3]:
                other_ptr.append(item)
            else:
                other_plain.append(item)
        if names not in (ALL, []):
            for n in names:
                if n not in win and n not in arm:
                    print('%s: %s is listed but neither compile defines it' % (entry, n), file=sys.stderr)
                    fatal = True
        totals = [t + c for t, c in zip(totals, counts)]
        print('%-22s %6d %6d %6d %6d   %s / %s' % (entry, *counts, errs['win32'], errs['arm64']))
    print('%-22s %6d %6d %6d %6d' % ('total', *totals))

    def report(title, items, limit):
        if not items:
            return
        print('\n%s (%d):' % (title, len(items)))
        for entry, name, d, ptr, cause in items:
            size = next((x for x in d if x[0] == 'sizeof'), None)
            head = '  %s %s' % (entry, name)
            align = next((x for x in d if x[0] == 'alignof'), None)
            if size:
                head += ': sizeof %s -> %s' % (size[1], size[2])
            if align:
                head += '%s alignof %s -> %s' % (',' if size else ':', align[1], align[2])
            if ptr:
                head += ' [holds pointers]'
            if cause:
                head += '; shift starts after: ' + cause
            print(head)
            shown = [x for x in d if x[0] not in ('sizeof', 'alignof')]
            for field, w, a in (shown if limit is None else shown[:limit]):
                print('      %-40s win32 %-8s arm64 %s' % (field, w, a))
            if limit is not None and len(shown) > limit:
                print('      ... and %d more fields' % (len(shown) - limit))

    report('Differences on the packet and file path', failing, None if verbose else 1)
    report('Records of the packet headers that hold pointers, so are not wire records (no failure)',
           pointer_packet, None if verbose else 1)
    if verbose:
        report('Other records that differ and hold pointers, references, a vtable or containers (no failure)', other_ptr, 1)
    elif other_ptr:
        print('\nOther records that differ and hold pointers, references, a vtable or containers: %d (no failure; -v lists them)' % len(other_ptr))
    report('Other records that differ without holding pointers (no failure, worth a look)', other_plain, None if verbose else 1)

    names_failing = ', '.join(sorted({i[1] for i in failing})[:12])
    print('\nlayoutcheck: %s: %d records on the packet and file path, %d differ%s'
          % (os.path.relpath(tree, ROOT) if tree.startswith(ROOT) else tree, totals[0], totals[1],
             (' (%s%s)' % (names_failing, ', ...' if len(failing) > 12 else '')) if failing else ''))
    names = ([i[1] for i in failing], [i[1] for i in pointer_packet])
    if fatal:
        print('layoutcheck: incomplete, see the messages above', file=sys.stderr)
        return (2,) + names
    if write:
        write_reference(written, ref_tree)
    print('layoutcheck: %s' % ('FAIL' if failing else 'PASS'))
    return (1 if failing else 0,) + names


if __name__ == '__main__':
    sys.exit(main())
