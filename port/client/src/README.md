# Replacements for units out of the build, and the entry point

Port code that is compiled into the client with the client's own flags (`bf_client_flags`; listed in `BF_CLIENT_PORT_SOURCES` in the root `CMakeLists.txt`) because it stands in for a unit of `sources.mk` that the port does not build (`BF_CLIENT_EXCLUDE`, implementation plan M2.5).

| File | Replaces | What it does |
|---|---|---|
| `bf_client_state.cpp` | (new) | The client's state for the control socket: `bf_port_mode` maps the mode manager and the login sub-mode to PROTOCOL.md's `mode` (`login`, `connecting`, `servers`, `charselect`, `makechar`, `loading`, `game`, `quitting`) and the client's own name (`LSM_IDENTRY`, `MT_GAME`); game mode reports `loading` until `OnRun` (`g_bfModeRunning`), i.e. while `OnInit` loads the map (M6.4). `bf_client_ui_windows` (contract `bf_client_ui.h`, plain C) describes the windows shown, front to back, for the state part `windows` (M5.6): id and name from what `UIWindowMgr::MakeWindow` recorded (`bf_client_window_made`; a window made elsewhere, such as a message box, has id -1 and its class name without `UI`, upper case: `MESSAGEBOX`), rect, focus, and the `controls` below it found by walking the children: `edit` (masked fields as `*`), `button` (named after its control id: `ok`, `cancel`, `make`, `delete`, `str`, ..., the `ID_*` names in lower case), `check` (`on`/`off`), `list` (the selected item), `item` (slots), `text` (static texts); unnamed ones are `<kind>:<n>`. Per window: `LOGINWND` `id_edit`, `pw_edit`, `ok` (connect), `save`; `SELECTCHARWND` `slot:<n>` (n the character slot, text the name, focus on the selected one), `name`, `job`, `level`, `exp`, `hp`, `sp`, `str` ... `luk`; `MAKECHARWND` `name_edit`, `str` ... `luk` (the values), and the buttons `ok`, `cancel`, `left`/`right` (hair style), `up` (hair colour), `str` ... `luk` (stat arrows; button and value share the id, the button comes first). It reads the classes' own members as `BfUiProbe`, a friend declared under `BF_PORT` in `UISys.h` and `UIFrameWnd.h`. |
| | (M8.1e) | `BfUiProbe::Npc`, the NPC, deal and storage windows (PROTOCOL.md, `controls` of the NPC, deal and storage windows): `SAYDIALOGWND` reports its text viewer as `text` and only the buttons in `m_buttonList` (`UM_DELETE_BUTTON` moves a used button to `(m_w, m_h)` and keeps it as a child), named `next` or `close` by `m_target`; `CHOOSEWND` its entries as `choice:<entry>` at the rows `UIListBox::HitTest` uses; `NPCEDITDIALOGWND`/`NPCTEXTEDITDIALOGWND` and `ITEMDROPCNTWND` their field as `edit`; the deal and storage windows (every `UIItemBaseWnd` but `UIItemWnd`, which `Items` reports with inventory indexes) their items as `item:<item id>` in the cells of `UIItemBaseWnd::GetItemInfo`'s walk, and the storage's `UITab` as `tab:item`/`tab:equip`/`tab:etc`, as the item window's. `BfUiProbe` is a friend of `UISayDialogWnd`, `UIChooseWnd`, `UINpcEditDialogWnd`, `UIItemBaseWnd` (`UIFrameWnd.h`) and `UIListBox` (`UISys.h`) under `BF_PORT`. |
| `bf_client_game.cpp` | (new; M6.4) | The game mode for the control socket (contract `bf_client_game.h`, plain C): the map (`m_rswName` without extension, the size of `C3dAttr`), the own character (`CSession`: ids, name, job, levels, HP/SP, zeny, weight / 10; `CPlayer`: cell through `CGameMode::GetServerCoor`, `DegToDir(m_roty)`, `m_stateId == ST_MOVE`, `m_moveDestX/Y`), the actors (`CWorld::m_actorList` without the player, `m_itemList`, `m_skillList`; names read from `CGameMode::m_actorNameList` as `BfClientGame`, a friend declared under `BF_PORT` in `GameMode.h`, so nothing is requested; item ids from `CItem::m_bfItemId`, recorded under `BF_PORT` in `Item.cpp`; screen positions projected with the view matrix) and the pointer's cell (`CGameMode::GetGroundAttrPos`). Ready only while game mode runs on a loaded map. Chat lines: `UIWindowMgr::SendMsg` (under `BF_PORT`) passes every `UIM_PUSHINTOCHATHISTORY(2)` to `bf_client_chat_line`, which maps the client's chat type (1 normal, 2 whisper, 3 party, 4 guild, 13 or a line pushed into both lists: broadcast; else system) and splits `name : text`. |
| `bf_control_game.cpp` | (new; M6.4) | Port side of the above (in `bf_port_main`): registers the state parts `map`, `self`, `actors`, `pointer` and the events `map`, `chat`, writes them as PROTOCOL.md says, and sends the events from `bf_port_frame` before `bf_control_poll`. Also the Windows-1252 to UTF-8 writer that `windows` uses. |
| `bf_control_sound.cpp` | (new; M7 exit) | The state part `sound` and the event `sound` (in `bf_port_main`): the counters of `port/native/audio` and its ring of recent voices, which `PlayWave` (`src/resource/Wave.cpp`) tags with the file name and distance; one event per voice, sent from `bf_port_frame` before `bf_control_poll`. File names are written as UTF-8 from code page 949. |
| `bf_control_movie.cpp` | (new; M12.4a) | The state part `movie` and the event `movie` (`start`, `end`) in `bf_port_main`, from the movie player's state (`bf_movie_get_state`): the movie CBink plays, its frame, sound and whether it ended or a key stopped it. Events sent from `bf_port_frame` before `bf_control_poll`; CBink's loop calls `bf_port_frame` while the movie plays. |
| `bf_movie.cpp` | (new; M11.10, was the Bink adapter `port/compat/bink` of M12.4a) | The intro movie's player that `src/movie/CBink.cpp` calls (contract `port/client/include/bf_movie.h`, in `bf_port_main`), on `port/native/formats/bink`, `port/native/audio` and the sys clock and file lookup. `bf_movie_open` looks a name up in the game folder (an absolute path as it is), `bf_movie_open_memory` takes a movie from the archives (`BF_INTRO_MOVIE`); absent or not Bink 1: NULL, `bink: openning.bik not found in the game folder` (`gfx`), counter `bink.missing`. Frame n is due n-1 periods after frame 1 was decoded (`bf_movie_wait` on `bf_clock_us`, nothing skipped); `bf_movie_next` wraps to frame 1 after the last, as the SDK; `bf_movie_goto` clamps and moves the clock and the sound (`Break('g')` goes to 1500, the last frame); `bf_movie_copy` converts to 32-bit BGRA, 5:6:5 or 1:5:5:5, the width from the pitch. Sound (asked for by CBink's `bf_movie_use_sound(1)`, the SDK's DirectSound): the first track decoded at open from a second reader handle without picture, cut to the picture's length, played as one voice tagged with the movie and started with frame 1; pause and goto restart the rest from the movie's position; it starts the sound engine when the client has not yet. Trace `bink: open ...`/`bink: close <name> after frame n of N (ended|stopped)`. Test: `bifrost-movie-test` (`port/client/test/movie_test.cpp`, ctest `movie`, headless; own clock, sound engine offline; the public sample `test/bink/samples/original.bik` under the game folder, else `skipped`). |
| `bf_replay.cpp` | (new; M6.5) | Packet replay. With `BF_PKT_REPLAY=<log>` the client's connections open no socket: `device/Connection.cpp` (under `BF_PORT`) asks it at `Connect`, `Poll`, `SendPacket` and `Disconnect`, and it hands over the `R` bytes of a log that `BF_PKT_LOG` recorded. The client's n-th connect is the log's n-th connection (seventh field `account/1`, `char/2`, `zone/3`). Pacing by the client's sends: the received bytes up to the next `S` line are delivered, that packet releases the rest; a sent packet is matched by header among the pending `S` lines up to the next `R` (keep-alives the client did not send are passed over), a mismatch is a trace line and never blocks; an `S` line not sent within `BF_PKT_REPLAY_WAIT` ms (5000; 0 waits for ever) is passed over. Lengths are compared, bytes with `BF_PKT_REPLAY_BYTES=1`. The end of the log's last connection is a server-side close `BF_PKT_REPLAY_HOLD` ms later (5000; -1 never). Trace lines `net` "replay: ...", counters `replay.*`. Also linked into `bifrost-netprobe` and `bifrost-bot`, which share `Connection.cpp`. Contract: `port/client/include/bf_replay.h`; test: `port/harness/scripts/replay.py`. |
| `bf_crash.cpp` | `src/ExceptionHandler.cpp` (crash dialog, minidump) | A static constructor installs a handler for SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT and SIGTRAP on an alternate stack. On a crash it writes the signal, the faulting address, pc, lr and a backtrace as `crash:` lines through the trace (category `err`, always on), dumps the counters, then re-raises with the default action so the system's crash report still sees the fault. `BF_NO_CRASH_HANDLER=1` turns it off. It also defines `ExceptionFilter`, the one name of the old unit that `WinMain` uses (`SetUnhandledExceptionFilter`, which the Win32 adapter provides as a stub). |

