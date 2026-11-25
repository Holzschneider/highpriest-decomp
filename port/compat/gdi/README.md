# compat/gdi: what is left after M11.8

M11.8 moved the client's text, its window bitmaps (DIB sections) and the
triangle fill from gdi32 onto `port/native/text` (`bf_text.h`): `DrawDC`,
`FontInfo` and `FontMgr` (`src/3dlib/DrawUtil.cpp`), `CDCBitmap` and
`CDCSurface` (`src/3dlib/DC.cpp`), `UIWindow::DrawTriangleDC`
(`src/window/UISys.cpp`). What the adapter decided moved with them:

| Was here | Now |
|---|---|
| a memory DC's text colour, kept between uses; a surface DC fresh each time | `bf_text_canvas.color` (a `CDCBitmap` keeps its canvas; the d3d adapter's `GetDC` zeroes its own) |
| `SetTextColor`'s COLORREF as the pixel `0x00RRGGBB` (alpha 0) | `bf_text_color_from_colorref` |
| `TextOutW` (transparent, `TA_LEFT|TA_TOP`) and the dirty rectangle of a surface DC | `bf_text_canvas_draw`, `bf_text_canvas_mark` |
| `CreatePolygonRgn` + `FillRgn` | `bf_text_canvas_fill_polygon` |
| `CreateFont`: the face name from the character set's code page to UTF-8, unsmoothed | `FontInfo::Create` |
| `EnumFontFamiliesEx`: the faces an English XP lists under each character set | `bf_text_face_listed`, called by `FontInfo::GetFontFace` |
| `CreateDIBSection`: 32 bpp top-down, zeroed, none for 0 or over 1 GB | `CDCBitmap::CreateDCSurface` (`calloc`) |
| the d3d adapter's surface DC (`bf_compat_gdi_dc_for_bitmap`, `_take_dirty`) | a `bf_text_canvas` handed out as the `HDC` (port/compat/d3d) |
| the coverage hook in `TextOut` (M9.5) | `DrawDC::_textout` |

## What remains, and why

Three names whose callers are not M11.8's files:

- `GetStockObject` (brushes only): `src/WinMain.cpp` registers its window
  class with `BLACK_BRUSH`.
- `bf_compat_gdi_brush_color`: `port/compat/input`'s `RegisterClassA` reads
  that brush's colour.
- `bf_compat_gdi_screen_dc`: `port/compat/input`'s `GetDC` returns it (no
  caller in the client since `FontInfo::GetFontFace` stopped enumerating).

They go with M11.7 (the window class and the input adapter); then this
directory and its entry in `BF_COMPAT_AREAS` (CMakeLists.txt) are deleted.

## Build and test

```
cmake --build --preset debug --target bifrost-compat-gdi-test
build/cmake/debug/compat/gdi/bifrost-compat-gdi-test
```

Headless; checks the stock brushes' colours and the screen DC. The text
checks of the former test (exact text colour, cells, extents adding up,
pieces equal to the whole, the triangle) are in `bifrost-text-test`
(`port/native/text`, "canvas").
