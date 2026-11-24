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
- Before restarting: remove the round-1 worktrees and `pkg/*` branches, or the workers cannot create theirs.
