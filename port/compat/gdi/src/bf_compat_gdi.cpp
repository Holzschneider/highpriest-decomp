// bf_compat_gdi: gdi32 on port/native/text (M4.5a). The names and what the
// client passes to them are in port/native/text/GDI-USAGE.md; the table of
// names is in ../README.md. Thin by design: handles, defaults and argument
// checks here; fonts, drawing and the polygon fill in bf_text.

#include <windows.h>

#include <algorithm>
#include <mutex>
#include <new>
#include <string>
#include <unordered_set>
#include <vector>

#include "bf_compat_gdi.h"
#include "bf_coverage.h"
#include "bf_text.h"
#include "bf_trace.h"

namespace {

const uint32_t kMagic = 0x47444930;   // "GDI0"

struct Obj {
	uint32_t magic = kMagic;
	int type;                 // OBJ_*
	bool stock = false;       // never freed, DeleteObject is a no-op
	bool deleted = false;     // DeleteObject while selected: freed on deselect
	int selected = 0;         // DCs it is selected into
	explicit Obj(int t) : type(t) {}
	virtual ~Obj() { magic = 0; }
};

struct Bitmap : Obj {
	void *bits = nullptr;     // owned (calloc), NULL for the default bitmap
	int width = 1, height = 1, stride = 4, bottom_up = 0;
	Bitmap() : Obj(OBJ_BITMAP) {}
	~Bitmap() override { free(bits); }
};

struct Font : Obj {
	bf_text_font *font = nullptr;
	bf_text_font_desc desc = {};
	std::string face;         // UTF-8, desc.face points here
	Font() : Obj(OBJ_FONT) {}
	~Font() override { if (font) bf_text_font_destroy(font); }
};

struct Brush : Obj {
	COLORREF color = 0;
	bool hollow = false;
	Brush() : Obj(OBJ_BRUSH) {}
};

struct Pen : Obj {
	COLORREF color = 0;
	bool null_pen = false;
	Pen() : Obj(OBJ_PEN) {}
};

struct Region : Obj {
	std::vector<int> xy;      // x, y pairs
	int mode = WINDING;
	Region() : Obj(OBJ_REGION) {}
};

struct Palette : Obj {
	Palette() : Obj(OBJ_PAL) {}
};

struct Dc : Obj {
	Bitmap *bitmap = nullptr;  // memory DC: the selected bitmap
	Font *font = nullptr;
	Brush *brush = nullptr;
	Pen *pen = nullptr;
	COLORREF text_color = RGB(0, 0, 0);
	COLORREF bk_color = RGB(255, 255, 255);
	int bk_mode = OPAQUE;
	bool external = false;     // bf_compat_gdi_dc_for_bitmap
	bf_text_bitmap ext = {};
	bool screen = false;
	bool dirty = false;
	RECT dirty_rect = {};
	Dc() : Obj(OBJ_MEMDC) {}
};

std::recursive_mutex g_lock;
std::unordered_set<const Obj *> g_live;      // every valid handle, stock ones too
int g_user_objects = 0;                      // created by the client, not yet freed

const char *TypeName(int type)
{
	switch (type) {
	case OBJ_PEN: return "pen";
	case OBJ_BRUSH: return "brush";
	case OBJ_DC: case OBJ_MEMDC: return "DC";
	case OBJ_PAL: return "palette";
	case OBJ_FONT: return "font";
	case OBJ_BITMAP: return "bitmap";
	case OBJ_REGION: return "region";
	}
	return "object";
}

template <class T> T *Register(T *o)
{
	g_live.insert(o);
	if (!o->stock) ++g_user_objects;
	return o;
}

void Free(Obj *o)
{
	g_live.erase(o);
	if (!o->stock) --g_user_objects;
	delete o;
}

// The object behind a handle, of the given type (0: any), or NULL with a trace.
Obj *Lookup(const void *h, int type, const char *fn)
{
	if (!h) {
		BF_TRACE(BFT_W32, "gdi: %s: NULL handle", fn);
		return nullptr;
	}
	const Obj *o = static_cast<const Obj *>(h);
	if (!g_live.count(o) || o->magic != kMagic) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: %s: %p is not a live GDI handle", fn, h);
		bf_count("gdi.bad_handle", 1);
		return nullptr;
	}
	if (type && o->type != type) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: %s: %p is a %s, not a %s", fn, h, TypeName(o->type), TypeName(type));
		bf_count("gdi.bad_handle", 1);
		return nullptr;
	}
	return const_cast<Obj *>(o);
}

