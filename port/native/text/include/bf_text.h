/*
 * bf_text: text drawing on CoreText, without antialiasing, into 32-bit
 * bitmaps the caller owns. It replaces the GDI text calls of the client
 * (CreateFont, TextOutW, GetTextExtentPoint32W, GetTextMetrics) and the
 * code-page functions around them (MultiByteToWideChar, CharNextExA).
 * The GDI adapter (port/compat) puts the Windows-shaped calls on top.
 *
 * Text crosses as bytes in a named Windows code page (949 Korean, 1252
 * Western, any other the system knows), or as UTF-16 with the code page
 * BF_TEXT_CP_UTF16, or UTF-8 with BF_TEXT_CP_UTF8. A length < 0 means the
 * text ends at its first NUL (byte or UTF-16 unit).
 *
 * Every pixel the drawing functions write is either the full text colour
 * or the full background colour (opaque mode); no pixel is blended.
 *
 * Threads: the conversion functions (bf_text_to_*, bf_text_from_*,
 * bf_text_char_*, bf_text_is_lead_byte, bf_text_codepage_for_charset) may
 * be called from any thread. A font may be used by one thread at a time;
 * the client uses them on its main thread only.
 *
 * Failure: functions return -1 (or NULL) and write a trace line under
 * BFT_ERR. Characters no installed font has are drawn as the font's
 * missing-glyph box, counted as "text.missing_glyph" and traced once.
 */
#ifndef BF_TEXT_H
#define BF_TEXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Code pages ------------------------------------------------------- */

enum {
	BF_TEXT_CP_KOREAN  = 949,    /* Unified Hangul Code, the kRO client's */
	BF_TEXT_CP_WESTERN = 1252,   /* Windows Latin 1, the English client's */
	BF_TEXT_CP_UTF16   = 1200,   /* text is const uint16_t *, length in units */
	BF_TEXT_CP_UTF8    = 65001
};

/* GDI character-set numbers, as LOGFONT.lfCharSet holds them. */
enum {
	BF_TEXT_CHARSET_ANSI        = 0,
	BF_TEXT_CHARSET_DEFAULT     = 1,
	BF_TEXT_CHARSET_SYMBOL      = 2,
	BF_TEXT_CHARSET_SHIFTJIS    = 128,
	BF_TEXT_CHARSET_HANGUL      = 129,
	BF_TEXT_CHARSET_GB2312      = 134,
	BF_TEXT_CHARSET_CHINESEBIG5 = 136,
	BF_TEXT_CHARSET_GREEK       = 161,
	BF_TEXT_CHARSET_TURKISH     = 162,
	BF_TEXT_CHARSET_VIETNAMESE  = 163,
	BF_TEXT_CHARSET_HEBREW      = 177,
	BF_TEXT_CHARSET_ARABIC      = 178,
	BF_TEXT_CHARSET_BALTIC      = 186,
	BF_TEXT_CHARSET_RUSSIAN     = 204,
	BF_TEXT_CHARSET_THAI        = 222,
	BF_TEXT_CHARSET_EASTEUROPE  = 238
};

/* The code page GDI uses for a character set (the client's
   GetCodePageFromCharset): HANGUL 949, SHIFTJIS 932, ... others 1252. */
int bf_text_codepage_for_charset(int charset);

/* 1 if byte starts a two-byte character in a double-byte code page
   (932, 936, 949, 950), else 0. IsDBCSLeadByteEx. */
int bf_text_is_lead_byte(int codepage, unsigned char byte);

/* Bytes in the character starting at text[0]: 0 at the end (len == 0 or a
   NUL), 1, or 2 for a lead byte followed by a non-NUL byte. CharNextExA is
   text + bf_text_char_length(...). len < 0: NUL-terminated. */
int bf_text_char_length(int codepage, const char *text, int len);

/* Offset of the character before offset `at` in text (CharPrevExA): 0 when
   at <= 0. Scans from the start, as Windows does for DBCS code pages. */
int bf_text_char_prev(int codepage, const char *text, int at);

/*
 * Conversions (MultiByteToWideChar / WideCharToMultiByte). Each returns
 * the number of units (UTF-16 units or bytes) the result has, writes at
 * most out_max of them to out (no NUL is added), and with out == NULL only
 * counts. Bytes a code page does not define become U+FFFD, except in 1252
 * where 0x81 0x8D 0x8F 0x90 0x9D become U+0081... as on Windows.
 * Characters a code page cannot hold become '?'. Returns -1 for a code
 * page the system does not know.
 */
int bf_text_to_utf16(int codepage, const char *text, int len, uint16_t *out, int out_max);
int bf_text_from_utf16(int codepage, const uint16_t *text, int len, char *out, int out_max);
int bf_text_to_utf8(int codepage, const char *text, int len, char *out, int out_max);
int bf_text_from_utf8(int codepage, const char *utf8, int len, char *out, int out_max);

/* ---- Fonts ------------------------------------------------------------ */

typedef struct bf_text_font bf_text_font;