The other removals of M2.5 are inside `src/`, under `BF_PORT`, because that was the smaller change:

- **GameGuard**: `src/base/FindHack.cpp` makes its four entry points (`SetHwnd`, `SendID`, `ExitFindHack`, `Auth2`) empty and leaves out `FindHack1`, `FindHack2`, `ProcessFindHack` and the monitor callback, which nothing calls. `CNPGameLib`'s inline wrappers are no longer used, so no GameGuard function is referenced.
- **Bink** (M2.5, replaced in M12.4a, native in M11.10): `src/movie/CBink.cpp` plays through the movie player `bf_movie.cpp` (`bf_movie.h` instead of the SDK headers, which are deleted); under `BF_PORT` the option `SkipIntroScreens` (trace `intro: skipped (option SkipIntroScreens)`) and then, as in 2008, the registry's `BINKMODE` 1 skip the movie at the start, `BF_INTRO_MOVIE` names another movie in `InitOpenning` (an absolute path, or a name the file manager finds: `data\video\ad\01.bik`), `bf_port_frame` runs in its loop and `g_sysQuit` stops it, the back buffer is locked write-only, no pause in the background (spec section 3); trace `intro: start, N frames`, `intro: end at frame n of N (ended|stopped)`, `intro: no movie (...)`. The copy is BGRA8888, the render target's format (what `BinkDDSurfaceType` gave for it), through a write-only lock of the target (`bf_render_lock`); `bf_render_present_part` stretches the movie's 512x347 rectangle over the window, as DirectDraw's `Blt` to the primary did (M11.9). The login window's intro button is offered only when `..\openning.bik` exists and only for the services 0, 3, 10, 11, 13 and 14 (as in the 2008 executable), so it never shows with the port's data. `WinMain` calls `g_Bink.OpenningMain(1)` before the login mode, as the 2008 executable does.
- **Granny** (M2.5 to M12.3a): `src/resource/Model.cpp` stubbed `C3dGrannyModelRes` and `C3dGrannyBoneRes` until M12.3a; they are now built as the 2008 client had them, since M11.10 directly on the reader and its runtime ([`port/native/formats/granny`](../../native/formats/granny/README.md); the granny2 adapter of M12.3a is deleted). `CGameMode::NewNpc` makes a Granny actor for a `.gr2` job name whose model loads, and a `CNpc` (the placeholder) for the two whose file is not in the archives.

