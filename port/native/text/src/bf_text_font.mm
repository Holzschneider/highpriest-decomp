// Fonts, measuring and drawing for bf_text, on CoreText.
//
// How it stays close to GDI:
// - Glyphs are rasterised by CoreGraphics with antialiasing, font smoothing
//   and subpixel positioning off, into an 8-bit mask that is then read as
//   on/off. Each glyph is rasterised once per font at an integer pen
//   position and cached; a string is drawn by copying cached masks, so a
//   glyph looks the same wherever it is drawn, as with GDI's glyph cache.
// - Advances are integers: the font's hdmx entry for the em size when it
//   has one (the hinted widths GDI uses), else the rounded advance. No
//   kerning and no ligatures, so a prefix's extent is the pen position.
// - Cell height, ascent and descent follow GDI's rules for lfHeight using
//   the metrics of the Windows original (VDMX and OS/2 win metrics where
//   the macOS font is the same font, a table for the substituted faces).
// - Faces whose Windows original has no bold (Gulim, Dotum, MS Sans Serif)
//   are emboldened as GDI does: the glyph doubled one pixel to the right
//   and every advance one pixel wider.

#include "bf_text.h"
#include "bf_text_internal.h"
#include "bf_trace.h"

#import <CoreFoundation/CoreFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>

#include <math.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// ---- The face table ------------------------------------------------------

enum { kRegular, kBold, kItalic, kBoldItalic };

struct Face {
	const char *gdi;
	const char *aliases[8];
	const char *ps[4][3];     // [style][candidate] PostScript names; empty: derive
	// Vertical metrics of the Windows original. 0: the macOS font is the
	// same font, read its own OS/2 and VDMX tables.
	int upem, winAscent, winDescent;
	bool cjk;                 // Hangul and other characters >= U+2000 are one em wide
	bool boldSynthetic;       // the Windows original has no bold face
};

// The Korean Windows faces (Gulim, Dotum, Batang, Gungsuh) and MS Gothic,
// SimSun, MingLiU share one design grid: 256 units per em, ascent 220,
// descent 36, so the cell equals the em (internal leading 0) and Hangul
// advances are one em.
const Face kFaces[] = {
	{ "Gulim", { "\xEA\xB5\xB4\xEB\xA6\xBC", "GulimChe", "\xEA\xB5\xB4\xEB\xA6\xBC\xEC\xB2\xB4" },
	  { { "AppleGothic", "AppleSDGothicNeo-Regular" } }, 256, 220, 36, true, true },
	{ "Dotum", { "\xEB\x8F\x8B\xEC\x9B\x80", "DotumChe", "\xEB\x8F\x8B\xEC\x9B\x80\xEC\xB2\xB4" },
	  { { "AppleGothic", "AppleSDGothicNeo-Regular" } }, 256, 220, 36, true, true },
	{ "Batang", { "\xEB\xB0\x94\xED\x83\x95", "BatangChe", "\xEB\xB0\x94\xED\x83\x95\xEC\xB2\xB4",
	              "Gungsuh", "\xEA\xB6\x81\xEC\x84\x9C", "GungsuhChe", "\xEA\xB6\x81\xEC\x84\x9C\xEC\xB2\xB4" },
	  { { "AppleMyungjo", "AppleGothic", "AppleSDGothicNeo-Regular" } }, 256, 220, 36, true, true },
	{ "Arial", { },
	  { { "ArialMT" }, { "Arial-BoldMT" }, { "Arial-ItalicMT" }, { "Arial-BoldItalicMT" } }, 0, 0, 0, false, false },
	{ "Tahoma", { },
	  { { "Tahoma" }, { "Tahoma-Bold" } }, 0, 0, 0, false, false },
	{ "MS Sans Serif", { "Microsoft Sans Serif", "System" },
	  { { "MicrosoftSansSerif" } }, 0, 0, 0, false, true },
	{ "Times New Roman", { },
	  { { "TimesNewRomanPSMT" }, { "TimesNewRomanPS-BoldMT" }, { "TimesNewRomanPS-ItalicMT" },
	    { "TimesNewRomanPS-BoldItalicMT" } }, 0, 0, 0, false, false },
	{ "Verdana", { },
	  { { "Verdana" }, { "Verdana-Bold" }, { "Verdana-Italic" }, { "Verdana-BoldItalic" } }, 0, 0, 0, false, false },
	{ "Courier New", { },
	  { { "CourierNewPSMT" }, { "CourierNewPS-BoldMT" }, { "CourierNewPS-ItalicMT" },
	    { "CourierNewPS-BoldItalicMT" } }, 0, 0, 0, false, false },
	{ "MS Gothic", { "\xEF\xBC\xAD\xEF\xBC\xB3 \xE3\x82\xB4\xE3\x82\xB7\xE3\x83\x83\xE3\x82\xAF", "MS PGothic", "MS UI Gothic" },
	  { { "HiraginoSans-W3" }, { "HiraginoSans-W6" } }, 256, 220, 36, true, false },
	{ "SimSun", { "\xE5\xAE\x8B\xE4\xBD\x93", "NSimSun" },
	  { { "STHeitiSC-Light", "STSongti-SC-Regular" }, { "STHeitiSC-Medium", "STSongti-SC-Bold" } }, 256, 220, 36, true, false },
	{ "MingLiU", { "\xE7\xB4\xB0\xE6\x98\x8E\xE9\xAB\x94", "PMingLiU" },
	  { { "STHeitiTC-Light", "STSongti-TC-Regular" }, { "STHeitiTC-Medium", "STSongti-TC-Bold" } }, 256, 220, 36, true, false },
};
const int kFaceCount = sizeof(kFaces) / sizeof(kFaces[0]);
const int kGulim = 0, kArial = 3, kMsGothic = 9, kSimSun = 10, kMingLiU = 11;

