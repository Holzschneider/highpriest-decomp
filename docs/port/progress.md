# Progress log

Entries are added at every milestone exit, for every parked item, for every deviation from the plans, and when the run stops. Newest last. The state of each step is in [checklist.md](checklist.md).

## 2026-11-24: run set up

- The run's rules (`CLAUDE.md`), prompt ([run-prompt.md](run-prompt.md)), checklist and allowlist were written from the owner's decisions of this day (specification, section 9).
- Deviations from the plan as first written, all by the owner's decision:
  - Tracks run ahead of the milestone order; milestone exits still close in order.
  - M1 gets a step 0: the port's prefix `HP`/`hp` becomes `BF`/`bf`.
  - Reference frames stay outside git; the repository holds checksums and verdicts.
  - Protocol differences are resolved in the client only; the test server stays stock.
  - M12 in the run is the application bundle with its settings window and the Granny and Bink readers; the rest of the former M12 list is future work.

## 2026-11-24: run stopped by the owner

- Stopped during round 1, a few minutes after its 16 packages started. Nothing from round 1 was integrated; the checklist is accurate (only M1.0 is done).
- Left behind, outside git: empty worktrees `build/wt/<package>` with branches `pkg/<package>` (no commits), a draft `port/data/fetch.sh` in `build/wt/data`, and a partial copy of `data.grf` (801 MB) in the game folder. The test server is stopped with its database kept.
- The round-1 worktrees and `pkg/*` branches were removed afterwards at the owner's request, the draft `fetch.sh` with them. The partial `data.grf` stays.

## 2026-11-24: run restarted, round 2

- The owner restarted the run with the run prompt. Session on Opus 5.5, effort high, as pinned.
- Round 2 fans out 16 packages, every step whose prerequisites are done: `m1-remove` (M1.1–M1.5, the serial `src` track, one commit per step), `refexe` (M1.6), `winheaders` (M2.1), `layout` (M2.8), `sys` (M3.1 native part, M3.3, M3.4), `codec` (M3.6), `data` (M3.8–M3.10), `app` (M4.1), `gfx` (M4.2), `text` (M4.5), `refshots` (M4.8), `harness` (M5.7), `audio` (M7.1, M7.2), `bot` (M8.2), `granny` (M12.3), `bink` (M12.4). Mechanical or collecting work (`codec`, `refshots`) runs on Sonnet 5.5, everything else on Opus 5.5.
- Decisions taken by the lead:
  - The lead copied `data.grf`, `rdata.grf`, `BGM/` and ROenglishRE into the game folder before the round, so that the data, Granny and Bink packages could start at once; `data` writes the repeatable fetch script.
  - The application name and bundle identifier are defined in `port/name.cmake`; `flags.sh` reads it and passes `BF_APP_NAME` to the compiler.
  - The test server logs zeny, chat, MVP drops and branches to SQL (`port/server/import/log_conf.txt`), configuration only, so that scripted play can be confirmed there.
  - The control protocol is specified by the `harness` package (`port/native/control/PROTOCOL.md`) together with a mock client, so the harness is built before the client can run; the control module (M4.6, M5.6, M6.4) implements that document.
  - Workers do not restart the shared test server. Server configuration that a package produces (the enterable-map list of M3.10) is applied by the lead at integration.
- Found while briefing: the intro movie `openning.bik` that `CBink.cpp` opens is not in the 2021 data; the archive holds one other Bink file (`data\video\ad\01.bik`). The Bink package looks for the intro elsewhere and decodes what exists.

## 2026-11-24: round 2, first integrations

- Integrated: M2.1 (Wine 11.0 headers vendored, with their C runtime headers after one send-back), M2.8 (layout check), M3.1, M3.3, M3.4 (`sys`), M3.6 (`codec`), M3.8 to M3.10 (game folder, table checker, enterable maps), M4.1 (`app`), M4.5 (`text`).
- Census on `src/` with the vendored headers: 60 of 152 units clean, 3,215 distinct error locations (3,226 with the old headers).
- Deviations:
  - M2.2 also narrows the `time_t` fields of `packet.h` to 32 bits. The layout check found that eight packet records still differ on the rewritten tree only through them (Visual C++ 6.0 had a 32-bit `time_t`).
  - The loose `data/` folder holds, besides the missing tables, the pre-renewal `.rsw`/`.gnd`/`.gat` of eight maps (prontera, prt_fild05, prt_fild08, prt_in, prt_church, izlude, alberta, morocc), copied unchanged from ROenglishRE. The 2021 archive has their renewal layout, which differs from the pre-renewal map cache the server walks by thousands of cells. This follows the rule that the client adapts to the server; the files stay outside git.
  - `msgstringtable.txt` comes from ROenglishRE's renewal folder, which is the only one with that table; its first 1,387 entries match the client's message ids one to one.
  - Maps removed from the test server: 294 of 1,265 (77 with world files too new, 193 with no world file, 24 PvP rooms whose aliases point at the replaced cities). Applied by the lead at the next server restart.
