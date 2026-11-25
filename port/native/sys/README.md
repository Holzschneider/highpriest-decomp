# sys: clock, threads, files, settings, Keychain, shell

The client's operating system, designed for macOS with no Windows types (steps M3.1, M3.3, M3.4). The adapters of M3.1a give these functions the Win32 names the client calls; in M11 the client calls them directly.

| Header | Offers |
|---|---|
| `include/bf_sys.h` | threads, recursive mutex, Keychain, alerts, opening URLs and files; includes the three below |
| `include/bf_clock.h` | the one clock (`mach_absolute_time`), sleeps, a replaceable clock source, the calendar |
| `include/bf_file.h` | the read-only game folder, the write locations, client-path translation, files, C streams (`bf_file_fopen`), `mmap`, folder listing |
| `include/bf_settings.h` | the one settings object, its defaults and its property list |

## Build and test

```
cmake -S port/native/sys -B build/native/sys -G Ninja
cmake --build build/native/sys
build/native/sys/bifrost-sys-test
```

With sanitizers: add `-DBF_SYS_SANITIZE=ON` (address and undefined behaviour) and use another build folder. The test writes only below its build folder (`sys-test/`, with `BF_HOME` pointed at `sys-test/home`). Checks on the real game folder read `BF_CLIENT_DIR` (default `~/Games/<name>`) and are skipped when it is absent. The Keychain check is skipped when the login keychain is locked or the session has no window server (an SSH or background session); it then needs a run from a logged-in desktop session.

## Environment

| Variable | Effect |
|---|---|
| `BF_CLIENT_DIR` | The game folder. Wins over the setting `Paths/GameFolder`; `bf_file_set_game_folder` wins over both. Default `~/Games/<name>` |
| `BF_HOME` | Replaces the home folder for the write locations and the settings file (tests). The default game folder still uses the real home |
| `BF_NO_ALERTS` | Alerts are logged as a `BFT_ERR` line and answered with their first button. Also the behaviour without a window-server session or off the main thread |
| `BF_NO_OPEN` | `bf_sys_open_url` and `bf_sys_open_path` log instead of opening |

`<name>` is `BF_APP_NAME` from `port/name.cmake`, passed as a compile definition to the library.

## Client paths

The client's paths are bytes: backslashes, any letter case, Korean names in CP949, relative to its current folder (which is the game folder) or below `g_baseDir3`, which the client sets to the drive letter `C:` (not the folder's path, so that a folder path of any length up to `PATH_MAX` stays out of the client's fixed buffers). `bf_file_resolve` and every `bf_file_*` function translate them: both separators split, `.`/`..` are applied (never above the root), a drive letter or leading separator means "relative to the game folder", an absolute path inside the game folder is made relative to it, and each component is found without regard to case or Unicode normalisation (NFC and NFD both match) through a per-folder listing cache that is rescanned when the folder's modification time changes. Resolved paths carry the names as they are on disk.

Names are decoded as CP949 under the game folder, and as the user code page (`bf_file_set_user_codepage`: 949 by default, 1252 for the English service) under the write locations, where names come from chat titles and character names. An absolute path under a write location is taken as UTF-8, because native code built it.

## Where the client writes

The game folder is never written. A write is sent by its first path component to a write location; any other write fails with `EACCES` and a `BFT_ERR` line (`file: refused to write into the game folder: ...`), counted as `file.write_refused`. Reading one of these paths looks in the write location first, then in the game folder.

