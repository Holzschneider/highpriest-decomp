#!/usr/bin/env python3
"""Checks that no unit of the build reads a Windows or DirectX header (M11).

  nowin.py [--build DIR] [--units GLOB ...] [--summary] [-j N]
  nowin.py --ninja [--build DIR] [--units GLOB ...] [--summary]
  nowin.py --text
  nowin.py --write-names

  --build    the CMake build directory whose compile_commands.json lists the
             units (default: build/cmake/debug). The presets export it
             (CMAKE_EXPORT_COMPILE_COMMANDS=ON in CMakePresets.json); a build
             configured by hand needs -DCMAKE_EXPORT_COMPILE_COMMANDS=ON.
  --units    check only the units whose repository-relative path matches one
             of these shell patterns (fnmatch: `*` also crosses `/`), e.g.
             --units 'src/*' --units 'port/native/*'
  --summary  print counts per header and per unit instead of the pairs
  -j         parallel compiles (default: the number of cores)
  --ninja    do not preprocess: read the headers each object of the last
             build read from Ninja's dependency log (`ninja -C DIR -t
             deps`; the build writes it with -MD, system headers included).
             Under a second; checks only what the last build compiled, so
             build first (the fast check does). The unit of an object is its
             first source dependency.
  --text     do not build or preprocess: fail on any #include line in a
             tracked C, C++ or Objective-C file of src/ or port/ whose header
             name is a Windows or DirectX header's (WINDOWS_NAMES below),
             unless it names one of OWN_HEADERS; lines in #if blocks count
             too. Lists `file:line<TAB>header`.
  --write-names  rewrite WINDOWS_NAMES from a Wine tree (BF_WIN_HEADERS;
             port/README.md says how to get it back from history) and the
             CRT stand-ins (STANDIN_NAMES)

For every unit, the unit's own compile command is rerun with `-M` (clang's
dependency output: every header the preprocessor reads, system headers
included; `-MM` would hide the Wine tree, which the build passes with
-isystem). A header is reported when it is

  wine    under the Wine tree (BF_WIN_HEADERS; port/compat/winheaders until
          M11.11 removed it);
  compat  anywhere under port/compat (the adapters and the CRT stand-ins
          until M11.11 removed them; the prefix header and the rest of the
          port's own code, port/client, are not compat since M11.0d);
  name    elsewhere, but named like a Windows or DirectX header: its path
          ends in the path of a header of the Wine tree (windows.h, ddraw.h,
          ddk/ntddk.h, ...; the Wine tree's msvcrt/ directory, which holds
          the C library's names, does not count) or of a CRT stand-in that
          was in port/compat/include (io.h, direct.h, conio.h, process.h,
          mbstring.h, minmax.h, winsock.h): the list WINDOWS_NAMES
          (port/tools/winheaders.txt, written from the Wine tree by
          --write-names before it left the repository), compared without regard to
          case, as the file system does (src/WinCrypt.h, a copy of the
          Windows SDK's wincrypt.h, counts). Headers of the SDK and the
          toolchain never count by name, nor do the client's own headers in
          OWN_HEADERS below whose names a Wine header happens to share
          (src/device/Control.h, not DirectShow's control.h).
  sdk     one of the headers of the Windows libraries the 2008 client
          linked, copied into src/ until M11.11 (SDK_HEADERS below: Miles' Mss.h,
          Granny's granny.h, Bink's BINK.H, RAD.H and radbase.h, Intel's
          ijl.h, nProtect's NPGameLib.h, Microsoft's DbgHelp.h). They declare
          the Windows-shaped calls M11.6 and M11.10 replace and include
          windows.h themselves; this list is by hand (found by their
          copyright lines and export macros).

Paths are reported in the case the directory holds them (the dependency
output spells a header the way the #include line does: wincrypt.h and
WinCrypt.h are one file here). A unit whose source does not exist yet (a
generated file of a build that has not run) is skipped and named on stderr.

The default output is one line per unit and header, `unit<TAB>kind<TAB>header`
(paths relative to the repository). Exit status: 0 when no header is
reported, 1 when one is, 2 when a unit could not be preprocessed (its error
is printed; such a unit is not counted as clean).
"""
import argparse
import collections
import concurrent.futures
import fnmatch
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
WINE = os.path.realpath(os.environ['BF_WIN_HEADERS']) + os.sep if os.environ.get('BF_WIN_HEADERS') \
    else os.path.join(ROOT, 'port', 'compat', 'winheaders') + os.sep   # gone since M11.11
