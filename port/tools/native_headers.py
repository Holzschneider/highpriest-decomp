#!/usr/bin/env python3
"""Takes the Windows headers out of the client's include lines (M11.11).

  native_headers.py --apply [--build DIR] [--dry-run]
  native_headers.py --snapshot FILE [--build DIR] [-j N]
  native_headers.py --check FILE [--build DIR] [-j N]

--apply rewrites the include lines of every tracked file of src/ and
port/client that a unit of the build reads (Ninja's dependency log of
build/cmake/debug: build first), by these rules and nothing else:

  - `#include <windows.h>` becomes `#include "bf_types.h"`
    (port/client/include/bf_types.h: the types, structures and constants the
    client keeps), the line's own spacing and trailing text kept;
  - every other #include line naming a Windows or DirectX header (the names
    of port/tools/winheaders.txt: mmsystem.h, d3d.h, dinput.h, shellapi.h,
    tlhelp32.h, nb30.h, imm.h, ...), a CRT stand-in (io.h, direct.h,
    process.h, conio.h, minmax.h, mbstring.h) or a Windows library header
    copied into src/ (WinCrypt.h, ijl.h) is deleted, line and all;
  - the #define lines of DIRECTINPUT_VERSION, WIN32_LEAN_AND_MEAN and
    D3D_OVERLOADS, which only tell the Windows headers what to declare, are
    deleted.

Lines inside #if blocks are treated the same. Running it again changes
nothing.

--snapshot and --check prove that the rewrite changes no code: --snapshot,
run before, preprocesses every unit of compile_commands.json and records,
per unit and per file of src/ and port/ (outside port/compat), the lines it
produces with whitespace removed and hexadecimal numbers written in decimal
(the Windows headers spell some constants in hex, bf_types.h in decimal),
in order and without line numbers (the deleted lines move the rest up); and
`nm` of every object file of the build (name and type of each symbol).
--check, run after the rewrite and a build, does the same and lists every
difference; exit 0 when there is none. Not differences: bf_types.h's own
lines (it stands where the Windows headers stood, whose lines are not
recorded either; bifrost-types-test holds it to them), a line number after
the file's own name in a string (__LINE__ in assert and BF_STUB: the deleted
lines move it), assembler-local labels (ltmpN) and the static data of
Wine's __wine_uuidof templates. Symbols whose only change is their linkage
(a C name that became the C++ mangling of the same name: a function the
SDK's headers define inline, which the Wine headers had included inside
their extern "C" blocks) are listed on their own and not counted.
"""
import argparse
import concurrent.futures
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nowin  # noqa: E402

ROOT = nowin.ROOT
TREES = ('src/', 'port/client/')
INCLUDE = re.compile(r'^(\s*#\s*include\s*)([<"])([^">]+)([">])(.*)$')
DEFINES = re.compile(r'^\s*#\s*define\s+(DIRECTINPUT_VERSION|WIN32_LEAN_AND_MEAN|D3D_OVERLOADS)\b')
# Windows library headers copied into src/ (nowin.py's SDK_HEADERS and the
# copy of wincrypt.h), by the name the include lines use.
LIBRARY = {'wincrypt.h', 'ijl.h', 'dbghelp.h', 'npgamelib.h'}


def built_files(build):
    """Tracked files of TREES that a unit of the build reads."""
    p = subprocess.run(['ninja', '-C', build, '-t', 'deps'], capture_output=True, text=True)
    if p.returncode != 0:
        sys.exit('native_headers: ninja -C %s -t deps failed (build first)\n%s' % (build, p.stderr))
    read = set()
    for line in p.stdout.split('\n'):
        if line.startswith('    '):
            read.add(nowin.rel(os.path.join(build, line.strip())))
    tracked = subprocess.run(['git', 'ls-files', '--'] + list(TREES), cwd=ROOT, capture_output=True,
                             text=True, check=True).stdout.split()
    return sorted(t for t in tracked if nowin.rel(os.path.join(ROOT, t)) in read)


