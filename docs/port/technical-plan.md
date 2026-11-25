# macOS port: technical plan

Goal: recover a general Ragnarok Online client from this source and deliver it as a native arm64 macOS application, with no emulation and no Wine runtime, that plays on a private server in English.

What the finished client must be, and how the work is run, is fixed in the [specification](spec.md). Two things are built. The **recovery** undoes what turned the client into the single-player game *HighPriest*. The **port** replaces everything Windows provided. The [implementation plan](implementation-plan.md) orders both into milestones. This document records what the code is, what was tried, and the design that follows.

Measurements quoted here come from the tools in [`port/`](../../port/README.md) and can be repeated.

## 1. What the source is

### 1.1 A client turned into a single-player game

The executable is *HighPriest*, a single-player game Gravity built on the Ragnarok Online client in 2008. The online client was not removed, only bypassed:

- `WinMain` ends in `g_modeMgr.Run(MT_GAME, "World_HP.rsw")` (`src/WinMain.cpp:1016`). An online client starts in `MT_LOGIN` with `login.rsw`.
- It opens `data_hp.grf`. `data.grf` is opened only if `g_serviceType != 0`, tested before the client-info XML that sets the service type is read, so with the default it is never opened (`WinMain.cpp:889-892`).
- `CGameMode::OnUpdate` does not call `CGameMode::PollNetworkStatus` (`GameMode.cpp:656`); in game mode the network is never read.
- `CGameMode::Initialize` ends with the single-player setup: `HP_AddItem`, `Zc_Notify_Entry_HP`, story state (`GameMode.cpp:613-620`).
- Settings live under `HKLM\Software\Gravity Soft\HighPriest`, written by an external `Setup_HP.exe`, and the program refuses to run unless its process is named `HIGHPRIEST.EXE`.

What the single-player game added:

| | Amount |
|---|---|
| Its own files: `HP_Battle.cpp`, `HP_BuySell.cpp`, `HP_RestTravell.cpp`, `HP_SaveLoad.cpp`, `HP_Tetris.cpp` | 7,097 lines, 54 functions (44 of them methods of `UIWindowMgr`) |
| Globals used by those files and by at most four others | 58 (`g_Aco_*`, `g_curStory`, `g_menuSelect`, …) |
| Lines in shared files that mention its functions or globals | about 450, in `GameMode.cpp` (119), `Session.cpp` (93), `UIWindowMgr.cpp` (81), `GameModePacket.cpp` (63), `window/UISys.h` (44), `window/UIFrameWnd.cpp` (38) |

The online client is all there: the login mode with account, character and zone server steps (`LoginMode.cpp`), the packet dispatch of 16,570 lines (`GameModePacket.cpp`), 746 packet lengths (`packet.cpp`), the whole UI. It was compiled and linked into the original executable.

### 1.2 Size and shape

- 152 game translation units, about 230,000 lines, in `src/`. C++ in the Visual C++ 6.0 dialect, with its standard library.
- Statically linked libraries the port drops: the Visual C++ runtime (`src/library`, `vendor/`), the GameGuard client library (9 files), DirectX GUID and DirectInput data objects.
- Lua 5.0 (`src/l*.c`) is portable C and compiles as it is. Nothing reaches it at runtime: `MercenaryAI.cpp` is an empty stub.
- Imported DLLs with no arm64 build: `mss32` (Miles sound), `binkw32` (Bink video), `granny2` (Granny 3D models), `ijl15` (Intel JPEG), `cps` (zlib under another name).

### 1.3 Platform dependencies

