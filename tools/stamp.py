#!/usr/bin/env python3
"""Writes the link time and the program database record of a linked PE image.

LINK stores the time of the link in the COFF header and in the debug
directory, and ends the file with an NB10 record that names the program
database: its signature, its age and its path. This script copies IMAGE to
OUTPUT with those values replaced by the given ones.
"""
import argparse
import struct


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image")
    parser.add_argument("output")
    parser.add_argument("--time", type=lambda v: int(v, 0), required=True)
    parser.add_argument("--signature", type=lambda v: int(v, 0), required=True)
    parser.add_argument("--age", type=lambda v: int(v, 0), required=True)
    parser.add_argument("--pdb", required=True)
    args = parser.parse_args()

    with open(args.image, "rb") as file:
        data = bytearray(file.read())

    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise SystemExit(f"{args.image}: not a PE image")
    section_count = struct.unpack_from("<H", data, pe + 6)[0]
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise SystemExit(f"{args.image}: not a PE32 image")
    sections = optional + struct.unpack_from("<H", data, pe + 20)[0]
    debug_rva, debug_size = struct.unpack_from("<II", data, optional + 96 + 6 * 8)
    if debug_size != 28:
        raise SystemExit(f"{args.image}: expected one debug directory entry")

    debug = None
    for index in range(section_count):
        _, address, raw_size, raw = struct.unpack_from("<IIII", data, sections + index * 40 + 8)
        if address <= debug_rva < address + raw_size:
            debug = raw + debug_rva - address
    if debug is None:
        raise SystemExit(f"{args.image}: the debug directory is in no section")

    kind, size, _, record = struct.unpack_from("<IIII", data, debug + 12)
    if kind != 2 or data[record:record + 4] != b"NB10" or record + size != len(data):
        raise SystemExit(f"{args.image}: expected an NB10 record at the end of the file")

    struct.pack_into("<I", data, pe + 8, args.time)
    struct.pack_into("<I", data, debug + 4, args.time)
    nb10 = b"NB10" + struct.pack("<III", 0, args.signature, args.age) + args.pdb.encode("ascii") + b"\0"
    struct.pack_into("<I", data, debug + 16, len(nb10))
    data[record:] = nb10

    with open(args.output, "wb") as file:
        file.write(data)


if __name__ == "__main__":
    main()
