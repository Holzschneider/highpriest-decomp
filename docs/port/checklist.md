# Checklist

The state of the run. Step ids are the milestone and step numbers of the [implementation plan](implementation-plan.md); letters mark steps the run added. The lead updates this file with every integration.

- `[ ]` open, `[x]` done (with the commit), `[p]` parked (evidence in [progress.md](progress.md)).
- **Needs** names the steps that must be done first. A step with no entry there can start at any time, whatever its milestone: tracks run ahead of the milestone order, exits close in order.
- **Track**: `src` changes the client source and is integrated one package at a time; everything else is new code under `port/` or data outside the repository and runs in parallel.

## M1. Recover the client source

| | Step | Track | Needs |
|---|---|---|---|
| [x] | M1.0 Rename the port's prefix from `HP`/`hp` to `BF`/`bf` (switches, environment variables, files, programs, docker project) | src | |
| [x] d02c541 | M1.1 Inventory tool and committed inventory with a verdict per site | src | M1.0 |
| [x] de9d589 | M1.2 Remove the `HP_*` files, declarations, globals and the sites classed as added | src | M1.1 |
| [x] 930c2bf | M1.3 Restore start-up: `MT_LOGIN` with `login.rsw`, `PollNetworkStatus`, `CGameMode::Initialize` tail, archive list from configuration | src | M1.2 |
| [x] 2d8cee7 | M1.4 Restore the remaining replaced sites; mark the unsettled ones | src | M1.3 |
| [x] 3c3c2b6 | M1.5 General client: name constant, no process-name check, no `Setup_HP.exe` loop, settings independent of the HighPriest key | src | M1.3 |
| [x] 174f7f0 | M1.6 Reference executable downloaded, checksummed, loaded in a disassembler | ref | |
| [x] dff2f75 | M1 exit (verifier) | | M1.0–M1.6 |

## M2. Compiles for arm64

| | Step | Track | Needs |
|---|---|---|---|
| [x] 829fac4 | M2.1 Wine headers vendored under `port/compat/winheaders` with licence notice and regeneration script | headers | |
| [x] 634ca47 | M2.2 Codemods on `src/`: `long32`, `allocator`, `enumfwd`, `typename`, and `time_t` fields of `packet.h` to 32 bits (found by M2.8) | src | M1.5 |
| [x] e6ccdcb | M2.3 Message plumbing widened to `INT_PTR`; `ptrcast` applied; reverse direction reviewed | src | M2.2 |
| [ ] | M2.4 Loop-scope, iterator-as-pointer, runtime-name and one-off errors | src | M2.3, M2.1 |
| [x] 0f6c768 | M2.5 Units out of the build with reasons; `CBink` and Granny classes stubbed behind their interfaces | src | M2.2 |
| [x] 8899bf1 | M2.6 The three assembly sites replaced | src | M2.2 |
| [x] a55de3e | M2.7 CMake project with Ninja: debug, sanitizer, release presets | build | M2.1 |
| [x] 32ba708 | M2.8 Structure-layout check for packet and file structures | tools | |
| [ ] | M2 exit (verifier) | | M1 exit, M2.1–M2.8 |

## M3. Links and starts without a window

| | Step | Track | Needs |
|---|---|---|---|
| [x] aef1fdb | M3.1 Native modules for files, settings, time, threads, shell functions (`port/native`), with test programs | native | |
| [x] 2e9b18e | M3.1a Their adapters in `port/compat` | compat | M3.1 |
| [ ] | M3.2 Every other imported function as a reporting stub; the binary links | compat | M2.4, M2.7, M3.1a, M4.4, M4.5a, M7.2a |
| [x] ce92fc1 | M3.3 File lookup: game folder, backslashes, case-insensitive, CP949 names; writes to the Mac locations | native | M3.1 |
| [x] d8b18e0 | M3.4 Settings: defaults, property list behind the registry adapter, one settings object | native | M3.1 |
| [ ] | M3.5 Missing files and table entries become placeholders with a counted trace line | src | M3.2 |
| [x] d4cc27f | M3.6 zlib and JPEG replacements | native | |
| [ ] | M3.7 `--headless` start | src | M3.2 |
| [x] b430822 | M3.8 Game folder `~/Games/Bifrost`: archives and music copied; script assembling the loose `data/` folder and `clientinfo.xml` | data | |
| [x] a4901c1 | M3.9 Table checker | data | M3.8 |
| [x] 509f590 | M3.10 `grfaudit.py` follows model references; list of enterable maps applied to the test server | data | M3.8 |
| [ ] | M3 exit (verifier) | | M2 exit, M3.1–M3.10 |

