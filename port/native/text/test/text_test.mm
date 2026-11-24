// bifrost-text-test: checks the text module without game data and writes a
// sample sheet of every mapped face at the client's sizes.
//
//   bifrost-text-test [sheet.png]
//
// Prints one line per check; exits non-zero when one fails.

#include "bf_text.h"
#include "bf_text_internal.h"
#include "bf_trace.h"

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

#include <CommonCrypto/CommonDigest.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

static int g_failures = 0;

static void Check(bool ok, const char *name, const char *detail = "")
{
	printf("%s  %s%s%s\n", ok ? "ok  " : "FAIL", name, *detail ? ": " : "", detail);
	if (!ok) ++g_failures;
}

static std::string Fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
static std::string Fmt(const char *f, ...)
{
	char buf[1024];
	va_list a;
	va_start(a, f);
	vsnprintf(buf, sizeof(buf), f, a);
	va_end(a);
	return buf;
}

// ---- Strings ---------------------------------------------------------------

// "한국어 라그나로크 상점" and "가나다" in CP949.
static const char kKorean949[] = "\xC7\xD1\xB1\xB9\xBE\xEE \xB6\xF3\xB1\xD7\xB3\xAA\xB7\xCE\xC5\xA9 \xBB\xF3\xC1\xA1";
static const char kGanada949[] = "\xB0\xA1\xB3\xAA\xB4\xD9";
// "Café Müller Straße ÉÜ" in CP1252.
static const char kWestern1252[] = "Caf\xE9 M\xFCller Stra\xDF" "e \xC9\xDC";
static const char kReference[] = "The quick brown fox 0123456789";

// ---- Bitmaps ---------------------------------------------------------------

struct Canvas {
	int w, h;
	bool bottomUp;
	std::vector<uint32_t> px;
	Canvas(int w_, int h_, uint32_t fill, bool bu = false) : w(w_), h(h_), bottomUp(bu), px((size_t)w_ * h_, fill) {}
	bf_text_bitmap bitmap() { return bf_text_bitmap{ px.data(), w, h, w * 4, bottomUp ? 1 : 0 }; }
	// Pixel at picture coordinates (top-down).
	uint32_t &at(int x, int y) { return px[(size_t)(bottomUp ? h - 1 - y : y) * w + x]; }
};

static bf_text_font *Font(const char *face, int height, bool bold = false, int charset = 0, bool italic = false, bool underline = false)
{
	bf_text_font_desc d = {};
	d.face = face;
	d.height = height;
	d.weight = bold ? 700 : 400;
	d.charset = charset;
	d.italic = italic;
	d.underline = underline;
	return bf_text_font_create(&d);
}

// ---- Checks ----------------------------------------------------------------

