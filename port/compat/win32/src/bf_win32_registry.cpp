// advapi32's registry on the settings plist (bf_settings), as the sys
// README maps it:
//
//   HKLM\Software\<name>[\sub\key]   group Client[/sub/key]   (g_regPath)
//   any other key                    group Registry/<HKLM|HKCU|...>/<path>
//
// Key and value names and REG_SZ data are the client's bytes in CP_ACP and
// are stored as UTF-8. REG_DWORD is an integer, REG_SZ a string, everything
// else data. Two values of the client's root are not stored there:
//
//   ID        the login window's remembered account: Account/Name
//   PASSWORD  the Keychain item of that account (bf_sys_keychain_*)
//
// RegCloseKey saves the plist when something changed.
#include "bf_win32_internal.h"

#include <mutex>
#include <string.h>
#include <string>
#include <unordered_set>
#include <vector>

using namespace bf_win32;

namespace {

const uint32_t kKeyMagic = 0x4B455931;   // "KEY1"

struct Key {
	uint32_t magic = kKeyMagic;
	std::string root;                // "HKLM", ...
	std::vector<std::string> path;   // components below the root, UTF-8
};

// Never destroyed: the client's static objects (CFileMgr's archives) close
// their handles from their destructors at exit, after this file's statics.
std::mutex &g_keyLock = *new std::mutex;
std::unordered_set<Key *> &Keys()
{
	static std::unordered_set<Key *> *keys = new std::unordered_set<Key *>;
	return *keys;
}

const char *RootName(HKEY key)
{
	switch ((uint32_t)(uintptr_t)key) {
	case 0x80000000u: return "HKCR";
	case 0x80000001u: return "HKCU";
	case 0x80000002u: return "HKLM";
	case 0x80000003u: return "HKU";
	case 0x80000005u: return "HKCC";
	default:          return NULL;
	}
}

bool IsPredefined(HKEY key)
{
	uintptr_t v = (uintptr_t)key;
	return (v >> 32) == 0xFFFFFFFFu && RootName(key) != NULL;
}

// The key behind a handle (a predefined root or an open key), or false.
bool Resolve(HKEY handle, Key *out)
{
	if (IsPredefined(handle)) {
		out->root = RootName(handle);
		out->path.clear();
		return true;
	}
	std::lock_guard<std::mutex> guard(g_keyLock);
	Key *key = (Key *)handle;
	if (!Keys().count(key))
		return false;
	*out = *key;
	return true;
}

void Append(Key *key, const char *sub)
{
	if (!sub)
		return;
	std::string text = ToUtf8(sub);
	size_t i = 0;
	while (i <= text.size()) {
		size_t j = text.find_first_of("\\/", i);
		if (j == std::string::npos)
			j = text.size();
		if (j > i)
			key->path.push_back(text.substr(i, j - i));
		i = j + 1;
	}
}

// True for HKLM\Software\<name>\...: the client's own root, group Client.
bool IsClient(const Key &key)
{
	return key.root == "HKLM" && key.path.size() >= 2 && strcasecmp(key.path[0].c_str(), "Software") == 0
	       && strcasecmp(key.path[1].c_str(), BF_APP_NAME) == 0;
}

std::string Group(const Key &key)
{
	std::string group;
	size_t first;
	if (IsClient(key)) {
		group = BF_SETTINGS_CLIENT;
		first = 2;
	} else {
		group = std::string(BF_SETTINGS_REGISTRY) + "/" + key.root;
		first = 0;
	}
	for (size_t i = first; i < key.path.size(); ++i)
		group += "/" + key.path[i];
	return group;
}

bool IsClientRoot(const Key &key)
{
	return IsClient(key) && key.path.size() == 2;
}

HKEY NewHandle(const Key &key)
{
	Key *copy = new Key(key);
	std::lock_guard<std::mutex> guard(g_keyLock);
	Keys().insert(copy);
	return (HKEY)copy;
}

bf_settings *Settings()
{
	return bf_settings_shared();
}

std::string AccountName()
{
	char name[256];
	if (bf_settings_account_name(Settings(), name, sizeof(name)) != 1)
		return std::string();
	return name;
}

// Copies a value out as RegQueryValueEx does.
LSTATUS Deliver(DWORD type, const void *bytes, DWORD size, LPDWORD type_out, LPBYTE data, LPDWORD size_io)
{
	if (type_out)
		*type_out = type;
	if (!data) {
		if (size_io)
			*size_io = size;
		return ERROR_SUCCESS;
	}
	if (!size_io)
		return ERROR_INVALID_PARAMETER;
	if (*size_io < size) {
		*size_io = size;
		return ERROR_MORE_DATA;
	}
	memcpy(data, bytes, size);
	*size_io = size;
	return ERROR_SUCCESS;
}

LSTATUS DeliverString(const std::string &utf8, LPDWORD type_out, LPBYTE data, LPDWORD size_io)
{
	std::string bytes = FromUtf8(utf8);
	return Deliver(REG_SZ, bytes.c_str(), (DWORD)bytes.size() + 1, type_out, data, size_io);
}

// A REG_SZ as the client passes it: up to the first NUL within size.
std::string StringOf(const BYTE *data, DWORD size)
{
	if (!data)
		return std::string();
	return ToUtf8((const char *)data, (int)strnlen((const char *)data, size));
}

}  // namespace