| Area | What the game uses | Where |
|---|---|---|
| 3D | DirectDraw 7 and Direct3D 7, fixed function. Every vertex is transformed, lit and projected on the CPU and submitted as a pre-transformed vertex (two formats, one or two texture coordinates). `DrawPrimitive` and `DrawIndexedPrimitive` from user memory, about 15 render states, 12 texture stage states, 16 multitexture presets | Only `src/3dlib/` (5 files) and one use in `movie/CBink.cpp` |
| Textures | 16-bit A1R5G5B5 and A4R4G4B4 surfaces, filled by locking and writing pixels. Sprites with palettes are expanded on the CPU into 256×256 atlases | `3dlib/Texture.cpp`, `Renderer.cpp` |
| 2D and text | Each UI window owns a 32-bit DIB section. Sprites are copied in by the CPU, text is drawn by GDI (`CreateFont`, `TextOutW`, `GetTextExtentPoint32W`), then the bitmap is uploaded as texture tiles. Text is not antialiased; the 1-bit alpha depends on that | `3dlib/DC.cpp`, `DrawUtil.cpp` |
| Window and input | One window, a `PeekMessage` pump called from the game loop, about 20 window messages. Mouse through DirectInput 7 (relative, immediate). Keyboard through `WM_KEYDOWN` and `WM_CHAR`. IME through 7 `Imm*` functions, with the game drawing the composition window | `WinMain.cpp`, `device/Control.cpp`, `framework/Language.cpp` |
| Sound | Miles: 45 `AIL_*` functions. Only the 2D path runs (the registry reader forces it): samples from memory with distance-scaled volume, and one looping MP3 stream read from a file | `device/Sound.cpp`, `resource/Wave.cpp` |
| Network | Winsock 1.1, non-blocking sockets polled from the game loop with `select`. No window-message sockets. Packet encryption is not active | `device/Connection.cpp`, `PacketQueue.cpp` |
| Files | GRF archives memory-mapped (`CreateFileMapping`), loose files with `CreateFile`/`ReadFile`, backslash paths, CP949 file names, case-insensitive lookups | `base/File.cpp`, `GPak.cpp` |
| Settings | About 420 registry reads and writes, 352 of them window positions in `UIWindowMgr.cpp` | `Session.cpp`, `UIWindowMgr.cpp`, `WinMain.cpp` |
| Time and threads | `timeGetTime` (about 500 calls) drives fixed 60 Hz logic. Three kinds of worker thread, one critical section | `device/Timer.cpp`, `GameWorld.cpp`, `Mode.cpp` |
| Protection | GameGuard, two keyboard-encryption DLLs, the process-name check, an import-table hook probe, a crash dialog using `dbghelp` | `base/FindHack.cpp`, `WinMain.cpp:948`, `ExceptionHandler.cpp` |
| x86 only | Three inline-assembly sites: `_sqrt`/`_sin`/`_cos` in x87 (`3dlib/Prim.cpp:34`), `int 3` in asserts, self-patching code in the unused `EncClient.cpp`. `_controlfp` sets 24-bit precision | |

The rendering dependency is narrow. No game code outside `3dlib/` calls DirectDraw or Direct3D. What leaks out is data: the vertex layout (`tlvertex3d`, filled at about 115 call sites), `D3DBLEND_*` values, and blend and multitexture numbers stored in `.str` effect files.

## 2. What was tried

### 2.1 Compiling with clang for arm64

Each game source was compiled for `arm64-apple-macos` against Wine's Windows headers, without generating code (`port/tools/census.sh`).

| | clean units of 152 | distinct error locations |
|---|---:|---:|
| Source as reconstructed | 60 | 3,226 |
| After four mechanical rewrites (`port/tools/codemod.py`, applied to a copy) | 93 | 1,142 |

What the errors are:

| Cause | Locations | Remedy |
|---|---:|---|
| A pointer cast to `int` to pass through a message parameter: `SendMsg(NULL, AM_CHAT, (int)msg, 0, 0)` | 1,627 | Widen the parameters to `INT_PTR`; rewrite the casts from the compiler's own locations. The tool rewrote 1,539 |
| `long` and `unsigned long` meaning 32 bits (1,473 uses). This breaks declarations against definitions, and every structure read from a packet or a file | cascades | Rewrite to 32-bit types |
| Bink SDK headers, which do not know the platform | 642 | Gone when `CBink.cpp` stops including them |
| A loop variable used after its `for` (legal in Visual C++ 6.0) | 170 | Hoist the declaration |
| Maps with an explicit `std::allocator<V>`, missing `typename`, a forward-declared enum, vector iterators used as pointers, MSVC runtime names (`_unlink`, `__max`, `_lrotl`) | about 330 | By hand or small rewrites; most repeat one header line |

