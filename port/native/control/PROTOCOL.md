# Control interface, protocol version 0

The contract between the client's control module (`port/native/control`, steps M4.6, M5.6, M6.4) and whatever drives the client from outside: the test harness in `port/harness` now, agents later (spec section 6, technical plan 3.8). The harness's mock (`port/harness/mock_client.py`) serves this protocol too, so every rule here is exercised by `python3 port/harness/run.py --self-test` before the client can run. When the client and this document disagree, the client is changed.

## 1. Transport

- **Opened only on request.** The client listens when it is started with `--control <path>`, or when the environment variable `BF_CONTROL` names a path; the flag wins. Without either, no socket exists.
- **Unix-domain stream socket** at that path. The client removes a stale file at the path before binding, and removes the socket when it exits. macOS limits the path to 103 bytes; the client refuses a longer one with a trace line and runs without the socket.
- **One connection at a time.** A second connection while one is open receives one line, `{"event":"error","code":"busy"}`, and is closed. After the first connection closes, the next one is accepted. The client never waits for a connection: it starts and runs whether or not anyone connects.
- **Framing:** UTF-8 JSON, one object per line, each line ended by `\n`. A line holds exactly one JSON object; JSON escapes keep newlines out of strings. A line may be up to 1 MiB.
- **Threads:** the socket is read and written off the client's main thread, but every command is carried out on the main thread between logic steps, and every input event enters the client's one event queue, the same queue AppKit feeds (spec section 6). Nothing in this protocol reaches the client behind that queue.

## 2. Messages

Three shapes, told apart by their keys.

| Shape | Direction | Keys |
|---|---|---|
| request | to the client | `"id"` (integer or string, chosen by the sender), `"cmd"` (command name), and the command's arguments as further keys |
| reply | from the client | `"id"` (copied from the request), `"ok"` (`true` or `false`), `"seq"` (see below), and the result as further keys, or `"error"` |
| event | from the client | `"event"` (event name), `"seq"`, `"t"`, and the event's fields; never `"id"` or `"ok"` |

```json
{"id": 7, "cmd": "pointer", "action": "click", "x": 320, "y": 240}
{"id": 7, "ok": true, "seq": 41, "x": 320, "y": 240}
{"event": "mode", "seq": 42, "t": 18234, "mode": "charselect", "previous": "connecting", "detail": "LSM_SELECTCHAR"}
```

- **Telling them apart:** a message from the client with `"ok"` is a reply, one without it is an event. Events never carry `id` or `ok`; a field holding an object's id is named after the object instead (`wid` for a window).
- **Order.** Requests on a connection are carried out one after another, in the order received; replies come in the same order. A `wait` holds back the requests behind it until it completes. Events are sent whenever they happen, also between a request and its reply.
- **`seq`** numbers the events of a connection, from 1. A reply's `seq` is the number of the last event sent before it, so a script knows exactly which events came before and after a command (see `wait`, `after`).
- **`t`** is the client's clock in milliseconds since the client started, read from the one clock of the `sys` module.
- **Errors.** A failed request is answered with `"ok": false` and `"error": {"code": "...", "message": "..."}`; `message` is for people and may change. A line that is not a JSON object, or has no `cmd`, is answered with `"id": null` (or its `id` if one could be read) and the code `bad_request`; so is an `id` that is neither an integer nor a string.
- **Shutdown.** A request the client has not finished when it shuts down (a `key` `press` of `cmd`+`q`, a `wait`) gets no reply: `bye` is the last line before the socket closes.

| Code | Meaning |
|---|---|
| `bad_request` | malformed line, missing argument, argument of the wrong type or out of range |
| `unknown_command` | no command of that name |
| `unsupported` | the name is reserved (section 5) or the command exists but this build cannot do it (for example `capture` with no renderer) |
| `bad_key` | key name not in the list of section 4 |
| `bad_text` | text with a character that Windows-1252 cannot represent |
| `bad_state` | not possible now, for example `capture` before the first frame or `resize` in fullscreen |
| `timeout` | a `wait` ran out of time; the reply also carries the fields of the state that was being watched |
| `io` | a file could not be written |
| `internal` | anything else; the client's trace says more |

- **Extending.** Receivers ignore keys they do not know. Adding a command, an event, an argument with a default, or a result field keeps the version; changing or removing one raises it. `hello` tells both sides the version.

## 3. Commands

Coordinates are **logical pixels**: the window's pixel size divided by the UI scale, origin at the top left of the window's content, integers. They are the coordinates the client's own UI and picking use. Durations are milliseconds.

### `hello`

Arguments: `protocol` (integer, the highest version the sender speaks; optional), `name` (string naming the sender, for the trace; optional).

