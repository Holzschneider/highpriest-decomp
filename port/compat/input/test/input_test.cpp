// bifrost-compat-input-test: the input adapter through the Win32 names, called
// as WinMain.cpp, CMouse and CLanguage call them. It opens a window (its own
// single-instance slot) and injects every event through bf_app_post_event, as
// the control socket does, so it runs on the locked screen of the unattended
// Mac. One line per check; exit status 1 on any failure.
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#include <imm.h>

#include <math.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "bf_app.h"
#include "bf_compat_input.h"
#include "bf_view.h"

extern char **environ;
extern int g_fake_global_live;
HINSTANCE GetModuleHandleA_stub();

namespace {

int g_failures = 0;

void Check(bool ok, const char *what, const std::string &detail = "")
{
	printf("%-4s %s%s%s\n", ok ? "ok" : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
	if (!ok)
		g_failures++;
}

void Skip(const char *what, const char *why)
{
	printf("skip %s: %s\n", what, why);
}

std::string Fmt(const char *f, ...)
{
	char b[512];
	va_list a;
	va_start(a, f);
	vsnprintf(b, sizeof b, f, a);
	va_end(a);
	return b;
}

// ------------------------------------------------------------ the window procedure

struct Rec {
	UINT msg;
	WPARAM w;
	LPARAM l;
	bool ctrl, shift, alt;   // GetKeyState high bits while it was handled
	bool async;              // GetAsyncKeyState(wParam) for key messages
};
std::vector<Rec> g_recs;
HWND g_hwnd = nullptr;
bool g_created = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	Rec r = {message, wparam, lparam, false, false, false, false};
	r.ctrl = HIBYTE(GetKeyState(VK_CONTROL)) != 0;
	r.shift = HIBYTE(GetKeyState(VK_SHIFT)) != 0;
	r.alt = HIBYTE(GetAsyncKeyState(VK_MENU)) != 0;
	if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)
		r.async = HIBYTE(GetAsyncKeyState((int)wparam)) != 0;
	if (message != WM_SETCURSOR && message != WM_MOUSEMOVE)
		g_recs.push_back(r);
	switch (message) {
	case WM_CREATE:
		g_created = true;
		return 0;
	case WM_SETCURSOR:
		SetCursor(NULL);   // as the client does outside fullscreen
		return 0;
	case WM_MOUSEMOVE:
		g_recs.push_back(r);
		return 0;
	case WM_CLOSE:
		return 0;          // the client sets g_sysQuit and returns
	case WM_CHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
		return 0;
	}
	return DefWindowProcA(hwnd, message, wparam, lparam);
}

// CheckSystemMessage of WinMain.cpp.
void Pump()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// ------------------------------------------------------------ injection

uint64_t Key(int type, int key, int repeat = 0)
{
	bf_app_event e = {};
	e.type = type;
	e.key.key = (uint16_t)key;
	e.key.mac_keycode = 0;
	e.key.repeat = (uint8_t)repeat;
	return bf_app_post_event_seq(&e);
}
uint64_t Down(int key, int repeat = 0) { return Key(BF_APP_EVENT_KEY_DOWN, key, repeat); }
uint64_t Up(int key) { return Key(BF_APP_EVENT_KEY_UP, key); }

void Text(const char *utf8, uint64_t key_seq)
{
	bf_app_event e = {};
	e.type = BF_APP_EVENT_TEXT;
	strncpy(e.text.utf8, utf8, sizeof e.text.utf8 - 1);
	e.text.key_seq = key_seq;
	bf_app_post_event(&e);
}

void Pointer(int type, float x_px, float y_px, uint32_t button = 0, float wheel = 0)
{
	bf_app_event e = {};
	e.type = type;
	e.pointer.x_px = x_px;
	e.pointer.y_px = y_px;
	e.pointer.button = button;
	e.pointer.wheel_y = wheel;
	bf_app_post_event(&e);
}

void Value(int type, int value)
{
	bf_app_event e = {};
	e.type = type;
	e.value = value;
	bf_app_post_event(&e);
}

// Injects, pumps, and returns what the window procedure saw.
std::vector<Rec> Run()
{
	g_recs.clear();
	Pump();
	return g_recs;
}

std::string Describe(const std::vector<Rec> &v)
{
	std::string s;
	for (const Rec &r : v)
		s += Fmt("[%04x w=%llx l=%08llx%s] ", r.msg, (unsigned long long)r.w, (unsigned long long)(uint32_t)r.l,
		         r.ctrl ? " ctrl" : "");
	return s.empty() ? "(none)" : s;
}

const Rec *Find(const std::vector<Rec> &v, UINT msg, size_t nth = 0)
{
	for (const Rec &r : v)
		if (r.msg == msg && nth-- == 0)
			return &r;
	return nullptr;
}