LSTATUS WINAPI RegOpenKeyExA(HKEY parent, LPCSTR sub, DWORD options, REGSAM sam, PHKEY result)
{
	(void)options;
	(void)sam;
	if (!result)
		return ERROR_INVALID_PARAMETER;
	*result = NULL;
	Key key;
	if (!Resolve(parent, &key))
		return ERROR_INVALID_HANDLE;
	Append(&key, sub);
	if (!key.path.empty() && !bf_settings_has_group(Settings(), Group(key).c_str())) {
		BF_TRACE(BFT_W32, "RegOpenKeyEx(%s): absent", Group(key).c_str());
		return ERROR_FILE_NOT_FOUND;
	}
	*result = NewHandle(key);
	return ERROR_SUCCESS;
}

LSTATUS WINAPI RegCreateKeyExA(HKEY parent, LPCSTR sub, DWORD reserved, LPSTR key_class, DWORD options,
                               REGSAM sam, LPSECURITY_ATTRIBUTES attributes, PHKEY result, LPDWORD disposition)
{
	(void)reserved;
	(void)key_class;
	(void)options;
	(void)sam;
	(void)attributes;
	if (!result)
		return ERROR_INVALID_PARAMETER;
	*result = NULL;
	Key key;
	if (!Resolve(parent, &key))
		return ERROR_INVALID_HANDLE;
	Append(&key, sub);
	std::string group = Group(key);
	bool existed = key.path.empty() || bf_settings_has_group(Settings(), group.c_str());
	if (!existed && !bf_settings_create_group(Settings(), group.c_str()))
		return ERROR_ACCESS_DENIED;
	if (disposition)
		*disposition = existed ? REG_OPENED_EXISTING_KEY : REG_CREATED_NEW_KEY;
	*result = NewHandle(key);
	return ERROR_SUCCESS;
}

LSTATUS WINAPI RegCloseKey(HKEY handle)
{
	if (IsPredefined(handle))
		return ERROR_SUCCESS;
	{
		std::lock_guard<std::mutex> guard(g_keyLock);
		Key *key = (Key *)handle;
		if (!Keys().erase(key))
			return ERROR_INVALID_HANDLE;
		delete key;
	}
	if (bf_settings_save(Settings()) < 0)
		bf_tracef(BFT_ERR, "registry: cannot save %s", bf_settings_path(Settings()));
	return ERROR_SUCCESS;
}

