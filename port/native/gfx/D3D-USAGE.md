# How the client uses DirectDraw 7 and Direct3D 7

This is the specification of the DirectDraw/Direct3D adapter (M4.4): every call, state and value the client's renderer uses, read from `src/3dlib/` (all files) and from the rest of `src/` (`grep` for `SetRenderState`, `SetTextureStageState`, `D3DBLEND_`, `D3DTOP_`, `DrawPrimitive`, `DrawIndexedPrimitive`, `Lock`, `Blt`, `GetDC`), and how each maps onto `include/bf_gfx.h`. Numbers in parentheses are the Direct3D 7 values (the SDK headers are under `tools/dx8sdk/Include/`); the module's enumerations carry the same numbers.

Outside `src/3dlib/`, only `movie/CBink.cpp` touches DirectDraw (stubbed in the port), and game code passes Direct3D numbers as data: `D3DBLEND_*` in `RagEffect2.cpp`, `GameActor.cpp`, `3dGround.cpp`, `resource/Skybox.cpp`, and blend and preset numbers inside `.str` effect files.

## 1. Device and frame

| Client call (where) | What it does | Module |
|---|---|---|
| `C3dDevice::Init` → `CreateDirectDraw`, `CreateBuffers`, `CreateDirect3D`, `CreateZBuffer` (`Device.cpp`) | Windowed: primary surface with clipper, an offscreen back buffer `OFFSCREENPLAIN \| 3DDEVICE` of the client-area size, a 16-bit z-buffer (`EnumZBufferFormats` stops at the first 16-bit format), a system-memory surface `m_pddsRenderBackup` of the same size, viewport = whole target (`D3DUtil_InitViewport`, z 0..1). Fullscreen: flipping chain with one back buffer. | `bf_gfx_create` (target + 32-bit float depth, one persistent target; fullscreen is the application module's job). |
| Then sets the initial states | see sections 3 and 4 | `bf_gfx_set_state` |
| `CreateRenderer` | reads the frame-buffer bit masks → `PF_R5G6B5`, `PF_R5G5B5` or `PF_A8R8G8B8`; `GetCaps` for `DDCAPS_BLTSTRETCH`, texture size limits, `wMaxTextureBlendStages >= 2 && MODULATEALPHA_ADDCOLOR` (→ `g_isSupportLMTex`, the single-pass lightmap path), `LINEARMIPLINEAR` (mipmaps if the trilinear option is on), guard band (unused: `m_guardBandLeft` is always 0 there, so ±640/±480 is used); then `TestScreen()` | The adapter reports a 32-bit X8R8G8B8 frame buffer, `BLTSTRETCH`, 2 blend stages, all texture-op caps below, `FOGVERTEX`, `LINEARMIPLINEAR`, max texture 16384 (`bf_gfx_max_texture_size`), min 1, aspect ratio 0 (no limit). |
| `TestScreen` | draws 65,536 4×4 quads with sub-texel UV offsets, reads them back, picks the sharpest offset → `CTexture::m_uOffset/m_vOffset` | **Not called.** The module's +0.5 native-pixel offset puts pixel centres where Direct3D 7 had them, so the right texel offset is exactly 0.5 texel, the value `Texture.cpp` initialises it to. `bifrost-gfx-test` checks this ("texel offset 0.5 + bilinear"). |
| `Clear(color)` | `Clear(TARGET \| ZBUFFER, color, 1.0)` over the whole target; called with `0xFF000000` (`CRenderer::Clear(TRUE)`) or `m_nClearColor` (`ClearBackground`) | `bf_gfx_clear(COLOR \| DEPTH, color, 1, NULL)` |
| `ClearZBuffer()` | `Clear(ZBUFFER, 1.0)` (`CRenderer::Clear(FALSE)`) | `bf_gfx_clear(DEPTH, 0, 1, NULL)` |
| `BeginScene` / `EndScene` (`CRenderer::DrawScene`) | brackets `FlushRenderList` | `bf_gfx_begin_frame` (optional) / nothing; draws work at any time |
| `CRenderer::Flip(vsync)` | `Lock(); Unlock();` of the render surface (no pixel touched), then `ShowFrame(vsync)` | Make that lock lazy: no read-back. `bf_gfx_present` (layer) or `bf_gfx_end_frame` (offscreen). |
| `ShowFrame(vsync)` | windowed: `WaitForVerticalBlank` + `Blt` back → front; fullscreen: `Flip(DDFLIP_WAIT)`. Also called once from `WinMain.cpp:390` | `bf_gfx_present` (paces on the drawable) |
| `CloneFrame` | `Blt` front → back | nothing: the module's target already holds the last frame |
| `BackupFrame` / `RestoreFrame` (`Mode.cpp` fade-out: backup once, then restore + `DrawBoxScreen` every step) | `Blt` render target ↔ system-memory surface | `bf_gfx_backup_frame` / `bf_gfx_restore_frame` (GPU copies, no read-back) |
| `RestoreSurfaces` (`LoginMode.cpp`, `GameMode.cpp` on reactivation) | `RestoreAllSurfaces` | nothing (Metal loses no surfaces) |
| `EnableClipper`, `FlipToGDISurface`, `GDIFlip` | fullscreen/GDI interplay | nothing |
| `GetFrontBuffer` / `GetBackBuffer` + `Lock` + `Blt` (`CBink::BinkShowNextFrame`) | Bink writes a 512×347 frame into the back buffer and blits it to the window | `bf_gfx_write_pixels` (Bink is a stub in the port) |

## 2. Vertices and primitives

| Client structure | FVF | Layout | Module |
|---|---|---|---|
| `tlvertex3d` (`Prim.h`), drawn by `RPFace::DrawPri` | `0x1C4` = `XYZRHW \| DIFFUSE \| SPECULAR \| TEX1` (`D3DFVF_TLVERTEX`) | x, y, z, rhw (`oow`), diffuse ARGB, specular ARGB, tu, tv — 32 bytes | `BF_GFX_VERTEX_TL1` |
| `lmtlvertex3d`, drawn by `RPLmFace::DrawPri` (ground with lightmap) | `0x2C4` = the same with `TEX2` | + lightmap u, v — 40 bytes | `BF_GFX_VERTEX_TL2` |

- Calls: `DrawPrimitive(primType, FVF, verts, n, 0)` and `DrawIndexedPrimitive(primType, FVF, verts, n, indices, ni, 0)` from user memory, flags 0 (clipping on). Indexed draws happen only when `numIndices != 0`, which no code in the current source sets; the module supports them anyway.
- Primitive types: `D3DPT_TRIANGLELIST` (4) (`RPFace` default, `RPMesh`, `RPTriFace`) and `D3DPT_TRIANGLESTRIP` (5) (`RPQuadFace`, `RPLmQuadFace`, `TestScreen`). Nothing else.
- `z` is in 0..1 (`GetZFromOow`; UI quads use 0.00001 or `1/32236`), `oow` = 1/w is used for perspective-correct interpolation (`TEXTUREPERSPECTIVE` is on). x, y are pixels of the logical resolution (the client's `m_width`, `m_height`).
- `DrawPrimitiveVB` / `DrawIndexedPrimitiveVB` with `CDynamicVB` and `SetTransform(WORLD)` (`RPRaw`, `CDynamicVB::DrawPri`) are dead code: nothing calls `AddRawRP` or `CDynamicVB::Init`. The adapter can leave them unimplemented (trace once). `SetLight`/`LightEnable` (`CRenderer::SetLight`) and `SetTransform` in `DoFog` have no effect on pre-transformed vertices.

## 3. Render states

| State (number) | Values used, where | Module field |
|---|---|---|
| `CULLMODE` (22) | `NONE` (1) in `Init` and after the lightmap pass; `CW` (2) for the ground lightmap pass (`FlushRenderList`) | `cull`; CW means clockwise on the client's y-down screen, as in Direct3D |
| `ZFUNC` (23) | `LESSEQUAL` (4), `Init` only | `depth_func` |
| `ZENABLE` (7) | `TRUE` in `Init` and at the start of `FlushRenderList`; `FALSE` (`D3DZB_FALSE`) before the no-depth lists (UI); `FALSE` in `TestScreen` | `depth_enable` |
| `ZWRITEENABLE` (14) | `TRUE`; `FALSE` from the non-depth-write alpha lists on | `depth_write` |
| `ALPHATESTENABLE` (15) | `TRUE` from `FlushRenderList` on, never turned off (except `TestScreen`) | `alpha_test_enable` |
| `ALPHAFUNC` (25) | `GREATEREQUAL` (7), `Init` only | `alpha_func` |
| `ALPHAREF` (24) | `0xF0` (`Init`), `0xCF` (opaque lists), `TRUE` = 1 (alpha lists, written as `ALPHAREF, TRUE`) | `alpha_ref`. Compared on the 8-bit alpha: A4R4G4B4 alpha 0xC (204) fails 0xCF, 0xD (221) passes. |
| `ALPHABLENDENABLE` (27) | `FALSE` for opaque lists, `TRUE` for the rest | `blend_enable` |
| `SRCBLEND` (19), `DESTBLEND` (20) | fixed per list in `FlushRenderList`, per face in `FlushEffectList` (from `RPFace::srcAlphaMode/destAlphaMode`) — see section 6 | `src_blend`, `dst_blend` |
| `FOGENABLE` (28) | `CRenderer::FogSwitch(sw)` (map has a fog entry in `fogparametertable.txt`, `GameMode.cpp:5509`); turned `FALSE` before the emissive lists | `fog_enable` |
| `FOGCOLOR` (34), `FOGSTART/END/DENSITY`, `FOGTABLEMODE`, `FOGVERTEXMODE`, `RANGEFOGENABLE` | only in `CRenderer::DoFog`, **which nothing calls** | `fog_color` stays 0 (black) unless the client is changed to set it — a reconstruction gap to check against the reference executable |
| `AMBIENT` (139) | `0xFFFFFFFF` / 0 around the lightmap pass | no effect on pre-transformed vertices; ignore |
| `TEXTUREPERSPECTIVE` (4) | `TRUE` (`Init`) | always on |
| `COLORKEYENABLE` (41) | `TRUE` (`Init`) | ignore: texture colour keys are turned into alpha when textures are filled (section 7), no texture carries a key |
| `SPECULARENABLE` (29), `TEXTUREFACTOR` (60), `DITHERENABLE`, `SHADEMODE` | never set (defaults: no specular add, factor `0xFFFFFFFF`, Gouraud) | `specular_enable` 0, `texture_factor` |

Fog: for pre-transformed vertices Direct3D takes the fog factor from the specular alpha. When the device reports `FOGVERTEX` (the adapter does), `AddRP`/`AddLmRP` call `InitSpecular`, which writes `GetSpecularFromOow(oow)` (factor in the top byte) into every vertex. Final colour = f · colour + (1 − f) · fog colour: `fog_enable`.

## 4. Texture stage states

| State (number) | Values used | Module field |
|---|---|---|
| `COLOROP` (1), `ALPHAOP` (4) | Init: stage 0 `MODULATE` (4)/`MODULATE`; stage 1 `MODULATEALPHA_ADDCOLOR` (18)/`DISABLE` (1); stage 2 `DISABLE`/`DISABLE`. `FlushRenderList`: stage 0 `MODULATE`/`MODULATE`, stage 1 `DISABLE`/`DISABLE`; stage 1 `MODULATEALPHA_ADDCOLOR` around `FlushLmList`; fallback lightmap path (no 2-stage caps, never taken with this adapter): stage 0 `SELECTARG1` (2). Presets: section 5. | `stage[i].color_op`, `alpha_op` |
| `COLORARG1/2` (2, 3), `ALPHAARG1/2` (5, 6) | `TEXTURE` (2), `DIFFUSE` (0), `CURRENT` (1), with `COMPLEMENT` (0x10) and `ALPHAREPLICATE` (0x20) in presets | `color_arg1/2`, `alpha_arg1/2` |
| `TEXCOORDINDEX` (11) | stage 0 → 0, stage 1 → 1 | `texcoord_index` |
| `ADDRESS` (12), `ADDRESSV` (14) | stage 0: `ADDRESSV = WRAP` (1) then `ADDRESS = CLAMP` (3), so both clamp; stage 1: `BORDER` (4) | `address_u`, `address_v` |
| `BORDERCOLOR` (15) | stage 1: `0xFF000000` | `border_color` (opaque black, exact) |
| `MAGFILTER` (16), `MINFILTER` (17) | `LINEAR` (2) for stages 0 and 1, set in `Init` and again at the end of `TestScreen`. **Never `POINT`** anywhere. | `mag_filter`, `min_filter` |
| `MIPFILTER` (18) | `LINEAR` (3) at the start of `FlushRenderList`, `NONE` (1) at its end (so: on for everything the client draws, since the UI lists are flushed inside) | `mip_filter`; only matters for textures created with mip levels (trilinear option) |

Stage 2 is only ever disabled; the module has two stages.

`ALPHAOP = DISABLE` on a stage whose colour operation is enabled (stage 1 in the lightmap pass) is undefined in the Direct3D documentation; drivers pass the current alpha through, and so does the module.

## 5. The 16 multitexture presets

Defined in `CRenderer::SetMultiTextureMode(int)` (`src/3dlib/Renderer.cpp`, the `switch` at about line 1290). Selected per effect face: `.str` files store `mtpreset` in each position keyframe (`KAC_XFORMDATA::mtpreset`, `resource/EZeffect.h`), `CRagEffect::RenderAniClip` copies it into `RPFace::mtPreset` and adds the face with `RF_EFFECT`, and `CRenderer::FlushEffectList` calls `SetMultiTextureMode(rp->mtPreset)` before each draw. `RPTriFace` faces get preset 0. T = stage 0 texture, T1 = stage 1 texture, D = diffuse, c = stage 0 result; "keeps" = a field the preset does not set and inherits.

| # | Stage 0 colour; alpha | Stage 1 (texcoord 1) colour; alpha |
|---|---|---|
| 0 | T·D; T·D | off |
| 1 | T·D; T | off |
| 2 | T·D; T (identical to 1) | off |
| 3 | T+D; D | off |
| 4 | `BLENDTEXTUREALPHA`(T, D); D | off |
| 5 | T·D; D | T1·c; T1 |
| 6 | (1−T)·D; D | (1−T1)·c; T1 |
| 7 | T (`SELECTARG1`, arg2 keeps); T | T1.aaa·c; T1 |
| 8 | T·D; T | T1+c; T1 |
| 9 | T·D; D | `BLENDTEXTUREALPHA`(T1, c); T1 |
| 10 | T·D; D | `BLENDDIFFUSEALPHA`(T1, c); T1 |
| 11 | T·D; D | T1+c; T1 |
| 12 | T·D; D | (1−T1)+c; T1 |
| 13 | T+D; T | T1·c; T1 |
| 14 | T·D; D | `MODULATE2X`(T1, c); T1 |
| 15 | T·D; D | `ADDSIGNED`(T1, c); T1 |

Every preset is a combination of module operations; `bifrost-gfx-test` draws each one with two generated textures and checks all 4096 pixels of colour and alpha against an independent reference.

What the game data uses: of the 1443 readable `.str` files in `rdata.grf` and `data.grf` (285 more are DES-encrypted and were not read), position keyframes use preset **0** (51,717 keys) and **1** (60 keys, all with blend `SRCCOLOR`/`DESTCOLOR`) only. Counted by `tools/str_blend_census.py ~/Games/Bifrost/rdata.grf ~/Games/Bifrost/data.grf`, which reads `srcalpha`, `destalpha`, `mtpreset` of every position keyframe (`KAC_KEYFRAME` is 124 bytes; the three fields are at offsets 112, 116, 120).

Quirks the adapter reproduces by passing states through literally:

- Effect faces are `tlvertex3d` (one texture coordinate set). Presets 5–15 read stage 1 at texcoord set 1, which such a vertex does not have: the module reads (0, 0), as Direct3D does for a missing set. The stage 1 texture is whatever `SetLmapTexture` left bound: the last lightmap of `FlushLmList` (with lightmaps on) or none (`FlushFaceList` ends with `SetLmapTexture(NULL)`). The data never selects these presets.
- A preset stays in force after `FlushEffectList`: the no-depth lists (UI) are drawn with the last effect's preset. Preset 1 (alpha from the texture only) would make vertex alpha ineffective for the UI drawn after it.

## 6. Blend modes in use

Fixed pairs (`CRenderer::FlushRenderList`): opaque lists without blending; `SRCALPHA`/`INVSRCALPHA` (5/6) for alpha lists and UI; `ONE`/`ONE` (2/2) for emissive lists; `ONE`/`SRCALPHA` (2/5) for the fallback lightmap path. Per-face pairs (effect list only; for other lists the face fields are ignored): `GameActor.cpp:1813` 5/2 or 5/6; `RagEffect2.cpp` 5/2 or 5/6; `3dGround.cpp:281` 5/6; `resource/Skybox.cpp` 2/2 and 5/6; and from `.str` files:

| src / dst | keys | | src / dst | keys |
|---|---|---|---|---|
| 5 / 7 `SRCALPHA` / `DESTALPHA` | 41,579 | | 2 / 1 `ONE` / `ZERO` | 65 |
| 5 / 6 `SRCALPHA` / `INVSRCALPHA` | 9,409 | | 1 / 6 `ZERO` / `INVSRCALPHA` | 46 |
| 6 / 6 `INVSRCALPHA` / `INVSRCALPHA` | 238 | | 2 / 2 `ONE` / `ONE` | 30 |
| 3 / 9 `SRCCOLOR` / `DESTCOLOR` | 244 (60 with preset 1) | | 5 / 9 `SRCALPHA` / `DESTCOLOR` | 27 |
| 5 / 5 `SRCALPHA` / `SRCALPHA` | 94 | | 1 / 3 `ZERO` / `SRCCOLOR` | 24 |
| 2 / 6 `ONE` / `INVSRCALPHA` | 8 | | 5 / 2 `SRCALPHA` / `ONE` | 6 |
| 3 / 6 `SRCCOLOR` / `INVSRCALPHA` | 4 | | 2 / 7 `ONE` / `DESTALPHA` | 2 |
| 5 / 8 `SRCALPHA` / `INVDESTALPHA` | 1 | | | |

`DESTALPHA` is by far the most common destination factor in effects. The 2008 client's frame buffer (16-bit R5G6B5 or 32-bit X8R8G8B8) has no alpha, so destination alpha reads as 1: `SRCALPHA`/`DESTALPHA` is additive. The module never writes target alpha and maps `DESTALPHA` to 1 and `INVDESTALPHA` to 0. All 16 pairs and every one of the 13 factors as source and as destination are checked numerically in `bifrost-gfx-test`. The blend operation is always add.

## 7. Textures

| Client code | DirectDraw use | Module |
|---|---|---|
| `CTexture::Create` | `EnumTextureFormats` for `A1R5G5B5` (alpha mask 0x8000), `A4R4G4B4` (0xF000), `R5G6B5`, `A8R8G8B8`; `CreateSurface(TEXTURE, TEXTUREMANAGE)`, mip chain if the trilinear option is on (`g_ddsCapsMipmap`), size rounded up to a power of two by `AdjustTextureSize` | `bf_gfx_texture_create(w, h, format, mips)`; keep the client's power-of-two rounding (its UV math, `GetUAdjust`, relies on it) |
| `CTexMgr::LoadTexture` | `A4R4G4B4` for images with alpha (`m_isAlpha`, TGA), else `A1R5G5B5`; missing file → 1×1 transparent black | — |
| sprite cache (`CRenderer::AddSpriteIndex`) | 50 pre-made + on demand 256×256 `A1R5G5B5` atlases | — |
| `CTexMgr::s_dummy_texture` | constructed without a surface: `SetTexture(0, NULL)` | `bf_gfx_set_texture(dev, 0, NULL)`: texture reads opaque white, which `DrawBoxScreen` (fades) needs |
| `CTexture::Update` | `Lock(rect, WRITEONLY)`, converts 32-bit ARGB to the format: magenta (`0xFC1F` / `0x0F0F` after conversion) → 0, with `blackkey` also black → 0; then `UpdateMipmap` (`Blt` of the whole level 0 into each level) | `bf_gfx_texture_update` (or lock/unlock); mips regenerate automatically |
| `CTexture::UpdateSprite` | `Lock(NULL)` whole surface, writes palette-expanded RLE rows | `bf_gfx_texture_lock` / `unlock(rect)` |
| `CTexture::ClearSurface` | `Blt(COLORFILL)`; note the fill value mixes the shifts (R and B swapped); only called with 0 | `bf_gfx_texture_fill` |
| `CTexture::CopyTexture` (`Skill.cpp:1207`, a 32×32 glyph stretched to `gap`×32) | `Blt(dst, src, srcRect)` stretch | `bf_gfx_texture_copy` (nearest) |
| `CDCSurface::BltTexture2`, `CDCBitmap::BltTexture2` | **reads** texels: `Lock` of an `A1R5G5B5` texture (and `A4R4G4B4` in the `_DX9` variant) | `bf_gfx_texture_lock` returns the CPU copy, always current |
| `CTexture::CreateBump` (`PF_BUMP`, `DUDV` format) | never requested by any caller | not supported |

The colour key never reaches the GPU: `Update` writes 0 (alpha 0) for key colours, and `A1R5G5B5`/`A4R4G4B4` carry that alpha. Widening of 16-bit texels: section "textures" of `bf_gfx.h`.

## 8. Filtering, and which draws are unfiltered

Settled from the source: **the client never asks for point sampling.** Both stages are `LINEAR`/`LINEAR` from `Init` on (and again at the end of `TestScreen`), for the world, the UI and text alike. The original UI was pixel-exact only because at a 1:1 scale, with the calibrated 0.5-texel offset, bilinear filtering samples texel centres exactly (`bifrost-gfx-test`: "bilinear = exact texels"). At UI scale 2 or 3 that no longer holds, so the decision moves to the adapter, by texture kind, as the technical plan (3.7) foresaw:

- `BF_GFX_FILTER_NEAREST` via `bf_gfx_texture_set_filter_override` for the tiles of `CDCBitmap::UpdateSurface` (every UI window, its text and the icons blitted into it, drawn 1:1 by `CDCBitmap::DrawSurface` → `CTexture::DrawSurface`, uv offset 0.5). These come out as clean blocks at every scale (checked).
- Everything else keeps the stage state (`LINEAR`): world, sprites, effects, and the stretched UI images (`DrawSurfaceStretch`: wallpaper, minimap, world map), which were filtered in the original too.
- Open for M4.3/M4.4: sprites drawn on top of the UI (mouse cursor, drag image, damage numbers) go through the 256×256 sprite atlases shared with the world and are drawn with UVs on texel corners. If they look soft at scale 2–3, the smallest client change is to set `MINFILTER`/`MAGFILTER` to `POINT` around `FlushAlphaNoDepthList` in `FlushRenderList` (that list is `RF_UI`); the module needs no change for that.

## 9. Surfaces and the render target

| Client code | DirectDraw use | Module |
|---|---|---|
| `CSurface::Create` | offscreen plain surface in frame-buffer format, source colour key = magenta | adapter keeps such surfaces in CPU memory (32-bit) |
| `CSurface::Update`, `CSurfaceWallpaper::Update` | `Lock` + write, skipping magenta | CPU |
| `CSurface::DrawSurface` | `BltFast(SRCCOLORKEY)` onto the render target | `bf_gfx_write_pixels(LOGICAL, ..., SKIP_TRANSPARENT)` with key pixels given alpha 0 |
| `CSurface::DrawSurfaceStretch` | `Blt` stretch onto the render target | stretch on the CPU, then `bf_gfx_write_pixels`; only reached for wallpapers larger than the texture limit, i.e. never with 16384 |
| `CDCSurface::CopyRect` | `BltFast(SRCCOLORKEY)` between surfaces | CPU |
| `CDCSurface::GetDC` on the render target (`CRenderer::TextOutScreen`, `GetTextSize`; callers: `ShowInfo` debug display and the `HP_*` single-player files) | GDI text straight onto the back buffer | text module into a bitmap, then `bf_gfx_write_pixels(..., SKIP_TRANSPARENT)` |
| `CRenderer::Lock`/`Unlock` | `Lock` of the render target (`m_lpSurfacePtr`, `m_lPitch`, `m_bRGBBitCount`): `Flip` (touches nothing), `ScreenEffect` (row shifts of the whole frame, read-modify-write, only while `g_se_go > 0`), `SaveBmp`/`SaveJPG` (screenshots, read) | lazy: `bf_gfx_read_pixels(LOGICAL)` when the client actually reads, `bf_gfx_write_pixels(LOGICAL)` on unlock after a write; `Flip`'s empty lock costs nothing. Screenshots may prefer `NATIVE` / `bf_gfx_capture_png`. |

Read-back happens only through `bf_gfx_read_pixels` and `bf_gfx_capture_png`; the test checks that nothing else reads.

## 10. Sizes and resolution

The client sees the logical resolution (`C3dDevice::GetWidth/GetHeight`, `CRenderer::SetSize`), which is the window's pixel size divided by the UI scale. The module rasterises at native resolution with the +0.5 native-pixel offset described in `bf_gfx.h`; quads on integer coordinates cover exact `scale × scale` blocks (checked at 1, 2, 3). `bf_gfx_resize` follows a window resize, driven by the view module (`port/native/view`, M4.3), which computes the logical size and scale in one place and sets the drawable to the window's pixel size; the client's own resize path (projection, picking, window bounds) is M4.3a.

## 11. What the module does not express

- Bump mapping (`BUMPENVMAP`, `BUMPENVMAPLUMINANCE`, `DUDV` textures): unused by the client; the operations act as `DISABLE`.
- `PREMODULATE`: unused; acts as `SELECTARG1`.
- 16-bit rendering artefacts of the original: the target is 8-bit per channel, depth is 32-bit float instead of 16-bit (less z-fighting than in 2008), no dithering.
- Hardware colour keying and a stencil buffer: unused.
- Border colours other than transparent black, opaque black, opaque white (Metal's set); the client uses opaque black.
- Fog other than the per-vertex factor in specular alpha (table fog, `FOGSTART`/`END` evaluation): only reachable through `DoFog`, which nothing calls.
