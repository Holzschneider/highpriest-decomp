// Key code tables: macOS virtual key codes (kVK_*, Carbon's Events.h) to the
// portable HID usage codes of bf_key, and names for the control socket.

#include "bf_app.h"

#include <string.h>
#include <strings.h>

namespace {

struct KeyEntry { unsigned short mac; unsigned short key; const char *name; };

// Every key a Mac keyboard reports that the game or the adapter may care
// about. A PC keyboard's Insert arrives as kVK_Help (0x72), and its Print
// Screen, Scroll Lock and Pause as F13, F14 and F15.
const KeyEntry kKeys[] = {
	{ 0x00, BF_KEY_A, "a" }, { 0x0B, BF_KEY_B, "b" }, { 0x08, BF_KEY_C, "c" },
	{ 0x02, BF_KEY_D, "d" }, { 0x0E, BF_KEY_E, "e" }, { 0x03, BF_KEY_F, "f" },
	{ 0x05, BF_KEY_G, "g" }, { 0x04, BF_KEY_H, "h" }, { 0x22, BF_KEY_I, "i" },
	{ 0x26, BF_KEY_J, "j" }, { 0x28, BF_KEY_K, "k" }, { 0x25, BF_KEY_L, "l" },
	{ 0x2E, BF_KEY_M, "m" }, { 0x2D, BF_KEY_N, "n" }, { 0x1F, BF_KEY_O, "o" },
	{ 0x23, BF_KEY_P, "p" }, { 0x0C, BF_KEY_Q, "q" }, { 0x0F, BF_KEY_R, "r" },
	{ 0x01, BF_KEY_S, "s" }, { 0x11, BF_KEY_T, "t" }, { 0x20, BF_KEY_U, "u" },
	{ 0x09, BF_KEY_V, "v" }, { 0x0D, BF_KEY_W, "w" }, { 0x07, BF_KEY_X, "x" },
	{ 0x10, BF_KEY_Y, "y" }, { 0x06, BF_KEY_Z, "z" },
	{ 0x12, BF_KEY_1, "1" }, { 0x13, BF_KEY_2, "2" }, { 0x14, BF_KEY_3, "3" },
	{ 0x15, BF_KEY_4, "4" }, { 0x17, BF_KEY_5, "5" }, { 0x16, BF_KEY_6, "6" },
	{ 0x1A, BF_KEY_7, "7" }, { 0x1C, BF_KEY_8, "8" }, { 0x19, BF_KEY_9, "9" },
	{ 0x1D, BF_KEY_0, "0" },
	{ 0x24, BF_KEY_RETURN, "return" }, { 0x35, BF_KEY_ESCAPE, "escape" },
	{ 0x33, BF_KEY_BACKSPACE, "backspace" }, { 0x30, BF_KEY_TAB, "tab" },
	{ 0x31, BF_KEY_SPACE, "space" },
	{ 0x1B, BF_KEY_MINUS, "minus" }, { 0x18, BF_KEY_EQUAL, "equal" },
	{ 0x21, BF_KEY_LEFTBRACKET, "leftbracket" }, { 0x1E, BF_KEY_RIGHTBRACKET, "rightbracket" },
	{ 0x2A, BF_KEY_BACKSLASH, "backslash" }, { 0x29, BF_KEY_SEMICOLON, "semicolon" },
	{ 0x27, BF_KEY_QUOTE, "quote" }, { 0x32, BF_KEY_GRAVE, "grave" },
	{ 0x2B, BF_KEY_COMMA, "comma" }, { 0x2F, BF_KEY_PERIOD, "period" },
	{ 0x2C, BF_KEY_SLASH, "slash" }, { 0x39, BF_KEY_CAPSLOCK, "capslock" },
	{ 0x7A, BF_KEY_F1, "f1" }, { 0x78, BF_KEY_F2, "f2" }, { 0x63, BF_KEY_F3, "f3" },
	{ 0x76, BF_KEY_F4, "f4" }, { 0x60, BF_KEY_F5, "f5" }, { 0x61, BF_KEY_F6, "f6" },
	{ 0x62, BF_KEY_F7, "f7" }, { 0x64, BF_KEY_F8, "f8" }, { 0x65, BF_KEY_F9, "f9" },
	{ 0x6D, BF_KEY_F10, "f10" }, { 0x67, BF_KEY_F11, "f11" }, { 0x6F, BF_KEY_F12, "f12" },
	{ 0x69, BF_KEY_F13, "f13" }, { 0x6B, BF_KEY_F14, "f14" }, { 0x71, BF_KEY_F15, "f15" },
	{ 0x6A, BF_KEY_F16, "f16" }, { 0x40, BF_KEY_F17, "f17" }, { 0x4F, BF_KEY_F18, "f18" },
	{ 0x50, BF_KEY_F19, "f19" }, { 0x5A, BF_KEY_F20, "f20" },
	{ 0x72, BF_KEY_INSERT, "insert" }, { 0x73, BF_KEY_HOME, "home" },
	{ 0x74, BF_KEY_PAGEUP, "pageup" }, { 0x75, BF_KEY_DELETE, "delete" },
	{ 0x77, BF_KEY_END, "end" }, { 0x79, BF_KEY_PAGEDOWN, "pagedown" },
	{ 0x7C, BF_KEY_RIGHT, "right" }, { 0x7B, BF_KEY_LEFT, "left" },
	{ 0x7D, BF_KEY_DOWN, "down" }, { 0x7E, BF_KEY_UP, "up" },
	{ 0x47, BF_KEY_NUMLOCK, "numlock" }, { 0x4B, BF_KEY_KP_DIVIDE, "kp_divide" },
	{ 0x43, BF_KEY_KP_MULTIPLY, "kp_multiply" }, { 0x4E, BF_KEY_KP_MINUS, "kp_minus" },
	{ 0x45, BF_KEY_KP_PLUS, "kp_plus" }, { 0x4C, BF_KEY_KP_ENTER, "kp_enter" },
	{ 0x53, BF_KEY_KP_1, "kp_1" }, { 0x54, BF_KEY_KP_2, "kp_2" }, { 0x55, BF_KEY_KP_3, "kp_3" },
	{ 0x56, BF_KEY_KP_4, "kp_4" }, { 0x57, BF_KEY_KP_5, "kp_5" }, { 0x58, BF_KEY_KP_6, "kp_6" },
	{ 0x59, BF_KEY_KP_7, "kp_7" }, { 0x5B, BF_KEY_KP_8, "kp_8" }, { 0x5C, BF_KEY_KP_9, "kp_9" },
	{ 0x52, BF_KEY_KP_0, "kp_0" }, { 0x41, BF_KEY_KP_PERIOD, "kp_period" },
	{ 0x51, BF_KEY_KP_EQUAL, "kp_equal" },
	{ 0x0A, BF_KEY_NONUSBACKSLASH, "nonusbackslash" }, { 0x6E, BF_KEY_MENU, "menu" },
	{ 0x4A, BF_KEY_MUTE, "mute" }, { 0x48, BF_KEY_VOLUMEUP, "volumeup" },
	{ 0x49, BF_KEY_VOLUMEDOWN, "volumedown" },
	{ 0x3B, BF_KEY_LCTRL, "lctrl" }, { 0x38, BF_KEY_LSHIFT, "lshift" },
	{ 0x3A, BF_KEY_LALT, "lalt" }, { 0x37, BF_KEY_LCMD, "lcmd" },
	{ 0x3E, BF_KEY_RCTRL, "rctrl" }, { 0x3C, BF_KEY_RSHIFT, "rshift" },
	{ 0x3D, BF_KEY_RALT, "ralt" }, { 0x36, BF_KEY_RCMD, "rcmd" },
	{ 0x3F, BF_KEY_FN, "fn" },
};
const size_t kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

} // namespace

extern "C" int bf_app_key_from_mac(unsigned mac_keycode)
{
	for (size_t i = 0; i < kKeyCount; ++i)
		if (kKeys[i].mac == mac_keycode)
			return kKeys[i].key;
	return BF_KEY_NONE;
}

extern "C" int bf_app_key_to_mac(int key)
{
	for (size_t i = 0; i < kKeyCount; ++i)
		if (kKeys[i].key == key)
			return kKeys[i].mac;
	return -1;
}

extern "C" const char *bf_app_key_name(int key)
{
	for (size_t i = 0; i < kKeyCount; ++i)
		if (kKeys[i].key == key)
			return kKeys[i].name;
	return "?";
}

extern "C" int bf_app_key_from_name(const char *name)
{
	if (!name)
		return BF_KEY_NONE;
	for (size_t i = 0; i < kKeyCount; ++i)
		if (strcasecmp(kKeys[i].name, name) == 0)
			return kKeys[i].key;
	return BF_KEY_NONE;
}