## The entry point and the start-up glue (M3.2, M3.7)

The library `bf_port_main` (root `CMakeLists.txt`) is port code, compiled with `bf_compat_flags` and the native modules' headers, and linked into the client; the client's own sources reach it through `port/client/include/bf_port.h` under `BF_PORT`.

| File | What it does |
|---|---|
| `bf_main.cpp` | `main`. Takes the port's flags out of the command line (`--headless`, `--control <path>` / `--control=<path>`, the launch arguments `--server`, `--account`, `--password`, `--character` (also `--flag=value`; the environment's `BF_SERVER`, `BF_ACCOUNT`, `BF_PASSWORD`, `BF_CHARACTER` when a flag is absent), Finder's `-psn_`), joins the rest with spaces as `lpCmdLine` and calls the client's `WinMain`. Before that, unless headless: `bf_app_init` (single instance "main"; a second launch exits 0), `bf_app_set_clock` (the sys clock in ms), `bf_prefs_choose_game_folder` (0: quit, -1: exit 1), `bf_prefs_install_menu_item`, `bf_input_set_scale_setting(Display/UIScale, default 0 = auto)`. Opens the control socket when asked (`bf_control_path_from_args`): hooks `quit` (`bf_input_post(WM_CLOSE)` to the window, as Cmd+Q), `capture` (`bf_gfx_capture_png`), the pointer mapping through `bf_view`; state parts `mode`, `detail` and `windows`; the events `mode` and `window` (`open`/`close`, from comparing the windows of `bf_client_ui_windows` once per frame). `bf_port_launch(key)` hands the launch arguments to the login mode, NULL when the option `LaunchArguments` is off. Defines the `bf_port_*` functions of `bf_port.h`. |
| | `bf_port_start_graphics`: called by `WinMain` right after `InitApp` (`bf_input_window_create` has made the window, its layer and the view): creates the gfx device on `bf_app_window_metal_layer()` at the view's logical size and UI scale, attaches it to the view and hands it to the render target (`bf_render_set_device`), before `SetupDriver2`. At exit the render target lets it go (destroying the client's textures still alive) before `bf_gfx_destroy`. |
| | `bf_port_frame`: called at the top of `CheckSystemMessage`, i.e. once per frame of both modes. When the gfx device has presented a new frame it calls `bf_control_frame_done` (a capture reads the target, which still holds that frame); then, socket or not, `bf_view_sync()` (the window's size and the UI scale, M4.3a); then it reports a `mode` change as an event and calls `bf_control_poll`. |
| | The client's resize path (M4.3a): at the first frame after the client has its renderer, the view's listener and modal frame are installed. The listener calls `bf_client_view_changed(w, h)` (`WinMain.cpp`): `C3dDevice::BfResize` makes the back buffer, z-buffer and backup surface anew at the logical size and sets the viewport, `CRenderer::SetSize` (projection and picking) and the guard bands follow, and `UIWindowMgr::BfOnResize` sets the bounds and puts the windows back: the login mode's windows move with the placement `MakeWindow` gives them at that size, others keep the edge they touch, else their offset from the centre (x) and their share of the height (y), and all are clamped into view. The modal frame (`bf_client_modal_frame`) runs the current mode's `OnUpdate` once without the message pump while AppKit holds the event loop (live resize, an open menu); only while a mode is in `OnRun`. |
| | Control: `resize` (scale setting first, then the content size in pixels, then a sync; `bad_state` for a size in fullscreen; `resize_busy` while a fullscreen change asked for is under way) and the state part `view`. |
| | The end of the process: an `atexit` handler registered first in `main` saves the settings, writes the counters (when any trace category beyond `err`/`stub` is on), flushes and calls `_exit`. The client's static destructors (`CFileMgr`, `CSkinMgr`) would otherwise run after the port's own statics, in no defined order across units, and call into the file tables after they are gone; they release only what the system reclaims. An `exit(n)` elsewhere therefore ends with status 0. |
| `bf_input.cpp` | The client's window, event pump, keyboard, mouse, cursor and clipboard (`include/bf_input.h`, M11.7), on `port/native/app` and `port/native/view`, with the rules of the deleted input adapter (`port/compat/input`, M5.1, M5.2); see "bf_input" below. Test: `bifrost-input-test` (`port/client/test/input_test.cpp`, label `window`). |
| `bf_zlib.cpp` | `bf_zlib_uncompress`/`bf_zlib_compress` on `bf_codec_inflate`/`deflate`: `common.h` maps the client's `uncompress`/`compress` to them, because its declarations take a 32-bit `DWORD *` length and the system zlib's `uLongf` is 64 bits. |
| `bf_crt.cpp` | The Visual C++ runtime names macOS lacks, as real functions: `itoa`, `_itoa`, `_ltoa` (radix 2–36, sign only in radix 10), `strlwr`, `_strlwr`, `strupr`, `_strupr` (ASCII only, as the "C" locale), `_lrotl`, `_lrotr`. `spawnl` is a stub (`BF_STUB`, returns -1, `ENOENT`): its caller starts `findhack.exe` (GameGuard; `skin.cpp`'s `runzip.exe` call is `!BF_PORT` since M11.3). Since M11.3 also the functions behind `bf_crt.h` (the C library's file names on client paths): `bf_crt_fopen` (= `bf_file_fopen`), `bf_crt_access`, `bf_crt_remove`, `bf_crt_rename`, `bf_crt_unlink`, `bf_crt_rmdir`, `bf_crt_chmod`, `_mkdir`, on `bf_file.h`; tested by `bifrost-compat-win32-test` (`test/client_calls.cpp`) until that test goes in M11.11. |
| `bf_setting_key.cpp` | `bf_setting_*` of `include/bf_setting_key.h` (M11.4): the client's registry paths below HKLM (its code page, `\` between parts) on `bf_settings`, with the rules of the deleted registry adapter so that existing `Settings.plist` files read the same: `Software\<name>[\...]` is group `Client[/...]`, any other path `Registry/HKLM/<path>`; names and strings converted between `bf_file_user_codepage()` and UTF-8; a 4-byte integer write an integer, a string write the text up to its NUL within the size, anything else data; a read copies the value as `RegQueryValueEx` did (an integer as 4 bytes, a string with its NUL) and fails when it does not fit; `ID` of the root is `Account/Name`, `PASSWORD` the Keychain item of that account. `bf_setting_close` saves when something changed. `bf_setting_get`/`bf_setting_set` take only 4-byte values (static_assert). Test: `bifrost-setting-key-test` (`port/client/test/setting_key_test.cpp`, headless). `port/tools/native_settings.py` rewrote the client's calls. |
| `bf_render.cpp` | The render target (`include/bf_render.h`, M11.9) on `port/native/gfx`, with the rules of the deleted DirectDraw/Direct3D adapter (`port/compat/d3d`): the one device; the client's textures registered (destroyed with the device when it goes first) and stage bindings that keep a texture its owner destroyed until the stage is rebound (Direct3D 7 held a reference; `CRenderer` caches the last texture by pointer); draws that hand `C3dDevice`'s state and the bindings to gfx; the lazy render-target lock (protected pages, read back on the first access, written back only after a write; the fault handler chains to the one it replaced); the part present (a rectangle stretched linearly over the target, from the lock buffer while it is current); the target's change counter. `bifrost-render-test` (headless, 29 checks) carries the adapter test's checks for these. `src/3dlib` holds the rest: `C3dDevice` translates the Direct3D 7 render and stage states by number into one `bf_gfx_state` (`Device.cpp`), `CTexture` holds a `bf_gfx_texture`, `CSurface` is the render target, an offscreen surface or a texture with the adapter's pixel rules and the staging-canvas DC (`Texture.cpp`). |
| `bf_port_shell.cpp` | `bf_port_alert` and `bf_port_open` of `bf_port.h` (M11.5, for `WinMain.cpp` and `window/UIFrameWnd.cpp`): the client's message boxes (its code page to UTF-8, a `BFT_UI` trace line, `bf_sys_alert`) and "open" (a URL with `://` or `mailto:` to `bf_sys_open_url`, else the resolved client path to `bf_sys_open_path`), as the MessageBox and ShellExecute adapters did. |

The client's call sites under `BF_PORT` (M5 added: `UIWindowMgr::MakeWindow`, which records each window's id; `LoginMode.cpp`, the launch arguments in `OnChangeState` and the zero-filled account and password buffers; `UILoginWnd`, "save" keeping the password through the registry's `PASSWORD`, i.e. the Keychain; `UISelectCharWnd::InitFromReg`, `CURSLOT` initialised): `WinMain.cpp` (`bf_port_start_graphics` after `InitApp`, `bf_port_frame` in `CheckSystemMessage`, the `--headless` stop), `common.h` (zlib), `resource/Bitmap.cpp` `LoadJPGData` and `3dlib/Renderer.cpp` `SaveJPG` (JPEG through `bf_codec_jpeg_*`; rows unpadded, as the client's own loops read and write them; the screenshot path resolved with `bf_file_resolve`), `3dlib/Device.cpp` (no `TestScreen`), `Mode.cpp` (`CMode::GetSubMode`, declared but never defined in the reconstruction; `g_bfModeRunning` around `OnRun`), `3dlib/Device.cpp` (`BfResize`), `3dlib/Renderer.cpp` (the UI pass samples texel-exact faces unfiltered, snapped to logical pixels), `UIWindowMgr.cpp` (`BfOnResize`).