## M4. First pixels

| | Step | Track | Needs |
|---|---|---|---|
| [x] 1fcc699 | M4.1 Application module: window, fullscreen, Metal layer, single instance, event queue, no App Nap | native | |
| [x] 9c857f6 | M4.2 Graphics module on Metal, with test draws for the 16 multitexture presets and the blend modes | native | |
| [x] 5354114 | M4.3 Logical resolution, UI scale 1×/2×/3×, resize path: the native half (app and gfx follow the window, logical size and scale computed in one place, a resize test) | native | M4.1, M4.2 |
| [ ] | M4.3a Resize path in the client: renderer size, projection, picking, window-manager bounds, saved window positions clamped | src | M4.3, M4.4 |
| [x] 3bb1c7d | M4.4 DirectDraw and Direct3D adapter for the methods `3dlib` calls | compat | M4.2 |
| [x] 2a6bf9d | M4.5 Text module on CoreText, unsmoothed, with a metrics test | native | |
| [x] 86d81a8 | M4.5a GDI adapter | compat | M4.5 |
| [x] 3b7143e | M4.6 Control interface, first part: socket, JSON lines, frame capture | native | M4.1 |
| [ ] | M4.7 Options to skip the licence and intro screens | src | M3.7 |
| [x] a81ced5 | M4.8 Reference screenshots of 2008-era clients collected under `~/Games/Bifrost/test/reference/` | ref | |
| [ ] | M4 exit (verifier) | | M3 exit, M4.1–M4.8 |

## M5. Input and login

| | Step | Track | Needs |
|---|---|---|---|
| [x] 6d49fd8 | M5.1 Pointer through the DirectInput adapter | compat | M4.1, M4.3 |
| [x] 6d49fd8 | M5.2 Keyboard with the Mac-adapted defaults; clipboard | compat | M4.1, M4.3 |
| [ ] | M5.3 Edit control for account and password; Keychain | src | M5.2 |
| [ ] | M5.4 Character selection and creation screens | src | M5.3 |
| [ ] | M5.5 Launch arguments for server, account, character | src | M5.4 |
| [ ] | M5.6 Control interface, second part: inject events, query mode and open windows | native | M4.6 |
| [x] 9237814 | M5.7 Test harness in Python: server, client, socket, SQL logs | harness | |
| [ ] | M5 exit (verifier) | | M4 exit, M5.1–M5.7 |

## M6. In the world

| | Step | Track | Needs |
|---|---|---|---|
| [ ] | M6.1 World loading: ground, attributes, models, lightmaps, water, actors | src | M5.5 |
| [ ] | M6.2 Camera, picking, click-to-move; free-camera option | src | M6.1 |
| [ ] | M6.3 Open recovery sites of game mode settled | src | M6.1 |
| [ ] | M6.4 Control interface, third part: position, HP and SP, visible actors | native | M5.6 |
| [ ] | M6.5 Packet replay | src | M3.7 |
| [ ] | M6.6 Unsupported map version returns to character selection | src | M6.1 |
| [ ] | M6 exit (verifier) | | M5 exit, M6.1–M6.6 |

## M7. Sound

| | Step | Track | Needs |
|---|---|---|---|
| [x] d9c523f | M7.1 Sound module on AVAudioEngine with a test program | native | |
| [x] 63cf204 | M7.2 WAV and MP3 decoding through AudioToolbox | native | |
| [x] 528ff44 | M7.2a Miles adapter | compat | M7.1, M7.2 |
| [ ] | M7.3 Music fades with a lock around the shared volume | src | M7.2a |
| [ ] | M7 exit (verifier) | | M6 exit, M7.1–M7.3 |

