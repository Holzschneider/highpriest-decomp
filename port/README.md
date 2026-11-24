# macOS port: tools, probes and test server

Everything the macOS port adds lives here. The documents are in [docs/port](../docs/port): the [specification](../docs/port/spec.md) says what the client must be, the [technical plan](../docs/port/technical-plan.md) how it is built and why, the [implementation plan](../docs/port/implementation-plan.md) in which order.

Outputs go to `build/port/`, which is untracked. The scripts need Apple clang, Python 3 and, for the server, Docker. Until the port pins its own Windows headers (milestone M2), they read the Wine headers of Homebrew's `game-porting-toolkit` in `/usr/local/include/wine/windows`; `BF_WIN_HEADERS` names another directory.

## Measuring the compile work

```bash
port/tools/census.sh     # compiles every game source for arm64 without generating code, reports what clang rejects
port/tools/shadow.sh     # applies the automatic rewrites to a copy of src/ and reports what is left
```

`census.sh` is the progress measure of M2. `shadow.sh` leaves `src/` alone: it rewrites `build/port/shadow/src` with `codemod.py` and runs the census on that.

| | clean units of 152 | distinct error locations |
|---|---:|---:|
| `src/` as it is | 60 | 3,226 |
| after the automatic rewrites | 93 | 1,142 |

Of the 1,142, 642 are inside the Bink SDK headers, which one unit includes, and 170 are loop variables used after their loop.

## Probes

```bash
port/probe/build.sh                       # builds both probes as arm64 programs
build/port/bin/bifrost-packetlen               # the client's packet length table
build/port/bin/bifrost-netprobe                # logs in through to the zone server
port/tools/packetdiff.py --rathena <rathena checkout> 20080401 20080910
```

The probes compile the game's own `packet.cpp`, `device/PacketQueue.cpp` and `device/Connection.cpp`. Because `src/` still declares 32-bit fields as `long`, `build.sh` takes the sources from the rewritten copy; `BF_SRC=src` switches to the real tree once the rewrites are applied there.

`bifrost-netprobe` follows the steps of the game's login mode against a running server and reports the first packet whose header the client's length table does not know.

## Checking structure layouts

```bash
port/tools/layoutcheck.py [--tree DIR] [--ref-tree DIR]
```

Compiles the packet and file-format headers for 32-bit Windows and for arm64 and compares the size and field offsets of every record that travels on the wire or is read from a file; it exits 1 on a difference. `--discover` lists the record types the readers fill from buffers; `--self-test` checks the tool on a synthetic header. On the shadow tree eight records still differ, all through `time_t` fields in `packet.h`, which are 32 bits on Win32.

## Auditing game data

```bash
port/tools/grfaudit.py rdata.grf data.grf [--maps maps.txt] [--list-unsupported]
```

Reads the format version of every world, ground, attribute, model, sprite and action file and compares it with what the client's loaders accept; reports which maps are readable and which of the client's text tables are present. Archives are given in search order. It needs Python 3.6 and nothing else, so it can run on the machine that holds the data. On the kRO data of June 2021, 818 of 897 maps are readable and 12 of 29 text tables are absent.

## Instrumentation

`instrument/bf_trace.h` is compiled into the port and into the probes. The game sources call it where `BF_PORT` is defined: the game's own `Trace()` calls, `ErrorMsg`, assertions, connection events, every packet in both directions, file lookups, and mode changes.

| Variable | Effect |
|---|---|
| `BF_TRACE=net,pkt,file` | categories to print: `err game net pkt file res gfx snd input ui mode w32 stub all` |
| `BF_TRACE_FILE=path` | write the trace to a file instead of stderr |
| `BF_PKT_LOG=path` | append every packet as a line: time, direction, header, size, name, hex bytes |
| `BF_ASSERT=abort` | stop in the debugger on a failed `sAssert` |

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
