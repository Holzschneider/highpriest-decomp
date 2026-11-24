# input: user32 windows, messages, keyboard, cursor, clipboard; DirectInput; imm32

The Win32 adapter of area `input` (steps M5.1 and M5.2), on
[`port/native/app`](../../native/app) (window, event queue, key and pointer
state, clipboard) and [`port/native/view`](../../native/view) (logical size,
UI scale, pointer mapping). Stage A only: M11 deletes it and changes the call
sites. The port-side functions are in
[`include/bf_compat_input.h`](include/bf_compat_input.h).

## Build and test

```
cmake --preset debug && cmake --build --preset debug --target bf_compat_input bifrost-compat-input-test
build/cmake/debug/compat/input/bifrost-compat-input-test
```

- The test takes under a second. It opens a window in its own
  single-instance slot (`compat-input-test-<pid>`), needs no game data, and
  injects every event through `bf_app_post_event`, as the control socket
  does, so it runs on the locked screen.
- It calls the adapter as `WinMain.cpp`, `CMouse` and `CLanguage` do:
  `InitApp`'s window creation, `CheckSystemMessage`'s pump, a window procedure
  that records what it receives, and `CMouse::ReadState`'s integration of
  `lX`/`lY`.
- It sets the Mac clipboard during its run and puts the previous text back
  at the end.
- It spawns itself once as a second instance, which must exit through
  `FindWindowA`.
- Its ctest label is `window`, so `fastcheck.sh` builds it but does not run it.
- It links `bf_compat_gdi` when the tree has the gdi area, and stand-ins for
  `bf_compat_gdi_screen_dc` and `bf_compat_gdi_brush_color` when it does not.
  The kernel32 memory functions the clipboard needs (`GlobalAlloc`, ...) are
  stand-ins in `test/fake_kernel32.cpp`; the win32 area owns the real ones.

## What the client must call at start-up

The client's own calls work unchanged. `FindWindowA` (or the first
`RegisterClassA` or `CreateWindowExA`) starts the application with
`bf_app_init`. `CreateWindowExA` creates the window and starts the view. The
start-up glue adds:

| When | Call | Why |
|---|---|---|
| Before `InitApp` | `bf_app_set_clock(<sys ms clock>)` | event times (`MSG.time`) come from the one clock |
| Before `InitApp` | `bf_compat_input_set_scale_setting(<prefs UI scale>)` | `bf_view_init` takes it; 0 = auto (default) |
| Optional, first | `bf_compat_input_configure(id, single)` | another single-instance slot (tests); default `"main"`, on |
| d3d adapter, after creating the device | `bf_view_attach_gfx(dev)` | the device on `bf_app_window_metal_layer()` follows resizes |
| Resize path (M4.3a) | `bf_view_set_listener`, `bf_view_sync()` per frame, `bf_view_set_modal_frame` | the pump already passes `RESIZE` and `FULLSCREEN` to `bf_view_handle_event` |

## How messages are made

- **The pump.** `PeekMessageA` hands out pending messages first. When none
  match, it takes **one** event out of the app queue and translates it. The
  key and pointer state the client reads while it handles a message is
  therefore the state as of that message, as on Windows. When the queue is
  empty, it calls `bf_app_pump(0)` once and looks again. Then it reports "no
  message" and pumps again on the next call. `GetMessageA` waits in
  `bf_app_pump(100)`. `PostMessageA` and `PostQuitMessage` append to the
  pending messages; `SendMessageA` and `DispatchMessageA` call the window
  procedure.
