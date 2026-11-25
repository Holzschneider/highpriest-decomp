# compat/win32: kernel32, the registry, ShellExecute, MessageBox

The Win32 system names the client calls (step M3.1a), on `port/native/sys` (`bf_file`, `bf_settings`, Keychain, alerts) and `port/native/text` (code pages). Stage A only: M11 deletes this folder and changes the call sites to the native functions named below. Time, threads, critical sections and `Interlocked*` left with M11.1 (`bf_win32_thread.cpp`): the client calls `bf_clock_*`, `bf_sys_thread_*` and `bf_sys_mutex_*` itself, and the compiler's atomics. Files, archive mappings, large blocks and the heap left with M11.3: the client calls `bf_file_*` (`port/native/sys`) and the C library; the C library's file names on client paths (`bf_crt.h`) are `port/client/src/bf_crt.cpp`'s. What is left of `bf_win32_file.cpp` is `FindFirstFileA` and `FindClose` for `base/FindHack.cpp`.

```
cmake --preset debug && cmake --build --preset debug --target bf_compat_win32 bifrost-compat-win32-test
build/cmake/debug/compat/win32/bifrost-compat-win32-test     # or: ctest --test-dir build/cmake/debug -R compat-win32
```

The test writes only below `$BF_TEST_OUT/compat-win32-test` (`BF_HOME` points there, a small game folder is made there); the Keychain round trip says `skip` when the login keychain is out of reach. `test/client_calls.cpp` is compiled with the client's own flags, so its `fopen`, `access`, `_mkdir`, `remove`, ... are the client's, implemented by `port/client/src/bf_crt.cpp`, which the test compiles in.

## The client's file calls: one mechanism

Every path the client passes is translated by `bf_file` (backslashes, any letter case, CP949, relative to the game folder or below `g_baseDir3`): reads find the file in the game folder or in a write location first; writes go to the write locations (Documents, Pictures, Application Support, by the first path component) and are refused in the game folder (`ERROR_ACCESS_DENIED`, `errno` `EACCES`).

