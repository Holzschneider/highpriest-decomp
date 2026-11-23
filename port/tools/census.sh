#!/bin/bash
# Compiles every game source for arm64 macOS without generating code and
# reports what clang rejects. The totals are the progress measure of the
# first porting milestone.
#
#   port/tools/census.sh [--tree DIR] [--out DIR] [-- extra clang flags]
#
#   --tree  the source tree to check (default: src)
#   --out   where the per-file logs go (default: build/port/census)
#
# HP_WIN_HEADERS names the directory that holds windows.h, d3d.h and the rest
# (default: the Wine headers of Homebrew's game-porting-toolkit).
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TREE=$ROOT/src; OUT=$ROOT/build/port/census
while [ $# -gt 0 ]; do
  case "$1" in
    --tree) TREE=$(cd "$2" && pwd); shift 2;;
    --out) OUT=$2; shift 2;;
    --) shift; break;;
    *) echo "unknown argument: $1" >&2; exit 2;;
  esac
done
. "$ROOT/port/tools/flags.sh"
[ -f "$WIN_HEADERS/windows.h" ] || { echo "no windows.h in $WIN_HEADERS; set HP_WIN_HEADERS" >&2; exit 1; }
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd); find "$OUT" -name '*.log' -delete

# The link order of sources.mk lists the game's translation units.
GAME=$(awk '/^GAME :=/{f=1} f{print} f&&!/\\$/{f=0}' "$ROOT/sources.mk" | sed 's/GAME :=//; s/\\//g')

FLAGS="$HP_TARGET -fsyntax-only -ferror-limit=0 $HP_DEFINES $HP_INCLUDES -I$TREE -isystem $WIN_HEADERS $*"
export FLAGS TREE OUT

check() {
  local unit=$1 source driver
  if [ -f "$TREE/$unit.cpp" ]; then source=$unit.cpp; driver="clang++ -std=gnu++14"
  elif [ -f "$TREE/$unit.c" ]; then source=$unit.c; driver="clang -std=gnu89"
  else echo "missing source for $unit" >&2; return; fi
  $driver $FLAGS "$TREE/$source" > "$OUT/$(echo "$source" | tr / _).log" 2>&1
}
export -f check
echo $GAME | tr ' ' '\n' | xargs -P "$(sysctl -n hw.ncpu)" -I{} bash -c 'check {}'

cd "$OUT"
cat *.log > all.txt
errors() { grep -E ' (fatal )?error: ' "$@"; }
echo "sources:            $(ls *.log | wc -l | tr -d ' ')"
echo "clean:              $(grep -L -E ' (fatal )?error: ' *.log | wc -l | tr -d ' ')"
echo "errors:             $(errors all.txt | wc -l | tr -d ' ')"
echo "distinct locations: $(errors all.txt | cut -d: -f1-3 | sort -u | wc -l | tr -d ' ')"
echo "  pointer to int:   $(errors all.txt | grep 'cast from pointer to smaller type' | cut -d: -f1-3 | sort -u | wc -l | tr -d ' ')"
echo "  everything else:  $(errors all.txt | grep -v 'cast from pointer to smaller type' | cut -d: -f1-3 | sort -u | wc -l | tr -d ' ')"
echo
echo "most frequent messages (distinct locations, pointer casts left out):"
errors all.txt | grep -v 'cast from pointer to smaller type' | sort -u -t: -k1,3 \
  | sed -E "s/^[^ ]+ (fatal )?error: //; s/'[^']*'/'X'/g; s/[0-9]+/N/g" | sort | uniq -c | sort -rn | head -${HP_CENSUS_TOP:-15}
echo
echo "logs: $OUT"
