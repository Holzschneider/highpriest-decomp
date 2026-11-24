// The key table: portable keys (USB HID usages, bf_app.h) to the virtual key,
// set-1 scan code and extended flag that a Windows keyboard driver reports
// for the same key, and the CP1252 conversions of typed and pasted text.
#include "bf_compat_input_internal.h"

#include <string.h>

#include "bf_text.h"

namespace bfin {
namespace {

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
		// that the game's screenshot keys work (README, "Keys").
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

const Table &T()
{
	static const Table t;
	return t;
}

}  // namespace

const KeyInfo &KeyFor(int key)
{
	static const KeyInfo none = {0, 0, 0, 0};
	if (key <= 0 || key >= BF_KEY_COUNT)
		return none;
	return T().k[key];
}

unsigned ScanForVk(unsigned vk, int *extended)
{
	if (extended)
		*extended = 0;
	if (vk == 0 || vk > 0xFF)
		return 0;
	// The first key that sends it; the left one for the modifiers.
	for (int key = 1; key < BF_KEY_COUNT; key++) {
		const KeyInfo &k = T().k[key];
		if (k.vk == vk || k.state_vk == vk) {
			if (extended)
				*extended = k.extended;
			return k.scan;
		}
	}
	return 0;
}

unsigned VkForScan(unsigned scan)
{
	if (scan == 0)
		return 0;
	for (int key = 1; key < BF_KEY_COUNT; key++) {
		const KeyInfo &k = T().k[key];
		if (k.scan == scan && !k.extended && key < BF_KEY_F13)
			return k.vk;
	}
	return 0;
}

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

}  // namespace bfin

extern "C" unsigned bf_compat_input_vk_for_key(int key, unsigned *scan, int *extended)
{
	const bfin::KeyInfo &k = bfin::KeyFor(key);
	if (scan)
		*scan = k.scan;
	if (extended)
		*extended = k.extended;
	return k.vk;
}

extern "C" int bf_compat_input_is_alt_shortcut(unsigned vk)
{
	// UIWindowMgr::ProcessPushButton, the cases that test VK_MENU: the
	// basic-info window's buttons, the shortcut bar, X (quit), Insert
	// (compass), Home (attribute indicator), F10 (chat window).
	switch (vk) {
	case 'A': case 'B': case 'E': case 'G': case 'J': case 'O': case 'P': case 'Q':
	case 'R': case 'S': case 'T': case 'V': case 'W': case 'X':
	case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
	case VK_INSERT: case VK_HOME: case VK_F10:
		return 1;
	default:
		return 0;
	}
}