Dc *LookupDc(HDC h, const char *fn) { return static_cast<Dc *>(Lookup(h, OBJ_MEMDC, fn)); }

// ---- Stock objects -----------------------------------------------------------

struct Stock {
	Brush brushes[6];          // WHITE_BRUSH .. NULL_BRUSH
	Pen pens[3];               // WHITE_PEN, BLACK_PEN, NULL_PEN
	Font fonts[7];             // OEM_FIXED_FONT .. SYSTEM_FIXED_FONT, DEFAULT_GUI_FONT
	Palette palette;
	Bitmap bitmap;             // the 1x1 bitmap of a fresh memory DC
	Dc screen;
};

Stock *g_stock;

Stock *Stocks()
{
	if (g_stock) return g_stock;
	Stock *s = new Stock;
	static const COLORREF grays[5] = { RGB(255, 255, 255), RGB(192, 192, 192), RGB(128, 128, 128),
	                                    RGB(64, 64, 64), RGB(0, 0, 0) };
	for (int i = 0; i < 6; ++i) {
		s->brushes[i].stock = true;
		if (i < 5) s->brushes[i].color = grays[i]; else s->brushes[i].hollow = true;
		g_live.insert(&s->brushes[i]);
	}
	s->pens[0].color = RGB(255, 255, 255);
	s->pens[2].null_pen = true;
	for (Pen &p : s->pens) { p.stock = true; g_live.insert(&p); }
	// Faces and heights of the Windows stock fonts; made when first drawn with.
	static const struct { const char *face; int height, weight; } fonts[7] = {
		{ "Courier New", 12, 400 },     // OEM_FIXED_FONT
		{ "Courier New", 12, 400 },     // ANSI_FIXED_FONT
		{ "MS Sans Serif", 13, 400 },   // ANSI_VAR_FONT
		{ "System", 16, 700 },          // SYSTEM_FONT
		{ "System", 16, 700 },          // DEVICE_DEFAULT_FONT
		{ "Courier New", 15, 400 },     // SYSTEM_FIXED_FONT
		{ "MS Sans Serif", -11, 400 },  // DEFAULT_GUI_FONT
	};
	for (int i = 0; i < 7; ++i) {
		Font &f = s->fonts[i];
		f.stock = true;
		f.face = fonts[i].face;
		f.desc.face = f.face.c_str();
		f.desc.height = fonts[i].height;
		f.desc.weight = fonts[i].weight;
		f.desc.charset = ANSI_CHARSET;
		g_live.insert(&f);
	}
	s->palette.stock = true;
	g_live.insert(&s->palette);
	s->bitmap.stock = true;
	g_live.insert(&s->bitmap);
	s->screen.stock = true;
	s->screen.screen = true;
	s->screen.bitmap = &s->bitmap;
	s->screen.font = &s->fonts[3];
	s->screen.brush = &s->brushes[0];
	s->screen.pen = &s->pens[1];
	g_live.insert(&s->screen);
	g_stock = s;
	return s;
}

Font *StockFont(int index)
{
	switch (index) {
	case OEM_FIXED_FONT: return &Stocks()->fonts[0];
	case ANSI_FIXED_FONT: return &Stocks()->fonts[1];
	case ANSI_VAR_FONT: return &Stocks()->fonts[2];
	case SYSTEM_FONT: return &Stocks()->fonts[3];
	case DEVICE_DEFAULT_FONT: return &Stocks()->fonts[4];
	case SYSTEM_FIXED_FONT: return &Stocks()->fonts[5];
	case DEFAULT_GUI_FONT: return &Stocks()->fonts[6];
	}
	return nullptr;
}

// ---- Selection ---------------------------------------------------------------

void Retain(Obj *o) { if (o) ++o->selected; }

void Release(Obj *o)
{
	if (!o) return;
	if (--o->selected == 0 && o->deleted) Free(o);
}

