# macOS port: implementation plan

Thirteen milestones, M0 to M12, from the source as reconstructed to a general Ragnarok Online client that runs natively on macOS, plays in English and calls only macOS APIs. What the client must be is in the [specification](spec.md), the reasons for the design in the [technical plan](technical-plan.md); the tools named here are described in [`port/README.md`](../../port/README.md).

Each milestone ends in something that can be run and checked. Sizes are relative: **S** is a day or two of focused work, **M** several days, **L** a week or more, **XL** several weeks. They are estimates from the measurements so far, not commitments.

How the work is run (specification, section 8):

- All work is on the branch `macos-port`; `main` is not touched. One commit per completed step; the branch is pushed as work proceeds.
- Work does not stop between milestones. At each exit, `docs/port/progress.md` gets an entry: what passed, the commands that show it, captured frames, open recovery sites, deviations from this plan.
- Development runs against the rAthena in `port/server` on the development Mac and no other server.
- Criteria that need playing are scripts driving the client through its control interface, judged by the server's SQL logs and captured frames.

## Overview

| | Milestone | Ends with | Size | Stage |
|---|---|---|---|---|
| M0 | Foundations | Tools, probes, test server, first native login, data audit | done | |
| M1 | Recover the client source | Single-player game removed, online start-up restored | M | |
| M2 | Compiles for arm64 | Every unit compiles | M | A |
| M3 | Links and starts without a window | Binary reads archives and settings, reaches the login mode | M | A |
| M4 | First pixels | Login screen drawn with text, at any window size | L | A |
| M5 | Input and login | A script logs in and reaches character selection | M | A |
| M6 | In the world | A script walks a character through Prontera | L | A |
| M7 | Sound | Music and effects | S | A |
| M8 | Playable | Scripted gate test passes; a long session runs clean under sanitizers | XL | A |
| M9 | English | No Korean in the standard flows | M | A |
| M10 | Protocol seam | Packet ids and lengths behind one table | M | A |
| M11 | Native calls only | No Windows headers or adapters in the build | L | B |
| M12 | Application | `.app` bundle, settings, agent support, optional features | open | B |

M7 and M9 do not depend on M8 and can be done whenever M6 is reached. M11 can start per subsystem as soon as that subsystem is stable.

## M0. Foundations (done)

- Review of the source and its platform dependencies.
- `port/tools/census.sh`, `codemod.py`, `shadow.sh`: measure and preview the compile work.
- `port/tools/packetdiff.py`, `grfaudit.py`: measure the protocol against a server and an archive against the client's loaders.
- `port/instrument/hp_trace`: trace, packet log, counters, stub and assertion reports. Hooks under `HP_PORT` in `common.h`, `base/SimpleAssert.h`, `base/File.cpp`, `Mode.cpp`, `device/Connection.cpp`.
- `port/compat/include`: prefix header, runtime header stand-ins, `winsock.h` on BSD sockets.
- `port/probe`: `hp-packetlen` and `hp-netprobe`, the first arm64 programs built from the client's sources.
- `port/server`: rAthena pre-renewal at `PACKETVER=20080401`, with which `hp-netprobe` logs in through to the zone server.

## M1. Recover the client source

Turn the source back into the online client, before any effort goes into porting code that will be deleted.

1. Inventory tool: lists every line outside the `HP_*` files that mentions one of their 54 functions or 58 globals, with the enclosing function. The list is committed with a verdict per site: *added* (delete) or *replaced* (restore, with the evidence used).
2. Remove what was added: the five `HP_*` files, their declarations in `UIWindowMgr` and `window/UISys.h`, the globals, and the call sites classed as added.
3. Restore what is known to have been replaced:
   - `WinMain` starts `MT_LOGIN` with `login.rsw`; the single-player session set-up before it goes.
   - `CGameMode::OnUpdate` calls `PollNetworkStatus`.
   - `CGameMode::Initialize` loses its single-player tail.
   - The archive list is read from configuration, with `data.grf` as the default, before the first file is opened.
