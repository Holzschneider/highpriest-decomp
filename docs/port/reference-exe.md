# The reference executable

A stock kRO client of the same line and months as the source in `src/`, kept so that a function in doubt (a recovery site, a replaced site, a protocol detail) can be compared with the original in a disassembler. Technical plan 1, item 3; checklist step M1.6.

It is for reading only. **It is never run**, not under Wine, Rosetta, CrossOver or anything else. It is not in git and must never be.

## The file

| | |
|---|---|
| File | `2008-09-03aSakexe.exe` (kRO Sakray client, build 2008-09-03 "a") |
| Size | 3 399 766 bytes |
| SHA-256 | `a330d18dff3d52f6fbb45a1b6ce883ca7a8b1321b539e2da983109037a5d4df4` |
| CRC-32 | `0xC58B5246` (3314242118) |
| Format | PE32, i386 (machine 0x14c), GUI; linker 6.0 (Visual C++ 6); image base 0x400000, entry 0x6B961A |
| PE timestamp | 0x48BDDBF9 = 2008-09-03 00:36:09 UTC |
| Sections | `.text` 0x401000 (0x2D7A4A), `.rdata` 0x6D9000, `.data` 0x702000, `.rsrc` 0x7C7000. Not packed: the code is readable as it is, and the file offsets equal the virtual addresses minus 0x400000 |
| Archive | `2008-09-03aSakexe.rar` (1 092 667 bytes, SHA-256 `f88e85283e245e311ab72803c9c1404c0f2dfc4201a34d9ab4d90925032f5049`, git blob `628645fc…`), holding only the executable |
| Source | <https://raw.githubusercontent.com/conan513/ClientSide/master/Diff_Project/kRO/Sakexe/2008-09-03/2008-09-03aSakexe.rar>. The repository is a git-svn import (commit `bf9f1fa`, 2010-03-18, `git-svn-id: http://subversion.assembla.com/svn/ClientSide@6`) of the "Diff Team" ClientSide SVN that the eAthena community used to publish clean kRO clients and their diff files from 2008 to 2010 |
| Downloaded | 2026-11-24 |

**Why it is stock.** The same directory holds the Diff Team's patch file `2008-09-03aSakexe.diff` (kept beside the executable). Its header `OCRC:14242118` is the CRC-32 of the clean client the diffs were made against, written as its last eight decimal digits: this file's CRC-32 is 33**14242118**. Each of the diff's 1 105 patch bytes names the original byte at its offset; all 1 105 match this file, so none of the community patches (login packet, GameGuard, multiple GRFs, ...) has been applied. The mirror publishes no other checksum.

**Why this one.** The source was built on 2008-11-05, but its packet table matches rAthena `PACKETVER` 20080401 to 20080820 and differs from 20080910 on (technical plan 2.4: requests renumbered from 2008-09-10). 2008-09-03a is the newest clean Sakexe that predates that renumbering, a week after the suggested 2008-08-27 target, and the nearest clean client to it that could be found. Searched and rejected:

- 2008-08-13aSakexe exists only *hexed* (`kROsakexe0813aN.rar`, archived from the old `svn.games-service.net`, also in `github.com/Kelberwitz/RO_Undiff-Client` under `Sakexe/Hexed/2008`): about 30 patches applied. Reverting its Diff Team diff left a CRC-32 that is not the published one, so it cannot be made stock.
- 2008-08-27a: the Diff Team directory has its diff and packet list (`2008-08-27aSakexe.exe.txt`) but no executable.
- 2008-11-13aSakexe.rar (same repository) does not extract completely with libarchive, and its CRC does not match its diff's.
- No 2008 client was found on the Internet Archive, Nemo's client list (from 2010 on) or the kRO patch servers (closed).

## Where it lives

`$BF_REFEXE_DIR`, default `~/Games/Bifrost/refexe/`:

```
2008-09-03aSakexe.exe        the executable (not executable: mode 0644)
2008-09-03aSakexe.rar        the archive as downloaded
2008-09-03aSakexe.diff       the Diff Team's patch list (provenance, OCRC)
SHA256SUMS                   checksums of the three
ghidra/sakexe-2008-09-03a.gpr, .rep/   the Ghidra project, auto-analysed
ghidra/analysis.log          log of the import and analysis
```

To rebuild it on another Mac: download the archive from the URL above, check its SHA-256, `bsdtar -xf 2008-09-03aSakexe.rar`, check the executable's SHA-256, then `port/tools/refexe.sh --setup` (about 100 s of analysis) and `port/tools/refexe.sh --apply-anchors`.

