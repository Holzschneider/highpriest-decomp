// bifrost-compat-gdi-test: the gdi32 adapter, called through the Win32 names
// in the sequences of the client's DrawDC, FontInfo, CDCBitmap and
// UIWindow::DrawTriangleDC (port/native/text/GDI-USAGE.md). One line per
// check; exits non-zero on a failure. Needs no game data. Writes
// gdi-sample.png (a UI-window-like bitmap of generated text) to BF_TEST_OUT
// or the build directory and compares it with test/reference/.
#include <windows.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "bf_compat_gdi.h"
#include "bf_text.h"

int bf_gdi_test_write_png(const char *path, const void *pixels, int width, int height, int stride);

// ---- Names of other areas, faked for this test ----------------------------------

// kernel32 (the win32 area): DrawDC converts with MultiByteToWideChar first.
extern "C" INT WINAPI MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR text, INT len, LPWSTR out, INT out_max)
{
	(void)flags;
	return bf_text_to_utf16((int)cp, text, len, (uint16_t *)out, out_max);
}

// user32 (the input area): FontInfo::GetFontFace enumerates on GetDC(NULL).
extern "C" HDC WINAPI GetDC(HWND hwnd)
{
	return hwnd ? NULL : bf_compat_gdi_screen_dc();
}

namespace {

int g_failures = 0, g_checks = 0;

void Check(bool ok, const char *format, ...)
{
	++g_checks;
	if (!ok) ++g_failures;
	printf("%s ", ok ? "ok  " : "FAIL");
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

const uint32_t kKey = 0xFFFF00FF;   // CDCBitmap's colour key, after ClearSurface

uint32_t Pixel(COLORREF c) { return ((c & 0xFF) << 16) | (c & 0xFF00) | ((c >> 16) & 0xFF); }

std::string Cp(int cp, const char *utf8)
{
	char buf[512];
	int n = bf_text_from_utf8(cp, utf8, -1, buf, sizeof(buf));
	return std::string(buf, n > 0 ? n : 0);
}

// ---- The client's font table (data of src/3dlib/DrawUtil.cpp) -------------------

const char *kFaceNT[11] = { "Gulim", "Arial", "MS Gothic", "SimSun", "MingLiU", "Tahoma",
                            "Arial", "Verdana", "Verdana", "Arial", "Arial" };
const char *kFace9x[11] = { "\xB1\xBC\xB8\xB2", "Arial",
                            "\x82\x6C\x82\x72\x20\x82\x6F\x83\x53\x83\x56\x83\x62\x83\x4E",
                            "\xCB\xCE\xCC\xE5", "\xB2\xD3\xA9\xFA\xC5\xE9", "Tahoma", "Arial",
                            "Verdana", "Verdana", "Arial", "Arial" };
const int kCharset[11] = { HANGUL_CHARSET, ANSI_CHARSET, SHIFTJIS_CHARSET, GB2312_CHARSET,
                           CHINESEBIG5_CHARSET, THAI_CHARSET, TURKISH_CHARSET, ANSI_CHARSET,
                           ANSI_CHARSET, RUSSIAN_CHARSET, VIETNAMESE_CHARSET };

int CALLBACK EnumProc(const LOGFONTA *lf, const TEXTMETRICA *tm, DWORD type, LPARAM lparam)
{
	(void)tm; (void)type;
	return strcasecmp((const char *)lparam, lf->lfFaceName);
}

// FontInfo::GetFontFace: the 9x name if it enumerates, else the NT name, else NULL.
const char *GetFontFace(int type, int charset)
{
	LOGFONTA lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfCharSet = (BYTE)charset;
	if (!EnumFontFamiliesExA(GetDC(NULL), &lf, (FONTENUMPROCA)EnumProc, (LPARAM)kFace9x[type], 0))
		return kFace9x[type];
	if (EnumFontFamiliesExA(GetDC(NULL), &lf, (FONTENUMPROCA)EnumProc, (LPARAM)kFaceNT[type], 0) == 0)
		return kFaceNT[type];
	return NULL;
}

// FontInfo::Create.
HFONT MakeFont(int type, int height, int charset, bool bold)
{
	return CreateFontA(height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, 0, 0, 0, charset, OUT_DEFAULT_PRECIS,
	                   CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, GetFontFace(type, charset));
}

// ---- CDCBitmap ------------------------------------------------------------------

struct WindowBitmap {
	HDC dc = NULL;
	HBITMAP bitmap = NULL, old = NULL;
	uint32_t *image = NULL;
	int w = 0, h = 0;
	bool bottom_up = false;

	bool Create(int width, int height, bool up = false)
	{
		w = width; h = height; bottom_up = up;
		BITMAPINFO info;
		memset(&info, 0, sizeof(info));
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biWidth = w;
		info.bmiHeader.biHeight = up ? h : -h;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biXPelsPerMeter = 100;
		info.bmiHeader.biYPelsPerMeter = 100;
		bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void **)&image, NULL, 0);
		if (!bitmap) image = NULL;
		old = (HBITMAP)SelectObject(dc, bitmap);
		return bitmap && image && old;
	}
	bool Open(int width, int height, bool up = false)
	{
		dc = CreateCompatibleDC(NULL);
		return dc && Create(width, height, up);
	}
	void Close()
	{
		SelectObject(dc, old);
		DeleteObject(bitmap);
		DeleteDC(dc);
	}
	// Top-down coordinates whatever the orientation.
	uint32_t &At(int x, int y) { return image[(bottom_up ? h - 1 - y : y) * w + x]; }
	void Clear(uint32_t c) { for (int i = 0; i < w * h; ++i) image[i] = c; }
};

