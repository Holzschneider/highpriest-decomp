#!/usr/bin/env python3
"""Compares the client's packet length table with rAthena's packet database
for one or more packet versions, to find the version a server must be built
with and the packets that still differ.

  packetdiff.py --rathena DIR [--client FILE | --probe PATH] PACKETVER [PACKETVER ...]

  --rathena  a rAthena checkout (src/map/clif_packetdb.hpp, src/config/packets.hpp)
  --client   output of bifrost-packetlen; default: runs build/port/bin/bifrost-packetlen
  -v         list every differing packet, not only the totals

Only zone packets (header 0x72 and above) are compared: rAthena's table
carries no meaningful lengths for the account and character server packets.
rAthena lists few of the packets it sends, so "client only" is expected to be
large; what matters is the count of length mismatches.

Two sources on the rAthena side:
  packet db  clif_packetdb.hpp, the table the map server parses requests with.
             Entries written as HEADER_X / sizeof(PACKET_X) are resolved by
             compiling a small program against rAthena's own packets.hpp.
             An entry without a handler is a length rAthena never uses at
             run time (it neither parses nor sends by this table).
  structs    every PACKET_X structure in map/packets.hpp and packets_struct.hpp
             that has a HEADER_X: the layout rAthena sends or parses. A
             structure with a length field (packetLength, packetSize, ...) or
             a trailing [] member is variable length. A structure is listed
             whether or not rAthena uses it at this PACKETVER, so a mismatch
             there is a lead to check in clif.cpp, not a verdict.
The struct pass needs rAthena's 3rdparty/ headers in the checkout; mysql.h
is stubbed when it is missing.
"""
import argparse, os, re, subprocess, sys, tempfile

def client_table(args):
    if args.client:
        text = open(args.client).read()
    else:
        text = subprocess.run([args.probe], check=True, capture_output=True, text=True).stdout
    return {int(a, 16): int(b) for a, b in (line.split() for line in text.splitlines() if line.strip())}

LENGTH_FIELDS = ('packetLength', 'PacketLength', 'packetSize', 'PacketSize', 'packetLen', 'len', 'length')

def include_flags(root, shim):
    third = os.path.join(root, '3rdparty')
    flags = [f'-I{root}/src', f'-I{root}/src/custom']
    for sub in ('rapidyaml/src', 'rapidyaml/ext/c4core/src', 'yaml-cpp/include', 'libconfig', 'httplib', 'json/include'):
        flags.append(f'-I{third}/{sub}')
    return flags + ['-idirafter', shim]

def rathena_structs(root, packetver):
    """{name: (header, size, variable)} for every PACKET_name with a HEADER_name."""
    with tempfile.TemporaryDirectory() as tmp:
        with open(os.path.join(tmp, 'mysql.h'), 'w') as f:   # only the type names sql.hpp mentions
            f.write('typedef struct st_mysql MYSQL; typedef struct st_mysql_res MYSQL_RES; typedef char **MYSQL_ROW;\n'
                    'typedef struct st_mysql_stmt MYSQL_STMT; typedef struct st_mysql_bind MYSQL_BIND;\n')
        flags = include_flags(root, tmp)
        head = f'#define PACKETVER {packetver}\n#define PRERE\n#include "config/packets.hpp"\n#include "map/packets.hpp"\n'
        pre = subprocess.run(['clang++', '-std=c++17', '-E', '-P', '-x', 'c++', *flags, '-'],
                             input=head, capture_output=True, text=True)
        if pre.returncode:
            print(f'struct pass skipped: {pre.stderr.strip().splitlines()[-1]}', file=sys.stderr)
            return {}
        text = pre.stdout
        headers = set(re.findall(r'\bconst int16 HEADER_(\w+)\s*=', text))
        bodies = {}
        for m in re.finditer(r'\bstruct PACKET_(\w+)\s*\{', text):
            depth, i = 1, m.end()
            while depth and i < len(text):
                depth += {'{': 1, '}': -1}.get(text[i], 0); i += 1
            bodies[m.group(1)] = text[m.end():i - 1]
        names = sorted(n for n in headers if n in bodies)
        prog = head + '#include <cstdio>\nint main() {\n' + ''.join(
            f'  printf("{n} %d %zu\\n", (int)(unsigned short)HEADER_{n}, sizeof(PACKET_{n}));\n' for n in names) + '}\n'
        exe = os.path.join(tmp, 'sizes')
        build = subprocess.run(['clang++', '-std=c++17', '-x', 'c++', *flags, '-', '-o', exe],
                               input=prog, capture_output=True, text=True)
        if build.returncode:
            print(f'struct pass skipped: {build.stderr.strip().splitlines()[-1]}', file=sys.stderr)
            return {}
        out = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
    result = {}
    for line in out.splitlines():
        name, header, size = line.split()
        body = bodies[name]
        variable = bool(re.search(r'\[\s*\]\s*;', body)) or bool(re.search(
            r'^\s*u?int16\s+(%s)\s*;' % '|'.join(LENGTH_FIELDS), body.split(';', 2)[1] + ';', re.M))
        result[name] = (int(header), int(size), variable)
    return result

