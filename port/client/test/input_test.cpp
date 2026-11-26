// bifrost-input-test: the client's window, pump, keys, mouse and clipboard
// (port/client/include/bf_input.h, M11.7), called as WinMain.cpp, CMouse,
// CLanguage and CBink call them, with the checks of the input adapter's test
// it replaces (port/compat/input, deleted in M11.7). It opens a window in its
// own single-instance slot and injects every event through
// bf_app_post_event, as the control socket does, so it runs on the locked
// screen of the unattended Mac. One line per check; exit status 1 on any
// failure.
#include "bf_types.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "bf_app.h"
#include "bf_input.h"
#include "bf_view.h"

// What the client reports through bf_input_client_text_focus: whether a
// text field holds the keyboard (in game the chat line usually does).
bool g_text_focus = true;
extern "C" int bf_input_client_text_focus(void)
{
	return g_text_focus;
}

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

bool Held(int vk)
{
	return HIBYTE(bf_input_key_state(vk)) != 0;
}

// ------------------------------------------------------------ the window procedure

struct Rec {
	UINT msg;
	WPARAM w;
	LPARAM l;
	bool ctrl, shift, alt;   // key state high bits while it was handled
	bool async;              // bf_input_async_key_state(wParam) for key messages
};
std::vector<Rec> g_recs;
HWND g_hwnd = nullptr;
bool g_created = false;
int g_destroyed = 0;

LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	Rec r = {message, wparam, lparam, false, false, false, false};
	r.ctrl = Held(VK_CONTROL);
	r.shift = Held(VK_SHIFT);
	r.alt = HIBYTE(bf_input_async_key_state(VK_MENU)) != 0;
	if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)
		r.async = HIBYTE(bf_input_async_key_state((int)wparam)) != 0;
	if (message != WM_SETCURSOR && message != WM_MOUSEMOVE)
		g_recs.push_back(r);
	switch (message) {
	case WM_CREATE:
		g_created = true;
		return 0;
	case WM_DESTROY:
		g_destroyed++;
		return 0;
	case WM_SETCURSOR:
		bf_input_hide_cursor();   // as the client does outside fullscreen
		return 0;
	case WM_MOUSEMOVE:
		g_recs.push_back(r);
		return 0;
	case WM_CLOSE:
		return 0;   // the client sets g_sysQuit and returns
	case WM_CHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
		return 0;
	}
	return bf_input_default_proc(hwnd, message, wparam, lparam);
}