Result: `protocol` (0), `client` (the application name from the build), `version` (the client's build string), `pid`, `commands` (list of the command names this build carries out), `events` (list of event names it sends). A sender that does not speak the client's version closes the connection.

### `key`

Arguments: `action` and, per action:

| `action` | Further arguments | Effect |
|---|---|---|
| `down`, `up` | `key`, `mods` (optional) | the key goes down or up |
| `press` | `key`, `mods` (optional) | down, then up in the next logic step |
| `text` | `text` | each character is delivered as a typed character, as the system's text input would deliver it, one per logic step |

`key` is a name from section 4. `mods` is a list of `shift`, `ctrl`, `alt`, `cmd`; the modifiers are held for the duration of the event, as if pressed before it and released after: they go down in the logic step of the key's event, before it, and up after the key's last event (`press`: in the step of the key-up; `down`, `up`: in the next step). A modifier already held (by an earlier `down`) is left as it is. Option is `alt` (spec section 4). Key events from the socket go through the same handling as keyboard events from AppKit: `cmd`+`q` quits, `cmd`+`f` toggles fullscreen, `cmd`+`v` pastes. `text` takes any string whose characters are printable Windows-1252 characters; others, control characters included, are refused as a whole with `bad_text`. It does not press Enter (Enter, Tab and Backspace are keys).

Result: nothing beyond `ok`. The reply is sent once the client has taken the last of the events from its queue, so a following `state` sees their direct effect (a character in an edit field, a window opened by a key).

### `pointer`

Arguments: `action`, and `x`, `y` (optional: without them the pointer stays where it is), `button` (`left`, `right` or `middle`; default `left`), `dy` (for `wheel`).

| `action` | Effect |
|---|---|
| `move` | the pointer moves to `x`, `y` |
| `down`, `up` | moves to `x`, `y` if given, then the button goes down or up |
| `click` | moves, then down and up in consecutive logic steps (with `x`, `y`, the move is a logic step of its own before the button, for `down`, `up`, `doubleclick` and `wheel` too) |
| `doubleclick` | two clicks within the double-click interval |
| `wheel` | moves, then the wheel turns by `dy` notches; positive is away from the user (the Windows sign) |

A drag (right-drag rotates the camera) is `down`, one or more `move`, `up`. Positions outside the window are allowed: the pointer leaves the window as the real one does (spec section 4).

Result: `x`, `y`, the pointer's logical position after the action. The reply is sent once the client has taken the events from its queue, as for `key`.

### `capture`

Arguments: `path` (absolute path of the PNG to write; its directory must exist).

The next completed frame (the first frame finished after the request) is written as an 8-bit RGBA PNG at the size of the drawable, in device pixels (the logical size times the UI scale, on a Retina display times the backing factor as well). The client writes the file and nothing else.

Result: `path`, `width`, `height` (device pixels), `scale` (UI scale), `logical` (`{"w": ..., "h": ...}`), `frame` (the number of the frame captured), `sha256` (hex digest of the file's bytes). A relative `path` is `bad_request`, a missing directory `io`; when no frame is completed within 20 seconds the answer is `bad_state`.

### `state`

Arguments: `fields` (optional list naming the parts wanted; default all).

Result: the parts below. A part that does not apply in the current mode is `null` (`self` and `map` outside game mode). Lists are empty, never absent. A part the client does not provide (yet) is left out of the reply, as are names in `fields` that are not parts; a `wait` whose condition needs such a part answers `unsupported`.

| Part | Content |
|---|---|
| `mode` | one of `start`, `login`, `connecting`, `servers`, `charselect`, `makechar`, `loading`, `game`, `quitting` |
| `detail` | the client's own name for the mode, such as `LSM_SELECTCHAR` or `MT_GAME`, for the trace; not to be relied on |
| `frame` | frames drawn so far |
| `view` | `width`, `height` (window content, device pixels), `scale` (UI scale), `logical` (`{"w", "h"}`), `fullscreen`, `focused` |
| `pointer` | `x`, `y` (logical), `buttons` (list of buttons held), `cell` (`{"x", "y"}`, the map cell under the pointer as picking finds it, or `null`) |
| `map` | `name` (without extension, for example `prontera`), `width`, `height` (cells) |
| `self` | `id` (account id, as the server's packets use it), `char_id`, `name`, `job` (job id), `base_level`, `job_level`, `x`, `y` (cell), `dir` (0–7), `moving`, `dest` (`{"x", "y"}` or `null`), `hp`, `max_hp`, `sp`, `max_sp`, `zeny`, `weight`, `max_weight` |
| `actors` | visible actors other than self: `id` (the server's id), `type` (`pc`, `npc`, `monster`, `item`, `portal`, `skill`, `pet`, `homun`, `merc`, `other`), `name` (as displayed, `?name` if unknown), `job` (sprite or class id; item id for `item`), `x`, `y` (cell), `screen` (`{"x", "y"}`, logical position of the actor's feet on screen, or `null` when off screen) |
| `windows` | open UI windows, front to back: `id` (the client's window id, `WID_*` value), `name` (the `WID_*` name without the prefix, for example `LOGINWND`), `rect` (`[x, y, w, h]`, logical), `focus` (holds the keyboard), `controls` |
| `controls` (in a window) | what can be clicked or typed into, as far as the client exposes it: `id` (stable name within the window, for example `ok`, `cancel`, `id_edit`, `slot:0`), `kind` (`button`, `edit`, `list`, `item`, `check`, `text`), `rect` (logical), `text` (caption, or the content of an edit field; password fields report `*` per character), `focus` |

Text in `state` and in events is UTF-8, decoded from the client's Windows-1252 bytes; bytes that do not decode (Korean typed by others) appear as U+FFFD.

### `wait`

Arguments: `until` (a condition), `timeout` (default 10000, at most 600000), `after` (an event `seq`; only events after it count; default: the `seq` of the reply to the previous request).

Conditions, one key each; several keys in one object must all hold:

| Condition | Holds when |
|---|---|
| `{"mode": "game"}` | the mode is the one named |
| `{"map": "prontera"}` | game mode on that map |
| `{"window": "MAKECHARWND", "open": true}` | that window is open (`open: false`: closed) |
| `{"position": {"x": 150, "y": 150}}` | own cell is that cell |
| `{"moving": false}` | own character has stopped |
| `{"frames": 5}` | that many more frames have been drawn |
| `{"event": "chat", "contains": "text"}` | an event of that name arrived after `after`; `contains` (optional) is a substring of its `text`; other keys in `match` (optional object) must equal the event's fields |

Result: `met` (the condition), `event` (the matching event, for event conditions), and the `mode` and `frame` at that moment. On timeout the reply is the error `timeout`, with `mode` and `frame` as well.

### `resize`

Arguments, each optional: `width` (320–8192), `height` (240–8192) (window content in device pixels), `scale` (UI scale 1, 2 or 3), `fullscreen` (boolean); out of range is `bad_request`.

The window and the logical resolution change as a resize by the user does: renderer, projection, picking and window bounds are updated and saved window positions are clamped (spec section 3). The reply is sent when the next frame has been drawn at the new size. Result: `view` as in `state`.

### `quit`

No arguments. The reply comes first; then the client sends `bye` and shuts down as on Cmd+Q, exiting with status 0.

## 4. Key names

Physical keys, named after the US layout regardless of the keyboard's layout. Characters are typed with `key` `text`, not with key names.

| Group | Names |
|---|---|
| letters | `a` … `z` |
| digits | `0` … `9` |
| function | `f1` … `f12` |
| editing | `escape`, `enter`, `tab`, `space`, `backspace`, `delete`, `insert`, `home`, `end`, `pageup`, `pagedown` |
| arrows | `left`, `right`, `up`, `down` |
| punctuation | `minus`, `equal`, `leftbracket`, `rightbracket`, `backslash`, `semicolon`, `quote`, `comma`, `period`, `slash`, `grave` |
| keypad | `kp0` … `kp9`, `kpenter`, `kpplus`, `kpminus`, `kpmultiply`, `kpdivide`, `kpdecimal` |
| modifiers | `shift`, `ctrl`, `alt`, `cmd` |

`insert` reaches the client as Insert, whichever key AppKit maps to it.

## 5. Events

| Event | Fields | Sent when |
|---|---|---|
| `mode` | `mode`, `previous`, `detail` | the mode of `state` changes |
| `map` | `map`, `x`, `y` | game mode enters a map (first entry and every map change) |
| `chat` | `channel` (`normal`, `party`, `guild`, `whisper`, `broadcast`, `system`), `from` (sender's name or `null`), `text` | a line is added to the chat window, own lines included |
| `window` | `action` (`open`, `close`), `wid` (the window's `id` in `state`), `name` | a UI window opens or closes |
| `bye` | `reason` (`quit`, `signal`, `error`) | the client is about to close the socket and exit |
| `error` | `code` | without a request to answer (only `busy`, section 1) |

## 6. Reserved

These names are kept for later versions, for first-class agent use (spec section 6). A client of version 0 answers the commands with `unsupported`, leaves the `state` parts out of its replies and never sends the events; nobody uses them for anything else.

- Commands: `step` (advance a fixed number of logic steps), `pause`, `resume`, `render` (draw without a visible window), `subscribe` (choose events), `trace` (stream trace lines), `config` (read or change a setting), `act` (high-level actions).
- `state` parts: `inventory`, `equipment`, `skills`, `party`, `guild`, `status`.
- Events: `trace`, `frame`, `packet`.