def rathena_table(root, packetver, structs):
    source = f'#define PACKETVER {packetver}\n#include "config/packets.hpp"\n#include "map/clif_packetdb.hpp"\n'
    out = subprocess.run(['clang', '-E', '-P', '-x', 'c++', f'-I{root}/src', f'-I{root}/src/custom', '-'],
                         input=source, capture_output=True, text=True).stdout
    table = {}
    for header, length, handler in re.findall(
            r'packetdb_addpacket\(\s*(0x[0-9a-fA-F]+|HEADER_\w+)\s*,\s*(sizeof\s*\([^)]*\)|[^,]+?)\s*,\s*([A-Za-z_0-9]+)', out):
        if header.startswith('HEADER_'):
            if header[7:] not in structs:
                print(f'unresolved packet db entry {header}', file=sys.stderr)
                continue
            header = structs[header[7:]][0]
        else:
            header = int(header, 16)
        size = re.match(r'sizeof\s*\(\s*(?:struct\s+)?PACKET_(\w+)\s*\)', length)
        if size:
            length = str(structs[size.group(1)][1]) if size.group(1) in structs else length
        table[header] = (length.strip(), handler)   # later entries override earlier ones
    if not table:
        sys.exit(f'no packets found for PACKETVER {packetver}; is {root} a rAthena checkout?')
    return table

def header_uses(root, header):
    """Source files of rAthena's servers (packet db aside) that name the header
    as a literal, the way clif.cpp writes WFIFOW(fd,0) = 0x...: a lead to
    whether rAthena sends or parses it at all."""
    pat = re.compile(r'\b0x0*%x\b' % header, re.I)
    found = []
    for sub in ('map', 'char', 'login', 'common'):
        d = os.path.join(root, 'src', sub)
        for name in sorted(os.listdir(d)) if os.path.isdir(d) else ():
            if name.endswith(('.cpp', '.hpp')) and name not in ('clif_packetdb.hpp', 'des.cpp'):
                with open(os.path.join(d, name), errors='replace') as f:
                    if pat.search(f.read()):
                        found.append(f'{sub}/{name}')
    return found

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--rathena', required=True)
    ap.add_argument('--client')
    ap.add_argument('--probe', default=os.path.join(here, '..', '..', 'build', 'port', 'bin', 'bifrost-packetlen'))
    ap.add_argument('-v', '--verbose', action='store_true')
    ap.add_argument('packetver', nargs='+')
    args = ap.parse_args()

    client = {h: n for h, n in client_table(args).items() if h >= 0x72}
    print(f'client: {len(client)} zone packets, highest header 0x{max(client):04x}')
    for version in args.packetver:
        structs = rathena_structs(args.rathena, version)
        server = {h: v for h, v in rathena_table(args.rathena, version, structs).items() if h >= 0x72}
        common = [h for h in client if h in server]
        mismatch = [h for h in common if not server[h][0].lstrip('-').isdigit() or int(server[h][0]) != client[h]]
        requests = [h for h in mismatch if server[h][1] != 'nullptr']
        unknown = [h for h in server if h not in client and server[h][1] != 'nullptr']
        print(f'PACKETVER {version}: {len(common)} in common, {len(mismatch)} length mismatches '
              f'({len(requests)} of them client requests), {len(unknown)} server-parsed requests the client lacks')
        if args.verbose:
            for h in sorted(mismatch):
                note = ''
                if server[h][1] == 'nullptr':
                    uses = header_uses(args.rathena, h)
                    note = f'  named in {", ".join(uses)}' if uses else '  length only: no rAthena source names it'
                print(f'    0x{h:04x} client {client[h]:>4}  rathena {server[h][0]:>4}  {server[h][1]}{note}')
        differ = []
        for name, (h, size, variable) in sorted(structs.items(), key=lambda kv: kv[1][0]):
            if h < 0x72 or h not in client:
                continue
            if (client[h] == -1) != variable or (not variable and client[h] != size):
                differ.append((h, name, size, variable))
        print(f'PACKETVER {version}: {len(structs)} rAthena structures, {len(differ)} differ from the client')
        if args.verbose:
            for h, name, size, variable in differ:
                theirs = f'>={size} var' if variable else str(size)
                print(f'    0x{h:04x} client {client[h]:>4}  rathena {theirs:>8}  PACKET_{name}')

if __name__ == '__main__':
    main()