## Tools

- **Ghidra 12.2** (`brew install ghidra`, which brings `openjdk@25`). The project was made by `analyzeHeadless` with the default analysers (Windows PE x86, Visual Studio). Open it in the GUI with `ghidraRun`, File > Open Project, `~/Games/Bifrost/refexe/ghidra/sakexe-2008-09-03a.gpr`. A GUI session locks the project; close it before using the helper.
- **rizin 0.9** (`brew install rizin`) for quick byte and string searches without the project: `rizin -q -c 'iz~login.rsw' 2008-09-03aSakexe.exe`, `rizin -q -c '/x 24437200' ...` (a little-endian address as an immediate).
- **`port/tools/refexe.sh`**, headless queries against the kept project (it runs `port/tools/refexe/RefexeQuery.java` inside Ghidra and opens the project read-only):

```
port/tools/refexe.sh --string "login.rsw"        every function referencing a string (substring; --exact)
port/tools/refexe.sh --addr 0x5c1d71             the function containing an address
port/tools/refexe.sh --func CGameMode::OnUpdate  by name: an anchor below, or Ghidra's FUN_xxxxxxxx
port/tools/refexe.sh --callers 0x5a7940          who calls the function at an address
  --disasm | --both    instructions instead of / as well as the decompilation
  --max N              functions printed for --string (default 8)
```