## M8. Playable

| | Step | Track | Needs |
|---|---|---|---|
| [ ] | M8.1 Feature checklist in play order, each item a script (expanded into sub-steps when M6 closes) | src | M6 exit |
| [x] e2ff959 | M8.2 Headless bot from the network probe: second character, answers trade, party, guild, chat | harness | |
| [ ] | M8.4 Protocol differences resolved on the client side | src | M6 exit |
| [ ] | M8.5 Worker threads made safe | src | M6 exit |
| [ ] | M8.6 Sanitizer runs and one-hour soak | src | M8.1 |
| [ ] | M8 exit (verifier) | | M7 exit, M8.1–M8.6 |

## M9. English

| | Step | Track | Needs |
|---|---|---|---|
| [ ] | M9.1 Login packet and code page of the English service type confirmed | src | M5.5 |
| [x] f18f603 | M9.2 Extraction script for displayed literals; translation table keyed by original bytes; runtime lookup in `port/` (its call sites in `src/` follow with M3.2) | tools | M1.2 |
| [x] 9a31eb4 | M9.3 Translation of the table | tools | M9.2 |
| [ ] | M9.4 Font mapping and text test screen | src | M4.5a |
| [ ] | M9.5 Coverage report | src | M9.2, M6 exit |
| [ ] | M9 exit (verifier) | | M8 exit, M9.1–M9.5 |

## M10. Protocol seam

| | Step | Track | Needs |
|---|---|---|---|
| [ ] | M10.1 One table for every packet's id and length | src | M8 exit |
| [ ] | M10.2 Handlers and senders name packets by enumerator | src | M10.1 |
| [ ] | M10.3 Version-dependent request packets built and parsed in one place each | src | M10.2 |
| [ ] | M10.4 Note on adding a version | tools | M10.3 |
| [ ] | M10 exit (verifier) | | M9 exit, M10.1–M10.4 |

## M11. Native calls only

| | Step | Track | Needs |
|---|---|---|---|
| [ ] | M11.1 Time and threads | src | M8 exit |
| [ ] | M11.2 Network | src | M8 exit |
| [ ] | M11.3 Files | src | M8 exit |
| [ ] | M11.4 Settings | src | M8 exit |
| [ ] | M11.5 Shell functions | src | M8 exit |
| [ ] | M11.6 Sound | src | M8 exit |
| [ ] | M11.7 Input and window | src | M8 exit |
| [ ] | M11.8 Text | src | M8 exit |
| [ ] | M11.9 Graphics | src | M8 exit |
| [ ] | M11 exit (verifier) | | M10 exit, M11.1–M11.9 |

## M12. Application

| | Step | Track | Needs |
|---|---|---|---|
| [ ] | M12.1 `.app` bundle, ad-hoc signed | build | M11 exit |
| [x] 37667cf | M12.2 Game-folder chooser and settings window | native | M4.1, M3.4 |
| [x] 18c1f99 | M12.3 Granny reader: format study on the game's `.gr2` files, loader, test program that dumps a model | formats | M3.8 |
| [x] 888c4ff | M12.3b Granny textures (RAD's Bink texture encoding) decoded and animation curves evaluated, with the test program drawing textured, posed models | formats | M12.3 |
| [ ] | M12.3a Granny models drawn in the client (guild flags, Emperium, guardians) | src | M12.3, M12.3b, M6 exit |
| [x] c08899a | M12.4 Bink reader: format study on the game's `.bik` files, decoder, test program that writes frames | formats | M3.8 |
| [ ] | M12.4a Intro movie played in the client | src | M12.4, M7 exit |
| [ ] | M12 exit (verifier) | | M11 exit, M12.1–M12.4a |

## Final

| | Step |
|---|---|
| [ ] | Final report in `progress.md`: what passed, what is parked, what the owner should try by hand |