4. For each remaining site classed as replaced, restore behaviour that works against rAthena, from the evidence listed in the technical plan (3.2). Sites that cannot be settled by reading are marked in the source and carried to M6 and M8, where they can be observed.
5. Make it general: a neutral application name defined in one place, no process-name check, no `Setup_HP.exe` loop (defaults instead), settings independent of the HighPriest registry key.
6. Download a stock 2008 Sakexe executable as a reference binary, checksum it, keep it outside git.

**Done when**
- No source mentions a single-player function or global; the inventory tool reports nothing.
- `port/tools/census.sh` shows no error kind that the baseline did not have (removal broke nothing the compiler can see), and fewer errors than before.
- The committed inventory accounts for every one of the roughly 450 lines, and lists the sites still open.

## M2. Compiles for arm64

Make every source acceptable to clang as 64-bit code.

1. Windows headers: upstream Wine headers at a fixed version, committed under `port/compat/winheaders` with their licence notice and an entry in `THIRD-PARTY.md`. A script regenerates them (including the IDL-derived ones) and neutralises their calling-convention attributes for arm64. The dependency on the Game Porting Toolkit install goes.
2. Apply `codemod.py` to `src/`: `long32`, `allocator`, `enumfwd`, `typename`.
3. Widen the message plumbing: the `SendMsg`, `ProcessMsg`, `SendMsgToUI` and `MakeWindow` families take `INT_PTR` parameters. Then apply the `ptrcast` rule, and review the reverse direction with int-to-pointer warnings as errors.
4. Fix the loop-variable scope sites, iterator-as-pointer sites, missing runtime names, and the remaining one-off errors.
5. Take units out of the build with a recorded reason: GameGuard (9 files, four empty entry points remain), `EncClient`, `cKeyProtector`, `cSecureGamePack`, the Vietnamese input classes, `ExceptionHandler` (replaced by a signal handler). Replace `CBink` and the Granny resource classes by stubs behind their existing interfaces.
6. Replace the three assembly sites.
7. CMake project with Ninja: debug, sanitizer and release presets; the unit list comes from `sources.mk` minus the removals. Deployment target macOS 14, arm64.
8. Structure-layout check: a tool that computes size and field offsets of every packet and file structure with clang for the 32-bit Windows target and for arm64, and fails on a difference.

**Done when**
- `port/tools/census.sh` reports every unit clean, with each exclusion listed in the build file.
- The layout check passes for `packet.h`, `PaddingPacket.h` and the resource headers.
- `HP_SRC=src port/probe/build.sh` builds the probes from the real tree and `hp-netprobe` still ends with `OK`.

## M3. Links and starts without a window

Give the binary enough of an operating system to start, and give it its data.

1. Native modules and adapters for files, settings, time, threads and the shell functions. The clock is read in one place.
2. Every other imported function exists as a stub that calls `HP_STUB()`, so the binary links and reports what it reached.
3. Files: a read-only game folder named by setting or `HP_CLIENT_DIR`; backslashes translated; loose-file lookups case-insensitive; CP949 file names converted. Writes go to Application Support, Pictures and Documents under the application's name.
4. Settings: built-in defaults; the registry adapter reads and writes a property list. All access goes through one settings object.
5. Missing files and table entries produce a placeholder and a counted trace line instead of an error box.
6. zlib and JPEG replacements.
7. A `--headless` option that runs the start-up and stops before the first frame.
8. Game folder on the development Mac, outside the repository: `rdata.grf`, `data.grf` and `BGM/` copied from the owner's other Mac (about 3.5 GB), and a script that assembles the loose `data/` folder: the 12 text tables missing from the archives, taken unchanged from ROenglishRE, and a `clientinfo.xml` for the local server with the English service type.
9. Table checker: entry counts and separators of each table against what the client's parsers expect.
10. Extend `grfaudit.py` to follow each readable map's model references, and write the list of maps the client can enter; apply that list to the test server's map configuration.

**Done when**
- The client started with `--headless` opens both archives, parses `clientinfo.xml`, loads the name tables, constructs the login mode and exits 0.
- `HP_TRACE=file` shows each lookup with its source, and the stub report lists only graphics, sound, input and window functions.
- Nothing was written inside the game folder.
- The table checker passes, and the list of enterable maps exists.