- Untested so far, to be retried in a logged-in desktop session: the Keychain round trip (the login keychain is locked to this session), native fullscreen and visibility events (the screen is locked).
- M12.4 (Bink reader) integrated. `data\video\ad\01.bik` (BIKi, 450 frames) decodes completely and matches FFmpeg 7 used as an outside reference; both audio codecs decode, tested on public sample movies downloaded to the test folder (outside git). Lead's decision: the quantiser tables stay derived from the base matrices, as written, rather than copied from FFmpeg's precomputed tables; the cost is 20 luma pixels in 300 million that differ by one level. The intro `openning.bik` was not found in the game folder or the data host's installs; whether it is inside the kRO 2021 installer archive is open and is checked for M12.4a.
- M1.6 (reference executable: stock `2008-09-03aSakexe.exe`, unpatched by the Diff Team's CRC, Ghidra project outside git) and M7.1, M7.2 (`audio`) integrated. The reference shows the original `CGameMode::OnUpdate` polling the network and sending a time sync every 12 s; both were passed to the recovery package. The audio worker found map music hard-coded to `bgm\08.mp3` at three sites (`GameMode.cpp`, `GameMode2.cpp`), which the symbol-based inventory cannot see; also passed to the recovery package.
- The kRO 2021 installer archive (copied to `~/Games/Bifrost/sources/kro/`) holds no `.bik` either: `openning.bik` is in none of our data.
- M12.3 (Granny reader) integrated: all 21 `.gr2` files decode (Oodle0 sections to their exact sizes, 17,383 pointer fix-ups in range), and every mesh draws as the right object. Textures (stored in RAD's Bink texture encoding) and the evaluation of animation curves are not done; the lead added step M12.3b for them (a deviation: the plan's M12.3 did not separate them). Provenance, decided by the lead: the worker learned Oodle0 from blendergranny's `oodle0.py` (MIT, declared clean-room) and wrote its own code, which the spec allows ("may be read to understand a format"); of granny-ro-js, which says it consulted leaked SDK source, only the README and type declarations were read, and nothing of it is used.
- M4.2 (`gfx`) integrated: 78 numeric checks pass; the contact sheet of synthetic test draws (committed, checksummed) shows clean blocks at UI scale 2 and 3, exact half-pixel quads, the 16 multitexture presets and the blend pairs in use. Found: the client never sets point sampling, so nearest sampling for UI is chosen by texture kind (UI tiles); game data uses only presets 0 and 1; the Metal shader compiler is not installed, so shaders compile at run time from embedded source. Open: fog colour (the only code that sets it is never called).
- Deviation: M4.3 is split. The native half (app and gfx follow the window, one place computes logical size and scale) runs now; the client half becomes M4.3a, which needs the Direct3D adapter (M4.4).
- M12.2 (`prefs`) integrated: folder validation and order, the settings window's controls and their persistence are tested; the modal folder panel, the Choose sheet and Forget Saved Password need a person or an unlocked session and are on the list for the owner's hand test.
- M8.2 (`bifrost-bot`) integrated: its self-test plays three bots (whisper and chat echo, party invite, a 1-zeny trade, a guild invite) and finds every round trip in the SQL logs, 31 checks. It confirms the `time_t` fault from the server side: `0x2d4` ZC_ITEM_PICKUP_ACK3 arrives as 29 bytes, the client's table says 33. The bots keep test characters BfBotA, BfBotB, BfBotGM on the server.
- Server restarted with the enterable-map list (971 maps of 1,265) and the Job Master; `bifrost-netprobe` still ends `OK`.