bool SameName(const char *a, const char *b)
{
	for (; *a && *b; ++a, ++b) {
		unsigned ca = (unsigned char)*a, cb = (unsigned char)*b;
		if (ca < 0x80) ca = tolower(ca);
		if (cb < 0x80) cb = tolower(cb);
		if (ca != cb) return false;
	}
	return *a == *b;
}

int FindFace(const char *name, int charset)
{
	if (name && *name == '@') ++name;  // vertical-writing variant
	if (name && *name)
		for (int i = 0; i < kFaceCount; ++i) {
			if (SameName(name, kFaces[i].gdi)) return i;
			for (const char *alias : kFaces[i].aliases)
				if (alias && SameName(name, alias)) return i;
		}
	switch (charset) {
	case BF_TEXT_CHARSET_HANGUL:      return kGulim;
	case BF_TEXT_CHARSET_SHIFTJIS:    return kMsGothic;
	case BF_TEXT_CHARSET_GB2312:      return kSimSun;
	case BF_TEXT_CHARSET_CHINESEBIG5: return kMingLiU;
	default:                          return kArial;
	}
}

// A font by PostScript name, or NULL when it is not installed (CoreText
// would hand back a substitute).
CTFontRef CreateExact(const char *ps, CGFloat size, const CGAffineTransform *matrix)
{
	CFStringRef name = CFStringCreateWithCString(NULL, ps, kCFStringEncodingUTF8);
	CTFontRef font = CTFontCreateWithName(name, size, matrix);
	CFStringRef got = font ? CTFontCopyPostScriptName(font) : NULL;
	bool same = got && CFStringCompare(got, name, 0) == kCFCompareEqualTo;
	if (got) CFRelease(got);
	CFRelease(name);
	if (font && !same) { CFRelease(font); font = NULL; }
	return font;
}

std::string PsName(CTFontRef font)
{
	CFStringRef s = CTFontCopyPostScriptName(font);
	char buf[128] = "";
	if (s) { CFStringGetCString(s, buf, sizeof(buf), kCFStringEncodingUTF8); CFRelease(s); }
	return buf;
}

// ---- Tables --------------------------------------------------------------

