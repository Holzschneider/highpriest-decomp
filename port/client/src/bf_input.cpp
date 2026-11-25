// The client's window, event pump, keyboard, mouse, cursor and clipboard on
// port/native/app and port/native/view (bf_input.h, M11.7). The rules are
// those of the input adapter of stage A (port/compat/input, M5.1, M5.2),
// moved here unchanged when M11 deleted it: the key table (USB HID usage to
// virtual key, set-1 scan code, extended flag), Option as Alt, the WM_CHAR
// rules, the Mac meanings of Cmd, the message pump's order, the mouse in
// logical pixels with right-drag, the clipboard in CP1252.
// port/client/src/README.md ("bf_input") documents them.
//
// Compiled with bf_compat_flags: the Windows names here are the message ids,
// virtual keys and their parameters (WM_*, VK_*, HTCLIENT, WA_*, SIZE_*,
// MK_*, WHEEL_DELTA), numbers the client keeps as its own.
#include <windows.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <deque>
#include <string>

#include "bf_app.h"
#include "bf_input.h"
#include "bf_text.h"
#include "bf_trace.h"
#include "bf_view.h"

namespace {

// ---------------------------------------------------------------- the key table

struct KeyInfo {
	unsigned vk;        // VK_* as WM_KEYDOWN's wParam (VK_SHIFT for both shifts, ...)
	unsigned state_vk;  // the entry GetKeyState keeps (VK_LSHIFT, VK_RSHIFT, ...): vk otherwise
	unsigned scan;      // set-1 scan code
	int extended;       // the E0 prefix: lParam bit 24
};

struct Table {
	KeyInfo k[BF_KEY_COUNT];
	Table()
	{
		memset(k, 0, sizeof k);
		auto set = [&](int key, unsigned vk, unsigned scan, int ext = 0, unsigned state_vk = 0) {
			k[key] = KeyInfo{vk, state_vk ? state_vk : vk, scan, ext};
		};
		static const unsigned letters[26] = {
			0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
			0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C};
		for (int i = 0; i < 26; i++)
			set(BF_KEY_A + i, 'A' + i, letters[i]);
		for (int i = 0; i < 9; i++)
			set(BF_KEY_1 + i, '1' + i, 0x02 + i);
		set(BF_KEY_0, '0', 0x0B);
		set(BF_KEY_RETURN, VK_RETURN, 0x1C);
		set(BF_KEY_ESCAPE, VK_ESCAPE, 0x01);
		set(BF_KEY_BACKSPACE, VK_BACK, 0x0E);
		set(BF_KEY_TAB, VK_TAB, 0x0F);
		set(BF_KEY_SPACE, VK_SPACE, 0x39);
		set(BF_KEY_MINUS, VK_OEM_MINUS, 0x0C);
		set(BF_KEY_EQUAL, VK_OEM_PLUS, 0x0D);
		set(BF_KEY_LEFTBRACKET, VK_OEM_4, 0x1A);
		set(BF_KEY_RIGHTBRACKET, VK_OEM_6, 0x1B);
		set(BF_KEY_BACKSLASH, VK_OEM_5, 0x2B);
		set(BF_KEY_SEMICOLON, VK_OEM_1, 0x27);
		set(BF_KEY_QUOTE, VK_OEM_7, 0x28);
		set(BF_KEY_GRAVE, VK_OEM_3, 0x29);
		set(BF_KEY_COMMA, VK_OEM_COMMA, 0x33);
		set(BF_KEY_PERIOD, VK_OEM_PERIOD, 0x34);
		set(BF_KEY_SLASH, VK_OEM_2, 0x35);
		set(BF_KEY_CAPSLOCK, VK_CAPITAL, 0x3A);
		for (int i = 0; i < 10; i++)
			set(BF_KEY_F1 + i, VK_F1 + i, 0x3B + i);
		set(BF_KEY_F11, VK_F11, 0x57);
		set(BF_KEY_F12, VK_F12, 0x58);
		set(BF_KEY_PRINTSCREEN, VK_SNAPSHOT, 0x37, 1);
		set(BF_KEY_SCROLLLOCK, VK_SCROLL, 0x46);
		set(BF_KEY_PAUSE, VK_PAUSE, 0x45);
		// kVK_Help, which a PC keyboard's Insert sends, arrives as BF_KEY_INSERT.
		set(BF_KEY_INSERT, VK_INSERT, 0x52, 1);
		set(BF_KEY_HOME, VK_HOME, 0x47, 1);
		set(BF_KEY_PAGEUP, VK_PRIOR, 0x49, 1);
		set(BF_KEY_DELETE, VK_DELETE, 0x53, 1);
		set(BF_KEY_END, VK_END, 0x4F, 1);
		set(BF_KEY_PAGEDOWN, VK_NEXT, 0x51, 1);
		set(BF_KEY_RIGHT, VK_RIGHT, 0x4D, 1);
		set(BF_KEY_LEFT, VK_LEFT, 0x4B, 1);
		set(BF_KEY_DOWN, VK_DOWN, 0x50, 1);
		set(BF_KEY_UP, VK_UP, 0x48, 1);
		set(BF_KEY_NUMLOCK, VK_NUMLOCK, 0x45, 1);
		set(BF_KEY_KP_DIVIDE, VK_DIVIDE, 0x35, 1);
		set(BF_KEY_KP_MULTIPLY, VK_MULTIPLY, 0x37);
		set(BF_KEY_KP_MINUS, VK_SUBTRACT, 0x4A);
		set(BF_KEY_KP_PLUS, VK_ADD, 0x4E);
		set(BF_KEY_KP_ENTER, VK_RETURN, 0x1C, 1);
		// A Mac keypad has no Num Lock state: its digits are always digits.
		static const unsigned kp[10] = {0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49};
		for (int i = 0; i < 9; i++)
			set(BF_KEY_KP_1 + i, VK_NUMPAD1 + i, kp[i]);
		set(BF_KEY_KP_0, VK_NUMPAD0, 0x52);
		set(BF_KEY_KP_PERIOD, VK_DECIMAL, 0x53);
		set(BF_KEY_NONUSBACKSLASH, VK_OEM_102, 0x56);
		set(BF_KEY_MENU, VK_APPS, 0x5D, 1);
		set(BF_KEY_KP_EQUAL, 0x92 /* VK_OEM_NEC_EQUAL */, 0x59);
		// A PC keyboard on a Mac sends Print Screen, Scroll Lock and Pause as
		// F13, F14 and F15; an Apple keyboard with a keypad has those keys
		// in the same place. They become what the PC key would have sent, so
		// that the game's screenshot keys work.
		set(BF_KEY_F13, VK_SNAPSHOT, 0x37, 1);
		set(BF_KEY_F14, VK_SCROLL, 0x46);
		set(BF_KEY_F15, VK_PAUSE, 0x45);
		for (int i = 0; i < 5; i++)
			set(BF_KEY_F16 + i, VK_F16 + i, 0x67 + i);
		set(BF_KEY_MUTE, VK_VOLUME_MUTE, 0x20, 1);
		set(BF_KEY_VOLUMEUP, VK_VOLUME_UP, 0x30, 1);
		set(BF_KEY_VOLUMEDOWN, VK_VOLUME_DOWN, 0x2E, 1);
		set(BF_KEY_LCTRL, VK_CONTROL, 0x1D, 0, VK_LCONTROL);
		set(BF_KEY_RCTRL, VK_CONTROL, 0x1D, 1, VK_RCONTROL);
		set(BF_KEY_LSHIFT, VK_SHIFT, 0x2A, 0, VK_LSHIFT);
		set(BF_KEY_RSHIFT, VK_SHIFT, 0x36, 0, VK_RSHIFT);
		set(BF_KEY_LALT, VK_MENU, 0x38, 0, VK_LMENU);
		set(BF_KEY_RALT, VK_MENU, 0x38, 1, VK_RMENU);
		// Cmd and Fn have entries for GetKeyState only; they send no messages.
		set(BF_KEY_LCMD, VK_LWIN, 0x5B, 1);
		set(BF_KEY_RCMD, VK_RWIN, 0x5C, 1);
	}
};

const KeyInfo &KeyFor(int key)
{
	static const Table t;
	static const KeyInfo none = {0, 0, 0, 0};
	if (key <= 0 || key >= BF_KEY_COUNT)
		return none;
	return t.k[key];
}

// ---------------------------------------------------------------- CP1252

// UTF-8 to CP1252 bytes, characters it does not hold dropped.
std::string Utf8ToCp1252(const char *utf8)
{
	std::string out;
	if (!utf8)
		return out;
	const unsigned char *p = (const unsigned char *)utf8;
	while (*p) {
		int len = *p < 0x80 ? 1 : *p < 0xC0 ? 1 : *p < 0xE0 ? 2 : *p < 0xF0 ? 3 : 4;
		int have = 1;
		while (have < len && p[have])
			have++;
		char b[8];
		int n = bf_text_from_utf8(BF_TEXT_CP_WESTERN, (const char *)p, have, b, sizeof b);
		// A character 1252 does not hold comes back as '?': drop it.
		if (n == 1 && !(b[0] == '?' && !(have == 1 && *p == '?')))
			out += b[0];
		p += have;
	}
	return out;
}

std::string Cp1252ToUtf8(const char *bytes, size_t len)
{
	std::string out;
	if (!bytes || !len)
		return out;
	int n = bf_text_to_utf8(BF_TEXT_CP_WESTERN, bytes, (int)len, nullptr, 0);
	if (n <= 0)
		return out;
	out.resize(n);
	bf_text_to_utf8(BF_TEXT_CP_WESTERN, bytes, (int)len, &out[0], n);
	return out;
}

std::string ClipboardUtf8()
{
	char probe[1];
	size_t n = bf_app_clipboard_get_text(probe, sizeof probe);
	if (n == 0)
		return "";
	std::string s(n + 1, '\0');
	bf_app_clipboard_get_text(&s[0], s.size());
	s.resize(strlen(s.c_str()));
	return s;
}

// ---------------------------------------------------------------- state

const int kWindowToken = 0;   // its address is the window the client holds
HWND const kWindow = (HWND)&kWindowToken;

int g_scale_setting = BF_VIEW_SCALE_AUTO;
bool g_view_running = false;
bool g_window = false;
bf_input_proc g_proc = nullptr;

std::deque<bf_input_msg> g_pending;   // translated messages and posted ones, in order
bool g_pumped = false;                // bf_app_pump has run since bf_input_peek last said "none"
uint32_t g_last_mods = 0;

uint8_t g_vk[256];                    // held, by the state VK (VK_LSHIFT, ..., 'A')
uint16_t g_down_as[BF_KEY_COUNT];     // the key whose entry a held key's WM_KEYDOWN used, 0: none sent
int g_cmd_ctrl_key = 0;               // Cmd+X or Cmd+C held: reads as Ctrl+X, Ctrl+C

// Whether the text a key-down types reaches the client (by the key-down's seq).
struct TextRule {
	uint64_t seq;
	bool drop;
};
TextRule g_text_rules[32];
unsigned g_text_rule_next = 0;

void SetTextRule(uint64_t seq, bool drop)
{
	for (TextRule &r : g_text_rules)
		if (r.seq == seq) {
			r.drop = drop;
			return;
		}
	g_text_rules[g_text_rule_next++ % 32] = TextRule{seq, drop};
}

bool TextDropped(uint64_t key_seq)
{
	if (!key_seq)
		return false;   // injected text, input methods
	for (const TextRule &r : g_text_rules)
		if (r.seq == key_seq)
			return r.drop;
	return false;
}

// The mouse.
bool g_acquired = false;
double g_rest_x = 0, g_rest_y = 0;   // fractions of a logical pixel not yet reported (right-drag)
double g_rest_wheel = 0;             // fractions of WHEEL_DELTA not yet reported

// ---------------------------------------------------------------- helpers

// The view metrics in effect: bf_view_get once the view runs, else computed
// from the window (or 640 x 480 at scale 1 without one).
void Metrics(bf_view_metrics *out)
{
	bf_view_get(out);
	if (out->serial != 0)
		return;
	bf_app_size s = {};
	if (g_window)
		bf_app_window_get_size(&s);
	if (s.width_px > 0 && s.height_px > 0 && s.scale > 0)
		bf_view_compute(s.width_px, s.height_px, s.scale, g_scale_setting, out);
	else
		bf_view_compute(640, 480, 1.0, 1, out);
}

// The pointer's logical position, not clamped.
void PointerLogical(int *x, int *y)
{
	bf_app_pointer p;
	bf_app_pointer_get(&p);
	bf_view_metrics m;
	Metrics(&m);
	double lx, ly;
	bf_view_px_to_logical(&m, p.x_px, p.y_px, &lx, &ly);
	*x = (int)floor(lx);
	*y = (int)floor(ly);
}

LPARAM PointLParam(int x, int y)
{
	return (LPARAM)(DWORD)MAKELONG((WORD)(short)x, (WORD)(short)y);
}

void Post(UINT message, WPARAM wparam, LPARAM lparam)
{
	g_pending.push_back(bf_input_msg{message, wparam, lparam});
}

bool StateDown(unsigned vk)
{
	return vk < 256 && g_vk[vk];
}

bool VkDown(int vk)
{
	switch (vk) {
	case VK_LBUTTON: case VK_RBUTTON: case VK_MBUTTON: case VK_XBUTTON1: case VK_XBUTTON2: {
		bf_app_pointer p;
		bf_app_pointer_get(&p);
		int bit = vk == VK_LBUTTON ? BF_APP_BUTTON_LEFT : vk == VK_RBUTTON ? BF_APP_BUTTON_RIGHT
		        : vk == VK_MBUTTON ? BF_APP_BUTTON_MIDDLE : vk == VK_XBUTTON1 ? BF_APP_BUTTON_X1
		        : BF_APP_BUTTON_X2;
		return (p.buttons >> bit) & 1;
	}
	case VK_SHIFT:
		return StateDown(VK_LSHIFT) || StateDown(VK_RSHIFT);
	case VK_CONTROL:
		return StateDown(VK_LCONTROL) || StateDown(VK_RCONTROL) || g_cmd_ctrl_key;
	case VK_LCONTROL:
		return StateDown(VK_LCONTROL) || g_cmd_ctrl_key;
	case VK_MENU:
		return StateDown(VK_LMENU) || StateDown(VK_RMENU);
	default:
		return vk > 0 && vk < 256 && g_vk[vk];
	}
}

int VkToggled(int vk)
{
	if (vk == VK_CAPITAL)
		return (g_last_mods & BF_APP_MOD_CAPSLOCK) ? 1 : 0;
	if (vk == VK_NUMLOCK)
		return 1;   // a Mac keypad always types digits
	return 0;
}

bool IsModifierKey(int key)
{
	return key >= BF_KEY_LCTRL && key <= BF_KEY_RCMD;
}

bool IsSysKey(unsigned vk, bool alt, bool ctrl)
{
	// Windows: WM_SYSKEY* while Alt is held without Ctrl, for F10, and for Alt itself.
	return (alt && !ctrl) || vk == VK_F10 || (vk == VK_MENU && !ctrl);
}

// The WM_KEYDOWN / WM_SYSKEYDOWN for a key, sent with the entry of `as`.
void SendKeyDown(int key, int as, bool repeat, bool alt, bool ctrl)
{
	const KeyInfo &k = KeyFor(as);
	if (!k.vk)
		return;
	if (k.vk == VK_MENU)
		alt = true;   // Alt's own key-down has the context bit, as on Windows
	bool prev = StateDown(k.state_vk) || repeat;
	bool sys = IsSysKey(k.vk, alt, ctrl);
	LPARAM lp = 1 | ((LPARAM)k.scan << 16) | ((LPARAM)(k.extended ? 1 : 0) << 24)
	          | ((LPARAM)(sys && alt ? 1 : 0) << 29) | ((LPARAM)(prev ? 1 : 0) << 30);
	g_vk[k.state_vk] = 1;
	g_down_as[key] = (uint16_t)as;
	// Windows sends Print Screen as WM_KEYUP only (the key-down is the
	// system's); the client's WndProc takes the screenshot there. A key-down
	// as well would take a second picture when a frame falls in between.
	if (k.vk == VK_SNAPSHOT)
		return;
	Post(sys ? WM_SYSKEYDOWN : WM_KEYDOWN, k.vk, lp);
}

void PasteAsTyped()
{
	std::string bytes = Utf8ToCp1252(ClipboardUtf8().c_str());
	int n = 0;
	for (unsigned char c : bytes)
		if (c >= 0x20 && c != 0x7F) {
			Post(WM_CHAR, c, 1);
			n++;
		}
	BF_TRACE(BFT_INPUT, "input: Cmd+V typed %d characters from the clipboard\n", n);
}

void OnKeyDown(const bf_app_event &e)
{
	int key = e.key.key;
	bool alt = e.mods & BF_APP_MOD_ALT, ctrl = e.mods & BF_APP_MOD_CTRL, cmd = e.mods & BF_APP_MOD_CMD;
	bool repeat = e.key.repeat != 0;
	SetTextRule(e.seq, true);
	if (key == BF_KEY_NONE || key == BF_KEY_FN)
		return;
	if (key == BF_KEY_LCMD || key == BF_KEY_RCMD) {
		g_vk[KeyFor(key).state_vk] = 1;   // GetKeyState only; no message
		return;
	}
	if (cmd && !IsModifierKey(key)) {
		// Mac meanings first (spec section 4). AppKit takes Cmd+Q and Cmd+F
		// through the menu and queues QUIT or toggles fullscreen itself;
		// these key-downs come from the control socket.
		switch (key) {
		case BF_KEY_Q:
			if (!repeat)
				Post(WM_CLOSE, 0, 0);
			return;
		case BF_KEY_F:
			if (!repeat)
				bf_view_toggle_fullscreen();
			return;
		case BF_KEY_V:
			PasteAsTyped();
			return;
		case BF_KEY_X:
		case BF_KEY_C:
			// CLanguage::OnKeyDown cuts and copies on Ctrl+X, Ctrl+C.
			g_cmd_ctrl_key = key;
			SendKeyDown(key, key, repeat, alt, true);
			return;
		case BF_KEY_I:
			// The Insert substitute (sit): Cmd+I, Cmd+Option+I for Alt+Insert.
			SendKeyDown(key, BF_KEY_INSERT, repeat, alt, ctrl);
			return;
		case BF_KEY_S:
			// The Print Screen substitute (the game's screenshot key) for
			// Apple keyboards without F13: Cmd+Shift+S; with Ctrl, as
			// Ctrl+Print Screen, a BMP. Cmd+S alone stays the system's.
			if (e.mods & BF_APP_MOD_SHIFT)
				SendKeyDown(key, BF_KEY_PRINTSCREEN, repeat, alt, ctrl);
			return;
		default:
			return;   // Cmd+H, Cmd+M, ...: the system's
		}
	}
	const KeyInfo &k = KeyFor(key);
	if (!k.vk) {
		BF_TRACE(BFT_INPUT, "input: key %d (kVK 0x%x) has no virtual key\n", key, e.key.mac_keycode);
		return;
	}
	SendKeyDown(key, key, repeat, alt, ctrl);
	// Text reaches the client unless Ctrl is held, or Option is held and the
	// key is a game shortcut (Option acts as Alt and types nothing then).
	SetTextRule(e.seq, ctrl || (alt && bf_input_is_alt_shortcut(k.vk)));
}

void OnKeyUp(const bf_app_event &e)
{
	int key = e.key.key;
	if (key == BF_KEY_LCMD || key == BF_KEY_RCMD) {
		g_vk[KeyFor(key).state_vk] = 0;
		if (!StateDown(VK_LWIN) && !StateDown(VK_RWIN))
			g_cmd_ctrl_key = 0;
		return;
	}
	if (key <= 0 || key >= BF_KEY_COUNT || !g_down_as[key])
		return;   // its key-down was not sent (a Mac command, or none seen)
	const KeyInfo &k = KeyFor(g_down_as[key]);
	g_down_as[key] = 0;
	g_vk[k.state_vk] = 0;
	bool alt = VkDown(VK_MENU), ctrl = VkDown(VK_CONTROL);
	bool sys = IsSysKey(k.vk, alt, ctrl);
	LPARAM lp = 1 | ((LPARAM)k.scan << 16) | ((LPARAM)(k.extended ? 1 : 0) << 24)
	          | ((LPARAM)(sys && alt ? 1 : 0) << 29) | (LPARAM)0xC0000000u;
	Post(sys ? WM_SYSKEYUP : WM_KEYUP, k.vk, lp);
	if (key == g_cmd_ctrl_key)
		g_cmd_ctrl_key = 0;
}

void OnText(const bf_app_event &e)
{
	if (TextDropped(e.text.key_seq))
		return;
	std::string bytes = Utf8ToCp1252(e.text.utf8);
	for (unsigned char c : bytes)
		Post(WM_CHAR, c, 1);
}

void ReleaseAllKeys()
{
	memset(g_vk, 0, sizeof g_vk);
	memset(g_down_as, 0, sizeof g_down_as);
	g_cmd_ctrl_key = 0;
}

WPARAM MouseKeyFlags(uint32_t buttons)
{
	WPARAM w = 0;
	if (buttons & (1u << BF_APP_BUTTON_LEFT)) w |= MK_LBUTTON;
	if (buttons & (1u << BF_APP_BUTTON_RIGHT)) w |= MK_RBUTTON;
	if (buttons & (1u << BF_APP_BUTTON_MIDDLE)) w |= MK_MBUTTON;
	if (VkDown(VK_SHIFT)) w |= MK_SHIFT;
	if (VkDown(VK_CONTROL)) w |= MK_CONTROL;
	return w;
}

// One event of the app queue as the messages the client's window procedure
// handled on Windows.
void Translate(const bf_app_event &e)
{
	if (e.type == BF_APP_EVENT_KEY_DOWN || e.type == BF_APP_EVENT_KEY_UP || e.type == BF_APP_EVENT_TEXT
	    || e.type == BF_APP_EVENT_POINTER_MOVE)
		g_last_mods = e.mods;
	if (!g_window)
		return;
	switch (e.type) {
	case BF_APP_EVENT_KEY_DOWN:
		OnKeyDown(e);
		break;
	case BF_APP_EVENT_KEY_UP:
		OnKeyUp(e);
		break;
	case BF_APP_EVENT_TEXT:
		OnText(e);
		break;
	case BF_APP_EVENT_POINTER_MOVE: {
		int x, y;
		PointerLogical(&x, &y);
		bf_app_pointer p;
		bf_app_pointer_get(&p);
		if (p.inside)
			Post(WM_SETCURSOR, (WPARAM)kWindow, MAKELONG(HTCLIENT, WM_MOUSEMOVE));
		// The client reads WM_MOUSEMOVE only while inactive (CMouse otherwise).
		Post(WM_MOUSEMOVE, MouseKeyFlags(p.buttons), PointLParam(x, y));
		break;
	}
	case BF_APP_EVENT_POINTER_ENTER:
		Post(WM_SETCURSOR, (WPARAM)kWindow, MAKELONG(HTCLIENT, WM_MOUSEMOVE));
		break;
	case BF_APP_EVENT_RESIZE:
	case BF_APP_EVENT_FULLSCREEN: {
		// The resize path of M4.3: the view resizes the device and calls its
		// listener; WM_SIZE follows with the logical size, for the record.
		if (g_view_running)
			bf_view_handle_event(&e);
		bf_view_metrics m;
		Metrics(&m);
		Post(WM_SIZE, m.fullscreen ? SIZE_MAXIMIZED : SIZE_RESTORED, PointLParam(m.logical_w, m.logical_h));
		break;
	}
	case BF_APP_EVENT_FOCUS:
		if (e.value) {
			Post(WM_ACTIVATEAPP, TRUE, 0);
			Post(WM_NCACTIVATE, TRUE, 0);
			Post(WM_ACTIVATE, WA_ACTIVE, 0);
			Post(WM_SETFOCUS, 0, 0);
		} else {
			// The app module has queued the key-ups before this; whatever
			// was missed is let go here.
			ReleaseAllKeys();
			Post(WM_NCACTIVATE, FALSE, 0);
			Post(WM_ACTIVATE, WA_INACTIVE, 0);
			Post(WM_ACTIVATEAPP, FALSE, 0);
			Post(WM_KILLFOCUS, 0, 0);
		}
		break;
	case BF_APP_EVENT_QUIT:
		BF_TRACE(BFT_INPUT, "input: quit (reason %d) -> WM_CLOSE\n", e.value);
		Post(WM_CLOSE, 0, 0);
		break;
	default:
		break;   // BUTTON_*, WHEEL (the mouse reads them), VISIBILITY (the client keeps drawing), REOPEN, USER
	}
}

LRESULT Send(UINT message, WPARAM wparam, LPARAM lparam)
{
	if (!g_window || !g_proc)
		return 0;
	return g_proc(kWindow, message, wparam, lparam);
}

int Clamp(int v, int lo, int hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

}  // namespace

// ------------------------------------------------------------------ window

extern "C" void bf_input_set_scale_setting(int setting)
{
	g_scale_setting = setting;
	if (g_view_running)
		bf_view_set_scale_setting(setting);
}

extern "C" HWND bf_input_window_create(int width, int height, const char *title, bf_input_proc proc)
{
	if (g_window) {
		bf_tracef(BFT_ERR, "input: the client has one window; a second is not made\n");
		return nullptr;
	}
	// The size is the client area: logical pixels, made the content size in
	// points, which the UI scale and the backing scale then turn into the
	// logical size bf_view reports.
	if (width <= 0)
		width = 640;
	if (height <= 0)
		height = 480;
	std::string name = title ? Cp1252ToUtf8(title, strlen(title)) : std::string();
	if (bf_app_window_create(width, height, name.c_str()) != BF_APP_OK) {
		bf_tracef(BFT_ERR, "input: bf_app_window_create failed\n");
		return nullptr;
	}
	memset(g_vk, 0, sizeof g_vk);
	g_view_running = bf_view_init(g_scale_setting) == BF_VIEW_OK;
	g_window = true;
	g_proc = proc;
	bf_app_set_cursor_hidden_in_window(0);   // the class cursor: the arrow
	bf_view_metrics m;
	Metrics(&m);
	BF_TRACE(BFT_INPUT, "input: window %dx%d pt, logical %dx%d at UI scale %d\n", width, height, m.logical_w,
	         m.logical_h, m.ui_scale);
	if (!Send(WM_NCCREATE, 0, 0) || Send(WM_CREATE, 0, 0) == -1) {
		bf_input_window_destroy();
		return nullptr;
	}
	return kWindow;
}

extern "C" void bf_input_window_destroy(void)
{
	if (!g_window)
		return;
	Send(WM_DESTROY, 0, 0);
	Send(WM_NCDESTROY, 0, 0);
	g_pending.clear();
	if (g_view_running)
		bf_view_shutdown();
	g_view_running = false;
	bf_app_window_destroy();
	g_window = false;
	g_proc = nullptr;
}

extern "C" HWND bf_input_window(void)
{
	return g_window ? kWindow : nullptr;
}

extern "C" void bf_input_client_size(int *width, int *height)
{
	bf_view_metrics m;
	Metrics(&m);
	if (width)
		*width = m.logical_w;
	if (height)
		*height = m.logical_h;
}

// ------------------------------------------------------------------ messages

extern "C" int bf_input_peek(bf_input_msg *msg)
{
	for (;;) {
		if (!g_pending.empty()) {
			bf_input_msg m = g_pending.front();
			g_pending.pop_front();
			if (m.message == WM_KEYDOWN) {
				// What TranslateMessage typed that AppKit reports as no text:
				// the control characters CLanguage::OnChar and the edit
				// controls handle, right behind their key-down.
				int c = 0;
				switch (m.wParam) {
				case VK_BACK: c = 8; break;
				case VK_TAB: c = 9; break;
				case VK_RETURN: c = 13; break;
				case VK_ESCAPE: c = 27; break;
				}
				if (c && !VkDown(VK_CONTROL))
					g_pending.push_front(bf_input_msg{WM_CHAR, (WPARAM)c, m.lParam});
			}
			if (msg)
				*msg = m;
			return 1;
		}
		// One event at a time, so that the key and pointer state the client
		// reads while handling a message is the state of that message.
		bf_app_event e;
		if (bf_app_poll_event(&e)) {
			Translate(e);
			continue;
		}
		if (!g_pumped) {
			g_pumped = true;
			bf_app_pump(0);
			continue;
		}
		g_pumped = false;
		return 0;
	}
}

extern "C" LRESULT bf_input_dispatch(const bf_input_msg *msg)
{
	if (!msg)
		return 0;
	return Send(msg->message, msg->wParam, msg->lParam);
}

extern "C" void bf_input_post(UINT message, WPARAM wparam, LPARAM lparam)
{
	Post(message, wparam, lparam);
}

extern "C" LRESULT bf_input_default_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message) {
	case WM_NCCREATE:
	case WM_NCACTIVATE:
		return TRUE;
	default:
		return 0;
	}
}

