# Bifrost: the macOS port run

This branch (`macos-port`) turns the reconstructed HighPriest source into a general Ragnarok Online client and ports it to macOS as a native arm64 application, working name **Bifrost**. The work is done by an unattended, multi-agent run that the owner starts with the prompt in [docs/port/run-prompt.md](docs/port/run-prompt.md) and restarts with the same prompt after any interruption. This file holds the standing rules of that run. It is loaded into every session and every sub-agent, so it applies to whoever is reading it.

## Where the truth is

| File | What it is | Who changes it |
|---|---|---|
| `docs/port/spec.md` | What the client must be and how the work is run. Binding. | The owner only. A conflict with it is a stop (see "Stopping"). |
| `docs/port/implementation-plan.md`, `technical-plan.md` | Milestones M1–M12, their steps and exit criteria; the design. | The lead may amend them when they prove wrong; every amendment is logged as a deviation in the progress log. |
| `docs/port/checklist.md` | The state of the run: every step open, done, or parked. | The lead, with each integration. |
| `docs/port/progress.md` | The log: milestone exits with the verifier's verdict, parked items with their evidence, deviations. | The lead. |
| `port/README.md` | The tools, probes, test server and how to run them. | Whoever changes a tool. |

A session finds its place by reading the checklist and the tail of the progress log. Nothing about the state of the run lives only in a conversation.

## Roles

- **Lead**: the session that received the run prompt. It plans rounds, hands out work packages, integrates results, and is the only writer of `macos-port`. It runs on Opus 5.5 (`claude-opus-5-5`) at effort high.
- **Workers**: sub-agents, usually started through workflows. Each gets one work package and returns a local branch plus a short report. The owner authorised orchestration without a size cap; the lead sizes each round by what can be integrated, not by what can be started.
- **Verifier**: a fresh agent on the lead's model that did none of the work. A milestone closes only on its verdict.

Models: compiler-checked mechanical sweeps (loop-scope fixes, cast widening, stub generation, string extraction, renames) go to a smaller model (`sonnet`). Design and debugging (renderer, recovery sites, protocol faults, format readers) stay on the lead's model.

**Ceiling, set by the owner:** nothing in the run uses a model above Opus 5.5 or an effort above high. No agent, workflow agent or verifier is started with `model: "fable"` or any other model above Opus 5.5, or with an effort of `xhigh` or `max`. Omitting the model (inherit Opus 5.5) or choosing `sonnet` or `haiku` is allowed. `.claude/settings.json` pins the session model, limits the selectable models and caps the effort; if a session finds itself on a higher model, it stops and notifies the owner instead of working.

## A round

1. Read the checklist. Pick every open step whose prerequisites are done, across all milestones: the tracks are free to run ahead of the milestone order (native modules, data, harness, format readers and tools do not wait for the client to compile). Milestone *exits* still close in order.
2. Write a brief for each package (see "Briefing a worker") and fan them out.
3. Integrate what comes back, one package at a time: rebase onto `macos-port`, build, run the fast check, commit, tick the checklist. A package that fails is sent back once with the failure; see "Stuck work".
4. When all steps of a milestone are done or parked, start a verifier. On its verdict, write the progress entry and push.
5. Start the next round. Do not end the turn while an open step can move.

The fast check is whatever the tree can do at that point, and grows with it: the compile census, then the build, then the headless start, then the login script. `port/README.md` names the current command; keep it under a few minutes.

## Integration

- Workers never commit to `macos-port` and never push. Each works in its own worktree, created with `git worktree add build/wt/<package> -b pkg/<package> macos-port` (the `build/` directory is ignored by git). Do not rely on automatic worktree isolation: it may branch from `main`, which does not contain the port.
- New code that nothing else touches yet (a native module with its own test program, a tool, the test harness) can be developed fully in parallel. Changes to `src/` conflict easily, so the lead keeps packages on `src/` disjoint by file, or runs them one after another.
- A tree-wide mechanical rewrite (a codemod, a rename) runs alone: no other package on `src/` is out while it runs, and it is committed by itself, separate from any hand-written change, so that it can be reviewed and redone.
- One commit per completed step, saying what changed and why. Push `macos-port` after each milestone exit and at least once per round. Never force-push; never touch `main`.

## Briefing a worker

