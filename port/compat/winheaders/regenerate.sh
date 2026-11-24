#!/bin/bash
# Rebuilds the Windows headers in this directory from one upstream Wine
# release: the headers Wine installs as include/wine/windows, including those
# its build generates from .idl files with widl. See README.md.
#
#   port/compat/winheaders/regenerate.sh
#
# Needs git, make, clang, flex and bison 3 (Homebrew: brew install bison).
# The Wine checkout and its build stay under build/winheaders in the
# repository (BF_WINHEADERS_WORK overrides); a second run reuses the checkout.
set -euo pipefail

TAG=wine-11.0
COMMIT=db11d0fe6a169c457e23d007e20404643d067aa8
URL=https://gitlab.winehq.org/wine/wine.git

DEST=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$DEST/../../.." && pwd)
WORK=${BF_WINHEADERS_WORK:-$ROOT/build/winheaders}
SRC=$WORK/$TAG
OBJ=$WORK/obj

# Bison 3: macOS ships 2.3, which Wine's configure rejects.
BISON_DIR=$(brew --prefix bison 2>/dev/null)/bin
[ -x "$BISON_DIR/bison" ] || { echo "regenerate.sh: needs bison 3 (brew install bison)" >&2; exit 1; }
export PATH=$BISON_DIR:$PATH

# 1. The tag, checked against the commit it named when this script was written.
mkdir -p "$WORK"
[ -d "$SRC/.git" ] || git clone -q --depth 1 --branch "$TAG" -c advice.detachedHead=false "$URL" "$SRC"
[ "$(git -C "$SRC" rev-parse HEAD)" = "$COMMIT" ] || { echo "regenerate.sh: $SRC is not $COMMIT" >&2; exit 1; }

# 2. widl and the generated headers, nothing else. Wine's configure insists on
#    a PE cross-compiler for an aarch64 host, which only the DLLs need. The
#    tools are host programs, so configure is told the host is x86-64 (64-bit,
#    no -m32) and three of its checks are answered from the cache: the ms_abi
#    check and the __x86_64__ define only matter to code that is never built
#    here, and clang still compiles widl natively for arm64. Generated headers
#    do not depend on the host: widl writes the same text for every target.
rm -rf "$OBJ"; mkdir -p "$OBJ"
( cd "$OBJ" && "$SRC/configure" -q --build=x86_64-apple-darwin --host=x86_64-apple-darwin \
    --enable-win64 --disable-tests --without-mingw --without-x --without-freetype \
    --without-gnutls --without-gstreamer --without-sdl --without-usb --without-pcap \
    --without-cups --without-sane --without-krb5 --without-netapi --without-opencl \
    --without-vulkan --without-wayland --without-ffmpeg --without-inotify --without-unwind \
    --without-pulse --without-gphoto --without-v4l2 --without-capi --without-dbus \
    --without-coreaudio ac_cv_mabi_ms=yes ac_cv_cpp_def___x86_64__=no > configure.out 2>&1 ) \
  || { echo "regenerate.sh: configure failed, see $OBJ/configure.out" >&2; exit 1; }
make -C "$OBJ" -j"$(sysctl -n hw.ncpu)" include/all > "$OBJ/make.out" 2>&1 \
  || { echo "regenerate.sh: make failed, see $OBJ/make.out" >&2; exit 1; }

# 3. The installed set: include/*.h, .inl, .rh, .x and ddk/ from the tag, and
#    the generated include/*.h (config.h is the build's own, not installed).
#    Left out: the .idl sources (the generated headers replace them) and the
#    Windows Runtime headers (windows.*.h, Windows 8 and later, 15 MB) with any
#    header that includes one of them.
find "$DEST" -mindepth 1 ! -name README.md ! -name regenerate.sh -delete
mkdir -p "$DEST/ddk"
cp "$SRC"/include/*.h "$SRC"/include/*.inl "$SRC"/include/*.rh "$SRC"/include/*.x "$DEST/"
cp "$SRC"/include/ddk/*.h "$DEST/ddk/"
cp "$OBJ"/include/*.h "$DEST/"
rm -f "$DEST/config.h"
find "$DEST" -maxdepth 1 -name 'windows.*.h' ! -name windows.h -delete
while :; do
  orphans=$(cd "$DEST" && grep -oE '#include *[<"]windows\.[a-z0-9_.]+\.h[>"]' -- *.h ddk/*.h \
    | sed -E 's/:#include *[<"]/ /; s/[>"]$//' \
    | while read -r file inc; do [ -f "$DEST/$inc" ] || echo "$file"; done | sort -u) || true
  [ -n "$orphans" ] || break
  (cd "$DEST" && rm -f $orphans)
done

# 4. Licence and provenance.
cp "$SRC/LICENSE" "$SRC/COPYING.LIB" "$SRC/AUTHORS" "$DEST/"
printf '%s\n' "Wine $TAG, commit $COMMIT, from $URL" > "$DEST/WINE-VERSION.txt"

# 5. arm64 adjustments: none to the files. Wine's minwindef.h already defines
#    __stdcall, __cdecl, __fastcall and __thiscall as nothing on any target
#    other than i386 and x86-64, so WINAPI, CALLBACK, STDMETHODCALLTYPE and
#    the rest expand to nothing on arm64; port/compat/include/bf_prefix.h
#    covers the VC6 spellings the client uses before windows.h is read.

echo "regenerate.sh: $(find "$DEST" -type f -name '*.h' | wc -l | tr -d ' ') headers," \
  "$(du -sh "$DEST" | cut -f1) in $DEST"
