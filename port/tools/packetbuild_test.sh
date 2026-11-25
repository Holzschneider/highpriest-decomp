#!/bin/bash
# Builds and runs bifrost-packetbuild-test: each request builder of
# src/PaddingPacket.h (M10.3) against the construction the 2008 client used
# before, byte for byte, with a deterministic padding. The old side writes
# the packet ids as numbers on purpose: it is the reference the builders are
# held to. Exit 0 when all 15 are identical.
#
#   port/tools/packetbuild_test.sh      (BF_SRC names another tree)
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRC=${BF_SRC:-$ROOT/src}
. "$ROOT/port/tools/flags.sh"
OUT=$ROOT/build/port/bin; mkdir -p "$OUT"
clang++ -std=gnu++14 -O1 $BF_TARGET $BF_DEFINES -DBF_PORT_CODE $BF_INCLUDES -I"$SRC" \
	"$ROOT/port/tools/packetbuild_test.cpp" "$SRC/packet.cpp" -o "$OUT/bifrost-packetbuild-test"
"$OUT/bifrost-packetbuild-test"
