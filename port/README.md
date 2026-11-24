# macOS port: tools, probes and test server

Everything the macOS port adds lives here. The documents are in [docs/port](../docs/port): the [specification](../docs/port/spec.md) says what the client must be, the [technical plan](../docs/port/technical-plan.md) how it is built and why, the [implementation plan](../docs/port/implementation-plan.md) in which order.

Outputs go to `build/port/`, which is untracked. The scripts need Apple clang, Python 3 and, for the server, Docker. The scripts read the Windows headers vendored in `port/compat/winheaders` (Wine 11.0; `regenerate.sh` there rebuilds them); `BF_WIN_HEADERS` names another directory.

## Build and fast check

The port builds with CMake and Ninja from the repository root: `cmake --preset debug && cmake --build --preset debug` (presets `debug`, `asan` with address and undefined-behaviour sanitizers and pattern-filled locals, `release`; build folders `build/cmake/<preset>`). Targets: `bifrost` (the client, from `sources.mk`'s `GAME` list minus the exclusions listed in `CMakeLists.txt`), `native` (every module in `port/native` and its test program), `probes` (`bifrost-packetlen`, `bifrost-netprobe`; `-DBF_PROBE_SRC=src` builds them from the real tree instead of the shadow copy), `check` (all module tests; a window opens and sound plays) and `check-headless`. The client's flags are written once, in `port/tools/flags.sh`, which CMake reads at configure time and the census and probe scripts source.

**Running the client:** `build/cmake/debug/bifrost` (the game folder from settings, `BF_CLIENT_DIR` or `~/Games/Bifrost`); `--headless` runs the start-up (archives, `sclientinfo.xml`, name tables, login mode) and exits before the first frame; `--control <socket>` (or `BF_CONTROL`) opens the control socket of [PROTOCOL.md](native/control/PROTOCOL.md) (state parts `mode`, `detail`, `view`, `windows`; events `mode`, `window`). **Launch arguments** go straight into the game (spec section 2, option `LaunchArguments`, on by default): `bifrost --server 127.0.0.1:6900 --account bftest --password bftest --character BFProbe` logs in, takes the first character server and enters with that character (a name or a slot number) without input; each also as `--flag=value` or in the environment (`BF_SERVER`, `BF_ACCOUNT`, `BF_PASSWORD`, `BF_CHARACTER`; scripts should prefer the environment for the password, which `ps` would show). `--server` is an IPv4 address with an optional port; without it the account server of `sclientinfo.xml` is used. An account without a password fills in the login window. Each step happens once: after a refused login or a return to character selection the windows work as usual. The port's entry point and the client-facing glue (`main`, graphics start, per-frame hook, zlib wrappers, CRT functions, the crash handler) are in [`port/compat/stubs`](compat/stubs/README.md), library `bf_port_main`.

**The fast check run at every integration** is `port/tools/fastcheck.sh`: it configures and builds the debug preset, runs the headless module tests and the census, and prints one line per part; `--client` also requires the client to build, `--client-errors` instead checks that the client's compile errors match the census's. Logs go to `build/fastcheck/`.

## Measuring the compile work

```bash
port/tools/census.sh     # compiles every game source for arm64 without generating code, reports what clang rejects
port/tools/shadow.sh     # applies the automatic rewrites to a copy of src/ and reports what is left
```

`census.sh` is the progress measure of M2. `census.sh --warnings` keeps `-Wformat` as an error and counts non-literal format strings. `shadow.sh` leaves `src/` alone: it rewrites `build/port/shadow/src` with `codemod.py` and runs the census on that.

| | clean units of 152 | distinct error locations |
|---|---:|---:|
| `src/` as reconstructed (152 units, against the vendored headers) | 60 | 3,215 |
| after the recovery (M1, 147 units) | 60 | 2,775 |
| after the automatic rewrites (M2.2) | 90 | 2,283 |
| after the 64-bit plumbing, the stubs and the assembly sites (M2.3, M2.5, M2.6; 139 units, the build's exclusions skipped) | 89 | 352 |
| after the last sweep (M2.4) | 139 | 0 |

The census skips the units the build excludes (`BF_CLIENT_EXCLUDE` in the root `CMakeLists.txt`; `BF_CENSUS_ALL=1` counts them). Since M2.4 every unit compiles.

## Probes

```bash
port/probe/build.sh                       # builds both probes as arm64 programs
build/port/bin/bifrost-packetlen               # the client's packet length table
build/port/bin/bifrost-netprobe                # logs in through to the zone server
port/tools/packetdiff.py --rathena <rathena checkout> 20080401 20080910
```

`packetdiff.py` resolves the `HEADER_X, sizeof(PACKET_X)` entries of the client's table and, with `-v`, compares every rAthena `PACKET_X` structure that has a header with the client's table; the structure pass needs the checkout's `3rdparty/` (`docker cp bifrost-rathena-rathena-1:/rathena/src <dir>` and `.../3rdparty` give one). At 20080401: 2 length-only mismatches (0x8b, 0x23d), neither sent nor parsed by rAthena; 0 structure differences.

The probes compile the game's own `packet.cpp`, `device/PacketQueue.cpp` and `device/Connection.cpp` from `src/` (`BF_SRC` or `-DBF_PROBE_SRC` names another tree, such as the shadow copy).

`bifrost-netprobe` follows the steps of the game's login mode against a running server and reports the first packet whose header the client's length table does not know.

## Checking structure layouts

```bash
port/tools/layoutcheck.py [--tree DIR] [--ref-tree DIR]
```

Compiles the packet and file-format headers for 32-bit Windows and for arm64 and compares the size and field offsets of every record that travels on the wire or is read from a file; it exits 1 on a difference. `--discover` lists the record types the readers fill from buffers; `--self-test` checks the tool on a synthetic header. Since M2.2 every record on `src/` matches (804 records).

## Reference executable

`port/tools/refexe.sh --string TEXT | --addr 0xA | --func NAME | --callers 0xA | --refs 0xA` prints the decompilation of the matching function (`--refs` lists the reads and writes of a global) in the stock 2008-09-03a Sakexe (Ghidra project in `$BF_REFEXE_DIR`, default `~/Games/Bifrost/refexe`). Provenance, rebuild steps and anchors: [reference-exe.md](../docs/port/reference-exe.md). The executable is never run.

**M4 frames:** `BF_CLIENT=<absolute path to build/cmake/debug/bifrost> python3 port/harness/run.py m4_frames` captures the login screen at UI scale 1, 2, 3, 4:3 and wide into `<game folder>/test/frames/m4/`, checks clean blocks, placement and pacing, and writes the checksums to `port/harness/manifests/m4-frames.sha256`. Run clients with `BF_MULTI_INSTANCE=1` while another client is up.

## Second player

`port/bot/build.sh` builds `bifrost-bot`, a headless second player for the tests that need two: it logs a character in with the game's own connection code, answers trade, party and guild invitations, whispers and chat, and takes commands as JSON lines on stdin or `--control <socket>`. `port/bot/selftest.py` plays three of them against the test server and checks the round trips in `chatlog`, `party`, `zenylog` and `char`. Commands and events: [bot/README.md](bot/README.md).

## Auditing game data

```bash
port/tools/grfaudit.py rdata.grf data.grf [--maps maps.txt] [--list-unsupported]
```

Reads the format version of every world, ground, attribute, model, sprite and action file and compares it with what the client's loaders accept; reports which maps are readable and which of the client's text tables are present. Archives are given in search order. It needs Python 3.6 and nothing else, so it can run on the machine that holds the data. On the kRO data of June 2021, 818 of 897 maps are readable and 12 of 29 text tables are absent.

## Game folder

`port/data/` fills and checks `~/Games/Bifrost`: `fetch.sh` copies the archives, music and ROenglishRE from a machine named in the environment, `assemble.py` builds the loose `data/` folder (the missing text tables, `sclientinfo.xml`, the pre-renewal layouts of eight maps the server walks), `port/tools/tablecheck.py` checks the tables against the client's parsers, and `server-maps.sh` writes the server's list of enterable maps. Details in [data/README.md](data/README.md).

## Native modules

The modules that stay, each with its own test program (conventions in [native/README.md](native/README.md)). Build and run one with

```bash
cmake -S port/native/<module> -B build/native/<module> -G Ninja && cmake --build build/native/<module> && build/native/<module>/bifrost-<module>-test
```

| Module | Offers | Test needs |
|---|---|---|
| [`sys`](native/sys/README.md) | clock, threads, game folder and file lookup, write locations, settings, Keychain, alerts | game folder optional (`BF_HOME` redirects writes) |
| [`codec`](native/codec/README.md) | zlib, JPEG through ImageIO | game folder optional |
| [`app`](native/app/README.md) | window, Metal layer, event queue, input state, clipboard, single instance | opens a window; `--interactive` logs real input |
| [`text`](native/text/README.md) | CoreText fonts, metrics, unsmoothed drawing into bitmaps, code pages | nothing |
| [`prefs`](native/prefs/README.md) | game-folder chooser, settings window (UI scale, the four options, volumes, account) | opens an offscreen window; `--interactive` shows the real panel |
| [`view`](native/view/README.md) | logical size and UI scale in one place, the resize flow, pointer mapping | opens a window |
| [`control`](native/control/README.md) | the control socket of [PROTOCOL.md](native/control/PROTOCOL.md): JSON lines, command registry, input injection into the app queue, deferred replies | nothing; `test/crosscheck.py` runs the harness's protocol tests against it |
| [`gfx`](native/gfx/README.md) | Metal renderer for the client's pre-transformed vertices: textures, the 16 multitexture presets, blend, alpha test, depth, UI scale, lazy read-back, PNG capture; `D3D-USAGE.md` is the adapter's specification | nothing (draws offscreen) |
| [`audio`](native/audio/README.md) | AVAudioEngine voices, one music stream, WAV/MP3 decoding | plays sound; game folder optional (`BF_AUDIO_TEST_ALL=1` surveys every readable `.wav`) |
| [`formats/granny`](native/formats/granny/README.md) | Granny 2 model reader: sections (Oodle0), type tree, skeleton, meshes, materials, curves | game folder |
| [`formats/bink`](native/formats/bink/README.md) | Bink 1 movie decoder, video and audio | game folder; public sample movies under the test folder |

## Adapters

The Win32-shaped layer of stage A, which M11 deletes: each area in `port/compat/<area>` maps the Windows names the client calls onto a native module and has its own test program (built and run by `cmake --build --preset debug --target check`; areas are listed in the root `CMakeLists.txt`).

| Area | Windows names | On |
|---|---|---|
| [`win32`](compat/win32/README.md) | kernel32 files, mappings, threads, time; advapi32 registry (onto the settings plist and the Keychain); winmm; `ShellExecute`; `MessageBox`; the client's `fopen` and other C library file calls, translated through `port/compat/include/bf_win32_crt.h` | `sys`, `text` |
| [`d3d`](compat/d3d/README.md) | `DirectDrawCreateEx`, DirectDraw 7 and Direct3D 7 objects, the DirectX GUIDs; lazy render-target lock | `gfx` |
| [`gdi`](compat/gdi/README.md) | gdi32: memory DCs, DIB sections, fonts, text, the triangle fill | `text` |
| [`miles`](compat/miles/README.md) | the 46 `AIL_*` functions of Miles | `audio` |
| [`input`](compat/input/README.md) | user32 window, message pump, keyboard state, clipboard; DirectInput 7 mouse; imm32 (no IME) | `app`, `view` |
| [bink](compat/bink/README.md) | the Bink SDK names src/movie/CBink.cpp calls (BinkOpen … BinkClose, BinkDDSurfaceType, the sound system); the intro movie with its sound | formats/bink, audio |

`port/compat/granny`: the 44 `Granny*` functions and 2 data names of granny2.dll on `port/native/formats/granny` (step M12.3a), so that `src/resource/Model.cpp`'s Granny resource classes build as in 2008. `bifrost-compat-granny-test` replays the client's load, animate and skin sequence on the game's 21 `.gr2` files (`skipped` without `data.grf`). In the client: `port/harness/scripts/granny.py` (guild flags, Emperium, guardians, corpses; `--param woe=0` leaves out the castle step, which runs War of Emperium on the test server for about 10 s).

## Test harness

`port/harness/` drives the client through its control socket ([protocol](native/control/PROTOCOL.md)) and confirms each step in the test server's SQL logs; see [harness/README.md](harness/README.md). `python3 port/harness/run.py --self-test` checks the harness against its mock client; `BF_CLIENT=<binary> python3 port/harness/run.py <script>` runs a test script against the client and the test server, with the report under `build/harness/`.

## Instrumentation

`instrument/bf_trace.h` is compiled into the port and into the probes. The game sources call it where `BF_PORT` is defined: the game's own `Trace()` calls, `ErrorMsg`, assertions, connection events, every packet in both directions, file lookups, and mode changes.

| Variable | Effect |
|---|---|
| `BF_TRACE=net,pkt,file` | categories to print: `err game net pkt file res gfx snd input ui mode w32 stub all` |
| `BF_TRACE_FILE=path` | write the trace to a file instead of stderr |
| `BF_PKT_LOG=path` | append every packet as a line: time, direction, header, size, name, hex bytes |
| `BF_ASSERT=abort` | stop in the debugger on a failed `sAssert` |

`BF_PKT_REPLAY=log` replays a recorded log instead of the network (no socket opens; `BF_PKT_REPLAY_WAIT`, `_HOLD`, `_BYTES`; the rules are in `compat/include/bf_replay.h`). Capture lines carry the connection (`account/1`, `char/2`, `zone/3`) as a seventh field. Test: `BF_CLIENT=<absolute path> python3 port/harness/run.py replay [--param zone=1]`.

Failed assertions and the first call of each unimplemented compatibility function (`BF_STUB()`) are printed without any setting.

## Test server

```bash
cd port/server
docker compose up -d --build        # rAthena pre-renewal, PACKETVER 20080401, with MariaDB
docker compose logs -f rathena
docker compose stop
```

The server listens on 127.0.0.1:6900 (account), :6121 (character) and :5121 (zone). Accounts: `bftest` / `bftest` and `bfadmin` / `bfadmin` (group 99). To reach it from another machine, set `BF_BIND=0.0.0.0` and `BF_PUBLIC_IP` to the address the client connects to; `BF_LOGIN_PORT`, `BF_CHAR_PORT` and `BF_MAP_PORT` move the host ports when another server holds the defaults.

`PACKETVER` must stay between 20080401 and 20080820: `packetdiff.py` shows the client's zone packets matching that range and 15 client requests differing from 20080910 on.