int Count(const std::vector<Rec> &v, UINT msg)
{
	int n = 0;
	for (const Rec &r : v)
		n += r.msg == msg;
	return n;
}

std::string Chars(const std::vector<Rec> &v)
{
	std::string s;
	for (const Rec &r : v)
		if (r.msg == WM_CHAR)
			s += (char)r.w;
	return s;
}

bool Is(const Rec *r, WPARAM w, uint32_t l)
{
	return r && r->w == w && (uint32_t)r->l == l;
}

// ------------------------------------------------------------ the checks

void Keyboard()
{
	std::vector<Rec> v;

	// A, D, W, X: the lParam values UIWindowMgr::ExcuteMsgInBattleMode compares.
	uint64_t s = Down(BF_KEY_A);
	Text("a", s);
	v = Run();
	Check(Is(Find(v, WM_KEYDOWN), 'A', 0x1e0001) && Find(v, WM_KEYDOWN)->async, "key A: WM_KEYDOWN 'A' lParam 0x1e0001, GetAsyncKeyState down", Describe(v));
	Check(Chars(v) == "a" && Find(v, WM_CHAR) > Find(v, WM_KEYDOWN), "key A: WM_CHAR 'a' after it", Describe(v));
	Down(BF_KEY_A, 1);
	v = Run();
	Check(Is(Find(v, WM_KEYDOWN), 'A', 0x401e0001), "key A repeat: bit 30 set (ProcessPushButton's newKeydown 0)", Describe(v));
	Up(BF_KEY_A);
	v = Run();
	Check(Is(Find(v, WM_KEYUP), 'A', 0xc01e0001) && !Find(v, WM_KEYUP)->async && !HIBYTE(GetAsyncKeyState('A')),
	      "key A up: WM_KEYUP lParam 0xc01e0001, GetAsyncKeyState up", Describe(v));
	const int keys[3] = {BF_KEY_D, BF_KEY_W, BF_KEY_X};
	const uint32_t lps[3] = {0x200001, 0x110001, 0x2d0001};
	for (int i = 0; i < 3; i++) {
		Down(keys[i]);
		Up(keys[i]);
		v = Run();
		Check(Is(Find(v, WM_KEYDOWN), "DWX"[i], lps[i]), Fmt("key %c: lParam 0x%x", "DWX"[i], lps[i]).c_str(), Describe(v));
	}

	// Extended keys and function keys.
	Down(BF_KEY_LEFT);
	Up(BF_KEY_LEFT);
	Down(BF_KEY_F1);
	Up(BF_KEY_F1);
	Down(BF_KEY_F12);
	Up(BF_KEY_F12);
	v = Run();
	Check(Is(Find(v, WM_KEYDOWN, 0), VK_LEFT, 0x14b0001), "left arrow: VK_LEFT, scan 0x4b, extended", Describe(v));
	Check(Is(Find(v, WM_KEYDOWN, 1), VK_F1, 0x3b0001) && Is(Find(v, WM_KEYDOWN, 2), VK_F12, 0x580001), "F1, F12: VK_F1 scan 0x3b, VK_F12 scan 0x58", Describe(v));
	Down(BF_KEY_F10);
	Up(BF_KEY_F10);
	v = Run();
	Check(Is(Find(v, WM_SYSKEYDOWN), VK_F10, 0x440001) && Find(v, WM_SYSKEYUP) && !Find(v, WM_KEYDOWN),
	      "F10: WM_SYSKEYDOWN / WM_SYSKEYUP, as Windows sends it", Describe(v));
	Down(BF_KEY_F13);
	Up(BF_KEY_F13);
	v = Run();
	Check(Find(v, WM_KEYUP) && Find(v, WM_KEYUP)->w == VK_SNAPSHOT && !Find(v, WM_KEYDOWN),
	      "F13 (Print Screen of a PC keyboard): VK_SNAPSHOT, as WM_KEYUP only, as Windows sends it", Describe(v));

	// Shift reads as held while its keys are handled.
	Down(BF_KEY_LSHIFT);
	s = Down(BF_KEY_A);
	Text("A", s);
	Up(BF_KEY_A);
	Up(BF_KEY_LSHIFT);
	v = Run();
	const Rec *a = nullptr;
	for (const Rec &r : v)
		if (r.msg == WM_KEYDOWN && r.w == 'A')
			a = &r;
	Check(Is(Find(v, WM_KEYDOWN, 0), VK_SHIFT, 0x2a0001) && a && a->shift && Chars(v) == "A",
	      "Shift+A: WM_KEYDOWN VK_SHIFT scan 0x2a, GetKeyState(VK_SHIFT) down during 'A', WM_CHAR 'A'", Describe(v));
	Check(!HIBYTE(GetKeyState(VK_SHIFT)) && !HIBYTE(GetAsyncKeyState(VK_LSHIFT)), "Shift released: GetKeyState up");

	// Option is Alt: WM_SYSKEYDOWN, no character for a game shortcut.
	Down(BF_KEY_LALT);
	s = Down(BF_KEY_A);
	Text("\xc3\xa5", s);   // what Option+A types on a US layout
	Up(BF_KEY_A);
	v = Run();
	Check(Is(Find(v, WM_SYSKEYDOWN, 0), VK_MENU, 0x20380001) && Find(v, WM_SYSKEYDOWN, 0)->alt,
	      "Option: WM_SYSKEYDOWN VK_MENU, GetAsyncKeyState(VK_MENU) down", Describe(v));
	Check(Is(Find(v, WM_SYSKEYDOWN, 1), 'A', 0x201e0001), "Option+A: WM_SYSKEYDOWN 'A', bit 29 set", Describe(v));
	Check(Count(v, WM_CHAR) == 0, "Option+A (Alt+A opens a window): no WM_CHAR", Describe(v));
	s = Down(BF_KEY_L);
	Text("@", s);          // Option+L on a German layout
	Up(BF_KEY_L);
	Up(BF_KEY_LALT);
	v = Run();
	Check(Find(v, WM_SYSKEYDOWN) && Find(v, WM_SYSKEYDOWN)->w == 'L' && Chars(v) == "@",
	      "Option+L (no game shortcut): WM_SYSKEYDOWN 'L' and WM_CHAR '@'", Describe(v));
	Check(Find(v, WM_SYSKEYUP, 1) && Find(v, WM_SYSKEYUP, 1)->w == VK_MENU && !HIBYTE(GetAsyncKeyState(VK_MENU)),
	      "Option up: WM_SYSKEYUP VK_MENU, Alt reads up", Describe(v));

	// Ctrl: no text.
	Down(BF_KEY_LCTRL);
	s = Down(BF_KEY_A);
	Text("a", s);
	Up(BF_KEY_A);
	Up(BF_KEY_LCTRL);
	v = Run();
	Check(Find(v, WM_KEYDOWN, 1) && Find(v, WM_KEYDOWN, 1)->w == 'A' && Find(v, WM_KEYDOWN, 1)->ctrl && Count(v, WM_CHAR) == 0,
	      "Ctrl+A: WM_KEYDOWN 'A' with Ctrl held, no WM_CHAR", Describe(v));

	// What TranslateMessage typed: Backspace, Tab, Return, Escape.
	const int ctl[4] = {BF_KEY_BACKSPACE, BF_KEY_TAB, BF_KEY_RETURN, BF_KEY_ESCAPE};
	for (int k : ctl) {
		Down(k);
		Up(k);
	}
	v = Run();
	Check(Chars(v) == std::string("\b\t\r\x1b", 4) && Count(v, WM_KEYDOWN) == 4,
	      "Backspace, Tab, Return, Escape: WM_KEYDOWN then WM_CHAR 8, 9, 13, 27", Describe(v));
	Check(Find(v, WM_KEYDOWN) && Find(v, WM_CHAR) && Find(v, WM_KEYDOWN) < Find(v, WM_CHAR) && Find(v, WM_CHAR) < Find(v, WM_KEYUP),
	      "WM_CHAR 8 comes between its WM_KEYDOWN and WM_KEYUP", Describe(v));

	// Insert and its substitute.
	Down(BF_KEY_INSERT);   // kVK_Help: the Insert of a PC keyboard
	Up(BF_KEY_INSERT);
	v = Run();
	Check(Is(Find(v, WM_KEYDOWN), VK_INSERT, 0x1520001), "Insert (kVK_Help): VK_INSERT, scan 0x52, extended", Describe(v));
	Down(BF_KEY_LCMD);
	s = Down(BF_KEY_I);
	Text("i", s);          // AppKit types nothing with Cmd; injected text is dropped too
	v = Run();
	Check(Is(Find(v, WM_KEYDOWN), VK_INSERT, 0x1520001) && Find(v, WM_KEYDOWN)->async && Count(v, WM_CHAR) == 0 && v.size() == 1,
	      "Cmd+I (the Insert substitute): WM_KEYDOWN VK_INSERT only, GetAsyncKeyState(VK_INSERT) down", Describe(v));
	Up(BF_KEY_I);
	Down(BF_KEY_LALT);
	Down(BF_KEY_I);
	Up(BF_KEY_I);
	Up(BF_KEY_LALT);
	Up(BF_KEY_LCMD);
	v = Run();
	Check(Find(v, WM_KEYUP) && Find(v, WM_KEYUP)->w == VK_INSERT && Find(v, WM_SYSKEYDOWN, 1) && Find(v, WM_SYSKEYDOWN, 1)->w == VK_INSERT,
	      "Cmd+I up: WM_KEYUP VK_INSERT; Cmd+Option+I: WM_SYSKEYDOWN VK_INSERT (Alt+Insert)", Describe(v));
	Check(!HIBYTE(GetAsyncKeyState(VK_INSERT)) && !HIBYTE(GetAsyncKeyState(VK_LWIN)), "after Cmd+I: VK_INSERT and Cmd read up");

	// Cmd+Q: the quit path; the QUIT event (menu, close button) likewise.
	Down(BF_KEY_LCMD);
	Down(BF_KEY_Q);
	Up(BF_KEY_Q);
	Up(BF_KEY_LCMD);
	v = Run();
	Check(Count(v, WM_CLOSE) == 1 && Count(v, WM_KEYDOWN) == 0 && Count(v, WM_KEYUP) == 0, "Cmd+Q: WM_CLOSE and no key message", Describe(v));
	Value(BF_APP_EVENT_QUIT, BF_APP_QUIT_CLOSE_BUTTON);
	v = Run();
	Check(Count(v, WM_CLOSE) == 1, "QUIT event (close button, Quit menu): WM_CLOSE", Describe(v));

	// Cmd+H and other Mac commands do not reach the client.
	Down(BF_KEY_LCMD);
	Down(BF_KEY_H);
	Up(BF_KEY_H);
	Up(BF_KEY_LCMD);
	v = Run();
	Check(v.empty(), "Cmd+H: nothing reaches the client", Describe(v));
}