// CheckSystemMessage of WinMain.cpp.
void Pump()
{
	bf_input_msg msg;
	while (bf_input_peek(&msg))
		bf_input_dispatch(&msg);
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

// Pumps, and returns what the window procedure saw.
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
	Check(Is(Find(v, WM_KEYDOWN), 'A', 0x1e0001) && Find(v, WM_KEYDOWN)->async, "key A: WM_KEYDOWN 'A' lParam 0x1e0001, async key state down", Describe(v));
	Check(Chars(v) == "a" && Find(v, WM_CHAR) > Find(v, WM_KEYDOWN), "key A: WM_CHAR 'a' after it", Describe(v));
	Down(BF_KEY_A, 1);
	v = Run();
	Check(Is(Find(v, WM_KEYDOWN), 'A', 0x401e0001), "key A repeat: bit 30 set (ProcessPushButton's newKeydown 0)", Describe(v));
	Up(BF_KEY_A);
	v = Run();
	Check(Is(Find(v, WM_KEYUP), 'A', 0xc01e0001) && !Find(v, WM_KEYUP)->async && !HIBYTE(bf_input_async_key_state('A')),
	      "key A up: WM_KEYUP lParam 0xc01e0001, async key state up", Describe(v));
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
	      "Shift+A: WM_KEYDOWN VK_SHIFT scan 0x2a, VK_SHIFT held during 'A', WM_CHAR 'A'", Describe(v));
	Check(!Held(VK_SHIFT) && !HIBYTE(bf_input_async_key_state(VK_LSHIFT)), "Shift released: key state up");

	// Option is Alt: WM_SYSKEYDOWN, no character for a game shortcut.
	Down(BF_KEY_LALT);
	s = Down(BF_KEY_A);
	Text("\xc3\xa5", s);   // what Option+A types on a US layout
	Up(BF_KEY_A);
	v = Run();
	Check(Is(Find(v, WM_SYSKEYDOWN, 0), VK_MENU, 0x20380001) && Find(v, WM_SYSKEYDOWN, 0)->alt,
	      "Option: WM_SYSKEYDOWN VK_MENU, VK_MENU down", Describe(v));
	Check(Is(Find(v, WM_SYSKEYDOWN, 1), 'A', 0x201e0001), "Option+A: WM_SYSKEYDOWN 'A', bit 29 set", Describe(v));
	Check(Count(v, WM_CHAR) == 0, "Option+A (Alt+A opens a window): no WM_CHAR", Describe(v));
	s = Down(BF_KEY_L);
	Text("@", s);          // Option+L on a German layout
	Up(BF_KEY_L);
	s = Down(BF_KEY_F);
	Text("\xc6\x92", s);   // Option+F on a US layout: f with hook, CP1252 0x83
	Up(BF_KEY_F);
	Up(BF_KEY_LALT);
	v = Run();
	Check(Find(v, WM_SYSKEYDOWN, 0) && Find(v, WM_SYSKEYDOWN, 0)->w == 'L' && Find(v, WM_SYSKEYDOWN, 1) &&
	      Find(v, WM_SYSKEYDOWN, 1)->w == 'F' && Chars(v) == "\x83",
	      "Option+L (Alt+L, emotion list): WM_SYSKEYDOWN 'L', no '@'; Option+F (no game shortcut): WM_CHAR", Describe(v));
	Check(Find(v, WM_SYSKEYUP, 2) && Find(v, WM_SYSKEYUP, 2)->w == VK_MENU && !HIBYTE(bf_input_async_key_state(VK_MENU)),
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
	Down(BF_KEY_LCTRL);
	Down(BF_KEY_BACKSPACE);
	Up(BF_KEY_BACKSPACE);
	Up(BF_KEY_LCTRL);
	v = Run();
	Check(Count(v, WM_CHAR) == 0, "Ctrl+Backspace: no WM_CHAR 8", Describe(v));

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
	      "Cmd+I (the Insert substitute): WM_KEYDOWN VK_INSERT only, VK_INSERT reads down", Describe(v));
	Up(BF_KEY_I);
	Down(BF_KEY_LALT);
	Down(BF_KEY_I);
	Up(BF_KEY_I);
	Up(BF_KEY_LALT);
	Up(BF_KEY_LCMD);
	v = Run();
	Check(Find(v, WM_KEYUP) && Find(v, WM_KEYUP)->w == VK_INSERT && Find(v, WM_SYSKEYDOWN, 1) && Find(v, WM_SYSKEYDOWN, 1)->w == VK_INSERT,
	      "Cmd+I up: WM_KEYUP VK_INSERT; Cmd+Option+I: WM_SYSKEYDOWN VK_INSERT (Alt+Insert)", Describe(v));
	Check(!HIBYTE(bf_input_async_key_state(VK_INSERT)) && !HIBYTE(bf_input_async_key_state(VK_LWIN)), "after Cmd+I: VK_INSERT and Cmd read up");

	// Cmd+Shift+S: the Print Screen substitute, WM_KEYUP VK_SNAPSHOT only;
	// Cmd+S without Shift sends nothing.
	Down(BF_KEY_LCMD);
	Down(BF_KEY_LSHIFT);
	s = Down(BF_KEY_S);
	Text("S", s);
	Up(BF_KEY_S);
	Up(BF_KEY_LSHIFT);
	Down(BF_KEY_S);
	Up(BF_KEY_S);
	Up(BF_KEY_LCMD);
	v = Run();
	{
		int snaps = 0, snapDowns = 0;
		for (const auto &m : v) {
			if (m.w == VK_SNAPSHOT && m.msg == WM_KEYUP) snaps++;
			if (m.w == VK_SNAPSHOT && m.msg == WM_KEYDOWN) snapDowns++;
		}
		Check(snaps == 1 && snapDowns == 0 && Count(v, WM_CHAR) == 0 && !(Find(v, WM_KEYDOWN) && Find(v, WM_KEYDOWN)->w == 'S'),
		      "Cmd+Shift+S (the Print Screen substitute): one WM_KEYUP VK_SNAPSHOT, no 'S'; Cmd+S: nothing", Describe(v));
	}

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
	bf_input_post(WM_CLOSE, 0, 0);   // the control socket's quit
	v = Run();
	Check(Count(v, WM_CLOSE) == 1 && v.size() == 1, "bf_input_post(WM_CLOSE) (the control socket's quit): WM_CLOSE", Describe(v));

	// Cmd+H and other Mac commands do not reach the client.
	Down(BF_KEY_LCMD);
	Down(BF_KEY_H);
	Up(BF_KEY_H);
	Up(BF_KEY_LCMD);
	v = Run();
	Check(v.empty(), "Cmd+H: nothing reaches the client", Describe(v));

	// Caps Lock's latch and Num Lock.
	Check((bf_input_key_state(VK_NUMLOCK) & 1) && !(bf_input_async_key_state(VK_NUMLOCK) & 1),
	      "VK_NUMLOCK: toggled (a Mac keypad types digits); the async state has no bit 0");
}