def own(f, m):
    """Whether the include names one of the client's own headers whose name a
    Windows header shares (nowin.py's OWN_HEADERS: device/Control.h)."""
    for base in (os.path.dirname(f), 'src'):
        here = os.path.join(ROOT, base, m.group(3))
        if m.group(2) == '"' and os.path.exists(here) and nowin.rel(here) in nowin.OWN_HEADERS:
            return True
    return False


def rewrite(f, lines, names):
    out, changes = [], []
    for n, line in enumerate(lines, 1):
        m = INCLUDE.match(line)
        if m:
            name = os.path.normpath(m.group(3).replace('\\', '/')).lower()
            base = os.path.basename(name)
            if name == 'windows.h':
                new = '%s"bf_types.h"%s' % (m.group(1), m.group(5))
                out.append(new)
                changes.append((n, line, new))
                continue
            if (name in names or base in LIBRARY or any(name.endswith('/' + w) for w in names if '/' in w)) \
                    and not own(f, m):
                changes.append((n, line, None))
                continue
        elif DEFINES.match(line):
            changes.append((n, line, None))
            continue
        out.append(line)
    return out, changes


def apply(build, dry):
    names = nowin.windows_names()
    total = files = 0
    for f in built_files(build):
        path = os.path.join(ROOT, f)
        raw = open(path, 'rb').read()
        text = raw.decode('latin-1')
        eol = '\r\n' if '\r\n' in text else '\n'
        lines = text.split(eol)
        out, changes = rewrite(f, lines, names)
        if not changes:
            continue
        files += 1
        total += len(changes)
        for n, old, new in changes:
            print('%s:%d: %s%s' % (f, n, old.strip(), ' -> ' + new.strip() if new else ' (deleted)'))
        if not dry:
            open(path, 'wb').write(eol.join(out).encode('latin-1'))
    print('native_headers: %d lines in %d files%s' % (total, files, ' (dry run)' if dry else ''))


# ---------------------------------------------------------------- snapshot

MARK = re.compile(r'^# (\d+) "([^"]+)"')


def norm(line):
    line = re.sub(r'\b0[xX]([0-9a-fA-F]+)([uUlL]*)\b', lambda m: str(int(m.group(1), 16)) + m.group(2), line)
    line = re.sub(r'\(\s*\(\s*BYTE\s*\)\s*(\d+)\s*\)', r'(BYTE)\1', line)
    return re.sub(r'\s+', '', line)


def preprocess(entry):
    args = nowin.unit_args(entry)[:-2]  # drop -M -MG
    p = subprocess.run(args + ['-E'], cwd=entry['directory'], capture_output=True, text=True,
                       errors='replace')
    unit = nowin.rel(os.path.join(entry['directory'], entry['file']))
    if p.returncode != 0:
        return unit, None, p.stderr[-2000:]
    files, cur = {}, None
    for line in p.stdout.split('\n'):
        m = MARK.match(line)
        if m:
            f = nowin.rel(os.path.join(entry['directory'], m.group(2)))
            cur = f if (f.startswith('src/') or f.startswith('port/')) and not f.startswith('port/compat/') \
                and f.lower() not in ('src/wincrypt.h', 'src/ijl.h') else None
            continue
        if cur and line.strip():
            files.setdefault(cur, []).append(norm(line))
    return unit, files, ''


def symbols(build):
    out = {}
    objs = []
    for top, _, fs in os.walk(build):
        objs += [os.path.join(top, f) for f in fs if f.endswith('.o')]
    for o in sorted(objs):
        p = subprocess.run(['nm', '-P', o], capture_output=True, text=True)
        out[os.path.relpath(o, build)] = sorted('%s %s' % tuple(l.split()[:2]) for l in p.stdout.split('\n')
                                                if len(l.split()) >= 2)
    return out


def snapshot(build, jobs):
    entries = json.load(open(os.path.join(build, 'compile_commands.json')))
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        results = list(pool.map(preprocess, entries))
    pp, failed = {}, []
    for unit, files, err in results:
        if files is None:
            failed.append((unit, err))
            continue
        for f, lines in files.items():
            pp.setdefault(unit, {})[f] = lines
    for unit, err in failed:
        sys.stderr.write('%s: preprocessing failed\n%s\n' % (unit, err))
    return {'pp': pp, 'nm': symbols(build), 'failed': [u for u, _ in failed]}