void InitDc(Dc *dc)
{
	Stock *s = Stocks();
	dc->bitmap = &s->bitmap;
	dc->font = &s->fonts[3];
	dc->brush = &s->brushes[0];
	dc->pen = &s->pens[1];
	Retain(dc->bitmap); Retain(dc->font); Retain(dc->brush); Retain(dc->pen);
}

// The pixels a DC draws into, or false when it has none (the default bitmap).
bool Target(Dc *dc, bf_text_bitmap *out)
{
	if (dc->external) { *out = dc->ext; return true; }
	if (!dc->bitmap || !dc->bitmap->bits) return false;
	*out = { dc->bitmap->bits, dc->bitmap->width, dc->bitmap->height, dc->bitmap->stride, dc->bitmap->bottom_up };
	return true;
}

void MarkDirty(Dc *dc, const bf_text_bitmap &bm, int left, int top, int right, int bottom)
{
	left = (std::max)(left, 0); top = (std::max)(top, 0);
	right = (std::min)(right, bm.width); bottom = (std::min)(bottom, bm.height);
	if (left >= right || top >= bottom) return;
	if (!dc->dirty) {
		dc->dirty_rect = { left, top, right, bottom };
		dc->dirty = true;
	} else {
		RECT &r = dc->dirty_rect;
		r.left = (std::min<LONG>)(r.left, left); r.top = (std::min<LONG>)(r.top, top);
		r.right = (std::max<LONG>)(r.right, right); r.bottom = (std::max<LONG>)(r.bottom, bottom);
	}
}

// GDI writes 0x00RRGGBB into a 32-bit DIB; COLORREF is 0x00BBGGRR. The high
// byte (PALETTEINDEX, PALETTERGB) means nothing for a 32-bit target.
uint32_t Pixel(COLORREF c)
{
	return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF);
}

bf_text_font *FontOf(Font *f)
{
	if (!f->font) {
		f->font = bf_text_font_create(&f->desc);
		if (!f->font) BF_TRACE(BFT_ERR | BFT_W32, "gdi: no font for '%s' %d", f->face.c_str(), f->desc.height);
	}
	return f->font;
}

Font *MakeFont(const LOGFONTA &lf)
{
	// Face names cross in the code page of the font's character set (the 9x
	// names of the client's table: CP949, Shift-JIS, GB2312, Big5 bytes).
	char face[LF_FACESIZE + 1];
	memcpy(face, lf.lfFaceName, LF_FACESIZE);
	face[LF_FACESIZE] = 0;
	std::string utf8;
	if (face[0]) {
		int cp = bf_text_codepage_for_charset(lf.lfCharSet);
		int n = bf_text_to_utf8(cp, face, -1, nullptr, 0);
		if (n < 0) n = bf_text_to_utf8(BF_TEXT_CP_WESTERN, face, -1, nullptr, 0);
		if (n > 0) {
			utf8.resize(n);
			if (bf_text_to_utf8(cp, face, -1, &utf8[0], n) < 0)
				bf_text_to_utf8(BF_TEXT_CP_WESTERN, face, -1, &utf8[0], n);
		}
		utf8.resize(strlen(utf8.c_str()));
	}
	if (lf.lfWidth || lf.lfEscapement || lf.lfOrientation)
		BF_TRACE(BFT_W32, "gdi: CreateFont: width %d, escapement %d, orientation %d ignored",
		         (int)lf.lfWidth, (int)lf.lfEscapement, (int)lf.lfOrientation);
	Font *f = new (std::nothrow) Font;
	if (!f) return nullptr;
	f->face = utf8;
	f->desc.face = f->face.empty() ? nullptr : f->face.c_str();
	f->desc.height = lf.lfHeight;
	f->desc.weight = lf.lfWeight;
	f->desc.italic = lf.lfItalic;
	f->desc.underline = lf.lfUnderline;
	f->desc.strikeout = lf.lfStrikeOut;
	f->desc.charset = lf.lfCharSet;
	f->font = bf_text_font_create(&f->desc);
	if (!f->font) {
		delete f;
		return nullptr;
	}
	BF_TRACE(BFT_W32, "gdi: CreateFont '%s' height %d weight %d charset %d", utf8.c_str(),
	         (int)lf.lfHeight, (int)lf.lfWeight, (int)lf.lfCharSet);
	return Register(f);
}