// The key-down AppKit would queue, for bf_input_text_filter.
bf_app_event AppKitDown(int key, uint32_t mods, uint64_t seq)
{
	bf_app_event e = {};
	e.type = BF_APP_EVENT_KEY_DOWN;
	e.source = BF_APP_SOURCE_APPKIT;
	e.mods = mods;
	e.seq = seq;
	e.key.key = (uint16_t)key;
	e.key.mac_keycode = (uint16_t)bf_app_key_to_mac(key);
	return e;
}

std::string LayoutText(int key, uint32_t mods, int *dead = nullptr)
{
	char b[16];
	bf_app_key_layout_text((unsigned)bf_app_key_to_mac(key), mods, b, sizeof b, dead);
	return b;
}

// Option with a layout (F1): what the layout gives with Option types in a
// text field when the layout has it nowhere else; otherwise Option is Alt.
// The layouts are named through bf_app_key_layout_use, which reads an
// installed layout without selecting it, and the key-downs are injected with
// the text AppKit would type for them, linked by key_seq.
void OptionLayout()
{
	std::vector<Rec> v;
	const uint32_t opt = BF_APP_MOD_ALT, optShift = BF_APP_MOD_ALT | BF_APP_MOD_SHIFT;
	if (bf_app_key_layout_use("com.apple.keylayout.German") != BF_APP_OK) {
		Skip("German layout", "com.apple.keylayout.German is not installed");
		return;
	}
	Check(bf_app_key_layout_named() && strcmp(bf_app_key_layout_named(), "com.apple.keylayout.German") == 0,
	      "bf_app_key_layout_use: German named");

	// What the German layout gives.
	struct { int key; uint32_t mods; const char *want; const char *name; } de[] = {
		{BF_KEY_L, opt, "@", "Option+L"}, {BF_KEY_5, opt, "[", "Option+5"}, {BF_KEY_6, opt, "]", "Option+6"},
		{BF_KEY_7, opt, "|", "Option+7"}, {BF_KEY_7, optShift, "\\", "Option+Shift+7"}, {BF_KEY_8, opt, "{", "Option+8"},
		{BF_KEY_9, opt, "}", "Option+9"}, {BF_KEY_N, opt, "~", "Option+N"}, {BF_KEY_E, opt, "\xe2\x82\xac", "Option+E"},
		{BF_KEY_A, opt, "\xc3\xa5", "Option+A"}};
	std::string got;
	bool all = true;
	for (const auto &k : de) {
		std::string t = LayoutText(k.key, k.mods);
		got += Fmt("%s \"%s\" ", k.name, t.c_str());
		all = all && t == k.want;
	}
	Check(all, "German layout: Option+L @, 5 [, 6 ], 7 |, Shift+7 \\, 8 {, 9 }, N ~, E \xe2\x82\xac, A \xc3\xa5", got);
	int dead = 0;
	LayoutText(BF_KEY_N, opt, &dead);
	Check(dead == 1, "German Option+N is a dead key (its accent \"~\")");
	Check(!bf_app_key_layout_types_plain("@") && !bf_app_key_layout_types_plain("[") && bf_app_key_layout_types_plain("#") &&
	      bf_app_key_layout_types_plain("<") && bf_app_key_layout_types_plain("^"),
	      "German layout: @ and [ only with Option; #, <, ^ (dead) on keys of their own");

	// German, a text field holds the keyboard: Option+L types "@".
	g_text_focus = true;
	Down(BF_KEY_LALT);
	uint64_t s = Down(BF_KEY_L);
	Text("@", s);
	v = Run();
	const Rec *l = Find(v, WM_KEYDOWN);
	Check(Is(l, 'L', 0x260001) && !l->alt && !Find(v, WM_SYSKEYDOWN, 1) && Chars(v) == "@",
	      "German, text field: Option+L is WM_KEYDOWN 'L' without the Alt bit, Alt reads up, WM_CHAR '@' (no emotion list)",
	      Describe(v));
	Check(!HIBYTE(bf_input_async_key_state(VK_MENU)) && !HIBYTE(bf_input_key_state(VK_LMENU)),
	      "German, text field: VK_MENU and VK_LMENU read up while L is held");
	s = Down(BF_KEY_L, 1);
	Text("@", s);
	v = Run();
	Check(Is(Find(v, WM_KEYDOWN), 'L', 0x40260001) && Chars(v) == "@", "German, text field: Option+L repeat types '@' again", Describe(v));
	Up(BF_KEY_L);
	v = Run();
	Check(Is(Find(v, WM_KEYUP), 'L', 0xc0260001) && HIBYTE(bf_input_async_key_state(VK_MENU)),
	      "German, text field: L up is WM_KEYUP; Alt reads down again (Option still held)", Describe(v));

	// The other characters German has only with Option; Option+E (euro) and
	// Option+A (a ring) stay Alt+E (inventory) and Alt+A (status).
	struct { int key; bool shift; const char *text; } typed[] = {
		{BF_KEY_5, false, "["}, {BF_KEY_6, false, "]"}, {BF_KEY_7, false, "|"}, {BF_KEY_7, true, "\\"},
		{BF_KEY_8, false, "{"}, {BF_KEY_9, false, "}"}};
	for (const auto &k : typed) {
		if (k.shift)
			Down(BF_KEY_LSHIFT);
		s = Down(k.key);
		Text(k.text, s);
		Up(k.key);
		if (k.shift)
			Up(BF_KEY_LSHIFT);
	}
	v = Run();
	int sysDigits = 0;
	for (const Rec &r : v)
		sysDigits += r.msg == WM_SYSKEYDOWN && r.w >= '0' && r.w <= '9';
	Check(Chars(v) == "[]|\\{}" && sysDigits == 0,
	      "German, text field: Option+5, 6, 7, Shift+7, 8, 9 type [ ] | \\ { } as WM_KEYDOWN, no Alt+digit", Describe(v));
	s = Down(BF_KEY_E);
	Text("\xe2\x82\xac", s);
	Up(BF_KEY_E);
	s = Down(BF_KEY_A);
	Text("\xc3\xa5", s);
	Up(BF_KEY_A);
	v = Run();
	Check(Find(v, WM_SYSKEYDOWN) && Find(v, WM_SYSKEYDOWN)->w == 'E' && Find(v, WM_SYSKEYDOWN, 1) &&
	      Find(v, WM_SYSKEYDOWN, 1)->w == 'A' && Count(v, WM_CHAR) == 0,
	      "German, text field: Option+E (euro, not ASCII) and Option+A stay Alt+E, Alt+A: WM_SYSKEYDOWN, no WM_CHAR", Describe(v));

	// Ctrl+Option+L reaches Alt+L in a text field.
	Down(BF_KEY_LCTRL);
	s = Down(BF_KEY_L);
	Text("@", s);   // AppKit would type nothing with Ctrl; dropped anyway
	Up(BF_KEY_L);
	Up(BF_KEY_LCTRL);
	v = Run();
	l = nullptr;
	for (const Rec &r : v)
		if ((r.msg == WM_KEYDOWN || r.msg == WM_SYSKEYDOWN) && r.w == 'L')
			l = &r;
	Check(l && l->alt && l->ctrl && Count(v, WM_CHAR) == 0,
	      "German, text field: Ctrl+Option+L is 'L' with Alt (and Ctrl) held: Alt+L, the emotion list; no '@'", Describe(v));
	Up(BF_KEY_LALT);
	Run();

	// German, no text field: Option is Alt.
	g_text_focus = false;
	Down(BF_KEY_LALT);
	s = Down(BF_KEY_L);
	Text("@", s);
	Up(BF_KEY_L);
	s = Down(BF_KEY_5);
	Text("[", s);
	Up(BF_KEY_5);
	Up(BF_KEY_LALT);
	v = Run();
	Check(Is(Find(v, WM_SYSKEYDOWN, 1), 'L', 0x20260001) && Find(v, WM_SYSKEYDOWN, 1)->alt && Find(v, WM_SYSKEYDOWN, 2) &&
	      Find(v, WM_SYSKEYDOWN, 2)->w == '5' && Count(v, WM_CHAR) == 0,
	      "German, no text field: Option+L, Option+5 are Alt+L, Alt+5 (WM_SYSKEYDOWN, bit 29), no WM_CHAR", Describe(v));

	// The text filter, for AppKit's key-downs: what reaches the text system.
	g_text_focus = true;
	bf_app_event e = AppKitDown(BF_KEY_L, opt, 0);
	int deL = bf_input_text_filter(&e, nullptr);
	e = AppKitDown(BF_KEY_E, opt, 0);
	int deE = bf_input_text_filter(&e, nullptr);
	g_text_focus = false;
	e = AppKitDown(BF_KEY_L, opt, 0);
	int deLout = bf_input_text_filter(&e, nullptr);
	g_text_focus = true;
	Check(deL == 1 && deE == 0 && deLout == 0, "text filter, German: Option+L in a text field reaches the text system; "
	      "Option+E, and Option+L outside one, do not", Fmt("%d %d %d", deL, deE, deLout));

	// US layout: no Option character is ASCII, so every shortcut stays.
	Check(bf_app_key_layout_use("com.apple.keylayout.US") == BF_APP_OK, "bf_app_key_layout_use: US named");
	std::string usL = LayoutText(BF_KEY_L, opt), usE = LayoutText(BF_KEY_E, opt, &dead);
	int usGraveDead = 0;
	std::string usGrave = LayoutText(BF_KEY_GRAVE, opt, &usGraveDead);
	Check(usL == "\xc2\xac" && usE == "\xc2\xb4" && dead == 1 && usGrave == "`" && usGraveDead && bf_app_key_layout_types_plain("`") &&
	      bf_app_key_layout_types_plain("@"),
	      "US layout: Option+L \xc2\xac, Option+E \xc2\xb4 (dead), Option+` ` (dead, also on its own key), @ on Shift+2",
	      Fmt("\"%s\" \"%s\" \"%s\"", usL.c_str(), usE.c_str(), usGrave.c_str()));
	Down(BF_KEY_LALT);
	s = Down(BF_KEY_L);
	Text("\xc2\xac", s);
	Up(BF_KEY_L);
	s = Down(BF_KEY_F);
	Text("\xc6\x92", s);
	Up(BF_KEY_F);
	Up(BF_KEY_LALT);
	v = Run();
	Check(Is(Find(v, WM_SYSKEYDOWN, 1), 'L', 0x20260001) && Chars(v) == "\x83",
	      "US, text field: Option+L is Alt+L (emotion list), no character; Option+F (no shortcut) types \xc6\x92", Describe(v));
	e = AppKitDown(BF_KEY_E, opt, 0);
	int usEf = bf_input_text_filter(&e, nullptr);
	e = AppKitDown(BF_KEY_N, opt, 0);
	int usNf = bf_input_text_filter(&e, nullptr);
	e = AppKitDown(BF_KEY_A, 0, 0);
	int usAf = bf_input_text_filter(&e, nullptr);
	e = AppKitDown(BF_KEY_E, BF_APP_MOD_ALT | BF_APP_MOD_CTRL, 0);
	int usCEf = bf_input_text_filter(&e, nullptr);
	Check(usEf == 0 && usNf == 1 && usAf == 1 && usCEf == 1,
	      "text filter, US: Option+E (Alt+E, a dead key) is kept from the text system, so no accent is left pending; "
	      "Option+N (no shortcut: n tilde), A, Ctrl+Option+E reach it", Fmt("%d %d %d %d", usEf, usNf, usAf, usCEf));

	// No layout named: injected key-downs do not follow the Mac's layout.
	bf_app_key_layout_use(nullptr);
	Check(bf_app_key_layout_named() == nullptr, "bf_app_key_layout_use(NULL): back to the layout in effect");
	Down(BF_KEY_LALT);
	s = Down(BF_KEY_L);
	Text("@", s);
	Up(BF_KEY_L);
	Up(BF_KEY_LALT);
	v = Run();
	Check(Is(Find(v, WM_SYSKEYDOWN, 1), 'L', 0x20260001) && Count(v, WM_CHAR) == 0,
	      "no layout named, text field: an injected Option+L is Alt+L whatever the Mac's layout (scripted tests)", Describe(v));
	char active[16];
	bf_app_key_layout_text((unsigned)bf_app_key_to_mac(BF_KEY_L), opt, active, sizeof active, nullptr);
	printf("info the layout in effect types \"%s\" for Option+L\n", active);
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
	      "Cmd+C: WM_KEYDOWN 'C' with VK_CONTROL down", Describe(v));
	Check(!Held(VK_CONTROL), "Cmd+C released: Ctrl reads up again");
	Down(BF_KEY_X);
	v = Run();
	Check(Find(v, WM_KEYDOWN) && Find(v, WM_KEYDOWN)->w == 'X' && Find(v, WM_KEYDOWN)->ctrl, "Cmd+X: WM_KEYDOWN 'X' with Ctrl down", Describe(v));
	Up(BF_KEY_LCMD);   // Cmd let go before X
	v = Run();
	Check(!Held(VK_CONTROL), "Cmd released while X held: Ctrl reads up");
	Up(BF_KEY_X);
	Run();

	// CLanguage::CopyTextToClipBoard ...
	const char *text = "Gr\xfc\xdf" "e\r\nx";   // CP1252
	int ok = bf_input_clipboard_set_text(text, strlen(text));
	char got[64] = {};
	bf_app_clipboard_get_text(got, sizeof got);
	Check(ok && strcmp(got, "Gr\xc3\xbc\xc3\x9f" "e\nx") == 0, "clipboard set: the Mac clipboard holds the text as UTF-8, CR LF as LF", got);
	ok = bf_input_clipboard_set_text("abc\0def", 7);
	bf_app_clipboard_get_text(got, sizeof got);
	Check(ok && strcmp(got, "abc") == 0, "clipboard set: up to the first NUL within the length", got);
	// ... and CopyTextFromClipBoard.
	bf_app_clipboard_set_text("caf\xc3\xa9\nok");
	char *read = bf_input_clipboard_text();
	Check(read && strcmp(read, "caf\xe9\r\nok") == 0, "clipboard text: CP1252, lines ending CR LF", read ? read : "(null)");
	free(read);
	bf_app_clipboard_set_text("\xe4\xb8\x80");
	read = bf_input_clipboard_text();
	Check(!read, "clipboard text: NULL when nothing converts (CLanguage then adds nothing)");
	free(read);

	if (restore)
		bf_app_clipboard_set_text(saved);
	else
		bf_app_clipboard_set_text("");
}

