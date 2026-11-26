// The keyboard layout through the system (bf_app.h, "keyboard layout"):
// UCKeyTranslate on the layout's 'uchr' data, from Text Input Sources.
// Nothing here selects or changes an input source.
#include <Carbon/Carbon.h>

#include <string.h>

#include <string>

#include "bf_app.h"
#include "bf_trace.h"

namespace {

TISInputSourceRef g_named = NULL;   // bf_app_key_layout_use's layout, retained
std::string g_named_id;

// The layout to translate with, retained; NULL when none has 'uchr' data.
TISInputSourceRef CopyLayout()
{
	if (g_named) {
		CFRetain(g_named);
		return g_named;
	}
	TISInputSourceRef src = TISCopyCurrentKeyboardLayoutInputSource();
	if (src && !TISGetInputSourceProperty(src, kTISPropertyUnicodeKeyLayoutData)) {
		CFRelease(src);
		src = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
	}
	return src;
}

const UCKeyboardLayout *LayoutData(TISInputSourceRef src)
{
	if (!src)
		return nullptr;
	CFDataRef data = (CFDataRef)TISGetInputSourceProperty(src, kTISPropertyUnicodeKeyLayoutData);
	return data ? (const UCKeyboardLayout *)CFDataGetBytePtr(data) : nullptr;
}

// One key press as UTF-16 into out (up to 8 units); a dead key gives its
// accent (the dead state ended by Space) and sets *dead.
UniCharCount Translate(const UCKeyboardLayout *layout, unsigned keycode, uint32_t mods, UniChar *out, int *dead)
{
	UInt32 state = 0;
	if (mods & BF_APP_MOD_SHIFT) state |= shiftKey >> 8;
	if (mods & BF_APP_MOD_ALT) state |= optionKey >> 8;
	if (mods & BF_APP_MOD_CAPSLOCK) state |= alphaLock >> 8;
	UInt32 deadState = 0;
	UniCharCount n = 0;
	UInt32 kbd = LMGetKbdType();
	if (UCKeyTranslate(layout, (UInt16)keycode, kUCKeyActionDown, state, kbd, 0, &deadState, 8, &n, out) != noErr)
		return 0;
	if (dead)
		*dead = 0;
	if (n == 0 && deadState != 0) {
		if (dead)
			*dead = 1;
		if (UCKeyTranslate(layout, kVK_Space, kUCKeyActionDown, 0, kbd, 0, &deadState, 8, &n, out) != noErr)
			return 0;
	}
	return n;
}

std::string Utf8(const UniChar *u, UniCharCount n)
{
	std::string out;
	if (!n)
		return out;
	CFStringRef s = CFStringCreateWithCharacters(kCFAllocatorDefault, u, (CFIndex)n);
	if (!s)
		return out;
	char buf[64];
	if (CFStringGetCString(s, buf, sizeof buf, kCFStringEncodingUTF8))
		out = buf;
	CFRelease(s);
	return out;
}

}  // namespace

extern "C" size_t bf_app_key_layout_text(unsigned mac_keycode, uint32_t mods, char *buf, size_t size, int *dead)
{
	if (dead)
		*dead = 0;
	if (buf && size)
		buf[0] = 0;
	if (mac_keycode >= 128)
		return 0;
	TISInputSourceRef src = CopyLayout();
	const UCKeyboardLayout *layout = LayoutData(src);
	std::string text;
	if (layout) {
		UniChar u[8];
		text = Utf8(u, Translate(layout, mac_keycode, mods, u, dead));
	}
	if (src)
		CFRelease(src);
	if (buf && size) {
		size_t n = text.size() < size - 1 ? text.size() : size - 1;
		while (n > 0 && n < text.size() && ((unsigned char)text[n] & 0xC0) == 0x80)
			n--;   // at a character boundary
		memcpy(buf, text.data(), n);
		buf[n] = 0;
	}
	return text.size();
}

extern "C" int bf_app_key_layout_types_plain(const char *utf8)
{
	if (!utf8 || !*utf8)
		return 0;
	TISInputSourceRef src = CopyLayout();
	const UCKeyboardLayout *layout = LayoutData(src);
	int found = 0;
	for (unsigned k = 0; layout && k < 128 && !found; k++) {
		// The modifier keys and the function keys type nothing; asking is harmless.
		for (uint32_t mods : {0u, (uint32_t)BF_APP_MOD_SHIFT}) {
			UniChar u[8];
			if (Utf8(u, Translate(layout, k, mods, u, nullptr)) == utf8) {
				found = 1;
				break;
			}
		}
	}
	if (src)
		CFRelease(src);
	return found;
}

extern "C" int bf_app_key_layout_use(const char *source_id)
{
	if (!source_id) {
		if (g_named)
			CFRelease(g_named);
		g_named = NULL;
		g_named_id.clear();
		return BF_APP_OK;
	}
	CFStringRef id = CFStringCreateWithCString(kCFAllocatorDefault, source_id, kCFStringEncodingUTF8);
	if (!id)
		return BF_APP_ERROR;
	const void *keys[] = {kTISPropertyInputSourceID};
	const void *values[] = {id};
	CFDictionaryRef props = CFDictionaryCreate(kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks,
	                                           &kCFTypeDictionaryValueCallBacks);
	CFArrayRef list = props ? TISCreateInputSourceList(props, true) : NULL;
	TISInputSourceRef found = NULL;
	if (list && CFArrayGetCount(list) > 0) {
		TISInputSourceRef s = (TISInputSourceRef)CFArrayGetValueAtIndex(list, 0);
		if (TISGetInputSourceProperty(s, kTISPropertyUnicodeKeyLayoutData)) {
			found = s;
			CFRetain(found);
		}
	}
	if (list)
		CFRelease(list);
	if (props)
		CFRelease(props);
	CFRelease(id);
	if (!found) {
		bf_tracef(BFT_ERR, "app: no keyboard layout \"%s\" with key layout data is installed", source_id);
		return BF_APP_ERROR;
	}
	if (g_named)
		CFRelease(g_named);
	g_named = found;
	g_named_id = source_id;
	return BF_APP_OK;
}

extern "C" const char *bf_app_key_layout_named(void)
{
	return g_named ? g_named_id.c_str() : NULL;
}
