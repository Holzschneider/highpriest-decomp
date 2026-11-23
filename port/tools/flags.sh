# Sourced by the port's build scripts. Sets:
#   HP_DEFINES   the preprocessor symbols of the original project (BASE in
#                sources.mk) plus HP_PORT, which marks the macOS build
#   HP_INCLUDES  the port's include paths, ahead of the Windows headers
# ROOT must be the repository; HP_WIN_HEADERS may name the directory with
# windows.h, d3d.h and the rest.
WIN_HEADERS=${HP_WIN_HEADERS:-/usr/local/include/wine/windows}
HP_DEFINES="-DHP_PORT -DWIN32 -D_WINDOWS -DNDEBUG -D_MBCS -DWEDDING_PATCH -DMOB_SKILL_PATCH -D_UPDATE2005A \
  -D_UPDATE2005C -D_UPDATE2004B -DBALANCE_PATCH -DMOB_SKILL -DSECONDJOB_SKILL2 -D_HOMUN -D_MER -D_MER_R2 \
  -D_PACKET_VER=7 -DKILLER_SYSTEM_V2 -D_HUNTING_TIME_LIST -D__PARTY_INVITE -DBIND_ON_EQUIP"
HP_INCLUDES="-include $ROOT/port/compat/include/hp_prefix.h -I$ROOT/port/compat/include -I$ROOT/port/instrument"
HP_TARGET="-target arm64-apple-macos13 -fdeclspec -fshort-wchar -Wno-everything"