void Clipboard()
{
	std::vector<Rec> v;
	char saved[4096];
	size_t saved_len = bf_app_clipboard_get_text(saved, sizeof saved);
	bool restore = saved_len > 0 && saved_len < sizeof saved;

	// Cmd+V types the clipboard into the focused field: WM_CHAR, CP1252.
	bf_app_clipboard_set_text("h\xc3\xa9llo \xe2\x82\xac" "5\n\xe4\xb8\x80!");
	Down(BF_KEY_LCMD);
	Down(BF_KEY_V);
	Up(BF_KEY_V);
	Up(BF_KEY_LCMD);
	v = Run();
	Check(Chars(v) == "h\xe9llo \x80" "5!" && Count(v, WM_KEYDOWN) == 0,
	      "Cmd+V: the clipboard as WM_CHARs in CP1252 (line break and a character CP1252 lacks dropped), no 'V'", Describe(v));

	// Cmd+C and Cmd+X read as Ctrl+C and Ctrl+X (CLanguage::OnKeyDown).
	Down(BF_KEY_LCMD);
	Down(BF_KEY_C);
	Up(BF_KEY_C);
	v = Run();
	Check(Find(v, WM_KEYDOWN) && Find(v, WM_KEYDOWN)->w == 'C' && Find(v, WM_KEYDOWN)->ctrl && (uint32_t)Find(v, WM_KEYDOWN)->l == 0x2e0001,
	      "Cmd+C: WM_KEYDOWN 'C' with GetKeyState(VK_CONTROL) down", Describe(v));
	Check(!HIBYTE(GetKeyState(VK_CONTROL)), "Cmd+C released: Ctrl reads up again");
	Down(BF_KEY_X);
	v = Run();
	Check(Find(v, WM_KEYDOWN) && Find(v, WM_KEYDOWN)->w == 'X' && Find(v, WM_KEYDOWN)->ctrl, "Cmd+X: WM_KEYDOWN 'X' with Ctrl down", Describe(v));
	Up(BF_KEY_LCMD);   // Cmd let go before X
	v = Run();
	Check(!HIBYTE(GetKeyState(VK_CONTROL)), "Cmd released while X held: Ctrl reads up");
	Up(BF_KEY_X);
	Run();

	// The clipboard API as CLanguage::CopyTextToClipBoard uses it.
	int live = g_fake_global_live;
	const char *text = "Gr\xfc\xdf" "e";   // CP1252
	BOOL ok = OpenClipboard(g_hwnd);
	HANDLE h = GlobalAlloc(GMEM_DDESHARE, strlen(text) + 1);
	char *p = (char *)GlobalLock(h);
	memcpy(p, text, strlen(text) + 1);
	GlobalUnlock(h);
	ok = ok && EmptyClipboard();
	ok = ok && SetClipboardData(CF_TEXT, h) != NULL;
	ok = ok && CloseClipboard();
	char got[64] = {};
	bf_app_clipboard_get_text(got, sizeof got);
	Check(ok && strcmp(got, "Gr\xc3\xbc\xc3\x9f" "e") == 0, "SetClipboardData(CF_TEXT): the Mac clipboard holds the text as UTF-8", got);
	// ... and CopyTextFromClipBoard.
	bf_app_clipboard_set_text("caf\xc3\xa9\nok");
	ok = OpenClipboard(g_hwnd);
	h = GetClipboardData(CF_TEXT);
	p = h ? (char *)GlobalLock(h) : nullptr;
	std::string read = p ? p : "";
	if (h)
		GlobalUnlock(h);
	CloseClipboard();
	Check(ok && read == "caf\xe9\r\nok", "GetClipboardData(CF_TEXT): CP1252, lines ending CR LF", read);
	Check(OpenClipboard(g_hwnd) && !OpenClipboard(g_hwnd) && CloseClipboard() && !CloseClipboard(), "OpenClipboard twice fails, CloseClipboard twice fails");
	OpenClipboard(g_hwnd);
	EmptyClipboard();      // frees GetClipboardData's block
	CloseClipboard();
	Check(g_fake_global_live == live, "clipboard memory: nothing leaked", Fmt("%d blocks before, %d after", live, g_fake_global_live));

	if (restore)
		bf_app_clipboard_set_text(saved);
	else
		bf_app_clipboard_set_text("");
}

