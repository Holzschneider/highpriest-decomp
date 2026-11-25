# The client's compile flags: the one place they are written. Sourced by the
# port's scripts (census.sh, probe/build.sh, layoutcheck.py), and run by the
# CMake project at configure time, which takes every word from here. Sets:
#   BF_APP_NAME  the application name, from port/name.cmake
#   BF_DEFINES   the preprocessor symbols of the original project (BASE in
#                sources.mk) plus BF_PORT, which marks the macOS build
#   BF_INCLUDES  the port's include paths, ahead of the Windows headers
#   BF_TARGET    the target (arm64, macOS 14) and the language options
#   WIN_HEADERS  the Windows headers, passed with -isystem after the game's
#                own -I paths
# ROOT must be the repository; BF_WIN_HEADERS may name the directory with
# windows.h, d3d.h and the rest.
WIN_HEADERS=${BF_WIN_HEADERS:-$ROOT/port/compat/winheaders}
BF_APP_NAME=$(sed -n 's/^set(BF_APP_NAME "\(.*\)")/\1/p' "$ROOT/port/name.cmake")
BF_DEFINES="-DBF_PORT -DBF_APP_NAME=\"$BF_APP_NAME\" -DWIN32 -D_WINDOWS -DNDEBUG -D_MBCS -DWEDDING_PATCH -DMOB_SKILL_PATCH -D_UPDATE2005A \
  -D_UPDATE2005C -D_UPDATE2004B -DBALANCE_PATCH -DMOB_SKILL -DSECONDJOB_SKILL2 -D_HOMUN -D_MER -D_MER_R2 \
  -D_PACKET_VER=7 -DKILLER_SYSTEM_V2 -D_HUNTING_TIME_LIST -D__PARTY_INVITE -DBIND_ON_EQUIP"
BF_INCLUDES="-include $ROOT/port/client/include/bf_prefix.h -I$ROOT/port/client/include -I$ROOT/port/native/sys/include -I$ROOT/port/native/formats/granny/include -I$ROOT/port/compat/include -I$ROOT/port/instrument"
BF_TARGET="-target arm64-apple-macos14 -fdeclspec -fshort-wchar -Wno-everything"
