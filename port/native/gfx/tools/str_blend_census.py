#!/usr/bin/env python3
"""Counts the blend factors and multitexture presets the game's .str effects use.

  str_blend_census.py ARCHIVE.grf [ARCHIVE.grf ...]

Reads every .str file (version 0x94, src/resource/EZeffect.cpp) in the
archives, first archive first, and counts (srcalpha, destalpha, mtpreset) over
the position keyframes (dwType 0), the only ones whose values the client uses
(CRagEffect, RagEffect.cpp). DES-encrypted entries are counted and skipped.
Reads the archives only. The numbers are D3DBLEND values and preset indices.
"""
import collections, os, struct, sys, zlib

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'tools'))
import grfaudit  # noqa: E402

KEY = 124  # sizeof(KAC_KEYFRAME): iFrame, dwType, KAC_XFORMDATA (116 bytes)


def main():
    counts, seen = collections.Counter(), set()
    files = bad = encrypted = 0
    for path in sys.argv[1:]:
        _, _, entries = grfaudit.read_table(path)
        for name, e in entries.items():
            if not name.endswith(b'.str') or name in seen:
                continue
            seen.add(name)
            if e.flags & 6:
                encrypted += 1
                continue
            e.archive.seek(46 + e.offset)
            try:
                data = zlib.decompress(e.archive.read(e.aligned)[:e.packed])
                if struct.unpack_from('<I', data, 4)[0] != 0x94:
                    raise ValueError('version')
                layers = struct.unpack_from('<i', data, 16)[0]
                at = 36
                for _ in range(layers):
                    textures = struct.unpack_from('<i', data, at)[0]
                    at += 4 + 128 * textures
                    keys = struct.unpack_from('<i', data, at)[0]
                    at += 4
                    for _ in range(keys):
                        kind = struct.unpack_from('<I', data, at + 4)[0]
                        if kind == 0:
                            counts[struct.unpack_from('<III', data, at + 112)] += 1
                        at += KEY
                files += 1
            except (zlib.error, struct.error, ValueError):
                bad += 1
    print(f'{files} files read, {bad} unreadable, {encrypted} encrypted')
    print('src dst preset  keys')
    for (src, dst, preset), n in sorted(counts.items()):
        print(f'{src:3} {dst:3} {preset:6}  {n}')


if __name__ == '__main__':
    main()