WINDOWS_NAMES = os.path.join(ROOT, 'port', 'tools', 'winheaders.txt')
COMPAT = os.path.join(ROOT, 'port', 'compat') + os.sep
# The CRT stand-ins port/compat/include held (M11.11 removed them).
STANDIN_NAMES = {'conio.h', 'direct.h', 'io.h', 'mbstring.h', 'minmax.h', 'process.h'}
# The client's own headers whose names collide with a Wine header (found by
# comparing the file names of src/ and port/ with the Wine tree's).
OWN_HEADERS = {'src/device/Control.h'}
# None left: M11.6 and M11.10 deleted Mss.h, granny.h, BINK.H, RAD.H and
# radbase.h; M11.11 ijl.h, NPGameLib.h and DbgHelp.h (and src/WinCrypt.h).
SDK_HEADERS = set()
_case = {}


def canonical(p):
    """p with every component in the case its directory lists it."""
    if p in _case:
        return _case[p]
    parent, name = os.path.split(p)
    if not name:
        return p
    cparent = canonical(parent) if parent != p else parent
    try:
        entries = os.listdir(cparent)
    except OSError:
        entries = []
    if name not in entries:
        low = name.lower()
        name = next((e for e in entries if e.lower() == low), name)
    _case[p] = os.path.join(cparent, name)
    return _case[p]


def windows_names():
    """Relative paths (lower case) of the Windows and DirectX headers."""
    return {l.strip() for l in open(WINDOWS_NAMES) if l.strip() and not l.startswith('#')}


def write_names():
    """WINDOWS_NAMES from the Wine tree and the CRT stand-ins."""
    if not os.path.isfile(os.path.join(WINE, 'windows.h')):
        sys.exit('nowin: no windows.h in %s; set BF_WIN_HEADERS' % WINE)
    names = set()
    for top, dirs, files in os.walk(WINE):
        sub = os.path.relpath(top, WINE)
        if sub == 'msvcrt' or sub.startswith('msvcrt' + os.sep):
            continue
        for f in files:
            if f.endswith('.h') or f.endswith('.idl') or f.endswith('.inl'):
                names.add(os.path.normpath(os.path.join(sub, f)).lower())
    names |= STANDIN_NAMES
    with open(WINDOWS_NAMES, 'w') as f:
        f.write('# The Windows and DirectX header names nowin.py reports (paths relative to\n'
                '# the Wine include tree, lower case, msvcrt/ left out; and the CRT\n'
                '# stand-ins of port/compat/include). Written by nowin.py --write-names.\n')
        f.write(''.join(n + '\n' for n in sorted(names)))
    print('nowin: %d names in %s' % (len(names), rel(WINDOWS_NAMES)))


def system_dirs():
    dirs = ['/Applications/Xcode', '/Library/Developer/', '/usr/include/', '/usr/lib/']
    try:
        sdk = subprocess.run(['xcrun', '--show-sdk-path'], capture_output=True,
                             text=True).stdout.strip()
        if sdk:
            dirs.append(os.path.realpath(sdk) + os.sep)
    except OSError:
        pass
    return tuple(dirs)


def unit_args(entry):
    if 'arguments' in entry:
        args = list(entry['arguments'])
    else:
        args = shlex.split(entry['command'])
    out, skip = [], False
    for a in args:
        if skip:
            skip = False
            continue
        if a in ('-o', '-MF', '-MT', '-MQ'):
            skip = True
            continue
        if a in ('-c', '-MD', '-MMD') or a.startswith('-o'):
            continue
        out.append(a)
    return out + ['-M', '-MG']