// ---- DrawDC ---------------------------------------------------------------------

struct DrawDC {
	HDC hdc;
	HFONT old;
	explicit DrawDC(HDC dc) : hdc(dc)
	{
		old = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
		SetBkMode(hdc, TRANSPARENT);
	}
	~DrawDC() { SelectObject(hdc, old); }
	// _textout / _gettextsize: convert in the charset's code page, then the W call.
	BOOL Out(HFONT font, int cp, int x, int y, const std::string &bytes)
	{
		std::vector<WCHAR> wide(bytes.size() + 1);
		int n = MultiByteToWideChar(cp, 0, bytes.data(), (int)bytes.size(), wide.data(), (int)wide.size());
		SelectObject(hdc, font);
		return TextOutW(hdc, x, y, wide.data(), n);
	}
	SIZE Size(HFONT font, int cp, const std::string &bytes)
	{
		SIZE size = { -1, -1 };
		std::vector<WCHAR> wide(bytes.size() + 1);
		int n = MultiByteToWideChar(cp, 0, bytes.data(), (int)bytes.size(), wide.data(), (int)wide.size());
		SelectObject(hdc, font);
		GetTextExtentPoint32W(hdc, wide.data(), n, &size);
		return size;
	}
};

struct Ink { int count = 0, stray = 0, left = 1 << 30, top = 1 << 30, right = -1, bottom = -1; };

Ink Scan(WindowBitmap &b, uint32_t background, uint32_t color)
{
	Ink ink;
	for (int y = 0; y < b.h; ++y)
		for (int x = 0; x < b.w; ++x) {
			uint32_t p = b.At(x, y);
			if (p == background) continue;
			if (p != color) { ++ink.stray; continue; }
			++ink.count;
			if (x < ink.left) ink.left = x;
			if (x > ink.right) ink.right = x;
			if (y < ink.top) ink.top = y;
			if (y > ink.bottom) ink.bottom = y;
		}
	return ink;
}

// Every font the client asks for (GDI-USAGE.md, "Faces, character sets and
// heights"): the type, the height passed to CreateFont, bold.
struct Case { int type, height; bool bold; const char *utf8; };
const Case kCases[] = {
	{ 0, 10, false, u8"메신저 친구 10" }, { 0, 11, false, u8"작은 라벨 11" },
	{ 0, 12, false, u8"초보자 레벨 99 / 직업 50" }, { 0, 16, false, u8"사용할 수 없음 16" },
	{ 0, 12, true, u8"이름 풍선 12" }, { 0, 13, true, u8"이름 풍선 Name 13" },
	{ 1, 13, false, "Novice Lv. 10" }, { 1, 14, false, "Str 99 + 12  Zeny 1,234,567" },
	{ 1, 15, false, u8"Prontera Café — 5 € (Arial 15)" }, { 1, 19, false, "Not Available 19" },
	{ 1, 15, true, "Name balloon (bold 15)" }, { 1, 16, false, "Arial 16" }, { 1, 18, false, "Arial 18" },
	{ 2, 12, false, "MS Gothic 12" }, { 3, 12, false, "SimSun 12" }, { 4, 12, false, "MingLiU 12" },
	{ 5, 15, false, "Tahoma 15" }, { 6, 15, false, "Arial Turkish 15" }, { 7, 15, false, "Verdana 15" },
	{ 8, 14, false, "Verdana 14" }, { 9, 15, false, "Arial Russian 15" }, { 10, 15, false, "Arial Vietnamese 15" },
};

