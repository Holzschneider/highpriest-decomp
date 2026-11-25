# control: the control socket

Steps M4.6 and the native half of M5.6. The client listens on a Unix-domain socket and speaks [PROTOCOL.md](PROTOCOL.md) (version 0): JSON lines, requests in, replies and events out. The contract for the client is [`include/bf_control.h`](include/bf_control.h).

## Build and test

```
cmake -S port/native/control -B build/native/control -G Ninja
cmake --build build/native/control
build/native/control/bifrost-control-test               # the checks, one line each, about 1 s
python3 port/native/control/test/crosscheck.py          # the harness against the module, about 5 s
```

- **`bifrost-control-test`** plays both sides on the main thread:
  - the client's frame loop: drain the app queue, `bf_control_poll`, `bf_control_frame_done`;
  - the other end of the socket.

  It covers framing (a line in two writes, two lines in one write, a line over 1 MiB, invalid JSON and UTF-8, blank lines), every error code, injection reaching the app queue in order with the right types and one logic step per frame, replies held until the events are consumed, deferred replies, events interleaved with replies in `seq` order, `wait` timeouts, fake capture and resize hooks, the `busy` refusal, reconnects, and `quit` and `bye`. It uses the app queue without a window, needs no game data, and puts its socket in the build directory (or `$TMPDIR` when that path is over 103 bytes). It is also clean under ASan, UBSan and TSan:

  ```
  -DCMAKE_CXX_FLAGS=-fsanitize=thread -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread
  ```
