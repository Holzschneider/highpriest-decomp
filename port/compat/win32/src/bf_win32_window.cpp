// The user32 names that 3dlib/Device.cpp (M11.9, graphics) still calls,
// left behind when the input adapter (port/compat/input) was deleted in
// M11.7: GetClientRect, ClientToScreen, ShowWindow, SetWindowLongA,
// DrawMenuBar, RedrawWindow. They go with that caller. They behave as the
// adapter's did, on the port's window (bf_input.h): "screen" coordinates are
// the client area's logical ones, the window's state is macOS's.
#include "bf_win32_internal.h"

#include "bf_input.h"
#include "bf_trace.h"

namespace {

LONG g_style = 0x02CA0000;   // what InitApp created the window with

bool IsWindowHandle(HWND hwnd)
{
	return hwnd && hwnd == bf_input_window();
}

}  // namespace

extern "C" {

BOOL WINAPI GetClientRect(HWND hwnd, LPRECT rect)
{
	if (!rect || !IsWindowHandle(hwnd))
		return FALSE;
	int w, h;
	bf_input_client_size(&w, &h);
	rect->left = rect->top = 0;
	rect->right = w;
	rect->bottom = h;
	return TRUE;
}

BOOL WINAPI ClientToScreen(HWND hwnd, LPPOINT point)
{
	return point && IsWindowHandle(hwnd);   // the identity
}

BOOL WINAPI ShowWindow(HWND hwnd, INT cmd)
{
	if (!IsWindowHandle(hwnd))
		return FALSE;
	// The fullscreen path's SW_MAXIMIZE: the window's state is the user's on macOS.
	BF_TRACE(BFT_INPUT, "input: ShowWindow(%d) ignored\n", cmd);
	return TRUE;
}

LONG WINAPI SetWindowLongA(HWND hwnd, INT index, LONG value)
{
	if (!IsWindowHandle(hwnd))
		return 0;
	if (index != GWL_STYLE) {
		BF_STUB();
		return 0;
	}
	LONG old = g_style;
	g_style = value;   // WS_POPUP for fullscreen: nothing changes on macOS
	return old;
}

BOOL WINAPI DrawMenuBar(HWND)
{
	return TRUE;
}

BOOL WINAPI RedrawWindow(HWND, const RECT *, HRGN, UINT)
{
	return TRUE;
}

}  // extern "C"
