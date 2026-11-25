# port/client

The port's own code that stays in the client after M11: neither a native module (those know no client headers and no Windows types) nor an adapter (`port/compat`, which M11 deletes). M11.0d moved here the prefix header, the entry point, the control bindings, the packet replay, the crash handler and the CRT helpers, which lived in `port/compat/stubs` and `port/compat/include` (history is kept: `git log --follow`).

    include/             what the client's sources and the build include (-I, in flags.sh)
      bf_prefix.h        force-included in every client unit (VC6 spellings, the CRT names macOS lacks)
      bf_crt.h           the C library's file calls on client paths (was bf_win32_crt.h); included by bf_prefix.h
      bf_port.h          what `src/` calls under BF_PORT (graphics start, per-frame hook, zlib wrappers, ...)
      bf_replay.h        packet replay contract (BF_PKT_REPLAY)
      bf_setting_key.h   the client's settings keys on bf_settings (M11.4; C++): the registry paths it names
      bf_types.h         the Windows-shaped vocabulary the client keeps (M11.0b)
    src/                 the entry point, the control bindings, the client state for the control socket,
                         the crash handler, the zlib wrappers, the CRT helpers, packet replay;
                         README.md lists the files (headers used only by these sources sit beside them)
    test/                bifrost-types-test(-c): bf_types.h against wine_types.ref, what the Wine headers gave it
                         bifrost-crt-test: bf_crt.cpp called as the client calls it (root CMakeLists.txt)
                         bifrost-setting-key-test: bf_setting_key.h and the Settings.plist it leaves (root CMakeLists.txt)

`include/` holds the headers that a `src/` unit or another program includes by name (`bf_port.h` and `bf_replay.h` from `src/`, `bf_prefix.h` through `-include`); `flags.sh` puts it on the include path ahead of `port/compat/include`. The `bf_client_*.h` and `bf_control_*.h` headers are contracts between files of `src/` only and stay there. `bf_crt.h` still redirects `fopen` and the other file calls to `bf_crt_*`, which `src/bf_crt.cpp` implements on `bf_file.h` since M11.3; it is transitional: a call site that leaves the Windows names calls `bf_file.h` itself (M11.3 did so in `base/File.cpp`, `base/GPak.cpp`, `base/Util.cpp`, `skin.cpp`), and the macros go with the last of them.

## bf_types.h

What the client still needs once every call it makes is native: the Win32 integer and pointer typedefs, opaque handles, the structures its own code fills (RECT, POINT, SIZE, PALETTEENTRY, RGBQUAD, the bitmap headers, FILETIME, SYSTEMTIME, LOGFONTA, D3DMATRIX), the arithmetic macros (RGB, LOWORD, MAKELONG, ZeroMemory, MAX_PATH, the calling-convention words, `__max`/`__min`, `min`/`max` for C++ only), and the constants the client uses as its own numbers: `VK_*`, the `WM_*` ids it dispatches internally, the LOGFONTA character sets and weights, and the Direct3D 7 render, blend, texture-stage, primitive and vertex-format values in their 2008 numbering. M11.11 puts it where `src/common.h` includes `windows.h`, `d3d.h` and the rest today.

Every type, record and constant is exactly what the vendored Wine headers give the build today (arm64, the flags of `port/tools/flags.sh`); the test proves it. The names come from `port/tools/winnames.py` (group `keep`, which follows this header). Valid gnu89 C and C++14; it refuses Objective-C(++) (its `BOOL` is `int`).

A name a later M11 step finds it needs goes in here only if it is a type or a number of the client's own; a call never does. Add it, rebuild the test: the generator picks every new typedef, record, enumerator and macro up by itself (a function-like macro needs sample arguments in `SAMPLES` of `test/gen_types_test.py`).

## bifrost-types-test

    cmake --build --preset debug --target types && ctest --test-dir build/cmake/debug -R types
    cmake -S port/client/test -B build/client/test -G Ninja      (standalone)
    cmake --build build/client/test && (cd build/client/test && ctest)

`test/wine_types.ref` is what the Wine headers gave every name of the header, recorded in M11.11 before they left the repository: one line per name and check, `key<TAB>value`, a `[c++]` and a `[c]` section. `gen_types_test.py` reads the header's typedefs, records, enums and macros with clang (once as C++14, once as gnu89 C) and writes, per language, a program that includes `bf_types.h` alone with a `src/` unit's flags and compares at run time with the reference: typedefs by type (C++: spelled out, records and enums by tag; C: size, alignment, type class, signedness), records by size, alignment, field names, offsets (and field types in C++), enumerators by value and enums by underlying type, macros by value and type (function-like ones on sample arguments) or by expansion. `bifrost-types-test` (C++) and `bifrost-types-test-c` print one `FAIL` line per difference, one per name the reference lacks, and a summary; both are headless ctest tests of the root build. The build also compiles the header alone as gnu89 C and C++14 with a `src/` unit's flags, and strictly (`-Wall -Wextra -Werror`, `-pedantic` for C++) without the prefix; ctest checks that an Objective-C++ unit is refused.

A name added to `bf_types.h` fails until the reference has it: get the Wine headers back from history (port/README.md, "The Windows headers for reference work"), then `BF_WIN_HEADERS=<dir> python3 port/client/test/gen_types_test.py --write-reference port/client/test/wine_types.ref` compiles the same programs against Wine (with the Windows includes `src/common.h` had) and rewrites the file; the diff shows exactly what was added.