struct Table {
	CFDataRef data = NULL;
	const uint8_t *p = NULL;
	size_t n = 0;
	Table(CTFontRef font, CTFontTableTag tag) {
		data = CTFontCopyTable(font, tag, kCTFontTableOptionNoOptions);
		if (data) { p = CFDataGetBytePtr(data); n = (size_t)CFDataGetLength(data); }
	}
	~Table() { if (data) CFRelease(data); }
	bool has(size_t off, size_t len) const { return p && off + len <= n; }
	unsigned u8(size_t o) const { return has(o, 1) ? p[o] : 0; }
	unsigned u16(size_t o) const { return has(o, 2) ? (unsigned)(p[o] << 8 | p[o + 1]) : 0; }
	int s16(size_t o) const { return (int16_t)u16(o); }
	unsigned u32(size_t o) const { return has(o, 4) ? (unsigned)p[o] << 24 | p[o + 1] << 16 | p[o + 2] << 8 | p[o + 3] : 0; }
};

struct VdmxEntry { int ppem, yMax, yMin; };

// The VDMX group for a 1:1 device, sorted by ppem.
std::vector<VdmxEntry> ReadVdmx(CTFontRef font)
{
	std::vector<VdmxEntry> out;
	Table t(font, 'VDMX');
	if (!t.has(0, 6)) return out;
	unsigned numRatios = t.u16(4);
	for (unsigned i = 0; i < numRatios; ++i) {
		size_t r = 6 + 4 * i;
		unsigned x = t.u8(r + 1), y0 = t.u8(r + 2), y1 = t.u8(r + 3);
		bool match = (x == 0 && y0 == 0 && y1 == 0) || (y0 <= x && x <= y1);
		if (!match) continue;
		size_t group = t.u16(6 + 4 * numRatios + 2 * i);
		unsigned recs = t.u16(group);
		for (unsigned k = 0; k < recs; ++k) {
			size_t e = group + 4 + 6 * k;
			if (!t.has(e, 6)) break;
			out.push_back({ (int)t.u16(e), t.s16(e + 2), t.s16(e + 4) });
		}
		break;
	}
	return out;
}

// hdmx widths (per glyph) for one em size, or empty.
std::vector<uint8_t> ReadHdmx(CTFontRef font, int ppem)
{
	std::vector<uint8_t> out;
	Table t(font, 'hdmx');
	if (!t.has(0, 8)) return out;
	unsigned records = t.u16(2);
	unsigned size = t.u32(4);
	size_t glyphs = (size_t)CTFontGetGlyphCount(font);
	for (unsigned i = 0; i < records; ++i) {
		size_t r = 8 + (size_t)size * i;
		if (!t.has(r, 2 + glyphs)) break;
		if ((int)t.u8(r) == ppem) { out.assign(t.p + r + 2, t.p + r + 2 + glyphs); break; }
	}
	return out;
}

// ---- Glyphs --------------------------------------------------------------

struct Glyph {
	int advance = 0;
	int left = 0, top = 0, w = 0, h = 0;   // mask box: from the pen x, from the cell top
	std::vector<uint8_t> bits;             // w * h, 0 or 1
};

bool FullWidth(uint32_t c)
{
	if (c < 0x1100) return false;
	if (c >= 0xFF61 && c <= 0xFFDC) return false;   // half-width forms
	if (c >= 0x1100 && c <= 0x11FF) return true;    // Hangul Jamo
	return c >= 0x2000;
}

bool IsCjk(uint32_t c)
{
	return (c >= 0x1100 && c <= 0x11FF) || (c >= 0x2E80 && c <= 0x9FFF) || (c >= 0xAC00 && c <= 0xD7A3) ||
	       (c >= 0xF900 && c <= 0xFAFF) || (c >= 0xFF00 && c <= 0xFFEF);
}

} // namespace

struct bf_text_font {
	bf_text_metrics m;
	const Face *face = NULL;
	CTFontRef primary = NULL;
	bool shear = false;
	bool primaryIsOriginal = false;
	std::vector<uint8_t> hdmx;
	std::map<std::string, CTFontRef> fallbacks;   // by PostScript name
	std::unordered_map<uint32_t, Glyph> glyphs;
	int underlineRow = 1, underlineThick = 1, strikeRow = 0;

	~bf_text_font() {
		if (primary) CFRelease(primary);
		for (auto &f : fallbacks) if (f.second) CFRelease(f.second);
	}
};