// CMouse::ReadState's integration, as the client does it.
struct ClientMouse {
	IDirectInputDeviceA *dev;
	int x = 320, y = 240, wheel = 0;
	DIMOUSESTATE s = {};
	HRESULT Read()
	{
		bf_view_metrics m;
		bf_view_get(&m);
		HRESULT hr = dev->GetDeviceState(sizeof s, &s);
		if (SUCCEEDED(hr)) {
			x += s.lX;
			y += s.lY;
			wheel = s.lZ;
		}
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x >= m.logical_w) x = m.logical_w - 1;
		if (y >= m.logical_h) y = m.logical_h - 1;
		return hr;
	}
};

void Mouse()
{
	IDirectInputA *di = nullptr;
	IDirectInputDeviceA *dev = nullptr;
	HRESULT hr = DirectInputCreateA(GetModuleHandleA_stub(), DIRECTINPUT_VERSION, &di, NULL);
	Check(SUCCEEDED(hr) && di, "DirectInputCreateA");
	if (!di)
		return;
	hr = di->CreateDevice(GUID_SysMouse, &dev, NULL);
	Check(SUCCEEDED(hr) && dev, "IDirectInput::CreateDevice(GUID_SysMouse)");
	IDirectInputDeviceA *kbd = nullptr;
	Check(FAILED(di->CreateDevice(GUID_SysKeyboard, &kbd, NULL)) && !kbd, "CreateDevice(GUID_SysKeyboard) fails: the keyboard comes as messages");
	if (!dev)
		return;
	dev->SetDataFormat(&c_dfDIMouse);
	Check(SUCCEEDED(dev->SetCooperativeLevel(g_hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)), "SetCooperativeLevel(exclusive, foreground) accepted");
	Check(SUCCEEDED(dev->Acquire()), "Acquire");
	ClientMouse cm;
	cm.dev = dev;
	Run();
	cm.Read();

	bf_app_size size;
	bf_app_window_get_size(&size);
	for (int s = 1; s <= 3; s++) {
		bf_compat_input_set_scale_setting(s);
		Run();
		bf_view_metrics m;
		bf_view_get(&m);
		if (m.ui_scale != s) {
			Skip(Fmt("pointer at UI scale %d", s).c_str(), Fmt("the screen allows scale %d at most here (%dx%d px)", m.ui_scale, m.drawable_w, m.drawable_h).c_str());
			continue;
		}
		const float pts[][2] = {{0.5f, 0.5f}, {101.3f, 57.9f}, {(float)m.drawable_w * 0.5f, (float)m.drawable_h * 0.25f},
		                        {(float)m.view_w - 0.5f, (float)m.view_h - 0.5f}, {(float)m.drawable_w + 400, -300},
		                        {333.0f, 444.0f}};
		bool all = true;
		std::string bad;
		for (auto &pt : pts) {
			int ox = cm.x, oy = cm.y;
			Pointer(BF_APP_EVENT_POINTER_MOVE, pt[0], pt[1]);
			Run();
			cm.Read();
			int ex = (int)floor(pt[0] / s), ey = (int)floor(pt[1] / s);
			ex = ex < 0 ? 0 : ex >= m.logical_w ? m.logical_w - 1 : ex;
			ey = ey < 0 ? 0 : ey >= m.logical_h ? m.logical_h - 1 : ey;
			if (cm.x != ex || cm.y != ey || cm.s.lX != ex - ox || cm.s.lY != ey - oy) {
				all = false;
				bad += Fmt("(%.1f,%.1f) px -> client (%d,%d) lX %d lY %d, want (%d,%d) ", pt[0], pt[1], cm.x, cm.y,
				           (int)cm.s.lX, (int)cm.s.lY, ex, ey);
			}
		}
		Check(all, Fmt("UI scale %d (logical %dx%d): lX/lY bring CMouse to the pointer's logical pixel, clamped outside", s, m.logical_w, m.logical_h).c_str(), bad);

		// Right-drag: the device's movement, also past the window's edge.
		Pointer(BF_APP_EVENT_POINTER_MOVE, 200.0f * s, 200.0f * s);
		Run();
		cm.Read();
		Pointer(BF_APP_EVENT_BUTTON_DOWN, 200.0f * s, 200.0f * s, BF_APP_BUTTON_RIGHT);
		Run();
		cm.Read();
		bool rb = cm.s.rgbButtons[1] == 0x80 && cm.s.rgbButtons[0] == 0;
		int total = 0;
		float x = 200.0f * s;
		for (int i = 0; i < 8; i++) {
			x -= 50.0f * s;   // to -200 * s: 400 logical pixels left, the last 200 outside the window
			Pointer(BF_APP_EVENT_POINTER_MOVE, x, 200.0f * s);
			Run();
			cm.Read();
			total += cm.s.lX;
		}
		Check(rb && total == -400, Fmt("UI scale %d: right-drag reports the movement (sum of lX %d, want -400) past the edge; right button 0x80", s, total).c_str());
		Pointer(BF_APP_EVENT_BUTTON_UP, x, 200.0f * s, BF_APP_BUTTON_RIGHT);
		Pointer(BF_APP_EVENT_POINTER_MOVE, 90.0f * s + 0.5f, 100.0f * s + 0.5f);
		Run();
		cm.Read();
		Check(cm.x == 90 && cm.y == 100 && cm.s.rgbButtons[1] == 0, Fmt("UI scale %d: after the drag CMouse is back at the pointer", s).c_str(), Fmt("(%d,%d)", cm.x, cm.y));
	}

	// Wheel and buttons.
	Pointer(BF_APP_EVENT_WHEEL, 100, 100, 0, 1.0f);
	Run();
	cm.Read();
	int up = cm.wheel;
	Pointer(BF_APP_EVENT_WHEEL, 100, 100, 0, -2.0f);
	Run();
	cm.Read();
	Check(up == 120 && cm.wheel == -240, "wheel: lZ +120 per line away, -240 for two towards", Fmt("%d, %d", up, cm.wheel));
	Pointer(BF_APP_EVENT_BUTTON_DOWN, 100, 100, BF_APP_BUTTON_LEFT);
	Run();
	cm.Read();
	bool l = cm.s.rgbButtons[0] == 0x80 && HIBYTE(GetAsyncKeyState(VK_LBUTTON));
	Pointer(BF_APP_EVENT_BUTTON_UP, 100, 100, BF_APP_BUTTON_LEFT);
	Pointer(BF_APP_EVENT_BUTTON_DOWN, 100, 100, BF_APP_BUTTON_MIDDLE);
	Run();
	cm.Read();
	bool mid = cm.s.rgbButtons[2] == 0x80 && cm.s.rgbButtons[0] == 0;
	Pointer(BF_APP_EVENT_BUTTON_UP, 100, 100, BF_APP_BUTTON_MIDDLE);
	Run();
	cm.Read();
	Check(l && mid && cm.s.rgbButtons[2] == 0, "buttons: left and middle as rgbButtons[0], [2]; GetAsyncKeyState(VK_LBUTTON)");
	Check(GetSystemMetrics(SM_SWAPBUTTON) == 0, "GetSystemMetrics(SM_SWAPBUTTON) = 0");

	// Focus: WM_NCACTIVATE drives the client's SetAcquire.
	Value(BF_APP_EVENT_FOCUS, 0);
	std::vector<Rec> v = Run();
	Check(Find(v, WM_NCACTIVATE) && Find(v, WM_NCACTIVATE)->w == FALSE && Find(v, WM_ACTIVATE) && LOWORD(Find(v, WM_ACTIVATE)->w) == WA_INACTIVE && Find(v, WM_KILLFOCUS),
	      "focus lost: WM_NCACTIVATE(0), WM_ACTIVATE(WA_INACTIVE), WM_KILLFOCUS", Describe(v));
	Check(GetActiveWindow() == NULL, "focus lost: GetActiveWindow() is NULL");
	dev->Unacquire();
	Pointer(BF_APP_EVENT_POINTER_MOVE, 61, 43);
	v = Run();
	Check(cm.Read() == DIERR_NOTACQUIRED, "unacquired: GetDeviceState fails, as the client expects while inactive");
	bf_view_metrics m;
	bf_view_get(&m);
	int ex = (int)floor(61.0 / m.ui_scale), ey = (int)floor(43.0 / m.ui_scale);
	const Rec *mm = Find(v, WM_MOUSEMOVE);
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(g_hwnd, &pt);
	cm.x = pt.x;   // the client's WM_MOUSEMOVE handler: SetXPos, SetYPos
	cm.y = pt.y;
	Check(mm && LOWORD(mm->l) == ex && HIWORD(mm->l) == ey && pt.x == ex && pt.y == ey,
	      "inactive: WM_MOUSEMOVE and GetCursorPos/ScreenToClient give the logical position", Fmt("(%ld,%ld) want (%d,%d)", (long)pt.x, (long)pt.y, ex, ey));
	Value(BF_APP_EVENT_FOCUS, 1);
	v = Run();
	Check(Find(v, WM_NCACTIVATE) && Find(v, WM_NCACTIVATE)->w == TRUE && Find(v, WM_ACTIVATE) && LOWORD(Find(v, WM_ACTIVATE)->w) == WA_ACTIVE && Find(v, WM_SETFOCUS),
	      "focus gained: WM_NCACTIVATE(1), WM_ACTIVATE(WA_ACTIVE), WM_SETFOCUS", Describe(v));
	dev->Acquire();
	Pointer(BF_APP_EVENT_POINTER_MOVE, 61 + 10.0f * m.ui_scale, 43);
	Run();
	cm.Read();
	Check(cm.x == ex + 10 && cm.y == ey, "after reactivation CMouse follows the pointer from where WM_MOUSEMOVE left it", Fmt("(%d,%d)", cm.x, cm.y));

	// SetCursorPos records, never warps.
	SetCursorPos(5, 6);
	GetCursorPos(&pt);
	bool rec = pt.x == 5 && pt.y == 6;
	Pointer(BF_APP_EVENT_POINTER_MOVE, 61, 43);
	Run();
	GetCursorPos(&pt);
	Check(rec && pt.x == ex && pt.y == ey, "SetCursorPos: GetCursorPos reports it until the pointer moves; the pointer is not warped");
	bf_compat_input_mouse_set_client_pos(cm.x, cm.y);

	dev->Unacquire();
	dev->Release();
	di->Release();
}