// ------------------------------------------------------------------ keys

extern "C" int bf_input_key_state(int vk)
{
	return (VkDown(vk) ? 0x8000 : 0) | VkToggled(vk);
}

extern "C" int bf_input_async_key_state(int vk)
{
	return VkDown(vk) ? 0x8000 : 0;
}

extern "C" unsigned bf_input_vk_for_key(int key, unsigned *scan, int *extended)
{
	const KeyInfo &k = KeyFor(key);
	if (scan)
		*scan = k.scan;
	if (extended)
		*extended = k.extended;
	return k.vk;
}

extern "C" int bf_input_is_alt_shortcut(unsigned vk)
{
	// UIWindowMgr::ProcessPushButton, the cases that test VK_MENU: the
	// basic-info window's buttons and the other windows (A-Z but F, K, N),
	// the shortcut bar, X (quit), Insert (compass), Home (attribute
	// indicator), End (HP bar), F10 (chat window). U is a shortcut on the
	// Korean service only, but is kept here for all.
	switch (vk) {
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'G': case 'H': case 'I':
	case 'J': case 'L': case 'M': case 'O': case 'P': case 'Q': case 'R': case 'S':
	case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
	case VK_INSERT: case VK_HOME: case VK_END: case VK_F10:
		return 1;
	default:
		return 0;
	}
}

