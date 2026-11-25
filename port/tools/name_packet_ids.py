#!/usr/bin/env python3
"""Rewrites the numeric packet ids of src/ as HEADER_ enumerators (M10.2).

  name_packet_ids.py [--tree DIR] [--dry-run]

Every literal that port/tools/packetids.py reports (its rules say what counts
as a packet id) is replaced, in place and nothing else on the line, by the
enumerator src/packet.h gives that value: `case 120:` becomes
`case HEADER_ZC_NOTIFY_STANDENTRY:`, `p.PacketType = 0x9b;` becomes
`p.PacketType = HEADER_CZ_ENTER;`. A value without an enumerator stops the
tool before anything is written (name it in packet.h first). Running it
again on a rewritten tree changes nothing; packetids.py then exits 0.
"""
import argparse, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import packetids                                      # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--tree', default=os.path.join(packetids.ROOT, 'src'))
    ap.add_argument('--dry-run', action='store_true', help='list the rewrites, write nothing')
    args = ap.parse_args()
    tree = os.path.abspath(args.tree)
    names = packetids.header_names(tree)

    plan, missing = {}, []
    for path in packetids.source_files(tree):
        found = packetids.scan_file(path)
        for start, end, rule, lit, ln in found:
            value = int(lit.rstrip('uUlL'), 0)
            if value not in names:
                missing.append('%s:%d: 0x%x' % (os.path.relpath(path, tree), ln, value))
            plan.setdefault(path, []).append((start, end, names.get(value), lit, ln))
    if missing:
        print('no enumerator in packet.h for:\n  ' + '\n  '.join(missing), file=sys.stderr)
        return 1

    total = 0
    for path, edits in sorted(plan.items()):
        raw = open(path, 'rb').read().decode('latin-1')
        for start, end, name, lit, ln in sorted(edits, reverse=True):
            assert raw[start:end] == lit, (path, ln, lit)
            raw = raw[:start] + name + raw[end:]
            if args.dry_run:
                print('%s:%d: %s -> %s' % (os.path.relpath(path, tree), ln, lit, name))
        total += len(edits)
        if not args.dry_run:
            with open(path, 'wb') as f:
                f.write(raw.encode('latin-1'))
    print('name_packet_ids: %d literal(s) in %d file(s) %s' % (total, len(plan), 'to rewrite' if args.dry_run else 'rewritten'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
