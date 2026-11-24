#!/bin/bash
# Copies the game data into the game folder from a machine that holds it.
#
#   BF_DATA_HOST=user@host BF_DATA_PATH=/path/to/ro BF_ROE_PATH=/path/to/ROenglishRE port/data/fetch.sh
#
# BF_DATA_HOST  ssh destination of the source machine; empty for a local path
# BF_DATA_PATH  folder on it that holds data.grf, rdata.grf and BGM/
# BF_ROE_PATH   ROenglishRE checkout on it (github.com/llchrisll/ROenglishRE);
#               empty to skip
# BF_CLIENT_DIR game folder to fill (default ~/Games/Bifrost)
#
# The same can be given as arguments: fetch.sh [HOST] DATA_PATH [ROE_PATH].
#
# rsync only reads on the source side. Files already in the game folder with
# the same size and time are skipped, so running it again copies only what is
# missing or changed; nothing in the game folder is deleted. Afterwards run
# port/data/assemble.py to build data/.
set -euo pipefail

if [ $# -ge 2 ]; then
    BF_DATA_HOST=$1; BF_DATA_PATH=$2; BF_ROE_PATH=${3:-${BF_ROE_PATH:-}}
elif [ $# -eq 1 ]; then
    BF_DATA_HOST=; BF_DATA_PATH=$1
fi
: "${BF_DATA_PATH:?set BF_DATA_PATH (and BF_DATA_HOST) to where data.grf, rdata.grf and BGM/ are}"
BF_DATA_HOST=${BF_DATA_HOST:-}
BF_ROE_PATH=${BF_ROE_PATH:-}
GAME=${BF_CLIENT_DIR:-$HOME/Games/Bifrost}

src() { if [ -n "$BF_DATA_HOST" ]; then printf '%s:%s' "$BF_DATA_HOST" "$1"; else printf '%s' "$1"; fi; }

mkdir -p "$GAME/sources"
RSYNC=(rsync -a --partial --human-readable --info=stats1,progress2)
rsync --version 2>/dev/null | grep -q 'version 3' || RSYNC=(rsync -a --partial --human-readable --stats)

echo "[fetch] archives and music from $(src "$BF_DATA_PATH") to $GAME"
"${RSYNC[@]}" "$(src "$BF_DATA_PATH/data.grf")" "$(src "$BF_DATA_PATH/rdata.grf")" "$GAME/"
"${RSYNC[@]}" "$(src "$BF_DATA_PATH/BGM/")" "$GAME/BGM/"

if [ -n "$BF_ROE_PATH" ]; then
    echo "[fetch] ROenglishRE from $(src "$BF_ROE_PATH")"
    "${RSYNC[@]}" "$(src "$BF_ROE_PATH/")" "$GAME/sources/ROenglishRE/"
fi

ls -l "$GAME/data.grf" "$GAME/rdata.grf"
echo "[fetch] BGM: $(ls "$GAME/BGM" | wc -l | tr -d ' ') files"
