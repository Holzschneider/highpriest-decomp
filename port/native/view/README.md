# view: logical resolution, UI scale, resize and pointer mapping

The glue between the window (`port/native/app`) and the renderer
(`port/native/gfx`), step M4.3. It is the one place that turns the window's
pixel size, the backing scale and the UI scale setting into the logical
resolution the client draws in. It keeps the device and the window's minimum
size in step, tells the client when the view changes, and maps pointer
positions between window and logical coordinates. The contract is
[`include/bf_view.h`](include/bf_view.h), which states the rule and the
mapping in full.

## The rule

| Setting | UI scale in effect |
|---|---|
| 1, 2, 3 | that number, lowered (3 → 2 → 1) while the logical size would fall below 640×480 |
| auto (0) | the largest of 3, 2, 1 that keeps the logical size at least 800×600, else 1 |

- **Logical size:** `floor(window_px / scale)`, at least 640×480.
- **640×480 is the floor.** It is the smallest mode the 2008 client offered.
- **800×600 is what auto keeps.** It is the size the UI was laid out for.

Examples with auto:

| Window | Pixels | UI scale | Logical |
|---|---|---|---|
| 1024×768 pt on Retina | 2048×1536 | 2 | 1024×768 |
| 1680×720 pt on Retina | 3360×1440 | 2 | 1680×720 |
| 5K fullscreen | 5120×2880 | 3 | 1706×960 |
| 1080p at 1× | 1920×1080 | 1 | 1920×1080 |
| 1440p at 1× | 2560×1440 | 2 | 1280×720 |

With auto the scale is a pure function of the size, so dragging an edge
across a threshold changes the UI scale. A player who does not want that
picks a fixed scale.

**Leftover pixels.**
- The drawable is always the whole window.
- The logical screen covers its top left `logical × scale` pixels. The rest
  (at most `scale − 1` pixels at the right and the bottom) is cleared to
  black by `bf_gfx_present`.
- Nothing is centred, so window pixels and target pixels share one origin.

**Minimum window size.**
- Outside fullscreen it is `640s × 480s` pixels, where s is the setting and
  auto counts as 1. It is converted to points and capped at the screen's
  largest content area.
- A setting that raises the minimum above the current size enlarges the
  window.
- In fullscreen, and where the cap bites, the scale drops instead.

**Pointer.**
- A pointer at window pixel `p` (points × backing) is over the logical pixel
  `floor(p / s)`.
- The centre of logical pixel `x` is at `(x + 0.5) · s`.
- This matches gfx's `n = c·s + 0.5`: a quad over logical pixel x covers
  exactly the native pixels `[x·s, (x+1)·s)`. The test checks this against
  real frames, not only by arithmetic.

## Build and test

```sh
cmake -S port/native/view -B build/native/view -G Ninja
cmake --build build/native/view
build/native/view/bifrost-view-test        # optional argument: output directory
```

**About the run.**
- It takes about 11 s and opens a window. It needs no game data.
- It builds `bf_app` and `bf_gfx` from their directories unless a parent
  project already has those targets.

**Part 1: the rule and the mapping, against hand-worked values.**
- 14 sizes and settings.
- Block borders, the leftover strip, clamping, and the round trip over
  every pixel of a row.

**Part 2: per case, with a real window and a gfx device on its layer.**
The cases are UI scale 1, 2, 3 and auto at 1024×768 pt (4:3) and at
1680×720 pt (wide). The window is resized programmatically. For each case
the test checks:
- the first frame after the resize, before any event is pumped, presents a
  drawable of exactly the window's pixel size;
- the logical size and scale match an independent restatement of the rule,
  and so do the device's sizes;
- the listener fires once, and the `RESIZE` events pumped afterwards change
  nothing;
- in an ID frame, where every logical pixel has its own colour, every
  native pixel lies in its logical pixel's s×s block;
- 59 pointer positions map to the logical pixel the frame shows under them.
  These are corners, block borders, pixel centres and random points, plus
  one injected through the event queue;