// ---- Text ----------------------------------------------------------------------

BOOL TextOutCp(HDC hdc, int x, int y, int codepage, const void *text, int count, const char *fn)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, fn);
	if (!dc) return FALSE;
	if (count < 0 || (count > 0 && !text)) return FALSE;
	if (count == 0) return TRUE;
	bf_text_bitmap bm;
	if (!Target(dc, &bm)) return TRUE;      // the default 1x1 bitmap: nothing shows
	bf_text_font *font = FontOf(dc->font);
	if (!font) return FALSE;
	if (codepage == 0) codepage = bf_text_codepage_for_charset(dc->font->desc.charset);
	// The coverage report (M9.5): Korean or other non-Windows-1252 text drawn.
	bf_coverage_drawn(codepage == BF_TEXT_CP_UTF16 ? BF_COVERAGE_UTF16 : codepage, text, count);
	int mode = dc->bk_mode == OPAQUE ? BF_TEXT_OPAQUE : BF_TEXT_TRANSPARENT;
	int cx = bf_text_draw(font, &bm, x, y, codepage, text, count, Pixel(dc->text_color), mode, Pixel(dc->bk_color));
	if (cx < 0) return FALSE;
	bf_text_metrics m;
	int cy = bf_text_font_metrics(font, &m) == 0 ? m.height : 0;
	MarkDirty(dc, bm, x, y, x + cx, y + cy);
	return TRUE;
}

BOOL ExtentCp(HDC hdc, int codepage, const void *text, int count, LPSIZE size, const char *fn)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, fn);
	if (!dc || !size || count < 0 || (count > 0 && !text)) return FALSE;
	bf_text_font *font = FontOf(dc->font);
	if (!font) return FALSE;
	if (codepage == 0) codepage = bf_text_codepage_for_charset(dc->font->desc.charset);
	int cx = 0, cy = 0;
	if (bf_text_extent(font, codepage, count ? text : "", count, &cx, &cy) < 0) return FALSE;
	size->cx = cx;
	size->cy = cy;
	return TRUE;
}

// ---- Font enumeration --------------------------------------------------------

// The character sets a face is enumerated under, as on an English Windows XP
// with the East Asian fonts installed. The first is the face's own.
const int kLatin[] = { ANSI_CHARSET, TURKISH_CHARSET, RUSSIAN_CHARSET, VIETNAMESE_CHARSET, GREEK_CHARSET,
                       BALTIC_CHARSET, EASTEUROPE_CHARSET, 0 };

std::vector<int> CharsetsOf(const char *face)
{
	struct Entry { const char *face; int charset; };
	static const Entry own[] = {
		{ "Gulim", HANGUL_CHARSET }, { "Dotum", HANGUL_CHARSET }, { "Batang", HANGUL_CHARSET },
		{ "MS Gothic", SHIFTJIS_CHARSET }, { "SimSun", GB2312_CHARSET }, { "MingLiU", CHINESEBIG5_CHARSET },
	};
	for (const Entry &e : own)
		if (!strcasecmp(face, e.face)) return { e.charset, ANSI_CHARSET };
	if (!strcasecmp(face, "MS Sans Serif")) return { ANSI_CHARSET };
	std::vector<int> v(kLatin, kLatin + 7);
	if (!strcasecmp(face, "Tahoma")) v.push_back(THAI_CHARSET);
	return v;
}

} // namespace

// ================================================================== gdi32

extern "C" {

HDC WINAPI CreateCompatibleDC(HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	if (hdc && !LookupDc(hdc, "CreateCompatibleDC")) return nullptr;
	Dc *dc = new (std::nothrow) Dc;
	if (!dc) return nullptr;
	InitDc(dc);
	return reinterpret_cast<HDC>(Register(dc));
}

BOOL WINAPI DeleteDC(HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, "DeleteDC");
	if (!dc) return FALSE;
	if (dc->stock) {
		BF_TRACE(BFT_W32, "gdi: DeleteDC: the screen DC is released, not deleted");
		return FALSE;
	}
	Release(dc->bitmap); Release(dc->font); Release(dc->brush); Release(dc->pen);
	Free(dc);
	return TRUE;
}

