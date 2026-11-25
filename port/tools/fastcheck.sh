#!/bin/bash
# The check the lead runs at each integration: configures and builds the
# debug preset for everything that is expected to build, runs the native test
# programs that need no window and no sound, and runs the compile census.
# Prints one summary line per part; exits non-zero when a part fails.
#
#   port/tools/fastcheck.sh [--client | --client-errors]
#
#   --client         the client target is expected to build and link (turn
#                    this on when M2 lands)
#   --client-errors  build the client anyway and pass when its compile errors
#                    are exactly the census's for the same units (a mismatch
#                    means the CMake flags and flags.sh have drifted apart)
#
# Logs: build/fastcheck/<part>.log
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
CLIENT=none
case "${1:-}" in
  --client) CLIENT=build;;
  --client-errors) CLIENT=errors;;
  "") ;;
  *) echo "usage: $0 [--client | --client-errors]" >&2; exit 2;;
esac
cd "$ROOT"
LOGS=$ROOT/build/fastcheck; mkdir -p "$LOGS"
BIN=$ROOT/build/cmake/debug
FAILED=0
START=$SECONDS

line() { printf '%-10s %-5s %s\n' "$1" "$2" "$3"; }
fail() { line "$1" FAIL "$2 (log: build/fastcheck/$1.log)"; FAILED=1; }
since() { echo "$((SECONDS - $1))s"; }

# configure
t=$SECONDS
if cmake --preset debug > "$LOGS/configure.log" 2>&1; then
  line configure ok "debug preset, $(since $t)"
else
  fail configure "cmake --preset debug"
  exit 1
fi

# build: the native modules and their tests, the probes, the client if asked
TARGETS="native probes"; [ $CLIENT = build ] && TARGETS="$TARGETS $(sed -n 's/^set(BF_APP_NAME "\(.*\)")/\1/p' port/name.cmake | tr '[:upper:]' '[:lower:]')"
t=$SECONDS
if cmake --build --preset debug --target $TARGETS -- -k 0 > "$LOGS/build.log" 2>&1; then
  warnings=$(grep -c ' warning: ' "$LOGS/build.log")
  line build ok "$TARGETS; $warnings warnings, $(since $t)"
else
  fail build "$(grep -c '^FAILED' "$LOGS/build.log") steps failed in: $TARGETS"
fi

# tests: the native test programs that need no window and no sound
t=$SECONDS
if ctest --test-dir "$BIN" -L headless -j "$(sysctl -n hw.ncpu)" --output-on-failure > "$LOGS/tests.log" 2>&1; then
  line tests ok "$(grep -E 'tests passed' "$LOGS/tests.log"), headless, $(since $t)"
else
  fail tests "$(grep -E 'tests passed|tests failed' "$LOGS/tests.log")"
fi

# census
t=$SECONDS
if port/tools/census.sh > "$LOGS/census.log" 2>&1; then
  get() { sed -n "s/^$1: *//p" "$LOGS/census.log"; }
  line census ok "$(get sources) sources, $(get clean) clean, $(get errors) errors at $(get 'distinct locations') locations, $(since $t)"
else
  fail census "port/tools/census.sh"
fi

# client
case $CLIENT in
  none) line client skip "not expected to build yet (--client, --client-errors)";;
  build) ;;  # built above
  errors)
    t=$SECONDS
    exe=$(sed -n 's/^set(BF_APP_NAME "\(.*\)")/\1/p' port/name.cmake | tr '[:upper:]' '[:lower:]')
    cmake --build --preset debug --target "$exe" -- -k 0 > "$LOGS/client.log" 2>&1
    built=$(grep -cE ' (fatal )?error: ' "$LOGS/client.log")
    units=$(grep -c '^FAILED' "$LOGS/client.log")
    expected=0; expected_units=0
    while read -r source; do
      log=build/port/census/$(echo "${source#src/}" | tr / _).log
      n=$(grep -cE ' (fatal )?error: ' "$log" 2>/dev/null); n=${n:-0}
      expected=$((expected + n)); [ "$n" -gt 0 ] && expected_units=$((expected_units + 1))
    done < "$BIN/client-units.txt"
    if [ "$built" = "$expected" ] && [ "$units" = "$expected_units" ]; then
      line client ok "$built errors in $units units, as the census has for the same units, $(since $t)"
    else
      fail client "$built errors in $units units; the census has $expected in $expected_units"
    fi;;
esac

line total "$([ $FAILED = 0 ] && echo ok || echo FAIL)" "$(since $START)"
exit $FAILED
