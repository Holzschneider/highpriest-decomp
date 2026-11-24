# compat/gdi: gdi32 on port/native/text (stage A)

The gdi32 names the client calls, put on `port/native/text` (`bf_text.h`)
so that `DrawDC`, `FontInfo`, `CDCBitmap` and `UIWindow::DrawTriangleDC`
compile and run unchanged. M11 deletes this directory and moves those call
sites onto `bf_text` directly. The specification is
[port/native/text/GDI-USAGE.md](../../native/text/GDI-USAGE.md): which
names, with which arguments, and how the client uses the results.

## Build and test

```
cmake --preset debug
cmake --build --preset debug --target bf_compat_gdi bifrost-compat-gdi-test
build/cmake/debug/compat/gdi/bifrost-compat-gdi-test
```

Headless, no game data; `port/tools/fastcheck.sh` runs it (ctest label
`headless`). It calls the names in the client's sequences: the font of
every face, height and weight the client asks for, made through
`FontInfo::GetFontFace` (`EnumFontFamiliesEx` on `GetDC(NULL)`) and
`CreateFont`; `DrawDC`'s save / `SetBkMode(TRANSPARENT)` / `SetTextColor` /
`MultiByteToWideChar` / `SelectObject` / `TextOutW` /
`GetTextExtentPoint32W` / restore; `CDCBitmap`'s top-down section and its
resize; a bottom-up section; `DrawTriangleDC`; a surface DC as the d3d
adapter makes one; and wrong handles of every kind. It checks pixels
numerically: only the colour key or exactly the text colour as `0x00RRGGBB`
(alpha 0); ink inside the extent box; `cy` is the cell for any string;
prefix extents add up; text drawn in two pieces at the first piece's extent
equals text drawn whole; `TextOutA` equals `MultiByteToWideChar` +
`TextOutW` for CP949 and CP1252; every object freed at the end. It fakes
the two names of other areas it needs (`MultiByteToWideChar` on
`bf_text_to_utf16`, `GetDC(NULL)` returning `bf_compat_gdi_screen_dc()`).
It writes `gdi-sample.png` (a status-window-like bitmap of generated text,
magenta keyed to transparent as the upload does) to `BF_TEST_OUT` or the
build directory and compares it with `test/reference/gdi-sample.png`
(SHA-256 in `gdi-sample.png.sha256`); a difference is a note, not a failure.

## Names

