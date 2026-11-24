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
| `items` (in a `list` control) | the list's lines in order, as the client holds them (an NPC menu's choices, a letter's text in `MAILVIEWWND`); `text` stays the selected line |
| `controls` of the skill, shortcut and option windows | `SKILLLISTWND`: `skill:<SKID>` (`item`, one per skill row shown, at its icon; text `<skill id name> <level>`, as `SM_BASH 1`), `up:<SKID>` (the row's level-up button, where shown), `points` (`text`: free skill points), `ok` (use), `close`. `SHORTCUTWND`: `slot:<n>` (`item`, n 0–8 of the page shown, the F1–F9 keys; text `skill:<SKID>:<use level>` (0 for a skill whose level is not chosen, as the 2008 client keeps it), `item:<item id>` or empty), `page` (`text`: the page shown, 1–3; F12 turns it). `OPTIONWND`: the checks `bgm_on`, `sound_on`, `snap_attack`, `snap_skill`, `snap_item`, `noctrl`, and `bgm_volume`, `sound_volume` (kind `scroll`, a scroll bar; text: its position, 0–127) |
| `sound` | the sound module's counters (port/native/audio, `bf_audio_get_counters`): `mode` (`device`, `offline`, `silent`, or `off` before the driver opens), `max_voices`, `voices_started`, `voices_playing`, `voices_stolen`, `voices_rejected`, `decodes`, `decode_failures`, `effects_volume`, `music_volume` (masters, 0..1); `stream` (`file`: the path opened, `null` when none; `status`: `none`, `open`, `playing`, `paused`, `done`; `loops`: passes of the current file the output has played to their end, so 0 in `silent` mode, which renders nothing; `duration` and `position` in seconds; `volume` 0..1; `opened`, `failures`: streams since start); `recent`: the last 16 calls to start a voice, oldest first, each as the `sound` event's fields |
| `items` | the own character's items as the client holds them, `null` outside game mode: `index` (the server's inventory index), `id` (item id), `name` (as the client names it), `amount`, `equipped` (the worn location bits, 0 when not worn), `type` (the client's item type), `identified`; the items in the inventory first, then each worn item once. (`inventory` is reserved, section 6; this part is the client's list as it stands) |
| `movie` | the movie the client plays (`src/movie/CBink.cpp`, the intro; port/compat/bink): `playing` (a movie is open), `paused`, `file` (the name it was opened with, `null` before any), `width`, `height`, `frames`, `frame` (the frame last decoded, 1-based, 0 before the first), `fps`, `tracks` (sound tracks in the file), `sound` (the first track is played), `voice` (its sound-module voice, 0 when none), `sound_seconds`; `opened`, `closed` (movies since start); `ended`, `last_frame`: of the last movie closed, whether it was shown to its last frame (else a key stopped it) and the frame it stopped at. Kept after the movie closes |

Text in `state` and in events is UTF-8, decoded from the client's Windows-1252 bytes; bytes that do not decode (Korean typed by others) appear as U+FFFD.

What the client reports in the game parts, where its own data sets limits (step M6.4):