void TestHandles()
{
	HGDIOBJ black = GetStockObject(BLACK_BRUSH);
	COLORREF c = 1;
	Check(black && bf_compat_gdi_brush_color((HBRUSH)black, &c) && c == RGB(0, 0, 0),
	      "GetStockObject(BLACK_BRUSH) is a black brush (the window class background)");
	Check(DeleteObject(black) && GetStockObject(BLACK_BRUSH) == black && bf_compat_gdi_brush_color((HBRUSH)black, &c),
	      "DeleteObject of a stock object succeeds and leaves it valid");
	Check(!bf_compat_gdi_brush_color((HBRUSH)GetStockObject(NULL_BRUSH), &c), "NULL_BRUSH has no colour");
	Check(GetStockObject(SYSTEM_FONT) && GetStockObject(DEFAULT_GUI_FONT) && GetStockObject(BLACK_PEN)
	      && !GetStockObject(9) && !GetStockObject(1000), "stock fonts and pens exist; 9 and 1000 do not");

	HDC dc = CreateCompatibleDC(NULL);
	HBRUSH brush = CreateSolidBrush(RGB(1, 2, 3));
	int stray = 0;
	SIZE size;
	WCHAR text[2] = { 'A', 0 };
	Check(dc && brush, "CreateCompatibleDC(NULL), CreateSolidBrush");
	Check(!SelectObject(NULL, brush) && !SelectObject(dc, (HGDIOBJ)dc) && !SelectObject(dc, (HGDIOBJ)&stray),
	      "SelectObject: a NULL DC, a DC as object, a stray pointer all fail");
	Check(!DeleteObject((HGDIOBJ)&stray) && !DeleteObject(NULL) && !DeleteObject((HGDIOBJ)dc),
	      "DeleteObject: a stray pointer, NULL, a DC all fail");
	Check(!DeleteDC((HDC)brush) && !TextOutW((HDC)brush, 0, 0, text, 1)
	      && !GetTextExtentPoint32W((HDC)brush, text, 1, &size) && SetTextColor((HDC)brush, 0) == CLR_INVALID
	      && SetBkMode((HDC)brush, TRANSPARENT) == 0, "a brush passed as a DC is refused by every DC call");
	const POINT zero[3] = {};
	HRGN rgn = CreatePolygonRgn(zero, 3, WINDING);
	Check(rgn && !FillRgn(dc, (HRGN)brush, (HBRUSH)rgn) && FillRgn(dc, rgn, brush),
	      "FillRgn with region and brush swapped fails; in order succeeds");
	Check(!CreatePolygonRgn(NULL, 3, WINDING) && !CreatePolygonRgn((const POINT *)&stray, 3, 7),
	      "CreatePolygonRgn: no points, bad mode fail");
	Check(SetBkMode(dc, TRANSPARENT) == OPAQUE && SetBkMode(dc, 5) == 0 && SetBkMode(dc, OPAQUE) == TRANSPARENT,
	      "SetBkMode: a fresh DC is OPAQUE; bad modes fail; returns the old mode");
	Check(SetTextColor(dc, RGB(9, 8, 7)) == RGB(0, 0, 0) && SetTextColor(dc, 0) == RGB(9, 8, 7)
	      && SetBkColor(dc, 0) == RGB(255, 255, 255), "SetTextColor/SetBkColor return the old colour, defaults black/white");
	Check(DeleteObject(rgn) && DeleteObject(brush) && !DeleteObject(brush) && DeleteDC(dc) && !DeleteDC(dc),
	      "objects delete once; a second delete fails");
	Check(!DeleteDC(bf_compat_gdi_screen_dc()) && GetDC(NULL) == bf_compat_gdi_screen_dc(),
	      "the screen DC cannot be deleted");
	Check(bf_compat_gdi_live_objects() == 0, "no objects left (%d)", bf_compat_gdi_live_objects());
}

void TestBitmaps()
{
	WindowBitmap b;
	bool ok = b.Open(300, 70);
	bool zero = ok;
	for (int i = 0; ok && i < b.w * b.h; ++i) zero = zero && b.image[i] == 0;
	Check(ok && zero, "CreateDIBSection 300x70 top-down, 32 bpp: bits zeroed, SelectObject returns the default bitmap");
	Check(GetCurrentObject(b.dc, OBJ_BITMAP) == (HGDIOBJ)b.bitmap, "GetCurrentObject(OBJ_BITMAP) is the section");
	HDC other = CreateCompatibleDC(b.dc);
	Check(other && !SelectObject(other, b.bitmap), "a section selected into one DC cannot go into a second");
	DeleteDC(other);
	// CDCBitmap::Resize.
	HBITMAP first = b.bitmap;
	Check(SelectObject(b.dc, b.old) == (HGDIOBJ)first && DeleteObject(first), "Resize: deselect and delete the old section");
	ok = b.Create(400, 90);
	Check(ok && GetCurrentObject(b.dc, OBJ_BITMAP) == (HGDIOBJ)b.bitmap, "Resize: a 400x90 section, the default bitmap returned again");
	// Deleting a selected section frees it when deselected.
	int before = bf_compat_gdi_live_objects();
	Check(DeleteObject(b.bitmap) && bf_compat_gdi_live_objects() == before, "DeleteObject of a selected section: deferred");
	HGDIOBJ prev = SelectObject(b.dc, b.old);
	Check(prev == (HGDIOBJ)b.bitmap && bf_compat_gdi_live_objects() == before - 1, "freed when deselected");
	DeleteDC(b.dc);

	BITMAPINFO info;
	memset(&info, 0, sizeof(info));
	info.bmiHeader.biSize = sizeof(info.bmiHeader);
	info.bmiHeader.biWidth = 8;
	info.bmiHeader.biHeight = -8;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 16;
	void *bits = (void *)1;
	Check(!CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0) && bits == NULL,
	      "CreateDIBSection: 16 bpp is refused, bits set to NULL");
	Check(bf_compat_gdi_live_objects() == 0, "no objects left (%d)", bf_compat_gdi_live_objects());
}

