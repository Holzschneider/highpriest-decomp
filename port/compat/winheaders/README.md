# Windows headers

The client is compiled against Wine's Windows headers, for their declarations only; nothing of Wine is linked (technical plan 3.1, decision 7). This directory holds them at a fixed version, so the build needs neither the network nor a Wine installation. `port/tools/flags.sh` passes it to clang with `-isystem`, after the port's own `-I` paths, so `port/compat/include` wins where both have a header (`winsock.h`). `BF_WIN_HEADERS` names another directory instead.

**Version.** Tag `wine-11.0`, commit `db11d0fe6a16` (13 January 2026), recorded in `WINE-VERSION.txt`. It is the newest stable release (stable releases are the `.0` tags; 11.x after it are development snapshots). The headers this replaces, those of Homebrew's x86-64 Game Porting Toolkit, are an older CrossOver-derived Wine with Apple's changes.

**Contents.** What Wine installs as `include/wine/windows`: the headers of the tag's `include/` (`.h`, `.inl`, `.rh`, `.x`, `ddk/`) and those Wine's build generates from its `.idl` files with `widl` (`objidl.h`, `unknwn.h`, `oaidl.h`, `wtypes.h`, `ocidl.h`, `urlmon.h`, `mmdeviceapi.h`, ...; 252 of them here). Left out are the `.idl` sources themselves and the Windows Runtime headers (`windows.*.h`, Windows 8 and later, 15 MB), together with the one header that includes them (`dispatcherqueue.h`). About 37 MB, 833 headers. `msvcrt/` holds Wine's C runtime headers (what Wine installs as `include/wine/msvcrt`, 62 files) for the 32-bit Windows compile of `port/tools/layoutcheck.py`; being a subdirectory, it is never searched by the macOS build. `LICENSE`, `COPYING.LIB` and `AUTHORS` are Wine's.

**Licence.** LGPL 2.1 or later (`LICENSE`, `COPYING.LIB`); the row in `THIRD-PARTY.md` lists it. The files are unchanged copies.

**arm64.** No file is edited. Wine's `minwindef.h` defines `__stdcall`, `__cdecl`, `__fastcall` and `__thiscall` as nothing on every target except i386 and x86-64, so `WINAPI`, `CALLBACK`, `PASCAL`, `STDMETHODCALLTYPE` and the rest expand to nothing on arm64; no header uses `ms_abi` or another calling-convention attribute directly. `port/compat/include/bf_prefix.h` defines the VC6 spellings (`__stdcall`, `_cdecl`, ...) as nothing for code read before `windows.h`. (The Game Porting Toolkit headers defined `__stdcall` as `__attribute__((ms_abi))` on arm64, which clang turned into "previously declared without calling convention" errors; those are gone.)

## Regenerating

    port/compat/winheaders/regenerate.sh

It needs git, make, clang, flex (macOS has 2.6) and bison 3 (`brew install bison`; macOS ships 2.3). It clones the tag into `build/winheaders` (shallow; `BF_WINHEADERS_WORK` moves it), checks the commit, configures Wine there, builds `widl` and the generated headers (`make include/all`, nothing else), deletes everything in this directory but this README and the script, and copies the set back. About 25 seconds with the checkout present, plus about 40 for the clone. A second run produces the same files.

Wine's `configure` requires a PE cross-compiler on an aarch64 host, which only the DLLs need. The script therefore configures for an x86-64 host with `--enable-win64` and answers two cache checks (`ac_cv_mabi_ms=yes`, `ac_cv_cpp_def___x86_64__=no`) that concern only code it never builds; clang still compiles `widl` natively for arm64. Generated headers do not depend on the host.

To move to another release, change `TAG` and `COMMIT` at the top of the script, run it, and rerun `port/tools/census.sh` before and after.
