// user32 for the client: the one window, the message pump that turns the app
// module's event queue into window messages, keyboard state, cursor and
// clipboard. The translation rules are in port/native/app/README.md ("What
// the Win32 adapter must translate") and in this area's README.
#include "bf_compat_input_internal.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <deque>
#include <map>

#include "bf_trace.h"

namespace bfin {
namespace {

const unsigned kWindowMagic = 0x42465748;   // "BFWH"

// ---------------------------------------------------------------- start-up

std::string g_instance_id;
int g_single_instance = 1;
bool g_app_started = false;
int g_scale_setting = BF_VIEW_SCALE_AUTO;
bool g_view_running = false;

// ---------------------------------------------------------------- classes

struct Class {
	WNDPROC proc;
	HBRUSH background;
	HCURSOR cursor;
};
std::map<std::string, Class> g_classes;   // lower-case name
std::map<ATOM, std::string> g_atoms;
ATOM g_next_atom = 0xC000;

std::string Lower(const char *s)
{
	std::string out = s ? s : "";
	for (char &c : out)
		c = (char)tolower((unsigned char)c);
	return out;
}

// A class name argument: a string, or an atom (MAKEINTATOM).
std::string ClassKey(LPCSTR name)
{
	if (!name)
		return "";
	if (((uintptr_t)name >> 16) == 0) {
		auto it = g_atoms.find((ATOM)(uintptr_t)name);
		return it == g_atoms.end() ? "" : it->second;
	}
	return Lower(name);
}

// ---------------------------------------------------------------- the window

Window *g_window = nullptr;
bool g_shown = false;
bool g_active = true;     // as the client last heard (WM_ACTIVATE)

// ---------------------------------------------------------------- messages

std::deque<MSG> g_pending;   // translated messages and posted ones, in order
bool g_pumped = false;       // bf_app_pump has run since PeekMessage last said "none"
bf_compat_input_event_hook g_hook = nullptr;
void *g_hook_context = nullptr;
uint64_t g_event_time = 0;   // of the event being translated
uint32_t g_last_mods = 0;

// ---------------------------------------------------------------- keys

uint8_t g_vk[256];                 // held, by the state VK (VK_LSHIFT, ..., 'A')
uint16_t g_down_as[BF_KEY_COUNT];  // the key whose entry a held key's WM_KEYDOWN used, 0: none sent
int g_cmd_ctrl_key = 0;            // Cmd+X or Cmd+C held: reads as Ctrl+X, Ctrl+C

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

// ---------------------------------------------------------------- cursor

char g_arrow_cursor_object, g_icon_object;
HCURSOR const kArrowCursor = (HCURSOR)&g_arrow_cursor_object;
HICON const kIcon = (HICON)&g_icon_object;
HCURSOR g_cursor = kArrowCursor;
int g_show_count = 0;
bool g_cursor_override = false;   // SetCursorPos: what GetCursorPos reports until the pointer moves
POINT g_cursor_override_pos = {0, 0};

void ApplyCursor()
{
	bf_app_set_cursor_hidden_in_window(g_cursor == nullptr || g_show_count < 0);
}

// ---------------------------------------------------------------- clipboard

bool g_clipboard_open = false;
HGLOBAL g_clipboard_handle = nullptr;   // what GetClipboardData handed out; ours to free

void FreeClipboardHandle()
{
	if (g_clipboard_handle)
		GlobalFree(g_clipboard_handle);
	g_clipboard_handle = nullptr;
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

// ---------------------------------------------------------------- helpers

LPARAM PointLParam(int x, int y)
{
	return (LPARAM)(DWORD)MAKELONG((WORD)(short)x, (WORD)(short)y);
}

void Post(UINT message, WPARAM wparam, LPARAM lparam, bool front = false)
{
	MSG m = {};
	m.hwnd = (HWND)g_window;
	m.message = message;
	m.wParam = wparam;
	m.lParam = lparam;
	m.time = (DWORD)g_event_time;
	int x, y;
	PointerLogical(&x, &y);
	m.pt.x = x;
	m.pt.y = y;
	if (front)
		g_pending.push_front(m);
	else
		g_pending.push_back(m);
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
	SetTextRule(e.seq, ctrl || (alt && bf_compat_input_is_alt_shortcut(k.vk)));
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

void Translate(const bf_app_event &e)
{
	g_event_time = e.time_ms;
	if (e.type == BF_APP_EVENT_KEY_DOWN || e.type == BF_APP_EVENT_KEY_UP || e.type == BF_APP_EVENT_TEXT
	    || e.type == BF_APP_EVENT_POINTER_MOVE)
		g_last_mods = e.mods;
	if (g_hook && g_hook(&e, g_hook_context))
		return;
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
		g_cursor_override = false;
		int x, y;
		PointerLogical(&x, &y);
		bf_app_pointer p;
		bf_app_pointer_get(&p);
		if (p.inside)
			Post(WM_SETCURSOR, (WPARAM)g_window, MAKELONG(HTCLIENT, WM_MOUSEMOVE));
		// The client reads WM_MOUSEMOVE only while inactive (CMouse otherwise).
		Post(WM_MOUSEMOVE, MouseKeyFlags(p.buttons), PointLParam(x, y));
		break;
	}
	case BF_APP_EVENT_BUTTON_DOWN:
	case BF_APP_EVENT_BUTTON_UP:
	case BF_APP_EVENT_WHEEL:
		g_cursor_override = false;   // the DirectInput mouse reports them
		break;
	case BF_APP_EVENT_POINTER_ENTER:
		g_cursor_override = false;
		Post(WM_SETCURSOR, (WPARAM)g_window, MAKELONG(HTCLIENT, WM_MOUSEMOVE));
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
			g_active = true;
			Post(WM_ACTIVATEAPP, TRUE, 0);
			Post(WM_NCACTIVATE, TRUE, 0);
			Post(WM_ACTIVATE, WA_ACTIVE, 0);
			Post(WM_SETFOCUS, 0, 0);
		} else {
			// The app module has queued the key-ups before this; whatever
			// was missed is let go here.
			ReleaseAllKeys();
			g_active = false;
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
		break;   // VISIBILITY (the client keeps drawing), REOPEN, USER
	}
}

bool Matches(const MSG &m, HWND hwnd, UINT lo, UINT hi)
{
	if (hwnd && hwnd != (HWND)-1 && m.hwnd != hwnd)
		return false;
	if (hwnd == (HWND)-1 && m.hwnd)
		return false;
	if ((lo || hi) && (m.message < lo || m.message > hi))
		return false;
	return true;
}

BOOL Peek(MSG *out, HWND hwnd, UINT lo, UINT hi, UINT flags)
{
	EnsureApp();
	for (;;) {
		for (auto it = g_pending.begin(); it != g_pending.end(); ++it) {
			if (!Matches(*it, hwnd, lo, hi))
				continue;
			if (out)
				*out = *it;
			if (flags & PM_REMOVE)
				g_pending.erase(it);
			return TRUE;
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
		return FALSE;
	}
}

LRESULT Send(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	Window *w = WindowFrom(hwnd);
	if (!w || !w->proc)
		return 0;
	return w->proc(hwnd, message, wparam, lparam);
}

}  // namespace

// ------------------------------------------------------------------ shared

void EnsureApp()
{
	if (g_app_started)
		return;
	g_app_started = true;
	memset(g_vk, 0, sizeof g_vk);
	bf_app_config c = {};
	c.instance_id = g_instance_id.empty() ? nullptr : g_instance_id.c_str();
	c.single_instance = g_single_instance;
	int r = bf_app_init(&c);
	if (r == BF_APP_ALREADY_RUNNING) {
		// FindWindowA's single-instance test: the running client has been
		// brought to the front; this one leaves at once.
		bf_tracef(BFT_INPUT, "input: another instance is running and was brought to the front; exiting\n");
		exit(0);
	}
	if (r != BF_APP_OK)
		bf_tracef(BFT_ERR, "input: bf_app_init failed (%d)\n", r);
}

Window *WindowFrom(HWND hwnd)
{
	Window *w = (Window *)hwnd;
	return w && w == g_window && w->magic == kWindowMagic ? w : nullptr;
}

HWND MainWindow()
{
	return (HWND)g_window;
}

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

void PointerLogical(int *x, int *y)
{
	if (g_cursor_override) {
		*x = g_cursor_override_pos.x;
		*y = g_cursor_override_pos.y;
		return;
	}
	bf_app_pointer p;
	bf_app_pointer_get(&p);
	bf_view_metrics m;
	Metrics(&m);
	double lx, ly;
	bf_view_px_to_logical(&m, p.x_px, p.y_px, &lx, &ly);
	*x = (int)floor(lx);
	*y = (int)floor(ly);
}

}  // namespace bfin

using namespace bfin;

// ------------------------------------------------------------------ port side

extern "C" void bf_compat_input_configure(const char *instance_id, int single_instance)
{
	g_instance_id = instance_id ? instance_id : "";
	g_single_instance = single_instance;
}

extern "C" void bf_compat_input_set_scale_setting(int setting)
{
	g_scale_setting = setting;
	if (g_view_running)
		bf_view_set_scale_setting(setting);
}

extern "C" void bf_compat_input_set_event_hook(bf_compat_input_event_hook hook, void *context)
{
	g_hook = hook;
	g_hook_context = context;
}

extern "C" HWND bf_compat_input_main_window(void)
{
	return (HWND)g_window;
}

extern "C" {

// ------------------------------------------------------------------ classes

ATOM WINAPI RegisterClassA(const WNDCLASSA *wc)
{
	EnsureApp();
	if (!wc || !wc->lpszClassName || !wc->lpfnWndProc)
		return 0;
	std::string key = ClassKey(wc->lpszClassName);
	if (key.empty() || g_classes.count(key))
		return 0;   // ERROR_CLASS_ALREADY_EXISTS
	g_classes[key] = Class{wc->lpfnWndProc, wc->hbrBackground, wc->hCursor};
	ATOM atom = g_next_atom++;
	g_atoms[atom] = key;
	return atom;
}

ATOM WINAPI RegisterClassExA(const WNDCLASSEXA *wc)
{
	if (!wc || wc->cbSize != sizeof(WNDCLASSEXA))
		return 0;
	WNDCLASSA a = {wc->style, wc->lpfnWndProc, wc->cbClsExtra, wc->cbWndExtra, wc->hInstance,
	               wc->hIcon, wc->hCursor, wc->hbrBackground, wc->lpszMenuName, wc->lpszClassName};
	return RegisterClassA(&a);
}

HICON WINAPI LoadIconA(HINSTANCE instance, LPCSTR name)
{
	return kIcon;   // the bundle's icon is the Dock's (M12)
}

HCURSOR WINAPI LoadCursorA(HINSTANCE instance, LPCSTR name)
{
	return kArrowCursor;   // any cursor is the system arrow
}

// ------------------------------------------------------------------ the window

HWND WINAPI CreateWindowExA(DWORD ex_style, LPCSTR class_name, LPCSTR title, DWORD style, INT x, INT y,
                            INT width, INT height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
{
	EnsureApp();
	auto it = g_classes.find(ClassKey(class_name));
	if (it == g_classes.end()) {
		bf_tracef(BFT_ERR, "input: CreateWindowExA: class '%s' is not registered\n",
		          ((uintptr_t)class_name >> 16) ? class_name : "(atom)");
		return nullptr;
	}
	if (g_window) {
		bf_tracef(BFT_ERR, "input: CreateWindowExA: the client has one window; a second is not made\n");
		return nullptr;
	}
	// The size is the client area (AdjustWindowRect adds nothing): logical
	// pixels, made the content size in points, which the UI scale and the
	// backing scale then turn into the logical size bf_view reports.
	if (width <= 0 || width == CW_USEDEFAULT)
		width = 640;
	if (height <= 0 || height == CW_USEDEFAULT)
		height = 480;
	std::string name = title ? Cp1252ToUtf8(title, strlen(title)) : std::string();
	if (bf_app_window_create(width, height, name.c_str()) != BF_APP_OK) {
		bf_tracef(BFT_ERR, "input: CreateWindowExA: bf_app_window_create failed\n");
		return nullptr;
	}
	g_view_running = bf_view_init(g_scale_setting) == BF_VIEW_OK;
	Window *w = new Window();
	w->magic = kWindowMagic;
	w->proc = it->second.proc;
	w->style = (LONG)style;
	w->ex_style = (LONG)ex_style;
	w->user_data = 0;
	w->class_name = it->first;
	w->background = 0;
	if (it->second.background && !bf_compat_gdi_brush_color(it->second.background, &w->background))
		w->background = 0;
	g_window = w;
	g_cursor = it->second.cursor ? it->second.cursor : kArrowCursor;
	ApplyCursor();
	g_active = true;
	HWND hwnd = (HWND)w;
	bf_view_metrics m;
	Metrics(&m);
	BF_TRACE(BFT_INPUT, "input: window %dx%d pt, logical %dx%d at UI scale %d\n", width, height,
	         m.logical_w, m.logical_h, m.ui_scale);

	CREATESTRUCTA cs = {param, instance, menu, parent, m.logical_h, m.logical_w, 0, 0,
	                    (LONG)style, title, class_name, ex_style};
	if (!Send(hwnd, WM_NCCREATE, 0, (LPARAM)&cs) || Send(hwnd, WM_CREATE, 0, (LPARAM)&cs) == -1) {
		DestroyWindow(hwnd);
		return nullptr;
	}
	return hwnd;
}

BOOL WINAPI DestroyWindow(HWND hwnd)
{
	Window *w = WindowFrom(hwnd);
	if (!w)
		return FALSE;
	Send(hwnd, WM_DESTROY, 0, 0);
	Send(hwnd, WM_NCDESTROY, 0, 0);
	for (auto it = g_pending.begin(); it != g_pending.end();)
		it = it->hwnd == hwnd ? g_pending.erase(it) : it + 1;
	if (g_view_running)
		bf_view_shutdown();
	g_view_running = false;
	bf_app_window_destroy();
	g_window = nullptr;
	g_shown = false;
	delete w;
	return TRUE;
}

BOOL WINAPI IsWindow(HWND hwnd)
{
	return WindowFrom(hwnd) != nullptr;
}

HWND WINAPI FindWindowA(LPCSTR class_name, LPCSTR title)
{
	// InitApp's single-instance test. Another process holding the slot is
	// found by bf_app_init, which brings it to the front; this process then
	// exits (EnsureApp). Within the process, the window itself is found.
	if (g_window) {
		bool cls = !class_name || ClassKey(class_name) == g_window->class_name;
		if (cls)
			return (HWND)g_window;
	}
	EnsureApp();
	return nullptr;
}

BOOL WINAPI ShowWindow(HWND hwnd, INT cmd)
{
	if (!WindowFrom(hwnd))
		return FALSE;
	BOOL was = g_shown;
	switch (cmd) {
	case SW_HIDE:
	case SW_MINIMIZE:
	case SW_SHOWMINIMIZED:
	case SW_SHOWMINNOACTIVE:
	case SW_MAXIMIZE:
		// The window's state is the user's on macOS (the fullscreen path
		// and WM_ACTIVATE call these); bf_app shows the window at creation.
		BF_TRACE(BFT_INPUT, "input: ShowWindow(%d) ignored\n", cmd);
		break;
	default:
		g_shown = true;
		break;
	}
	return was;
}

BOOL WINAPI UpdateWindow(HWND hwnd)
{
	return WindowFrom(hwnd) != nullptr;
}

BOOL WINAPI ValidateRect(HWND hwnd, const RECT *rect)
{
	return TRUE;
}

BOOL WINAPI InvalidateRect(HWND hwnd, const RECT *rect, BOOL erase)
{
	return TRUE;
}

BOOL WINAPI RedrawWindow(HWND hwnd, const RECT *rect, HRGN region, UINT flags)
{
	return TRUE;
}

BOOL WINAPI DrawMenuBar(HWND hwnd)
{
	return TRUE;
}

BOOL WINAPI IsIconic(HWND hwnd)
{
	return FALSE;
}

BOOL WINAPI GetClientRect(HWND hwnd, LPRECT rect)
{
	if (!rect || !WindowFrom(hwnd))
		return FALSE;
	bf_view_metrics m;
	Metrics(&m);
	rect->left = rect->top = 0;
	rect->right = m.logical_w;
	rect->bottom = m.logical_h;
	return TRUE;
}

BOOL WINAPI GetWindowRect(HWND hwnd, LPRECT rect)
{
	// "Screen" coordinates are the client area's own: the window has no
	// frame in this model and sits at the origin (ClientToScreen is the identity).
	return GetClientRect(hwnd, rect);
}

BOOL WINAPI AdjustWindowRect(LPRECT rect, DWORD style, BOOL menu)
{
	return rect != nullptr;   // no frame: the window size is the client size
}

BOOL WINAPI AdjustWindowRectEx(LPRECT rect, DWORD style, BOOL menu, DWORD ex_style)
{
	return rect != nullptr;
}

BOOL WINAPI ClientToScreen(HWND hwnd, LPPOINT point)
{
	return point && WindowFrom(hwnd);
}

BOOL WINAPI ScreenToClient(HWND hwnd, LPPOINT point)
{
	return point && WindowFrom(hwnd);
}

static void ResizeLogical(int width, int height)
{
	bf_view_metrics m;
	Metrics(&m);
	if (width <= 0 || height <= 0 || m.backing_scale <= 0)
		return;
	bf_app_window_set_content_size(width * m.ui_scale / m.backing_scale, height * m.ui_scale / m.backing_scale);
}

BOOL WINAPI SetWindowPos(HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags)
{
	if (!WindowFrom(hwnd))
		return FALSE;
	if (!(flags & SWP_NOSIZE))
		ResizeLogical(cx, cy);   // the position is the system's
	return TRUE;
}

BOOL WINAPI MoveWindow(HWND hwnd, INT x, INT y, INT cx, INT cy, BOOL repaint)
{
	if (!WindowFrom(hwnd))
		return FALSE;
	ResizeLogical(cx, cy);
	return TRUE;
}

static LONG_PTR WindowLong(Window *w, INT index, LONG_PTR value, bool set)
{
	LONG_PTR old = 0;
	switch (index) {
	case GWL_STYLE:
		old = w->style;
		if (set) w->style = (LONG)value;   // WS_POPUP for fullscreen: nothing changes on macOS
		break;
	case GWL_EXSTYLE:
		old = w->ex_style;
		if (set) w->ex_style = (LONG)value;
		break;
	case GWLP_USERDATA:
		old = w->user_data;
		if (set) w->user_data = value;
		break;
	case GWLP_WNDPROC:
		old = (LONG_PTR)w->proc;
		if (set) w->proc = (WNDPROC)value;
		break;
	default:
		BF_STUB();
		break;
	}
	return old;
}

LONG WINAPI SetWindowLongA(HWND hwnd, INT index, LONG value)
{
	Window *w = WindowFrom(hwnd);
	return w ? (LONG)WindowLong(w, index, value, true) : 0;
}

LONG WINAPI GetWindowLongA(HWND hwnd, INT index)
{
	Window *w = WindowFrom(hwnd);
	return w ? (LONG)WindowLong(w, index, 0, false) : 0;
}

LONG_PTR WINAPI SetWindowLongPtrA(HWND hwnd, INT index, LONG_PTR value)
{
	Window *w = WindowFrom(hwnd);
	return w ? WindowLong(w, index, value, true) : 0;
}

LONG_PTR WINAPI GetWindowLongPtrA(HWND hwnd, INT index)
{
	Window *w = WindowFrom(hwnd);
	return w ? WindowLong(w, index, 0, false) : 0;
}

BOOL WINAPI SetWindowTextA(HWND hwnd, LPCSTR text)
{
	if (!WindowFrom(hwnd))
		return FALSE;
	bf_app_window_set_title(text ? Cp1252ToUtf8(text, strlen(text)).c_str() : "");
	return TRUE;
}

HWND WINAPI SetFocus(HWND hwnd)
{
	return g_active ? (HWND)g_window : nullptr;   // keyboard focus is the system's
}

HWND WINAPI GetFocus(void)
{
	return g_active ? (HWND)g_window : nullptr;
}

HWND WINAPI GetActiveWindow(void)
{
	return g_active ? (HWND)g_window : nullptr;
}

HWND WINAPI GetForegroundWindow(void)
{
	return g_active ? (HWND)g_window : nullptr;
}

INT WINAPI GetSystemMetrics(INT index)
{
	switch (index) {
	case SM_CXSCREEN:
	case SM_CYSCREEN: {
		// The screen in logical pixels (the units of everything here).
		bf_app_screen s = {};
		if (bf_app_window_get_screen(&s) != BF_APP_OK)
			return index == SM_CXSCREEN ? 1024 : 768;
		bf_view_metrics m;
		Metrics(&m);
		double px = (index == SM_CXSCREEN ? s.width_pt : s.height_pt) * s.scale;
		return (INT)(px / (m.ui_scale > 0 ? m.ui_scale : 1));
	}
	case SM_SWAPBUTTON:
		return 0;   // macOS swaps the primary button before AppKit reports it
	case SM_CMOUSEBUTTONS:
		return 3;
	case SM_MOUSEPRESENT:
		return 1;
	default:
		BF_TRACE(BFT_W32, "input: GetSystemMetrics(%d) = 0\n", index);
		return 0;
	}
}

// ------------------------------------------------------------------ messages

BOOL WINAPI PeekMessageA(LPMSG msg, HWND hwnd, UINT lo, UINT hi, UINT flags)
{
	return Peek(msg, hwnd, lo, hi, flags);
}

BOOL WINAPI GetMessageA(LPMSG msg, HWND hwnd, UINT lo, UINT hi)
{
	if (!msg)
		return -1;
	while (!Peek(msg, hwnd, lo, hi, PM_REMOVE))
		bf_app_pump(100);
	return msg->message != WM_QUIT;
}

BOOL WINAPI TranslateMessage(const MSG *msg)
{
	if (!msg)
		return FALSE;
	switch (msg->message) {
	case WM_KEYDOWN: {
		// What TranslateMessage typed that AppKit reports as no text: the
		// control characters CLanguage::OnChar and the edit controls handle.
		int c = 0;
		switch (msg->wParam) {
		case VK_BACK: c = 8; break;
		case VK_TAB: c = 9; break;
		case VK_RETURN: c = 13; break;
		case VK_ESCAPE: c = 27; break;
		}
		if (c && !VkDown(VK_CONTROL)) {
			MSG m = *msg;
			m.message = WM_CHAR;
			m.wParam = c;
			g_pending.push_front(m);
		}
		return TRUE;
	}
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return TRUE;
	default:
		return FALSE;
	}
}

LRESULT WINAPI DispatchMessageA(const MSG *msg)
{
	if (!msg)
		return 0;
	return Send(msg->hwnd, msg->message, msg->wParam, msg->lParam);
}

BOOL WINAPI PostMessageA(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (hwnd && !WindowFrom(hwnd))
		return FALSE;
	MSG m = {};
	m.hwnd = hwnd;
	m.message = message;
	m.wParam = wparam;
	m.lParam = lparam;
	m.time = (DWORD)bf_app_now_ms();
	g_pending.push_back(m);
	return TRUE;
}

LRESULT WINAPI SendMessageA(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	return Send(hwnd, message, wparam, lparam);
}

void WINAPI PostQuitMessage(INT code)
{
	MSG m = {};
	m.message = WM_QUIT;
	m.wParam = (WPARAM)code;
	m.time = (DWORD)bf_app_now_ms();
	g_pending.push_back(m);
}

LRESULT WINAPI DefWindowProcA(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message) {
	case WM_NCCREATE:
	case WM_NCACTIVATE:
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;
	case WM_SYSCOMMAND:
		if ((wparam & 0xFFF0) == SC_CLOSE)
			Send(hwnd, WM_CLOSE, 0, 0);
		return 0;
	case WM_SETCURSOR:
		// The class cursor, as Windows sets it when the window does not.
		if (LOWORD(lparam) == HTCLIENT && g_window) {
			auto it = g_classes.find(g_window->class_name);
			SetCursor(it != g_classes.end() && it->second.cursor ? it->second.cursor : kArrowCursor);
			return TRUE;
		}
		return FALSE;
	default:
		return 0;
	}
}

// ------------------------------------------------------------------ keyboard

SHORT WINAPI GetAsyncKeyState(INT vk)
{
	// Bit 0 ("pressed since the last call") stays clear: the client tests
	// the whole value (`if (GetAsyncKeyState(VK_SHIFT))`), and it reads as
	// held from it.
	return VkDown(vk) ? (SHORT)0x8000 : 0;
}

SHORT WINAPI GetKeyState(INT vk)
{
	return (SHORT)((VkDown(vk) ? 0x8000 : 0) | VkToggled(vk));
}

BOOL WINAPI GetKeyboardState(LPBYTE state)
{
	if (!state)
		return FALSE;
	for (int vk = 0; vk < 256; vk++)
		state[vk] = (BYTE)((VkDown(vk) ? 0x80 : 0) | VkToggled(vk));
	return TRUE;
}

UINT WINAPI MapVirtualKeyA(UINT code, UINT type)
{
	switch (type) {
	case MAPVK_VK_TO_VSC:
		return ScanForVk(code, nullptr);
	case MAPVK_VSC_TO_VK:
	case MAPVK_VSC_TO_VK_EX:
		return VkForScan(code);
	case MAPVK_VK_TO_CHAR:
		if ((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9'))
			return code;
		if (code >= VK_NUMPAD0 && code <= VK_NUMPAD9)
			return '0' + (code - VK_NUMPAD0);
		return 0;
	default:
		return 0;
	}
}

HKL WINAPI GetKeyboardLayout(DWORD thread)
{
	return (HKL)(uintptr_t)0x04090409;   // English (US): CLanguage takes its LOWORD as the language
}

INT WINAPI GetKeyboardType(INT which)
{
	switch (which) {
	case 0: return 4;    // enhanced 101/102-key
	case 1: return 0;
	case 2: return 12;   // function keys
	default: return 0;
	}
}

// ------------------------------------------------------------------ cursor

HCURSOR WINAPI SetCursor(HCURSOR cursor)
{
	// SetCursor(NULL) from WM_SETCURSOR: the system pointer disappears over
	// the content area, where the game draws its sprite; outside the
	// window it is shown as always.
	HCURSOR old = g_cursor;
	g_cursor = cursor;
	ApplyCursor();
	return old;
}

INT WINAPI ShowCursor(BOOL show)
{
	g_show_count += show ? 1 : -1;
	ApplyCursor();
	return g_show_count;
}

BOOL WINAPI GetCursorPos(LPPOINT point)
{
	if (!point)
		return FALSE;
	int x, y;
	PointerLogical(&x, &y);
	point->x = x;
	point->y = y;
	// The one caller (WM_MOUSEMOVE while inactive) hands this to
	// CMouse::SetXPos/SetYPos: the DirectInput mouse follows.
	bf_compat_input_mouse_set_client_pos(x, y);
	return TRUE;
}

BOOL WINAPI SetCursorPos(INT x, INT y)
{
	// The system pointer is never warped (spec section 4). What the client
	// asked for is what GetCursorPos reports until the pointer next moves.
	g_cursor_override = true;
	g_cursor_override_pos.x = x;
	g_cursor_override_pos.y = y;
	return TRUE;
}

// ------------------------------------------------------------------ DCs

HDC WINAPI GetDC(HWND hwnd)
{
	// The client calls GetDC(NULL) only, for EnumFontFamiliesEx; the window
	// has no DC of its own either (its picture is the gfx device's).
	return bf_compat_gdi_screen_dc();
}

INT WINAPI ReleaseDC(HWND hwnd, HDC hdc)
{
	return 1;   // the screen DC is never freed
}

// ------------------------------------------------------------------ clipboard

BOOL WINAPI OpenClipboard(HWND owner)
{
	if (g_clipboard_open)
		return FALSE;
	g_clipboard_open = true;
	return TRUE;
}

BOOL WINAPI CloseClipboard(void)
{
	if (!g_clipboard_open)
		return FALSE;
	g_clipboard_open = false;
	return TRUE;
}

BOOL WINAPI EmptyClipboard(void)
{
	if (!g_clipboard_open)
		return FALSE;
	FreeClipboardHandle();
	return bf_app_clipboard_set_text("") == BF_APP_OK;
}

HANDLE WINAPI GetClipboardData(UINT format)
{
	if (!g_clipboard_open || format != CF_TEXT)
		return nullptr;
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
	FreeClipboardHandle();
	HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
	if (!h)
		return nullptr;
	char *p = (char *)GlobalLock(h);
	if (!p) {
		GlobalFree(h);
		return nullptr;
	}
	memcpy(p, text.c_str(), text.size() + 1);
	GlobalUnlock(h);
	g_clipboard_handle = h;   // the clipboard's, as on Windows: freed on the next change
	return h;
}

HANDLE WINAPI SetClipboardData(UINT format, HANDLE data)
{
	if (!g_clipboard_open || !data)
		return nullptr;
	if (format != CF_TEXT) {
		BF_STUB();
		return nullptr;
	}
	const char *p = (const char *)GlobalLock(data);
	if (!p)
		return nullptr;
	size_t size = GlobalSize(data);
	size_t len = strnlen(p, size ? size : 0x100000);
	std::string utf8 = Cp1252ToUtf8(p, len);
	GlobalUnlock(data);
	// CR LF to the Mac's LF.
	std::string text;
	for (size_t i = 0; i < utf8.size(); i++)
		if (!(utf8[i] == '\r' && i + 1 < utf8.size() && utf8[i + 1] == '\n'))
			text += utf8[i];
	int ok = bf_app_clipboard_set_text(text.c_str());
	// The system owns the memory now; it has been copied, so it goes.
	GlobalFree(data);
	return ok == BF_APP_OK ? data : nullptr;
}

}  // extern "C"
