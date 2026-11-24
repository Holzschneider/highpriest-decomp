#!/bin/bash
# Builds the standalone probes as native arm64 programs:
#
#   build/port/bin/bifrost-packetlen   the client's packet length table
#   build/port/bin/bifrost-netprobe    a login through to the zone server
#
# They compile the game's own sources. Until the rewrites of
# port/tools/codemod.py are applied to src/, the build takes them from the
# rewritten copy that port/tools/shadow.sh makes (and makes it if absent).
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRC=${BF_SRC:-$ROOT/build/port/shadow/src}
[ -d "$SRC" ] || "$ROOT/port/tools/shadow.sh" > /dev/null
. "$ROOT/port/tools/flags.sh"
OUT=$ROOT/build/port; mkdir -p "$OUT/bin" "$OUT/obj"

CXX="clang++ -std=gnu++14 -O1 -g $BF_TARGET $BF_DEFINES $BF_INCLUDES -I$SRC -isystem $WIN_HEADERS"

"$ROOT/port/tools/gen_packet_names.py" "$SRC/packet.h" > "$OUT/obj/bf_packet_names.cpp"

compile() { $CXX -c "$1" -o "$OUT/obj/$(basename "${1%.*}").o"; }
compile "$SRC/packet.cpp"
compile "$SRC/device/PacketQueue.cpp"
compile "$SRC/device/Connection.cpp"
compile "$ROOT/port/instrument/bf_trace.cpp"
compile "$OUT/obj/bf_packet_names.cpp"
compile "$ROOT/port/probe/probe_support.cpp"
compile "$ROOT/port/probe/packetlen.cpp"
compile "$ROOT/port/probe/netprobe.cpp"

link() { clang++ -target arm64-apple-macos14 "$@"; }
link "$OUT/obj/packetlen.o" "$OUT/obj/packet.o" -o "$OUT/bin/bifrost-packetlen"
link "$OUT/obj/netprobe.o" "$OUT/obj/Connection.o" "$OUT/obj/PacketQueue.o" "$OUT/obj/packet.o" \
     "$OUT/obj/bf_trace.o" "$OUT/obj/bf_packet_names.o" "$OUT/obj/probe_support.o" -o "$OUT/bin/bifrost-netprobe"
file "$OUT/bin/bifrost-packetlen" "$OUT/bin/bifrost-netprobe"
