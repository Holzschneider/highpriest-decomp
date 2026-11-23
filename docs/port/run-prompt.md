# The run prompt

The whole port, M1 to M12, is run by one prompt. It is the same for the first start and for every restart: the state of the run is in the repository, not in a conversation.

## The prompt

Start Claude Code in the repository root on the branch `macos-port` and paste:

```text
Run the macOS port to its end.

Read CLAUDE.md, docs/port/spec.md, docs/port/checklist.md and the last entries of
docs/port/progress.md. Then work as the lead that CLAUDE.md describes: take every open
step whose prerequisites are done, in all milestones at once, hand the work to
sub-agents, integrate what they return, and have a fresh verifier close each milestone.

I authorise multi-agent orchestration for this: use workflows and fan out agents at
whatever scale you can integrate, for the whole run, without asking again.

Do not ask me questions. Decide from the spec and the plans and log what you decided.
Park what stays stuck and go on. Stop only for the reasons CLAUDE.md lists under
"Stopping", and notify me when you do.
```

## What it relies on

| File | Role |
|---|---|
| [`CLAUDE.md`](../../CLAUDE.md) | The standing rules: roles, how a round goes, integration, verification, stuck work, stop conditions, fences, conventions. Reloaded in every session and sub-agent, so it survives context compaction |
| [`spec.md`](spec.md) | What the client must be. Section 9 records the decisions about the run itself |
| [`implementation-plan.md`](implementation-plan.md) | The milestones, their steps and exit criteria |
| [`checklist.md`](checklist.md) | Every step as open, done or parked: where a restarted session finds its place |
| [`progress.md`](progress.md) | Milestone exits with the verifier's verdicts, parked items, deviations, and the reason for any stop |
| `.claude/settings.json` | The allowlist: build tools, git on `macos-port`, the test server's containers, Homebrew, downloads. A command outside it waits for approval |
| `.claude/settings.local.json` | Machine-specific permissions (the game folder, read-only copies from the other Mac). Not in git; see below |

## Before the first start on a machine

- Create `.claude/settings.local.json` with the game folder as an additional directory and the copy commands for the machine that holds the game data:

  ```json
  {
    "permissions": {
      "additionalDirectories": ["/Users/<you>/Games/Bifrost"],
      "allow": ["Bash(rsync *)", "Bash(ssh <data-host> ls *)", "Bash(scp <data-host>:*)"]
    }
  }
  ```

- Docker must be running; the test server is started by the run.
- For an unattended run, these Claude Code settings help and are the owner's to set (`/config`): continue automatically when a usage limit resets, and push a notification when a permission prompt is waiting.

## How to read the run from outside

- `docs/port/checklist.md` shows how far it is.
- `docs/port/progress.md` ends with either the latest milestone exit or the reason the run stopped.
- `git log macos-port` has one commit per completed step.
- To continue after a stop: resolve what the log names, then paste the prompt again.
