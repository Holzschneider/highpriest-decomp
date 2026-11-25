// bf_compat_gdi: what is left of the gdi32 adapter after M11.8. The client
// draws text, its window bitmaps and the triangle through port/native/text
// (src/3dlib/DrawUtil.cpp, DC.cpp, window/UISys.cpp); the d3d adapter's
// surface DC is a bf_text_canvas. Three names remain, for callers outside
// M11.8's files: GetStockObject (WinMain.cpp's WNDCLASS, BLACK_BRUSH) and
// the two port-side functions of the input adapter (port/compat/input:
// RegisterClassA reads the class brush's colour, GetDC returns the screen
// DC). They go with M11.7, which removes those callers; then this directory
// is deleted.

#include <windows.h>

#include "bf_compat_gdi.h"
#include "bf_trace.h"

namespace {

// WHITE_BRUSH .. NULL_BRUSH: their colours; NULL_BRUSH draws nothing.
struct StockBrush { COLORREF color; bool hollow; };
const StockBrush kBrushes[6] = {
	{ RGB(255, 255, 255), false }, { RGB(192, 192, 192), false }, { RGB(128, 128, 128), false },
	{ RGB(64, 64, 64), false }, { RGB(0, 0, 0), false }, { 0, true },
};

char g_screen;   // the screen DC: a handle nothing draws on

} // namespace

extern "C" {

HGDIOBJ WINAPI GetStockObject(INT index)
{
	if (index >= WHITE_BRUSH && index <= NULL_BRUSH)
		return reinterpret_cast<HGDIOBJ>(const_cast<StockBrush *>(&kBrushes[index]));
	BF_TRACE(BFT_W32, "gdi: GetStockObject(%d): only the stock brushes are left (M11.8)", (int)index);
	return nullptr;
}

BOOL bf_compat_gdi_brush_color(HBRUSH brush, COLORREF *out)
{
	const StockBrush *b = reinterpret_cast<const StockBrush *>(brush);
	if (b < kBrushes || b >= kBrushes + 6 || b->hollow) return FALSE;
	if (out) *out = b->color;
	return TRUE;
}

HDC bf_compat_gdi_screen_dc(void)
{
	return reinterpret_cast<HDC>(&g_screen);
}

} // extern "C"