// CMouse::ReadState's integration, as the client does it.
struct ClientMouse {
	int x = 320, y = 240, wheel = 0;
	bf_input_mouse_state s = {};
	int Read()
	{
		bf_view_metrics m;
		bf_view_get(&m);
		int hr = bf_input_mouse_read(x, y, &s);
		if (hr == 0) {
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
	bf_input_mouse_acquire(1);
	ClientMouse cm;
	Run();
	cm.Read();

	for (int s = 1; s <= 3; s++) {
		bf_input_set_scale_setting(s);
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
				           cm.s.lX, cm.s.lY, ex, ey);
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
	bool l = cm.s.rgbButtons[0] == 0x80 && HIBYTE(bf_input_async_key_state(VK_LBUTTON));
	Pointer(BF_APP_EVENT_BUTTON_UP, 100, 100, BF_APP_BUTTON_LEFT);
	Pointer(BF_APP_EVENT_BUTTON_DOWN, 100, 100, BF_APP_BUTTON_MIDDLE);
	Run();
	cm.Read();
	bool mid = cm.s.rgbButtons[2] == 0x80 && cm.s.rgbButtons[0] == 0;
	Pointer(BF_APP_EVENT_BUTTON_UP, 100, 100, BF_APP_BUTTON_MIDDLE);
	Run();
	cm.Read();
	Check(l && mid && cm.s.rgbButtons[2] == 0, "buttons: left and middle as rgbButtons[0], [2]; VK_LBUTTON reads down");

	// Focus: WM_NCACTIVATE drives the client's SetAcquire.
	Value(BF_APP_EVENT_FOCUS, 0);
	std::vector<Rec> v = Run();
	Check(Find(v, WM_NCACTIVATE) && Find(v, WM_NCACTIVATE)->w == FALSE && Find(v, WM_ACTIVATE) && LOWORD(Find(v, WM_ACTIVATE)->w) == WA_INACTIVE && Find(v, WM_KILLFOCUS),
	      "focus lost: WM_NCACTIVATE(0), WM_ACTIVATE(WA_INACTIVE), WM_KILLFOCUS", Describe(v));
	bf_input_mouse_acquire(0);
	Pointer(BF_APP_EVENT_POINTER_MOVE, 61, 43);
	v = Run();
	Check(cm.Read() == BF_INPUT_NOT_ACQUIRED && FAILED(BF_INPUT_NOT_ACQUIRED), "unacquired: the read fails, as the client expects while inactive");
	bf_view_metrics m;
	bf_view_get(&m);
	int ex = (int)floor(61.0 / m.ui_scale), ey = (int)floor(43.0 / m.ui_scale);
	const Rec *mm = Find(v, WM_MOUSEMOVE);
	int px = -1, py = -1;
	bf_input_cursor_pos(&px, &py);
	cm.x = px;   // the client's WM_MOUSEMOVE handler: SetXPos, SetYPos
	cm.y = py;
	Check(mm && LOWORD(mm->l) == ex && HIWORD(mm->l) == ey && px == ex && py == ey,
	      "inactive: WM_MOUSEMOVE and bf_input_cursor_pos give the logical position", Fmt("(%d,%d) want (%d,%d)", px, py, ex, ey));
	Value(BF_APP_EVENT_FOCUS, 1);
	v = Run();
	Check(Find(v, WM_NCACTIVATE) && Find(v, WM_NCACTIVATE)->w == TRUE && Find(v, WM_ACTIVATE) && LOWORD(Find(v, WM_ACTIVATE)->w) == WA_ACTIVE && Find(v, WM_SETFOCUS),
	      "focus gained: WM_NCACTIVATE(1), WM_ACTIVATE(WA_ACTIVE), WM_SETFOCUS", Describe(v));
	Check(Find(v, WM_NCACTIVATE) && bf_input_default_proc(g_hwnd, WM_NCACTIVATE, 1, 0) == TRUE, "default procedure: WM_NCACTIVATE is TRUE");
	bf_input_mouse_acquire(1);
	Pointer(BF_APP_EVENT_POINTER_MOVE, 61 + 10.0f * m.ui_scale, 43);
	Run();
	cm.Read();
	Check(cm.x == ex + 10 && cm.y == ey, "after reactivation CMouse follows the pointer from where WM_MOUSEMOVE left it", Fmt("(%d,%d)", cm.x, cm.y));
	bf_input_mouse_acquire(0);
}

void Resize()
{
	bf_input_set_scale_setting(0);
	Run();
	bf_app_window_set_content_size(1280, 800);
	bf_app_pump(50);
	std::vector<Rec> v = Run();
	bf_view_metrics m;
	bf_view_get(&m);
	int w = 0, h = 0;
	bf_input_client_size(&w, &h);
	const Rec *sz = nullptr;
	for (const Rec &r : v)
		if (r.msg == WM_SIZE)
			sz = &r;
	Check(sz && LOWORD(sz->l) == m.logical_w && HIWORD(sz->l) == m.logical_h && w == m.logical_w && h == m.logical_h
	      ,
	      "resize to 1280x800 pt: WM_SIZE and bf_input_client_size give the new logical size",
	      Fmt("logical %dx%d at scale %d", m.logical_w, m.logical_h, m.ui_scale));
	bf_app_window_set_content_size(1024, 768);
	bf_app_pump(50);
	Run();
}

// The key state's corners.
void Remaining()
{
	Down(BF_KEY_RSHIFT);
	Run();
	bool shift = Held(VK_SHIFT) && Held(VK_RSHIFT) && !Held(VK_LSHIFT);
	Up(BF_KEY_RSHIFT);
	Run();
	Check(shift && !Held(VK_SHIFT), "key state: right Shift as VK_SHIFT and VK_RSHIFT");
	unsigned scan = 0;
	int ext = 0;
	Check(bf_input_vk_for_key(BF_KEY_KP_ENTER, &scan, &ext) == VK_RETURN && scan == 0x1c && ext == 1, "keypad Enter: VK_RETURN, extended");
	Check(bf_input_is_alt_shortcut('A') && !bf_input_is_alt_shortcut('F') && bf_input_is_alt_shortcut(VK_INSERT),
	      "Alt shortcuts: A and Insert are the game's, F is not");
	bool none = true;
	for (int vk = 1; vk < 256 && none; vk++)
		if (vk != VK_NUMLOCK && (bf_input_key_state(vk) & 0x8000))
			none = false;
	Check(none, "nothing held after the tests");
}

}  // namespace

int main(int argc, char **argv)
{
	setvbuf(stdout, nullptr, _IOLBF, 0);
	char id[64];
	snprintf(id, sizeof id, "input-test-%d", (int)getpid());
	bf_app_config c = {id, 1, 0};
	if (bf_app_init(&c) != BF_APP_OK) {
		printf("FAIL bf_app_init\n");
		return 1;
	}

	// InitApp's sequence.
	Check(bf_input_window() == NULL, "no window before InitApp");
	g_hwnd = bf_input_window_create(1024, 768, "Bifrost test \xe9", WindowProc);
	Check(g_hwnd != NULL && g_created && bf_input_window() == g_hwnd, "bf_input_window_create: window made, WM_CREATE sent");
	if (!g_hwnd)
		return 1;
	Check(Find(g_recs, WM_NCCREATE) && Find(g_recs, WM_NCCREATE) < Find(g_recs, WM_CREATE), "WM_NCCREATE before WM_CREATE");
	Check(bf_input_window_create(640, 480, "second", WindowProc) == NULL, "a second window is not made");
	bf_view_metrics m;
	bf_view_get(&m);
	int w = 0, h = 0;
	bf_input_client_size(&w, &h);
	Check(w == m.logical_w && h == m.logical_h && m.logical_w > 0,
	      "bf_input_client_size: the logical size", Fmt("%dx%d, UI scale %d, backing %.0f", w, h, m.ui_scale, m.backing_scale));
	Run();

	Keyboard();
	OptionLayout();
	Clipboard();
	Mouse();
	Resize();
	Remaining();

	bf_app_event u = {};
	u.type = BF_APP_EVENT_USER;
	bf_app_post_event(&u);
	std::vector<Rec> v = Run();
	Check(v.empty(), "a USER event makes no message");

	bf_input_post(WM_USER + 1, 1, 2);
	v = Run();
	Check(v.size() == 1 && v[0].msg == WM_USER + 1 && v[0].w == 1 && v[0].l == 2, "bf_input_post reaches the window procedure");
	bf_input_msg msg = {WM_USER + 2, 3, 4};
	g_recs.clear();
	bf_input_dispatch(&msg);
	Check(g_recs.size() == 1 && g_recs[0].msg == WM_USER + 2, "bf_input_dispatch calls it directly");

	bf_input_window_destroy();
	Check(g_destroyed == 1 && bf_input_window() == NULL, "bf_input_window_destroy: WM_DESTROY sent, window gone");
	bf_app_shutdown();
	printf("%s: %d failed\n", g_failures ? "FAIL" : "PASS", g_failures);
	return g_failures ? 1 : 0;
}
