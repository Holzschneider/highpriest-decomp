#!/bin/bash
# Asks the kept Ghidra project of the reference executable (a stock 2008
# kRO Sakexe, see docs/port/reference-exe.md) for the function behind a
# string, an address or a name, and prints its decompilation or disassembly.
# The executable is only read, never run.
#
#   port/tools/refexe.sh --string TEXT [--exact]   functions referencing a string
#   port/tools/refexe.sh --addr 0x599B00          the function containing an address
#   port/tools/refexe.sh --func NAME              a function by name (FUN_00599b00,
#                                                 or CGameMode::OnUpdate once anchors
#                                                 are applied)
#   port/tools/refexe.sh --callers 0x599B00       who calls the function at an address
#   port/tools/refexe.sh --refs 0x7BEF10          every reference to an address (a
#                                                 global's reads and writes) and the
#                                                 functions that hold them
#   port/tools/refexe.sh --apply-anchors          name the functions of
#                                                 port/tools/refexe/anchors.tsv
#                                                 in the project (writes it)
#   port/tools/refexe.sh --setup                  import and auto-analyse the
#                                                 executable into a new project
# Options:
#   --disasm   print the instructions instead of the decompilation
#   --both     print both
#   --max N    at most N functions for --string (default 8)
#
# BF_REFEXE_DIR names the folder with the executable and the project
# (default ~/Games/<application name>/refexe). GHIDRA_HOME and JAVA_HOME
# default to Homebrew's ghidra and openjdk@25. Queries open the project
# read-only; one query runs at a time (a lock serialises callers).
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BF_APP_NAME=$(sed -n 's/^set(BF_APP_NAME "\(.*\)")/\1/p' "$ROOT/port/name.cmake")
DIR=${BF_REFEXE_DIR:-$HOME/Games/$BF_APP_NAME/refexe}
EXE_NAME=2008-09-03aSakexe.exe
PROJECT=sakexe-2008-09-03a
GHIDRA_HOME=${GHIDRA_HOME:-$(brew --prefix ghidra 2>/dev/null)/libexec}
export JAVA_HOME=${JAVA_HOME:-$(brew --prefix openjdk@25 2>/dev/null)/libexec/openjdk.jdk/Contents/Home}
HEADLESS=$GHIDRA_HOME/support/analyzeHeadless
SCRIPTS=$ROOT/port/tools/refexe

usage() { sed -n '2,/^set -u/p' "$0" | sed '$d; s/^# \{0,1\}//'; exit 2; }
MODE=; ARG=; VIEW=decomp; MAX=8; MATCH=substr
while [ $# -gt 0 ]; do
  case "$1" in
    --string) MODE=string; ARG=$2; shift 2;;
    --addr) MODE=addr; ARG=$2; shift 2;;
    --func) MODE=func; ARG=$2; shift 2;;
    --callers) MODE=xrefs; ARG=$2; shift 2;;
    --refs) MODE=refs; ARG=$2; shift 2;;
    --apply-anchors) MODE=anchors; ARG=$SCRIPTS/anchors.tsv; shift;;
    --setup) MODE=setup; shift;;
    --exact) MATCH=exact; shift;;
    --disasm) VIEW=disasm; shift;;
    --both) VIEW=both; shift;;
    --max) MAX=$2; shift 2;;
    -h|--help) usage;;
    *) echo "unknown argument: $1" >&2; usage;;
  esac
done
[ -n "$MODE" ] || usage
[ -x "$HEADLESS" ] || { echo "no Ghidra at $GHIDRA_HOME (brew install ghidra, or set GHIDRA_HOME)" >&2; exit 1; }
[ -x "$JAVA_HOME/bin/java" ] || { echo "no JDK at $JAVA_HOME (set JAVA_HOME)" >&2; exit 1; }

LOG=$DIR/ghidra/last-query.log
if [ "$MODE" = setup ]; then
  [ -f "$DIR/$EXE_NAME" ] || { echo "no $DIR/$EXE_NAME; see docs/port/reference-exe.md" >&2; exit 1; }
  [ -e "$DIR/ghidra/$PROJECT.gpr" ] && { echo "project exists: $DIR/ghidra/$PROJECT.gpr" >&2; exit 1; }
  mkdir -p "$DIR/ghidra"
  "$HEADLESS" "$DIR/ghidra" "$PROJECT" -import "$DIR/$EXE_NAME" -max-cpu 8 > "$DIR/ghidra/analysis.log" 2>&1
  rc=$?; tail -3 "$DIR/ghidra/analysis.log"; exit $rc
fi
[ -e "$DIR/ghidra/$PROJECT.gpr" ] || { echo "no project in $DIR/ghidra; run $0 --setup" >&2; exit 1; }

# Ghidra locks a project while it is open; wait for the previous query.
LOCK=$DIR/ghidra/.query.lock
for i in $(seq 600); do mkdir "$LOCK" 2>/dev/null && break; sleep 1; done
[ -d "$LOCK" ] || { echo "timed out waiting for $LOCK" >&2; exit 1; }
trap 'rmdir "$LOCK" 2>/dev/null' EXIT

OUT=$(mktemp "${TMPDIR:-/tmp}/refexe.XXXXXX")
RO=-readOnly; [ "$MODE" = anchors ] && RO=
"$HEADLESS" "$DIR/ghidra" "$PROJECT" -process "$EXE_NAME" -noanalysis $RO \
  -scriptPath "$SCRIPTS" -postScript RefexeQuery.java "$OUT" "$MODE" "$ARG" "$VIEW" "$MAX" "$MATCH" \
  > "$LOG" 2>&1
rc=$?
if [ -s "$OUT" ]; then cat "$OUT"; else
  echo "the query printed nothing (exit $rc); last lines of $LOG:" >&2; tail -15 "$LOG" >&2; rc=1
fi
rm -f "$OUT"
exit $rc