HBITMAP WINAPI CreateDIBSection(HDC hdc, const BITMAPINFO *info, UINT usage, void **bits, HANDLE section, DWORD offset)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	if (bits) *bits = nullptr;
	if (hdc && !LookupDc(hdc, "CreateDIBSection")) return nullptr;
	if (!info) return nullptr;
	const BITMAPINFOHEADER &h = info->bmiHeader;
	if (h.biSize < sizeof(BITMAPINFOHEADER) || h.biWidth <= 0 || h.biHeight == 0 || h.biPlanes != 1) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: CreateDIBSection: bad header (size %u, %dx%d, planes %d)",
		         (unsigned)h.biSize, (int)h.biWidth, (int)h.biHeight, (int)h.biPlanes);
		return nullptr;
	}
	if (h.biBitCount != 32 || (h.biCompression != BI_RGB && h.biCompression != BI_BITFIELDS)) {
		// The client makes 32-bit BI_RGB sections only (CDCBitmap).
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: CreateDIBSection: %d bpp, compression %u not supported",
		         (int)h.biBitCount, (unsigned)h.biCompression);
		BF_STUB();
		return nullptr;
	}
	if (section) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: CreateDIBSection: file-mapping sections not supported");
		BF_STUB();
		return nullptr;
	}
	(void)usage; (void)offset;   // DIB_RGB_COLORS / DIB_PAL_COLORS mean nothing at 32 bpp
	long long w = h.biWidth, rows = h.biHeight < 0 ? -(long long)h.biHeight : h.biHeight;
	if (w * rows * 4 > (1LL << 30)) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: CreateDIBSection: %lldx%lld is too large", w, rows);
		return nullptr;
	}
	Bitmap *b = new (std::nothrow) Bitmap;
	if (!b) return nullptr;
	b->width = (int)w;
	b->height = (int)rows;
	b->stride = (int)w * 4;
	b->bottom_up = h.biHeight > 0;
	b->bits = calloc((size_t)(w * rows), 4);   // a DIB section starts zeroed
	if (!b->bits) {
		delete b;
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: CreateDIBSection: out of memory for %lldx%lld", w, rows);
		return nullptr;
	}
	if (bits) *bits = b->bits;
	return reinterpret_cast<HBITMAP>(Register(b));
}

HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ handle)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, "SelectObject");
	if (!dc) return nullptr;
	Obj *o = Lookup(handle, 0, "SelectObject");
	if (!o || o->deleted) return nullptr;
	Obj *old = nullptr;
	switch (o->type) {
	case OBJ_BITMAP: {
		Bitmap *b = static_cast<Bitmap *>(o);
		if (dc->external || dc->screen) {
			BF_TRACE(BFT_ERR | BFT_W32, "gdi: SelectObject: a bitmap into a DC that is not a memory DC");
			return nullptr;
		}
		if (b == dc->bitmap) return handle;
		if (!b->stock && b->selected) {
			BF_TRACE(BFT_ERR | BFT_W32, "gdi: SelectObject: bitmap %p is selected into another DC", handle);
			return nullptr;
		}
		old = dc->bitmap;
		dc->bitmap = b;
		break;
	}
	case OBJ_FONT: old = dc->font; dc->font = static_cast<Font *>(o); break;
	case OBJ_BRUSH: old = dc->brush; dc->brush = static_cast<Brush *>(o); break;
	case OBJ_PEN: old = dc->pen; dc->pen = static_cast<Pen *>(o); break;
	case OBJ_REGION:
		// SelectClipRgn; the client never clips.
		BF_STUB();
		return nullptr;
	default:
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: SelectObject: a %s cannot be selected", TypeName(o->type));
		return nullptr;
	}
	Retain(o);
	Release(old);   // old stays valid unless the client deleted it while selected
	return reinterpret_cast<HGDIOBJ>(old);
}

