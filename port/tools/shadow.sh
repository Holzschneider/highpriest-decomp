#!/bin/bash
# Makes a throwaway copy of src/ under build/port/shadow and applies the
# automatic rewrites of codemod.py to it, twice over: the text rules first,
# then the pointer-cast rule from a census of the result. The copy shows how
# much of the port's compile work is mechanical without touching src/.
#
#   port/tools/shadow.sh        # prints the census of the rewritten copy
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SHADOW=$ROOT/build/port/shadow
rm -rf "$SHADOW"; mkdir -p "$SHADOW"
cp -R "$ROOT/src" "$SHADOW/src"
"$ROOT/port/tools/codemod.py" --tree "$SHADOW/src"
"$ROOT/port/tools/census.sh" --tree "$SHADOW/src" --out "$ROOT/build/port/census-shadow-1" > /dev/null
"$ROOT/port/tools/codemod.py" --tree "$SHADOW/src" --rule ptrcast --diagnostics "$ROOT/build/port/census-shadow-1/all.txt"
"$ROOT/port/tools/census.sh" --tree "$SHADOW/src" --out "$ROOT/build/port/census-shadow"
