#!/bin/bash
# Compiles every game source for arm64 macOS without generating code and
# reports what clang rejects. The totals are the progress measure of the
# first porting milestone.
#
#   port/tools/census.sh [--tree DIR] [--out DIR] [--warnings] [-- extra clang flags]
#
#   --tree      the source tree to check (default: src)
#   --out       where the per-file logs go (default: build/port/census)
#   --warnings  the warnings mode (also BF_CENSUS_WARN=1): the project's flags
#               hide every warning (-Wno-everything), so this mode turns
#               -Wformat back on as an error (-Werror=format), and the totals
#               then count format faults: a wrong length modifier, an object
#               passed for %s, a sprintf that always overflows, arguments the
#               format does not use. -Wformat-security (a format string that
#               is not a literal) is left out; it is counted on its own line.
#               The faults are listed by location at the end; the pass mark
#               is "errors: 0".
#
# The units the build excludes (BF_CLIENT_EXCLUDE in the root CMakeLists.txt)
# are skipped and counted on the "excluded" line; BF_CENSUS_ALL=1 checks them.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TREE=$ROOT/src; OUT=$ROOT/build/port/census; WARN=${BF_CENSUS_WARN:-}
while [ $# -gt 0 ]; do
  case "$1" in
    --tree) TREE=$(cd "$2" && pwd); shift 2;;
    --out) OUT=$2; shift 2;;
    --warnings) WARN=1; shift;;
    --) shift; break;;
    *) echo "unknown argument: $1" >&2; exit 2;;
  esac
done
. "$ROOT/port/tools/flags.sh"
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd); find "$OUT" -name '*.log' -delete

# The link order of sources.mk lists the game's translation units.
GAME=$(awk '/^GAME :=/{f=1} f{print} f&&!/\\$/{f=0}' "$ROOT/sources.mk" | sed 's/GAME :=//; s/\\//g')
# The units the build leaves out, each with its reason (BF_CLIENT_EXCLUDE in
# the root CMakeLists.txt), are not checked; BF_CENSUS_ALL=1 checks them too.
EXCLUDED=$(awk '/^set\(BF_CLIENT_EXCLUDE/{f=1;next} f&&/^\)/{f=0} f' "$ROOT/CMakeLists.txt" | sed 's/#.*//')
if [ -z "${BF_CENSUS_ALL:-}" ]; then
  GAME=$(echo $GAME | tr ' ' '\n' | grep -vxF -f <(echo $EXCLUDED | tr ' ' '\n'))
fi

[ -n "$WARN" ] && set -- -Wformat -Werror=format -Wno-format-security -Wformat-security "$@"
FLAGS="$BF_TARGET -fsyntax-only -ferror-limit=0 $BF_DEFINES $BF_INCLUDES -I$TREE $*"
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
[ -z "${BF_CENSUS_ALL:-}" ] && echo "excluded:           $(echo $EXCLUDED | wc -w | tr -d ' ') (BF_CLIENT_EXCLUDE in CMakeLists.txt)"
echo "clean:              $(grep -L -E ' (fatal )?error: ' *.log | wc -l | tr -d ' ')"
echo "errors:             $(errors all.txt | wc -l | tr -d ' ')"
echo "distinct locations: $(errors all.txt | cut -d: -f1-3 | sort -u | wc -l | tr -d ' ')"
echo "  pointer to int:   $(errors all.txt | grep 'cast from pointer to smaller type' | cut -d: -f1-3 | sort -u | wc -l | tr -d ' ')"
echo "  everything else:  $(errors all.txt | grep -v 'cast from pointer to smaller type' | cut -d: -f1-3 | sort -u | wc -l | tr -d ' ')"
echo
echo "most frequent messages (distinct locations, pointer casts left out):"
errors all.txt | grep -v 'cast from pointer to smaller type' | sort -u -t: -k1,3 \
  | sed -E "s/^[^ ]+ (fatal )?error: //; s/'[^']*'/'X'/g; s/[0-9]+/N/g" | sort | uniq -c | sort -rn | head -${BF_CENSUS_TOP:-15}
if [ -n "$WARN" ]; then
  echo
  echo "warnings mode: -Werror=format; non-literal format strings (-Wformat-security): $(grep 'Wformat-security' all.txt | cut -d: -f1-3 | sort -u | wc -l | tr -d ' ') locations"
  echo "format faults by location:"
  errors all.txt | sort -u -t: -k1,3 | sed -E "s|^$ROOT/||" | cut -c1-200
fi
echo
echo "logs: $OUT"
