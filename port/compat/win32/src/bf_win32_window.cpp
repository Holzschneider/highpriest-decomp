// The user32 names that client files outside M11.7's package still call,
// left behind when the input adapter (port/compat/input) was deleted in
// M11.7. Each goes with its last caller:
//   GetClientRect, ClientToScreen, ShowWindow, SetWindowLongA, DrawMenuBar,
//   RedrawWindow   3dlib/Device.cpp (M11.9, graphics)
//   GetKeyState, CharNextExA, CharPrevExA   UIControl.cpp (M11.8, text)
//   GetDC          3dlib/DrawUtil.cpp, for EnumFontFamiliesEx (M11.8)
//   wsprintfA      window/UIFrameWnd.cpp, GameMode2.cpp, LoginMode.cpp (M11.8)
// They behave as the adapter's did, on the port's window (bf_input.h):
// "screen" coordinates are the client area's logical ones, the window's
// state is macOS's.
#include "bf_win32_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "bf_input.h"
#include "bf_text.h"
#include "bf_trace.h"

// The gdi area's screen DC (bf_compat_gdi.h). A weak stand-in, so that this
// links without that area (the test, and the tree once it is gone with
// GetDC's caller); the gdi area's definition takes its place in the client.
extern "C" __attribute__((weak)) HDC bf_compat_gdi_screen_dc(void)
{
	return nullptr;
}

namespace {

LONG g_style = 0x02CA0000;   // what InitApp created the window with

bool IsWindowHandle(HWND hwnd)
{
	return hwnd && hwnd == bf_input_window();
}

int CodePage(WORD cp)
{
	// CP_ACP, CP_OEMCP, CP_MACCP and CP_THREAD_ACP: the client's Windows-1252.
	return cp <= 3 ? BF_TEXT_CP_WESTERN : cp;
}

// wsprintf's format in C99 terms: its `l` is 32 bits (Windows' long), `h`
// before s/c means narrow, which C's printf has as plain; the rest is the same.
std::string Format(const char *fmt)
{
	std::string out;
	for (const char *p = fmt; *p; p++) {
		out += *p;
		if (*p != '%')
			continue;
		if (p[1] == '%') {
			out += *++p;
			continue;
		}
		p++;
		while (*p && strchr("-+ #0", *p))
			out += *p++;
		while (*p && ((*p >= '0' && *p <= '9') || *p == '.' || *p == '*'))
			out += *p++;
		if (*p == 'l' && p[1] != 'l')
			p++;                            // 32-bit: the argument is an int
		else if (*p == 'h' && (p[1] == 's' || p[1] == 'c'))
			p++;
		if (!*p)
			break;
		out += *p;
	}
	return out;
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

SHORT WINAPI GetKeyState(INT vk)
{
	return (SHORT)bf_input_key_state(vk);
}

HDC WINAPI GetDC(HWND)
{
	// The client calls GetDC(NULL) only, for EnumFontFamiliesEx; the window
	// has no DC of its own (its picture is the gfx device's).
	return bf_compat_gdi_screen_dc();
}

LPSTR WINAPI CharNextExA(WORD codepage, LPCSTR text, DWORD)
{
	if (!text || !*text)
		return (LPSTR)text;
	return (LPSTR)(text + bf_text_char_length(CodePage(codepage), text, -1));
}

LPSTR WINAPI CharPrevExA(WORD codepage, LPCSTR start, LPCSTR current, DWORD)
{
	if (!start || !current || current <= start)
		return (LPSTR)start;
	return (LPSTR)(start + bf_text_char_prev(CodePage(codepage), start, (int)(current - start)));
}

INT WINAPI wvsprintfA(LPSTR buffer, LPCSTR format, __ms_va_list args)
{
	if (!buffer || !format)
		return 0;
	// wsprintf writes at most 1024 characters and the NUL.
	int n = vsnprintf(buffer, 1025, Format(format).c_str(), args);
	if (n < 0) {
		buffer[0] = 0;
		return 0;
	}
	return n > 1024 ? 1024 : n;
}

INT WINAPIV wsprintfA(LPSTR buffer, LPCSTR format, ...)
{
	va_list args;
	va_start(args, format);
	INT n = wvsprintfA(buffer, format, args);
	va_end(args);
	return n;
}

}  // extern "C"