static void CheckConversions()
{
	uint16_t u[64];
	int n = bf_text_to_utf16(949, kKorean949, -1, u, 64);
	Check(n == 12 && u[0] == 0xD55C && u[1] == 0xAD6D && u[2] == 0xC5B4 && u[3] == ' ' && u[4] == 0xB77C,
	      "cp949 to UTF-16", Fmt("%d units, U+%04X U+%04X U+%04X", n, u[0], u[1], u[2]).c_str());

	const char uhc[] = "\x81\x41\xC6\x52";  // UHC-only syllables (outside EUC-KR): U+AC02, U+D7A3
	n = bf_text_to_utf16(949, uhc, 4, u, 64);
	Check(n == 2 && u[0] == 0xAC02 && u[1] == 0xD7A3, "cp949 extension (UHC) pairs",
	      Fmt("%d units, U+%04X U+%04X", n, u[0], u[1]).c_str());

	char back[64];
	uint16_t hk[] = { 0xD55C, 0xAD6D, 0xC5B4, 'A' };
	n = bf_text_from_utf16(949, hk, 4, back, 64);
	Check(n == 7 && memcmp(back, "\xC7\xD1\xB1\xB9\xBE\xEE" "A", 7) == 0, "UTF-16 to cp949");

	n = bf_text_to_utf16(1252, kWestern1252, -1, u, 64);
	Check(n == 21 && u[3] == 0xE9 && u[6] == 0xFC && u[16] == 0xDF && u[19] == 0xC9, "cp1252 to UTF-16 (e-acute, u-umlaut, sharp s)");
	const char odd[] = "\x80\x81\x9F";
	n = bf_text_to_utf16(1252, odd, 3, u, 64);
	Check(n == 3 && u[0] == 0x20AC && u[1] == 0x0081 && u[2] == 0x0178, "cp1252 euro sign, undefined 0x81, Y-diaeresis",
	      Fmt("U+%04X U+%04X U+%04X", u[0], u[1], u[2]).c_str());
	uint16_t euro[] = { 0x20AC, 0xE9, 0xD55C };
	n = bf_text_from_utf16(1252, euro, 3, back, 64);
	Check(n == 3 && (uint8_t)back[0] == 0x80 && (uint8_t)back[1] == 0xE9 && back[2] == '?', "UTF-16 to cp1252, unmappable to '?'");

	char utf8[64];
	n = bf_text_to_utf8(949, kGanada949, -1, utf8, 63);
	utf8[n > 0 ? n : 0] = 0;
	Check(strcmp(utf8, "\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4") == 0, "cp949 to UTF-8");
	n = bf_text_from_utf8(949, "\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4", -1, back, 64);
	Check(n == 6 && memcmp(back, kGanada949, 6) == 0, "UTF-8 to cp949");

	const char mixed[] = "A\xB0\xA1" "B";
	Check(bf_text_char_length(949, mixed, -1) == 1 && bf_text_char_length(949, mixed + 1, -1) == 2 &&
	      bf_text_char_length(949, mixed + 4, -1) == 0, "CharNextExA lengths (cp949)");
	Check(bf_text_char_prev(949, mixed, 3) == 1 && bf_text_char_prev(949, mixed, 4) == 3 && bf_text_char_prev(949, mixed, 1) == 0,
	      "CharPrevExA offsets (cp949)");
	Check(bf_text_char_length(1252, "\xB0\xA1", -1) == 1, "cp1252 has no lead bytes");
	Check(bf_text_codepage_for_charset(BF_TEXT_CHARSET_HANGUL) == 949 && bf_text_codepage_for_charset(BF_TEXT_CHARSET_ANSI) == 1252,
	      "charset to code page");
	Check(bf_text_to_utf16(12345, "x", 1, u, 4) == -1, "unknown code page fails");

	// Every defined CP949 pair survives the round trip.
	int pairs = 0, bad = 0;
	for (int l = 0x81; l <= 0xFE; ++l)
		for (int t = 0x41; t <= 0xFE; ++t) {
			char b[2] = { (char)l, (char)t };
			uint16_t w;
			if (bf_text_to_utf16(949, b, 2, &w, 1) != 1 || w == 0xFFFD) continue;
			++pairs;
			char r[4];
			if (bf_text_from_utf16(949, &w, 1, r, 4) != 2 || r[0] != b[0] || r[1] != b[1]) ++bad;
		}
	Check(pairs > 17000 && bad == 0, "cp949 round trip of every pair", Fmt("%d pairs, %d differ", pairs, bad).c_str());
}

struct FaceCase { const char *face; int height; bool bold; int charset; };

// The faces and heights the client asks for (port/native/text/GDI-USAGE.md):
// Korean service: Gulim at 10..14, 16, 20, bold at 12 and 13 (name
// balloons); English service: Arial at the same + 3.
static const FaceCase kCases[] = {
	{ "Gulim", 10, false, 129 }, { "Gulim", 11, false, 129 }, { "Gulim", 12, false, 129 }, { "Gulim", 13, false, 129 },
	{ "Gulim", 14, false, 129 }, { "Gulim", 16, false, 129 }, { "Gulim", 20, false, 129 },
	{ "Gulim", 12, true, 129 }, { "Gulim", 13, true, 129 },
	{ "\xEA\xB5\xB4\xEB\xA6\xBC", 12, false, 129 },
	{ "Dotum", 12, false, 129 }, { "Batang", 12, false, 129 },
	{ "Arial", 13, false, 0 }, { "Arial", 14, false, 0 }, { "Arial", 15, false, 0 }, { "Arial", 16, false, 0 },
	{ "Arial", 17, false, 0 }, { "Arial", 19, false, 0 }, { "Arial", 23, false, 0 },
	{ "Arial", 15, true, 0 }, { "Arial", 16, true, 0 },
	{ "Tahoma", 12, false, 222 }, { "Tahoma", 15, false, 222 }, { "Tahoma", 15, true, 222 },
	{ "Verdana", 15, false, 0 }, { "MS Sans Serif", 12, false, 0 }, { "MS Sans Serif", 15, false, 0 },
	{ "Times New Roman", 15, false, 0 }, { "Courier New", 15, false, 0 },
	{ "MS Gothic", 12, false, 128 }, { "SimSun", 12, false, 134 }, { "MingLiU", 12, false, 136 },
	{ "RixSquirrel_10.eot", 12, false, 129 }, { "NoSuchFace", 15, false, 0 },
};

