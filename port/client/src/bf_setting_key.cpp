// The client's settings keys on bf_settings (bf_setting_key.h). The mapping
// and the value rules are those of the registry adapter of stage A
// (port/compat/win32/src/bf_win32_registry.cpp until M11.4), so that the
// Settings.plist files it wrote read the same.
#include "bf_setting_key.h"

#include <string.h>
#include <strings.h>
#include <vector>

#include "bf_file.h"
#include "bf_sys.h"
#include "bf_text.h"
#include "bf_trace.h"

namespace {

bf_settings *Settings()
{
	return bf_settings_shared();
}

// The client's code page (CP949 or CP1252) to UTF-8; bytes that do not
// convert are kept as they are.
std::string ToUtf8(const char *text, int len = -1)
{
	if (!text)
		return std::string();
	if (len < 0)
		len = (int)strlen(text);
	int cp = (int)bf_file_user_codepage();
	int n = bf_text_to_utf8(cp, text, len, NULL, 0);
	if (n <= 0)
		return n == 0 ? std::string() : std::string(text, (size_t)len);
	std::string out((size_t)n, '\0');
	bf_text_to_utf8(cp, text, len, &out[0], n);
	return out;
}

std::string FromUtf8(const std::string &utf8)
{
	int cp = (int)bf_file_user_codepage();
	int n = bf_text_from_utf8(cp, utf8.data(), (int)utf8.size(), NULL, 0);
	if (n <= 0)
		return n == 0 ? std::string() : utf8;
	std::string out((size_t)n, '\0');
	bf_text_from_utf8(cp, utf8.data(), (int)utf8.size(), &out[0], n);
	return out;
}

// The parts of a path below HKEY_LOCAL_MACHINE, split at "\" and "/" after
// the conversion (a CP949 trail byte may be 0x5C).
std::vector<std::string> Parts(const char *path)
{
	std::vector<std::string> parts;
	if (!path)
		return parts;
	std::string text = ToUtf8(path);
	size_t i = 0;
	while (i <= text.size()) {
		size_t j = text.find_first_of("\\/", i);
		if (j == std::string::npos)
			j = text.size();
		if (j > i)
			parts.push_back(text.substr(i, j - i));
		i = j + 1;
	}
	return parts;
}

// Software\<name>\...: the client's own root, group Client.
bool IsClient(const std::vector<std::string> &parts)
{
	return parts.size() >= 2 && strcasecmp(parts[0].c_str(), "Software") == 0
	       && strcasecmp(parts[1].c_str(), BF_APP_NAME) == 0;
}

std::string Group(const std::vector<std::string> &parts)
{
	std::string group;
	size_t first;
	if (IsClient(parts)) {
		group = BF_SETTINGS_CLIENT;
		first = 2;
	} else {
		group = std::string(BF_SETTINGS_REGISTRY) + "/HKLM";
		first = 0;
	}
	for (size_t i = first; i < parts.size(); ++i)
		group += "/" + parts[i];
	return group;
}

void Fill(bf_setting_key &key, const std::vector<std::string> &parts)
{
	key.group = Group(parts);
	key.root = IsClient(parts) && parts.size() == 2;
	key.open = true;
}

std::string AccountName()
{
	char name[256];
	if (bf_settings_account_name(Settings(), name, sizeof(name)) != 1)
		return std::string();
	return name;
}

// "Keep" (CHECKSAVE) belongs to the build that set it. The Keychain lets
// only the code that stored the password read it without asking, and an ad
// hoc signed build is new code after every rebuild: a new build therefore
// starts with "keep" off and does not touch the Keychain, and the access
// prompt can come only when the user ticks "keep" again and logs in (the
// password is then written). Account/SavedBy holds the identity
// (bf_sys_code_identity) of the build that ticked it.
const char *const kSavedBy = "SavedBy";

std::string CodeIdentity()
{
	static std::string identity;
	static bool known = false;
	if (!known) {
		char text[1024];
		bf_sys_code_identity(text, sizeof(text));
		identity = text;
		known = true;
	}
	return identity;
}

bool SavedByThisBuild()
{
	char saved[1024];
	if (bf_settings_get_string(Settings(), BF_SETTINGS_ACCOUNT, kSavedBy, saved, sizeof(saved), NULL) != 1)
		return false;
	return CodeIdentity() == saved;
}

// Copies a value out as RegQueryValueEx did.
bool Deliver(const void *bytes, unsigned int size, void *data, unsigned int *size_io)
{
	if (!data) {
		if (size_io)
			*size_io = size;
		return true;
	}
	if (!size_io)
		return false;
	if (*size_io < size) {
		*size_io = size;
		return false;
	}
	memcpy(data, bytes, size);
	*size_io = size;
	return true;
}

bool DeliverString(const std::string &utf8, void *data, unsigned int *size_io)
{
	std::string bytes = FromUtf8(utf8);
	return Deliver(bytes.c_str(), (unsigned int)bytes.size() + 1, data, size_io);
}

// A string as the client passes it: up to the first NUL within size.
std::string StringOf(const void *data, unsigned int size)
{
	if (!data)
		return std::string();
	return ToUtf8((const char *)data, (int)strnlen((const char *)data, size));
}

}  // namespace

bool bf_setting_open(bf_setting_key &key, const char *path)
{
	key.open = false;
	std::vector<std::string> parts = Parts(path);
	if (!parts.empty() && !bf_settings_has_group(Settings(), Group(parts).c_str())) {
		BF_TRACE(BFT_FILE, "settings: %s absent", Group(parts).c_str());
		return false;
	}
	Fill(key, parts);
	return true;
}

