# compat/win32: kernel32, the registry, winmm's clock, ShellExecute, MessageBox, the C library's file calls

The Win32 system names the client calls (step M3.1a), on `port/native/sys` (`bf_clock`, `bf_file`, `bf_settings`, threads, Keychain, alerts) and `port/native/text` (code pages). Stage A only: M11 deletes this folder and changes the call sites to the native functions named below.

```
cmake --preset debug && cmake --build --preset debug --target bf_compat_win32 bifrost-compat-win32-test
build/cmake/debug/compat/win32/bifrost-compat-win32-test     # or: ctest --test-dir build/cmake/debug -R compat-win32
```

The test writes only below `$BF_TEST_OUT/compat-win32-test` (`BF_HOME` points there, a small game folder is made there) and reads `data.grf` from the real game folder (`$BF_CLIENT_DIR` or `~/Games/<name>`); that check says `skip` without game data, as does the Keychain round trip when the login keychain is out of reach. `test/client_calls.cpp` is compiled with the client's own flags, so its `fopen`, `access`, `_mkdir`, `remove`, ... are the client's.

## The client's file calls: one mechanism

Every path the client passes is translated by `bf_file` (backslashes, any letter case, CP949, relative to the game folder or below `g_baseDir3`): reads find the file in the game folder or in a write location first; writes go to the write locations (Documents, Pictures, Application Support, by the first path component) and are refused in the game folder (`ERROR_ACCESS_DENIED`, `errno` `EACCES`).

