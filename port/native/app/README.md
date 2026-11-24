# app: window, event queue, key and pointer state, clipboard

The AppKit side of the client (step M4.1). The client keeps its own loop on
the main thread; each frame it calls `bf_app_pump(0)`, which processes the
AppKit events that are waiting and turns them into `bf_app_event` records,
and then drains the queue with `bf_app_poll_event()`. The control socket
injects into the same queue with `bf_app_post_event()` (any thread). The
contract is [`include/bf_app.h`](include/bf_app.h).

## Build and test

```
cmake -S port/native/app -B build/native/app -G Ninja
cmake --build build/native/app
build/native/app/bifrost-app-test                 # the checks, one line each
build/native/app/bifrost-app-test --interactive   # logs real events; close or Cmd+Q ends it
build/native/app/bifrost-app-test --interactive --hide-cursor
```

The test opens a window and takes about 18 s. It needs no game data. It
uses its own single-instance slot (`test-<pid>`), so it can run beside the
client and beside other copies of itself. Its lock file is removed at the
end.

When the screen is locked, as it usually is on the unattended Mac, no app
can become active, every window counts as occluded and macOS refuses
fullscreen. The test detects this (`CGSessionCopyCurrentDictionary`) and then:
- sends its synthetic `NSEvent`s straight to the main menu and to
  `-[NSWindow sendEvent:]` instead of through `[NSApp postEvent:]`;
- reports `fullscreen` and `visibility-event` as skipped, giving the reason.

On an unlocked screen those two checks run for real.

## What it does