- **`bifrost-control-echo`** is a headless stand-in client. It is the real module with a fake login screen behind it, shaped like the harness's mock: `LOGINWND` and `SELECTCHARWND`, typing, Tab, Enter, mode and window events, synthetic PNG captures, resize. Input reaches it only through the app queue. `crosscheck.py` runs the harness's own `tests/test_protocol.py` against it unchanged (through the harness's launcher and control client), plus drag, modifiers, Cmd+Q and a wait on a window event.

## Using it in the client

```c
const char *path = bf_control_path_from_args(argc, argv);   /* --control or BF_CONTROL */
bf_control_hooks hooks = {ctx, quit, capture, resize, resize_busy, logical_to_px, px_to_logical};
bf_control_set_hooks(&hooks);
bf_control_register_state("mode", write_mode, ctx);          /* and detail, view, pointer, map, self, actors, windows */
bf_control_declare_event("mode");                              /* and map, chat, window */
if (path) bf_control_open(&(bf_control_config){path, build_string});
for (;;) {                                                     /* per frame */
	bf_app_pump(0); while (bf_app_poll_event(&e)) handle(e);
	logic();
	bf_control_poll();                                         /* after the events have been handled */
	draw(); present();
	bf_control_frame_done(++frames);                           /* the target still holds the frame */
}
bf_control_close("quit");                                      /* or "signal", "error"; sends bye, removes the socket */
```

**Hooks** (any may be NULL; the command then answers `unsupported` and is left out of `hello`):

| Hook | What it does |
|---|---|
| `quit` | Starts the shutdown as Cmd+Q does. Called after the reply. |
| `capture` | Writes the frame just completed as a PNG and fills its size, scale and logical size (`bf_gfx_capture_png`). |
| `resize` | Starts a size, scale or fullscreen change. |
| `resize_busy` | Non-zero while a fullscreen animation runs; the reply waits for it. |
| `logical_to_px` | Logical coordinates to the pixels of `bf_app` pointer events (logical × UI scale × backing scale). |
| `px_to_logical` | The reverse, for the `x` and `y` of a pointer reply. |

**State providers:** one per part of `state`, each writing one JSON value. `wait` reads `mode`, `map` (`name`), `windows` (`name`) and `self` (`x`, `y`, `moving`) through them. The module provides `frame` itself, from `bf_control_frame_done`.

**Events:** `bf_control_event_begin(name)` returns a writer, or NULL when nobody is connected; `bf_control_event_end` numbers and sends the event.

**Own commands:** `bf_control_register(name, handler, context)`. A handler either replies (`bf_control_reply`, `bf_control_reply_error`) or defers (`bf_control_defer`). A deferred request takes an optional check function, which is called from every poll and frame until it replies.

## How it works

- **Threads.** An I/O thread owns the sockets: it accepts, reads lines into an inbox and writes the outbox. Everything else runs on the main thread inside `bf_control_poll` and `bf_control_frame_done`: parsing, handlers, providers, replies and events.
  - A connection is numbered on both sides, so output meant for a connection that has meanwhile closed is dropped.
  - Reading pauses while more than 16 MiB of requests wait.
  - A reader that leaves more than 64 MiB unread is disconnected.
- **Order.** Requests are carried out one after another. A deferred request (`wait`, `capture`, `resize`, `key`, `pointer`, or a client command that defers) holds back the ones behind it. Events go out whenever they are emitted. A reply's `seq` is that of the last event before it.
- **Injection.**
  - A request's events are split into logic steps:
    - `press`: key down, then key up;
    - `click`: move, then down, then up;
    - `text`: one character per step.
  - The first step is posted at once with `bf_app_post_event_seq`. Each later step is posted from the `bf_control_poll` after `bf_app_consumed_seq` shows the previous one taken. The reply goes out from the poll after the last step has been taken. So the client sees one step per frame, and the reply comes after the events' direct effect.
  - Modifiers in `mods` are pressed in the first step and released with or after the last one. Modifiers already held are left alone.
  - If the connection closes mid-way, the remaining steps are still posted, so no key is left held.
- **Key names** are the PROTOCOL.md section 4 names, mapped to `bf_key` by a table of this module's own. `bf_app_key_from_name` uses different names (`return`, `lshift`, `kp_plus`). `shift`, `ctrl`, `alt` and `cmd` are the left-hand keys.
- **`text`** accepts the printable Windows-1252 characters only: 0x20–0x7E, 0xA0–0xFF and the 27 characters at 0x80–0x9F. Each character is delivered as one `BF_APP_EVENT_TEXT` with `key_seq` 0.
- **JSON.** The module has its own strict RFC 8259 reader and writer:
  - the reader refuses comments, trailing commas, leading zeros, bad escapes, lone surrogates, invalid UTF-8 and nesting deeper than 64;
  - integers are told apart from other numbers, and `5.0` is not an integer argument;
  - the writer escapes control characters and writes invalid UTF-8 as U+FFFD.
- **`capture`** gets its SHA-256 from CommonCrypto, by reading the file back.

## Changed outside this module

`port/native/app` (additive):
- `bf_app_post_event_seq` returns the `seq` of an injected event;
- `bf_app_consumed_seq` returns the `seq` of the last event taken by `bf_app_poll_event`;
- `bf_app_queue_reset` counts the events it drops as taken.

## Untested or open

- Not yet run inside the client:
  - the hooks and providers are fakes;
  - the client registers `windows` and the `window` event (M5.6, `port/client/src/bf_main.cpp`); `self` and `actors` are M6.4.
- Pointer injection posts no `POINTER_ENTER` or `POINTER_LEAVE`, so `bf_app_pointer.inside` does not follow injected moves.
- A command that ends in the client's shutdown (Cmd+Q typed through `key`) gets no reply; `bye` follows instead. PROTOCOL.md section 2 now says so.
- The harness's mock accepts control characters in `key` `text`. PROTOCOL.md and this module refuse them (`bad_text`), because `bf_app` text events carry printable characters only.

## For `port/README.md`

> **Control module** (`port/native/control`, M4.6). The control socket of [PROTOCOL.md](native/control/PROTOCOL.md): I/O thread, JSON lines, command registry, input injection into the app queue, `wait`, `capture`, `state`, `resize`, `quit`. Test: `cmake -S port/native/control -B build/native/control -G Ninja && cmake --build build/native/control && build/native/control/bifrost-control-test` (about 1 s, no window, no game data). The harness's protocol test runs against the module through a headless stand-in client: `python3 port/native/control/test/crosscheck.py`.