void TestEnumeration()
{
	LOGFONTA lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfCharSet = HANGUL_CHARSET;
	Check(EnumFontFamiliesExA(GetDC(NULL), &lf, (FONTENUMPROCA)EnumProc, (LPARAM)"Gulim", 0) == 0,
	      "EnumFontFamiliesEx: Gulim enumerates under HANGUL_CHARSET");
	Check(EnumFontFamiliesExA(GetDC(NULL), &lf, (FONTENUMPROCA)EnumProc, (LPARAM)kFace9x[0], 0) != 0,
	      "EnumFontFamiliesEx: the CP949 name does not (an English NT: the client takes the NT name)");
	Check(EnumFontFamiliesExA(NULL, &lf, (FONTENUMPROCA)EnumProc, (LPARAM)0x81, 0) != 0,
	      "EnumFontFamiliesEx: an lParam that is no pointer (face type past the table) answers not found");
	lf.lfCharSet = THAI_CHARSET;
	Check(EnumFontFamiliesExA(NULL, &lf, (FONTENUMPROCA)EnumProc, (LPARAM)"Tahoma", 0) == 0, "Tahoma under THAI_CHARSET");
	lf.lfCharSet = ANSI_CHARSET;
	Check(EnumFontFamiliesExA(NULL, &lf, (FONTENUMPROCA)EnumProc, (LPARAM)"Gulim", 0) == 0
	      && EnumFontFamiliesExA(NULL, &lf, (FONTENUMPROCA)EnumProc, (LPARAM)"Arial", 0) == 0,
	      "Gulim and Arial under ANSI_CHARSET");
	for (int t = 0; t < 11; ++t) {
		const char *face = GetFontFace(t, kCharset[t]);
		Check(face && !strcmp(face, kFaceNT[t]), "GetFontFace(type %d, charset %d) = %s", t, kCharset[t], face ? face : "NULL");
	}
}

// One font of the table: the DrawDC sequence, pixels, extents, additivity.
void TestFont(const Case &c, HFONT font, int charset)
{
	int cp = bf_text_codepage_for_charset(charset);
	std::string bytes = Cp(cp, c.utf8);
	char label[96];
	snprintf(label, sizeof(label), "%s %d%s", kFaceNT[c.type], c.height, c.bold ? " bold" : "");
	const COLORREF color = RGB(0x20, 0x40, 0xC0);
	WindowBitmap b;
	b.Open(420, 40);
	b.Clear(kKey);
	SIZE all, empty;
	{
		DrawDC d(b.dc);
		SetTextColor(b.dc, color);
		Check(d.Out(font, cp, 4, 6, bytes), "%s: TextOutW of %d bytes in CP%d", label, (int)bytes.size(), cp);
		all = d.Size(font, cp, bytes);
		empty = d.Size(font, cp, std::string());
	}
	Ink ink = Scan(b, kKey, Pixel(color));
	Check(ink.count > 0 && ink.stray == 0, "%s: %d text pixels, all exactly 0x%08X (alpha 0); %d others", label,
	      ink.count, Pixel(color), ink.stray);
	Check(ink.left >= 4 && ink.right < 4 + all.cx && ink.top >= 6 && ink.bottom < 6 + all.cy,
	      "%s: ink [%d..%d]x[%d..%d] inside the extent box %ldx%ld at (4,6)", label, ink.left, ink.right, ink.top,
	      ink.bottom, (long)all.cx, (long)all.cy);
	Check(all.cy <= c.height && all.cy >= c.height - 3 && empty.cx == 0 && empty.cy == all.cy,
	      "%s: cy %ld is the cell (asked %d), whatever the string", label, (long)all.cy, c.height);

	// Prefix widths add up (caret, word wrap), and drawing in pieces at the
	// pieces' extents gives the same pixels as drawing the whole (colour codes).
	bool additive = true;
	int at = 0;
	int split = -1;
	while (at < (int)bytes.size()) {
		at += bf_text_char_length(cp, bytes.data() + at, (int)bytes.size() - at);
		HDC dc = b.dc;
		DrawDC d(dc);
		SIZE l = d.Size(font, cp, bytes.substr(0, at)), r = d.Size(font, cp, bytes.substr(at));
		if (l.cx + r.cx != all.cx) additive = false;
		if (split < 0 && at * 2 >= (int)bytes.size()) split = at;
	}
	Check(additive, "%s: extent(prefix) + extent(rest) = extent(all) at every character", label);
	WindowBitmap p;
	p.Open(420, 40);
	p.Clear(kKey);
	{
		DrawDC d(p.dc);
		SetTextColor(p.dc, color);
		SIZE l = d.Size(font, cp, bytes.substr(0, split));
		d.Out(font, cp, 4, 6, bytes.substr(0, split));
		d.Out(font, cp, 4 + l.cx, 6, bytes.substr(split));
	}
	Check(!memcmp(p.image, b.image, (size_t)b.w * b.h * 4), "%s: drawn in two pieces = drawn whole", label);
	Check(GetCurrentObject(b.dc, OBJ_FONT) == GetStockObject(SYSTEM_FONT), "%s: DrawDC restored the DC's font", label);
	p.Close();
	b.Close();
}