static std::string Label(const FaceCase &c)
{
	return Fmt("%s %d%s", strcmp(c.face, "\xEA\xB5\xB4\xEB\xA6\xBC") == 0 ? "(Korean name) Gulim" : c.face, c.height, c.bold ? " bold" : "");
}

// Draws text and checks: only background or text colour; nothing outside
// the extent box; returns the number of text pixels.
static int DrawAndInspect(bf_text_font *f, int cp, const void *text, int len, const char *what, bool checkBox = true)
{
	const uint32_t bg = 0xFFFF00FF, fg = 0x00102030;
	Canvas cv(1200, 64, bg);
	bf_text_bitmap b = cv.bitmap();
	int cx = 0, cy = 0;
	bf_text_extent(f, cp, text, len, &cx, &cy);
	int x = 20, y = 16;
	int ret = bf_text_draw(f, &b, x, y, cp, text, len, fg, BF_TEXT_TRANSPARENT, 0);
	int ink = 0, other = 0, outside = 0;
	for (int yy = 0; yy < cv.h; ++yy)
		for (int xx = 0; xx < cv.w; ++xx) {
			uint32_t p = cv.at(xx, yy);
			if (p == fg) {
				++ink;
				if (xx < x || xx >= x + cx || yy < y || yy >= y + cy) ++outside;
			} else if (p != bg) {
				++other;
			}
		}
	bool ok = ret == cx && other == 0 && ink > 0 && (!checkBox || outside == 0);
	if (!ok)
		Check(false, what, Fmt("returned %d extent %d, %d ink, %d blended, %d outside the box", ret, cx, ink, other, outside).c_str());
	return ok ? ink : -1;
}

static void CheckFaces()
{
	for (const FaceCase &c : kCases) {
		std::string name = Label(c);
		bf_text_font *f = Font(c.face, c.height, c.bold, c.charset);
		if (!f) { Check(false, name.c_str(), "no font"); continue; }
		bf_text_metrics m;
		bf_text_font_metrics(f, &m);
		bool sane = m.height == m.ascent + m.descent && m.ascent > 0 && m.descent >= 0 && m.em > 0 &&
		            m.height <= c.height && m.height >= c.height - 2 && m.ave_char_width > 0 &&
		            m.max_char_width >= m.ave_char_width && m.internal_leading == m.height - m.em;
		std::string detail = Fmt("-> %s em %d, height %d = %d + %d, leading %d, ave %d, max %d%s", m.mapped, m.em, m.height,
		                         m.ascent, m.descent, m.internal_leading, m.ave_char_width, m.max_char_width,
		                         m.synthetic_bold ? ", synthetic bold" : "");
		Check(sane, (name + " metrics").c_str(), detail.c_str());

		// Extent equals the sum of the advances, for each sample string.
		struct { int cp; const char *text; } samples[] = { { 1252, kReference }, { 1252, kWestern1252 }, { 949, kKorean949 } };
		bool sums = true;
		std::string widths;
		for (auto &s : samples) {
			int cx = 0, cy = 0;
			bf_text_extent(f, s.cp, s.text, -1, &cx, &cy);
			int adv[64], offs[64];
			int n = bf_text_advances(f, s.cp, s.text, -1, adv, offs, 64);
			int sum = 0;
			for (int i = 0; i < n; ++i) sum += adv[i];
			// Prefix extents equal pen positions (the caret relies on it).
			int pen = 0;
			for (int i = 0; i < n; ++i) {
				int pcx = 0;
				bf_text_extent(f, s.cp, s.text, offs[i], &pcx, NULL);
				if (pcx != pen) sums = false;
				pen += adv[i];
			}
			if (sum != cx || cy != m.height) sums = false;
			widths += Fmt(" %d", cx);
		}
		Check(sums, (name + " extent = sum of advances = prefix pen positions").c_str(), ("widths" + widths).c_str());

		int a = DrawAndInspect(f, 1252, kReference, -1, (name + " draw reference").c_str());
		int b = DrawAndInspect(f, 1252, kWestern1252, -1, (name + " draw cp1252").c_str());
		int k = DrawAndInspect(f, 949, kKorean949, -1, (name + " draw cp949").c_str());
		int outsideRows = bf_text_debug_rows_outside(f);
		Check(a > 0 && b > 0 && k > 0, (name + " unblended, inside the extent box").c_str(),
		      Fmt("%d/%d/%d ink pixels; glyph rows beyond the cell: %d", a, b, k, outsideRows).c_str());
		bf_text_font_destroy(f);
	}
}