void Resize()
{
	bf_compat_input_set_scale_setting(0);
	Run();
	bf_app_window_set_content_size(1280, 800);
	bf_app_pump(50);
	std::vector<Rec> v = Run();
	bf_view_metrics m;
	bf_view_get(&m);
	RECT rc;
	GetClientRect(g_hwnd, &rc);
	const Rec *sz = nullptr;
	for (const Rec &r : v)
		if (r.msg == WM_SIZE)
			sz = &r;
	Check(sz && LOWORD(sz->l) == m.logical_w && HIWORD(sz->l) == m.logical_h && rc.right == m.logical_w && rc.bottom == m.logical_h,
	      "resize to 1280x800 pt: WM_SIZE and GetClientRect give the new logical size", Fmt("logical %dx%d at scale %d", m.logical_w, m.logical_h, m.ui_scale));
	bf_app_window_set_content_size(1024, 768);
	bf_app_pump(50);
	Run();
}

void Misc()
{
	char buf[64];
	int n = wsprintfA(buf, "%d.%d.%d.%d", 127, 0, 0, 1);
	DWORD code = 7;
	char buf2[64];
	wsprintf(buf2, "GameGuard Error: %lu", code);
	Check(n == 9 && strcmp(buf, "127.0.0.1") == 0 && strcmp(buf2, "GameGuard Error: 7") == 0, "wsprintfA (its %lu is 32 bits)", std::string(buf) + " / " + buf2);
	const char *t = "a\xe9" "b";
	const char *k = "x\xb0\xa1y";   // CP949: x, one Hangul syllable, y
	Check(CharNextExA(1252, t, 0) == t + 1 && CharNextExA(1252, t + 1, 0) == t + 2 && CharNextExA(1252, t + 3, 0) == t + 3,
	      "CharNextExA(1252)");
	Check(CharNextExA(949, k + 1, 0) == k + 3 && CharPrevExA(949, k, k + 3, 0) == k + 1 && CharPrevExA(949, k, k, 0) == k,
	      "CharNextExA / CharPrevExA (949): a lead byte and its trail are one character");
	Check(ImmGetContext(g_hwnd) == NULL, "ImmGetContext: no input method context");
	Check(MapVirtualKeyA('A', MAPVK_VK_TO_VSC) == 0x1e && MapVirtualKeyA(0x1e, MAPVK_VSC_TO_VK) == 'A', "MapVirtualKeyA");
	Check(GetDC(NULL) != NULL && ReleaseDC(NULL, GetDC(NULL)) == 1, "GetDC(NULL): the gdi area's screen DC");
	BYTE ks[256];
	bool none = GetKeyboardState(ks);
	for (int i = 0; i < 256 && none; i++)
		if (ks[i] & 0x80)
			none = false;
	Check(none, "GetKeyboardState: nothing held after the tests");
	unsigned scan = 0;
	int ext = 0;
	Check(bf_compat_input_vk_for_key(BF_KEY_KP_ENTER, &scan, &ext) == VK_RETURN && scan == 0x1c && ext == 1, "keypad Enter: VK_RETURN, extended");
}

