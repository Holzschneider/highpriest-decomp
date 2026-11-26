// The saved account password in the Keychain: a generic password item.

#include "bf_sys.h"
#include "bf_trace.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include <stdlib.h>
#include <string.h>

namespace {

CFStringRef String(const char *utf8)
{
	return CFStringCreateWithCString(NULL, utf8, kCFStringEncodingUTF8);
}

// The query naming the item of one account. NULL if a name is not UTF-8.
CFMutableDictionaryRef Query(const char *account)
{
	CFStringRef service = String(BF_APP_NAME);
	CFStringRef user = account ? String(account) : NULL;
	if (!service || !user) {
		if (service) CFRelease(service);
		if (user) CFRelease(user);
		return NULL;
	}
	CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
	                                                         &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
	CFDictionarySetValue(query, kSecAttrService, service);
	CFDictionarySetValue(query, kSecAttrAccount, user);
	CFRelease(service);
	CFRelease(user);
	return query;
}

void Report(const char *what, OSStatus status)
{
	CFStringRef message = SecCopyErrorMessageString(status, NULL);
	char text[256] = "";
	if (message) {
		CFStringGetCString(message, text, sizeof(text), kCFStringEncodingUTF8);
		CFRelease(message);
	}
	bf_tracef(BFT_ERR, "sys: keychain %s failed: %d %s", what, (int)status, text);
}

} // namespace

extern "C" int bf_sys_keychain_store(const char *account, const char *password)
{
	CFMutableDictionaryRef query = Query(account);
	if (!query || !password) {
		if (query) CFRelease(query);
		return 0;
	}
	CFDataRef secret = CFDataCreate(NULL, (const UInt8 *)password, (CFIndex)strlen(password));
	CFMutableDictionaryRef change = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
	                                                          &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(change, kSecValueData, secret);
	OSStatus status = SecItemUpdate(query, change);
	if (status == errSecItemNotFound) {
		CFDictionarySetValue(query, kSecValueData, secret);
		CFStringRef label = String(BF_APP_NAME " account");
		CFDictionarySetValue(query, kSecAttrLabel, label);
		CFRelease(label);
		status = SecItemAdd(query, NULL);
	}
	CFRelease(change);
	CFRelease(secret);
	CFRelease(query);
	if (status != errSecSuccess) {
		Report("store", status);
		return 0;
	}
	return 1;
}

extern "C" int bf_sys_keychain_read(const char *account, char *out, size_t out_size)
{
	CFMutableDictionaryRef query = Query(account);
	if (!query)
		return -1;
	CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
	CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);
	CFTypeRef result = NULL;
	OSStatus status = SecItemCopyMatching(query, &result);
	CFRelease(query);
	if (status == errSecItemNotFound)
		return 0;
	if (status != errSecSuccess || !result || CFGetTypeID(result) != CFDataGetTypeID()) {
		if (result) CFRelease(result);
		Report("read", status);
		return -1;
	}
	CFDataRef data = (CFDataRef)result;
	size_t length = (size_t)CFDataGetLength(data);
	int answer = -1;
	if (out && length + 1 <= out_size) {
		memcpy(out, CFDataGetBytePtr(data), length);
		out[length] = 0;
		answer = 1;
	}
	CFRelease(result);
	return answer;
}

extern "C" int bf_sys_keychain_delete(const char *account)
{
	CFMutableDictionaryRef query = Query(account);
	if (!query)
		return -1;
	OSStatus status = SecItemDelete(query);
	CFRelease(query);
	if (status == errSecItemNotFound)
		return 0;
	if (status != errSecSuccess) {
		Report("delete", status);
		return -1;
	}
	return 1;
}

extern "C" int bf_sys_keychain_available(void)
{
	// BF_NO_KEYCHAIN=1 (the test harness sets it): never touch the Keychain.
	// A rebuilt, ad-hoc signed client is not on the item's access list, and
	// reading it raises a prompt on the screen of whoever works at the Mac,
	// with the client's main thread waiting for an answer.
	if (const char *off = getenv("BF_NO_KEYCHAIN"))
		if (*off && strcmp(off, "0") != 0) {
			BF_TRACE(BFT_FILE, "sys: keychain not available (BF_NO_KEYCHAIN)");
			return 0;
		}
	// A search succeeds in a locked keychain; only adding needs it unlocked,
	// so ask for the lock state. The SecKeychain calls are deprecated but are
	// the only way to learn it without touching an item.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	SecKeychainRef keychain = NULL;
	OSStatus status = SecKeychainCopyDefault(&keychain);
	SecKeychainStatus state = 0;
	if (status == errSecSuccess)
		status = SecKeychainGetStatus(keychain, &state);
	if (keychain)
		CFRelease(keychain);
#pragma clang diagnostic pop
	if (status != errSecSuccess || !(state & kSecUnlockStateStatus)) {
		BF_TRACE(BFT_FILE, "sys: keychain not available (status %d, state %u)", (int)status, (unsigned)state);
		return 0;
	}
	return 1;
}