void TestFonts(std::vector<HFONT> &fonts)
{
	for (const Case &c : kCases) {
		int charset = kCharset[c.type];
		HFONT font = MakeFont(c.type, c.height, charset, c.bold);
		Check(font != NULL, "CreateFont(%d, ..., %s, charset %d, NONANTIALIASED_QUALITY, %s)", c.height,
		      c.bold ? "FW_BOLD" : "FW_NORMAL", charset, kFaceNT[c.type]);
		if (!font) continue;
		fonts.push_back(font);
		TestFont(c, font, charset);
	}
	// An explicit Arial (type 1) keeps the Korean service's charset in the
	// client only for types >= 20; the Rix .eot faces come through
	// CreateFontIndirect (dead code) and fall back by charset.
	LOGFONTA lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = -13;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_RASTER_PRECIS;
	lf.lfQuality = PROOF_QUALITY;
	strcpy(lf.lfFaceName, "RixSquirrel_10.eot");
	HFONT rix = CreateFontIndirectA(&lf);
	Check(rix != NULL, "CreateFontIndirect(RixSquirrel_10.eot, -13): an unknown face falls back");
	HFONT none = CreateFontA(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, HANGUL_CHARSET, 0, 0, NONANTIALIASED_QUALITY, 0, NULL);
	Check(none != NULL, "CreateFont with a NULL face (GetFontFace found nothing): falls back by charset");
	for (HFONT f : { rix, none }) {
		if (!f) continue;
		fonts.push_back(f);
		WindowBitmap b;
		b.Open(200, 30);
		b.Clear(kKey);
		{
			DrawDC d(b.dc);
			SetTextColor(b.dc, RGB(255, 255, 255));
			d.Out(f, 949, 2, 2, Cp(949, u8"다람쥐 Squirrel"));
		}
		Ink ink = Scan(b, kKey, 0x00FFFFFF);
		Check(ink.count > 0 && ink.stray == 0, "fallback font draws (%d pixels, %d others)", ink.count, ink.stray);
		b.Close();
	}
}

bool SameImage(WindowBitmap &a, WindowBitmap &b)
{
	for (int y = 0; y < a.h; ++y)
		for (int x = 0; x < a.w; ++x)
			if (a.At(x, y) != b.At(x, y)) return false;
	return true;
}

void TestAandW()
{
	struct { int type, height; const char *utf8; } cases[] = {
		{ 0, 12, u8"한글 텍스트 CP949 똠방각하 뷁" }, { 1, 15, u8"Café crème, naïve — 5 € ‰ ©" },
	};
	for (auto &c : cases) {
		int charset = kCharset[c.type], cp = bf_text_codepage_for_charset(charset);
		HFONT font = MakeFont(c.type, c.height, charset, false);
		std::string bytes = Cp(cp, c.utf8);
		WindowBitmap w, a;
		w.Open(320, 30); w.Clear(kKey);
		a.Open(320, 30); a.Clear(kKey);
		SIZE sw, sa = { -1, -1 };
		{
			DrawDC d(w.dc);
			SetTextColor(w.dc, RGB(0, 0, 0));
			d.Out(font, cp, 3, 3, bytes);
			sw = d.Size(font, cp, bytes);
		}
		HGDIOBJ old = SelectObject(a.dc, font);
		SetBkMode(a.dc, TRANSPARENT);
		SetTextColor(a.dc, RGB(0, 0, 0));
		BOOL ok = TextOutA(a.dc, 3, 3, bytes.data(), (int)bytes.size());
		GetTextExtentPoint32A(a.dc, bytes.data(), (int)bytes.size(), &sa);
		SelectObject(a.dc, old);
		Ink ink = Scan(w, kKey, 0);
		Check(ok && ink.count > 0 && ink.stray == 0 && SameImage(w, a) && sw.cx == sa.cx && sw.cy == sa.cy,
		      "CP%d: TextOutA (font charset %d) = MultiByteToWideChar + TextOutW; extents %ldx%ld", cp, charset,
		      (long)sa.cx, (long)sa.cy);
		w.Close();
		a.Close();
		DeleteObject(font);
	}
}