static void CheckGlyphInk()
{
	// Every printable ASCII letter and digit and the Hangul sample leaves ink
	// at the sizes the UI uses most (strokes not lost by the 1-bit raster).
	const char *faces[] = { "Gulim", "Arial", "Tahoma" };
	for (const char *face : faces)
		for (int h = 11; h <= 15; ++h) {
			if (strcmp(face, "Gulim") == 0 && h > 13) continue;
			if (strcmp(face, "Gulim") != 0 && h < 13) continue;
			bf_text_font *f = Font(face, h, false, strcmp(face, "Gulim") == 0 ? 129 : 0);
			std::string empty;
			for (int c = 0x21; c < 0x7F; ++c) {
				char s[2] = { (char)c, 0 };
				if (DrawAndInspect(f, 1252, s, 1, "glyph", false) <= 0) empty += (char)c;
			}
			for (int i = 0; kKorean949[i]; i += (kKorean949[i] & 0x80) ? 2 : 1)
				if (kKorean949[i] & 0x80 && DrawAndInspect(f, 949, kKorean949 + i, 2, "glyph", false) <= 0) empty += "#";
			Check(empty.empty(), Fmt("%s %d: every glyph has ink", face, h).c_str(), empty.empty() ? "" : ("empty: " + empty).c_str());
			bf_text_font_destroy(f);
		}
}

