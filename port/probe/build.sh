#!/bin/bash
# Builds the standalone probes as native arm64 programs:
#
#   build/port/bin/bifrost-packetlen   the client's packet length table
#   build/port/bin/bifrost-netprobe    a login through to the zone server
#
# They compile the game's own sources from src/ (the rewrites of
# port/tools/codemod.py are applied there). BF_SRC names another tree, for
# instance the shadow copy that port/tools/shadow.sh makes.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRC=${BF_SRC:-$ROOT/src}
if [ ! -d "$SRC" ]; then echo "no source tree at $SRC" >&2; exit 1; fi
. "$ROOT/port/tools/flags.sh"
OUT=$ROOT/build/port; mkdir -p "$OUT/bin" "$OUT/obj"

# The probes open no game files, so they keep the C library's own file
# names (port/client/include/bf_crt.h) and link no adapter.
CXX="clang++ -std=gnu++14 -O1 -g $BF_TARGET $BF_DEFINES -DBF_PORT_CODE $BF_INCLUDES -I$SRC"

"$ROOT/port/tools/gen_packet_names.py" "$SRC/packet.h" > "$OUT/obj/bf_packet_names.cpp"

compile() { $CXX -c "$1" -o "$OUT/obj/$(basename "${1%.*}").o"; }
compile "$SRC/packet.cpp"
compile "$SRC/device/PacketQueue.cpp"
compile "$SRC/device/Connection.cpp"
compile "$ROOT/port/client/src/bf_replay.cpp"
compile "$ROOT/port/instrument/bf_trace.cpp"
compile "$OUT/obj/bf_packet_names.cpp"
compile "$ROOT/port/probe/probe_support.cpp"
clang++ -std=c++17 -O1 -g -target arm64-apple-macos14 -I"$ROOT/port/native/sys/include" -c "$ROOT/port/native/sys/src/bf_clock.cpp" -o "$OUT/obj/bf_clock.o"
compile "$ROOT/port/probe/packetlen.cpp"
compile "$ROOT/port/probe/netprobe.cpp"

link() { clang++ -target arm64-apple-macos14 "$@"; }
link "$OUT/obj/packetlen.o" "$OUT/obj/packet.o" -o "$OUT/bin/bifrost-packetlen"
link "$OUT/obj/netprobe.o" "$OUT/obj/Connection.o" "$OUT/obj/bf_replay.o" "$OUT/obj/PacketQueue.o" "$OUT/obj/packet.o" \
     "$OUT/obj/bf_trace.o" "$OUT/obj/bf_packet_names.o" "$OUT/obj/probe_support.o" "$OUT/obj/bf_clock.o" -o "$OUT/bin/bifrost-netprobe"
file "$OUT/bin/bifrost-packetlen" "$OUT/bin/bifrost-netprobe"
