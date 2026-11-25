# text: CoreText drawing into bitmaps, without smoothing

Replaces the client's GDI text: fonts by GDI face name, height and
character set; drawing bytes in a Windows code page (949, 1252, any other
the system knows) or UTF-16 into a 32-bit bitmap the caller owns; extents,
advances and `TEXTMETRIC` values; code-page conversion; the polygon fill of
`DrawTriangleDC`; canvases, which stand for the device context the client
drew on (a colour, the rectangle drawn so far); which GDI faces Windows
lists under a character set. The contract is `include/bf_text.h`. What the
client asked of GDI is in [GDI-USAGE.md](GDI-USAGE.md); since M11.8 the
client calls this module directly (`src/3dlib/DrawUtil.cpp`, `DC.cpp`,
`src/window/UISys.cpp`), and `port/compat/gdi`'s README lists what moved
where.

## Build and test

```
cmake -S port/native/text -B build/native/text -G Ninja
cmake --build build/native/text
build/native/text/bifrost-text-test
```

The test needs no game data. It checks code-page conversion (CP949 with
the UHC extension, every pair round-tripped; CP1252 including the bytes
Windows maps to C1 controls; UTF-8), `CharNextExA`/`CharPrevExA`
equivalents, and for every face and height the client uses: metrics
(height = ascent + descent, leading = height - em), extent = sum of
advances = prefix pen positions, drawn pixels are only background or text
colour and lie inside the extent box, every ASCII glyph and the Hangul
sample leave ink at 11–15 px; top-down and bottom-up bitmaps, UTF-16 equals
CP949, opaque background, clipping, underline, italic, font fallback for
characters a face lacks, polygon fill; canvases (text exactly its colour
with alpha 0, the drawn rectangle is the text's cell and clipped, pieces
drawn at the first's extent equal the whole, no pixels draws nothing, the
triangle's box) and the face listing per character set. It writes
`build/native/text/text-sample-sheet.png` and compares it with
`test/reference/text-sample-sheet.png` (a difference is a note, not a
failure, because CoreText's rasteriser may change with macOS).

## How it works

- **No antialiasing.** Each glyph is rasterised once per font by
  CoreGraphics with antialiasing, font smoothing and subpixel positioning
  off, read as on/off, and cached; strings are drawn by copying the cached
  masks at integer pen positions. A pixel is either the text colour or
  untouched (or the background colour in opaque mode).
- **Widths.** Advances are integers: the font's `hdmx` widths for the em
  size where the font has them (the hinted widths GDI uses), else the
  rounded advance. No kerning, so the extent of a prefix is the caret
  position.
- **Heights.** `height < 0` is the em in pixels; `height > 0` is the cell
  (ascent + descent): the em is the largest whose `VDMX` cell fits, as
  Windows does, else `round(h × unitsPerEm / (winAscent + winDescent))` of
  the Windows original. Arial at 15 gives em 12, cell 12 + 3, as on
  Windows. Glyph rows outside the cell are not drawn.
- **Bold.** Faces whose Windows original has no bold (Gulim, Dotum, MS Sans
  Serif) are emboldened like GDI: the glyph doubled one pixel to the right,
  every advance + 1. Others use their bold font.
- **Fallback.** Characters the font lacks come from AppleGothic for Korean
  in a Western face (GDI's font linking to Gulim), else from CoreText's
  cascade list; characters no font has are drawn as the missing-glyph box
  and counted (`text.missing_glyph`). Control characters draw nothing.

## Face mapping

| GDI face (aliases) | macOS font | Basis |
|---|---|---|
| Gulim 굴림, GulimChe; Dotum 돋움, DotumChe | AppleGothic (then Apple SD Gothic Neo) | AppleGothic's Hangul is one em wide like Gulim's, its Latin is proportional, and it has `hdmx` widths. Apple SD Gothic Neo's Hangul is 0.865 em, so it is only the fallback; the module forces Hangul and other characters from U+2000 to one em in these faces either way |
| Batang 바탕, Gungsuh 궁서 (+Che) | AppleMyungjo | serif Korean; not used by the client |
| Arial | ArialMT, Arial-BoldMT, -ItalicMT, -BoldItalicMT | the same font |
| Tahoma | Tahoma, Tahoma-Bold | the same font |
| MS Sans Serif, Microsoft Sans Serif | MicrosoftSansSerif | the TrueType twin of the bitmap face |
| Times New Roman, Verdana, Courier New | the same fonts | |
| MS Gothic ＭＳ ゴシック | HiraginoSans-W3 / W6 | |
| SimSun 宋体, MingLiU 細明體 | STHeiti SC / TC Light | Songti's thin strokes break up at 12 px without hinting; Heiti stays legible |
| unknown | by charset: Hangul → Gulim, Shift-JIS → MS Gothic, GB2312 → SimSun, Big5 → MingLiU, else Arial | Rix\*.eot faces land here |

Gulim, Dotum, Batang, MS Gothic, SimSun and MingLiU share the vertical
metrics recorded for their Windows originals (256 units per em, ascent 220, descent
36: the cell equals the em; from the fonts' published metrics, not measured here). Their substitutes are taller than that grid:
at Gulim 12 (ascent 10, descent 2) AppleGothic's `g j p q y @` lose their
lowest row and accented capitals the top of the accent. Hangul fits.

## Untested and open

- Widths against real GDI: Arial, Tahoma, Verdana, Times New Roman and MS
  Sans Serif use the Windows fonts' own `hdmx` and `VDMX`, so their widths
  and cell metrics should equal GDI's; this has not been compared with a
  Windows machine. Gulim's Latin widths are AppleGothic's, which are likely
  wider than Gulim's; the font test screen (M9.4) measures this against
  reference screenshots.
- CoreText does not run TrueType hinting, so glyph shapes are the
  unhinted outlines sampled at pixel centres: legible from 10 px up for
  the sans faces, but not the hinted shapes GDI drew. Spacing can look
  uneven where the hinted width (`hdmx`) differs from the outline.
- Vertical text (`@` faces) is drawn horizontally; the client does not use
  it.
- The render-target path of the client (`CDCSurface::GetDC`) draws on a
  canvas over a staging bitmap that `CSurface` keeps (`src/3dlib/Texture.cpp`,
  M11.9).

## For port/README.md

> **Text (`port/native/text`)**: `bifrost-text-test` checks the CoreText text
> module (metrics, unblended 1-bit drawing, CP949/CP1252/UTF-16, bitmap
> orientations) and writes a sample sheet of every mapped face to
> `build/native/text/text-sample-sheet.png`. Build and run:
> `cmake -S port/native/text -B build/native/text -G Ninja && cmake --build build/native/text && build/native/text/bifrost-text-test`.
