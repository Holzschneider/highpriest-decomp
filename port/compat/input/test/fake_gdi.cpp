// Stand-ins for the two port/compat/gdi functions the input adapter calls,
// for a tree without that area (the root build links the real ones).
#include <windows.h>

static char g_screen_dc_object;

extern "C" HDC bf_compat_gdi_screen_dc(void)
{
	return (HDC)&g_screen_dc_object;
}

extern "C" BOOL bf_compat_gdi_brush_color(HBRUSH brush, COLORREF *out)
{
	if (!brush || !out)
		return FALSE;
	*out = 0;   // the client registers BLACK_BRUSH
	return TRUE;
}