static void CheckModes()
{
	bf_text_font *f = Font("Gulim", 12, false, 129);
	const uint32_t bg = 0xFFFF00FF, fg = 0x00FFFFFF, bk = 0x00202020;

	// Top-down and bottom-up bitmaps give the same picture.
	Canvas td(200, 40, bg, false), bu(200, 40, bg, true);
	bf_text_bitmap tb = td.bitmap(), bb = bu.bitmap();
	bf_text_draw(f, &tb, 3, 5, 949, kKorean949, -1, fg, BF_TEXT_TRANSPARENT, 0);
	bf_text_draw(f, &bb, 3, 5, 949, kKorean949, -1, fg, BF_TEXT_TRANSPARENT, 0);
	bool same = true;
	int ink = 0;
	for (int y = 0; y < 40; ++y)
		for (int x = 0; x < 200; ++x) {
			if (td.at(x, y) != bu.at(x, y)) same = false;
			if (td.at(x, y) == fg) ++ink;
		}
	bool flipped = bu.px[(size_t)(39 - 10) * 200 + 0] == bu.at(0, 10);
	Check(same && ink > 0 && flipped, "top-down and bottom-up bitmaps match", Fmt("%d ink pixels", ink).c_str());

	// The first row in memory of a bottom-up bitmap is the picture's last row.
	Canvas edge(50, 20, bg, true);
	bf_text_bitmap eb = edge.bitmap();
	bf_text_draw(f, &eb, 0, 0, 1252, "W", 1, fg, BF_TEXT_TRANSPARENT, 0);
	bool topInk = false, lastRowClean = true;
	for (int x = 0; x < 50; ++x) {
		if (edge.px[(size_t)19 * 50 + x] == fg || edge.px[(size_t)18 * 50 + x] == fg) topInk = true;
		if (edge.px[x] == fg) lastRowClean = false;
	}
	Check(topInk && lastRowClean, "bottom-up: text at y 0 lands in the last memory rows");

	// UTF-16 and CP949 give identical pixels and extents.
	uint16_t w[64];
	int n = bf_text_to_utf16(949, kKorean949, -1, w, 64);
	Canvas a(200, 30, bg), b(200, 30, bg);
	bf_text_bitmap ab = a.bitmap(), b2 = b.bitmap();
	int ca = bf_text_draw(f, &ab, 2, 2, 949, kKorean949, -1, fg, BF_TEXT_TRANSPARENT, 0);
	int cb = bf_text_draw(f, &b2, 2, 2, BF_TEXT_CP_UTF16, w, n, fg, BF_TEXT_TRANSPARENT, 0);
	Check(ca == cb && a.px == b.px, "UTF-16 path equals the cp949 path", Fmt("extent %d / %d", ca, cb).c_str());
	DrawAndInspect(f, BF_TEXT_CP_UTF16, w, n, "UTF-16 draw unblended");

	// Opaque: the whole cell box is background or text, nothing else.
	Canvas o(200, 30, bg);
	bf_text_bitmap ob = o.bitmap();
	int cx = bf_text_draw(f, &ob, 4, 4, 1252, kReference, -1, fg, BF_TEXT_OPAQUE, bk);
	bf_text_metrics m;
	bf_text_font_metrics(f, &m);
	int wrong = 0, outside = 0;
	for (int y = 0; y < 30; ++y)
		for (int x = 0; x < 200; ++x) {
			bool in = x >= 4 && x < 4 + cx && y >= 4 && y < 4 + m.height;
			uint32_t p = o.at(x, y);
			if (in && p != fg && p != bk) ++wrong;
			if (!in && p != bg) ++outside;
		}
	Check(cx > 0 && wrong == 0 && outside == 0, "opaque background fills exactly the cell box",
	      Fmt("%d wrong inside, %d changed outside", wrong, outside).c_str());

	// Clipping at the bitmap's edges.
	Canvas small(10, 6, bg);
	bf_text_bitmap sb = small.bitmap();
	int r = bf_text_draw(f, &sb, -5, -4, 1252, kReference, -1, fg, BF_TEXT_OPAQUE, bk);
	Check(r > 0, "drawing across the bitmap's edges is clipped");

	// Underline: one extra row of text colour across the extent.
	bf_text_font *u = Font("Arial", 15, false, 0, false, true);
	bf_text_metrics um;
	bf_text_font_metrics(u, &um);
	Canvas uc(300, 30, bg);
	bf_text_bitmap ub = uc.bitmap();
	int ux = bf_text_draw(u, &ub, 0, 0, 1252, "under", -1, fg, BF_TEXT_TRANSPARENT, 0);
	int full = -1;
	for (int y = um.ascent; y < um.height && full < 0; ++y) {
		bool all = true;
		for (int x = 0; x < ux; ++x) if (uc.at(x, y) != fg) all = false;
		if (all) full = y;
	}
	Check(full > 0, "underline is drawn below the baseline", Fmt("row %d of %d", full, um.height).c_str());
	bf_text_font_destroy(u);

	// Characters the face lacks come from other fonts (Cyrillic, Greek,
	// Japanese, Hanja, Thai in Arial), unblended, none missing.
	bf_text_font *ar = Font("Arial", 15, false, 0);
	const uint16_t other[] = { 0x041F, 0x0440, 0x0438, 0x0432, 0x0435, 0x0442, ' ', 0x03B1, 0x03B2, ' ', 0x3042, 0x30AB,
	                           ' ', 0x97D3, 0x570B, ' ', 0x0E2A, 0x0E27, 0x0E31, 0x0E2A, ' ', 0x20AC, 0x2605, 0 };
	Check(DrawAndInspect(ar, BF_TEXT_CP_UTF16, other, -1, "fallback draw", false) > 0, "fallback fonts for characters Arial lacks");
	bf_text_font_destroy(ar);

	// Italic stays unblended (it may lean out of the box, as on Windows).
	bf_text_font *it = Font("Arial", 15, false, 0, true);
	Check(DrawAndInspect(it, 1252, kReference, -1, "italic draw", false) > 0, "italic draw unblended");
	bf_text_font_destroy(it);

	bf_text_font_destroy(f);
}

static void CheckPolygon()
{
	const uint32_t bg = 0, fg = 0x00FF0000;
	Canvas c(40, 40, bg);
	bf_text_bitmap b = c.bitmap();
	int tri[] = { 0, 0, 20, 0, 0, 20 };
	int n = bf_text_fill_polygon(&b, tri, 3, fg, 1);
	// Pixel centres with (i + .5) + (j + .5) < 20, i.e. i + j <= 18: 190.
	Check(n == 190 && c.at(0, 0) == fg && c.at(18, 0) == fg && c.at(19, 0) == bg && c.at(0, 18) == fg && c.at(10, 10) == bg,
	      "polygon fill: triangle by pixel centres", Fmt("%d pixels", n).c_str());
	Canvas d(40, 40, bg);
	bf_text_bitmap db = d.bitmap();
	int sq[] = { 5, 5, 15, 5, 15, 15, 5, 15 };
	n = bf_text_fill_polygon(&db, sq, 4, fg, 0);
	Check(n == 100 && d.at(5, 5) == fg && d.at(14, 14) == fg && d.at(15, 15) == bg, "polygon fill: square, right and bottom edges excluded",
	      Fmt("%d pixels", n).c_str());
}