LSTATUS WINAPI RegQueryValueExA(HKEY handle, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data,
                                LPDWORD size)
{
	(void)reserved;
	Key key;
	if (!Resolve(handle, &key))
		return ERROR_INVALID_HANDLE;
	if (data && !size)
		return ERROR_INVALID_PARAMETER;
	std::string value = ToUtf8(name ? name : "");

	if (IsClientRoot(key) && strcasecmp(value.c_str(), "ID") == 0) {
		std::string account = AccountName();
		if (account.empty())
			return ERROR_FILE_NOT_FOUND;
		return DeliverString(account, type, data, size);
	}
	if (IsClientRoot(key) && strcasecmp(value.c_str(), "PASSWORD") == 0) {
		std::string account = AccountName();
		char password[256];
		if (account.empty() || !bf_sys_keychain_available()
		    || bf_sys_keychain_read(account.c_str(), password, sizeof(password)) != 1)
			return ERROR_FILE_NOT_FOUND;
		LSTATUS r = DeliverString(password, type, data, size);
		memset(password, 0, sizeof(password));
		return r;
	}

	std::string group = Group(key);
	switch (bf_settings_type(Settings(), group.c_str(), value.c_str())) {
	case BF_SETTING_INT: {
		int32_t v = 0;
		bf_settings_get_int(Settings(), group.c_str(), value.c_str(), &v);
		return Deliver(REG_DWORD, &v, sizeof(v), type, data, size);
	}
	case BF_SETTING_STRING: {
		size_t length = 0;
		bf_settings_get_string(Settings(), group.c_str(), value.c_str(), NULL, 0, &length);
		std::string text(length + 1, '\0');
		bf_settings_get_string(Settings(), group.c_str(), value.c_str(), &text[0], text.size(), NULL);
		text.resize(length);
		return DeliverString(text, type, data, size);
	}
	case BF_SETTING_DATA: {
		size_t length = 0;
		bf_settings_get_data(Settings(), group.c_str(), value.c_str(), NULL, 0, &length);
		std::vector<unsigned char> bytes(length ? length : 1);
		bf_settings_get_data(Settings(), group.c_str(), value.c_str(), bytes.data(), bytes.size(), NULL);
		return Deliver(REG_BINARY, bytes.data(), (DWORD)length, type, data, size);
	}
	default:
		return ERROR_FILE_NOT_FOUND;
	}
}

LSTATUS WINAPI RegSetValueExA(HKEY handle, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD size)
{
	(void)reserved;
	Key key;
	if (!Resolve(handle, &key))
		return ERROR_INVALID_HANDLE;
	if (!data && size)
		return ERROR_INVALID_PARAMETER;
	std::string value = ToUtf8(name ? name : "");
	bool string_type = type == REG_SZ || type == REG_EXPAND_SZ;

	if (IsClientRoot(key) && strcasecmp(value.c_str(), "ID") == 0) {
		std::string account = string_type ? StringOf(data, size) : std::string();
		return bf_settings_set_account_name(Settings(), account.c_str()) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
	}
	if (IsClientRoot(key) && strcasecmp(value.c_str(), "PASSWORD") == 0) {
		std::string account = AccountName();
		std::string password = string_type ? StringOf(data, size) : std::string();
		if (account.empty() || !bf_sys_keychain_available()) {
			BF_TRACE(BFT_W32, "registry: PASSWORD not stored (no account name or no Keychain)");
			return ERROR_SUCCESS;
		}
		if (password.empty())
			bf_sys_keychain_delete(account.c_str());
		else if (!bf_sys_keychain_store(account.c_str(), password.c_str()))
			return ERROR_ACCESS_DENIED;
		return ERROR_SUCCESS;
	}

	std::string group = Group(key);
	int ok;
	if (type == REG_DWORD && size == 4) {
		int32_t v;
		memcpy(&v, data, 4);
		ok = bf_settings_set_int(Settings(), group.c_str(), value.c_str(), v);
	} else if (string_type) {
		ok = bf_settings_set_string(Settings(), group.c_str(), value.c_str(), StringOf(data, size).c_str());
	} else {
		ok = bf_settings_set_data(Settings(), group.c_str(), value.c_str(), data, size);
	}
	return ok ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
}

// Removes the key with everything below it (Windows refuses a key with
// subkeys; the client deletes only BookMark keys, which have none).
LSTATUS WINAPI RegDeleteKeyA(HKEY handle, LPCSTR sub)
{
	Key key;
	if (!Resolve(handle, &key))
		return ERROR_INVALID_HANDLE;
	Append(&key, sub);
	if (key.path.empty())
		return ERROR_ACCESS_DENIED;
	return bf_settings_remove(Settings(), Group(key).c_str(), NULL) ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}

LSTATUS WINAPI RegDeleteValueA(HKEY handle, LPCSTR name)
{
	Key key;
	if (!Resolve(handle, &key))
		return ERROR_INVALID_HANDLE;
	std::string value = ToUtf8(name ? name : "");
	return bf_settings_remove(Settings(), Group(key).c_str(), value.c_str()) ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}
