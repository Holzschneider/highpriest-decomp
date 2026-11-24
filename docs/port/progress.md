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