| Name | Client caller | Here → native |
|---|---|---|
| `CreateCompatibleDC` | `CDCBitmap` | a memory DC with GDI's defaults: the 1×1 default bitmap, `SYSTEM_FONT`, white brush, black pen, black text, white background, `OPAQUE` |
| `DeleteDC` | `CDCBitmap` | frees the DC; objects deleted while selected go with it |
| `CreateDIBSection` | `CDCBitmap::CreateDCSurface` | 32 bpp `BI_RGB`/`BI_BITFIELDS`, top-down or bottom-up, zeroed memory, stride `w*4` (→ `bf_text_bitmap`). Other depths and file-mapping sections: trace, `BF_STUB`, NULL |
| `SelectObject` | `CDCBitmap`, `DrawDC` | bitmap (memory DCs only; one DC at a time, as GDI), font, brush, pen; returns the previous object. Regions (`SelectClipRgn`): `BF_STUB`, NULL |
| `GetCurrentObject` | `DrawDC` | font, bitmap, brush, pen, palette |
| `DeleteObject` | `CDCBitmap`, `FontMgr`, `DrawTriangleDC` | frees; a selected object is freed when deselected (as Windows); stock objects: TRUE, kept; DCs refused |
| `GetStockObject` | `WinMain` (`BLACK_BRUSH`) | brushes, pens, `DEFAULT_PALETTE`, the seven stock fonts (made in `bf_text` when first drawn with) |
| `CreateFontA` | `FontInfo::Create` | → `bf_text_font_create` with `{face, height, weight, italic, underline, strikeout, charset}`; the face converted to UTF-8 from the charset's code page (`bf_text_to_utf8`). Width, escapement, orientation: traced, ignored; quality: always unsmoothed |
| `CreateFontIndirectA` | `FontInfo::CreateEotFont` (dead code) | as `CreateFontA`; the Rix\*.eot faces fall back by charset in `bf_text` |
| `EnumFontFamiliesExA` | `FontInfo::GetFontFace` | the installed faces of `bf_text_face_mapping` under the character sets an English Windows XP lists them for (Gulim/Dotum/Batang: Hangul and ANSI; MS Gothic, SimSun, MingLiU likewise; Arial, Verdana, Times, Courier, Tahoma: the Latin, Cyrillic, Greek, Turkish, Baltic, Vietnamese sets, Tahoma also Thai). Any DC, NULL included. So the client gets the NT name of each face. Returns the last callback value, 1 when nothing was enumerated. An `lParam` below 4 GB is no pointer (the client's face table read past its end for font types ≥ 11): the callback is not called, the answer is 1 ("not found") and the client passes a NULL face |
| `SetBkMode` | `DrawDC` | `TRANSPARENT`/`OPAQUE`, returns the old mode |
| `SetBkColor` | — (not called) | stored; used by `OPAQUE` text |
| `SetTextColor` | `DrawDC::SetTextColor` | stored; drawn as `0x00RRGGBB` |
| `TextOutW` | `DrawDC::_textout` | → `bf_text_draw(..., BF_TEXT_CP_UTF16, ...)`, `TA_LEFT|TA_TOP` |
| `TextOutA` | — (DrawDC converts itself) | → `bf_text_draw` in the code page of the font's charset (`bf_text_codepage_for_charset`) |
| `GetTextExtentPoint32W` | `DrawDC::_gettextsize` | → `bf_text_extent` (UTF-16) |
| `GetTextExtentPoint32A` | — | → `bf_text_extent` in the font's code page |
| `CreatePolygonRgn` | `DrawTriangleDC` | the points and the fill mode; fewer than 3 points: an empty region |
| `CreateSolidBrush` | `DrawTriangleDC` | |
| `FillRgn` | `DrawTriangleDC` | → `bf_text_fill_polygon(..., winding = mode == WINDING)` |

Not provided because no caller exists (GDI-USAGE.md): `GetTextMetrics`,
`GetObject`, `GdiFlush`, `BitBlt`, `StretchBlt`, `PatBlt`, `ExtTextOut`,
`DrawText`, `SetTextAlign`, `GetCharWidth*`. Names of other areas next to
the text code: `GetDC`/`ReleaseDC` (user32, input area), `MultiByteToWideChar`,
`WideCharToMultiByte`, `IsDBCSLeadByte` (kernel32, win32 area),
`CharNextExA`/`CharPrevExA` (user32, input area); their `bf_text` functions
are in GDI-USAGE.md.

## Port-side functions (`include/bf_compat_gdi.h`)

- `HDC bf_compat_gdi_dc_for_bitmap(void *bits, int w, int h, int pitch, int bottom_up)`:
  for the d3d adapter's `IDirectDrawSurface7::GetDC` (`CDCSurface`: the FPS
  counter, a mode message, `GetTextSize`). The adapter keeps a 32-bit staging
  bitmap (B,G,R,A bytes) for the surface, makes a DC over it on `GetDC` and
  `DeleteDC`s it on `ReleaseDC`; a fresh DC each time gives GDI's defaults,
  like a real surface DC. Text writes `0x00RRGGBB` and touches nothing else,
  so the adapter may pre-fill the staging bitmap with a key (or the
  surface's contents) and upload the changed pixels after `ReleaseDC`.
- `BOOL bf_compat_gdi_dc_take_dirty(HDC, RECT *)`: the rectangle written
  since the last call, to upload only that.
- `HDC bf_compat_gdi_screen_dc(void)`: what the input area's `GetDC(NULL)`
  returns (its `ReleaseDC` does nothing with it). `EnumFontFamiliesEx`
  accepts NULL too, so a NULL from `GetDC` also works.
- `BOOL bf_compat_gdi_brush_color(HBRUSH, COLORREF *)`: the colour of
  `WNDCLASS.hbrBackground` (the client registers `BLACK_BRUSH`).
- `int bf_compat_gdi_live_objects(void)`: objects the client has not
  deleted, for leak checks.

Handles are pointers to tagged objects kept in a set of live handles: a
handle of the wrong type, a deleted handle or a stray pointer is traced
(`BFT_ERR|BFT_W32`, counted as `gdi.bad_handle`) and fails as GDI does. All
calls take one lock; the client uses GDI from its main thread only.

## Open

- The client's own code passes `(LONG)fontFace9x[fontType]` as `lParam`,
  which truncates the pointer on arm64; M2 widens it to `LPARAM` (a hard
  compile error until then). Font types 20–28 index past both face tables
  in `FontInfo::GetFontFace` (no caller uses them today).
- `CDCBitmap` uploads its bitmap as `PF_A1R5G5B5` tiles after setting every
  alpha to 0xFF; that is the d3d adapter's and gfx's business.
- Glyph shapes and widths are `bf_text`'s (see its README "Untested and
  open").

## For port/README.md

> **GDI adapter (`port/compat/gdi`)**: gdi32 on `port/native/text`.
> `bifrost-compat-gdi-test` (headless, in `fastcheck.sh`) runs the client's
> DrawDC, CDCBitmap and DrawTriangleDC sequences through the GDI names,
> checks the pixels and extents, and writes `gdi-sample.png` to its build
> directory.