- a 1-pixel checkerboard comes out as clean s×s blocks;
- the 1-pixel frame at the logical edges is s native pixels wide.

**Part 3: the minimum size, live resize and fullscreen.**
- **Minimum size:** scale 3 enlarges a 1600×700 pt window to 1600×720.
- **Live resize, simulated:** AppKit's tracking run-loop mode runs with
  the modal frame installed, and the size changes every 20 ms. Every frame
  is presented at the size of the moment.
- **Fullscreen:** real when the screen is unlocked. Otherwise it is
  simulated with an injected `FULLSCREEN` event and a resize to the
  screen's largest content area.

**Output.**
- The captures (`bf_gfx_capture_png`) go to `build/native/view/test-out/`,
  with `contact-sheet.png`.
- [`test/reference/contact-sheet.png`](test/reference/contact-sheet.png) is
  the reviewed sheet (synthetic, generated textures only).
- `test/reference/SHA256SUMS` lists the checksums of the sheet and of the 8
  captures from Apple M2 Ultra, macOS 27.2.1, with the window on a 2×
  screen. The captures were bit-identical between runs.
- On a 1× screen the window's pixel sizes, and with them the captures,
  differ. The numeric checks are the criterion.

To compare a new run:

```sh
cd build/native/view/test-out && shasum -a 256 -c ../../../../port/native/view/test/reference/SHA256SUMS
```

## For the client's resize path (M4.3a) and the input adapters

**Start-up.** `bf_app_window_create`, then `bf_view_init(setting)`. Then
`bf_view_get(&m)` and `bf_gfx_create({layer, m.logical_w, m.logical_h,
m.ui_scale})`. Then `bf_view_attach_gfx(dev)`, and
`bf_view_set_listener(fn, ctx)`.

**Every frame.** Call `bf_view_sync()` first. Pass each event from
`bf_app_poll_event` to `bf_view_handle_event`.

**The listener.** It is where `CRenderer::SetSize`, the projection, the
picking and the window manager's bounds are updated, and saved window
positions are clamped. It runs after the device has been resized. Nothing
may be drawn at the old size afterwards.

**Live resize.** `bf_view_set_modal_frame(draw_one_frame, ctx)`.

**Settings.** `bf_view_set_scale_setting(0..3)`. Cmd+F:
`bf_view_toggle_fullscreen()`.

**Pointer.**
- The DirectInput adapter uses `bf_view_px_to_logical_pixel(&m, x_px,
  y_px, &lx, &ly)` (clamped).
- The control socket aims at a logical pixel with
  `bf_view_logical_pixel_center_px` and posts that as `x_px`/`y_px`.
- `bf_view_get` may be called from any thread.

## Untested or open

- **On the locked screen of the unattended Mac:**
  - **Real fullscreen.** macOS refuses it there. The flow was run with a
    simulated event and size; the test does it for real when the screen is
    unlocked.
  - **A real live resize by dragging the window's edge.** The tracking
    run-loop mode was driven by the test instead.
  - **What the layer shows while AppKit stretches the previous drawable
    during a drag.** The app module sets
    `NSViewLayerContentsPlacementTopLeft`. The look of it is for a person
    to judge.
- **Moving the window between a 1× and a 2× screen.** This changes the
  backing scale and should arrive as `RESIZE` through the same path; not
  tried.
- **Auto's thresholds (800×600) and the 640×480 floor** are decided here
  (spec §3 leaves them open). Revisit once the login screen draws.

## For `port/README.md`

> **View module** (`port/native/view`, M4.3). Logical resolution and UI
> scale (1, 2, 3 or auto) from the window's pixel size, in one place; keeps
> the gfx device and the window's minimum size in step on resize, backing
> scale change and fullscreen; notifies the client; maps pointer positions
> to logical pixels. Test: `cmake -S port/native/view -B build/native/view
> -G Ninja && cmake --build build/native/view &&
> build/native/view/bifrost-view-test` (about 11 s, opens a window, no
> game data; captures and a contact sheet in `build/native/view/test-out/`).