HGDIOBJ WINAPI GetCurrentObject(HDC hdc, UINT type)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, "GetCurrentObject");
	if (!dc) return nullptr;
	switch (type) {
	case OBJ_FONT: return reinterpret_cast<HGDIOBJ>(dc->font);
	case OBJ_BITMAP: return dc->external ? nullptr : reinterpret_cast<HGDIOBJ>(dc->bitmap);
	case OBJ_BRUSH: return reinterpret_cast<HGDIOBJ>(dc->brush);
	case OBJ_PEN: return reinterpret_cast<HGDIOBJ>(dc->pen);
	case OBJ_PAL: return reinterpret_cast<HGDIOBJ>(&Stocks()->palette);
	}
	BF_TRACE(BFT_W32, "gdi: GetCurrentObject: type %u", type);
	return nullptr;
}

BOOL WINAPI DeleteObject(HGDIOBJ handle)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Obj *o = Lookup(handle, 0, "DeleteObject");
	if (!o) return FALSE;
	if (o->type == OBJ_MEMDC || o->type == OBJ_DC) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: DeleteObject: %p is a DC (DeleteDC)", handle);
		return FALSE;
	}
	if (o->stock) return TRUE;
	if (o->deleted) return FALSE;
	if (o->selected) {
		// Windows frees an object deleted while selected when it is deselected.
		BF_TRACE(BFT_W32, "gdi: DeleteObject: %s %p is still selected; freed when deselected", TypeName(o->type), handle);
		o->deleted = true;
		g_live.erase(o);
		return TRUE;
	}
	Free(o);
	return TRUE;
}

HGDIOBJ WINAPI GetStockObject(INT index)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Stock *s = Stocks();
	if (index >= WHITE_BRUSH && index <= NULL_BRUSH) return reinterpret_cast<HGDIOBJ>(&s->brushes[index]);
	if (index >= WHITE_PEN && index <= NULL_PEN) return reinterpret_cast<HGDIOBJ>(&s->pens[index - WHITE_PEN]);
	if (index == DEFAULT_PALETTE) return reinterpret_cast<HGDIOBJ>(&s->palette);
	if (Font *f = StockFont(index)) return reinterpret_cast<HGDIOBJ>(f);
	BF_TRACE(BFT_W32, "gdi: GetStockObject(%d): no such stock object", (int)index);
	return nullptr;
}

HFONT WINAPI CreateFontA(INT height, INT width, INT escapement, INT orientation, INT weight, DWORD italic,
                         DWORD underline, DWORD strikeout, DWORD charset, DWORD out_precision,
                         DWORD clip_precision, DWORD quality, DWORD pitch, LPCSTR face)
{
	LOGFONTA lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = height;
	lf.lfWidth = width;
	lf.lfEscapement = escapement;
	lf.lfOrientation = orientation;
	lf.lfWeight = weight;
	lf.lfItalic = (BYTE)italic;
	lf.lfUnderline = (BYTE)underline;
	lf.lfStrikeOut = (BYTE)strikeout;
	lf.lfCharSet = (BYTE)charset;
	lf.lfOutPrecision = (BYTE)out_precision;
	lf.lfClipPrecision = (BYTE)clip_precision;
	lf.lfQuality = (BYTE)quality;       // always unsmoothed: bf_text never blends
	lf.lfPitchAndFamily = (BYTE)pitch;
	if (face) strncpy(lf.lfFaceName, face, LF_FACESIZE - 1);
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	return reinterpret_cast<HFONT>(MakeFont(lf));
}

HFONT WINAPI CreateFontIndirectA(const LOGFONTA *lf)
{
	if (!lf) return nullptr;
	LOGFONTA copy = *lf;
	copy.lfFaceName[LF_FACESIZE - 1] = 0;
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	return reinterpret_cast<HFONT>(MakeFont(copy));
}