- **A regular app without `[NSApp run]`.** `bf_app_init` sets the regular
  activation policy (Dock icon, menu bar) and calls `finishLaunching`. This
  works for an unbundled binary too. The menus are *App* (Hide, Hide
  Others, Show All, Quit ⌘Q), *View* (Toggle Full Screen ⌘F) and *Window*
  (Minimize ⌘M). Cmd+Q, the window's close button and any terminate request
  (the Dock's Quit, logout, AppleScript) each become a `BF_APP_EVENT_QUIT`
  event with its reason. The process is never terminated behind the client's
  back: `applicationShouldTerminate:` answers `NSTerminateCancel`. As a
  result a logout waits until the client has quit by itself.
- **The window.** A titled, resizable window that can go fullscreen in its
  own Space, with a minimum size of 320×240 pt. Its content view is backed
  by a `CAMetalLayer`: system default device, BGRA8. The module keeps the
  layer's `drawableSize` equal to the content size in pixels and its
  `contentsScale` equal to the backing scale. Size and scale changes become
  `BF_APP_EVENT_RESIZE` events. Fullscreen (⌘F or
  `bf_app_window_toggle_fullscreen`) is animated; `BF_APP_EVENT_FULLSCREEN`
  arrives when it completes.
- **Single instance.** On start, the module takes `flock(LOCK_EX|LOCK_NB)`
  on `~/Library/Application Support/<name>/instance-<id>.lock`, which holds
  the owner's pid. The owner also registers a `CFMessagePort` named
  `<bundle id>.instance.<id>`. A second process finds the lock taken,
  messages the port, yields activation to the owner and asks it to activate,
  and `bf_app_init` returns `BF_APP_ALREADY_RUNNING` before any Dock icon
  appears. The owner deminiaturizes its window, orders it front, activates,
  and queues `BF_APP_EVENT_REOPEN` (1). `flock` ends when the process dies,
  so a crash leaves no stale lock. Neither the lock nor the port needs a
  bundle. `BF_MULTI_INSTANCE=1` turns the check off; `instance_id` in
  `bf_app_config` gives tests a slot of their own.
- **No App Nap.** `bf_app_init` holds a `beginActivityWithOptions:`
  activity with `NSActivityUserInitiatedAllowingIdleSystemSleep |
  NSActivityLatencyCritical` until `bf_app_shutdown`. The activity disables
  App Nap and timer coalescing. It still lets the system sleep when idle, so
  that a client left running does not keep the Mac awake.
- **Drawing in the background.** Nothing more is needed:
  - with the window occluded (behind the lock screen), the test presents
    121 frames per second to the layer (`nextDrawable`, clear, present);
  - with the window minimised it presents 121, and no drawable comes back
    nil.

  The graphics module can keep presenting to the layer whatever
  `bf_app_window_is_visible()` says, and must not pause when occlusion
  starts.
- **Live resize and menus.** While the user drags the window's edge or a
  menu is open, AppKit runs a loop of its own inside `bf_app_pump`. With
  `bf_app_set_modal_tick(fn, ctx)`, the module calls `fn` about 60 times a
  second during such a loop, from a timer in the tracking and modal-panel
  run-loop modes only, so that the client can keep its frame going.
- **The pointer** is never captured, warped or hidden outside the window.
  - Movement arrives from the key window (`acceptsMouseMovedEvents`, also
    outside the content area) and from an always-active tracking area (over
    the content area while the app is in the background).
  - Drags report positions outside the window.
  - `bf_app_set_cursor_hidden_in_window(1)` shows a blank cursor over the
    content area only, through `cursorUpdate:`.

## The event queue and the state

- The `BF_APP_EVENT_*` types are listed in the header. Every event has a
  `seq` (queue order), a `time_ms`, `mods` and `source` (AppKit or
  injected).
- **The state follows dequeuing.** `bf_app_poll_event` applies each event
  to the key and pointer state (`bf_app_key_is_down`,
  `bf_app_mac_key_is_down`, `bf_app_modifiers`, `bf_app_pointer_get`,
  `bf_app_pointer_take_delta`). The state is therefore what the client has
  seen, as `GetKeyState` is on Windows, and injected events change it
  exactly as real ones do.
- **Injected events.** The queue fills in:
  - `seq`, `source`, and `time_ms` when it is 0;
  - the modifiers of held modifier keys, so that "lshift down, a down" reads
    as Shift+A;
  - `mac_keycode` from `key`, or `key` from `mac_keycode`;
  - pointer points from pixels, the held-buttons mask, and the delta (the
    difference to the previous position) when the event does not carry
    `BF_APP_EVF_DELTA_VALID`.
- **Keys.**
  - `key.mac_keycode` is the macOS virtual key code; `key.key` is a
    portable `bf_key`, whose values are the USB HID usage IDs (page 7).
  - Modifier keys come through `flagsChanged` as their own key down and key
    up, left and right apart (from the device bits).
  - Caps Lock reports a press and a release each time it toggles.
  - Fn counts as a modifier only while the Fn key itself is held: AppKit
    also sets its flag on arrow and F-keys.
  - When focus is lost, the module queues key-ups and button-ups for
    everything AppKit had reported held, before `FOCUS(0)`.
  - `NSApplication` drops key-ups while Cmd is held; the pump hands those to
    the window itself.
- **Text.**
  - What a key types goes through `interpretKeyEvents:` and
    `NSTextInputClient`, which handles dead keys and input methods. It
    arrives as `BF_APP_EVENT_TEXT` events after the key-down, with
    `text.key_seq` = that key-down's `seq`, so that a consumer can drop the
    text of an Option shortcut.
  - Only printable characters are reported: no control characters and none
    of AppKit's function-key code points (U+F700–U+F8FF).
  - Cmd combinations type nothing.
  - Press-and-hold accent menus are off (`ApplePressAndHoldEnabled` is
    registered as NO, in memory only), so held keys repeat.
- **The clock.** Event times come from the clock installed with
  `bf_app_set_clock`, which should be the `sys` module's millisecond clock.
  Until one is installed, the module uses a monotonic placeholder that
  starts at 0, and says so once in the `input` trace.

## Keys from a Mac keyboard

- **F1–F12** arrive as ordinary key-downs whenever macOS delivers them as
  function keys:
  - the codes are kVK 0x7A, 0x78, 0x63, 0x76, 0x60, 0x61, 0x62, 0x64, 0x65,
    0x6D, 0x67 and 0x6F;
  - they type no text;
  - they are mapped to `BF_KEY_F1`…`F12`.

  When they are delivered:
  - **"Use F1, F2, etc. keys as standard function keys" on**
    (`com.apple.keyboard.fnState = 1`, the setting on this Mac): always,
    unless a system shortcut claims the key (Mission Control's defaults can
    take F11/F12 or Ctrl+F-keys).
  - **That setting off** (the factory default): only while Fn is held,
    which also delivers `BF_KEY_FN` down first. Without Fn, the keys perform
    brightness, Mission Control and media actions, and the window gets no
    key event for them.

  These cases come from the system's documented behaviour and the kVK
  table. Only the delivery of the kVK codes through the module was tested,
  with synthetic events; `--interactive` shows what a real keyboard sends.
- **A PC keyboard** on a Mac sends:
  - **Insert** as kVK_Help (0x72), mapped to `BF_KEY_INSERT`;
  - **Print Screen, Scroll Lock and Pause** as F13, F14 and F15;
  - **Num Lock** as the keypad's Clear key (0x47), mapped to
    `BF_KEY_NUMLOCK`.
- **Apple keyboards** have no Insert key. Choosing its substitute is the
  adapter's job (spec §4).

## What the Win32 adapter (M5.1, M5.2) must translate

- `PeekMessage`: `bf_app_pump(0)`, then one `bf_app_poll_event` per
  message.
- `FindWindow`, the second-instance test in `InitApp` (`g_multiSTOP`): the
  return value of `bf_app_init`.
- `KEY_DOWN` / `KEY_UP` become messages:
  - `WM_KEYDOWN` / `WM_KEYUP`, or `WM_SYSKEYDOWN` / `WM_SYSKEYUP` while Alt
    (Option) is held, and for F10;
  - `wParam` is the VK code;
  - `lParam` is `1 | scan << 16 | repeat << 30`, plus `0xC0000000` on key
    up, where `scan` is the set-1 scan code. `UIWindowMgr::ProcessPushButton`
    compares `lParam` against `0x1e0001` (A), `0x200001` (D), `0x110001`
    (W) and `0x2d0001` (X), and reads bit 30 as "repeat".
- `TEXT` becomes one `WM_CHAR` per character:
  - converted to CP1252, with characters that do not map dropped;
  - dropped altogether when the key-down it belongs to (`text.key_seq`)
    had Alt or Ctrl, or was taken as a shortcut.

  `TranslateMessage` also typed control characters, so the adapter
  synthesises `WM_CHAR` 8, 9, 13 and 27 for Backspace, Tab, Return and
  Escape: `CLanguage::OnChar` handles `'\b'` there.
- **Cmd+X, Cmd+C and Cmd+V** keep their Mac meaning. `CLanguage::OnKeyDown`
  expects Ctrl+X, Ctrl+C and Ctrl+V, so while the adapter delivers those
  key-downs, `GetKeyState(VK_CONTROL)` must read as down.
- `GetAsyncKeyState` and `GetKeyState`: VK to `bf_key`, then
  `bf_app_key_is_down`. `VK_SHIFT`, `VK_CONTROL` and `VK_MENU` are left
  *or* right; down reads as `0x8000`.
- **The DirectInput mouse** (`CMouse::ReadState`):
  - `lX` / `lY` are the position from `bf_app_pointer_get`, divided by the
    UI scale, minus the client's own `m_xPos` / `m_yPos` (technical plan
    3.7). Right-drag camera rotation can use `bf_app_pointer_take_delta`
    instead.
  - `lZ` is the wheel × 120.
  - The buttons are `bf_app_pointer.buttons`, honouring `SM_SWAPBUTTON` as
    the client does.