- **The client's own file code** (`base/File.cpp`, `base/GPak.cpp`, `base/Util.cpp`, `skin.cpp`) calls `bf_file` directly since M11.3; `FindFirstFile` here (for `base/FindHack.cpp`) too.
- **The C library's file functions** in game units go through `port/client/include/bf_crt.h`, which `bf_prefix.h` includes: `fopen`, `access`, `remove`, `rename`, `unlink`, `rmdir` are object-like macros for `bf_crt_*` (`port/client/src/bf_crt.cpp`; `fopen` is `bf_file_fopen`); `_access`, `_unlink`, `_rmdir` map onto them, `_chmod` to `bf_crt_chmod`; `_mkdir` is a real function there, `_getcwd` here. Lua's `io.open`, `os.remove`, `os.rename` and `luaL_loadfile` are covered the same way.
- **Port code** keeps the C library's names: `bf_compat_flags` (root `CMakeLists.txt`) defines `BF_COMPAT_ADAPTER`, which turns the macros off; native modules do not see the prefix at all.
- `GetCurrentDirectory` (the client's `g_baseDir3`) and `_getcwd` return the game folder's POSIX path; the process's working folder is not changed and nothing depends on it.

The macros rename the name everywhere in a game unit, the standard headers included, so `std::remove` (the algorithm) and `std::list::remove` keep working (checked by the test, and the compile census of `macos-port`'s `src/` has no new error: 2841 -> 2829, the 12 fewer are `_unlink`/`_rmdir`/`_chmod`/`_S_IWRITE` now declared). Not covered: iostreams (`basic_filebuf<char>` lives in libc++.dylib and calls the real `fopen`; the client uses none), `tmpfile`, `freopen`, `stat`, `open` (the client calls none of them on game paths).

## Names

Kernel objects (folder listings) are registered handles: a handle the client never got (`FindClose(INVALID_HANDLE_VALUE)`, `RegCloseKey` of a key whose open failed) fails with `ERROR_INVALID_HANDLE` instead of crashing. `GetLastError` is per thread. `CP_ACP` is `bf_file_user_codepage()`, the code page the client sets with `bf_file_set_user_codepage` per service (949 by default, 1252); it also decodes registry names and `REG_SZ` data and message box text.

| Name | Maps to | Notes |
|---|---|---|
| `CloseHandle` | the object's release | the process pseudo-handle ignored |
| `FindFirstFileA`, `FindClose` | `bf_file_find_*` | `*`/`?` without regard to case; `.` and `..` not listed; game files `FILE_ATTRIBUTE_READONLY` |
| `GlobalAlloc/Lock/Unlock/Free/Size`, `LocalAlloc/Lock/Free` | `malloc` with a size header | the handle is the memory (as `GMEM_FIXED`); `GlobalSize` for the clipboard adapter; callers: `framework/Language.cpp` and the input adapter (clipboard), `FormatMessageA` for `base/FindHack.cpp` |
| `GlobalMemoryStatus` | `sysctl hw.memsize` | |
| `GetLastError`, `SetLastError` | thread-local | |
| `GetCurrentDirectoryA`, `_getcwd` | `bf_file_location(BF_LOCATION_GAME)` | |
| `GetModuleFileNameA`, `GetModuleHandleA` | `_NSGetExecutablePath` | the executable (CLoginMode hashes it); `GetModuleHandle(name)` fails |
| `GetCommandLineA`, `ExitProcess`, `GetCurrentProcess`, `GetCurrentProcessId`, `SetPriorityClass` | `_NSGetArgv`, `exit`, `getpid`; no-op | |
| `GetVersionExA`, `GetSystemDirectoryA`, `GetUserDefaultLangID` | constants | Windows XP SP2; `C:\WINDOWS\system32`; English (the language folder `english\`) |
| `MultiByteToWideChar`, `WideCharToMultiByte` | `bf_text_to_utf16`, `bf_text_from_utf16` | `-1` lengths count the NUL; small buffer `ERROR_INSUFFICIENT_BUFFER` |
| `IsDBCSLeadByte`, `IsDBCSLeadByteEx`, `GetACP` | `bf_text_is_lead_byte`, `bf_file_user_codepage` | |
| `MulDiv`, `lstrcmpiA`, `lstrcatA`, `lstrcpyA`, `lstrlenA` | arithmetic, `strcasecmp`, `strcat`... | |
| `OutputDebugStringA` | `BF_TRACE(BFT_W32)` | |
| `FormatMessageA` | "Error N." | `FROM_SYSTEM` only (with `ALLOCATE_BUFFER`); other forms: stub |
| `RegOpenKeyExA`, `RegCreateKeyExA` | `bf_settings_has_group`, `bf_settings_create_group` | see below |
| `RegQueryValueExA`, `RegSetValueExA` | `bf_settings_type` + typed getters, typed setters | size queries, `ERROR_MORE_DATA` with the size; `REG_SZ` stored up to its first NUL |
| `RegDeleteKeyA`, `RegDeleteValueA` | `bf_settings_remove` | a key goes with its subkeys |
| `RegCloseKey` | `bf_settings_save` | |
| `MessageBoxA` | trace (`BFT_UI`) + `bf_sys_alert` | shown unless `BF_NO_ALERTS`, no window server or off the main thread; then the first button (`IDOK`/`IDYES`) |
| `ShellExecuteA` | `bf_sys_open_url`, `bf_sys_open_path` | `open` of a URL or a resolved path; > 32 on success |

Called by the client and deliberately failing (`BF_STUB()` or a trace line, with the value the callers handle):

| Name | Returns | Caller |
|---|---|---|
| `LoadLibraryA`, `GetProcAddress`, `FreeLibrary` | `NULL`/`ERROR_MOD_NOT_FOUND`, `NULL`, `TRUE` | base/FindHack.cpp (GameGuard, keyboard protection) |
| `CreateProcessA` | `FALSE`, `ERROR_NOT_SUPPORTED`, zeroed `PROCESS_INFORMATION` | WinMain's patcher restart |
| `OpenProcess` | `NULL`, `ERROR_ACCESS_DENIED` | base/FindHack.cpp |
| `CryptAcquireContextA` and `CryptCreateHash`, `CryptHashData`, `CryptDeriveKey`, `CryptDecrypt` | `FALSE` (`CryptRelease*`, `CryptDestroy*`: `TRUE`) | base/FindHack.cpp |
| `SetUnhandledExceptionFilter` | the previous filter; never called | WinMain (the port's signal handler writes the backtrace) |

Not implemented because the client does not call them: events, `ExitThread`/`_endthreadex`, `TerminateThread`, `SetCurrentDirectory`, `CopyFile`, `RegEnumValue`, `_findfirst` (declared by `io.h`).

ole32 and netapi32 (`src/bf_win32_misc.cpp`, M3.2):

| Name | Returns | Caller |
|---|---|---|
| `CoInitialize`, `CoUninitialize` | `S_OK`, nothing: the client makes no COM object through ole32 (DirectX comes from the adapters' entry points) | `WinMain` |
| `Netbios` | `NRC_SYSTEM` (`BF_STUB`): no machine identifier is handed out; the login code then sends `111111111111` | `CLoginMode` (Korean and PC-bang login packets) |

`wsprintfA` is the input area's.

## The registry

```
HKLM\Software\<name>[\sub\key]   ->  Client[/sub/key]                 (g_regPath, all the client's keys)
any other key                    ->  Registry/<HKLM|HKCU|HKCR|HKU>/<path>
```

Key names are decoded from `CP_ACP` to UTF-8 (a server name in a `BookMark` or `NameBaloon` key is stored readable) and compared without regard to case, by `bf_settings`. `REG_DWORD` (4 bytes) is an integer, `REG_SZ`/`REG_EXPAND_SZ` a string, anything else data. The client's root keys exist before anything is written (the defaults of `bf_settings`), so `ReadRegistry` succeeds and the window positions of `UIWindowMgr` read their defaults. Two values of the client's root are not stored as values:

- `ID` (the login window's remembered account) is `Account/Name` (`bf_settings_account_name`);
- `PASSWORD` is the Keychain item of that account (`bf_sys_keychain_*`): written there, `""` deletes it, read back from it; never in the plist. Without an account name or a reachable Keychain it is "not found", which is what the login window starts with.

## Changes outside this folder

- `port/native/sys`: `bf_file_user_codepage()` (the code page last set, for `CP_ACP`) and `bf_file_remove_folder()` (for `rmdir`), with checks in `bifrost-sys-test`.
- `port/client/include/bf_crt.h` (new) and its `#include` in `bf_prefix.h`.
- Root `CMakeLists.txt`: `bf_compat_flags` defines `BF_COMPAT_ADAPTER`.