| Client path (first component) | Written by | Goes to |
|---|---|---|
| `ScreenShot\screen*.jpg/.bmp` | `CMode::ScreenShot` (Mode.cpp) | `~/Pictures/<name>/` (the `ScreenShot` folder itself is dropped) |
| `Chat\*.txt` | chat saves (UIFrameWnd*.cpp), GM autosave `CGameMode::WriteChat` | `~/Documents/<name>/Chat/` |
| `Chat_BM\*.txt` | battle-message saves (UIFrameWnd3.cpp) | `~/Documents/<name>/Chat_BM/` |
| `Whisper\<name>.txt` | whisper window save (UIFrameWnd2.cpp) | `~/Documents/<name>/Whisper/` |
| `Report\<char>.txt` | chat reports (UIFrameWnd2.cpp, window/UIFrameWnd.cpp) | `~/Documents/<name>/Report/` |
| `emblem\*.bmp` | read only: the player's own guild emblems (UIGuildWnd.cpp) | `~/Documents/<name>/emblem/` |
| `PingLog.txt` | LoginMode.cpp, GameModePacket.cpp | `~/Documents/<name>/` |
| `TraceAI.txt` | AI trace (GameMode.cpp) | `~/Documents/<name>/` |
| `ProhibitionLog.txt` | chat-ban log (Session2.cpp) | `~/Documents/<name>/` |
| `ProhibitionLog.gd` | chat-ban store (Session2.cpp) | `~/Library/Application Support/<name>/` |
| `_tmpEmblem\*.ebm` | emblem cache (Session.cpp) | `~/Library/Application Support/<name>/_tmpEmblem/` |
| `_tmpskin_\<skin>` | unpacked UI skins (skin.cpp) | `~/Library/Application Support/<name>/_tmpskin_/` |
| `notice_patch.txt`, `notice_check.txt`, `notice_read.txt` | notice window (window/UIFrameWnd.cpp) | `~/Library/Application Support/<name>/` |
| `savedata\...` | reserved for the client's own save files | `~/Library/Application Support/<name>/savedata/` |

Settings and UI window positions (the registry) go to `~/Library/Application Support/<name>/Settings.plist`; the saved password to the Keychain. The single-player save files (`SaveData*.KSY`, HP_SaveLoad.cpp) leave with M1 and are not mapped. Lua's `io.open` in AI scripts calls the C library directly; if it should write, the `fopen` adapter has to resolve through `bf_file_resolve`.

## Settings

One object, `bf_settings_shared()`, loaded on first use from `Settings.plist` in Application Support. Built-in defaults lie underneath and are never written. The shape:

```
<dict>
  FormatVersion  integer 1
  Options        dict  SkipIntroScreens 1, FreeCamera 1, LaunchArguments 1, AttackWithoutCtrl 0
  Account        dict  Name ""
  Paths          dict  GameFolder (absent by default)
  Client         dict  the client's registry root HKLM\<g_regPath>:
                       WIDTH 1024, HEIGHT 768, BITPERPIXEL 32, ISFULLSCREENMODE 0, SOUNDMODE 1,
                       DIGITALRATETYPE 0, DIGITALBITSTYPE 0, NUMSAMPLETYPE 1, SPRITEMODE 2,
                       TEXTUREMODE 2, ISLIGHTMAP 1, FOG 1, PROVIDERNAME, GUIDDEVICE/GUIDDRIVER (16 zero bytes), ...
                       Option       dict  isEffectOn 1, bgmIsPaused 0, isSoundOn 1, streamVolume 100, soundVolume 100
                       UIRectInfo   dict  window positions (352 values) as the client writes them
                       ShortcutItem, BookMark/<server>/<char>, NameBaloon/<server>, Option/ChatOption ...
  Registry       dict  any other key: Registry/<HKLM|HKCU>/<subkey path>
</dict>
```

A group is a dictionary; inside it a dictionary is a subgroup and anything else a value: integer (REG_DWORD as signed 32 bits), string (REG_SZ, UTF-8), data (REG_BINARY). Names are UTF-8 and compared without regard to case, as in the registry. Because `Client` and `Client/Option` exist in the defaults, the client's `RegOpenKeyEx` on them succeeds before anything was written, so `ReadRegistry` returns 1 and `Setup_HP.exe` is never wanted.