- **`mode`** is `loading` from the zone server's answer until the map is loaded (the login mode's `LSM_READY_FOR_ZONESVR`, then game mode's `OnInit`, detail `MT_GAME (loading)`), and on every map change; `game` once the map is loaded and play has begun. `map` and `self` are not `null` exactly while the mode is `game`.
- **`self`**: `x`, `y` are the cell of the character as the client draws it; during a walk they advance with the animation, so they can be a cell ahead of or behind the server's position, and agree with it once the walk has ended. `moving` is true from the server's acknowledgement of a move (`ZC_NOTIFY_PLAYERMOVE`) until the client's walk ends; between a click and that acknowledgement it is still false, so a script waits for `position` together with `moving: false`. `dest` is the walk's destination while `moving`, else `null`. `dir` uses the server's convention (0 north, counting counter-clockwise to 7 north-east). `weight` and `max_weight` are as the client shows them: the server's tenths divided by 10.
- **`actors`** lists what the client holds: the characters, NPCs and monsters the server has shown it, the items on the ground and the skill units. `type` comes from the client's own tests where it has them (its pet, homunculus and mercenary ids; a pet's head type) and otherwise from the job id: `pc` for player jobs, `portal` for 45 and 139, `monster` for 1001–3999 and 25000–25017, `npc` for 46–999, 10000–19999 and 20000–20017, `homun` for 6001–6016, `merc` for 6017–6046, `other` for the rest. `name` is `?name` until the client knows the name: the 2008 client asks the server for an actor's name (`CZ_REQNAME`) only when the pointer rests on it, and `state` never asks, so a script that looks an actor up by name lets the pointer rest on its `screen` position first (`bfharness.control.Control.actor` does). Names are as the name balloon shows them: for all but players the part from `#` on is cut (rAthena's `Guide#prt` is `Guide`). An item's `name` is the label the client shows for it on the ground (the item's name and count). `screen` is the projection of the actor's position with the current camera, `null` behind the camera or outside the window.
- **`pointer`**: `x`, `y` and `buttons` are the pointer as the application module holds it, also outside game mode; `cell` is the cell the client's own ground picking (`CGameMode::GetGroundAttrPos`) finds under the pointer, `null` outside game mode or off the ground.
- **`actors[].emotion`** (M8.1b): the action of `emotion.act` that the actor's newest emote balloon shows while it is drawn (a `MET_EMOTION` message effect of the actor), else `null`. The client's own table (`CGameMode::MakeEmotion`) maps the server's `ZC_EMOTION` type to the action; types 0–3 (`ET_SURPRISE`, `ET_QUESTION`, `ET_DELIGHT`, `ET_THROB`) map to themselves.
- **`windows` controls in game** (M8.1b): `CHATWND` names its chat line `chat_edit` and the whisper name field left of it `whisper_edit` (a line sent with a name there, not starting with `/`, is a whisper to that name). `MENUWND`, the menu a right-click on a player opens, reports its entries as `button` controls `menu:0`, `menu:1`, … in order, with their text and the rectangle a click selects them by (the entries are drawn, not windows of their own).

- **`controls` of the item, equipment and status windows** (M8.1d), which draw their contents themselves: `ITEMWND` reports its tabs `tab:item`, `tab:equip`, `tab:etc` (kind `button`, text `on` for the open tab) and each item it shows as `item:<index>` (kind `item`, the cell `UIItemWnd::OnDraw` draws it in, text the item's name); `EQUIPWND` each worn item as `item:<index>` at its slot; `STATUSWND` its values as kind `text`, formatted as the window prints them: `stat:str` … `stat:luk` (`5`, or `5 + 1` with a bonus), `need:str` … (the points a raise costs), `atk` (`24 + 0`), `matk` (`5 ~ 6`), `hit`, `critical`, `def` (`1 + 5`), `mdef`, `flee`, `aspd`, `status_point`. Its raise buttons are `str` … `luk`.
- **`controls` of the NPC, deal and storage windows** (M8.1e): `SAYDIALOGWND` `text` (kind `list`, the dialogue's lines joined by `\n`) and the one live button, `next` (after `ZC_WAIT_DIALOG`) or `close` (after `ZC_CLOSE_DIALOG`; buttons already used are left out); `CHOOSEWND` `list`, one `choice:<entry>` (kind `item`, text = the entry as the server sent it, colour codes included) per entry the list shows, `ok`, `cancel`: a click selects an entry, `ok` (or a double click) sends it; `NPCEDITDIALOGWND` (number) and `NPCTEXTEDITDIALOGWND` (text) `edit`, `ok`; `ITEMDROPCNTWND` (the amount asked when an item is dragged into a deal, storage or item window) `edit`, `ok`; the deal and storage windows (`ITEMSHOPWND`, `ITEMPURCHASEWND`, `ITEMSELLWND`, `ITEMSTOREWND`) one `item:<item id>` (kind `item`, text `<name> <count>`, the cell of the item's icon; `item:<id>:<n>` for a second stack of the same id) per item shown, and in `ITEMSTOREWND` the tabs `tab:item`, `tab:equip`, `tab:etc` as in `ITEMWND`. The input windows open with their text selected, so typed text replaces it. The 2008 client ignores a Next or OK within 300 ms of the previous one.
- **`item:` ids, per window**: `ITEMWND` and `EQUIPWND` name an item by the server's inventory index (`item:<index>`, as the `items` part's `index`); the deal and storage windows by its item id (`item:<id>`, as `items[].id`), since a shop's items have no inventory index and the storage's index is the storage's own.

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
| PC keys on a Mac | `f13`, `f14`, `f15`: what a PC keyboard's Print Screen, Scroll Lock and Pause send on a Mac; the client takes them as those keys (the game's screenshot keys) |

