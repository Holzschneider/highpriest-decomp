#!/usr/bin/env python3
"""Compares the client's packet length table with rAthena's packet database
for one or more packet versions, to find the version a server must be built
with and the packets that still differ.

  packetdiff.py --rathena DIR [--client FILE | --probe PATH] PACKETVER [PACKETVER ...]

  --rathena  a rAthena checkout (src/map/clif_packetdb.hpp, src/config/packets.hpp)
  --client   output of hp-packetlen; default: runs build/port/bin/hp-packetlen
  -v         list every differing packet, not only the totals

Only zone packets (header 0x72 and above) are compared: rAthena's table
carries no meaningful lengths for the account and character server packets.
rAthena lists few of the packets it sends, so "client only" is expected to be
large; what matters is the count of length mismatches.
"""
import argparse, os, re, subprocess, sys

def client_table(args):
    if args.client:
        text = open(args.client).read()
    else:
        text = subprocess.run([args.probe], check=True, capture_output=True, text=True).stdout
    return {int(a, 16): int(b) for a, b in (line.split() for line in text.splitlines() if line.strip())}

def rathena_table(root, packetver):
    source = f'#define PACKETVER {packetver}\n#include "config/packets.hpp"\n#include "map/clif_packetdb.hpp"\n'
    out = subprocess.run(['clang', '-E', '-P', '-x', 'c++', f'-I{root}/src', f'-I{root}/src/custom', '-'],
                         input=source, capture_output=True, text=True).stdout
    table = {}
    for header, length, handler in re.findall(r'packetdb_addpacket\((0x[0-9a-fA-F]+),([^,]+),([A-Za-z_0-9]+)', out):
        table[int(header, 16)] = (length.strip(), handler)   # later entries override earlier ones
    if not table:
        sys.exit(f'no packets found for PACKETVER {packetver}; is {root} a rAthena checkout?')
    return table

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--rathena', required=True)
    ap.add_argument('--client')
    ap.add_argument('--probe', default=os.path.join(here, '..', '..', 'build', 'port', 'bin', 'hp-packetlen'))
    ap.add_argument('-v', '--verbose', action='store_true')
    ap.add_argument('packetver', nargs='+')
    args = ap.parse_args()

    client = {h: n for h, n in client_table(args).items() if h >= 0x72}
    print(f'client: {len(client)} zone packets, highest header 0x{max(client):04x}')
    for version in args.packetver:
        server = {h: v for h, v in rathena_table(args.rathena, version).items() if h >= 0x72}
        common = [h for h in client if h in server]
        mismatch = [h for h in common if not server[h][0].lstrip('-').isdigit() or int(server[h][0]) != client[h]]
        requests = [h for h in mismatch if server[h][1] != 'nullptr']
        unknown = [h for h in server if h not in client and server[h][1] != 'nullptr']
        print(f'PACKETVER {version}: {len(common)} in common, {len(mismatch)} length mismatches '
              f'({len(requests)} of them client requests), {len(unknown)} server-parsed requests the client lacks')
        if args.verbose:
            for h in sorted(mismatch):
                print(f'    0x{h:04x} client {client[h]:>4}  rathena {server[h][0]:>4}  {server[h][1]}')

if __name__ == '__main__':
    main()