namespace {

CGAffineTransform ShearMatrix() { return CGAffineTransformMake(1, 0, 0.2, 1, 0, 0); }

CTFontRef Fallback(bf_text_font *f, const char *ps)
{
	auto it = f->fallbacks.find(ps);
	if (it != f->fallbacks.end()) return it->second;
	CGAffineTransform shear = ShearMatrix();
	CTFontRef font = CreateExact(ps, f->m.em, f->shear ? &shear : NULL);
	f->fallbacks[ps] = font;  // NULL remembered as well
	return font;
}

bool GlyphFor(CTFontRef font, const UniChar *u, int n, CGGlyph *g)
{
	CGGlyph gs[2] = { 0, 0 };
	if (!font || !CTFontGetGlyphsForCharacters(font, u, gs, n)) return false;
	*g = gs[0];
	return gs[0] != 0;
}

// Rasterises glyph g of font at the pen, x offset xoff, into out.
void Rasterize(bf_text_font *f, CTFontRef font, CGGlyph g, int xoff, Glyph &out)
{
	CGRect box;
	CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationHorizontal, &g, &box, 1);
	if (CGRectIsEmpty(box) || CGRectIsNull(box)) return;
	int margin = 2 + (f->shear ? f->m.em / 2 : 0);
	int x0 = (int)floor(box.origin.x) - margin, x1 = (int)ceil(CGRectGetMaxX(box)) + margin;
	int y0 = (int)floor(box.origin.y) - margin, y1 = (int)ceil(CGRectGetMaxY(box)) + margin;
	int W = x1 - x0, H = y1 - y0;
	if (W <= 0 || H <= 0 || W > 4096 || H > 4096) return;

	std::vector<uint8_t> mask((size_t)W * H, 0);
	CGColorSpaceRef gray = CGColorSpaceCreateDeviceGray();
	CGContextRef ctx = CGBitmapContextCreate(mask.data(), W, H, 8, W, gray, kCGImageAlphaNone);
	CGColorSpaceRelease(gray);
	if (!ctx) return;
	CGContextSetAllowsAntialiasing(ctx, false);
	CGContextSetShouldAntialias(ctx, false);
	CGContextSetAllowsFontSmoothing(ctx, false);
	CGContextSetShouldSmoothFonts(ctx, false);
	CGContextSetAllowsFontSubpixelPositioning(ctx, false);
	CGContextSetShouldSubpixelPositionFonts(ctx, false);
	CGContextSetAllowsFontSubpixelQuantization(ctx, false);
	CGContextSetShouldSubpixelQuantizeFonts(ctx, false);
	CGContextSetGrayFillColor(ctx, 1, 1);
	CGPoint at = CGPointMake(-x0, -y0);   // the pen and the baseline
	CTFontDrawGlyphs(font, &g, &at, 1, ctx);
	CGContextRelease(ctx);

	// Memory row r is CG row H-1-r; its pixel spans [H-1-r, H-r) in CG y, so
	// relative to the baseline it is (H-r+y0) above it: cell row ascent - that.
	int minc = W, maxc = -1, minr = H, maxr = -1;
	for (int r = 0; r < H; ++r)
		for (int c = 0; c < W; ++c)
			if (mask[(size_t)r * W + c] >= 128) {
				minc = std::min(minc, c); maxc = std::max(maxc, c);
				minr = std::min(minr, r); maxr = std::max(maxr, r);
			}
	if (maxc < 0) return;
	int bold = f->m.synthetic_bold ? 1 : 0;
	out.w = maxc - minc + 1 + bold;
	out.h = maxr - minr + 1;
	out.left = minc + x0 + xoff;
	out.top = f->m.ascent - (H + y0 - minr);
	out.bits.assign((size_t)out.w * out.h, 0);
	for (int r = 0; r < out.h; ++r)
		for (int c = 0; c < out.w - bold; ++c)
			if (mask[(size_t)(r + minr) * W + c + minc] >= 128) {
				out.bits[(size_t)r * out.w + c] = 1;
				if (bold) out.bits[(size_t)r * out.w + c + 1] = 1;
			}
}

