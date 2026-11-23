# macOS port: specification

What the finished client is and how the work is run, as decided with the project owner on 2026-11-23. The [technical plan](technical-plan.md) explains how it is built and the [implementation plan](implementation-plan.md) in which order; where they and this document disagree, this document holds.

## 1. Product

A general Ragnarok Online client, recovered from the HighPriest source, running natively on Apple Silicon Macs.

| | Decision |
|---|---|
| Users | A person playing comes first. An agent driving the client (as in the owner's aiRO project) comes later as a first-class use; no design decision may rule it out (section 6) |
| Platforms | arm64 only, macOS 14 or later. No Intel build, no iPad requirement |
| Dependencies | The shipped application links Apple frameworks and zlib only. Formats Apple cannot decode (Bink, Granny) get their own minimal reader or stay stubbed |
| Name | A neutral working name, defined in one place, until the owner chooses one. Not "HighPriest", not "Ragnarok" |
| Single-player game | Removed. The reference commit on `main` preserves it |

## 2. Recovery

- **Standard:** restored code must behave correctly against rAthena. The surrounding code and the protocol are the first evidence; the stock 2008 executable is consulted only where a site is unclear. Small deviations from the 2008 original are acceptable; an archival reconstruction is not the aim.
- **Protocol:** the 2008-04 to 2008-08 packet set. The seams for a later version are prepared: packet ids and lengths go through one table and handlers carry no hard-coded ids, so a second version is additive. No second version is implemented.
- **Conveniences**, each a named option in the settings file:

| Option | Default |
|---|---|
| Skip the licence and intro screens | on |
| Free camera: no limits on zoom distance and vertical rotation | on |
| Launch arguments for server, account and character, going straight into the game | on |
| Attack without Ctrl | off, as in the original (`/noctrl` still works) |

  Anything not listed behaves as the 2008 client did.

## 3. Picture and window

- **World at native resolution, UI in pixels.** The 3D world is rasterised at the display's native resolution. The UI keeps its original pixel art and bitmap-style text and is drawn at an integer scale of 1×, 2× or 3×. Text is not smoothed.
- **Freely resizable window**, any size and aspect, and native fullscreen. The client sees a logical resolution of the window's pixel size divided by the UI scale, so a wider window shows more of the world. Saved UI window positions are clamped back into view after a resize.
- **Frame pacing:** one frame per display refresh, up to 120 Hz, with game logic at its fixed 60 steps per second.
- **In the background** the client keeps running at full rate, drawing included, so the connection never stalls and frames can still be captured.
- **Single instance.** A second launch brings the first to the front. Settings access still goes through one object that could be keyed by profile later.

## 4. Input

- **Pointer:** the game's cursor sprite is drawn where the macOS pointer is. The pointer is never captured and leaves the window freely. Camera rotation by right-drag uses movement deltas while the button is held.
- **Keyboard:** Mac-adapted defaults.
  - Option acts as Alt and types no character when a game shortcut matches.
  - F1–F12 reach the game directly wherever macOS allows it.
  - Insert (sit) has a substitute key.
  - Cmd+Q, Cmd+F (fullscreen) and Cmd+V keep their Mac meaning.
  - No user-editable key map for now.

## 5. Data, text and files

- **Game data:** the 2021 kRO archives, read as they are. An archive from 2008–2010 is used if one is found; nothing waits for it.
- **Game folder is read-only.** The client is pointed at a folder holding the archives and never writes there.

| Written | Where |
|---|---|
| Settings, UI window positions | `~/Library/Application Support/<name>/` |
| Screenshots | `~/Pictures/<name>/` |
| Chat logs and reports | `~/Documents/<name>/` |
| Account password, if "save password" is used | macOS Keychain |

  The account name is remembered in settings. Scripted runs take credentials from the command line or the environment.
- **Text:** Windows-1252 in chat, names and on the wire: English and Western European letters. Korean typed by others is not decoded.
- **Names:** item, skill and message tables come from ROenglishRE's pre-renewal tables, unchanged. Where a server message uses another name, the difference stays.
- **Missing data:** an obvious placeholder is drawn (a magenta texture, a default sprite, `?name`), the miss is counted in the trace, and the client never stops for it. No error boxes for absent files.

## 6. Control interface

The client can be driven and observed from outside. Scripted tests use it now; agents use it later.

- A Unix-domain socket, opened only when the client is started with a flag. Commands and events are JSON, one object per line. Any language can connect.
- First version, needed for the tests of this plan: inject key and pointer events, capture a frame to a file, and query state (mode, own position, HP and SP, visible actors, open UI windows).
- What must stay possible for first-class agent use, although it is not built yet:
  - Input reaches the client through one event queue, fed by AppKit and by the socket alike.
  - The renderer can draw without a visible window.
  - The loop can be advanced a fixed number of logic steps on request.
  - Nothing reads the clock, the pointer or the keyboard behind the back of the native modules.

## 7. Testing and acceptance

- **Scripted play decides.** Milestone criteria that need playing are scripts that drive the client through the control interface. A step passes when the test server's SQL logs show the action and the captured frames look right. The owner plays by hand at the end, not between milestones.
- **Second player:** a headless bot grown from `hp-netprobe` logs in a second character and answers trades, party invites and chat at packet level. The real client stays single-instance.
- **Server:** only the rAthena in `port/server` on the development Mac. No other server is used, changed or deployed to.

## 8. How the work is run

- **Branches:** `main` stays exactly at the original commit and keeps its byte-identical Windows build. All port work, including what was done before this document, lives on the branch `macos-port`, which never merges back.
- **Commits:** one per completed step, saying what changed and why.
- **No stops between milestones.** Work continues as long as exit criteria pass. It stops when blocked, or when something needs the owner's eyes or hands.
- **Progress log:** `docs/port/progress.md` gets an entry at every milestone exit: what passed, the commands that show it, the captured frames, open recovery sites, and deviations from the plan.
- **Pre-approved without asking again:**
  - Copying game data and translation tables, read-only, from the owner's other Mac to a client folder on the development Mac.
  - Downloading a stock 2008 client executable from a community mirror, kept outside git and never run.
  - Pushing `macos-port` to the GitHub remote.
  - Installing build tools with Homebrew.
- **What goes into git:** port code, tools, documents, and the Wine headers the build reads, with their licence notice. Never game data, the reference executable, or translation tables; those are fetched or copied by script.
