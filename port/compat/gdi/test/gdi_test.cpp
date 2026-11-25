// bifrost-compat-gdi-test: the three names left in port/compat/gdi after
// M11.8 (GetStockObject's brushes and the input adapter's two functions).
// Text, DIB sections and polygon fills are tested in bifrost-text-test.

#include <windows.h>

#include <stdio.h>

#include "bf_compat_gdi.h"

static int g_failures = 0;

static void Check(bool ok, const char *what)
{
	printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
	if (!ok) ++g_failures;
}

int main()
{
	COLORREF c = 1;
	HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
	Check(black && bf_compat_gdi_brush_color(black, &c) && c == RGB(0, 0, 0), "BLACK_BRUSH is black (WinMain's class brush)");
	Check(bf_compat_gdi_brush_color((HBRUSH)GetStockObject(WHITE_BRUSH), &c) && c == RGB(255, 255, 255), "WHITE_BRUSH is white");
	Check(!bf_compat_gdi_brush_color((HBRUSH)GetStockObject(NULL_BRUSH), &c), "NULL_BRUSH has no colour");
	Check(!bf_compat_gdi_brush_color((HBRUSH)&c, &c) && !bf_compat_gdi_brush_color(NULL, &c), "a stray handle is no brush");
	Check(GetStockObject(DEFAULT_GUI_FONT) == NULL, "stock fonts are gone");
	Check(bf_compat_gdi_screen_dc() != NULL && bf_compat_gdi_screen_dc() == bf_compat_gdi_screen_dc(), "one screen DC");
	printf("%s: %d failure%s\n", g_failures ? "FAILED" : "passed", g_failures, g_failures == 1 ? "" : "s");
	return g_failures ? 1 : 0;
}