const Glyph &GetGlyph(bf_text_font *f, uint32_t c)
{
	auto it = f->glyphs.find(c);
	if (it != f->glyphs.end()) return it->second;
	Glyph &out = f->glyphs[c];
	if (c < 0x20 || c == 0x7F) return out;   // control characters draw nothing

	UniChar u[2];
	int n = 1;
	if (c >= 0x10000) { u[0] = (UniChar)(0xD800 + ((c - 0x10000) >> 10)); u[1] = (UniChar)(0xDC00 + ((c - 0x10000) & 0x3FF)); n = 2; }
	else u[0] = (UniChar)c;

	CTFontRef font = NULL;
	CGGlyph g = 0;
	if (GlyphFor(f->primary, u, n, &g)) font = f->primary;
	if (!font && IsCjk(c) && !f->face->cjk) {
		// Korean in a Western face: GDI's font linking draws it in Gulim.
		for (const char *ps : kFaces[kGulim].ps[kRegular])
			if (ps && GlyphFor(Fallback(f, ps), u, n, &g)) { font = Fallback(f, ps); break; }
	}
	if (!font) {
		CFStringRef s = CFStringCreateWithCharacters(NULL, u, n);
		CTFontRef sys = CTFontCreateForString(f->primary, s, CFRangeMake(0, n));
		CFRelease(s);
		if (sys) {
			// Cascade fonts may be hidden system fonts (".SF..."), which cannot be
			// made by name: copy the one CoreText returned to the em size.
			std::string ps = PsName(sys);
			auto it = f->fallbacks.find(ps);
			if (it == f->fallbacks.end()) {
				CGAffineTransform shear = ShearMatrix();
				it = f->fallbacks.emplace(ps, CTFontCreateCopyWithAttributes(sys, f->m.em, f->shear ? &shear : NULL, NULL)).first;
			}
			CFRelease(sys);
			if (GlyphFor(it->second, u, n, &g)) font = it->second;
		}
	}
	if (!font) {
		bf_count("text.missing_glyph", 1);
		BF_TRACE(BFT_UI, "text: no font has U+%04X; drawn as the missing glyph of %s", (unsigned)c, f->m.mapped);
		font = f->primary;
		g = 0;
	}

	CGSize adv;
	CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, &g, &adv, 1);
	int natural = (font == f->primary && g < f->hdmx.size()) ? f->hdmx[g] : (int)lround(adv.width);
	int advance = natural, xoff = 0;
	if (f->face->cjk && FullWidth(c)) {
		advance = f->m.em;
		xoff = (advance - natural) / 2;
	}
	Rasterize(f, font, g, xoff, out);
	out.advance = advance + (f->m.synthetic_bold ? 1 : 0);
	return out;
}

struct Decoded {
	std::vector<uint32_t> chars;
	std::vector<int> offsets;
};

bool DecodeChars(int codepage, const void *text, int len, Decoded &d)
{
	std::vector<uint16_t> units;
	std::vector<int> offs;
	if (!text) return true;
	if (bf_text_decode(codepage, text, len, units, &offs) < 0) return false;
	for (size_t i = 0; i < units.size(); ++i) {
		uint32_t c = units[i];
		int off = offs[i];
		if (c >= 0xD800 && c < 0xDC00 && i + 1 < units.size() && units[i + 1] >= 0xDC00 && units[i + 1] < 0xE000) {
			c = 0x10000 + ((c - 0xD800) << 10) + (units[i + 1] - 0xDC00);
			++i;
		}
		d.chars.push_back(c);
		d.offsets.push_back(off);
	}
	return true;
}

void Put(const bf_text_bitmap *b, int x, int y, uint32_t color)
{
	if (x < 0 || y < 0 || x >= b->width || y >= b->height) return;
	int row = b->bottom_up ? b->height - 1 - y : y;
	((uint32_t *)((uint8_t *)b->pixels + (size_t)row * b->stride))[x] = color;
}

bool ValidBitmap(const bf_text_bitmap *b)
{
	return b && b->pixels && b->width > 0 && b->height > 0 && b->stride >= b->width * 4;
}

std::vector<std::string> g_resolved;   // per face: the macOS font used, "" if none

} // namespace