**For the registry adapter (M3.1a; since M11.4 the client's own `port/client/include/bf_setting_key.h`, which keeps these rules):** map `HKLM\<g_regPath>[\sub\key]` to group `Client[/sub/key]` and every other key to `Registry/<root>/<path>`; `RegOpenKeyEx` = `bf_settings_has_group`, `RegCreateKeyEx` = `bf_settings_create_group`, `RegQueryValueEx` = `bf_settings_type` plus the typed getter (a too-small buffer returns -1 with the length, i.e. `ERROR_MORE_DATA`), `RegSetValueEx` = the typed setter, `RegDeleteKey`/`RegDeleteValue` = `bf_settings_remove`, `RegEnumValue` = `bf_settings_value_at`, `RegCloseKey` = `bf_settings_save`. Convert CP949/CP1252 names and REG_SZ data to UTF-8 and back, and treat REG_SZ lengths as bytes including the NUL. The root values `ID` and `PASSWORD` of the login window are better mapped to `Account/Name` and the Keychain (`bf_sys_keychain_*`, account = the account name) than stored as plain values.

## Notes for the adapters

- M11.3: the client's own file code calls these functions directly: `CFileMgr`/`CFile` (`base/File.cpp`: `bf_file_open/read/write/seek`, `bf_file_stat`; archive and folder data in `calloc` blocks with 8 zero bytes after archive data, freed with `free`), `CMemMapFile` (`base/GPak.cpp`: `bf_file_map_open_file`, a view is `data + offset`; an empty file counts as not mapped), `GetFileNames`/`DeleteDirectory` (`base/Util.cpp`) and `CSkinMgr` (`skin.cpp`: `bf_file_find_*`, `bf_file_stat` for `access`, `bf_file_make_folder` for `_mkdir`, `bf_file_remove(_folder)`). `bf_file_fopen` is the C library's `fopen` on a client path (was the win32 adapter's `bf_crt_fopen`).
- `GetCurrentDirectory`/`GetModuleFileName` should return the game folder's POSIX path (`bf_file_location(BF_LOCATION_GAME)`); the client appends `\ScreenShot\...` to it, which resolves correctly.
- `CreateFileMapping` + `MapViewOfFile(offset, size)`: map the whole file once with `bf_file_map_open_file` and return `data + offset`; `UnmapViewOfFile` of a view is then a no-op until the mapping handle closes.
- `CRITICAL_SECTION`: store the `bf_sys_mutex *` in the structure (Wine's is 40 bytes); `InitializeCriticalSection` creates it.
- `Interlocked*`: compiler atomics on the `LONG`; this module has none.
- `CreateThread`/`_beginthreadex`: the thread function returns `uint32_t`; the handle is a `bf_sys_thread *`; `WaitForSingleObject` = `bf_sys_thread_wait`, `CloseHandle` = `bf_sys_thread_release`. There is no `TerminateThread`.
- `timeGetTime`/`GetTickCount` = `bf_clock_ticks` (`bf_clock_ms` plus an hour), `QueryPerformanceCounter` = `bf_clock_us` at 1 MHz, `Sleep` = `bf_clock_sleep_ms`, `GetLocalTime`/`GetSystemTime` = `bf_clock_local_time`/`bf_clock_utc_time`.
- The first call into `bf_file` or `bf_settings` should happen on one thread at start-up (the game folder may be read from the settings, and the settings file's place from the file module).

## Untested here

- The Keychain round trip: this run's session is a background session whose login keychain is locked (`errSecInteractionNotAllowed`), so the check is skipped. `bf_sys_keychain_available` reports it.
- A shown `NSAlert` and an actual `NSWorkspace` open: the test exercises only the logging paths, so it never blocks or opens a browser.
- A case-sensitive volume: on the default (case-insensitive) APFS the folded listing still decides every lookup, and enumeration is checked with other case and NFD names, but no case-sensitive volume was tried.