// ------------------------------------------------------------------ mouse

extern "C" void bf_input_mouse_acquire(int acquire)
{
	if (acquire && !g_acquired) {
		bf_app_pointer_take_delta(nullptr, nullptr, nullptr);   // what happened before is not reported
		g_rest_x = g_rest_y = g_rest_wheel = 0;
	}
	g_acquired = acquire != 0;
}

extern "C" int bf_input_mouse_read(int x, int y, bf_input_mouse_state *s)
{
	float dx = 0, dy = 0, wheel = 0;
	bf_app_pointer_take_delta(&dx, &dy, &wheel);   // always drained, so nothing piles up
	if (!g_acquired)
		return BF_INPUT_NOT_ACQUIRED;
	memset(s, 0, sizeof *s);

	bf_view_metrics m;
	Metrics(&m);
	int scale = m.ui_scale > 0 ? m.ui_scale : 1;
	bf_app_pointer p;
	bf_app_pointer_get(&p);
	if (p.buttons & (1u << BF_APP_BUTTON_RIGHT)) {
		// Right-drag: the device's movement, in logical pixels.
		g_rest_x += dx / scale;
		g_rest_y += dy / scale;
		s->lX = (int)trunc(g_rest_x);
		s->lY = (int)trunc(g_rest_y);
		g_rest_x -= s->lX;
		g_rest_y -= s->lY;
	} else {
		// To the logical pixel under the pointer, from where the client
		// holds its sprite (CMouse then clamps to the screen).
		int lx, ly;
		bf_view_px_to_logical_pixel(&m, p.x_px, p.y_px, &lx, &ly);
		s->lX = lx - Clamp(x, 0, m.logical_w - 1);
		s->lY = ly - Clamp(y, 0, m.logical_h - 1);
		g_rest_x = g_rest_y = 0;
	}

	g_rest_wheel += (double)wheel * WHEEL_DELTA;
	s->lZ = (int)trunc(g_rest_wheel);
	g_rest_wheel -= s->lZ;

	static const int order[4] = {BF_APP_BUTTON_LEFT, BF_APP_BUTTON_RIGHT, BF_APP_BUTTON_MIDDLE, BF_APP_BUTTON_X1};
	for (int i = 0; i < 4; i++)
		s->rgbButtons[i] = ((p.buttons >> order[i]) & 1) ? 0x80 : 0;
	return 0;
}