// ---- Fonts ---------------------------------------------------------------

extern "C" bf_text_font *bf_text_font_create(const bf_text_font_desc *desc)
{
	bf_text_font_desc d = {};
	if (desc) d = *desc;
	if (d.height == 0) d.height = -16;
	bool bold = d.weight >= 600;
	int style = (bold ? 1 : 0) + (d.italic ? 2 : 0);
	int faceIndex = FindFace(d.face, d.charset);
	const Face *face = &kFaces[faceIndex];

	// Find the style's font; failing that a lesser style, simulating the rest.
	static const int order[4][4] = {
		{ kRegular, -1 }, { kBold, kRegular, -1 }, { kItalic, kRegular, -1 },
		{ kBoldItalic, kBold, kItalic, kRegular } };
	CTFontRef probe = NULL;
	const char *psName = NULL;
	int got = -1;
	for (int k = 0; k < 4 && order[style][k] >= 0 && !probe; ++k) {
		int s = order[style][k];
		for (const char *ps : face->ps[s]) {
			if (!ps) continue;
			if ((probe = CreateExact(ps, 12, NULL))) { psName = ps; got = s; break; }
		}
	}
	if (!probe) {
		// The face's fonts are not installed: Arial, then whatever CoreText offers.
		for (const char *ps : { "ArialMT", "Helvetica" })
			if ((probe = CreateExact(ps, 12, NULL))) { psName = ps; got = kRegular; break; }
		if (!probe) { bf_tracef(BFT_ERR, "text: no font for face %s", d.face ? d.face : "(none)"); return NULL; }
		bf_tracef(BFT_UI, "text: no installed font for %s, using %s", face->gdi, psName);
	}

	bf_text_font *f = new bf_text_font;
	memset(&f->m, 0, sizeof(f->m));
	f->face = face;
	bool gotBold = got == kBold || got == kBoldItalic;
	bool gotItalic = got == kItalic || got == kBoldItalic;
	f->m.synthetic_bold = bold && !gotBold;
	f->shear = d.italic && !gotItalic;
	f->primaryIsOriginal = face->upem == 0;

	// Vertical metrics and the em, by GDI's rules.
	int upem = face->upem, asc = face->winAscent, desc_ = face->winDescent;
	std::vector<VdmxEntry> vdmx;
	if (!upem) {
		Table os2(probe, 'OS/2');
		upem = (int)CTFontGetUnitsPerEm(probe);
		asc = (int)os2.u16(74);
		desc_ = (int)os2.u16(76);
		if (!asc) { asc = (int)lround(CTFontGetAscent(probe) / 12 * upem); desc_ = (int)lround(CTFontGetDescent(probe) / 12 * upem); }
		vdmx = ReadVdmx(probe);
	}
	int em = 0, ascent = 0, descent = 0;
	if (d.height > 0) {
		for (const VdmxEntry &e : vdmx)
			if (e.yMax - e.yMin <= d.height && e.ppem > em) { em = e.ppem; ascent = e.yMax; descent = -e.yMin; }
		if (!em) {
			em = std::max(1, (int)lround((double)d.height * upem / (asc + desc_)));
			ascent = (int)lround((double)asc * em / upem);
			descent = (int)lround((double)desc_ * em / upem);
		}
	} else {
		em = -d.height;
		for (const VdmxEntry &e : vdmx)
			if (e.ppem == em) { ascent = e.yMax; descent = -e.yMin; }
		if (!ascent && !descent) {
			ascent = (int)lround((double)asc * em / upem);
			descent = (int)lround((double)desc_ * em / upem);
		}
	}
	CFRelease(probe);

	CGAffineTransform shear = ShearMatrix();
	f->primary = CreateExact(psName, em, f->shear ? &shear : NULL);
	if (!f->primary) { bf_tracef(BFT_ERR, "text: %s vanished", psName); delete f; return NULL; }
	f->hdmx = ReadHdmx(f->primary, em);

	bf_text_metrics &m = f->m;
	m.em = em;
	m.ascent = ascent;
	m.descent = descent;
	m.height = ascent + descent;
	m.internal_leading = std::max(0, m.height - em);
	m.external_leading = 0;
	m.weight = bold ? 700 : 400;
	m.italic = d.italic ? 1 : 0;
	m.underline = d.underline ? 1 : 0;
	m.strikeout = d.strikeout ? 1 : 0;
	m.charset = d.charset;
	m.pitch_and_family = 0x07;
	strlcpy(m.face, face->gdi, sizeof(m.face));
	strlcpy(m.mapped, psName, sizeof(m.mapped));

	{
		Table os2(f->primary, 'OS/2'), hhea(f->primary, 'hhea'), post(f->primary, 'post');
		int fupem = (int)CTFontGetUnitsPerEm(f->primary);
		int sb = m.synthetic_bold ? 1 : 0;
		if (face->cjk) {
			m.ave_char_width = em / 2 + sb;
			m.max_char_width = em + sb;
		} else {
			int avg = os2.has(2, 2) ? os2.s16(2) : fupem / 2;
			m.ave_char_width = (int)lround((double)avg * em / fupem) + sb;
			int maxw = 0;
			if (!f->hdmx.empty()) maxw = *std::max_element(f->hdmx.begin(), f->hdmx.end());
			else if (hhea.has(10, 2)) maxw = (int)lround((double)hhea.u16(10) * em / fupem);
			m.max_char_width = maxw + sb;
		}
		int upos = post.has(8, 2) ? post.s16(8) : -fupem / 10;
		int uthick = post.has(10, 2) ? post.s16(10) : fupem / 20;
		f->underlineRow = std::max(1, (int)lround(-(double)upos * em / fupem));
		f->underlineThick = std::max(1, (int)lround((double)uthick * em / fupem));
		int spos = os2.has(28, 2) ? os2.s16(28) : (int)(fupem * 0.26);
		if (face->cjk || spos <= 0) spos = (int)(fupem * 0.3);
		f->strikeRow = std::max(1, (int)lround((double)spos * em / fupem));
	}
	return f;
}