def parse_deps(text):
    text = text.replace('\\\n', ' ')
    _, _, deps = text.partition(': ')
    paths, cur = [], ''
    i = 0
    while i < len(deps):
        c = deps[i]
        if c == '\\' and i + 1 < len(deps) and deps[i + 1] == ' ':
            cur += ' '
            i += 2
            continue
        if c.isspace():
            if cur:
                paths.append(cur)
            cur = ''
        else:
            cur += c
        i += 1
    if cur:
        paths.append(cur)
    return paths[1:]  # the first is the unit itself


def classify(path, names, sysdirs):
    p = canonical(os.path.realpath(path if os.path.isabs(path) else os.path.join(ROOT, path)))
    if p.startswith(WINE):
        return 'wine'
    if p.startswith(COMPAT):
        return 'compat'
    if rel(p) in SDK_HEADERS:
        return 'sdk'
    if p.startswith(sysdirs) or rel(p) in OWN_HEADERS:
        return None
    low = p.lower()
    for n in names:
        if low.endswith(os.sep + n):
            return 'name'
    return None


SOURCE_EXT = ('.c', '.cpp', '.cc', '.cxx', '.m', '.mm')
INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^">]+)[">]')


def ninja_units(build, names, sysdirs):
    """[(unit, hits)] from the dependency log of the build's last run."""
    p = subprocess.run(['ninja', '-C', build, '-t', 'deps'], capture_output=True, text=True)
    if p.returncode != 0:
        sys.exit('nowin: ninja -C %s -t deps failed\n%s' % (build, p.stderr))
    objects, cur = [], None
    for line in p.stdout.split('\n'):
        if not line.strip():
            continue
        if not line.startswith(' '):
            cur = [line.split(':')[0], 'STALE' in line, []]
            objects.append(cur)
        elif cur is not None:
            cur[2].append(line.strip())
    results, stale, kinds = {}, 0, {}
    for obj, is_stale, deps in objects:
        stale += is_stale
        src = next((d for d in deps if d.endswith(SOURCE_EXT)), None)
        if src is None:
            continue
        unit = rel(os.path.join(build, src))
        hits = set(results.get(unit, ()))
        for d in deps:
            if d not in kinds:
                k = classify(os.path.join(build, d), names, sysdirs)
                kinds[d] = (k, rel(os.path.join(build, d))) if k else None
            if kinds[d]:
                hits.add(kinds[d])
        results[unit] = sorted(hits)
    if stale:
        sys.stderr.write('nowin: %d objects have stale dependencies (build first)\n' % stale)
    return sorted(results.items())


def text_check(names):
    """Every #include line of a tracked source file that names a Windows header."""
    files = subprocess.run(['git', 'ls-files', '--', 'src', 'port'], cwd=ROOT, capture_output=True,
                           text=True, check=True).stdout.split('\n')
    hits = []
    for f in files:
        if not f.endswith(SOURCE_EXT + ('.h', '.hpp', '.hh', '.inl')):
            continue
        try:
            lines = open(os.path.join(ROOT, f), errors='replace').read().split('\n')
        except OSError:
            continue
        for n, line in enumerate(lines, 1):
            m = INCLUDE.match(line)
            if not m:
                continue
            name = os.path.normpath(m.group(2).replace('\\', '/')).lower()
            if not any(name == w or name.endswith('/' + w) for w in names):
                continue
            here = os.path.join(ROOT, os.path.dirname(f), m.group(2))
            if m.group(1) == '"' and os.path.exists(here) and rel(here) in OWN_HEADERS:
                continue
            hits.append('%s:%d\t%s' % (f, n, m.group(2)))
    for h in hits:
        print(h)
    sys.stderr.write('nowin: %d #include lines name a Windows or DirectX header\n' % len(hits))
    return 1 if hits else 0