Part of this disappears with the recovery: `HP_RestTravell.cpp` alone holds 274 of the pointer casts.

Clang's Microsoft compatibility mode (`-fms-compatibility`) was tried and makes it worse: 5 clean units, because it conflicts with the macOS system headers.

The code is regular and the reconstruction is clean. Getting it to compile is days of mechanical work, not a research problem.

### 2.2 `long` is the dangerous one

Unlike the pointer casts, a 64-bit `long` compiles silently. `CHARACTER_INFO` in `packet.h` starts with `unsigned long GID`; built as it is, the character packet is 114 bytes instead of 110. With `long` rewritten, the packet structures have their original sizes: the probe below reads 59 server packets without losing its place.

### 2.3 The client's network code against rAthena

`bifrost-netprobe` links the client's own `Connection.cpp`, `PacketQueue.cpp` and `packet.cpp` as arm64 code and follows the login mode's steps. Against rAthena (master of 2026-11-23, pre-renewal, `PACKETVER=20080401`) running on this Mac:

```
account server: accepted, account 2000101, 1 character server(s), first "HighPriest" at 127.0.0.1:6121
character server: accepted, 1 character(s), 108 bytes each
character server: zone server 127.0.0.1:5121, map "prontera.gat"
zone server: accepted, position (155, 183)
zone server: 59 packets of 19 kinds in the first 3000 ms after loading, all with known headers
```

Two Winsock differences surfaced and are handled in `port/compat/include/winsock.h`: `select` ignores its first argument on Windows and the client passes 0, and a non-blocking `connect` reports a different error number.

### 2.4 Which protocol the client speaks

The executable is dated 2008-11-05, but its packet table is older. `port/tools/packetdiff.py` compares the client's 722 zone packet lengths with rAthena's packet database:

| rAthena `PACKETVER` | length mismatches | of them requests the client sends |
|---|---:|---:|
| 20071106 – 20080820 | 2 | 0 |
| 20080910 and later | 22 | 15 |

The client sends `CZ_ENTER` as 0x9b with 26 bytes and its table ends at 0x35b. From 2008-09-10 rAthena expects the same request ids with new layouts (and adds 0x0436–0x0439); what adding such a version takes is in [packet-versions.md](packet-versions.md). A server for this client is built with a `PACKETVER` from 20080401 to 20080820; rAthena master compiles at 20080401 without changes.

Two length entries differ (0x8b, 0x23d), and rAthena neither sends nor parses either; 0x24d and 0x251 were an artefact of the measuring tool. rAthena answers "end my auction" with a 4-byte 0x25d where the client's table has 6; the client reads 4 (M8.4).

### 2.5 The 2021 game data

`port/tools/grfaudit.py` reads the format version of every world, ground, attribute, model, sprite and action file in an archive and compares it with the highest version the client's loaders accept. On the kRO data of June 2021 (`rdata.grf` over `data.grf`, 139,652 files):

| Kind | Client reads up to | Files | Readable | Too new | Old encryption, not inspected |
|---|---|---:|---:|---:|---:|
| World `.rsw` | 2.1 | 897 | 178 | 79 | 640 |
| Ground `.gnd` | 1.7 | 897 | 256 | 0 | 641 |
| Attributes `.gat` | 1.2 | 897 | 254 | 0 | 643 |
| Model `.rsm` | 1.5 | 7,033 | 4,014 | 0 | 3,019 |
| Sprite `.spr` | 2.1 | 34,107 | 24,601 | 0 | 9,506 |
| Action `.act` | 2.5 | 39,029 | 29,450 | 0 | 9,579 |

The last column is files stored with the archive's old DES scheme, which the client decodes; they predate every format change.