extern "C" void bf_text_font_destroy(bf_text_font *font)
{
	delete font;
}

extern "C" int bf_text_font_metrics(const bf_text_font *font, bf_text_metrics *out)
{
	if (!font || !out) return -1;
	*out = font->m;
	return 0;
}

// ---- Measuring -----------------------------------------------------------

extern "C" int bf_text_extent(bf_text_font *font, int codepage, const void *text, int len, int *cx, int *cy)
{
	if (!font) return -1;
	Decoded d;
	if (!DecodeChars(codepage, text, len, d)) return -1;
	int w = 0;
	for (uint32_t c : d.chars) w += GetGlyph(font, c).advance;
	if (cx) *cx = w;
	if (cy) *cy = font->m.height;
	return 0;
}

extern "C" int bf_text_advances(bf_text_font *font, int codepage, const void *text, int len,
                                int *advances, int *offsets, int max)
{
	if (!font) return -1;
	Decoded d;
	if (!DecodeChars(codepage, text, len, d)) return -1;
	int n = (int)d.chars.size();
	for (int i = 0; i < n && i < max; ++i) {
		if (advances) advances[i] = GetGlyph(font, d.chars[i]).advance;
		if (offsets) offsets[i] = d.offsets[i];
	}
	return n;
}

extern "C" int bf_text_char_advance(bf_text_font *font, uint32_t codepoint)
{
	if (!font) return -1;
	return GetGlyph(font, codepoint).advance;
}

// ---- Drawing -------------------------------------------------------------