bool bf_setting_create(bf_setting_key &key, const char *path)
{
	key.open = false;
	std::vector<std::string> parts = Parts(path);
	std::string group = Group(parts);
	bool existed = parts.empty() || bf_settings_has_group(Settings(), group.c_str());
	if (!existed && !bf_settings_create_group(Settings(), group.c_str()))
		return false;
	Fill(key, parts);
	return true;
}

void bf_setting_close(bf_setting_key &key)
{
	if (!key.open)
		return;
	key.open = false;
	if (bf_settings_save(Settings()) < 0)
		bf_tracef(BFT_ERR, "settings: cannot save %s", bf_settings_path(Settings()));
}

bool bf_setting_remove(const char *path)
{
	std::vector<std::string> parts = Parts(path);
	if (parts.empty())
		return false;
	return bf_settings_remove(Settings(), Group(parts).c_str(), NULL) != 0;
}

bool bf_setting_read(const bf_setting_key &key, const char *name, void *data, unsigned int *size)
{
	if (!key.open)
		return false;
	if (data && !size)
		return false;
	std::string value = ToUtf8(name ? name : "");

	if (key.root && strcasecmp(value.c_str(), "ID") == 0) {
		std::string account = AccountName();
		if (account.empty())
			return false;
		return DeliverString(account, data, size);
	}
	if (key.root && strcasecmp(value.c_str(), "PASSWORD") == 0) {
		std::string account = AccountName();
		char password[256];
		if (account.empty() || !SavedByThisBuild() || !bf_sys_keychain_available()
		    || bf_sys_keychain_read(account.c_str(), password, sizeof(password)) != 1)
			return false;
		bool ok = DeliverString(password, data, size);
		memset(password, 0, sizeof(password));
		return ok;
	}

	int32_t check = 0;
	if (key.root && strcasecmp(value.c_str(), "CHECKSAVE") == 0
	    && bf_settings_get_int(Settings(), key.group.c_str(), value.c_str(), &check) == 1) {
		int32_t v = check;
		if (v == 1 && !SavedByThisBuild()) {
			BF_TRACE(BFT_FILE, "settings: \"keep\" was set by another build; off, the Keychain left alone");
			v = 0;
		}
		return Deliver(&v, sizeof(v), data, size);
	}

	const char *group = key.group.c_str();
	switch (bf_settings_type(Settings(), group, value.c_str())) {
	case BF_SETTING_INT: {
		int32_t v = 0;
		bf_settings_get_int(Settings(), group, value.c_str(), &v);
		return Deliver(&v, sizeof(v), data, size);
	}
	case BF_SETTING_STRING: {
		size_t length = 0;
		bf_settings_get_string(Settings(), group, value.c_str(), NULL, 0, &length);
		std::string text(length + 1, '\0');
		bf_settings_get_string(Settings(), group, value.c_str(), &text[0], text.size(), NULL);
		text.resize(length);
		return DeliverString(text, data, size);
	}
	case BF_SETTING_DATA: {
		size_t length = 0;
		bf_settings_get_data(Settings(), group, value.c_str(), NULL, 0, &length);
		std::vector<unsigned char> bytes(length ? length : 1);
		bf_settings_get_data(Settings(), group, value.c_str(), bytes.data(), bytes.size(), NULL);
		return Deliver(bytes.data(), (unsigned int)length, data, size);
	}
	default:
		return false;
	}
}

bool bf_setting_write(const bf_setting_key &key, const char *name, bf_setting_type type, const void *data,
                      unsigned int size)
{
	if (!key.open)
		return false;
	if (!data && size)
		return false;
	std::string value = ToUtf8(name ? name : "");
	bool string_type = type == BF_SETTING_STRING;

	if (key.root && strcasecmp(value.c_str(), "ID") == 0) {
		std::string account = string_type ? StringOf(data, size) : std::string();
		return bf_settings_set_account_name(Settings(), account.c_str()) != 0;
	}
	if (key.root && strcasecmp(value.c_str(), "PASSWORD") == 0) {
		std::string account = AccountName();
		std::string password = string_type ? StringOf(data, size) : std::string();
		if (account.empty() || !bf_sys_keychain_available()) {
			BF_TRACE(BFT_FILE, "settings: PASSWORD not stored (no account name or no Keychain)");
			return true;
		}
		if (password.empty()) {
			// Only this build's item: deleting another build's would ask too.
			if (SavedByThisBuild())
				bf_sys_keychain_delete(account.c_str());
		} else if (!bf_sys_keychain_store(account.c_str(), password.c_str()))
			return false;
		return true;
	}
	if (key.root && strcasecmp(value.c_str(), "CHECKSAVE") == 0 && type == BF_SETTING_INT && size == 4) {
		int32_t v;
		memcpy(&v, data, 4);
		if (v == 1)
			bf_settings_set_string(Settings(), BF_SETTINGS_ACCOUNT, kSavedBy, CodeIdentity().c_str());
		else
			bf_settings_remove(Settings(), BF_SETTINGS_ACCOUNT, kSavedBy);
		// and stored as usual below
	}

	const char *group = key.group.c_str();
	int ok;
	if (type == BF_SETTING_INT && size == 4) {
		int32_t v;
		memcpy(&v, data, 4);
		ok = bf_settings_set_int(Settings(), group, value.c_str(), v);
	} else if (string_type) {
		ok = bf_settings_set_string(Settings(), group, value.c_str(), StringOf(data, size).c_str());
	} else {
		ok = bf_settings_set_data(Settings(), group, value.c_str(), data, size);
	}
	return ok != 0;
}