- **818 of 897 maps** have world, ground and attribute files the client reads, among them Prontera, Morroc, Geffen, Payon, Alberta, Izlude, Aldebaran, Yuno, Comodo, Lighthalzen, Rachel, Veins, Moscovia, Brasilis and Malangdo.
- The 79 that do not are world files of version 2.2 to 2.5: instance dungeons and maps added after 2012.
- The archive also holds 889 `.rsm2` models, a format this client does not know. The audit has not yet checked which readable maps, if any, refer to one.
- 12 of the 29 text tables the client loads are no longer in the archive (kRO moved them to Lua): the message table, the item name, description and resource tables, and the skill tables. English versions of all of them are in ROenglishRE, which keeps the old table format for clients before 2012 (`Translation/Compatibility/2012-04-10/Pre-Renewal/data`, `Translation/Pre-Renewal/data`).

A first search for a kRO installation from 2008–2010 found none; the old kRO patch server has been closed ([RO Patcher Lite notes](https://nn.ai4rei.net/dev/rsu/help/qa.htm)).

### 2.6 Wine on Apple Silicon

- The Wine installed here is the Game Porting Toolkit: x86-64, run under Rosetta. Linking against it does not give an arm64 binary.
- Upstream Wine builds for arm64 macOS but did not run there as of the reports found: it needs a fixed mapping at 0x7ffe0000 and the x18 register preserved, both of which depended on macOS changes ([Wine bug 58567](https://list.winehq.org/archives/list/wine-bugs@list.winehq.org/thread/AIEGB45LZWRUP7UAVUWQPPRXFVZU5I7L)). CodeWeavers announced an arm64 CrossOver preview in July 2026 that requires macOS 26.5 ([CodeWeavers blog](https://www.codeweavers.com/blog/mjohnson/2026/07/31/crossover-preview-the-right-to-bear-arm64-on-mac)).
- A Winelib build would still need the 64-bit source work of 2.1, and still have no Miles, Bink or Granny: those DLLs exist for x86 only.
- arm64 macOS reserves the low 4 GB of the address space, so 32-bit pointers cannot be kept by placing memory low. That is why x86 Wine on macOS needs Rosetta and segment tricks.

## 3. Design

### 3.1 Decisions

1. **The product is the online client.** The single-player game is removed from the port branch, not kept as a second mode. The reference commit preserves it.
2. **Recovered to work, not to match.** Restored code must behave correctly against rAthena; the 2008 original is consulted where a site is unclear, not reproduced line for line.
3. **One protocol, with the seams for another.** "General" means: works with any server that speaks the packet set in this source (rAthena or Hercules built for 2008-04 to 2008-08), with its address, archives and language taken from configuration. Packet ids and lengths are moved behind one table so a later version is additive; none is implemented.
4. **The 2021 data.** Development uses the kRO data of 2021 with English tables from ROenglishRE. An archive from 2008–2010 would be the closer match and is used if one turns up, but nothing waits for it.
5. **A local server.** Development runs against `port/server` on the development Mac. No other server is used or changed.
6. **A real 64-bit port for Apple Silicon.** arm64 only, macOS 14 or later. Pointers are 64 bits; Win32 integer types stay 32 bits (`DWORD`, `LONG` as `int` types); raw `long` is rewritten; message parameters that carry pointers become `INT_PTR`.
7. **No Wine runtime; Wine's headers as declarations.** Wine's headers describe the Win32 and DirectX 7 types correctly for a 64-bit `long` and let the sources compile unchanged at first. They are committed with their licence notice so the port builds offline. The functions behind them are written for this port on macOS frameworks and linked statically. A Winelib build on CodeWeavers' arm64 Wine stays a fallback and is not on the path.
8. **Two layers per subsystem: a native module that stays, and a Win32-shaped adapter that goes.** This is how "first a compatibility build, then native calls" is done without writing everything twice. The native module (`port/native`) is designed for macOS and has no Windows types. The adapter (`port/compat`) gives it the Win32 name the client calls today. Stage A ships both; stage B changes the client's call sites to the native module and deletes the adapter.
9. **Apple frameworks and zlib only.** Nothing else is linked into the application.
10. **Apple clang, C++14, libc++.** C++14 because the code uses pieces removed in C++17 (`binary_function`).
11. **Strings stay bytes inside the client.** The protocol, the archives and the data tables are CP949 or CP1252 bytes. Conversion happens only at three edges: drawing text, the file system, and keyboard input.
12. **The native modules never assume a person at the screen.** Input arrives through one queue, the renderer does not need a visible window, and time comes from one clock, so that an agent can drive the client later (3.8).
13. **`main` stays as it is.** All port work lives on the branch `macos-port`, which gives up the byte-identical build and never merges back.

### 3.2 Recovering the client

The recovery has two parts of very different certainty.

**Removing what was added** is mechanical. The five `HP_*` files leave the build. Their 54 functions and 58 globals name every call site, so the compiler finds what the inventory misses: after removal, any remaining reference is an error. The `UIWindowMgr` and `UIWindow` members declared for them go with them.

**Restoring what was replaced** needs evidence for each case, because the original lines are gone. The known cases are the start mode, the archive list, the network poll and the session state set up in `WinMain`. The unknown ones are in the 450 lines: input handling that was rerouted to the single-player menus, rendering paths that draw its backgrounds, packet handlers that were given single-player entry points. For each, the evidence is, in order of weight:

1. The code itself. Online functions that exist and are never called show where a call was removed (`CGameMode::PollNetworkStatus` is the model case).
2. The protocol. A handler must leave the client in the state the next packets assume, and the server's side of each exchange is open source.
3. A stock client of the same line and months (2008 Sakexe) as a reference binary. The two were built from the same code, so a function in doubt can be compared in disassembly; this source has the names.
4. Other open clients, for behaviour only.

The standard is that the feature works against rAthena. Where the first two kinds of evidence settle a site, the reference binary is not consulted.

**Making it general** is small: the server address, service type and language already come from `clientinfo.xml`. Added are an archive list in configuration (as later clients have in `DATA.INI`), a neutral application name and settings location, built-in defaults in place of `Setup_HP.exe`, and the removal of the process-name check. Three conveniences are options that default to on: skipping the licence and intro screens, a camera without zoom and rotation limits, and launch arguments that go straight into the game.

### 3.3 Subsystems

| Subsystem | Native module (stays) | Adapter for stage A (goes) |
|---|---|---|
| Application | AppKit window with a Metal layer, resizable, single instance; the client keeps its loop on the main thread and drains one event queue when it calls its pump | `CreateWindowEx`, `PeekMessage`, the window procedure and its `WM_*` messages |
| Graphics | Metal: textures, one vertex shader for pre-transformed vertices, fragment programs generated from the texture-stage state, alpha test, blend, depth, present, read-back | DirectDraw 7 and Direct3D 7 objects with the roughly 60 methods `3dlib` calls |
| Text | CoreText drawing into 32-bit bitmaps without antialiasing; font lookup by face and character set; code-page conversion through CoreFoundation | The GDI subset: fonts, memory DCs, DIB sections, `TextOutW`, extents, the polygon region fill |
| Input | Key and pointer state from the event queue; the pointer is the system pointer, never captured; clipboard through `NSPasteboard` | DirectInput mouse device, `GetAsyncKeyState`, `GetKeyState`, clipboard functions |
| Sound | AVAudioEngine: PCM buffers with per-voice volume, one looping file stream | The 45 `AIL_*` functions; the 3D provider functions report "none" |
| Network | BSD sockets | `winsock.h` mapping only (done) |
| Files | A read-only game folder and separate write locations; path translation, case-insensitive lookup with a directory cache, `mmap` | `CreateFile`, `ReadFile`, `FindFirstFile`, file mappings, `VirtualAlloc` |
| Settings | One property list in the application's support folder; the saved password in the Keychain | `Reg*` functions over it |
| Time, threads | `mach_absolute_time`, pthreads, C++ atomics | `timeGetTime`, `Sleep`, `CreateThread`, `_beginthreadex`, critical sections, `Interlocked*` |
| Shell | `NSAlert`, `NSWorkspace` | `MessageBox`, `ShellExecute` |

Replaced outright, with no adapter stage:

- **zlib** (`cps.dll`): the system zlib behind the two functions, with the length type corrected.
- **JPEG** (`ijl15`): ImageIO, at the two call sites.
- **Math assembly**: `sqrtf`, `sinf`, `cosf`.
- **Protection**: GameGuard's 9 files leave the build; its four entry points become empty; keyboard encryption and the hook probe are removed; the crash dialog becomes a signal handler that writes a backtrace through the trace.
- **Bink**: a stub that reports no movie. Its only use is an optional intro button.
- **Granny**: a stub. The 69 job names ending in `.gr2` (guild flags, the Emperium, guardians, treasure boxes) draw nothing until a loader is written; that is a late, optional milestone.

Details that decide whether the picture is right:

- Direct3D 7 puts pixel centres on integers, Metal on halves. The adapter shifts positions by half a pixel, and the client's start-up calibration (`C3dDevice::TestScreen`, 65,536 quads and a read-back) is replaced by fixed offsets.
- `CRenderer::Flip` locks the render target every frame. Honoured literally that is a GPU read-back per frame, so the adapter makes a lock lazy and reads back only when pixels are touched (screen effects, screenshots, fades).
- The colour key is already turned into alpha when textures are filled, so no colour-key state is needed on the GPU.

### 3.4 Game data

The game folder holds the archives, a loose `data/` folder that overrides them (the XML `readfolder` option makes the client look there first), and `BGM/`. The client only reads it. Settings go to Application Support, screenshots to Pictures, chat logs to Documents.

- **Archives**: `rdata.grf` and `data.grf`, listed in configuration. Nothing is repacked.
- **Tables**: the 12 missing text tables and `clientinfo.xml` go into `data/`, taken from ROenglishRE. A table checker confirms each has the entries the client indexes (1,387 message ids) and the separators its parser expects.
- **Maps the client cannot read**: the client reports an unsupported world version with an error box. The port turns that into a traced error and a refusal to enter, and the server's map list is cut to the 818 readable maps so nothing warps there. Reading world files 2.2 to 2.5 and `.rsm2` models is an optional later extension.
- **Missing files and entries**: the server knows items and monsters added after 2008, and the 2021 archive may lack or have renamed files the 2008 client asks for. In every such case the client draws an obvious placeholder (a magenta texture, a default sprite, `?name`), counts the miss in the trace and carries on. The original's error boxes for absent files are removed.

### 3.5 English

The client already has what an English service needs; this build only never used it.

- **Service selection.** `clientinfo.xml` with `<servicetype>america</servicetype>` selects English, code page 1252, the Arial font slot, and loads `msgStringTable.txt` from the data folder instead of the 1,248 built-in Korean messages. It also changes the login packet (`CA_LOGIN` instead of the Korean `CA_LOGIN_HAN`) and skips GameGuard.
- **Data tables.** As in 3.4.
- **Text in the code.** 4,679 string literals contain Korean, written as escaped CP949 bytes. About 2,500 are file and folder names inside the archives and must stay as they are. The 407 displayed strings of the single-player files leave with the recovery. About 124 displayed strings remain that bypass the message table (billing notices, pet states, name-balloon brackets). They are extracted by script into a table keyed by the original bytes and looked up at runtime, so the source keeps the original literal and a missing translation falls back to it.
- **Fonts.** The UI is laid out for 9-point bitmap Gulim and Arial. Face names map to macOS fonts; widths are checked on a test screen because clipped labels are the usual failure.
- Chat, names and the wire use Windows-1252. Korean typed by others is not decoded. The Korean service type still selects code page 949 and a Korean font.

### 3.6 Instrumentation and testing

Already in place (see `port/README.md`): categorised trace, packet log, counters, a report of unimplemented functions, assertions that the release build had compiled out, and the measuring tools for compile errors, packets and archives.

Added as the port grows:

- Scripted play through the control interface (3.8), judged by the test server's SQL logs and captured frames. This is how milestones that need playing are accepted.
- A headless bot grown from `bifrost-netprobe` as the second player for trade, party and chat.
- A check that every packet and file structure has the size and field offsets of the original, computed by clang for the 32-bit Windows target from the same headers.
- Frame capture to PNG on request, for comparing screens between changes.
- Replay of a recorded packet log into the client, to exercise game mode without a server.
- Address and undefined-behaviour sanitizer builds, and pattern-filled automatic variables: release code built by Visual C++ 6.0 tends to read uninitialised memory that happened to be zero.

### 3.7 Picture, window and input

**World at native resolution, UI in pixels.** Every vertex the client submits is already in screen coordinates of its own, logical, resolution. The graphics module multiplies positions by the UI scale (1, 2 or 3) in the vertex shader and rasterises at the display's native resolution. World geometry and its filtered textures come out sharp. UI quads and text bitmaps are sampled without filtering, so at an integer scale each original pixel becomes a clean block. No separate UI pass is needed. What has to be right is which draws are sampled unfiltered. How the client sets its texture filters for UI draws has not been read from `3dlib` yet; if it filters them, the graphics module decides by texture kind instead, since UI tiles and text bitmaps are uploaded through their own path (`3dlib/DC.cpp`).

**Resizing.** The logical resolution is the window's pixel size divided by the UI scale. The original fixed it at start-up, so a resize path is added: the renderer's size, the projection, mouse picking and the window manager's bounds are updated, the UI is invalidated, and saved window positions are clamped into view. A wider window shows more of the world.

**Pacing.** A frame is drawn per display refresh, up to 120 Hz; the logic keeps its 60 steps per second, which the client's own skip-frame count already separates from drawing. The process opts out of App Nap and keeps drawing when not visible.

**Pointer.** The cursor sprite is drawn at the system pointer's position, converted to logical coordinates. The DirectInput adapter reports position differences, so the client's own integration lands on the same spot; right-drag camera rotation uses the same differences.

**Keyboard.** Option is Alt: when a game shortcut matches, the character macOS would type is suppressed. Function keys are taken from key events whether or not the system is set to use them as standard function keys, where macOS delivers them. Insert gets a substitute. Cmd+Q, Cmd+F and Cmd+V are handled before the client sees the event.

### 3.8 Control interface

Started with a flag, the client listens on a Unix-domain socket and speaks JSON, one object per line: commands in, events and answers out.

- **Now**, for scripted tests: inject key and pointer events into the event queue, capture a frame to a file, and query state (mode, own position, HP and SP, visible actors, open UI windows).
- **Later**, for agents: drawing without a visible window, advancing the loop by a fixed number of logic steps, and a fuller state feed.

The later part is not built, but three rules keep it possible: input enters only through the event queue; drawing goes to a target the application module provides, which need not be on screen; and the clock is read in one place.

## 4. Risks

| Risk | Likelihood | Consequence | Answer |
|---|---|---|---|
| The single-player game replaced more online code than the known switches | Medium | Features fail in ways that look like port bugs | Inventory with a verdict per site (3.2); reference binary for cases in doubt; packet replay separates protocol from engine faults |
| Pointer truncation left in a place the compiler did not flag (int back to pointer) | Medium | Crashes far from the cause | Warnings for int-to-pointer conversion treated as errors; address sanitizer |
| 2008 UI files changed or gone in the 2021 archive | Medium | Missing or wrong-sized interface pieces | `BF_TRACE=file` lists every miss; replacements go into the loose `data/` folder |
| Readable maps that refer to `.rsm2` models | Unknown | Missing objects on those maps | Extend the audit to model references before relying on a map |
| Layout or precision differences from x87 and 64-bit | Low | Visual or pathfinding differences | Structure-layout check; captured frames |
| Text metrics differ from GDI | Medium | Clipped or misaligned UI text | Font test screen; per-face adjustments |
| UI code assumes a resolution fixed at start-up | Medium | Misplaced windows or stale sizes after a resize | One resize path that updates every holder of the size; scripted resize test with captured frames |
| macOS keeps some function keys or shortcuts for itself | Medium | A skill-bar key does nothing | Key test screen; documented substitutes |
| rAthena drifts away from old packet versions | Low | Server build breaks | The server image pins a commit once the first login works |
| Third-party formats (Granny, Bink) | Certain | Guild objects and the intro missing | Stubbed; optional milestone |

## 5. Not in scope

Windows builds of the port, the byte-identical build on the port branch, the single-player game, packet versions other than the one in the source, Intel Macs and iPad, several clients at once, Unicode chat, a key-map editor, distribution of game data, and play on official servers.
