// bifrost-crt-test: the C runtime names the client calls that macOS lacks or
// that bf_crt.h redirects (port/client/src/bf_crt.cpp), called as the client
// calls them. One line per check, exit status 1 if any fails. Writes only
// below $BF_TEST_OUT/crt-test (BF_HOME points there). crt_client_calls.cpp is
// compiled with the client's flags, so its fopen, access, remove and rename
// are the client's; this file is port code and sees the C library's own.
// (Was the CRT part of the win32 adapter's test until M11.11.)
#include <filesystem>
#include <fstream>
#include <string>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bf_file.h"

extern "C" {
int client_init_base_dir(void);
int client_write_ping_log(const char *line);
int client_write_chat_log(const char *title, char *path_out, int path_size);
int client_write_screenshot(char *path_out, int path_size);
int client_read_data_ini(char *first_line, int size);
int client_write_into_game_folder(int *error);
int client_cache_cleanup(void);
int client_std_library(void);
int client_helpers(void);
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

std::string g_game, g_home, g_documents, g_pictures, g_support;

void TestCrt()
{
	char path[512];
	Check(client_init_base_dir(), "g_baseDir3 from bf_file_location(BF_LOCATION_GAME): the game folder");

	Check(client_write_ping_log("ping 12") && client_write_ping_log("ping 13")
	      && ReadText(g_documents + "/PingLog.txt") == "ping 12\nping 13\n",
	      "fopen(\"PingLog.txt\", \"at\"): appended in Documents");
	Check(client_write_chat_log("chat", path, sizeof(path)) && Exists(g_documents + "/Chat/chat000.txt")
	      && client_write_chat_log("chat", path, sizeof(path)) && Exists(g_documents + "/Chat/chat001.txt"),
	      "access + bf_file_make_folder + fopen of g_baseDir3\\Chat\\...: chat logs land in Documents/Chat");
	Check(client_write_screenshot(path, sizeof(path)) && Exists(g_pictures + "/screen000.jpg")
	      && client_write_screenshot(path, sizeof(path)) && Exists(g_pictures + "/screen001.jpg")
	      && !Exists(g_game + "/ScreenShot"),
	      "g_baseDir3\\ScreenShot\\screen%03d.jpg: screenshots land in Pictures, numbered on");
	char line[64] = {0};
	Check(client_read_data_ini(line, sizeof(line)) && strncmp(line, "[Data]", 6) == 0, "fopen(\"DATA.INI\", \"r\") from the game folder");
	int error = 0;
	Check(!client_write_into_game_folder(&error) && error == EACCES && !Exists(g_game + "/data/bifrost_must_not_exist.txt"),
	      "fopen of a game-folder path for writing: refused, EACCES");
	Check(client_cache_cleanup() && !Exists(g_support + "/_tmpskin_/test/a.txt") && !Exists(g_support + "/_tmpskin_/test/b.txt"),
	      "fopen, rename, access, remove in _tmpskin_ (Application Support), as Lua's os.rename and os.remove");
	Check(client_std_library(), "std::remove and std::list::remove under the renaming macros");
	Check(client_helpers(), "itoa, _itoa, _ltoa, strlwr, _strlwr, strupr, _lrotl, _lrotr as Visual C++ 6.0 has them");
}

}  // namespace

int main()
{
	const char *out = getenv("BF_TEST_OUT");
	std::string base = std::string(out && *out ? out : ".") + "/crt-test";
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

	WriteText(g_game + "/data/readme.txt", "0123456789");
	WriteText(g_game + "/DATA.INI", "[Data]\n1=data.grf\n");
	bf_file_set_game_folder(g_game.c_str());
	printf("game folder %s\nhome %s\n", g_game.c_str(), g_home.c_str());

	TestCrt();

	printf("%s: %d failed\n", g_failed ? "FAIL" : "PASS", g_failed);
	return g_failed ? 1 : 0;
}