`insert` reaches the client as Insert, whichever key AppKit maps to it.

## 5. Events

| Event | Fields | Sent when |
|---|---|---|
| `mode` | `mode`, `previous`, `detail` | the mode of `state` changes |
| `map` | `map`, `x`, `y` | game mode enters a map (first entry and every map change): sent when `mode` becomes `game`, with the `self` cell at that moment. A teleport within the same map sends none |
| `chat` | `channel` (`normal`, `party`, `guild`, `whisper`, `broadcast`, `system`), `from` (sender's name or `null`), `text` | a line is added to the chat window, own lines included (as the server sends them back). For `normal`, `party` and `guild`, `from` is the name the line begins with and `text` the rest after the client's `name : `; a `whisper`'s `from` is its sender; `broadcast` and the client's own messages (`system`) have `from` `null` and the whole line as `text`. Lines said in a chat room go to the chat room's window and are not sent |
| `window` | `action` (`open`, `close`), `wid` (the window's `id` in `state`), `name` | a UI window opens or closes |
| `sound` | `serial` (1, 2, … per call since start), `file` (the name `PlayWave` was given, UTF-8 from code page 949, or `null`), `volume` (0..1: Miles' 0..127 that `PlayDigital` computed, divided by 127), `pan` (-1..1), `distance` (world units from the listener as `PlayWave` was given it, 5 per cell; `null` for a voice not started through `PlayWave`), `loop`, `started` (false when the module refused the voice), `duration` (seconds of the sound) | the client asks the sound module for a voice; sent in the next frame. A sound `PlayDigital` drops (beyond 250 units, or no free handle) sends none. More than 64 between two frames: the oldest are not sent |
| `damage` | `target` (the server's id of the actor the number is drawn over: an `actors` id, or `self.id`), `number` (as the client was given it; it draws at most 999999), `kind` (`normal`, `critical`, `heal`, `half`, `total`, `other`) | the client draws a number over an actor (`CGameActor::Am_Make_Number`): damage dealt or taken, a recovered amount; sent in the next frame. A miss draws no number and sends none; a hit of several numbers (a multi-hit skill) sends one per number drawn |
| `movie` | `what` (`start`, `end`), `file`, `frames`, `frame`, `ended` (for `end`: shown to its last frame) | a movie opens or closes; sent in the next frame |
| `bye` | `reason` (`quit`, `signal`, `error`) | the client is about to close the socket and exit |
| `error` | `code` | without a request to answer (only `busy`, section 1) |

**`chat` in a 1:1 window** (M8.1b): the 2008 client writes a whisper into the whisper window with the other player (`WHISPERWND`, which it opens for a whisper sent or received when its option `m_isShowWhisperWnd` is on, the default) instead of the chat window. Such a line is a `chat` event too, `channel` `whisper`, `from` the name the line begins with (the sender, the own name for an own line), `text` the rest, and an extra field `window`: the other player's name, the window's. Lines in the chat window have no `window` field.

## 6. Reserved

These names are kept for later versions, for first-class agent use (spec section 6). A client of version 0 answers the commands with `unsupported`, leaves the `state` parts out of its replies and never sends the events; nobody uses them for anything else.

- Commands: `step` (advance a fixed number of logic steps), `pause`, `resume`, `render` (draw without a visible window), `subscribe` (choose events), `trace` (stream trace lines), `config` (read or change a setting), `act` (high-level actions).
- `state` parts: `inventory`, `equipment`, `skills`, `party`, `guild`, `status`.
- Events: `trace`, `frame`, `packet`.
