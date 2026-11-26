// bifrost-setting-key-test: port/client's settings keys (bf_setting_key.h)
// called as the client calls them, and the Settings.plist they leave: the
// checks the registry adapter's test made until M11.4 moved its rules here.
// One line per check, exit status 1 if any fails. Writes only below
// $BF_TEST_OUT/setting-key-test (BF_HOME points there).
#include "bf_setting_key.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf_file.h"
#include "bf_sys.h"
#include "bf_text.h"

namespace fs = std::filesystem;

namespace {

int g_failed = 0;

void Check(bool ok, const char *what)
{
	printf("%s %s\n", ok ? "ok  " : "FAIL", what);
	if (!ok)
		++g_failed;
}

void Skip(const char *what)
{
	printf("skip %s\n", what);
}

// "Yujeo-inteopeiseu" in CP949: a server name as the client writes it.
const char kKoreanCp949[] = "\xC0\xAF\xC0\xFA\xC0\xCE\xC5\xCD\xC6\xE4\xC0\xCC\xBD\xBA";

std::string Utf8Of949(const char *bytes)
{
	char out[256];
	int n = bf_text_to_utf8(949, bytes, (int)strlen(bytes), out, sizeof(out));
	return std::string(out, n > 0 ? (size_t)n : 0);
}

std::string ReadText(const std::string &path)
{
	std::ifstream in(path, std::ios::binary);
	std::stringstream text;
	text << in.rdbuf();
	return text.str();
}

bool PlistInt(bf_settings *s, const char *group, const char *name, int32_t expected)
{
	int32_t v = 0;
	return bf_settings_get_int(s, group, name, &v) == 1 && v == expected;
}

void TestKeys()
{
	const char *root = "Software\\" BF_APP_NAME;
	bf_setting_key key;
	uint32_t value = 0;
	unsigned int size = sizeof(value);
	Check(bf_setting_open(key, root) && bf_setting_read(key, "WIDTH", &value, &size) && value == 1024 && size == 4,
	      "open(Software\\<name>) before any write; WIDTH is the default 1024");
	bf_setting_close(key);
	Check(!key.open && !bf_setting_read(key, "WIDTH", &value, &size), "a closed key reads nothing");

	bf_setting_key never;
	bf_setting_close(never);
	Check(!never.open, "close of a key never opened does nothing (WinMain does it)");
	Check(!bf_setting_open(key, "Software\\" BF_APP_NAME "\\NoSuchKey") && !key.open, "open of an absent key: false");

	// UIWindowMgr's window positions.
	std::string ui = std::string(root) + "\\UIRectInfo";
	Check(bf_setting_create(key, ui.c_str()) && key.group == "Client/UIRectInfo" && !key.root,
	      "create(UIRectInfo): group Client/UIRectInfo");
	int x = 123;
	Check(bf_setting_set(key, "ITEMWNDINFO.X", x), "set: a 4-byte integer");
	float f = -45.0f;
	bf_setting_set(key, "LATITUDE", f);
	int back = 0;
	Check(bf_setting_get(key, "itemwndinfo.x", back) && back == 123, "get: names without regard to case");
	float fback = 0;
	Check(bf_setting_get(key, "LATITUDE", fback) && fback == -45.0f, "a float is stored as its bytes and read back");
	back = 77;
	Check(!bf_setting_get(key, "ITEMWNDINFO.Y", back) && back == 77, "get of an absent value: false, untouched");
	bf_setting_close(key);

	// Session's options: a string written with a fixed length (garbage after the NUL).
	bf_setting_create(key, (std::string(root) + "\\Option").c_str());
	char skin[128];
	memset(skin, 'x', sizeof(skin));
	strcpy(skin, "default");
	Check(bf_setting_set_string(key, "SKINNAME", skin, 128), "set_string(128 bytes)");
	char text[128];
	size = 0;
	Check(bf_setting_read(key, "SKINNAME", NULL, &size) && size == 8, "read of the size only: bytes with the NUL");
	size = 4;
	Check(!bf_setting_read(key, "SKINNAME", text, &size) && size == 8, "read into a small buffer: false and the size");
	size = sizeof(text);
	Check(bf_setting_read(key, "SKINNAME", text, &size) && strcmp(text, "default") == 0 && size == 8, "read a string");
	int asInt = 0;
	Check(bf_setting_get(key, "SKINNAME", asInt) == false, "a 4-byte read of an 8-byte string fails");
	unsigned char guid[16];
	for (int i = 0; i < 16; ++i)
		guid[i] = (unsigned char)(i * 17);
	unsigned char gback[16] = {0};
	size = sizeof(gback);
	Check(bf_setting_write(key, "GUIDTEST", BF_SETTING_DATA, guid, 16) && bf_setting_read(key, "GUIDTEST", gback, &size)
	      && size == 16 && memcmp(guid, gback, 16) == 0, "write/read data");
	short shortInt = 5;
	Check(bf_setting_write(key, "SHORT", BF_SETTING_INT, &shortInt, 2)
	      && bf_settings_type(bf_settings_shared(), "Client/Option", "SHORT") == BF_SETTING_DATA,
	      "an integer write of 2 bytes is data (as REG_DWORD with size 2)");
	bf_setting_close(key);

	// Bookmarks keyed by server and character name (CP949), then removed (UIFrameWnd2.cpp).
	std::string server1 = std::string(root) + "\\BookMark\\" + kKoreanCp949 + "\\Hero";
	std::string server2 = std::string(root) + "\\BookMark\\Other\\Hero";
	int page = 7;
	Check(bf_setting_create(key, server1.c_str()) && bf_setting_set(key, "book", page),
	      "create of a new key with a CP949 name");
	bf_setting_close(key);
	bf_setting_create(key, server2.c_str());
	bf_setting_set(key, "book", page);
	bf_setting_close(key);
	Check(bf_setting_create(key, server2.c_str()), "create of an existing key");
	bf_setting_close(key);
	Check(bf_setting_remove(server2.c_str()) && !bf_setting_open(key, server2.c_str()), "remove: the key is gone");

	// Any other path goes below Registry/HKLM.
	bf_setting_create(key, "Software\\Elsewhere");
	int other = 5;
	bf_setting_set(key, "Value", other);
	Check(key.group == "Registry/HKLM/Software/Elsewhere", "another path: Registry/HKLM/<path>");
	bf_setting_close(key);

	// The login window: ID is the account name, PASSWORD the Keychain.
	bf_setting_key login;
	bf_setting_create(login, root);
	Check(login.root && login.group == "Client", "the root: group Client");
	char id[26];
	memset(id, 0, sizeof(id));
	strcpy(id, "bf_setting_key_test");
	int checkSave = 1;
	Check(bf_setting_set_string(login, "ID", id, 25) && bf_setting_set(login, "CHECKSAVE", checkSave),
	      "set_string(ID) as UILoginWnd::WriteToReg");
	char idBack[26] = {0};
	size = 25;
	char account[64] = {0};
	Check(bf_setting_read(login, "ID", idBack, &size) && strcmp(idBack, "bf_setting_key_test") == 0
	      && bf_settings_account_name(bf_settings_shared(), account, sizeof(account)) == 1
	      && strcmp(account, "bf_setting_key_test") == 0, "  ... read back, stored as Account/Name");
	char password[26] = {0};
	if (bf_sys_keychain_available()) {
		Check(bf_setting_set_string(login, "PASSWORD", "secret", 7)
		      && (size = 25, bf_setting_read(login, "PASSWORD", password, &size)) && strcmp(password, "secret") == 0,
		      "PASSWORD: through the Keychain");
		bf_setting_set_string(login, "PASSWORD", "", 1);
		Check(!bf_setting_read(login, "PASSWORD", password, &size), "PASSWORD \"\": the Keychain item is deleted");
	} else {
		size = 25;
		Check(!bf_setting_read(login, "PASSWORD", password, &size), "PASSWORD without a Keychain: not found (as the client starts)");
		Skip("PASSWORD Keychain round trip: the Keychain is locked or out of reach in this session");
	}

	// "Keep" belongs to the build that ticked it (Account/SavedBy): another
	// build reads it as off and leaves the Keychain alone.
	int checkBack = -1;
	char savedBy[1024] = {0};
	Check(bf_setting_get(login, "CHECKSAVE", checkBack) && checkBack == 1
	      && bf_settings_get_string(bf_settings_shared(), "Account", "SavedBy", savedBy, sizeof(savedBy), NULL) == 1,
	      "CHECKSAVE 1: read back by the same build, its identity in Account/SavedBy");
	bf_settings_set_string(bf_settings_shared(), "Account", "SavedBy", "another build");
	checkBack = -1;
	size = 25;
	Check(bf_setting_get(login, "CHECKSAVE", checkBack) && checkBack == 0
	      && !bf_setting_read(login, "PASSWORD", password, &size),
	      "set by another build: CHECKSAVE reads 0, PASSWORD absent (no Keychain access)");
	checkSave = 1;
	bf_setting_set(login, "CHECKSAVE", checkSave);
	checkBack = -1;
	Check(bf_setting_get(login, "CHECKSAVE", checkBack) && checkBack == 1, "ticked again: this build's");
	checkSave = 0;
	bf_setting_set(login, "CHECKSAVE", checkSave);
	Check(bf_settings_type(bf_settings_shared(), "Account", "SavedBy") == BF_SETTING_NONE,
	      "CHECKSAVE 0: Account/SavedBy removed");
	checkSave = 1;
	bf_setting_set(login, "CHECKSAVE", checkSave);   // as the plist checks below expect
	bf_setting_close(login);

	// What the plist holds.
	bf_settings *plist = bf_settings_open_file(bf_settings_path(bf_settings_shared()));
	std::string korean = Utf8Of949(kKoreanCp949);
	char skinBack[64] = {0};
	Check(plist && bf_settings_load(plist) == 1, "close saved Settings.plist");
	Check(PlistInt(plist, "Client/UIRectInfo", "ITEMWNDINFO.X", 123), "  ... Client/UIRectInfo/ITEMWNDINFO.X = 123");
	Check(bf_settings_get_string(plist, "Client/Option", "SKINNAME", skinBack, sizeof(skinBack), NULL) == 1
	      && strcmp(skinBack, "default") == 0, "  ... Client/Option/SKINNAME = \"default\"");
	Check(PlistInt(plist, ("Client/BookMark/" + korean + "/Hero").c_str(), "book", 7),
	      "  ... Client/BookMark/<server in UTF-8>/Hero/book = 7");
	Check(!bf_settings_has_group(plist, "Client/BookMark/Other/Hero"), "  ... the removed key is gone");
	Check(PlistInt(plist, "Registry/HKLM/Software/Elsewhere", "Value", 5), "  ... Registry/HKLM/Software/Elsewhere/Value = 5");
	Check(PlistInt(plist, "Client", "CHECKSAVE", 1)
	      && bf_settings_type(plist, "Client", "ID") == BF_SETTING_NONE
	      && bf_settings_type(plist, "Client", "PASSWORD") == BF_SETTING_NONE
	      && bf_settings_account_name(plist, account, sizeof(account)) == 1 && strcmp(account, "bf_setting_key_test") == 0,
	      "  ... Account/Name holds the ID; no ID or PASSWORD value in Client");
	bf_settings_close(plist);
	std::string raw = ReadText(bf_settings_path(bf_settings_shared()));
	Check(raw.find("secret") == std::string::npos, "  ... no password in the file");
}

}  // namespace

int main()
{
	const char *out = getenv("BF_TEST_OUT");
	std::string base = std::string(out && *out ? out : ".") + "/setting-key-test";
	std::error_code ignored;
	fs::remove_all(base, ignored);
	std::string home = base + "/home";
	fs::create_directories(home);
	setenv("BF_HOME", fs::canonical(home).string().c_str(), 1);
	setenv("BF_NO_ALERTS", "1", 1);
	printf("home %s\n", home.c_str());

	TestKeys();

	printf("%s: %d failed\n", g_failed ? "FAIL" : "PASS", g_failed);
	return g_failed ? 1 : 0;
}
