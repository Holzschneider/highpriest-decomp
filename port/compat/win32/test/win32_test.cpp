// bifrost-compat-win32-test: the win32 adapter called as the client calls it.
// One line per check, exit status 1 if any fails. Writes only below
// $BF_TEST_OUT/compat-win32-test (BF_HOME points there). The C library's
// file names on client paths (bf_crt.h) are port/client/src/bf_crt.cpp's,
// compiled into this test; the archive mapping and the client's file calls
// are tested with port/native/sys (bifrost-sys-test).
#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include <shellapi.h>
#include <objbase.h>

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

// ---- Folder listings (what base/FindHack.cpp still calls) ------------------------

void TestFind()
{
	WIN32_FIND_DATAA wfd;
	HANDLE h = FindFirstFileA("data\\texture\\*.bmp", &wfd);
	Check(h != INVALID_HANDLE_VALUE && strcmp(wfd.cFileName, "Foo.BMP") == 0 && wfd.nFileSizeLow == 4
	      && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY), "FindFirstFile: wildcard without regard to case");
	Check(FindClose(h) && !CloseHandle(h), "FindClose; the handle is gone");

	h = FindFirstFileA("data\\Texture", &wfd);
	Check(h != INVALID_HANDLE_VALUE && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY), "FindFirstFile of a folder: FILE_ATTRIBUTE_DIRECTORY");
	FindClose(h);

	std::string korean = std::string(kKoreanCp949) + "\\*.bmp";
	h = FindFirstFileA(korean.c_str(), &wfd);
	Check(h != INVALID_HANDLE_VALUE && strcmp(wfd.cFileName, "Title.bmp") == 0, "FindFirstFile below a CP949 folder");
	FindClose(h);

	h = FindFirstFileA("data\\*.wav", &wfd);
	Check(h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND, "FindFirstFile without match: ERROR_FILE_NOT_FOUND");
	Check(!FindClose(h), "FindClose(INVALID_HANDLE_VALUE) fails safely");
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
	// WinMain's COM start-up.
	Check(CoInitialize(NULL) == S_OK, "CoInitialize(NULL): S_OK");
	CoUninitialize();
	bf_file_set_user_codepage(949);
	Check(MulDiv(10, 3, 4) == 8 && MulDiv(-10, 3, 4) == -8 && MulDiv(1, 2, 0) == -1, "MulDiv rounds, -1 for /0");
	Check(lstrcmpiA("Gulim.BMP", "gulim.bmp") == 0 && lstrcmpiA("a", "B") < 0, "lstrcmpiA");

	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	Check(ms.dwTotalPhys > 163000000, "GlobalMemoryStatus: not a low-memory machine");
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

	// A small game folder: mixed case, a CP949 folder, DATA.INI.
	WriteText(g_game + "/data/readme.txt", "0123456789");
	WriteText(g_game + "/data/Texture/Foo.BMP", "BMxx");
	WriteText(g_game + "/data/Texture/bar.bmp", "BMyy");
	WriteText(g_game + "/" + Utf8Of949(kKoreanCp949) + "/Title.bmp", "BMz");
	WriteText(g_game + "/DATA.INI", "[Data]\n1=data.grf\n");
	bf_file_set_game_folder(g_game.c_str());
	printf("game folder %s\nhome %s\n", g_game.c_str(), g_home.c_str());

	TestFind();
	TestCrt();
	TestLastError();
	TestMisc();

	printf("%s: %d failed\n", g_failed ? "FAIL" : "PASS", g_failed);
	return g_failed ? 1 : 0;
}