typedef struct bf_text_font_desc {
	/* The GDI face name in UTF-8 ("Gulim", "굴림", "Arial", ...), matched
	   without regard to case. NULL or unknown: chosen by charset (see
	   bf_text_face_mapping). The adapter converts CP949 face names first. */
	const char *face;
	/* LOGFONT.lfHeight. < 0: character height, |height| pixels per em.
	   > 0: cell height (ascent + descent) in pixels; the em is the largest
	   whose cell fits, from the font's VDMX table where it has one, else
	   round(height * unitsPerEm / (winAscent + winDescent)) of the Windows
	   original (the face mapping records the originals' metrics). 0: -16. */
	int height;
	int weight;      /* 0..1000, 0 = 400; >= 600 is bold */
	int italic;      /* non-zero: italic face, or a 0.2 shear without one */
	int underline;
	int strikeout;
	int charset;     /* BF_TEXT_CHARSET_*; picks the fallback face */
} bf_text_font_desc;

/* Returns NULL (and traces) only when no font at all can be made. */
bf_text_font *bf_text_font_create(const bf_text_font_desc *desc);
void bf_text_font_destroy(bf_text_font *font);

/* What TEXTMETRIC needs, in pixels. height = ascent + descent; the cell
   of a line of text is height rows from the y it is drawn at. */
typedef struct bf_text_metrics {
	int height;
	int ascent;
	int descent;
	int internal_leading;   /* height - em */
	int external_leading;
	int ave_char_width;
	int max_char_width;
	int weight;             /* 400 or 700 */
	int overhang;           /* always 0, as GDI reports for TrueType */
	int italic, underline, strikeout;
	int charset;
	int pitch_and_family;   /* GDI TMPF_* bits: TMPF_VECTOR|TMPF_TRUETYPE, plus
	                           TMPF_FIXED_PITCH (meaning variable pitch) */
	int em;                 /* pixels per em the glyphs are rasterised at */
	int synthetic_bold;     /* bold drawn by doubling, advances + 1 */
	char face[64];          /* the GDI face name it stands for (UTF-8) */
	char mapped[64];        /* the macOS font it draws with (PostScript name) */
} bf_text_metrics;

int bf_text_font_metrics(const bf_text_font *font, bf_text_metrics *out);

/* ---- Measuring -------------------------------------------------------- */

/*
 * GetTextExtentPoint32: cx = sum of the characters' advances (integers, no
 * kerning, so the extent of a prefix is the pen position after it), cy =
 * metrics.height. Returns 0, or -1 for a bad code page.
 */
int bf_text_extent(bf_text_font *font, int codepage, const void *text, int len, int *cx, int *cy);

/*
 * Per-character advances. For each character of text (a DBCS pair or a
 * surrogate pair counts as one), writes its advance to advances[i] and,
 * if offsets is not NULL, the offset of its first unit in text to
 * offsets[i]. Returns the number of characters, writing at most max.
 */
int bf_text_advances(bf_text_font *font, int codepage, const void *text, int len,
                     int *advances, int *offsets, int max);

/* Advance of one Unicode code point (GetCharWidth32W). */
int bf_text_char_advance(bf_text_font *font, uint32_t codepoint);

/* ---- Drawing ---------------------------------------------------------- */

/*
 * A 32-bit bitmap the caller owns, such as a DIB section. Pixels are
 * uint32_t 0xAARRGGBB in native (little-endian) order, i.e. B,G,R,A bytes.
 * stride is bytes per row (>= width * 4). bottom_up != 0: the first row in
 * memory is the bottom row of the picture (a DIB with positive biHeight);
 * coordinates are always top-down.
 */
typedef struct bf_text_bitmap {
	void *pixels;
	int width;
	int height;
	int stride;
	int bottom_up;
} bf_text_bitmap;

enum { BF_TEXT_TRANSPARENT = 1, BF_TEXT_OPAQUE = 2 };

/*
 * TextOut with TA_LEFT|TA_TOP: (x, y) is the top-left corner of the text's
 * cell; the baseline is at row y + ascent. Text pixels are set to `color`
 * exactly as given (GDI writes alpha 0, so the adapter passes 0x00RRGGBB);
 * with BF_TEXT_OPAQUE the cell rectangle (x, y, extent cx, height) is first
 * filled with bk_color. Glyph pixels outside the cell rows are dropped;
 * everything is clipped to the bitmap. Returns the extent cx, or -1.
 */
int bf_text_draw(bf_text_font *font, const bf_text_bitmap *bitmap, int x, int y,
                 int codepage, const void *text, int len,
                 uint32_t color, int bk_mode, uint32_t bk_color);

/*
 * CreatePolygonRgn + FillRgn: fills the polygon given as count (x, y)
 * pairs in xy. A pixel is filled when its centre is inside; winding != 0
 * uses the non-zero rule (WINDING), 0 the even-odd rule (ALTERNATE).
 * Returns the number of pixels set, or -1.
 */
int bf_text_fill_polygon(const bf_text_bitmap *bitmap, const int *xy, int count,
                         uint32_t color, int winding);

/* ---- The face table --------------------------------------------------- */

/*
 * Entry `index` of the face mapping: the GDI face name, and the macOS font
 * (PostScript name) it resolves to on this machine, or NULL when none of
 * its candidates is installed. Returns 0, or -1 past the end.
 */
int bf_text_face_mapping(int index, const char **gdi_face, const char **mapped);

#ifdef __cplusplus
}
#endif

#endif