A query takes 15–40 s (Ghidra's start-up, then the decompiler). Queries are serialised by a lock in the project folder.

## Finding a function

1. **Strings first.** The source's literals are in the executable as they are: error and trace texts (`"Sc_Notify_Ban - login"`, `"No Packet Encryption !!!"`, `"PingLog.txt"`), resource names (`"login.rsw"`, `"sdata.grf"`, `"sclientinfo.xml"`), command-line switches (`"1rag1"`, `"1sak1"`, `"/account:"`), registry keys (`"Software\Gravity Soft\Ragnarok"`). Pick a literal that the source uses in the function in question and nowhere else, and ask `--string`. The game's messages are not literals but `MsgStr(MSI_...)` indices into `msgstringtable.txt`; they show as an index pushed before a call to the message lookup. Korean literals are CP949 (`python3 -c 'print(open(f,"rb").read()[va-0x400000:][:64].decode("cp949"))'`).
2. **Then the call graph.** From a known function go to its callers (`--callers`) or its callees (the decompilation names the anchors). A packet handler is found through its dispatcher: `CGameMode::PollNetworkStatus` is one `switch` over the packet type, its cases written in hex (`case 0x78:` is the source's `case 120:`), each calling or inlining the `Zc_...` handler.
3. **Expect inlining.** Visual C++ 6 inlined small members: `CLoginMode::CheckPingTime` sits inside `CLoginMode::OnUpdate`, `CGameMode::SynchronizeTimeWithServer` and `UnloadRarelyUsedCaches` inside `CGameMode::OnUpdate`, `Sc_Ack_Encryption` inside the login dispatcher. A member function's `this` shows as `in_ECX`, its fields as `*(int *)(in_ECX + 0x14)` (`m_loopCond`).
4. **Name what you found.** Add a line to `port/tools/refexe/anchors.tsv` (address, `Class::Method`, evidence) and run `--apply-anchors`, so the next decompilation shows the name.

## Anchors

The functions located so far, with what makes them that function. They are in `port/tools/refexe/anchors.tsv` and applied to the project.

| Address | Function | Evidence |
|---|---|---|
| 0x697D70 | `WinMain` | Called by the CRT entry (call at 0x6B96F5) with four arguments; first `SetUnhandledExceptionFilter`, then `strstr(lpCmdLine, ...)` over `"1rag1"`, `"1sak1"`, `"1gld1"`, `"2rag2"` as in `src/WinMain.cpp`, `"sdata.grf"`, `"sclientinfo.xml"`, `"/account:"`; ends in `CModeMgr::Run(0, "login.rsw")` |
| 0x5D54A0 | `CLoginMode::OnUpdate` | `CheckSystemMessage`, then `CLoginMode::PollNetworkStatus`, then the inlined `CheckPingTime` (trace `"CharServer Ping"`, packet 0x187 every 12 000 ms in states 7, 8, 11, 19): the order of `src/LoginMode.cpp:283` |
| 0x5D7300 | `CLoginMode::PollNetworkStatus` | The only caller of the login `Sc_Notify_Ban`; `Poll()` failing sends `SendMsg(MM_SOCKETERROR)`, `g_mustPumpOutReceiveQueue` reads 4 bytes, then the `RecvPacket` loop and packet switch; inlines `Sc_Ack_Encryption` |
| 0x5DA610 | `CLoginMode::Sc_Notify_Ban` | References `"Sc_Notify_Ban - login"` (`src/LoginMode.cpp:1640`) |
| 0x57BC90 | `CGameMode::OnUpdate` | The only caller of `CGameMode::PollNetworkStatus`; the static `time_update_start = timeGetTime()`; `m_loopCond` at `this+0x14`; CZ_REQUEST_TIME (0x89) every 12 000 ms and the 15 000 ms cache unload, as in `src/GameMode.cpp:656` and `:2652` |
| 0x5C1C10 | `CGameMode::PollNetworkStatus` | The only caller of `Zc_Restart_Ack`; a 0x5ABC-byte frame (the 20 480-byte packet buffer); `Poll`, `g_mustPumpOutReceiveQueue`, the `RecvPacket` loop and the zone packet switch (`case 0x75`, `0x78`, ...) of `src/GameModePacket.cpp:14924` |
| 0x5A7940 | `CGameMode::Zc_Restart_Ack` | `if (type == 1) { Disconnect(); g_loginStartMode = 5; Switch(0, "login.rsw"); }`, `src/GameModePacket.cpp:3939` line for line |
| 0x592F20 | `CGameMode::SendMsg` | The 31 KB message switch; references `"login.rsw"` in its `MT_LOGIN` switch (`src/GameMode2.cpp:2769`) |
| 0x513BD0 | `CModeMgr::Switch` | Called with `(0, "login.rsw")` by `Zc_Restart_Ack` and `SendMsg` |
| 0x513A70 | `CModeMgr::Run` | Called at the end of `WinMain` with `(0, "login.rsw")` |
| 0x6966F0 | `CMode::CheckSystemMessage` | The first call of both `OnUpdate` functions |
| 0x4194B0 | `CRagConnection::instanceR` | Fetched before every `Poll`/`Recv`/`Send` |
| 0x418AF0 | `CRagConnection::Poll` | Its false result raises `MM_SOCKETERROR` in both dispatchers |
| 0x418BB0 | `CRagConnection::Recv` | `Recv(&aid, 4, true)` under `g_mustPumpOutReceiveQueue` |
| 0x419200 | `CRagConnection::RecvPacket` | The loop condition of both dispatchers |
| 0x419380 | `CRagConnection::GetPacketType` | The switch value inside that loop |
| 0x418B20 | `CRagConnection::Disconnect` | The first call of `Zc_Restart_Ack` |

## Differences seen so far

Found while locating the anchors; each is a replaced or missing site in `src/` that later steps (M6, M8) may need:

- **`CGameMode::OnUpdate` polls the network.** The original calls `CheckSystemMessage(); PollNetworkStatus();` and then enters the `m_loopCond` block. The source calls `CheckSystemMessage(); m_scheduler.OnRun();` instead: the single-player game's scheduler took the place of the network poll.
- **`CGameMode::OnUpdate` keeps time with the server.** Inside the `g_isSkipFrame > 0` block, the original sends CZ_REQUEST_TIME (0x89, `clientTime = timeGetTime()`) when 12 000 ms have passed since `m_syncRequestTime` (`this+0x290`), resetting `m_receiveSyneRequestTime` (`this+0x28C`) and appending to the ping log: that is `SynchronizeTimeWithServer(false)` inlined. The source calls only `ProcessAssignAID(); UnloadRarelyUsedCaches();` there and syncs once, at `OnInit` (`src/GameMode.cpp:521`). Without it rAthena receives no keep-alive from the zone client.
- **The original's `m_loopCond` block calls five functions** (0x57C060, 0x537140, 0x588CC0, 0x58D550, 0x58E460) before the frame-skip logic where the source has `ProcessInput(); ProcessPlayWave();`. Not yet identified.
- **`CGameMode::PollNetworkStatus` starts with a play-time notice.** Before `Poll()`, the original computes the hours since a start time (`(now - start) / 3 600 000`) and, when the hour changes, opens a window (`this+0x2D4`) with `"%d 시간이 경과했습니다."` ("%d hours have passed"), from 3 hours on with `"잠시 쉬었다 하시기 바랍니다."` ("please take a break"): Korea's play-time warning. The source has nothing of it; a general client can leave it out.
- **`WinMain`** of the original ends in `g_modeMgr.Run(MT_LOGIN, "login.rsw")`; the source's single-player `WinMain` has `g_modeMgr.Run(MT_GAME, "World_HP.rsw")` (`src/WinMain.cpp:1016`).
