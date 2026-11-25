# What the client asks of GDI for text

The specification of the GDI adapter (M4.5a), read from `src/` at the
start of the port. The adapter put these calls on top of `bf_text.h`; since
M11.8 the call sites call `bf_text.h` themselves (port/compat/gdi/README.md
lists what moved where). Kept as the record of what the 2008 client did.

## Where GDI is called

All drawn text goes through one class, `DrawDC` in `src/3dlib/DrawUtil.cpp`.
Windows reach it through the helpers of `UIWindow` in `src/window/UISys.cpp`
(`TextOutA`, `TextOutWithRightAlign`, `...Shadow`, `...Outline`,
`...Decoration`, `...Vertical`, `...TwoColors`, `GetTextWidth`,
`GetTextHeight`, `PolishText`); the renderer reaches it through
`CRenderer::TextOutScreen` and `GetTextSize`. No other file calls a GDI text
function.

| Call | Where | Arguments the client passes |
|---|---|---|
| `CreateFont` | `FontInfo::Create` | `(height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, 0, 0, 0, charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, face)`. Height is positive: a **cell height**. Never italic, underlined or struck out |
| `CreateFontIndirect` | `FontInfo::CreateEotFont` | Rix\*.eot faces, height -13. No caller: dead code |
| `EnumFontFamiliesEx` | `FontInfo::GetFontFace` | On `GetDC(NULL)`, to pick between the Windows 9x name (`fontFace9x`, CP949/CP932/CP936/CP950 bytes) and the NT name (`fontFaceNT`) of the same face. The adapter can answer "the NT name exists" |
| `GetDC(NULL)` | same | only for the enumeration above |
| `GetCurrentObject(hdc, OBJ_FONT)`, `SelectObject` | `DrawDC` | save, select, restore the font |
| `SetBkMode(hdc, TRANSPARENT)` | `DrawDC::DrawDC` | the only mode used; no `SetBkColor`, no `OPAQUE` |
| `SetTextColor(hdc, COLORREF)` | `DrawDC::SetTextColor` | `0x00BBGGRR` |
| `TextOutW(hdc, x, y, wide, n)` | `DrawDC::_textout` | after `MultiByteToWideChar(GetCodePageFromCharset(charset), 0, ...)`; default alignment `TA_LEFT|TA_TOP` (no `SetTextAlign` anywhere) |
| `GetTextExtentPoint32W` | `DrawDC::_gettextsize` | same conversion first |
| `CreateCompatibleDC(NULL)`, `CreateDIBSection`, `SelectObject`, `DeleteObject`, `DeleteDC` | `CDCBitmap` (`src/3dlib/DC.cpp`) | 32 bpp, `BI_RGB`, **top-down** (`biHeight = -h`), one per UI window, resized when a window grows |
| `IDirectDrawSurface7::GetDC/ReleaseDC` | `CDCSurface` (`DC.cpp`) | text straight onto the render target: `TextOutScreen` (FPS counter, a mode message in `Mode.cpp`) and `GetTextSize` |
| `CreatePolygonRgn(pts, 3, WINDING)`, `CreateSolidBrush`, `FillRgn`, `DeleteObject` | `UIWindow::DrawTriangleDC` | a filled triangle in a window bitmap |

Not used anywhere: `GetTextMetrics`, `GetCharWidth*`, `GetCharABCWidths`,
`ExtTextOut`, `DrawText`, `SetTextAlign`, `SetBkColor`, `GetGlyphOutline`.

Code-page functions next to the text code, which `bf_text` also provides:

| Call | Where | bf_text |
|---|---|---|
| `MultiByteToWideChar` | `DrawDC` | `bf_text_to_utf16` |
| `WideCharToMultiByte` | `framework/Language.cpp` (IME), `NPGameLib.cpp` | `bf_text_from_utf16` |
| `CharNextExA(g_codePage, p, 0)` | `Insult.cpp`, `Session.cpp`, `UIControl.cpp`, `UIFrameWnd2.cpp`, `UISys.cpp`, `Language.cpp` | `p + bf_text_char_length(cp, p, -1)` |
| `CharPrevExA(g_codePage, start, p, 0)` | `UIControl.cpp`, `Language.cpp` | `start + bf_text_char_prev(cp, start, p - start)` |
| `IsDBCSLeadByte` | `GameModePacket.cpp` | `bf_text_is_lead_byte(cp, b)` |
| `GetCodePageFromCharset` (the client's own) | `framework/Language.cpp` | `bf_text_codepage_for_charset` |

## Faces, character sets and heights

`DrawDC::SetFont(fontType, fontHeight, bold)` turns the client's font type
into a face. Type 0 means "the language's default" and is rewritten:

| Language / service | Type | Face (NT name) | Charset | Height |
|---|---|---|---|---|
| Korean (kRO) | 0 | Gulim (9x name 굴림 in CP949) | HANGUL 129 | as passed |
| English (`america`, our default) | 1 | Arial | ANSI 0 | **+3** |
| English, Indonesia | 8 | Verdana | ANSI | +2 |
| English, Russia | 9 | Arial | RUSSIAN 204 | +3 |
| English, Turkish script | 6 | Arial | TURKISH 162 | +3 |
| Japanese | 2 | MS Gothic | SHIFTJIS 128 | as passed |
| Simplified / Traditional Chinese | 3 / 4 | SimSun / MingLiU | GB2312 134 / BIG5 136 | as passed |
| Thai | 5 | Tahoma | THAI 222 | +3 |
| Portuguese | 7 | Verdana | ANSI | +3 (+1 for Brazil, France) |
| Vietnamese | 10 | Arial | VIETNAMESE 163 | +3 |

Types 20–28 (`FT_SQUIRREL` ... `FT_JJANGU`, the Rix\*.eot faces) keep the
Korean charset; `GetFontFace` then indexes past the end of `fontFace9x`, so
the adapter must treat any type outside 0..10 as an unknown face (bf_text
falls back to Gulim for the Hangul charset). An explicit type 1 is Arial in
every service, without the +3.

Heights at the call sites (literal arguments, about 600 calls):

| Height | Calls | Use |
|---|---|---|
| 12 | ~430 | nearly everything: window titles, lists, chat, edit fields, item names |
| 14 | 41 | **explicit Arial** (type 1): numbers in the status, basic-info, homunculus, mercenary and exchange windows |
| 11 | 13 | small labels |
| 13 bold / 12 | 12 | name balloons: `NameBalloonfontSize` 13 and bold for Korea and Indonesia, else 12 regular |
| 16, 18 | 7 | Arial 16 and 18 in a few windows ("Not Available") |
| 10 | 3 | messenger, small notes |
| 20 | 32 | `TextOutScreen` in the single-player files only (they leave with M1) |
| variables | ~75 | pass-throughs of the above (`fontHeight`, `m_fontSize` 12 by default) |

Bold is requested by name balloons (above), by `TextOutWithOutline` /
`...DecoOutline` callers (9 literal `true`), and by a few controls with
`m_isFontBold`.

So the fonts the adapter will be asked for are: Gulim (charset 129) at
cell heights 10, 11, 12, 16 regular and 12, 13 bold for the Korean service;
Arial (charset 0) at 13, 14, 15, 19 regular and 15 bold for the English one
(the default type plus 3); and Arial at 14, 16, 18 explicitly in both.

## How extents are used

- **Caret and selection** (`UIEditCtrl`, `UIControl.cpp` around 1150, 1245,
  1699, 1769): `GetTextWidth(text, n)` of a prefix of n bytes is the caret's
  x. Prefix widths must add up: the width of a prefix is the pen position
  after it. No kerning.
- **Horizontal scrolling of edit fields**: steps through characters with
  `CharNextExA` until the remaining width fits `m_w - 12`.
- **Word wrap** (`PolishText`, `PolishText3`, `UIControl.cpp` 1485, 1531):
  sums widths of words/characters and breaks when the sum exceeds the
  box; widths of pieces must equal the width of the whole.
- **Name and chat balloons** (`UIControl.cpp` 837–971): bitmap width =
  `GetTextWidth + 8`, at least 32; vertical balloons sum per-character
  `size.cy`.
- **Right alignment** (`TextOutWithRightAlign`): `x - cx`.
- **Colour codes** (`TextOutWithDecoration`, `^RRGGBB`): draws segments,
  advancing x by the extent of the bytes before each segment.
- **Vertical text** (`TextOutVertical`): one character per row, centred by
  its own cx, rows advanced by cy.
- **Line spacing**: `GetTextHeight` (= `size.cy`, the cell height) + 4.

`size.cy` is always the font's cell height (`tmHeight`), whatever the
string.

## Composite styles

Shadow, outline and two-colour text are several `TextOutW` calls at offsets:
shadow `(x+1, y+1)` then `(x, y)`; outline the four neighbours then the
centre; two colours `(x, y)` then `(x-1, y)`. All rely on transparent
background and on identical glyphs at every position.

## What the bitmap expects

The window bitmap is cleared to the colour key `0xFFFF00FF` (magenta) where
it should be transparent. `CDCBitmap::UpdateSurface` sets every alpha byte
to `0xFF` and uploads 256×256 tiles; the upload turns magenta into
transparent pixels. Text pixels therefore must be exactly the text colour:
an antialiased edge would be a mix with magenta and show as a pink fringe.
GDI writes text pixels as `0x00RRGGBB` (alpha 0), which the upload ignores.

## Mapping onto bf_text

- `CreateFont` → `bf_text_font_create` with `{face, height, weight, 0, 0, 0,
  charset}`; convert a CP949 face name to UTF-8 first (`bf_text_to_utf8`).
  The client's height is positive, so it is a cell height.
- `TextOutW` / `GetTextExtentPoint32W` → `bf_text_draw` /
  `bf_text_extent` with `BF_TEXT_CP_UTF16`, or skip the client's conversion
  and pass its bytes with the code page.
- `SetTextColor(COLORREF c)` → colour `((c & 0xFF) << 16) | (c & 0xFF00) |
  ((c >> 16) & 0xFF)` (alpha 0, as GDI writes).
- The DIB section → `bf_text_bitmap{bits, w, h, w * 4, 0}` (top-down).
- `DrawTriangleDC` → `bf_text_fill_polygon(..., winding = 1)` with the brush
  colour converted the same way.
- `CDCSurface::GetDC` on the render target has no bitmap to give; the
  adapter needs a 32-bit staging bitmap for it (only the FPS counter and one
  message use it outside the single-player files).