void TestBottomUp()
{
	HFONT font = MakeFont(1, 15, ANSI_CHARSET, false);
	WindowBitmap td, bu;
	td.Open(200, 24); td.Clear(kKey);
	bu.Open(200, 24, true); bu.Clear(kKey);
	for (WindowBitmap *b : { &td, &bu }) {
		DrawDC d(b->dc);
		SetTextColor(b->dc, RGB(255, 0, 0));
		d.Out(font, 1252, 2, 2, "Bottom-up DIB gjpqy");
		const POINT tri[3] = { { 170, 4 }, { 196, 4 }, { 183, 20 } };
		HRGN rgn = CreatePolygonRgn(tri, 3, WINDING);
		HBRUSH brush = CreateSolidBrush(RGB(0, 0, 255));
		FillRgn(b->dc, rgn, brush);
		DeleteObject(rgn);
		DeleteObject(brush);
	}
	bool first_row_is_bottom = true;
	for (int x = 0; x < 200; ++x)
		if (bu.image[x] != td.image[23 * 200 + x]) first_row_is_bottom = false;
	Ink ink = Scan(bu, kKey, 0x00FF0000);
	Check(SameImage(td, bu) && first_row_is_bottom && ink.count > 0,
	      "biHeight > 0: the same picture, its first row in memory the bottom row");
	td.Close();
	bu.Close();
	DeleteObject(font);
}

void TestTriangle()
{
	// UIWindow::DrawTriangleDC.
	WindowBitmap b;
	b.Open(64, 64);
	b.Clear(kKey);
	POINT point[3] = { { 8, 8 }, { 56, 8 }, { 32, 50 } };
	const COLORREF color = RGB(0x10, 0x80, 0xF0);
	HDC dc = b.dc;
	HRGN rgn = CreatePolygonRgn(point, 3, WINDING);
	HBRUSH brush = CreateSolidBrush(color);
	BOOL ok = FillRgn(dc, rgn, brush);
	DeleteObject(rgn);
	DeleteObject(brush);
	Ink ink = Scan(b, kKey, Pixel(color));
	int area = 48 * 42 / 2;
	Check(ok && ink.stray == 0 && ink.count > area * 9 / 10 && ink.count < area * 11 / 10,
	      "FillRgn(triangle, WINDING): %d pixels of 0x%08X (area %d), no others", ink.count, Pixel(color), area);
	Check(b.At(32, 20) == Pixel(color) && b.At(4, 4) == kKey && b.At(10, 40) == kKey && b.At(60, 60) == kKey
	      && ink.left >= 8 && ink.right < 56 && ink.top >= 8 && ink.bottom < 50,
	      "the triangle's inside is filled, its outside untouched");
	std::vector<uint32_t> ref(64 * 64, kKey);
	bf_text_bitmap bm = { ref.data(), 64, 64, 256, 0 };
	int xy[6] = { 8, 8, 56, 8, 32, 50 };
	bf_text_fill_polygon(&bm, xy, 3, Pixel(color), 1);
	Check(!memcmp(ref.data(), b.image, ref.size() * 4), "the same pixels as bf_text_fill_polygon");
	b.Close();
}

