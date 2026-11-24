// Shared between the input adapter's units. Not a contract.
#ifndef BF_COMPAT_INPUT_INTERNAL_H
#define BF_COMPAT_INPUT_INTERNAL_H

#include <windows.h>
#include <string>

#include "bf_app.h"
#include "bf_view.h"
#include "bf_compat_input.h"

// From port/compat/gdi (bf_compat_gdi.h), declared here so that this area
// builds in a tree without it: GetDC(NULL) is the screen DC, and the
// window-class brush's colour.
extern "C" HDC bf_compat_gdi_screen_dc(void);
extern "C" BOOL bf_compat_gdi_brush_color(HBRUSH brush, COLORREF *out);

namespace bfin {

// The key table (bf_compat_input_keys.cpp).
struct KeyInfo {
	unsigned vk;        // VK_* as WM_KEYDOWN's wParam (VK_SHIFT for both shifts, ...)
	unsigned state_vk;  // the entry GetKeyState keeps (VK_LSHIFT, VK_RSHIFT, ...): vk otherwise
	unsigned scan;      // set-1 scan code
	int extended;       // the E0 prefix: lParam bit 24
};
const KeyInfo &KeyFor(int key);
unsigned ScanForVk(unsigned vk, int *extended);
unsigned VkForScan(unsigned scan);

// UTF-8 to CP1252 bytes, characters it does not hold dropped; and back.
std::string Utf8ToCp1252(const char *utf8);
std::string Cp1252ToUtf8(const char *bytes, size_t len);

// Started on first use: bf_app_init with the configured slot. Exits the
// process when another instance has been brought to the front.
void EnsureApp();

// The window object behind an HWND.
struct Window {
	unsigned magic;
	WNDPROC proc;
	LONG style, ex_style;
	LONG_PTR user_data;
	std::string class_name;
	COLORREF background;   // the class brush's colour; the picture is gfx's, nothing paints it
};
Window *WindowFrom(HWND hwnd);   // NULL for anything else
HWND MainWindow();

// The view metrics in effect: bf_view_get once the view runs, else computed
// from the window (or 640 x 480 at scale 1 without one).
void Metrics(bf_view_metrics *out);

// The pointer's logical position, not clamped (GetCursorPos).
void PointerLogical(int *x, int *y);

}  // namespace bfin

#endif