- **Hook.** Every event is first offered to the hook of
  `bf_compat_input_set_event_hook`, which may consume it (the control
  socket's `USER` events).

| App event | Messages |
|---|---|
| `KEY_DOWN` | `WM_KEYDOWN`, or `WM_SYSKEYDOWN` while Alt (Option) is held without Ctrl, for F10 and for Alt itself; `wParam` the VK (`VK_SHIFT`/`VK_CONTROL`/`VK_MENU` for either side); `lParam` = `1 \| scan << 16 \| extended << 24 \| alt << 29 \| previous << 30`, where previous is the auto-repeat flag or the key already held |
| `KEY_UP` | `WM_KEYUP`/`WM_SYSKEYUP` with `0xC0000000` added; only for keys whose down was sent |
| `TEXT` | one `WM_CHAR` per character, CP1252; characters CP1252 lacks are dropped; the whole text is dropped when its key-down had Ctrl, Cmd, or Option on a game shortcut |
| `POINTER_MOVE` | `WM_SETCURSOR` (`HTCLIENT`) when inside, then `WM_MOUSEMOVE` with the logical position |
| `POINTER_ENTER` | `WM_SETCURSOR` |
| `BUTTON_*`, `WHEEL` | none: the client reads them through DirectInput |
| `RESIZE`, `FULLSCREEN` | `bf_view_handle_event`, then `WM_SIZE` with the logical size |
| `FOCUS(1)` | `WM_ACTIVATEAPP(1)`, `WM_NCACTIVATE(1)`, `WM_ACTIVATE(WA_ACTIVE)`, `WM_SETFOCUS` |
| `FOCUS(0)` | held keys forgotten; `WM_NCACTIVATE(0)`, `WM_ACTIVATE(WA_INACTIVE)`, `WM_ACTIVATEAPP(0)`, `WM_KILLFOCUS` |
| `QUIT` (Cmd+Q, close button, Dock, logout) | `WM_CLOSE` |
| `VISIBILITY`, `REOPEN`, `USER` | none |

- **`TranslateMessage`** adds the `WM_CHAR` 8, 9, 13 and 27 that AppKit
  reports as no text, for Backspace, Tab, Return and Escape without Ctrl. It
  puts them right after the key-down, as Windows does; `CLanguage::OnChar`
  handles `'\b'`.
- **`WM_CREATE`** and **`WM_NCCREATE`** are sent from `CreateWindowExA`.
  `WM_MOVE` and `WM_PAINT` are never sent. `WM_MOVE` would make the client
  draw a frame while inactive, before the device exists.

## Keys (spec section 4)

| Mac key | Reaches the client as |
|---|---|
| Letters, digits, punctuation | `'A'`…`'Z'`, `'0'`…`'9'`, `VK_OEM_*`, set-1 scan codes (A 0x1E, D 0x20, W 0x11, X 0x2D, ...) |
| Return, Tab, Delete (⌫), Esc, Space | `VK_RETURN` 0x1C, `VK_TAB` 0x0F, `VK_BACK` 0x0E, `VK_ESCAPE` 0x01, `VK_SPACE` 0x39 |
| Forward Delete (fn+⌫), Home, End, Page Up/Down, arrows | `VK_DELETE` 0x53, `VK_HOME` 0x47, `VK_END` 0x4F, `VK_PRIOR` 0x49, `VK_NEXT` 0x51, `VK_LEFT` 0x4B, `VK_RIGHT` 0x4D, `VK_UP` 0x48, `VK_DOWN` 0x50; all extended |
| F1–F12 (as function keys, see the app README) | `VK_F1`…`VK_F12`, scan 0x3B–0x44, 0x57, 0x58; F10 as `WM_SYSKEYDOWN` |
| **Cmd+I** | **`VK_INSERT`**, scan 0x52, extended: **the Insert substitute** (sit); Cmd+Option+I is Alt+Insert (compass), Cmd+Shift+I is Shift+Insert |
| Help (kVK 0x72, the Insert of a PC keyboard) | `VK_INSERT` |
| **Cmd+Shift+S** (Cmd+Ctrl+Shift+S) | **`VK_SNAPSHOT`** as `WM_KEYUP` only: **the Print Screen substitute** (the game's screenshot key; with Ctrl a BMP) for Apple keyboards, which have no F13 |
| F13, F14, F15 (Print Screen, Scroll Lock, Pause of a PC keyboard) | `VK_SNAPSHOT` (extended), `VK_SCROLL`, `VK_PAUSE`: the game's screenshot keys. `VK_SNAPSHOT` comes as `WM_KEYUP` only, as Windows sends it (the client takes the picture there; a key-down too would take two) |
| F16–F20 | `VK_F16`…`VK_F20` |
| Keypad digits, `. / * - + =`, Enter, Clear | `VK_NUMPAD0`…`9` (a Mac keypad has no Num Lock state), `VK_DECIMAL`, `VK_DIVIDE` (ext), `VK_MULTIPLY`, `VK_SUBTRACT`, `VK_ADD`, 0x92, `VK_RETURN` (ext), `VK_NUMLOCK` (ext) |
| Shift, Control, Option left/right | `VK_SHIFT` 0x2A/0x36, `VK_CONTROL` 0x1D/0x1D ext, `VK_MENU` 0x38/0x38 ext; `GetKeyState` also knows `VK_L*`/`VK_R*` |
| Caps Lock | `VK_CAPITAL`; `GetKeyState(VK_CAPITAL) & 1` is the latch |
| Cmd, Fn | no messages; `GetKeyState(VK_LWIN/VK_RWIN)` reads Cmd |
| ISO key left of Z, the menu key | `VK_OEM_102` 0x56, `VK_APPS` |

- **Option is Alt.** An Option key-down is `WM_SYSKEYDOWN`. The character
  macOS types for it is dropped when the key is a game shortcut. The
  shortcuts are those `UIWindowMgr::ProcessPushButton` tests with `VK_MENU`:
  A B E G J O P Q R S T V W X, 0–9, Insert, Home and F10
  (`bf_compat_input_is_alt_shortcut`). For other keys the character is
  typed, so that, for example, Option+L still types `@` on a German layout.
- **Ctrl** drops the text as well.
- **Cmd keeps its Mac meaning**, handled before the client sees anything.
  AppKit's menu takes Cmd+Q and Cmd+F itself (`QUIT`, fullscreen); the
  adapter does the same for injected key-downs.

  | Keys | What happens |
  |---|---|
  | Cmd+Q | `WM_CLOSE` |
  | Cmd+F | `bf_view_toggle_fullscreen` |
  | Cmd+V | the clipboard is typed as `WM_CHAR`s into the focused field: CP1252, control characters (line breaks, tabs) dropped, no key message |
  | Cmd+X, Cmd+C | `WM_KEYDOWN 'X'`/`'C'` while `GetKeyState(VK_CONTROL)` reads down, until the key or Cmd is let go: the cut and copy of `CLanguage::OnKeyDown` |
  | Cmd+I | Insert (above) |
  | Cmd+Shift+S | Print Screen (above); Cmd+S alone does nothing |
  | any other Cmd combination (Cmd+H, Cmd+M, ...) | the system's; nothing reaches the client |

- **Why Cmd+Shift+S for Print Screen.** The game takes screenshots with
  Print Screen (and Scroll Lock), which an Apple keyboard lacks; F13 is there
  only on full-size ones. macOS keeps Cmd+Shift+3, 4 and 5 for its own
  screenshots, the menus do not use Cmd+Shift+S, and "S" reads as "shot". The
  game itself has no use for Cmd. Ctrl, which makes the client save a BMP
  instead of a JPEG, can be held as well.
- **Why Cmd+I for Insert.** Every Apple keyboard has it, it is not a system
  shortcut, and the menus do not take it. It keeps Insert's own modifier
  combinations (Option for Alt, Shift). A single key without a modifier was
  not possible: every key of a compact Apple keyboard already means
  something to the game or to the system.
- **`GetAsyncKeyState`** returns 0x8000 or 0. Bit 0 stays clear, because the
  client tests the whole value (`if (GetAsyncKeyState(VK_SHIFT))`).
  `GetKeyState` adds the toggle bit for Caps Lock (and 1 for Num Lock).
  `VK_LBUTTON`, `VK_RBUTTON` and `VK_MBUTTON` read the pointer's buttons.

## Pointer and cursor

- **DirectInput 7.** `DirectInputCreateA`/`Ex` and `IDirectInput7A`. Only
  `GUID_SysMouse` is offered (an `IDirectInputDevice7A`, which is what the
  client's `IDirectInputDeviceA` points at), with `c_dfDIMouse` and
  `c_dfDIMouse2` and immediate data (`GetDeviceState`). `GetDeviceData`,
  `EnumObjects`, the properties, effects and `GUID_SysKeyboard` are refused.
  `SetCooperativeLevel` accepts the client's `DISCL_EXCLUSIVE`, but nothing
  is captured, confined or hidden outside the window.
- **`lX`/`lY`** are the logical pixel under the pointer (`bf_view_px_to_logical_pixel`,
  clamped to the logical screen) minus the position the adapter believes
  `CMouse` holds. The adapter then moves its belief by the same amount and
  clamps it as `CMouse::ReadState` does, so the client's sprite is where the
  macOS pointer is.
  - The belief starts at (320, 240), `CMouse::Init`'s values.
  - It follows `GetCursorPos`, whose only caller (`WM_MOUSEMOVE` while
    inactive) hands the result to `SetXPos`/`SetYPos`.
  - `bf_compat_input_mouse_get/set_client_pos` read and correct it.
- **Right-drag.** While the right button is held, `lX`/`lY` are the device's
  own movement (`bf_app_pointer_take_delta`, divided by the UI scale, with the
  fractions carried over). Camera rotation therefore goes on when the pointer
  reaches the window's edge or leaves the window. When the button is let go,
  the next read brings the client back to the pointer.
- **`lZ`** is the wheel × 120 (`WHEEL_DELTA`), positive away from the user.
- **Buttons.** `rgbButtons[0..2]` are left, right and middle; `[3]` is X1
  (`[3..4]` with `c_dfDIMouse2`). `GetSystemMetrics(SM_SWAPBUTTON)` is 0,
  because macOS has already swapped the buttons when the user asked for it.
- **The cursor.** `SetCursor(NULL)`, which the client calls on
  `WM_SETCURSOR`, and `ShowCursor(FALSE)` hide the system pointer over the
  content area only (`bf_app_set_cursor_hidden_in_window`). Any cursor
  handle shows the arrow again.
- **`SetCursorPos`** never warps the pointer. It records the position, and
  `GetCursorPos` reports that position until the pointer next moves.
- **Coordinates.** "Screen" coordinates are logical client coordinates:
  - `ClientToScreen` and `ScreenToClient` are the identity;
  - `GetWindowRect` equals `GetClientRect`, which is `{0, 0, logical_w, logical_h}`;
  - `AdjustWindowRect` adds no frame;
  - `GetSystemMetrics(SM_CXSCREEN/SM_CYSCREEN)` is the screen in logical
    pixels.

  `Session.cpp` therefore saves `window_left`/`window_top` as 0. The window is
  centred by `bf_app`, and macOS owns its position.

## Names

Implemented (on `bf_app` / `bf_view` / `bf_text` unless said):

| Names | Notes |
|---|---|
| `RegisterClassA`, `RegisterClassExA`, `LoadIconA`, `LoadCursorA` | icon and cursor handles are placeholders (the arrow) |
| `CreateWindowExA`, `DestroyWindow`, `IsWindow`, `FindWindowA` | one window; `bf_app_window_create`, `bf_view_init`; `FindWindowA` is the single-instance test: another instance → `bf_app_init` brings it to the front and this process `exit(0)`s |
| `ShowWindow`, `UpdateWindow`, `ValidateRect`, `InvalidateRect`, `RedrawWindow`, `DrawMenuBar`, `IsIconic` | `SW_MINIMIZE`/`SW_MAXIMIZE`/`SW_HIDE` (fullscreen path, `WM_ACTIVATE`) are ignored: macOS owns the window's state |
| `GetClientRect`, `GetWindowRect`, `AdjustWindowRect(Ex)`, `ClientToScreen`, `ScreenToClient`, `SetWindowPos`, `MoveWindow` | logical coordinates; a size given becomes the content size |
| `SetWindowLongA`, `GetWindowLongA`, `SetWindowLongPtrA`, `GetWindowLongPtrA`, `SetWindowTextA` | style, ex-style, user data, window procedure; the title through `bf_app_window_set_title` |
| `SetFocus`, `GetFocus`, `GetActiveWindow`, `GetForegroundWindow` | the window while it has focus (from `FOCUS` events), else NULL |
| `GetSystemMetrics` | `SM_CXSCREEN`, `SM_CYSCREEN`, `SM_SWAPBUTTON`, `SM_CMOUSEBUTTONS`, `SM_MOUSEPRESENT`; others 0 with a `w32` trace line |
| `PeekMessageA`, `GetMessageA`, `TranslateMessage`, `DispatchMessageA`, `PostMessageA`, `SendMessageA`, `PostQuitMessage`, `DefWindowProcA` | see "How messages are made" |
| `GetAsyncKeyState`, `GetKeyState`, `GetKeyboardState`, `MapVirtualKeyA`, `GetKeyboardLayout`, `GetKeyboardType` | layout 0x04090409 (English, US); type 4 |
| `SetCursor`, `ShowCursor`, `GetCursorPos`, `SetCursorPos` | see "Pointer and cursor" |
| `OpenClipboard`, `CloseClipboard`, `EmptyClipboard`, `GetClipboardData`, `SetClipboardData` | `CF_TEXT` only, CP1252 ⇄ UTF-8, CR LF ⇄ LF; memory through the win32 area's `GlobalAlloc`/`GlobalLock`/`GlobalSize`/`GlobalFree`; the block `GetClipboardData` returns is freed on the next change, the one given to `SetClipboardData` at once |
| `GetDC`, `ReleaseDC` | `GetDC(NULL)` (and any `GetDC(hwnd)`) is `bf_compat_gdi_screen_dc()`; `ReleaseDC` does nothing. The client calls only `GetDC(NULL)` (`DrawUtil.cpp`, for `EnumFontFamiliesEx`) |
| `CharNextExA`, `CharPrevExA`, `CharNextA`, `CharPrevA` | `bf_text_char_length`, `bf_text_char_prev`; code pages 0–3 mean 1252 |
| `wsprintfA`, `wvsprintfA` | `vsnprintf` with wsprintf's 1024-character limit; its `%l` is 32 bits, `%hs` is `%s` |
| `SetRect`, `CopyRect` | |
| `DirectInputCreateA`, `DirectInputCreateEx`, `IDirectInput7A`, `IDirectInputDevice7A` (mouse), `c_dfDIMouse`, `c_dfDIMouse2`, the dinput GUIDs | see "Pointer and cursor" |
| `ImmGetContext`, `ImmReleaseContext` | no context (NULL), which `CLanguage` handles |

Stubbed (`BF_STUB`, report "no IME"; never reached because no context
exists): `ImmGetCompositionStringA`, `ImmGetCompositionStringW`,
`ImmGetCandidateListW`, `ImmNotifyIME`, `ImmGetVirtualKey`. Also stubbed:
the DirectInput mouse's `EnumObjects`, `GetProperty`, `SetProperty`,
`GetDeviceData`, `SetEventNotification`, `GetObjectInfo`, `GetDeviceInfo`,
and `IDirectInput::EnumDevices`.

Not provided, because their only callers are units the port does not build
(`ExceptionHandler`, `LanguageKeyProcess`, `movie/CBink`): `DialogBoxParamA`,
`EndDialog`, `GetDlgItem`. `MessageBox*` is the win32 area's.

## Untested or open

- **Real devices.** Real keyboard and mouse events have not been through the
  adapter: the unattended Mac's screen is locked, and the test injects
  everything. The app module's `--interactive` mode shows what AppKit
  delivers.
- **The client's code has not run on the adapter.** It does not compile yet
  (M2). `WindowProc` returns `long`, while `WNDPROC` returns `LRESULT`; M2.2
  settles that.
- **The `CMouse` belief** can drift from the client's real `m_xPos` in two
  cases:
  - `CMouse::ResetState` (centring) is called; nothing calls it today;
  - `g_3dDevice.GetWidth()` differs from `bf_view`'s logical width while the
    pointer is at the right edge.

  Both are corrected by `bf_compat_input_mouse_set_client_pos`, and stage B
  removes the guess.
- **Injected key-downs type nothing by themselves.** The control socket
  injects text as `TEXT` events (`key_seq` 0), which are always delivered.
- **Korean input** through the client's composition and candidate windows
  is not supported: imm32 reports no IME. macOS input methods still commit
  text, which arrives as `WM_CHAR` in CP1252, so Hangul is dropped.
- **Mouse button and wheel window messages** (`WM_LBUTTONDOWN`,
  `WM_MOUSEWHEEL`) are not sent. The client reads buttons and the wheel
  through DirectInput only.

## For `port/README.md`

> **Input adapter** (`port/compat/input`, M5.1, M5.2). user32's window,
> message pump, keyboard state, cursor and clipboard, DirectInput 7's mouse
> and imm32 (no IME) on `port/native/app` and `port/native/view`. It contains
> the Mac keyboard defaults: Option as Alt, Cmd+Q/F/V/X/C, and Cmd+I as the
> Insert substitute. Test: `cmake --build --preset debug --target
> bifrost-compat-input-test && build/cmake/debug/compat/input/bifrost-compat-input-test`
> (under a second, opens a window, injects events, sets and restores the
> clipboard).