extern "C" void bf_input_cursor_pos(int *x, int *y)
{
	int px, py;
	PointerLogical(&px, &py);
	if (x)
		*x = px;
	if (y)
		*y = py;
}

extern "C" void bf_input_hide_cursor(void)
{
	bf_app_set_cursor_hidden_in_window(1);
}

// ------------------------------------------------------------------ clipboard

extern "C" char *bf_input_clipboard_text(void)
{
	std::string bytes = Utf8ToCp1252(ClipboardUtf8().c_str());
	if (bytes.empty())
		return nullptr;
	// CF_TEXT ends lines with CR LF.
	std::string text;
	for (char c : bytes) {
		if (c == '\n' && (text.empty() || text.back() != '\r'))
			text += '\r';
		text += c;
	}
	char *out = (char *)malloc(text.size() + 1);
	if (out)
		memcpy(out, text.c_str(), text.size() + 1);
	return out;
}

extern "C" int bf_input_clipboard_set_text(const char *bytes, size_t len)
{
	std::string utf8 = Cp1252ToUtf8(bytes, bytes ? strnlen(bytes, len) : 0);
	// CR LF to the Mac's LF.
	std::string text;
	for (size_t i = 0; i < utf8.size(); i++)
		if (!(utf8[i] == '\r' && i + 1 < utf8.size() && utf8[i + 1] == '\n'))
			text += utf8[i];
	return bf_app_clipboard_set_text(text.c_str()) == BF_APP_OK;
}