def rel(p):
    p = canonical(os.path.realpath(p))
    return os.path.relpath(p, ROOT) if p.startswith(ROOT + os.sep) else p


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--build', default=os.path.join(ROOT, 'build', 'cmake', 'debug'))
    ap.add_argument('--units', action='append', default=[])
    ap.add_argument('--summary', action='store_true')
    ap.add_argument('-j', type=int, default=os.cpu_count() or 4)
    ap.add_argument('--ninja', action='store_true')
    ap.add_argument('--text', action='store_true')
    ap.add_argument('--write-names', action='store_true')
    o = ap.parse_args()
    if o.write_names:
        write_names()
        return 0
    names, sysdirs = windows_names(), system_dirs()
    if o.text:
        return text_check(names)
    if o.ninja:
        results = [(r, h, '') for r, h in ninja_units(o.build, names, sysdirs)
                   if not o.units or any(fnmatch.fnmatch(r, g) for g in o.units)]
        return report(results, [], o.units, o.summary)

    db = os.path.join(o.build, 'compile_commands.json')
    if not os.path.exists(db):
        sys.exit('no %s: configure with a preset (cmake --preset debug) or with '
                 '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON' % db)
    entries = json.load(open(db))
    units, skipped = [], []
    for e in entries:
        f = os.path.realpath(os.path.join(e['directory'], e['file']))
        r = rel(f)
        if not os.path.exists(f):
            skipped.append(r)
            continue
        if o.units and not any(fnmatch.fnmatch(r, g) for g in o.units):
            continue
        units.append((r, e))
    units.sort(key=lambda u: u[0])

    def run(item):
        r, e = item
        p = subprocess.run(unit_args(e), cwd=e['directory'], capture_output=True, text=True)
        if p.returncode != 0:
            return r, None, p.stderr
        hits = []
        for d in parse_deps(p.stdout):
            k = classify(os.path.join(e['directory'], d), names, sysdirs)
            if k:
                hits.append((k, rel(os.path.join(e['directory'], d))))
        return r, sorted(set(hits)), ''

    with concurrent.futures.ThreadPoolExecutor(o.j) as pool:
        results = list(pool.map(run, units))
    return report(results, skipped, o.units, o.summary)


def report(results, skipped, patterns, summary):
    for r in sorted(set(skipped)):
        if not patterns or any(fnmatch.fnmatch(r, g) for g in patterns):
            sys.stderr.write('%s: skipped, the source does not exist yet (build first)\n' % r)
    failed = [(r, err) for r, h, err in results if h is None]
    for r, err in failed:
        sys.stderr.write('%s: preprocessing failed\n%s' % (r, err))
    reported = [(r, h) for r, h, _ in results if h]
    if summary:
        per_header = collections.Counter()
        kinds = {}
        for r, h in reported:
            for k, path in h:
                per_header[path] += 1
                kinds[path] = k
        print('units checked: %d   reading a Windows header: %d   clean: %d   failed: %d'
              % (len(results), len(reported), len(results) - len(reported) - len(failed),
                 len(failed)))
        by_kind = collections.Counter(kinds.values())
        print('distinct headers: %d (%s)' % (len(per_header), ', '.join(
            '%s %d' % (k, by_kind[k]) for k in ('wine', 'compat', 'name', 'sdk') if by_kind[k])))
        tree = collections.Counter()
        for r, h in reported:
            tree[r.split('/')[0] if not r.startswith('port/') else '/'.join(r.split('/')[:2])] += 1
        print('units by tree: ' + ', '.join('%s %d' % kv for kv in sorted(tree.items())))
        print('\nheaders (units reading it, kind):')
        for path, n in sorted(per_header.items(), key=lambda kv: (-kv[1], kv[0])):
            print('%5d  %-6s %s' % (n, kinds[path], path))
        print('\nunits (headers read: wine/compat/name/sdk):')
        for r, h in sorted(reported, key=lambda x: (-len(x[1]), x[0])):
            c = collections.Counter(k for k, _ in h)
            print('%5d  %3d/%3d/%d/%d  %s' % (len(h), c['wine'], c['compat'], c['name'],
                                               c['sdk'], r))
    else:
        for r, h in reported:
            for k, path in h:
                print('%s\t%s\t%s' % (r, k, path))
        sys.stderr.write('nowin: %d of %d units read a Windows or DirectX header\n'
                         % (len(reported), len(results)))
    if failed:
        return 2
    return 1 if reported else 0


if __name__ == '__main__':
    sys.exit(main())