A worker knows only its brief and this file. A brief states: the goal and what it is for; the step ids it covers; the files it owns and the files it must not touch; the interface it has to meet or may define; what is already known or ruled out, with pointers to the documents instead of retyped content; the command that shows the package works; and what to return (branch name, what was verified and how, what was left open). Ask for evidence, not for "done": a claim like "builds clean" comes with the command and its last lines.

Native modules are written to the technical plan (section 3.3) before the client can exercise them. Each therefore ships a standalone test program, and its header is the contract: when the client's call sites later disagree with it, the lead changes the module, not the client.

## Verifying a milestone

The verifier gets the milestone's "Done when" list and the commit, nothing else: no summary of what was done, no claim that it works. It checks out the commit in a clean worktree, builds, reruns every command, and looks at the captured frames itself. For frames it uses its own judgement (sprites whole, text legible, no placeholder magenta, layout sane) and the public screenshots of 2008-era clients collected under `~/Games/Bifrost/test/reference/`. It returns pass, fail or parked per criterion with the evidence. A criterion is never reworded to make it pass.

## Stuck work

- A package that fails its check three times is **parked**: the checklist marks it, the progress log gets the evidence and what was tried, and the run continues with everything that does not depend on it. Parked items are retried when later work makes them easier, and listed in the final report.
- An exit criterion that does not pass is parked the same way; the milestone is then closed as "closed with parked items", not as passed.
- **Stall detector:** if three consecutive rounds close no checklist step, stop and report.

## Stopping

Stop, write the reason at the end of the progress log, and send the owner a push notification, when:

- the stall detector fires;
- a step cannot be done without contradicting `docs/port/spec.md`;
- everything left is parked or done (then write the final report: what passed, what is parked, what the owner should try by hand);
- a command is needed that the allowlist in `.claude/settings.json` will not pass. Notify first, then issue it, so the owner knows why a session is waiting.

Nothing else is a reason to stop or to ask. Open design questions are decided from the spec and the plans, and logged.

## Fences

- **Writes** go to this repository, to `~/Games/Bifrost` (game folder and test artefacts), to the application's own folders under `~/Library/Application Support/Bifrost`, `~/Pictures/Bifrost`, `~/Documents/Bifrost`, and to the scratchpad. Nowhere else.
- **No `sudo`, no changes to system settings.** Build tools are installed with Homebrew.
- **Server:** only the rAthena in `port/server` on this machine, started with `docker compose`. It stays stock rAthena at `PACKETVER=20080401`, changed by configuration only. Where client and server disagree, the client adapts, even where that departs from the 2008 original.
- **The owner's other Mac** is a source of game data and nothing else: copy from it (`rsync`, `scp`), never write to it, never start, stop or change anything there. Its name and the names of the owner's other projects stay out of committed files.
- **Never in git:** game data, captured frames that show game artwork, the reference executable, translation tables. Frames live under `~/Games/Bifrost/test/`; the repository holds their checksums, the script that regenerates them, and the verifier's verdicts. Synthetic test draws made from generated textures may be committed.
- **The reference executable** (a stock 2008 client) is for reading in a disassembler. It is never run.
- **Third-party code:** the application links Apple frameworks and zlib only. The Granny and Bink readers are written from public format descriptions and from the game's own files; open-source decoders may be read to understand a format, but no code is copied.
- **This machine is unattended** during the run: tests may take focus, play sound and open windows. Input still reaches the client through its control socket, not through the real pointer or keyboard, so that tests are repeatable.

## Conventions

- **Names:** the application name is `Bifrost`, defined once in the build and never hard-coded in a path. The port's own identifiers use `BF_` (build switches, environment variables: `BF_PORT`, `BF_TRACE`, `BF_CLIENT_DIR`) and `bf_` (functions, files); programs are named `bifrost-<name>`. After M1, any `HP_` left in the source is a leftover of the single-player game and a defect.
- **The client source** (`src/`) keeps its style and its structure: the smallest change that works, no reformatting, no modernising. New code lives under `port/` (`port/native` for modules that stay, `port/compat` for Windows-shaped adapters that M11 deletes).
- **Tests:** scripted play through the control socket, confirmed in the server's SQL logs. Game-master commands may set a scene up (place, level, equip a character); the action under test is always played through the client.
- **Missing data** never stops the client: placeholder, counted trace line, continue.
- **Commit messages** end with the attribution lines the session provides.
