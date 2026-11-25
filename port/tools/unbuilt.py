#!/usr/bin/env python3
"""Lists the tracked files of src/ that no unit of the build reads (M11.11).

  unbuilt.py [--build DIR] [--summary]

A file counts as read when it is a compiled unit or a header some unit read
in the last build (Ninja's dependency log of DIR, default build/cmake/debug:
build every target first, `cmake --build --preset debug`), or when the build
reads it otherwise (KEEP: the packet table generator's input). Paths are
compared without regard to case, as the file system does. One path per line,
sorted; --summary prints counts per folder instead. M11.11 deleted what this
listed (`git rm $(port/tools/unbuilt.py)`): the Windows-only parts of src/
(GameGuard, the Vietnamese input, the crash dialog, the Visual C++ runtime
and library sources, the import libraries, the resources), which main keeps.
"""
import argparse
import collections
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nowin  # noqa: E402

ROOT = nowin.ROOT
KEEP = {'src/packet.h'}   # gen_packet_names.py reads it (it is also compiled)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--build', default=os.path.join(ROOT, 'build', 'cmake', 'debug'))
    ap.add_argument('--summary', action='store_true')
    o = ap.parse_args()
    p = subprocess.run(['ninja', '-C', o.build, '-t', 'deps'], capture_output=True, text=True)
    if p.returncode != 0:
        sys.exit('unbuilt: ninja -C %s -t deps failed (build first)\n%s' % (o.build, p.stderr))
    read = set()
    for line in p.stdout.split('\n'):
        if line.startswith('    '):
            read.add(nowin.rel(os.path.join(o.build, line.strip())).lower())
    if not any(r.startswith('src/') for r in read):
        sys.exit('unbuilt: the dependency log names no file of src/ (build every target first)')
    tracked = subprocess.run(['git', 'ls-files', '--', 'src'], cwd=ROOT, capture_output=True,
                             text=True, check=True).stdout.split('\n')
    out = sorted(t for t in tracked if t and t not in KEEP
                 and nowin.rel(os.path.join(ROOT, t)).lower() not in read)
    if o.summary:
        folders = collections.Counter(os.path.dirname(t) for t in out)
        size = sum(os.path.getsize(os.path.join(ROOT, t)) for t in out)
        for f, n in sorted(folders.items()):
            print('%5d  %s' % (n, f))
        print('unbuilt: %d of %d tracked files of src/, %.1f MB' % (len(out), len([t for t in tracked if t]),
                                                                 size / 1e6))
    else:
        print('\n'.join(out))


if __name__ == '__main__':
    main()