## M4. First pixels

1. Application module: resizable window, native fullscreen, Metal layer, single instance, one event queue drained from the client's pump, quit handling, no App Nap.
2. Graphics module on Metal: textures in the two 16-bit formats and 32-bit, the pre-transformed vertex path, fragment programs from texture-stage state, alpha test, the blend modes in use, depth, present per display refresh, lazy read-back. It draws to a target the application module provides.
3. Logical resolution and UI scale: positions are scaled in the vertex shader; UI and text draws are sampled unfiltered. The resize path updates the renderer, projection, picking and window-manager bounds and clamps saved window positions.
4. DirectDraw and Direct3D adapter for the methods `3dlib` calls; fixed texel offsets in place of the start-up calibration.
5. Text module on CoreText and the GDI adapter: fonts, DIB sections, text output and extents, region fill. Text is not smoothed.
6. Control interface, first part: the socket, the JSON-lines framing, and the frame-capture command.
7. The options to skip the licence and intro screens, on by default.

**Done when**
- The login screen appears with wallpaper, window frames, buttons and text, at one frame per display refresh.
- Frames captured through the socket at UI scale 1, 2 and 3 and at a 4:3 and a wide window size are checked in as reference images; UI pixels are clean blocks at every scale.
- The 16 multitexture presets and the blend modes each have a test draw whose capture has been reviewed.
- Every interface file the login screen asked for and did not find is listed, with a replacement or a decision.

## M5. Input and login

1. Pointer: the system pointer's position in logical coordinates through the DirectInput adapter; buttons, wheel, focus changes. The pointer is never captured or hidden outside the window.
2. Keyboard with the Mac-adapted defaults: Option as Alt without stray characters, function keys, a substitute for Insert, Cmd+Q, Cmd+F and Cmd+V handled before the client; clipboard.
3. The client's edit control working for account and password; "save password" stores it in the Keychain.
4. Character selection and creation screens.
5. Launch arguments for server, account and character.
6. Control interface, second part: inject key and pointer events into the event queue; query the mode and the open UI windows.
7. Test harness in Python: starts the test server and the client, drives it through the socket, reads the server's SQL logs.

**Done when**
- The login script types an account and password, clicks through and reaches character selection with characters drawn; a second script creates a character.
- The packet log of that session matches the sequence `hp-netprobe` produces.
- The same login works from launch arguments alone.

## M6. In the world

1. World loading: ground, attributes, models, lightmaps, water; then sprites and actions for actors.
2. The camera, mouse picking and click-to-move; the free-camera option, on by default.
3. Settle the recovery sites left open in M1 that concern game mode, now that their effect can be seen.
4. Control interface, third part: own position, HP and SP, visible actors.
5. Packet replay (`HP_PKT_REPLAY`): feed a recorded log to the client so game mode can be debugged without a server.
6. An unsupported map version becomes a traced error and a return to character selection.

**Done when**
- The walk script enters Prontera, clicks a destination, and both the client's reported position and the server's agree on arrival; captured frames show the map, the character, NPCs and other actors.
- A five-minute scripted session leaves no unknown packet header and no failed assertion in the trace.
- A map change by warp portal works between two readable maps.
- Resizing the window in game keeps picking correct: a click lands on the cell under the pointer at two window sizes.

## M7. Sound

1. Sound module on AVAudioEngine: decoded PCM voices with volume, one looping stream.
2. Miles adapter: WAV and MP3 decoding through AudioToolbox behind `AIL_WAV_info` and `AIL_decompress_ASI`; the 3D provider functions report none.
3. Music fades on the existing thread, with a lock around the shared volume.

**Done when** the login music loops, map music changes on map change, and attack and UI effects play with distance-scaled volume. The check is the sound module's own counters of started voices and the stream's file name; how it sounds is left for the owner's play test.

## M8. Playable

Work through the client feature by feature against rAthena, fixing what the recovery, the port or the protocol breaks.