// ---- The sample sheet --------------------------------------------------------

static bool WritePng(Canvas &c, const char *path)
{
	// Pixels are 0xAARRGGBB in native order with alpha as the client leaves it
	// (0); the sheet is opaque.
	std::vector<uint32_t> px(c.px);
	for (uint32_t &p : px) p |= 0xFF000000u;
	CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
	CGContextRef ctx = CGBitmapContextCreate(px.data(), c.w, c.h, 8, c.w * 4, rgb,
	                                         kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
	CGImageRef img = CGBitmapContextCreateImage(ctx);
	NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
	CGImageDestinationRef dst = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, CFSTR("public.png"), 1, NULL);
	bool ok = false;
	if (dst) {
		CGImageDestinationAddImage(dst, img, NULL);
		ok = CGImageDestinationFinalize(dst);
		CFRelease(dst);
	}
	CGImageRelease(img);
	CGContextRelease(ctx);
	CGColorSpaceRelease(rgb);
	return ok;
}

static void SampleSheet(const char *path)
{
	const uint32_t white = 0x00FFFFFF, black = 0x00000000, grey = 0x00707070, dark = 0x00303848;
	Canvas c(1000, 1, white);
	bf_text_font *label = Font("Arial", 14, false, 0);
	int rows = 0;
	for (const FaceCase &fc : kCases) {
		bf_text_font *f = Font(fc.face, fc.height, fc.bold, fc.charset);
		bf_text_metrics m;
		bf_text_font_metrics(f, &m);
		rows += m.height + 6;
		bf_text_font_destroy(f);
	}
	c = Canvas(1000, rows + 80, white);
	bf_text_bitmap b = c.bitmap();
	int y = 4;
	bf_text_draw(label, &b, 4, y, 1252, "bf_text sample sheet: GDI face and cell height, the macOS font drawing it; 1 bit, no smoothing", -1, black, BF_TEXT_TRANSPARENT, 0);
	y += 22;
	for (const FaceCase &fc : kCases) {
		bf_text_font *f = Font(fc.face, fc.height, fc.bold, fc.charset);
		bf_text_metrics m;
		bf_text_font_metrics(f, &m);
		std::string l = Label(fc) + "  " + m.mapped;
		bf_text_draw(label, &b, 4, y + (m.height - 13) / 2, 1252, l.c_str(), -1, grey, BF_TEXT_TRANSPARENT, 0);
		int x = 290;
		x += bf_text_draw(f, &b, x, y, 1252, kReference, -1, black, BF_TEXT_TRANSPARENT, 0) + 8;
		x += bf_text_draw(f, &b, x, y, 1252, "\xE9\xFC\xDF\xC9", -1, black, BF_TEXT_TRANSPARENT, 0) + 8;
		x += bf_text_draw(f, &b, x, y, 949, kKorean949, -1, black, BF_TEXT_TRANSPARENT, 0) + 8;
		bf_text_draw(f, &b, x, y, 1252, "Agi 70+10", -1, white, BF_TEXT_OPAQUE, dark);
		y += m.height + 6;
		bf_text_font_destroy(f);
	}
	// The client's decorated styles: shadow and outline, as UISys draws them.
	bf_text_font *g = Font("Gulim", 12, false, 129);
	bf_text_font *gb = Font("Gulim", 13, true, 129);
	bf_text_font *ar = Font("Arial", 15, false, 0);
	int strip[] = { 286, y - 3, 760, y - 3, 760, y + 41, 286, y + 41 };
	bf_text_fill_polygon(&b, strip, 4, 0x00A0A8B8, 1);
	for (int i = 0; i < 2; ++i) {
		bf_text_font *f = i ? ar : g;
		int cp = i ? 1252 : 949;
		const char *t = i ? "Poring  Lv 1  Novice" : kKorean949;
		bf_text_draw(f, &b, 290 + 1, y + 1, cp, t, -1, 0x00FFFFFF, BF_TEXT_TRANSPARENT, 0);
		bf_text_draw(f, &b, 290, y, cp, t, -1, black, BF_TEXT_TRANSPARENT, 0);
		int x = 560;
		for (int k = 0; k < 4; ++k)
			bf_text_draw(i ? ar : gb, &b, x + (k == 0 ? -1 : k == 1 ? 1 : 0), y + (k == 2 ? -1 : k == 3 ? 1 : 0), cp, t, -1, black, BF_TEXT_TRANSPARENT, 0);
		bf_text_draw(i ? ar : gb, &b, x, y, cp, t, -1, 0x00FFFF00, BF_TEXT_TRANSPARENT, 0);
		y += 22;
	}
	bf_text_font_destroy(g);
	bf_text_font_destroy(gb);
	bf_text_font_destroy(ar);
	bf_text_font_destroy(label);
	Check(WritePng(c, path), "sample sheet written", path);
}