void TestSurfaceDc()
{
	// What the d3d adapter does for IDirectDrawSurface7::GetDC (CDCSurface):
	// a DC over its own 32-bit staging bitmap.
	const int w = 96, h = 32, pitch = w * 4 + 16;
	std::vector<uint32_t> staging(pitch / 4 * h, 0xFF000000);
	HDC hdc = bf_compat_gdi_dc_for_bitmap(staging.data(), w, h, pitch, 0);
	Check(hdc != NULL && !bf_compat_gdi_dc_for_bitmap(staging.data(), w, h, w * 2, 0),
	      "bf_compat_gdi_dc_for_bitmap: a DC over 96x32 (pitch %d); a pitch < width * 4 is refused", pitch);
	RECT r;
	Check(!bf_compat_gdi_dc_take_dirty(hdc, &r) && GetCurrentObject(hdc, OBJ_FONT) == GetStockObject(SYSTEM_FONT),
	      "fresh surface DC: nothing dirty, the system font selected");
	HBITMAP bmp;
	{
		BITMAPINFO info;
		memset(&info, 0, sizeof(info));
		info.bmiHeader = { sizeof(BITMAPINFOHEADER), 4, -4, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
		void *bits;
		bmp = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0);
	}
	Check(bmp && !SelectObject(hdc, bmp), "selecting a bitmap into a surface DC fails");
	DeleteObject(bmp);
	HFONT font = MakeFont(1, 15, ANSI_CHARSET, false);
	{
		// CRenderer::TextOutScreen: the FPS counter.
		DrawDC d(hdc);
		SetTextColor(hdc, RGB(255, 255, 0));
		d.Out(font, 1252, 5, 4, "FPS 60");
	}
	Check(bf_compat_gdi_dc_take_dirty(hdc, &r) && r.left >= 5 && r.top >= 4 && r.right <= w && r.bottom <= h
	      && r.right > r.left && r.bottom > r.top, "dirty rect (%ld,%ld)-(%ld,%ld) after TextOutW",
	      (long)r.left, (long)r.top, (long)r.right, (long)r.bottom);
	int changed = 0, stray = 0, outside = 0;
	for (int y = 0; y < h; ++y)
		for (int x = 0; x < w; ++x) {
			uint32_t p = staging[y * pitch / 4 + x];
			if (p == 0xFF000000) continue;
			++changed;
			if (p != 0x00FFFF00) ++stray;
			if (x < r.left || x >= r.right || y < r.top || y >= r.bottom) ++outside;
		}
	bool padding = true;
	for (int y = 0; y < h; ++y)
		for (int x = w; x < pitch / 4; ++x) padding = padding && staging[y * pitch / 4 + x] == 0xFF000000;
	Check(changed > 0 && stray == 0 && outside == 0 && padding,
	      "%d pixels written, all 0x00FFFF00, all inside the dirty rect, none in the row padding", changed);
	Check(!bf_compat_gdi_dc_take_dirty(hdc, &r), "take_dirty resets");
	// A fresh surface DC is OPAQUE like GDI's: the cell is filled with the
	// background colour (white) where the client did not set TRANSPARENT.
	HDC fresh = bf_compat_gdi_dc_for_bitmap(staging.data(), w, h, pitch, 0);
	SelectObject(fresh, font);
	WCHAR text[] = { 'x', 'y', 0 };
	TextOutW(fresh, 40, 10, text, 2);
	SIZE s;
	GetTextExtentPoint32W(fresh, text, 2, &s);
	Check(staging[10 * pitch / 4 + 40] == 0x00FFFFFF && staging[(10 + s.cy - 1) * pitch / 4 + 40 + s.cx - 1] == 0x00FFFFFF
	      && staging[(10 + s.cy) * pitch / 4 + 40] == 0xFF000000, "OPAQUE default: the cell %ldx%ld filled white",
	      (long)s.cx, (long)s.cy);
	SelectObject(fresh, GetStockObject(SYSTEM_FONT));
	Check(DeleteDC(fresh) && DeleteDC(hdc), "DeleteDC of surface DCs");
	DeleteObject(font);
}

// ---- The sample: a UI window -------------------------------------------------

