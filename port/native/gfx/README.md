# gfx: the Metal renderer

Draws the client's pre-transformed vertices with Metal. Contract: [`include/bf_gfx.h`](include/bf_gfx.h). What the client calls and how each call maps onto the module: [`D3D-USAGE.md`](D3D-USAGE.md), the specification of the DirectDraw/Direct3D adapter (M4.4), which M11.9 replaced: `src/3dlib` calls the module directly, the render-target rules are `port/client`'s `bf_render.h`.

## What it offers

- A device on a `CAMetalLayer` or offscreen, with a logical resolution and a UI scale of 1, 2 or 3. Positions are scaled in the vertex shader and rasterised at native resolution; a fixed +0.5 native-pixel offset replaces the client's start-up calibration (`C3dDevice::TestScreen`).
- Textures in A1R5G5B5, A4R4G4B4, R5G6B5 and A8R8G8B8, each with a CPU copy in its own format (lock-style read and write, update, fill, stretch copy), mip levels regenerated on change, uploads ordered with the draws around them.
- State in native terms with Direct3D 7 numbers: two texture stages (all operations the client uses and the rest of the Direct3D 7 set except bump mapping), blend factors, alpha test, depth, cull, vertex fog, address modes, filters. Fragment programs are specialised per texture-stage combination through Metal function constants and cached.
- A drawable size of its own (`bf_gfx_set_drawable_size`): the window's pixels, of which the target covers the top left `logical × scale`; present clears the leftover to black. `bf_gfx_get_present_size` reports the size of the last presented drawable. The view module (`port/native/view`) drives both.
- Draws from caller memory (lists, strips, fans, lines, points; indexed or not), clears, present per display refresh, a persistent target with backup/restore, lazy read-back (logical or native rectangles as 32-bit BGRA), ordered writes into the target, PNG capture through ImageIO.

The shaders (`src/bf_gfx_shaders.metal`) are compiled **at run time** from source embedded at build time: the Metal compiler is not part of the command-line tools on this machine (`xcrun metal` reports a missing Metal Toolchain). Compiling the library takes a few milliseconds at device creation; pipelines are built on first use of a state combination.

## Build and test

```sh
cmake -S port/native/gfx -B build/native/gfx -G Ninja
cmake --build build/native/gfx
build/native/gfx/bifrost-gfx-test            # optional argument: output directory
```

The test needs no window and no game data. It renders offscreen, checks pixels numerically (half-pixel coverage at scales 1–3, the 0.5-texel offset, clean pixel blocks at scales 2 and 3, the four texture formats, alpha test, depth, cull, fan and indexed draws, all 16 blend pairs the client uses and every blend factor, the 16 multitexture presets and the lightmap pass against an independent reference, fog, the unbound-texture rule, address modes, mipmaps, upload ordering, lock and copy, a large draw, read-back and writes, backup/restore, a layer device), writes every capture as a PNG into `build/native/gfx/test-out/`, and a labelled `contact-sheet.png` of all of them. It prints one line per check and exits non-zero on a failure.

`test/reference/contact-sheet.png` is the contact sheet of the reviewed run (synthetic, generated textures only); `test/reference/SHA256SUMS` lists the checksums of it and of every capture of that run (Apple M2 Ultra, macOS 27.2.1). To compare a new run:

```sh
cd build/native/gfx/test-out && shasum -a 256 -c ../../../../port/native/gfx/test/reference/SHA256SUMS
```

The captures are bit-identical between runs on one machine. Another GPU or OS version may round differently in the last bit of a filtered pixel and the contact sheet's text may render differently; the numeric checks are the criterion, the picture is for review.

## Untested or open

- Present to a layer in a visible window: the test presents to a window-less layer only (it vends drawables, so `bf_gfx_present` runs its full path). Pacing: display sync alone did not hold a windowed layer to the display (the window server returned drawables 120 times a second on 60 Hz displays, F7); the client sets `bf_gfx_set_frame_interval` to the window's screen's refresh interval, and `m4_frames` measures the result.
- Behaviour under memory pressure and with thousands of textures (the client creates ~50 atlases at start and loads textures as maps need them).
- Sprites drawn on top of the UI (cursor, numbers) at UI scale 2–3: see D3D-USAGE.md section 8.
- The fog colour: the client never sets it (`CRenderer::DoFog` is not called), so fog fades to black; see D3D-USAGE.md section 3.

## For port/README.md

> **Graphics module** (`port/native/gfx`): the Metal renderer for the client's pre-transformed vertices. Build and test with `cmake -S port/native/gfx -B build/native/gfx -G Ninja && cmake --build build/native/gfx && build/native/gfx/bifrost-gfx-test`; it renders offscreen, checks pixels numerically and writes its captures and a contact sheet to `build/native/gfx/test-out/`. The client's Direct3D usage, which the adapter has to cover, is in `port/native/gfx/D3D-USAGE.md`.
