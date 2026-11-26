# macOS port: tools, probes and test server

Everything the macOS port adds lives here. The documents are in [docs/port](../docs/port): the [specification](../docs/port/spec.md) says what the client must be, the [technical plan](../docs/port/technical-plan.md) how it is built and why, the [implementation plan](../docs/port/implementation-plan.md) in which order.

Outputs go to `build/port/`, which is untracked. The scripts need Apple clang, Python 3 and, for the server, Docker. Nothing in the build reads a Windows or DirectX header (M11.11): the client's Windows-shaped types and constants are `port/client/include/bf_types.h`'s.

**The Windows headers for reference work.** The Wine 11.0 headers the build used until M11.11 are in the history, not in the tree. The two references taken from them (`port/client/test/wine_types.ref` for `bf_types.h`, `port/tools/layout-win32.ref` for the 32-bit Windows record layouts) are rewritten with them back, which a new name in `bf_types.h` or a new record on the packet and file path needs:

```bash
last=$(git log -1 --format=%H --diff-filter=D -- port/compat/winheaders/windows.h)^   # the last commit with them
mkdir -p build/port/winheaders-ref && git archive $last port/compat/winheaders | tar -x -C build/port/winheaders-ref
export BF_WIN_HEADERS=$PWD/build/port/winheaders-ref/port/compat/winheaders
python3 port/client/test/gen_types_test.py --write-reference port/client/test/wine_types.ref
mkdir -p build/port/layout/reftree && git archive 454ff09 src | tar -x -C build/port/layout/reftree
python3 port/tools/layoutcheck.py --live --ref-tree build/port/layout/reftree/src --write-reference
python3 port/tools/nowin.py --write-names      # port/tools/winheaders.txt, the header names nowin reports
```