extern "C" int bf_text_draw(bf_text_font *font, const bf_text_bitmap *bitmap, int x, int y,
                            int codepage, const void *text, int len,
                            uint32_t color, int bk_mode, uint32_t bk_color)
{
	if (!font || !ValidBitmap(bitmap)) { bf_tracef(BFT_ERR, "text: bf_text_draw without a font or bitmap"); return -1; }
	Decoded d;
	if (!DecodeChars(codepage, text, len, d)) return -1;

	int cx = 0;
	for (uint32_t c : d.chars) cx += GetGlyph(font, c).advance;
	int height = font->m.height;

	if (bk_mode == BF_TEXT_OPAQUE)
		for (int r = 0; r < height; ++r)
			for (int c = 0; c < cx; ++c) Put(bitmap, x + c, y + r, bk_color);

	int pen = x;
	for (uint32_t ch : d.chars) {
		const Glyph &g = GetGlyph(font, ch);
		for (int r = 0; r < g.h; ++r) {
			int row = g.top + r;
			if (row < 0 || row >= height) continue;   // outside the cell
			const uint8_t *bits = &g.bits[(size_t)r * g.w];
			for (int c = 0; c < g.w; ++c)
				if (bits[c]) Put(bitmap, pen + g.left + c, y + row, color);
		}
		pen += g.advance;
	}

	if (font->m.underline) {
		int row = std::min(font->m.ascent + font->underlineRow, height - 1);
		for (int t = 0; t < font->underlineThick && row + t < height; ++t)
			for (int c = 0; c < cx; ++c) Put(bitmap, x + c, y + row + t, color);
	}
	if (font->m.strikeout) {
		int row = font->m.ascent - font->strikeRow;
		for (int c = 0; c < cx; ++c) Put(bitmap, x + c, y + row, color);
	}
	return cx;
}

extern "C" int bf_text_fill_polygon(const bf_text_bitmap *bitmap, const int *xy, int count,
                                    uint32_t color, int winding)
{
	if (!ValidBitmap(bitmap) || !xy || count < 3) return -1;
	int ymin = xy[1], ymax = xy[1];
	for (int i = 1; i < count; ++i) { ymin = std::min(ymin, xy[2 * i + 1]); ymax = std::max(ymax, xy[2 * i + 1]); }
	ymin = std::max(ymin, 0);
	ymax = std::min(ymax, bitmap->height);
	int set = 0;
	std::vector<std::pair<double, int>> hits;
	for (int y = ymin; y < ymax; ++y) {
		double yc = y + 0.5;
		hits.clear();
		for (int i = 0; i < count; ++i) {
			int j = (i + 1) % count;
			double x0 = xy[2 * i], y0 = xy[2 * i + 1], x1 = xy[2 * j], y1 = xy[2 * j + 1];
			if ((y0 <= yc && yc < y1) || (y1 <= yc && yc < y0)) {
				double xc = x0 + (yc - y0) * (x1 - x0) / (y1 - y0);
				hits.push_back({ xc, y1 > y0 ? 1 : -1 });
			}
		}
		std::sort(hits.begin(), hits.end());
		int wind = 0;
		for (size_t k = 0; k + 1 < hits.size(); ++k) {
			wind = winding ? wind + hits[k].second : (wind ^ 1);
			if (wind == 0) continue;
			// Pixels whose centre lies in [hits[k], hits[k+1]).
			int c0 = (int)ceil(hits[k].first - 0.5), c1 = (int)ceil(hits[k + 1].first - 0.5);
			for (int c = std::max(c0, 0); c < c1 && c < bitmap->width; ++c) { Put(bitmap, c, y, color); ++set; }
		}
	}
	return set;
}

int bf_text_debug_rows_outside(const bf_text_font *font)
{
	int n = 0;
	for (const auto &e : font->glyphs) {
		const Glyph &g = e.second;
		if (g.h == 0) continue;
		n += std::max(0, -g.top) + std::max(0, g.top + g.h - font->m.height);
	}
	return n;
}

// ---- The face table ------------------------------------------------------

extern "C" int bf_text_face_mapping(int index, const char **gdi_face, const char **mapped)
{
	if (index < 0 || index >= kFaceCount) return -1;
	if (g_resolved.empty()) {
		for (int i = 0; i < kFaceCount; ++i) {
			std::string found;
			for (const char *ps : kFaces[i].ps[kRegular]) {
				if (!ps) continue;
				CTFontRef font = CreateExact(ps, 12, NULL);
				if (font) { CFRelease(font); found = ps; break; }
			}
			g_resolved.push_back(found);
		}
	}
	if (gdi_face) *gdi_face = kFaces[index].gdi;
	if (mapped) *mapped = g_resolved[index].empty() ? NULL : g_resolved[index].c_str();
	return 0;
}