- **Other messages:**
  - `WM_MOUSEMOVE`, which the client uses only while inactive: from
    `POINTER_MOVE`.
  - `FOCUS`: `WM_SETFOCUS` / `WM_KILLFOCUS`, and `WM_NCACTIVATE` /
    `WM_ACTIVATE` with `wParam` 1/0.
  - `QUIT`: `WM_CLOSE`.
  - `RESIZE`: the resize path of M4.3.
- **Other calls:**
  - `SetCursor(NULL)` / `ShowCursor(FALSE)`:
    `bf_app_set_cursor_hidden_in_window(1)`.
  - `OpenClipboard` / `GetClipboardData(CF_TEXT)` / `SetClipboardData`:
    `bf_app_clipboard_get_text` / `_set_text`, with UTF-8 ⇄ CP1252.
  - `VK_PROCESSKEY` and the `Imm*` calls have nothing behind them: macOS
    composes the text, and only committed text arrives, as `TEXT`.

## Untested or open

- These have not been seen working:
  - **On the locked screen of the unattended run:** fullscreen (macOS
    refuses it: `windowDidFailToEnterFullScreen`), visibility events, and
    focus changes and activation from a second launch. The second launch
    itself is tested: it exits, and the first instance receives `REOPEN`.
  - **Needing a real keyboard, pointer or user:** the modal tick during
    live resize, hiding the cursor, dead keys and input methods, wheel and
    trackpad scrolling, and the Dock's reopen.

  `--interactive` covers what needs a person.
- A logout waits for the client: the terminate request is cancelled and
  turned into a quit event (see "What it does").
- Media-key events (`NSEventTypeSystemDefined`) are not turned into
  anything.

## For `port/README.md`

> **Application module** (`port/native/app`, M4.1). The window, its Metal
> layer, the one event queue, key and pointer state, clipboard, single
> instance, no App Nap. Test: `cmake -S port/native/app -B build/native/app
> -G Ninja && cmake --build build/native/app &&
> build/native/app/bifrost-app-test` (about 18 s, opens a window, no game
> data); `--interactive` logs real input. Set `BF_MULTI_INSTANCE=1` to
> allow several clients at once.