TYPES = 'port/client/include/bf_types.h'
LOCAL = re.compile(r'^ltmp\d+ |__wine_uuidof')


def same_lines(f, x, y):
    """The token streams of file f, a __LINE__ after f's own name aside."""
    if len(x) != len(y):
        return False
    pat = re.compile(r'("%s",)\d+' % re.escape(os.path.join(ROOT, f)))
    return all(a == b or pat.sub(r'\1N', a) == pat.sub(r'\1N', b) for a, b in zip(x, y))


def mangled(name):
    """'_tolower' -> 'tolower', '__Z7toloweri' -> 'tolower'."""
    m = re.match(r'__Z(\d+)', name)
    return name[len(m.group(0)):len(m.group(0)) + int(m.group(1))] if m else name[1:]


def check(path, build, jobs):
    before = json.load(open(path))
    after = snapshot(build, jobs)
    bad = 0
    linkage = set()
    units = sorted(set(before['pp']) | set(after['pp']))
    same_files = 0
    for u in units:
        a, b = before['pp'].get(u), after['pp'].get(u)
        if a is None or b is None:
            print('unit %s: only %s' % (u, 'after' if a is None else 'before'))
            bad += 1
            continue
        for f in sorted(set(a) | set(b)):
            if f == TYPES:
                continue
            if not same_lines(f, a.get(f, []), b.get(f, [])):
                x, y = a.get(f, []), b.get(f, [])
                i = next((k for k in range(min(len(x), len(y))) if x[k] != y[k]), min(len(x), len(y)))
                print('pp %s: %s: %d lines before, %d after; first difference at its line %d:\n'
                      '    before: %s\n    after:  %s' % (u, f, len(x), len(y), i + 1,
                                                          (x[i] if i < len(x) else '(end)')[:200],
                                                          (y[i] if i < len(y) else '(end)')[:200]))
                bad += 1
            else:
                same_files += 1
    nm_same = 0
    for o in sorted(set(before['nm']) | set(after['nm'])):
        a, b = before['nm'].get(o), after['nm'].get(o)
        if a is None or b is None:
            print('nm %s: only %s' % (o, 'after' if a is None else 'before'))
            bad += 1
            continue
        gone = {x for x in set(a) - set(b) if not LOCAL.search(x)}
        new = {x for x in set(b) - set(a) if not LOCAL.search(x)}
        for x in list(gone):
            name, kind = x.split(' ')
            twin = next((y for y in new if y.split(' ')[1] == kind and y.startswith('__Z')
                         and mangled(y.split(' ')[0]) == name[1:]), None)
            if twin:
                gone.discard(x)
                new.discard(twin)
                linkage.add('%s -> %s' % (name, twin.split(' ')[0]))
        if gone or new:
            print('nm %s: -%s +%s' % (o, sorted(gone)[:8], sorted(new)[:8]))
            bad += 1
        else:
            nm_same += 1
    if linkage:
        print('linkage only (C to C++, same name; not counted): %s' % ', '.join(sorted(linkage)))
    print('native_headers: %d units, %d (unit, file) token streams identical; %d objects with identical '
          'symbol lists; %d differences' % (len(units), same_files, nm_same, bad))
    return 1 if bad else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--build', default=os.path.join(ROOT, 'build', 'cmake', 'debug'))
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--snapshot')
    ap.add_argument('--check')
    ap.add_argument('-j', type=int, default=os.cpu_count() or 4)
    o = ap.parse_args()
    if o.apply:
        apply(o.build, o.dry_run)
        return 0
    if o.snapshot:
        s = snapshot(o.build, o.j)
        json.dump(s, open(o.snapshot, 'w'))
        print('native_headers: %d units, %d objects in %s' % (len(s['pp']), len(s['nm']), o.snapshot))
        return 2 if s['failed'] else 0
    if o.check:
        return check(o.check, o.build, o.j)
    ap.error('--apply, --snapshot or --check')


if __name__ == '__main__':
    sys.exit(main())