1. Checklist in play order, each item a script: movement and camera, chat, combat and damage numbers, items on the ground, inventory and equipment, NPC dialogue, shops, storage, skills and the skill tree, status window, emotes, options, screenshots, map change, logout and character switch.
2. Second player: `hp-netprobe` grows into a headless bot that logs in another character and answers trade, party, guild and chat at packet level. Scripts for those features run the client against it.
3. Each failure is first attributed: a recovery site (compare with the inventory and the reference binary), a port fault (stub report, sanitizers), or a protocol difference (packet log against the server's log).
4. Protocol differences: the four known length mismatches (mail and auction), anything `HP_TRACE=pkt` or the server log reports. Each is fixed on the side that is wrong for this client era.
5. Worker threads that call into the renderer and effects without locks (far-model loading) are made safe.
6. Sanitizer runs and a one-hour scripted soak.

**Done when**
- The scripted gate test passes: create a character, kill a Poring at the south gate, pick up the drop, buy a potion and sell the loot, change job at the Job Master, chat. Each step is confirmed in the server's SQL logs.
- The recovery inventory has no open site.
- The address sanitizer build completes the gate test with no report.
- A one-hour scripted session ends with no crash, no unknown packet and stable memory.

## M9. English

1. Confirm the login packet and code page the English service type selects; chat and names are Windows-1252.
2. Extraction script for displayed string literals outside the message table (about 124), a translation table keyed by the original bytes, and the runtime lookup. Path literals are excluded by the classifier and by review.
3. Translation of that table.
4. Font mapping and a text test screen for widths and line heights.
5. Coverage report: displayed strings that fell back to Korean, and table lookups that fell back to a placeholder name, during a session.

**Done when** login, character creation, the gate test of M8 and the option windows show no Korean in captured frames, and the coverage report for that session is empty.

## M10. Protocol seam

Prepare for a later packet version without implementing one.

1. One table holds every packet's id and length; `packet.cpp` fills it, and nothing else knows a number.
2. Handlers and senders name packets by enumerator; the numeric ids in `GameModePacket.cpp`, `LoginMode.cpp` and `packet.cpp` (0x9b, 0x72, the `case 120:` style of the dispatch) become names.
3. The request packets whose layout changed in later versions (the ones `packetdiff.py` flags from 20080910 on) are built and parsed in one place each.
4. A short note in `docs/port` on what adding a version takes, with the `packetdiff.py` output for one later version as the worked example.

**Done when** no handler or sender contains a numeric packet id, `hp-packetlen` prints the same table as before, and the gate test passes.

## M11. Native calls only

Remove the Windows-shaped layer, one subsystem at a time. For each: change the client's call sites to the native module, delete the adapter, run the gate test and compare captured frames.

Order, easiest first: time and threads, network (the socket code is already native; only the header mapping goes), files, settings, shell functions, sound (`device/Sound.cpp` and `resource/Wave.cpp` call the sound module), input and window (`WinMain.cpp` becomes a native entry point and `device/Control.cpp` reads the input module), text (`3dlib/DrawUtil.cpp`, `DC.cpp`), graphics (`3dlib/Device.cpp`, `Texture.cpp` and the flush code of `Renderer.cpp` call the graphics module; the vertex layout and blend constants move to the port's own header).

**Done when**
- No source includes a Windows or DirectX header; `port/compat` and the Wine headers are gone from the build.
- The binary links against Apple frameworks and zlib only.
- The gate test passes and the reference frames match.

## M12. Application

Open-ended; each item stands alone.

- `.app` bundle with a chosen game folder and a settings window.
- First-class agent support: drawing without a visible window, advancing the loop by a fixed number of logic steps, a fuller state feed, and the control interface documented as stable.
- Settings profiles and more than one client at a time.
- A user-editable key map.
- World files 2.2 to 2.5 and `.rsm2` models, for the 79 maps the client cannot read today.
- Granny models: an own loader for `.gr2` so guild flags, the Emperium and guardians appear.
- Intro movie through an own Bink decoder.
- Korean and other input methods through the system text input.
- A second packet version, on the seam of M10.
- An archive from 2008–2010, if one is found, as an alternative data set.

## Open points

| Point | State |
|---|---|
| The application's name | A neutral working name is used until the owner chooses one |
| The owner's play test | After M8 and M9; it judges feel and sound, which scripts cannot |
