#!/bin/bash
# Builds the headless second player as a native arm64 program:
#
#   build/port/bin/bifrost-bot
#
# Like the probes (port/probe/build.sh) it compiles the game's own
# packet.cpp, device/PacketQueue.cpp and device/Connection.cpp, taken from
# the rewritten copy that port/tools/shadow.sh makes until the rewrites are
# applied to src/ (BF_SRC=src switches to the real tree). Objects go to
# build/port/obj/bot so that the two builds can run side by side.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRC=${BF_SRC:-$ROOT/build/port/shadow/src}
[ -d "$SRC" ] || "$ROOT/port/tools/shadow.sh" > /dev/null
. "$ROOT/port/tools/flags.sh"
OUT=$ROOT/build/port; OBJ=$OUT/obj/bot; mkdir -p "$OUT/bin" "$OBJ"

# The bot opens no game files, so it keeps the C library's own file names
# (port/client/include/bf_crt.h) and links no adapter.
GAME="clang++ -std=gnu++14 -O1 -g $BF_TARGET $BF_DEFINES -DBF_COMPAT_ADAPTER $BF_INCLUDES -I$SRC -isystem $WIN_HEADERS"
# The command channel uses the POSIX socket headers and none of the game's.
PLAIN="clang++ -std=c++14 -O1 -g -target arm64-apple-macos14 -Wall -Wextra -Wno-unused-parameter"

"$ROOT/port/tools/gen_packet_names.py" "$SRC/packet.h" > "$OBJ/bf_packet_names.cpp"

compile() { $GAME -c "$1" -o "$OBJ/$(basename "${1%.*}").o"; }
compile "$SRC/packet.cpp" &
compile "$SRC/device/PacketQueue.cpp" &
compile "$SRC/device/Connection.cpp" &
compile "$ROOT/port/client/src/bf_replay.cpp" &
compile "$ROOT/port/instrument/bf_trace.cpp" &
compile "$OBJ/bf_packet_names.cpp" &
compile "$ROOT/port/probe/probe_support.cpp" &
compile "$ROOT/port/bot/bot.cpp" &
$PLAIN -c "$ROOT/port/bot/bot_io.cpp" -o "$OBJ/bot_io.o" &
$PLAIN -I"$ROOT/port/native/sys/include" -c "$ROOT/port/native/sys/src/bf_clock.cpp" -o "$OBJ/bf_clock.o" &
for job in $(jobs -p); do wait "$job"; done

clang++ -target arm64-apple-macos14 \
	"$OBJ/bot.o" "$OBJ/bot_io.o" "$OBJ/Connection.o" "$OBJ/bf_replay.o" "$OBJ/PacketQueue.o" "$OBJ/packet.o" \
	"$OBJ/bf_trace.o" "$OBJ/bf_packet_names.o" "$OBJ/probe_support.o" "$OBJ/bf_clock.o" -o "$OUT/bin/bifrost-bot"
file "$OUT/bin/bifrost-bot"