`$last` is the parent of the commit that deleted the headers; `454ff09` the last whose `src/` compiled for 32-bit Windows (before M11.1 put the native modules' headers in it); `--ref-tree` gives a tree with newer records instead. Then `git diff` shows what changed.

## Build and fast check

The port builds with CMake and Ninja from the repository root: `cmake --preset debug && cmake --build --preset debug` (presets `debug`, `asan` with address and undefined-behaviour sanitizers and pattern-filled locals, `tsan` (thread sanitizer, M8.5; `TSAN_OPTIONS=log_path=... exitcode=0` keeps the harness's exit check meaningful, as Metal's completion handler gives reports TSan cannot order), `release`; build folders `build/cmake/<preset>`). Targets: `bifrost` (the client, from `sources.mk`'s `GAME` list minus the exclusions listed in `CMakeLists.txt`), `native` (every module in `port/native` and its test program), `probes` (`bifrost-packetlen`, `bifrost-netprobe`; `-DBF_PROBE_SRC=<dir>` builds them from another tree, such as the shadow copy), `check` (all module tests; a window opens and sound plays) and `check-headless`. The client's flags are written once, in `port/tools/flags.sh`, which CMake reads at configure time and the census and probe scripts source.

**Running the client:** `build/cmake/debug/bifrost` (the game folder from settings, `BF_CLIENT_DIR` or `~/Games/Bifrost`); `--headless` runs the start-up (archives, `sclientinfo.xml`, name tables, login mode) and exits before the first frame; `--mute` (or `BF_MUTE=1`) silences the sound output while the sound module runs and counts as usual (the harness sets it for every client it starts; `BF_MUTE=0` to listen); `--background` (or `BF_BACKGROUND=1`) shows the window without activating the application: the keyboard stays where it is and the real pointer passing over the window is ignored until a click activates it (the harness starts every client so, bundles with `open -g`; `BF_BACKGROUND=0` to have them come forward); `BF_NO_KEYCHAIN=1` keeps the client away from the Keychain (the harness sets it: a rebuilt, ad-hoc signed client is not on the saved password's access list and would raise an access prompt); `--control <socket>` (or `BF_CONTROL`) opens the control socket of [PROTOCOL.md](native/control/PROTOCOL.md) (state parts `mode`, `detail`, `view`, `windows`; events `mode`, `window`). **Launch arguments** go straight into the game (spec section 2, option `LaunchArguments`, on by default): `bifrost --server 127.0.0.1:6900 --account bftest --password bftest --character BFProbe` logs in, takes the first character server and enters with that character (a name or a slot number) without input; each also as `--flag=value` or in the environment (`BF_SERVER`, `BF_ACCOUNT`, `BF_PASSWORD`, `BF_CHARACTER`; scripts should prefer the environment for the password, which `ps` would show). `--server` is an IPv4 address with an optional port; without it the account server of `sclientinfo.xml` is used. An account without a password fills in the login window. Each step happens once: after a refused login or a return to character selection the windows work as usual. The port's entry point and the client-facing glue (`main`, graphics start, per-frame hook, zlib wrappers, CRT functions, the crash handler) are in [`port/client/src`](client/src/README.md) (headers in [`port/client/include`](client/README.md)), library `bf_port_main`. `port/client` is the port's own client code that stays after M11 (entry point, control bindings, prefix header, CRT helpers, `bf_types.h`); `flags.sh` puts `port/client/include` on the include path.

**The application bundle** ([`port/app`](app/README.md), M12.1). `cmake --build --preset debug` also makes `build/cmake/debug/<name>.app` (target `app`): the client executable, an Info.plist from `port/app/Info.plist.in` (bundle id and name from `port/name.cmake`, macOS 14, arm64), an icon drawn by `bifrost-icon`, no game data, ad-hoc signed and verified in the build. The bare executable `build/cmake/debug/<exe>` stays for the fast check and `BF_CLIENT`. `open build/cmake/debug/<name>.app` starts it as Finder does; the game folder comes from the settings or the chooser. `BF_APP=<path>.app python3 port/harness/run.py gate` runs a harness script against the bundle launched through LaunchServices, with the game folder stored in the run's settings (`BF_APP_FOLDER=links` stores a folder of links, so that the trace proves the setting was used). The fast check (`--client`) builds the bundle and verifies its signature.

**Game-folder path limit.** The game folder's path may be at most `BF_FILE_FOLDER_MAX` = 768 bytes. The kernel takes paths of at most 1,023 bytes, and the frameworks the client hands whole paths to (AVFoundation for `BGM/`) go through it; 768 leaves 254 bytes for names below the folder (longest measured: 209, a `data.grf` name in NFD). A longer folder is refused by the chooser, a stored `Paths/GameFolder` and `BF_CLIENT_DIR` alike, with a reason naming the length and the limit, and is never replaced by `~/Games/<name>` ([sys/README.md](native/sys/README.md), "Long paths"). The harness fails a launch whose trace names another game folder than the one it stored or passed.

**Usable game folder.** Besides the length limit, the chooser, a stored `Paths/GameFolder`, `BF_CLIENT_DIR` and `--headless` refuse, with a reason naming what is missing, a folder that is relative, holds none of the archives the client opens (`DATA.INI`'s `[Data]` list, else `data.grf`), holds an archive the client cannot open, or lacks `data/sclientinfo.xml`, `data/msgstringtable.txt` or the default sprite `data/sprite/몬스터/poring.spr`/`.act` both loose and in those archives ([prefs/README.md](native/prefs/README.md)). Everything else may be missing: a missing sprite is the poring, a missing bitmap a magenta square, a missing table counted. ctest `foldercheck` runs the headless start on generated incomplete folders.

**The fast check run at every integration** is `port/tools/fastcheck.sh`: it configures and builds the debug preset, checks that no unit read a Windows or DirectX header and no tracked source names one (`nowin.py --ninja`, `--text`), that the client links only Apple frameworks, `libz`, `libc++`, `libSystem` and `libobjc` (`otool -L`; with `--client`), runs the headless module tests and the census, and prints one line per part; `--client` also requires the client to build, `--client-errors` instead checks that the client's compile errors match the census's. Logs go to `build/fastcheck/`.

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

The census checks the units of `sources.mk` (since M11.11 exactly the client's: the units the build had excluded left `src/` then). Since M2.4 every unit compiles.

## Probes

```bash
port/probe/build.sh                       # builds both probes as arm64 programs
build/port/bin/bifrost-packetlen               # the client's packet length table
build/port/bin/bifrost-netprobe                # logs in through to the zone server
port/tools/packetdiff.py --rathena <rathena checkout> 20080401 20080910
```

`packetdiff.py` resolves the `HEADER_X, sizeof(PACKET_X)` entries of the client's table and, with `-v`, compares every rAthena `PACKET_X` structure that has a header with the client's table; the structure pass needs the checkout's `3rdparty/` (`docker cp bifrost-rathena-rathena-1:/rathena/src <dir>` and `.../3rdparty` give one). At 20080401: 2 length-only mismatches (0x8b, 0x23d), neither sent nor parsed by rAthena; 0 structure differences. `port/tools/packetids.py [--names] [--self-test]` lists every packet id `src/` writes as a number (exit 0 when none, M10.2); `name_packet_ids.py` rewrites them to `packet.h`'s enumerators. `packetbuild_test.sh` builds `bifrost-packetbuild-test`, which checks the 15 request builders of `src/PaddingPacket.h` byte for byte. Adding a packet version: [packet-versions.md](../docs/port/packet-versions.md).

The probes compile the game's own `packet.cpp`, `device/PacketQueue.cpp` and `device/Connection.cpp` from `src/` (`BF_SRC` or `-DBF_PROBE_SRC` names another tree, such as the shadow copy).

`bifrost-netprobe` follows the steps of the game's login mode against a running server and reports the first packet whose header the client's length table does not know.

## Checking structure layouts

```bash
port/tools/layoutcheck.py [--tree DIR] [--ref-tree DIR]
```

Compiles the packet and file-format headers for 32-bit Windows and for arm64 and compares the size and field offsets of every record that travels on the wire or is read from a file; it exits 1 on a difference. `--discover` lists the record types the readers fill from buffers; `--self-test` checks the tool on a synthetic header. Since M2.2 every record on `src/` matches (804 records).

**Coverage (M9.5):** `BF_COVERAGE=<path>` writes the session's coverage report at exit (JSON): untranslated literals, placeholder lookups, drawn Korean or non-Windows-1252 text, each with its call frames; the `coverage` state part shows it live. `BF_CLIENT=<binary> python3 port/harness/run.py english_session` plays the M9 session and passes only with an empty report. Details in [native/strings/README.md](native/strings/README.md).

## Reference executable

`port/tools/refexe.sh --string TEXT | --addr 0xA | --func NAME | --callers 0xA | --refs 0xA` prints the decompilation of the matching function (`--refs` lists the reads and writes of a global) in the stock 2008-09-03a Sakexe (Ghidra project in `$BF_REFEXE_DIR`, default `~/Games/Bifrost/refexe`). Provenance, rebuild steps and anchors: [reference-exe.md](../docs/port/reference-exe.md). The executable is never run.

`port/tools/nowin.py [--build DIR] [--units GLOB] [--summary]`: reruns every unit of `build/cmake/<preset>/compile_commands.json` (the presets export it; a hand-configured build needs `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`) with `clang -M` and lists each header read from a Wine tree (`BF_WIN_HEADERS`), from `port/compat`, by a Windows/DirectX name (`port/tools/winheaders.txt`), or from the Windows libraries' headers copied into `src/`; exit 0 only when there are none. `--ninja` reads the same from Ninja's dependency log of the last build (under a second); `--text` fails on any tracked `#include` line in `src/` or `port/` that names a Windows header. `port/tools/native_headers.py` is the M11.11 include rewrite (`--apply`, idempotent) and its proof (`--snapshot` before, `--check` after: preprocessed tokens and `nm` symbol lists unchanged). `port/tools/unbuilt.py [--summary]`: the tracked files of `src/` that no unit of the last full build read (M11.11 deleted the 306 it listed; empty since). `port/tools/winnames.py`, the per-step report of the Win32/DirectX names the built `src/` files still used, was retired in M11.11 after its last report (every M11 group 0); it is in the history. Each runs in under a minute. `port/tools/native_clock.py` rewrites `timeGetTime()`/`GetTickCount()`/`Sleep()` in the files the build reads to `bf_clock_ticks`/`bf_clock_sleep_ms` (M11.1); running it again changes nothing; it needs `build/cmake/debug/compile_commands.json`.

**M4 frames:** `BF_CLIENT=<absolute path to build/cmake/debug/bifrost> python3 port/harness/run.py m4_frames` captures the login screen at UI scale 1, 2, 3, 4:3 and wide into `<game folder>/test/frames/m4/`, checks clean blocks, placement and pacing, and writes the checksums to `port/harness/manifests/m4-frames.sha256`. Run clients with `BF_MULTI_INSTANCE=1` while another client is up.

## Second player

`port/bot/build.sh` builds `bifrost-bot`, a headless second player for the tests that need two: it logs a character in with the game's own connection code, answers trade, party and guild invitations, whispers and chat, and takes commands as JSON lines on stdin or `--control <socket>`. `port/bot/selftest.py` plays three of them against the test server and checks the round trips in `chatlog`, `party`, `zenylog` and `char`. Commands and events: [bot/README.md](bot/README.md).

## Auditing game data

```bash
port/tools/grfaudit.py rdata.grf data.grf [--maps maps.txt] [--list-unsupported]
```

Reads the format version of every world, ground, attribute, model, sprite and action file and compares it with what the client's loaders accept; reports which maps are readable and which of the client's text tables are present. Archives are given in search order. It needs Python 3.6 and nothing else, so it can run on the machine that holds the data. On the kRO data of June 2021, 818 of 897 maps are readable and 12 of 29 text tables are absent.

## Game folder

`port/data/` fills and checks `~/Games/Bifrost`: `fetch.sh` copies the archives, music and ROenglishRE from a machine named in the environment, `assemble.py` builds the loose `data/` folder (the missing text tables, `sclientinfo.xml`, the pre-renewal layouts of eight maps the server walks), `port/tools/tablecheck.py` checks the tables against the client's parsers (and `tipOfTheday_e.txt` in the game folder root, which `assemble.py` places from ROenglishRE), and `server-maps.sh` writes the server's list of enterable maps. Details in [data/README.md](data/README.md).

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

The Win32-shaped layer of stage A (`port/compat/<area>`, each area mapping the Windows names the client called onto a native module) is gone: M11 moved every subsystem to the native modules, one step at a time, and M11.11 deleted the last area (`win32`), the CRT stand-ins (`port/compat/include`) and the Wine headers. Where each area's rules went: `d3d` (M11.9) to `port/client/src/bf_render.cpp` on `port/native/gfx` (test `bifrost-render-test`); `input` and `gdi` (M11.7, M11.8) to `port/client`'s `bf_input` (see "bf_input" in [client/src/README.md](client/src/README.md), test `bifrost-input-test`) and `port/native/text`'s `bf_text_canvas`; `miles` (M11.6) to `src/device/Sound.cpp` on `port/native/audio` (see "The client" in [native/audio/README.md](native/audio/README.md)); `granny` and `bink` (M11.10) to `port/native/formats/granny` (`bifrost-granny-test`) and `port/client/src/bf_movie.cpp` (`bifrost-movie-test`; in the client `port/harness/scripts/granny.py`, `intro.py`); the registry calls (M11.4) to `bf_setting_*` (`port/client/include/bf_setting_key.h`; `port/tools/native_settings.py` is the rewrite that did it); the C runtime's file names on client paths to `port/client/src/bf_crt.cpp` (`bifrost-crt-test`). The adapters' READMEs are in the history (`git log -- port/compat`).

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

`BF_PKT_REPLAY=log` replays a recorded log instead of the network (no socket opens; `BF_PKT_REPLAY_WAIT`, `_HOLD`, `_BYTES`; the rules are in `client/include/bf_replay.h`). Capture lines carry the connection (`account/1`, `char/2`, `zone/3`) as a seventh field. Test: `BF_CLIENT=<absolute path> python3 port/harness/run.py replay [--param zone=1]`.

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