- **Win32 file functions** (`CreateFile`, `FindFirstFile`, ...) call `bf_file` directly.
- **The C library's file functions** in game units go through `port/compat/include/bf_win32_crt.h`, which `bf_prefix.h` includes: `fopen`, `access`, `remove`, `rename`, `unlink`, `rmdir` are object-like macros for `bf_crt_*` here, which resolve the path with `bf_file_resolve` and then call the C library; `_access`, `_unlink`, `_rmdir` map onto them, `_chmod` to `bf_crt_chmod`; `_mkdir` and `_getcwd` are real functions here. Lua's `io.open`, `os.remove`, `os.rename` and `luaL_loadfile` are covered the same way.
- **Port code** keeps the C library's names: `bf_compat_flags` (root `CMakeLists.txt`) defines `BF_COMPAT_ADAPTER`, which turns the macros off; native modules do not see the prefix at all.
- `GetCurrentDirectory` (the client's `g_baseDir3`) and `_getcwd` return the game folder's POSIX path; the process's working folder is not changed and nothing depends on it.

The macros rename the name everywhere in a game unit, the standard headers included, so `std::remove` (the algorithm) and `std::list::remove` keep working (checked by the test, and the compile census of `macos-port`'s `src/` has no new error: 2841 -> 2829, the 12 fewer are `_unlink`/`_rmdir`/`_chmod`/`_S_IWRITE` now declared). Not covered: iostreams (`basic_filebuf<char>` lives in libc++.dylib and calls the real `fopen`; the client uses none), `tmpfile`, `freopen`, `stat`, `open` (the client calls none of them on game paths).

## Names

Kernel objects (files, mappings, finds, threads) are registered handles: a handle the client never got (`FindClose(INVALID_HANDLE_VALUE)`, `RegCloseKey` of a key whose open failed, `WaitForSingleObject(NULL)`) fails with `ERROR_INVALID_HANDLE` instead of crashing. `GetLastError` is per thread. `CP_ACP` is `bf_file_user_codepage()`, the code page the client sets with `bf_file_set_user_codepage` per service (949 by default, 1252); it also decodes registry names and `REG_SZ` data and message box text.

| Name | Maps to | Notes |
|---|---|---|
| `CreateFileA` | `bf_file_open` | dispositions 1-5 to `BF_FILE_CREATE/TRUNCATE/EXCLUSIVE`; `ERROR_ALREADY_EXISTS` after `CREATE_ALWAYS`/`OPEN_ALWAYS` on an existing file; `ERROR_FILE_EXISTS` for `CREATE_NEW` |
| `ReadFile`, `WriteFile` | `bf_file_read`, `bf_file_write` | no `OVERLAPPED` (stub) |
| `SetFilePointer`, `GetFileSize`, `SetEndOfFile`, `FlushFileBuffers` | `bf_file_seek/tell/size`, `bf_file_truncate_here` | 64-bit with the high part; `ERROR_NEGATIVE_SEEK` |
| `CloseHandle` | the object's release (`bf_file_close`, mapping, `bf_sys_thread_release`) | pseudo-handles ignored |
| `DeleteFileA`, `MoveFileA`, `CreateDirectoryA`, `GetFileAttributesA` | `bf_file_remove`, `bf_file_rename`, `bf_file_make_folder`, `bf_file_stat` | not called by the client today |
| `FindFirstFileA`, `FindNextFileA`, `FindClose` | `bf_file_find_*` | `*`/`?` without regard to case; `.` and `..` not listed; game files `FILE_ATTRIBUTE_READONLY` |
| `CreateFileMappingA`, `MapViewOfFile`, `UnmapViewOfFile` | `bf_file_map_open_file` | the whole file mapped read-only once; a view is `data + offset` (offset a multiple of 64 KiB, else `ERROR_MAPPED_ALIGNMENT`); the mapping lives until its handle is closed and its last view unmapped. `PAGE_READWRITE` and paging-file sections: stub |
| `GetSystemInfo` | `getpagesize`, `sysconf` | granularity 64 KiB (GPak aligns its views to it) |
| `VirtualAlloc`, `VirtualFree` | `calloc`, `free` | committed memory is zero (the client reads 8 padding bytes); reservations at an address: stub |
| `GlobalAlloc/Lock/Unlock/Free/Size`, `LocalAlloc/Lock/Free` | `malloc` with a size header | the handle is the memory (as `GMEM_FIXED`); `GlobalSize` for the clipboard adapter |
| `GetProcessHeap`, `HeapCompact`, `HeapAlloc`, `HeapFree` | `malloc`, `free` | |
| `GlobalMemoryStatus` | `sysctl hw.memsize` | |
| `GetTickCount`, `timeGetTime` | `bf_clock_ms` | |
| `QueryPerformanceFrequency`, `QueryPerformanceCounter` | 1 MHz, `bf_clock_us` | |
| `Sleep` | `bf_clock_sleep_ms` | |
| `GetLocalTime`, `GetSystemTime` | `bf_clock_local_time`, `bf_clock_utc_time` | |
| `FileTimeToSystemTime` | `gmtime_r` | |
| `timeGetDevCaps`, `timeBeginPeriod`, `timeEndPeriod` | 1 ms .. 1000 s; no-ops | |
| `CreateThread`, `_beginthreadex` | `bf_sys_thread_create` | ids from one counter, reported before the thread runs and equal to its `GetCurrentThreadId`; `CREATE_SUSPENDED`: stub (starts at once) |
| `WaitForSingleObject`, `GetExitCodeThread` | `bf_sys_thread_wait` | threads only (the client waits on nothing else) |
| `GetCurrentThread`, `GetCurrentThreadId`, `SetThreadPriority` | pseudo-handle, counter, `bf_sys_thread_set_priority` | priority of the calling thread only |
| `InitializeCriticalSection[AndSpinCount]`, `Enter/TryEnter/Leave/DeleteCriticalSection` | `bf_sys_mutex_*` | the mutex pointer lives in `LockSemaphore`; created on first entry if never initialised |
| `Interlocked*` | the Wine headers' inline functions | compiler atomics on the `LONG` (32-bit) |
| `GetLastError`, `SetLastError` | thread-local | `errno` translated (`ENOENT` -> 2, `EACCES` -> 5, ...) |
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
| `fopen`, `access`, `remove`, `rename`, `unlink`, `rmdir`, `_access`, `_unlink`, `_rmdir`, `_chmod`, `_mkdir` | `bf_file_resolve` + C library, `bf_file_stat`, `bf_file_remove`, `bf_file_rename`, `bf_file_remove_folder`, `bf_file_make_folder` | text mode `t` dropped; `_mkdir` of an existing folder `-1`/`EEXIST` |

Called by the client and deliberately failing (`BF_STUB()` or a trace line, with the value the callers handle):

| Name | Returns | Caller |
|---|---|---|
| `LoadLibraryA`, `GetProcAddress`, `FreeLibrary` | `NULL`/`ERROR_MOD_NOT_FOUND`, `NULL`, `TRUE` | base/FindHack.cpp (GameGuard, keyboard protection) |
| `CreateProcessA` | `FALSE`, `ERROR_NOT_SUPPORTED`, zeroed `PROCESS_INFORMATION` | WinMain's patcher restart |
| `OpenProcess` | `NULL`, `ERROR_ACCESS_DENIED` | base/FindHack.cpp |
| `CryptAcquireContextA` and `CryptCreateHash`, `CryptHashData`, `CryptDeriveKey`, `CryptDecrypt` | `FALSE` (`CryptRelease*`, `CryptDestroy*`: `TRUE`) | base/FindHack.cpp |
| `SetUnhandledExceptionFilter` | the previous filter; never called | WinMain (the port's signal handler writes the backtrace) |

Not implemented because the client does not call them: events, `ExitThread`/`_endthreadex`, `TerminateThread`, `SetCurrentDirectory`, `CopyFile`, `RegEnumValue`, `_findfirst` (declared by `io.h`).

Names the client calls that belong to no area yet: `wsprintfA` (user32, `Mode.cpp`, `window/UIFrameWnd.cpp`, `base/FindHack.cpp`) and `CoInitialize`/`CoUninitialize` (ole32, `WinMain.cpp`).

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
- `port/compat/include/bf_win32_crt.h` (new) and its `#include` in `bf_prefix.h`.
- Root `CMakeLists.txt`: `bf_compat_flags` defines `BF_COMPAT_ADAPTER`.