static void PrintWidths()
{
	printf("\nwidths of \"%s\" (pixels):\n", kReference);
	const struct { const char *face; int charset; } faces[] = {
		{ "Gulim", 129 }, { "Dotum", 129 }, { "Batang", 129 }, { "Arial", 0 }, { "Tahoma", 0 },
		{ "MS Sans Serif", 0 }, { "Times New Roman", 0 }, { "Verdana", 0 }, { "Courier New", 0 } };
	const int heights[] = { 10, 11, 12, 13, 14, 15, 16, 17, 20, 23 };
	printf("  %-16s %-26s", "face", "macOS font");
	for (int h : heights) printf(" %4d", h);
	printf("   (cell height)\n");
	for (auto &fc : faces) {
		bf_text_font *f = Font(fc.face, 12, false, fc.charset);
		bf_text_metrics m;
		bf_text_font_metrics(f, &m);
		printf("  %-16s %-26s", fc.face, m.mapped);
		bf_text_font_destroy(f);
		for (int h : heights) {
			f = Font(fc.face, h, false, fc.charset);
			int cx = 0;
			bf_text_extent(f, 1252, kReference, -1, &cx, NULL);
			printf(" %4d", cx);
			bf_text_font_destroy(f);
		}
		printf("\n");
	}
	printf("  Hangul syllable advance in Gulim:");
	for (int h : heights) {
		bf_text_font *f = Font("Gulim", h, false, 129);
		printf(" %d", bf_text_char_advance(f, 0xAC00));
		bf_text_font_destroy(f);
	}
	printf("\n\nface mapping:\n");
	const char *gdi, *mapped;
	for (int i = 0; bf_text_face_mapping(i, &gdi, &mapped) == 0; ++i) printf("  %-16s -> %s\n", gdi, mapped ? mapped : "(not installed)");
	printf("\n");
}

static std::string Sha256(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) return "";
	std::vector<unsigned char> data;
	unsigned char buf[65536];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0) data.insert(data.end(), buf, buf + n);
	fclose(f);
	unsigned char md[CC_SHA256_DIGEST_LENGTH];
	CC_SHA256(data.data(), (CC_LONG)data.size(), md);
	std::string hex;
	for (unsigned char c : md) hex += Fmt("%02x", c);
	return hex;
}

// Compares the sheet with the committed reference. A difference is reported
// but is not a failure: CoreText's rasteriser may change with macOS.
static void CompareReference(const char *sheet)
{
	std::string ref = std::string(BF_TEXT_SOURCE_DIR) + "/test/reference/text-sample-sheet.png";
	std::string a = Sha256(sheet), b = Sha256(ref.c_str());
	printf("%s  sample sheet sha256 %s %s the reference%s\n", a == b ? "ok  " : "note", a.c_str(),
	       a == b ? "equals" : "differs from", b.empty() ? " (none committed)" : "");
}

int main(int argc, char **argv)
{
	@autoreleasepool {
		bf_trace_init();
		std::string sheet = argc > 1 ? argv[1] : std::string(BF_TEXT_OUTPUT_DIR) + "/text-sample-sheet.png";
		CheckConversions();
		CheckFaces();
		CheckGlyphInk();
		CheckModes();
		CheckPolygon();
		PrintWidths();
		SampleSheet(sheet.c_str());
		CompareReference(sheet.c_str());
		printf("%s: %d failure%s\n", g_failures ? "FAILED" : "passed", g_failures, g_failures == 1 ? "" : "s");
		return g_failures ? 1 : 0;
	}
}