int g_user_events = 0;
int Hook(const bf_app_event *e, void *)
{
	if (e->type == BF_APP_EVENT_USER) {
		g_user_events++;
		return 1;
	}
	return 0;
}

int SecondInstance(const char *id)
{
	bf_compat_input_configure(id, 1);
	FindWindowA("Bifrost test", "Bifrost test");   // exits when the slot is taken
	return 3;
}

}  // namespace

// The win32 area owns GetModuleHandle; the client passes g_hInstance, which
// DirectInputCreateA ignores.
HINSTANCE GetModuleHandleA_stub() { return nullptr; }

int main(int argc, char **argv)
{
	if (argc == 3 && strcmp(argv[1], "--second") == 0)
		return SecondInstance(argv[2]);
	setvbuf(stdout, nullptr, _IOLBF, 0);
	char id[64];
	snprintf(id, sizeof id, "compat-input-test-%d", (int)getpid());
	bf_compat_input_configure(id, 1);

	// InitApp's sequence.
	Check(FindWindowA("Bifrost test", "Bifrost test") == NULL, "FindWindowA: no other instance");
	WNDCLASSA wc = {0, WindowProc, 0, 0, NULL, LoadIconA(NULL, MAKEINTRESOURCEA(0x77)), LoadCursorA(NULL, IDC_ARROW),
	                NULL, NULL, "Bifrost test"};
	Check(RegisterClassA(&wc) != 0, "RegisterClassA");
	RECT r = {0, 0, 1024, 768};
	AdjustWindowRect(&r, 0x02CA0000, FALSE);
	g_hwnd = CreateWindowExA(0, wc.lpszClassName, "Bifrost test", 0x02CA0000, 0, 0, r.right - r.left, r.bottom - r.top,
	                         NULL, NULL, NULL, NULL);
	Check(g_hwnd != NULL && g_created && bf_compat_input_main_window() == g_hwnd, "CreateWindowExA: window made, WM_CREATE sent");
	if (!g_hwnd)
		return 1;
	ShowWindow(g_hwnd, SW_SHOW);
	UpdateWindow(g_hwnd);
	SetFocus(g_hwnd);
	Check(FindWindowA("Bifrost test", "Bifrost test") == g_hwnd, "FindWindowA finds the window in its own process");
	bf_view_metrics m;
	bf_view_get(&m);
	RECT rc;
	GetClientRect(g_hwnd, &rc);
	Check(rc.right == m.logical_w && rc.bottom == m.logical_h && m.logical_w > 0,
	      "GetClientRect: the logical size", Fmt("%dx%d, UI scale %d, backing %.0f", (int)rc.right, (int)rc.bottom, m.ui_scale, m.backing_scale));
	bf_compat_input_set_event_hook(Hook, nullptr);
	Run();

	Keyboard();
	Clipboard();
	Mouse();
	Resize();

	bf_app_event u = {};
	u.type = BF_APP_EVENT_USER;
	bf_app_post_event(&u);
	std::vector<Rec> v = Run();
	Check(g_user_events == 1 && v.empty(), "event hook: consumes a USER event");
	bf_compat_input_set_event_hook(nullptr, nullptr);

	Misc();

	// A second launch with the same slot: bf_app_init brings this one to
	// the front and FindWindowA ends the second process.
	pid_t pid;
	char *args[] = {argv[0], (char *)"--second", id, nullptr};
	time_t t0 = time(nullptr);
	int st = -1;
	if (posix_spawn(&pid, argv[0], nullptr, nullptr, args, environ) == 0) {
		while (waitpid(pid, &st, WNOHANG) == 0 && time(nullptr) - t0 < 20)
			bf_app_pump(50);
	}
	Check(WIFEXITED(st) && WEXITSTATUS(st) == 0, "second instance: FindWindowA exits it (status 0)", Fmt("status %d", st));
	Run();

	PostMessageA(g_hwnd, WM_USER + 1, 1, 2);
	v = Run();
	Check(v.size() == 1 && v[0].msg == WM_USER + 1 && v[0].w == 1 && v[0].l == 2, "PostMessageA reaches the window procedure");
	Check(SendMessageA(g_hwnd, WM_USER + 2, 0, 0) == 0 && g_recs.back().msg == WM_USER + 2, "SendMessageA calls it directly");
	PostQuitMessage(0);
	MSG msg;
	Check(GetMessageA(&msg, NULL, 0, 0) == FALSE && msg.message == WM_QUIT, "PostQuitMessage: GetMessageA returns FALSE with WM_QUIT");

	Check(DestroyWindow(g_hwnd) && !IsWindow(g_hwnd) && !DestroyWindow(g_hwnd), "DestroyWindow");
	bf_app_shutdown();
	printf("%s: %d failed\n", g_failures ? "FAIL" : "PASS", g_failures);
	return g_failures ? 1 : 0;
}
