#!/usr/bin/env python3
"""Rewrites the client's clock calls to the native clock (M11.1).

  native_clock.py [--tree DIR] [--build DIR] [--dry-run]

In every file of the tree that the build reads, the name of a call, and
nothing else on the line, is replaced:

  timeGetTime(...)   ->  bf_clock_ticks(...)
  GetTickCount(...)  ->  bf_clock_ticks(...)
  Sleep(...)         ->  bf_clock_sleep_ms(...)

A call is the name as a whole word followed by an opening parenthesis
(spaces allowed: `timeGetTime ( )` keeps its spaces). The values do not
change: the adapter's timeGetTime and GetTickCount returned bf_clock_ticks()
(port/native/sys/include/bf_clock.h: bf_clock_ms plus an hour, following an
installed source), its Sleep called bf_clock_sleep_ms, and the types are the
same (DWORD and uint32_t are both unsigned int), so the 32-bit wrap of the
client's arithmetic stays. bf_prefix.h includes bf_clock.h in every unit.

"Read by the build": every header, and every .c/.cpp that the build's
compile_commands.json compiles. Units the port never builds (GameGuard's
NPGameLib.cpp, which takes the functions' addresses) were left for M11.11,
which deleted them. Running the tool again on a rewritten tree changes nothing.
"""
import argparse, json, os, re, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
RULES = [
    (re.compile(r'\btimeGetTime(?=\s*\()'), 'bf_clock_ticks'),
    (re.compile(r'\bGetTickCount(?=\s*\()'), 'bf_clock_ticks'),
    (re.compile(r'\bSleep(?=\s*\()'), 'bf_clock_sleep_ms'),
]


def built_units(build):
    path = os.path.join(build, 'compile_commands.json')
    if not os.path.exists(path):
        sys.exit('no %s (configure the debug preset first: cmake --preset debug)' % path)
    units = set()
    for entry in json.load(open(path)):
        f = entry['file']
        if not os.path.isabs(f):
            f = os.path.join(entry['directory'], f)
        units.add(os.path.realpath(f))
    return units


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--tree', default=os.path.join(ROOT, 'src'))
    ap.add_argument('--build', default=os.path.join(ROOT, 'build', 'cmake', 'debug'))
    ap.add_argument('--dry-run', action='store_true', help='list the rewrites, write nothing')
    args = ap.parse_args()
    tree = os.path.realpath(args.tree)
    units = built_units(args.build)

    total, files, skipped = 0, 0, []
    for dirpath, dirnames, filenames in os.walk(tree):
        dirnames.sort()
        for name in sorted(filenames):
            ext = os.path.splitext(name)[1].lower()
            if ext not in ('.h', '.hpp', '.inl', '.c', '.cpp'):
                continue
            path = os.path.realpath(os.path.join(dirpath, name))
            raw = open(path, 'rb').read().decode('latin-1')
            count = sum(len(rx.findall(raw)) for rx, _ in RULES)
            if not count:
                continue
            rel = os.path.relpath(path, tree)
            if ext in ('.c', '.cpp') and path not in units:
                skipped.append('%s (%d)' % (rel, count))
                continue
            new = raw
            for rx, repl in RULES:
                if args.dry_run:
                    for m in rx.finditer(new):
                        ln = new.count('\n', 0, m.start()) + 1
                        print('%s:%d: %s -> %s' % (rel, ln, m.group(0), repl))
                new = rx.sub(repl, new)
            total += count
            files += 1
            if not args.dry_run:
                with open(path, 'wb') as f:
                    f.write(new.encode('latin-1'))
    print('native_clock: %d call(s) in %d file(s) %s' % (total, files, 'to rewrite' if args.dry_run else 'rewritten'))
    if skipped:
        print('native_clock: not built, left: ' + ', '.join(skipped))
    return 0


if __name__ == '__main__':
    sys.exit(main())