bool WriteSample(std::vector<HFONT> &fonts)
{
	(void)fonts;
	WindowBitmap b;
	b.Open(300, 196);
	b.Clear(kKey);   // CDCBitmap::ClearSurface with the colour key
	// The window frame, as the client blits its sprites: opaque pixels.
	for (int y = 0; y < b.h; ++y)
		for (int x = 0; x < b.w; ++x) {
			bool corner = (x < 3 || x >= b.w - 3) && (y < 3 || y >= b.h - 3);
			if (corner) continue;
			uint32_t c = y < 17 ? 0xFF5A6E9C : (x < 2 || x >= b.w - 2 || y >= b.h - 2) ? 0xFF8C96B4 : 0xFFF4F4F8;
			b.At(x, y) = c;
		}
	HFONT title = MakeFont(1, 15, ANSI_CHARSET, true), arial15 = MakeFont(1, 15, ANSI_CHARSET, false);
	HFONT arial14 = MakeFont(1, 14, ANSI_CHARSET, false), gulim12 = MakeFont(0, 12, HANGUL_CHARSET, false);
	HFONT gulim13b = MakeFont(0, 13, HANGUL_CHARSET, true), arial18 = MakeFont(1, 18, ANSI_CHARSET, false);
	{
		DrawDC d(b.dc);
		// Title with shadow: (x+1, y+1) then (x, y).
		SetTextColor(b.dc, RGB(0, 0, 0));
		d.Out(title, 1252, 8, 2, "Status Window");
		SetTextColor(b.dc, RGB(255, 255, 255));
		d.Out(title, 1252, 7, 1, "Status Window");
		// Labels and right-aligned numbers (TextOutWithRightAlign: x - cx).
		const char *labels[3] = { "Str", "Agi", "Vit" };
		const char *values[3] = { "99 + 12", "80 + 5", "1,234,567" };
		for (int i = 0; i < 3; ++i) {
			SetTextColor(b.dc, RGB(0, 0, 0));
			d.Out(arial15, 1252, 10, 22 + i * 19, labels[i]);
			SIZE s = d.Size(arial14, 1252, values[i]);
			SetTextColor(b.dc, RGB(0x10, 0x30, 0x90));
			d.Out(arial14, 1252, 140 - s.cx, 23 + i * 19, values[i]);
		}
		// Korean, Gulim 12, in CP949 bytes.
		SetTextColor(b.dc, RGB(0, 0, 0));
		d.Out(gulim12, 949, 150, 22, Cp(949, u8"캐릭터 정보"));
		d.Out(gulim12, 949, 150, 38, Cp(949, u8"초보자 Lv 99"));
		// Name balloon text with outline: four neighbours, then the centre.
		const std::string name = Cp(949, u8"하이프리스트");
		SetTextColor(b.dc, RGB(0, 0, 0));
		const int ox = 150, oy = 58;
		for (const POINT &o : { POINT{ -1, 0 }, POINT{ 1, 0 }, POINT{ 0, -1 }, POINT{ 0, 1 } })
			d.Out(gulim13b, 949, ox + o.x, oy + o.y, name);
		SetTextColor(b.dc, RGB(255, 255, 0));
		d.Out(gulim13b, 949, ox, oy, name);
		// Colour codes (TextOutWithDecoration): segments at the extents of what precedes.
		struct { COLORREF c; const char *t; } seg[3] = {
			{ RGB(0xC0, 0, 0), "Red text, " }, { RGB(0, 0x80, 0), "green text, " }, { RGB(0, 0, 0xC0), "blue." } };
		int x = 10;
		for (auto &s : seg) {
			SetTextColor(b.dc, s.c);
			d.Out(arial15, 1252, x, 84, s.t);
			x += d.Size(arial15, 1252, s.t).cx;
		}
		// Western text with accents and the euro sign, CP1252.
		SetTextColor(b.dc, RGB(0x30, 0x30, 0x30));
		d.Out(arial15, 1252, 10, 104, Cp(1252, u8"Café Prontera — 5 € · naïve"));
		d.Out(arial18, 1252, 10, 124, "Not Available");
		// Two colours: (x, y) then (x-1, y).
		SetTextColor(b.dc, RGB(0x80, 0x40, 0));
		d.Out(gulim12, 949, 11, 150, Cp(949, u8"두 가지 색 Two colours"));
		SetTextColor(b.dc, RGB(0xFF, 0x90, 0x20));
		d.Out(gulim12, 949, 10, 150, Cp(949, u8"두 가지 색 Two colours"));
		SetTextColor(b.dc, RGB(0, 0, 0));
		d.Out(arial14, 1252, 10, 170, "Zeny: 1,234,567");
	}
	// A drop-down arrow (DrawTriangleDC).
	const POINT arrow[3] = { { 270, 170 }, { 288, 170 }, { 279, 182 } };
	HRGN rgn = CreatePolygonRgn(arrow, 3, WINDING);
	HBRUSH brush = CreateSolidBrush(RGB(0x40, 0x40, 0x60));
	FillRgn(b.dc, rgn, brush);
	DeleteObject(rgn);
	DeleteObject(brush);
	for (HFONT f : { title, arial15, arial14, gulim12, gulim13b, arial18 }) DeleteObject(f);

	// CDCBitmap::UpdateSurface: every alpha byte to 0xFF; the upload keys magenta.
	std::vector<uint32_t> out(b.w * b.h);
	for (int i = 0; i < b.w * b.h; ++i) {
		uint32_t p = b.image[i] | 0xFF000000;
		out[i] = p == kKey ? 0 : p;
	}
	b.Close();
	const char *dir = getenv("BF_TEST_OUT");
	std::string path = std::string(dir && *dir ? dir : BF_GDI_OUTPUT_DIR) + "/gdi-sample.png";
	bool ok = bf_gdi_test_write_png(path.c_str(), out.data(), 300, 196, 300 * 4) == 0;
	Check(ok, "wrote %s", path.c_str());
	if (ok) {
		std::string ref = std::string(BF_GDI_SOURCE_DIR) + "/test/reference/gdi-sample.png";
		auto slurp = [](const std::string &p) {
			std::string s;
			if (FILE *f = fopen(p.c_str(), "rb")) {
				char buf[65536];
				size_t n;
				while ((n = fread(buf, 1, sizeof(buf), f)) > 0) s.append(buf, n);
				fclose(f);
			}
			return s;
		};
		std::string a = slurp(path), r = slurp(ref);
		if (r.empty()) printf("note no reference picture at %s\n", ref.c_str());
		else if (a != r) printf("note the sample differs from %s (CoreText's rasteriser may differ between macOS versions)\n", ref.c_str());
		else printf("note the sample equals the reference picture\n");
	}
	return ok;
}

} // namespace

int main()
{
	TestHandles();
	TestBitmaps();
	TestEnumeration();
	std::vector<HFONT> fonts;
	TestFonts(fonts);
	TestAandW();
	TestBottomUp();
	TestTriangle();
	TestSurfaceDc();
	WriteSample(fonts);
	// FontMgr::~FontMgr deletes the cached fonts.
	for (HFONT f : fonts) DeleteObject(f);
	Check(bf_compat_gdi_live_objects() == 0, "every object freed at the end (%d left)", bf_compat_gdi_live_objects());
	printf("%d checks, %d failed\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}
