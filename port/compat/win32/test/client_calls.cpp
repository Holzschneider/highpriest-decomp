// Compiled with the client's flags (see CMakeLists.txt): the C library's
// file calls here are the client's, through bf_win32_crt.h. The patterns are
// those of the client's call sites (named in each function), not its code.
// The standard headers are included after the prefix to show that the
// renaming macros leave them usable (std::remove, std::list::remove, <fstream>).
#include <windows.h>
#include <direct.h>
#include <io.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <list>
#include <string>
#include <vector>

#include <errno.h>
#include <string.h>

extern "C" int __cdecl access(const char *, int);   // as GameMode.cpp declares it
extern "C" int __cdecl _mkdir(const char *);

static char s_baseDir3[128];   // g_baseDir3: GetCurrentDirectoryA(0x80, g_baseDir3) in InitApp

extern "C" int client_init_base_dir(void)
{
	return GetCurrentDirectoryA(0x80, s_baseDir3) > 0 && s_baseDir3[0] == '/';
}

// PingLog.txt appended in text mode (LoginMode.cpp, GameModePacket.cpp).
extern "C" int client_write_ping_log(const char *line)
{
	FILE *file = fopen("PingLog.txt", "at");
	if (!file)
		return 0;
	fprintf(file, "%s\n", line);
	fclose(file);
	return 1;
}

// A chat save (UIFrameWnd2.cpp WriteChatToFile): folder under g_baseDir3,
// made when access says it is missing, then a numbered file not yet there.
extern "C" int client_write_chat_log(const char *title, char *path_out, int path_size)
{
	char tmp[256];
	sprintf(tmp, "%s\\Chat", s_baseDir3);
	if (access(tmp, 0)) {
		if (_mkdir(tmp))
			return 0;
	}
	int n = 0;
	sprintf(tmp, "%s\\Chat\\%s%03d.txt", s_baseDir3, title, n);
	while (access(tmp, 0) == 0)
		sprintf(tmp, "%s\\Chat\\%s%03d.txt", s_baseDir3, title, ++n);
	FILE *file = fopen(tmp, "wt");
	if (!file)
		return 0;
	fputs("chat line\n", file);
	fclose(file);
	snprintf(path_out, path_size, "%s", tmp);
	return 1;
}

// A screenshot (Mode.cpp CMode::ScreenShot, Renderer.cpp SaveJPG).
extern "C" int client_write_screenshot(char *path_out, int path_size)
{
	char tmp[256];
	sprintf(tmp, "%s\\ScreenShot", s_baseDir3);
	if (access(tmp, 0))
		_mkdir(tmp);
	int captureCnt = 0;
	sprintf(tmp, "%s\\ScreenShot\\screen%03d.jpg", s_baseDir3, captureCnt++);
	while (access(tmp, 0) == 0)
		sprintf(tmp, "%s\\ScreenShot\\screen%03d.jpg", s_baseDir3, captureCnt++);
	FILE *file = fopen(tmp, "wb");
	if (!file)
		return 0;
	static const unsigned char jpeg[4] = { 0xFF, 0xD8, 0xFF, 0xD9 };
	fwrite(jpeg, 1, sizeof(jpeg), file);
	fclose(file);
	snprintf(path_out, path_size, "%s", tmp);
	return 1;
}

// DATA.INI read from the game folder (WinMain.cpp AddPaksFromDataIni).
extern "C" int client_read_data_ini(char *first_line, int size)
{
	FILE *fp = fopen("DATA.INI", "r");
	if (!fp)
		return 0;
	int ok = fgets(first_line, size, fp) != NULL;
	fclose(fp);
	return ok;
}

// A write into the game folder: refused, errno EACCES.
extern "C" int client_write_into_game_folder(int *error)
{
	errno = 0;
	FILE *fp = fopen("data\\bifrost_must_not_exist.txt", "w");
	*error = errno;
	if (fp) {
		fclose(fp);
		return 1;
	}
	return 0;
}

// Cache clean-up as base/Util.cpp DeleteDirectory: _chmod, _unlink, _rmdir,
// and rename/remove as Lua's os.rename and os.remove.
extern "C" int client_cache_cleanup(void)
{
	char tmp[256];
	sprintf(tmp, "%s\\_tmpskin_", s_baseDir3);   // as skin.cpp
	if (access(tmp, 0)) {
		if (_mkdir(tmp))
			return 0;
	}
	strcat(tmp, "\\test");
	if (access(tmp, 0)) {
		if (_mkdir(tmp))
			return 0;
	}
	FILE *f = fopen("_tmpskin_\\test\\a.txt", "w");
	if (!f)
		return 0;
	fclose(f);
	if (rename("_tmpskin_\\test\\a.txt", "_tmpskin_\\test\\b.txt") != 0)
		return 0;
	if (access("_tmpskin_\\test\\b.txt", 0) != 0 || access("_tmpskin_\\test\\a.txt", 0) == 0)
		return 0;
	_chmod("_tmpskin_\\test\\b.txt", _S_IWRITE);
	if (_unlink("_tmpskin_\\test\\b.txt") != 0)
		return 0;
	f = fopen("_tmpskin_\\test\\c.txt", "w");
	if (!f)
		return 0;
	fclose(f);
	if (remove("_tmpskin_\\test\\c.txt") != 0)
		return 0;
	if (_rmdir(tmp) != 0)
		return 0;
	return access(tmp, 0) != 0;
}

// The standard library still works under the renaming macros.
extern "C" int client_std_library(void)
{
	std::vector<int> v;
	v.push_back(1); v.push_back(9); v.push_back(2); v.push_back(9);
	v.erase(std::remove(v.begin(), v.end(), 9), v.end());   // as GameModePacket.cpp
	std::list<int> l(v.begin(), v.end());
	l.remove(1);
	// <fstream> compiles, but its files are not translated: basic_filebuf<char>
	// is instantiated in libc++.dylib, which calls the C library's fopen. The
	// client uses no iostreams on files.
	return v.size() == 2 && l.size() == 1;
}