INT WINAPI EnumFontFamiliesExA(HDC hdc, LPLOGFONTA lf, FONTENUMPROCA proc, LPARAM lparam, DWORD flags)
{
	(void)hdc; (void)flags;   // any DC (the client passes GetDC(NULL)); flags are reserved
	if (!lf || !proc) return 0;
	// The client passes a face-name pointer as lParam and reads it in its
	// callback; an index past its face table yields a value that is no
	// pointer. Nothing below 4 GB (__PAGEZERO) is one: answer "not found".
	if ((uintptr_t)lparam < 0x100000000ull) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: EnumFontFamiliesEx: lParam %#llx is not a pointer, callback not called",
		         (unsigned long long)lparam);
		return 1;
	}
	char want[LF_FACESIZE + 1];
	memcpy(want, lf->lfFaceName, LF_FACESIZE);
	want[LF_FACESIZE] = 0;
	const char *gdi = nullptr, *mapped = nullptr;
	int ret = 1;   // nothing enumerated: the callback's "no match"
	for (int i = 0; bf_text_face_mapping(i, &gdi, &mapped) == 0; ++i) {
		if (!mapped) continue;                       // not installed here
		if (want[0] && strcasecmp(want, gdi)) continue;
		std::vector<int> sets = CharsetsOf(gdi);
		for (int cs : sets) {
			if (lf->lfCharSet != DEFAULT_CHARSET && cs != lf->lfCharSet) continue;
			ENUMLOGFONTEXA elf;
			NEWTEXTMETRICEXA ntm;
			memset(&elf, 0, sizeof(elf));
			memset(&ntm, 0, sizeof(ntm));
			elf.elfLogFont.lfHeight = 16;
			elf.elfLogFont.lfWeight = FW_NORMAL;
			elf.elfLogFont.lfCharSet = (BYTE)cs;
			elf.elfLogFont.lfOutPrecision = OUT_STROKE_PRECIS;
			elf.elfLogFont.lfClipPrecision = CLIP_STROKE_PRECIS;
			elf.elfLogFont.lfQuality = DRAFT_QUALITY;
			elf.elfLogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
			strncpy(elf.elfLogFont.lfFaceName, gdi, LF_FACESIZE - 1);
			strncpy((char *)elf.elfFullName, gdi, LF_FULLFACESIZE - 1);
			strcpy((char *)elf.elfStyle, "Regular");
			NEWTEXTMETRICA &tm = ntm.ntmTm;
			tm.tmHeight = 16;
			tm.tmWeight = FW_NORMAL;
			tm.tmPitchAndFamily = TMPF_TRUETYPE | TMPF_VECTOR | TMPF_FIXED_PITCH | FF_SWISS;
			tm.tmCharSet = (BYTE)cs;
			tm.ntmFlags = NTM_REGULAR;
			ret = proc(&elf.elfLogFont, (const TEXTMETRICA *)&ntm, TRUETYPE_FONTTYPE, lparam);
			if (!ret) return 0;
			if (lf->lfCharSet == DEFAULT_CHARSET) break;   // once per face is enough
		}
	}
	return ret;
}

INT WINAPI SetBkMode(HDC hdc, INT mode)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, "SetBkMode");
	if (!dc) return 0;
	if (mode != TRANSPARENT && mode != OPAQUE) {
		BF_TRACE(BFT_W32, "gdi: SetBkMode: mode %d", (int)mode);
		return 0;
	}
	int old = dc->bk_mode;
	dc->bk_mode = mode;
	return old;
}

COLORREF WINAPI SetBkColor(HDC hdc, COLORREF color)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, "SetBkColor");
	if (!dc) return CLR_INVALID;
	COLORREF old = dc->bk_color;
	dc->bk_color = color;
	return old;
}

COLORREF WINAPI SetTextColor(HDC hdc, COLORREF color)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, "SetTextColor");
	if (!dc) return CLR_INVALID;
	COLORREF old = dc->text_color;
	dc->text_color = color;
	return old;
}

BOOL WINAPI TextOutW(HDC hdc, INT x, INT y, LPCWSTR text, INT count)
{
	return TextOutCp(hdc, x, y, BF_TEXT_CP_UTF16, text, count, "TextOutW");
}

BOOL WINAPI TextOutA(HDC hdc, INT x, INT y, LPCSTR text, INT count)
{
	return TextOutCp(hdc, x, y, 0, text, count, "TextOutA");
}

BOOL WINAPI GetTextExtentPoint32W(HDC hdc, LPCWSTR text, INT count, LPSIZE size)
{
	return ExtentCp(hdc, BF_TEXT_CP_UTF16, text, count, size, "GetTextExtentPoint32W");
}

BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR text, INT count, LPSIZE size)
{
	return ExtentCp(hdc, 0, text, count, size, "GetTextExtentPoint32A");
}

HRGN WINAPI CreatePolygonRgn(const POINT *points, INT count, INT mode)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	if (!points || count < 0 || (mode != ALTERNATE && mode != WINDING)) return nullptr;
	Region *r = new (std::nothrow) Region;
	if (!r) return nullptr;
	r->mode = mode;
	if (count >= 3) {   // fewer points enclose nothing: an empty region
		r->xy.reserve(count * 2);
		for (int i = 0; i < count; ++i) {
			r->xy.push_back(points[i].x);
			r->xy.push_back(points[i].y);
		}
	}
	return reinterpret_cast<HRGN>(Register(r));
}

HBRUSH WINAPI CreateSolidBrush(COLORREF color)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Brush *b = new (std::nothrow) Brush;
	if (!b) return nullptr;
	b->color = color;
	return reinterpret_cast<HBRUSH>(Register(b));
}

BOOL WINAPI FillRgn(HDC hdc, HRGN region, HBRUSH brush)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = LookupDc(hdc, "FillRgn");
	Region *r = static_cast<Region *>(Lookup(region, OBJ_REGION, "FillRgn"));
	Brush *b = static_cast<Brush *>(Lookup(brush, OBJ_BRUSH, "FillRgn"));
	if (!dc || !r || !b) return FALSE;
	bf_text_bitmap bm;
	if (b->hollow || r->xy.empty() || !Target(dc, &bm)) return TRUE;
	int n = (int)r->xy.size() / 2;
	if (bf_text_fill_polygon(&bm, r->xy.data(), n, Pixel(b->color), r->mode == WINDING) < 0) return FALSE;
	int l = INT32_MAX, t = INT32_MAX, rr = INT32_MIN, bb = INT32_MIN;
	for (int i = 0; i < n; ++i) {
		l = (std::min)(l, r->xy[2 * i]); rr = (std::max)(rr, r->xy[2 * i]);
		t = (std::min)(t, r->xy[2 * i + 1]); bb = (std::max)(bb, r->xy[2 * i + 1]);
	}
	MarkDirty(dc, bm, l, t, rr + 1, bb + 1);
	return TRUE;
}

// ================================================================== port side

HDC bf_compat_gdi_dc_for_bitmap(void *bits, int width, int height, int pitch, int bottom_up)
{
	if (!bits || width <= 0 || height <= 0 || pitch < width * 4) {
		BF_TRACE(BFT_ERR | BFT_W32, "gdi: dc_for_bitmap: bad bitmap %p %dx%d pitch %d", bits, width, height, pitch);
		return nullptr;
	}
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Dc *dc = new (std::nothrow) Dc;
	if (!dc) return nullptr;
	InitDc(dc);
	Release(dc->bitmap);
	dc->bitmap = nullptr;
	dc->external = true;
	dc->ext = { bits, width, height, pitch, bottom_up ? 1 : 0 };
	return reinterpret_cast<HDC>(Register(dc));
}

BOOL bf_compat_gdi_dc_take_dirty(HDC hdc, RECT *out)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	if (out) *out = { 0, 0, 0, 0 };
	Dc *dc = LookupDc(hdc, "bf_compat_gdi_dc_take_dirty");
	if (!dc || !dc->dirty) return FALSE;
	if (out) *out = dc->dirty_rect;
	dc->dirty = false;
	return TRUE;
}

HDC bf_compat_gdi_screen_dc(void)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	return reinterpret_cast<HDC>(&Stocks()->screen);
}

BOOL bf_compat_gdi_brush_color(HBRUSH brush, COLORREF *out)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	Brush *b = static_cast<Brush *>(Lookup(brush, OBJ_BRUSH, "bf_compat_gdi_brush_color"));
	if (!b || b->hollow) return FALSE;
	if (out) *out = b->color;
	return TRUE;
}

int bf_compat_gdi_live_objects(void)
{
	std::lock_guard<std::recursive_mutex> lock(g_lock);
	return g_user_objects;
}

} // extern "C"
