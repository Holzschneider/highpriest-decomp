// bifrost-compat-win32-test: the win32 adapter called as the client calls it.
// One line per check, exit status 1 if any fails. Writes only below
// $BF_TEST_OUT/compat-win32-test (BF_HOME points there); reads the real game
// folder ($BF_CLIENT_DIR or ~/Games/<name>) for the data.grf check, which is
// skipped without it.
#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include <shellapi.h>
#include <objbase.h>
#include <nb30.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <thread>
#include <string>
#include <vector>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bf_sys.h"
#include "bf_text.h"

extern "C" {
int client_init_base_dir(void);
int client_write_ping_log(const char *line);
int client_write_chat_log(const char *title, char *path_out, int path_size);
int client_write_screenshot(char *path_out, int path_size);
int client_read_data_ini(char *first_line, int size);
int client_write_into_game_folder(int *error);
int client_cache_cleanup(void);
int client_std_library(void);
}

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

bool Exists(const std::string &path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

void WriteText(const std::string &path, const std::string &text)
{
	fs::create_directories(fs::path(path).parent_path());
	std::ofstream(path, std::ios::binary) << text;
}

std::string ReadText(const std::string &path)
{
	std::ifstream in(path, std::ios::binary);
	return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// 유저인터페이스 ("user interface", the client's UI folder) in CP949.
const char kKoreanCp949[] = "\xC0\xAF\xC0\xFA\xC0\xCE\xC5\xCD\xC6\xE4\xC0\xCC\xBD\xBA";

std::string Utf8Of949(const char *bytes)
{
	char out[256];
	int n = bf_text_to_utf8(949, bytes, (int)strlen(bytes), out, sizeof(out));
	return std::string(out, n > 0 ? (size_t)n : 0);
}

std::string g_game, g_home, g_documents, g_pictures, g_support;

// ---- Files ---------------------------------------------------------------------

void TestFiles()
{
	char buf[64] = {0};
	DWORD done = 0;
	HANDLE h = CreateFileA("data\\README.TXT", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
	                       FILE_ATTRIBUTE_NORMAL, NULL);
	Check(h != INVALID_HANDLE_VALUE && GetFileSize(h, NULL) == 10, "CreateFile: backslashes and any case find data/readme.txt");
	Check(ReadFile(h, buf, sizeof(buf), &done, NULL) && done == 10 && memcmp(buf, "0123456789", 10) == 0,
	      "ReadFile: whole file, short read at the end");
	Check(SetFilePointer(h, 2, NULL, FILE_BEGIN) == 2 && ReadFile(h, buf, 3, &done, NULL) && done == 3
	      && memcmp(buf, "234", 3) == 0, "SetFilePointer(FILE_BEGIN) + ReadFile");
	LONG high = -1;   // with a high part the distance is high:low, here -1
	Check(SetFilePointer(h, -1, &high, FILE_END) == 9 && high == 0 && ReadFile(h, buf, 5, &done, NULL) && done == 1,
	      "SetFilePointer(FILE_END, -1) with a high part");
	Check(SetFilePointer(h, -20, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER && GetLastError() == ERROR_NEGATIVE_SEEK,
	      "SetFilePointer before the start: ERROR_NEGATIVE_SEEK");
	Check(CloseHandle(h) && !CloseHandle(h) && GetLastError() == ERROR_INVALID_HANDLE,
	      "CloseHandle, and a second CloseHandle is ERROR_INVALID_HANDLE");

	h = CreateFileA("data\\missing.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	Check(h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND, "CreateFile of a missing file: ERROR_FILE_NOT_FOUND");

	std::string korean = std::string(kKoreanCp949) + "\\Title.bmp";
	h = CreateFileA(korean.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	Check(h != INVALID_HANDLE_VALUE && GetFileSize(h, NULL) == 3, "CreateFile: a CP949 folder name (stored as UTF-8)");
	CloseHandle(h);

	// CFile::Open(modeWrite) on a game path: refused, nothing written.
	h = CreateFileA("data\\written.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
	                FILE_ATTRIBUTE_NORMAL, NULL);
	Check(h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED && !Exists(g_game + "/data/written.txt"),
	      "CreateFile(CREATE_ALWAYS) into the game folder: ERROR_ACCESS_DENIED, nothing created");
	Check(!DeleteFileA("data\\README.TXT") && GetLastError() == ERROR_ACCESS_DENIED && Exists(g_game + "/data/readme.txt"),
	      "DeleteFile in the game folder: ERROR_ACCESS_DENIED");

	h = CreateFileA("savedata\\test.bin", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	Check(h != INVALID_HANDLE_VALUE && GetLastError() == ERROR_SUCCESS && WriteFile(h, "abcdef", 6, &done, NULL)
	      && done == 6, "CreateFile + WriteFile into a write location");
	CloseHandle(h);
	Check(ReadText(g_support + "/savedata/test.bin") == "abcdef", "  ... the file is in Application Support/savedata");
	h = CreateFileA("SAVEDATA\\test.bin", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	Check(h != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS && GetFileSize(h, NULL) == 6,
	      "CreateFile(OPEN_ALWAYS) on it: ERROR_ALREADY_EXISTS, contents kept");
	Check(SetFilePointer(h, 3, NULL, FILE_BEGIN) == 3 && SetEndOfFile(h) && GetFileSize(h, NULL) == 3, "SetEndOfFile");
	CloseHandle(h);
	h = CreateFileA("savedata\\test.bin", GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
	Check(h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS, "CreateFile(CREATE_NEW) on it: ERROR_FILE_EXISTS");
	Check(DeleteFileA("savedata\\test.bin") && !Exists(g_support + "/savedata/test.bin"), "DeleteFile in a write location");

	Check(CreateDirectoryA("Chat", NULL) && !CreateDirectoryA("Chat", NULL) && GetLastError() == ERROR_ALREADY_EXISTS
	      && Exists(g_documents + "/Chat"), "CreateDirectory: Chat in Documents, again ERROR_ALREADY_EXISTS");
	Check(GetFileAttributesA("data") == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY)
	      && GetFileAttributesA("nothing") == INVALID_FILE_ATTRIBUTES, "GetFileAttributes");
}

void TestFind()
{
	WIN32_FIND_DATAA wfd;
	HANDLE h = FindFirstFileA("data\\texture\\*.bmp", &wfd);
	Check(h != INVALID_HANDLE_VALUE && strcmp(wfd.cFileName, "Foo.BMP") == 0 && wfd.nFileSizeLow == 4
	      && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY), "FindFirstFile: wildcard without regard to case");
	Check(FindNextFileA(h, &wfd) && strcmp(wfd.cFileName, "bar.bmp") == 0, "FindNextFile: the next match");
	Check(!FindNextFileA(h, &wfd) && GetLastError() == ERROR_NO_MORE_FILES, "FindNextFile at the end: ERROR_NO_MORE_FILES");
	Check(FindClose(h), "FindClose");

	h = FindFirstFileA("data\\*.*", &wfd);
	int folders = 0, files = 0;
	if (h != INVALID_HANDLE_VALUE) {
		do {
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				++folders;
			else
				++files;
		} while (FindNextFileA(h, &wfd));
		FindClose(h);
	}
	Check(folders == 1 && files == 1, "FindFirstFile(\"*.*\"): folders flagged FILE_ATTRIBUTE_DIRECTORY (skin.cpp)");

	std::string korean = std::string(kKoreanCp949) + "\\*.bmp";
	h = FindFirstFileA(korean.c_str(), &wfd);
	Check(h != INVALID_HANDLE_VALUE && strcmp(wfd.cFileName, "Title.bmp") == 0, "FindFirstFile below a CP949 folder");
	FindClose(h);

	h = FindFirstFileA("data\\*.wav", &wfd);
	Check(h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND, "FindFirstFile without match: ERROR_FILE_NOT_FOUND");
	Check(!FindClose(h), "FindClose(INVALID_HANDLE_VALUE) fails safely (base/Util.cpp does it)");
}

// CMemMapFile (base/GPak.cpp): CreateFile, GetFileSize, CreateFileMapping,
// MapViewOfFile at offsets aligned to the allocation granularity.
bool MapLikeGPak(const char *name, const char *magic, uint64_t *size_out, const char **why)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	unsigned long mask = 0xFFFFFFFF - (si.dwAllocationGranularity - 1);
	HANDLE file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return *why = "CreateFile", false;
	DWORD high = 0;
	unsigned long size = GetFileSize(file, &high);
	*size_out = ((uint64_t)high << 32) | size;
	HANDLE map = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, name);
	if (!map)
		return CloseHandle(file), *why = "CreateFileMapping", false;
	bool ok = true;
	unsigned long window = 0x20000000;
	const BYTE *view = (const BYTE *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, window < size ? window : 0);
	if (!view || (magic && memcmp(view, magic, strlen(magic) + 1) != 0))
		ok = false, *why = "view at 0";
	if (view)
		UnmapViewOfFile(view);
	// A view at the last aligned offset, to the end (the offset + size > file size case).
	unsigned long last = size ? (size - 1) & mask : 0;
	view = (const BYTE *)MapViewOfFile(map, FILE_MAP_READ, 0, last, 0);
	if (ok && (!view || (magic == NULL && view[size - 1 - last] != (BYTE)((size - 1) * 7))))
		ok = false, *why = "view at the end";
	if (view)
		UnmapViewOfFile(view);
	if (ok && (MapViewOfFile(map, FILE_MAP_READ, 0, 1, 16) != NULL || GetLastError() != ERROR_MAPPED_ALIGNMENT))
		ok = false, *why = "unaligned view";
	// A view kept across CloseHandle of the mapping stays readable.
	view = (const BYTE *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
	CloseHandle(map);
	CloseHandle(file);
	if (ok && (!view || (magic && view[0] != (BYTE)magic[0]) || (!magic && view[1] != 7)))
		ok = false, *why = "view after CloseHandle";
	if (ok && !(UnmapViewOfFile(view) && !UnmapViewOfFile(view)))
		ok = false, *why = "UnmapViewOfFile twice";
	return ok;
}

void TestMapping()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	Check(si.dwAllocationGranularity == 0x10000 && si.dwPageSize >= 4096, "GetSystemInfo: granularity 64 KiB");

	const char *why = "";
	uint64_t size = 0;
	Check(MapLikeGPak("BIG.bin", NULL, &size, &why) && size == 300001,
	      "CreateFileMapping + MapViewOfFile as CMemMapFile does, on a 300001-byte file");
	if (*why)
		printf("     (failed at: %s)\n", why);

	std::string real;
	const char *env = getenv("BF_CLIENT_DIR");
	real = env && *env ? env : std::string(getenv("HOME")) + "/Games/" BF_APP_NAME;
	if (!Exists(real + "/data.grf")) {
		Skip("data.grf: no game data");
		return;
	}
	bf_file_set_game_folder(real.c_str());
	why = "";
	bool ok = MapLikeGPak("DATA.GRF", "Master of Magic", &size, &why);
	printf("     data.grf: %llu bytes\n", (unsigned long long)size);
	Check(ok && size > 0x20000000, "data.grf from the game folder: mapped whole, read-only, \"Master of Magic\" at 0");
	if (*why)
		printf("     (failed at: %s)\n", why);
	bf_file_set_game_folder(g_game.c_str());
}

// ---- The C library, as the client calls it ---------------------------------------

void TestCrt()
{
	char path[512];
	Check(client_init_base_dir(), "GetCurrentDirectory into g_baseDir3: the game folder");
	char tiny[4];
	Check(GetCurrentDirectoryA(sizeof(tiny), tiny) == g_game.size() + 1, "GetCurrentDirectory: too small a buffer gives the size");

	Check(client_write_ping_log("ping 12") && client_write_ping_log("ping 13")
	      && ReadText(g_documents + "/PingLog.txt") == "ping 12\nping 13\n",
	      "fopen(\"PingLog.txt\", \"at\"): appended in Documents");
	Check(client_write_chat_log("chat", path, sizeof(path)) && Exists(g_documents + "/Chat/chat000.txt")
	      && client_write_chat_log("chat", path, sizeof(path)) && Exists(g_documents + "/Chat/chat001.txt"),
	      "access + _mkdir + fopen of g_baseDir3\\Chat\\...: chat logs land in Documents/Chat");
	Check(client_write_screenshot(path, sizeof(path)) && Exists(g_pictures + "/screen000.jpg")
	      && client_write_screenshot(path, sizeof(path)) && Exists(g_pictures + "/screen001.jpg")
	      && !Exists(g_game + "/ScreenShot"),
	      "g_baseDir3\\ScreenShot\\screen%03d.jpg: screenshots land in Pictures, numbered on");
	char line[64] = {0};
	Check(client_read_data_ini(line, sizeof(line)) && strncmp(line, "[Data]", 6) == 0, "fopen(\"DATA.INI\", \"r\") from the game folder");
	int error = 0;
	Check(!client_write_into_game_folder(&error) && error == EACCES && !Exists(g_game + "/data/bifrost_must_not_exist.txt"),
	      "fopen of a game-folder path for writing: refused, EACCES");
	Check(client_cache_cleanup() && !Exists(g_support + "/_tmpskin_/test"),
	      "_mkdir, rename, _chmod, _unlink, remove, _rmdir in _tmpskin_ (Application Support)");
	Check(client_std_library(), "std::remove and std::list::remove under the renaming macros");

	char exe[1024];
	DWORD n = GetModuleFileNameA(NULL, exe, sizeof(exe));
	Check(n > 0 && exe[0] == '/' && Exists(exe), "GetModuleFileName: the executable's path");
}

// ---- Registry ----------------------------------------------------------------------

bool PlistInt(bf_settings *s, const char *group, const char *name, int32_t expected)
{
	int32_t v = 0;
	return bf_settings_get_int(s, group, name, &v) == 1 && v == expected;
}

void TestRegistry()
{
	const char *root = "Software\\" BF_APP_NAME;
	HKEY key = NULL;
	DWORD type = 0, value = 0, size = sizeof(value);
	Check(RegOpenKeyExA(HKEY_LOCAL_MACHINE, root, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS
	      && RegQueryValueExA(key, "WIDTH", 0, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS
	      && type == REG_DWORD && value == 1024 && size == 4,
	      "RegOpenKeyEx(HKLM\\Software\\<name>) before any write; WIDTH is the default 1024");
	RegCloseKey(key);

	HKEY bad = (HKEY)0x1234;
	Check(RegCloseKey(bad) == ERROR_INVALID_HANDLE && RegCloseKey(NULL) == ERROR_INVALID_HANDLE,
	      "RegCloseKey of a key never opened: ERROR_INVALID_HANDLE (WinMain does it)");
	Check(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Bifrost\\NoSuchKey", 0, KEY_QUERY_VALUE, &key) == ERROR_FILE_NOT_FOUND
	      && key == NULL, "RegOpenKeyEx of an absent key: ERROR_FILE_NOT_FOUND");

	// UIWindowMgr's window positions.
	std::string ui = std::string(root) + "\\UIRectInfo";
	DWORD disposition = 0;
	Check(RegCreateKeyExA(HKEY_LOCAL_MACHINE, ui.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition) == ERROR_SUCCESS,
	      "RegCreateKeyEx(UIRectInfo)");
	DWORD x = 123;
	Check(RegSetValueExA(key, "ITEMWNDINFO.X", 0, REG_DWORD, (LPBYTE)&x, 4) == ERROR_SUCCESS, "RegSetValueEx(REG_DWORD)");
	value = 0;
	size = 4;
	Check(RegQueryValueExA(key, "itemwndinfo.x", 0, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS && value == 123
	      && type == REG_DWORD, "RegQueryValueEx(REG_DWORD), names without regard to case");
	size = 0;
	Check(RegQueryValueExA(key, "ITEMWNDINFO.Y", 0, &type, (LPBYTE)&value, &size) == ERROR_FILE_NOT_FOUND,
	      "RegQueryValueEx of an absent value: ERROR_FILE_NOT_FOUND");
	RegCloseKey(key);

	// Session's options: a REG_SZ written with a fixed length (garbage after the NUL).
	RegCreateKeyExA(HKEY_LOCAL_MACHINE, (std::string(root) + "\\Option").c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL,
	                &key, &disposition);
	char skin[128];
	memset(skin, 'x', sizeof(skin));
	strcpy(skin, "default");
	Check(RegSetValueExA(key, "SKINNAME", 0, REG_SZ, (LPBYTE)skin, 128) == ERROR_SUCCESS, "RegSetValueEx(REG_SZ, 128 bytes)");
	char text[128];
	size = 0;
	Check(RegQueryValueExA(key, "SKINNAME", 0, &type, NULL, &size) == ERROR_SUCCESS && size == 8 && type == REG_SZ,
	      "RegQueryValueEx size query: bytes with the NUL");
	size = 4;
	Check(RegQueryValueExA(key, "SKINNAME", 0, &type, (LPBYTE)text, &size) == ERROR_MORE_DATA && size == 8,
	      "RegQueryValueEx into a small buffer: ERROR_MORE_DATA and the size");
	size = sizeof(text);
	Check(RegQueryValueExA(key, "SKINNAME", 0, &type, (LPBYTE)text, &size) == ERROR_SUCCESS && strcmp(text, "default") == 0,
	      "RegQueryValueEx(REG_SZ)");
	unsigned char guid[16];
	for (int i = 0; i < 16; ++i)
		guid[i] = (unsigned char)(i * 17);
	unsigned char back[16] = {0};
	size = sizeof(back);
	Check(RegSetValueExA(key, "GUIDTEST", 0, REG_BINARY, guid, 16) == ERROR_SUCCESS
	      && RegQueryValueExA(key, "GUIDTEST", 0, &type, back, &size) == ERROR_SUCCESS && type == REG_BINARY
	      && size == 16 && memcmp(guid, back, 16) == 0, "RegSetValueEx/RegQueryValueEx(REG_BINARY)");
	Check(RegDeleteValueA(key, "GUIDTEST") == ERROR_SUCCESS && RegQueryValueExA(key, "GUIDTEST", 0, &type, NULL, &size)
	      == ERROR_FILE_NOT_FOUND, "RegDeleteValue");
	RegCloseKey(key);

	// Bookmarks keyed by server and character name (CP949), then deleted (UIFrameWnd2.cpp).
	std::string server1 = std::string(root) + "\\BookMark\\" + kKoreanCp949 + "\\Hero";
	std::string server2 = std::string(root) + "\\BookMark\\Other\\Hero";
	DWORD page = 7;
	RegCreateKeyExA(HKEY_LOCAL_MACHINE, server1.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition);
	Check(disposition == REG_CREATED_NEW_KEY && RegSetValueExA(key, "book", 0, REG_DWORD, (BYTE *)&page, 4) == ERROR_SUCCESS,
	      "RegCreateKeyEx of a new key with a CP949 name: REG_CREATED_NEW_KEY");
	RegCloseKey(key);
	RegCreateKeyExA(HKEY_LOCAL_MACHINE, server2.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition);
	RegSetValueExA(key, "book", 0, REG_DWORD, (BYTE *)&page, 4);
	RegCloseKey(key);
	Check(RegCreateKeyExA(HKEY_LOCAL_MACHINE, server2.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition)
	      == ERROR_SUCCESS && disposition == REG_OPENED_EXISTING_KEY, "RegCreateKeyEx of an existing key: REG_OPENED_EXISTING_KEY");
	RegCloseKey(key);
	Check(RegDeleteKeyA(HKEY_LOCAL_MACHINE, server2.c_str()) == ERROR_SUCCESS
	      && RegOpenKeyExA(HKEY_LOCAL_MACHINE, server2.c_str(), 0, KEY_QUERY_VALUE, &key) == ERROR_FILE_NOT_FOUND,
	      "RegDeleteKey: the key is gone");

	// Any other key goes below Registry/.
	RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Elsewhere", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition);
	DWORD other = 5;
	RegSetValueExA(key, "Value", 0, REG_DWORD, (BYTE *)&other, 4);
	RegCloseKey(key);

	// The login window: ID is the account name, PASSWORD the Keychain.
	HKEY login;
	RegCreateKeyExA(HKEY_LOCAL_MACHINE, root, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &login, &disposition);
	char id[26];
	memset(id, 0, sizeof(id));
	strcpy(id, "bf_compat_win32_test");
	DWORD checkSave = 1;
	Check(RegSetValueExA(login, "ID", 0, REG_SZ, (LPBYTE)id, 25) == ERROR_SUCCESS
	      && RegSetValueExA(login, "CHECKSAVE", 0, REG_DWORD, (LPBYTE)&checkSave, 4) == ERROR_SUCCESS,
	      "RegSetValueEx(ID) as UILoginWnd::WriteToReg");
	char idBack[26] = {0};
	size = 25;
	char account[64] = {0};
	Check(RegQueryValueExA(login, "ID", 0, &type, (LPBYTE)idBack, &size) == ERROR_SUCCESS
	      && strcmp(idBack, "bf_compat_win32_test") == 0
	      && bf_settings_account_name(bf_settings_shared(), account, sizeof(account)) == 1
	      && strcmp(account, "bf_compat_win32_test") == 0, "  ... read back, stored as Account/Name");
	char password[26] = {0};
	if (bf_sys_keychain_available()) {
		Check(RegSetValueExA(login, "PASSWORD", 0, REG_SZ, (LPBYTE)"secret", 7) == ERROR_SUCCESS
		      && (size = 25, RegQueryValueExA(login, "PASSWORD", 0, &type, (LPBYTE)password, &size)) == ERROR_SUCCESS
		      && strcmp(password, "secret") == 0, "PASSWORD: through the Keychain");
		RegSetValueExA(login, "PASSWORD", 0, REG_SZ, (LPBYTE)"", 1);
		Check(RegQueryValueExA(login, "PASSWORD", 0, &type, (LPBYTE)password, &size) == ERROR_FILE_NOT_FOUND,
		      "PASSWORD \"\": the Keychain item is deleted");
	} else {
		size = 25;
		Check(RegQueryValueExA(login, "PASSWORD", 0, &type, (LPBYTE)password, &size) == ERROR_FILE_NOT_FOUND,
		      "PASSWORD without a Keychain: not found (as the client starts)");
		Skip("PASSWORD Keychain round trip: the Keychain is locked or out of reach in this session");
	}
	Check(RegCloseKey(login) == ERROR_SUCCESS, "RegCloseKey saves");

	// What the plist holds.
	bf_settings *plist = bf_settings_open_file(bf_settings_path(bf_settings_shared()));
	std::string korean = Utf8Of949(kKoreanCp949);
	char skinBack[64] = {0};
	Check(plist && bf_settings_load(plist) == 1, "Settings.plist written");
	Check(PlistInt(plist, "Client/UIRectInfo", "ITEMWNDINFO.X", 123), "  ... Client/UIRectInfo/ITEMWNDINFO.X = 123");
	Check(bf_settings_get_string(plist, "Client/Option", "SKINNAME", skinBack, sizeof(skinBack), NULL) == 1
	      && strcmp(skinBack, "default") == 0, "  ... Client/Option/SKINNAME = \"default\"");
	Check(PlistInt(plist, ("Client/BookMark/" + korean + "/Hero").c_str(), "book", 7),
	      "  ... Client/BookMark/<server in UTF-8>/Hero/book = 7");
	Check(!bf_settings_has_group(plist, "Client/BookMark/Other/Hero"), "  ... the deleted key is gone");
	Check(PlistInt(plist, "Registry/HKCU/Software/Elsewhere", "Value", 5), "  ... Registry/HKCU/Software/Elsewhere/Value = 5");
	Check(PlistInt(plist, "Client", "CHECKSAVE", 1)
	      && bf_settings_type(plist, "Client", "ID") == BF_SETTING_NONE
	      && bf_settings_type(plist, "Client", "PASSWORD") == BF_SETTING_NONE
	      && bf_settings_account_name(plist, account, sizeof(account)) == 1 && strcmp(account, "bf_compat_win32_test") == 0,
	      "  ... Account/Name holds the ID; no ID or PASSWORD value in Client");
	bf_settings_close(plist);
	std::string raw = ReadText(bf_settings_path(bf_settings_shared()));
	Check(raw.find("secret") == std::string::npos, "  ... no password in the file");
}

// ---- Last error ------------------------------------------------------------------------
// Threads, critical sections and the clock left with M11.1: the client calls
// port/native/sys (bf_sys_thread_*, bf_sys_mutex_*, bf_clock_*) directly.

void TestLastError()
{
	SetLastError(1234);
	DWORD seen = 0;
	std::thread other([&] { SetLastError(5); seen = GetLastError(); });
	other.join();
	Check(GetLastError() == 1234 && seen == 5, "GetLastError is per thread");
}

// ---- Strings, memory, the rest -----------------------------------------------------------

void TestMisc()
{
	WCHAR wide[16];
	const char *two = "\xC0\xAF\xC0\xFA";   // 유저
	Check(MultiByteToWideChar(949, 0, two, -1, NULL, 0) == 3 && MultiByteToWideChar(949, 0, two, -1, wide, 16) == 3
	      && wide[0] == 0xC720 && wide[1] == 0xC800 && wide[2] == 0, "MultiByteToWideChar(949): count and conversion");
	char narrow[16];
	Check(WideCharToMultiByte(949, 0, wide, -1, narrow, sizeof(narrow), NULL, NULL) == 5 && strcmp(narrow, two) == 0,
	      "WideCharToMultiByte(949): back");
	Check(MultiByteToWideChar(CP_ACP, 0, two, 4, wide, 1) == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
	      "MultiByteToWideChar into a small buffer: ERROR_INSUFFICIENT_BUFFER");
	Check(IsDBCSLeadByte(0xC0) && !IsDBCSLeadByte('A') && GetACP() == 949, "IsDBCSLeadByte with CP_ACP 949");
	bf_file_set_user_codepage(1252);
	Check(!IsDBCSLeadByte(0xC0) && GetACP() == 1252
	      && MultiByteToWideChar(CP_ACP, 0, "\xE9", 1, wide, 16) == 1 && wide[0] == 0xE9,
	      "CP_ACP follows bf_file_set_user_codepage (1252)");
	// WinMain's COM start-up, and CLoginMode's MAC-address query (fails: "111111111111").
	Check(CoInitialize(NULL) == S_OK, "CoInitialize(NULL): S_OK");
	CoUninitialize();
	NCB ncb;
	memset(&ncb, 0, sizeof(ncb));
	ncb.ncb_command = NCBASTAT;
	Check(Netbios(&ncb) != 0 && ncb.ncb_retcode != 0, "Netbios(NCBASTAT): fails, as the login code handles");
	bf_file_set_user_codepage(949);
	Check(MulDiv(10, 3, 4) == 8 && MulDiv(-10, 3, 4) == -8 && MulDiv(1, 2, 0) == -1, "MulDiv rounds, -1 for /0");
	Check(lstrcmpiA("Gulim.BMP", "gulim.bmp") == 0 && lstrcmpiA("a", "B") < 0, "lstrcmpiA");

	unsigned char *v = (unsigned char *)VirtualAlloc(NULL, 100000 + 8, MEM_COMMIT, PAGE_READWRITE);
	bool zero = v != NULL;
	for (int i = 0; zero && i < 100008; ++i)
		zero = v[i] == 0;
	Check(zero && VirtualFree(v, 0, MEM_RELEASE), "VirtualAlloc: zeroed; VirtualFree");
	HGLOBAL g = GlobalAlloc(GMEM_DDESHARE, 32);
	char *p = (char *)GlobalLock(g);
	Check(p && GlobalSize(g) == 32 && (strcpy(p, "clip"), true) && !GlobalUnlock(g) && GlobalFree(g) == NULL,
	      "GlobalAlloc, GlobalLock, GlobalSize, GlobalUnlock, GlobalFree (the clipboard's text)");
	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	Check(ms.dwTotalPhys > 163000000, "GlobalMemoryStatus: not a low-memory machine");
	OSVERSIONINFOA osv;
	osv.dwOSVersionInfoSize = sizeof(osv);
	Check(GetVersionExA(&osv) && osv.dwPlatformId == VER_PLATFORM_WIN32_NT && osv.dwMajorVersion == 5, "GetVersionEx: Windows XP");
	Check(LoadLibraryA("npkcrypt.dll") == NULL && GetLastError() == ERROR_MOD_NOT_FOUND, "LoadLibrary: no DLLs");
	HCRYPTPROV prov;
	Check(!CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT), "CryptAcquireContext: no provider");
	OutputDebugStringA("compat-win32 test\n");

	DWORD start = bf_clock_ticks();
	Check(MessageBoxA(NULL, "Window Creation Failed", "ERROR", 0) == IDOK
	      && MessageBoxA(NULL, "patch.exe is changed. Press ok to restart", "Alert", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK
	      && MessageBoxA(NULL, "\xC0\xAF\xC0\xFA?", "Keycrypt", 0x1044) == IDYES && bf_clock_ticks() - start < 2000,
	      "MessageBox with BF_NO_ALERTS: logged, answered with the first button at once");
	Check((INT_PTR)ShellExecuteA(NULL, "open", "http://127.0.0.1/", NULL, NULL, SW_SHOWNORMAL) > 32,
	      "ShellExecute(open, URL) with BF_NO_OPEN: done");
	Check((INT_PTR)ShellExecuteA(NULL, "open", "data\\nothing.txt", NULL, NULL, SW_SHOWNORMAL) == SE_ERR_FNF,
	      "ShellExecute of a missing file: SE_ERR_FNF");
}

}  // namespace

int main()
{
	const char *out = getenv("BF_TEST_OUT");
	std::string base = std::string(out && *out ? out : ".") + "/compat-win32-test";
	std::error_code ignored;
	fs::remove_all(base, ignored);
	g_home = base + "/home";
	g_game = base + "/game";
	fs::create_directories(g_home);
	g_home = fs::canonical(g_home).string();
	g_game = fs::weakly_canonical(g_game).string();
	setenv("BF_HOME", g_home.c_str(), 1);
	setenv("BF_NO_ALERTS", "1", 1);
	setenv("BF_NO_OPEN", "1", 1);
	g_documents = g_home + "/Documents/" BF_APP_NAME;
	g_pictures = g_home + "/Pictures/" BF_APP_NAME;
	g_support = g_home + "/Library/Application Support/" BF_APP_NAME;

	// A small game folder: mixed case, a CP949 folder, DATA.INI, a 300001-byte file.
	WriteText(g_game + "/data/readme.txt", "0123456789");
	WriteText(g_game + "/data/Texture/Foo.BMP", "BMxx");
	WriteText(g_game + "/data/Texture/bar.bmp", "BMyy");
	WriteText(g_game + "/" + Utf8Of949(kKoreanCp949) + "/Title.bmp", "BMz");
	WriteText(g_game + "/DATA.INI", "[Data]\n1=data.grf\n");
	std::string big(300001, '\0');
	for (size_t i = 0; i < big.size(); ++i)
		big[i] = (char)(i * 7);
	WriteText(g_game + "/big.bin", big);
	bf_file_set_game_folder(g_game.c_str());
	printf("game folder %s\nhome %s\n", g_game.c_str(), g_home.c_str());

	TestFiles();
	TestFind();
	TestMapping();
	TestCrt();
	TestRegistry();
	TestLastError();
	TestMisc();

	printf("%s: %d failed\n", g_failed ? "FAIL" : "PASS", g_failed);
	return g_failed ? 1 : 0;
}