## bf_input: window, pump, keys, mouse, clipboard (M11.7)

`src/` calls `port/client/include/bf_input.h` under `BF_PORT`: `WinMain.cpp`
(`InitApp`'s window, `CheckSystemMessage`'s pump, the window procedure's
cursor, default procedure and inactive `WM_MOUSEMOVE`, `ExitApp`),
`movie/CBink.cpp` (the movie's pump and its destination rectangle),
`device/Control.cpp` (`CMouse`), `framework/Language.cpp` (key state, the
clipboard), `UIWindowMgr.cpp`, `GameMode.cpp`, `Mode.cpp` (`CheckKeyPressed`).
The client's window procedure stays its own; the message ids, virtual keys
and their parameters are the Windows numbers, which the client keeps
(`include/bf_types.h`). These are the input adapter's rules, moved unchanged
(`bifrost-input-test` has its checks):

- **Start-up.** `main` has run `bf_app_init` (single instance "main"; a
  second launch brings the first to the front and exits 0, so `InitApp`'s
  `FindWindow` test is gone and `g_multiSTOP` stays 0) and set the UI scale
  setting. `bf_input_window_create(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME,
  WindowProc)` makes the window with that content size in points (title
  CP1252 to UTF-8), starts the view and sends `WM_NCCREATE` and `WM_CREATE`.
  macOS places the window; `Session.cpp` stores 0 for `window_left` and
  `window_top`, as before.
- **The pump.** `bf_input_peek` hands out pending messages first. When none
  are pending it takes **one** event out of the app queue and translates it,
  so the key and pointer state the client reads while it handles a message
  is the state as of that message. When the queue is empty it calls
  `bf_app_pump(0)` once and looks again; then it reports "no message" and
  pumps again on the next call. A `WM_KEYDOWN` it hands out puts the
  `WM_CHAR` 8, 9, 13 or 27 that `TranslateMessage` typed for Backspace,
  Tab, Return and Escape (not with Ctrl) right behind it. `bf_input_post`
  appends (the control socket's `quit`). AppKit's own modal loops (live
  resize, menus) run the view's modal frame (`bf_main.cpp`).

| App event | Messages |
|---|---|
| `KEY_DOWN` | `WM_KEYDOWN`, or `WM_SYSKEYDOWN` while Alt (Option) is held without Ctrl, for F10 and for Alt itself; `wParam` the VK (`VK_SHIFT`/`VK_CONTROL`/`VK_MENU` for either side); `lParam` = `1 \| scan << 16 \| extended << 24 \| alt << 29 \| previous << 30`, previous being the repeat flag or the key already held |
| `KEY_UP` | `WM_KEYUP`/`WM_SYSKEYUP` with `0xC0000000` added; only for keys whose down was sent |
| `TEXT` | one `WM_CHAR` per character, CP1252; characters CP1252 lacks are dropped; the whole text is dropped when its key-down had Ctrl, Cmd, or Option on a game shortcut |
| `POINTER_MOVE` | `WM_SETCURSOR` (`HTCLIENT`) when inside, then `WM_MOUSEMOVE` with the logical position |
| `POINTER_ENTER` | `WM_SETCURSOR` |
| `BUTTON_*`, `WHEEL` | none: `CMouse` reads them (`bf_input_mouse_read`) |
| `RESIZE`, `FULLSCREEN` | `bf_view_handle_event`, then `WM_SIZE` with the logical size |
| `FOCUS(1)` | `WM_ACTIVATEAPP(1)`, `WM_NCACTIVATE(1)`, `WM_ACTIVATE(WA_ACTIVE)`, `WM_SETFOCUS` |
| `FOCUS(0)` | held keys forgotten; `WM_NCACTIVATE(0)`, `WM_ACTIVATE(WA_INACTIVE)`, `WM_ACTIVATEAPP(0)`, `WM_KILLFOCUS` |
| `QUIT` (Cmd+Q, close button, Dock, logout) | `WM_CLOSE` |
| `VISIBILITY`, `REOPEN`, `USER` | none |

`WM_MOVE`, `WM_PAINT` and `WM_SYSCOMMAND` are never sent (the window
procedure's cases for them, and for the menu and size-move loops and the
IME, are `#ifndef BF_PORT` where they named Windows calls). The default
procedure (`bf_input_default_proc`) answers `TRUE` to `WM_NCCREATE` and
`WM_NCACTIVATE`, 0 to the rest.

- **Keys** (spec section 4). Letters, digits and punctuation are `'A'`…,
  `'0'`…, `VK_OEM_*` with set-1 scan codes (A 0x1E, D 0x20, W 0x11, X 0x2D);
  Return 0x1C, Tab 0x0F, Delete (⌫) `VK_BACK` 0x0E, Esc 0x01, Space 0x39;
  Forward Delete, Home, End, Page Up/Down and the arrows are extended;
  F1–F12 (F10 as `WM_SYSKEYDOWN`); keypad digits are `VK_NUMPAD*` (a Mac
  keypad has no Num Lock state), keypad Enter `VK_RETURN` extended, Clear
  `VK_NUMLOCK`; Help (the Insert of a PC keyboard) `VK_INSERT`; F13, F14,
  F15 (Print Screen, Scroll Lock, Pause of a PC keyboard) `VK_SNAPSHOT`,
  `VK_SCROLL`, `VK_PAUSE`, and `VK_SNAPSHOT` comes as `WM_KEYUP` only, as
  Windows sends it (the client takes the picture there); Cmd and Fn send no
  messages (`VK_LWIN`/`VK_RWIN` read Cmd).
- **Option is Alt.** Its key-down is `WM_SYSKEYDOWN`. The character macOS
  types for it is dropped when the key is a game shortcut
  (`bf_input_is_alt_shortcut`: the letters `UIWindowMgr::ProcessPushButton`
  tests with `VK_MENU` but F, K, N; the digits; Insert, Home, End, F10), and
  typed otherwise (Option+L still types `@` on a German layout). Ctrl drops
  the text as well.
- **Cmd keeps its Mac meaning**: Cmd+Q `WM_CLOSE`; Cmd+F
  `bf_view_toggle_fullscreen`; Cmd+V types the clipboard as `WM_CHAR`s into
  the focused field (CP1252, control characters dropped, no key message);
  Cmd+X, Cmd+C are `WM_KEYDOWN 'X'`/`'C'` while `VK_CONTROL` reads down,
  until the key or Cmd is let go (`CLanguage::OnKeyDown`'s cut and copy);
  **Cmd+I is Insert** (sit; Cmd+Option+I is Alt+Insert, the compass);
  **Cmd+Shift+S is Print Screen** (with Ctrl a BMP; Cmd+S alone does
  nothing); any other Cmd combination is the system's. AppKit's menu takes
  Cmd+Q and Cmd+F itself; these rules cover injected key-downs.
- **Key state.** `bf_input_key_state` is `GetKeyState`: 0x8000 while held,
  plus the Caps Lock latch and always 1 for Num Lock; `bf_input_async_key_state`
  is `GetAsyncKeyState`: 0x8000 or 0, never bit 0. `VK_LBUTTON`,
  `VK_RBUTTON`, `VK_MBUTTON` read the pointer's buttons.
- **The mouse** (`CMouse`): `bf_input_mouse_read(m_xPos, m_yPos, &state)`
  in place of `GetDeviceState`, with `DIMOUSESTATE`'s field names. `lX`/`lY`
  are the logical pixel under the pointer (`bf_view_px_to_logical_pixel`,
  clamped to the logical screen) minus the position the client holds, so
  the sprite is where the macOS pointer is. While the right button is held
  they are the device's own movement (`bf_app_pointer_take_delta`, divided
  by the UI scale, fractions carried over), so the camera turns on past the
  window's edge; let go, the next read brings the client back to the
  pointer. `lZ` is the wheel × 120, positive away from the user. Buttons:
  left, right, middle, X1; `m_bSwapButton` is 0 (macOS has swapped them
  already). Not acquired (focus lost: `SetAcquire`), a read fails as
  `DIERR_NOTACQUIRED` did; acquiring forgets the movement before. The stage A
  adapter guessed the client's position; the client now passes it.
- **The cursor.** `WM_SETCURSOR`'s `SetCursor(NULL)` and the fullscreen
  start's `ShowCursor(FALSE)` are `bf_input_hide_cursor`: the system pointer
  is hidden over the content area only. The pointer is never captured,
  confined or warped. `bf_input_cursor_pos` (inactive `WM_MOUSEMOVE`) is the
  logical position, not clamped. "Screen" coordinates are client ones:
  `bf_input_client_size` is the logical size.
- **Clipboard.** `bf_input_clipboard_text` (Shift+Insert, Ctrl+V): the Mac
  clipboard in CP1252, characters it lacks dropped, lines ending CR LF, NULL
  for nothing. `bf_input_clipboard_set_text` (Ctrl+C, Ctrl+X, Shift+Delete):
  CP1252 to UTF-8, CR LF as LF.
- **No IME.** macOS input methods compose and commit text, which arrives as
  `WM_CHAR` in CP1252 (Hangul is dropped). `CLanguage` behaves as with no
  input method context: no composition, no candidates (`GetImeCompStr` and
  the others empty the composition; `GetCandidateString` is ""); the
  keyboard language is English (US), `MAKELANGID(LANG_ENGLISH,
  SUBLANG_ENGLISH_US)`.

`3dlib/Device.cpp` takes its client area from `bf_input_client_size` (M11.9;
`ClientToScreen` was the identity, the full-screen window calls changed
nothing). `UIControl.cpp`'s Shift+Enter test reads
`bf_input_key_state(VK_SHIFT)` too: one key state for the client.

Untested: real keyboards and pointers (the unattended Mac's screen is
locked; everything is injected), Korean input.
