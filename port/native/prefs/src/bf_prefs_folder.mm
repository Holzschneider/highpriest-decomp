// bf_prefs: game-folder validation and the start-up chooser.

#import <AppKit/AppKit.h>

#include "bf_prefs.h"
#include "bf_prefs_internal.h"
#include "bf_sys.h"
#include "bf_trace.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {

std::vector<std::string> g_archives;   // empty: what the client reads (DATA.INI, else data.grf)

void Say(char *reason, size_t size, const char *fmt, ...)
{
	if (!reason || size == 0)
		return;
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(reason, size, fmt, ap);
	va_end(ap);
}

NSString *Ns(const char *utf8)
{
	return utf8 ? [NSString stringWithUTF8String:utf8] : nil;
}

// "data.grf", "rdata.grf or data.grf".
std::string ArchiveListText(const std::vector<std::string> &names)
{
	std::string s;
	for (size_t i = 0; i < names.size(); ++i) {
		if (i)
			s += i + 1 == names.size() ? " or " : ", ";
		s += names[i];
	}
	return s;
}

// The entry of dir named name, compared as the client's file lookup does:
// without regard to case or to the Unicode normalisation form. "" if none.
std::string FindEntry(const std::string &dir, const std::string &name)
{
	struct stat st;
	if (lstat((dir + "/" + name).c_str(), &st) == 0)
		return name;   // as written (a case-insensitive volume finds the others too)
	DIR *d = opendir(dir.c_str());
	if (!d)
		return std::string();
	std::string found;
	@autoreleasepool {
		NSString *want = Ns(name.c_str());
		while (struct dirent *e = readdir(d)) {
			if (strcasecmp(e->d_name, name.c_str()) == 0) {
				found = e->d_name;
				break;
			}
			NSString *have = want ? Ns(e->d_name) : nil;
			if (have && [have caseInsensitiveCompare:want] == NSOrderedSame) {
				found = e->d_name;
				break;
			}
		}
	}
	closedir(d);
	return found;
}

// A file below folder, by a relative path with '/' between its parts; the
// path found, or "" if absent. A link counts as what it points to.
std::string FindFile(const std::string &folder, const std::string &relative)
{
	std::string at = folder;
	size_t from = 0;
	while (from <= relative.size()) {
		size_t slash = relative.find('/', from);
		std::string part = relative.substr(from, slash == std::string::npos ? std::string::npos : slash - from);
		std::string entry = FindEntry(at, part);
		if (entry.empty())
			return std::string();
		at += "/" + entry;
		if (slash == std::string::npos)
			break;
		from = slash + 1;
	}
	struct stat st;
	return stat(at.c_str(), &st) == 0 && S_ISREG(st.st_mode) ? at : std::string();
}

// The archives the client opens (WinMain.cpp, AddPaksFromDataIni): the
// numbered entries of [Data] in the folder's DATA.INI, lowest first, at most
// 16; without one, data.grf.
std::vector<std::string> ClientArchives(const std::string &folder, bool *fromIni)
{
	*fromIni = false;
	if (!g_archives.empty())
		return g_archives;
	std::vector<std::pair<int, std::string>> listed;
	std::string ini = FindFile(folder, "DATA.INI");
	if (FILE *fp = ini.empty() ? NULL : fopen(ini.c_str(), "r")) {
		char line[1100];
		bool inData = false;
		while (listed.size() < 16 && fgets(line, sizeof line, fp)) {
			char *p = line + strspn(line, " \t");
			p[strcspn(p, "\r\n")] = 0;
			char *value = strchr(p, '=');
			if (*p == '[')
				inData = strncasecmp(p, "[Data]", 6) == 0;
			else if (inData && isdigit((unsigned char)*p) && value && value[1]) {
				std::string name = value + 1;
				for (char &c : name)
					if (c == '\\')
						c = '/';
				listed.emplace_back(atoi(p), name);
			}
		}
		fclose(fp);
	}
	std::stable_sort(listed.begin(), listed.end(),
	                 [](const std::pair<int, std::string> &a, const std::pair<int, std::string> &b) { return a.first < b.first; });
	std::vector<std::string> names;
	for (const auto &l : listed)
		names.push_back(l.second);
	if (names.empty())
		names.push_back("data.grf");
	else
		*fromIni = true;
	return names;
}

// What the client cannot start without (bf_prefs.h): found loose below the
// folder or in one of its archives. The archive name is the client's CP949
// path, the loose one the folder's UTF-8 path.
struct Required {
	const char *archiveName;
	const char *looseName;
	const char *what;
};
const Required kRequired[] = {
	{ "data\\sclientinfo.xml", "data/sclientinfo.xml", "the service and the login server" },
	{ "data\\msgstringtable.txt", "data/msgstringtable.txt", "the interface's text" },
	{ "data\\sprite\\\xb8\xf3\xbd\xba\xc5\xcd\\poring.spr", "data/sprite/\xeb\xaa\xac\xec\x8a\xa4\xed\x84\xb0/poring.spr",
	  "the default sprite, which stands in for every missing one" },
	{ "data\\sprite\\\xb8\xf3\xbd\xba\xc5\xcd\\poring.act", "data/sprite/\xeb\xaa\xac\xec\x8a\xa4\xed\x84\xb0/poring.act",
	  "the default sprite's actions" },
};
const size_t kRequiredCount = sizeof kRequired / sizeof kRequired[0];

std::string LowerAscii(const char *s, size_t n)
{
	std::string out(s, n);
	for (char &c : out)
		if (c >= 'A' && c <= 'Z')
			c = (char)(c - 'A' + 'a');
	return out;
}

enum ArchiveResult { ARCHIVE_UNREADABLE = -1, ARCHIVE_NOT_GRF = 0, ARCHIVE_READ = 1, ARCHIVE_NAMES_UNKNOWN = 2 };

uint32_t Le32(const unsigned char *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

// Reads an archive's header and, for version 0x2xx, its file table, as
// CGPak::Open does; marks the required names it holds in found[] and counts
// its files. ARCHIVE_NAMES_UNKNOWN: a 0x1xx archive, whose names are
// encrypted; it opens, but what it holds is not looked at. *problem says
// what is wrong otherwise (errno text, or which part is damaged).
ArchiveResult ReadArchive(const std::string &path, bool found[], uint64_t *files, std::string *problem)
{
	int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		*problem = strerror(errno);
		return ARCHIVE_UNREADABLE;
	}
	struct stat st;
	unsigned char head[46];
	ssize_t got = fstat(fd, &st) == 0 ? pread(fd, head, sizeof head, 0) : -1;
	if (got < 0) {
		*problem = strerror(errno);
		close(fd);
		return ARCHIVE_UNREADABLE;
	}
	if (got < 15 || memcmp(head, "Master of Magic", 15) != 0) {
		close(fd);
		return ARCHIVE_NOT_GRF;
	}
	ArchiveResult result = ARCHIVE_UNREADABLE;
	char text[128];
	uint32_t offset = got == 46 ? Le32(head + 30) : 0, seed = got == 46 ? Le32(head + 34) : 0;
	uint32_t count = got == 46 ? Le32(head + 38) : 0, version = got == 46 ? Le32(head + 42) : 0;
	if (got != 46) {
		*problem = "its header is cut short";
	} else if ((version & 0xFF00) == 0x0100) {
		*files = count - (seed + 7);
		result = ARCHIVE_NAMES_UNKNOWN;
	} else if ((version & 0xFF00) != 0x0200) {
		snprintf(text, sizeof text, "it is of version 0x%x, which the client does not read", version);
		*problem = text;
	} else {
		uint64_t at = 46 + (uint64_t)offset;
		unsigned char info[8];
		uint32_t packed = 0, size = 0;
		if (at + 8 > (uint64_t)st.st_size || pread(fd, info, 8, (off_t)at) != 8
		    || (packed = Le32(info), size = Le32(info + 4), at + 8 + packed > (uint64_t)st.st_size)
		    || size > (1u << 30)) {
			*problem = "its file table lies outside the file";
		} else {
			std::vector<unsigned char> z(packed), table((size_t)size + 8);
			uLongf tableSize = (uLongf)table.size();
			if (pread(fd, z.data(), packed, (off_t)(at + 8)) != (ssize_t)packed
			    || uncompress(table.data(), &tableSize, z.data(), packed) != Z_OK) {
				*problem = "its file table cannot be unpacked";
			} else {
				std::string want[kRequiredCount];
				for (size_t i = 0; i < kRequiredCount; ++i)
					want[i] = kRequired[i].archiveName;
				uint64_t entries = 0, n = count - (uint64_t)(seed + 7);
				size_t p = 0;
				while (entries < n && p < tableSize) {
					const char *name = (const char *)&table[p];
					size_t len = strnlen(name, tableSize - p);
					if (p + len + 1 + 17 > tableSize)
						break;
					unsigned char type = table[p + len + 1 + 12];
					if (type & 1) {
						++*files;
						for (size_t i = 0; i < kRequiredCount; ++i)
							if (!found[i] && len == want[i].size() && LowerAscii(name, len) == want[i])
								found[i] = true;
					}
					p += len + 1 + 17;
					++entries;
				}
				result = ARCHIVE_READ;
			}
		}
	}
	close(fd);
	return result;
}

}  // namespace

// ---- Validation ---------------------------------------------------------

extern "C" void bf_prefs_set_archives(const char *const *names, int count)
{
	g_archives.clear();
	for (int i = 0; names && i < count; ++i)
		if (names[i] && names[i][0])
			g_archives.push_back(names[i]);
}

extern "C" int bf_prefs_game_folder_valid(const char *path, char *reason, size_t reason_size)
{
	if (!path || !path[0]) {
		Say(reason, reason_size, "No game folder is set.");
		return 0;
	}
	// First, before the path reaches the kernel: a path over the limit may
	// still be one it takes, but not every file below it would be.
	char tooLong[256];
	if (!bf_file_game_folder_fits(path, tooLong, sizeof tooLong)) {
		Say(reason, reason_size, "%s", tooLong);
		return 0;
	}
	// The client is started from anywhere (Finder starts it in /), so a
	// relative folder would name another folder each time.
	if (path[0] != '/') {
		Say(reason, reason_size, "The game folder must be given as a full path; %s is relative.", path);
		return 0;
	}
	struct stat st;
	if (stat(path, &st) != 0) {
		if (errno == ENOENT)
			Say(reason, reason_size, "The folder %s does not exist.", path);
		else
			Say(reason, reason_size, "The folder %s cannot be reached: %s.", path, strerror(errno));
		return 0;
	}
	if (!S_ISDIR(st.st_mode)) {
		Say(reason, reason_size, "%s is a file, not a folder.", path);
		return 0;
	}
	DIR *dir = opendir(path);
	if (!dir) {
		Say(reason, reason_size, "The folder %s cannot be read: %s.", path, strerror(errno));
		return 0;
	}
	closedir(dir);

	// The archives the client opens: at least one is there, and each one
	// there opens as the client opens it.
	std::string folder = path;
	while (folder.size() > 1 && folder.back() == '/')
		folder.pop_back();
	bool fromIni = false;
	std::vector<std::string> archives = ClientArchives(folder, &fromIni);
	bool found[kRequiredCount] = {};
	bool namesUnknown = false;
	uint64_t files = 0;
	std::vector<std::string> present;
	for (const std::string &name : archives) {
		std::string full = FindFile(folder, name);
		if (full.empty())
			continue;
		std::string problem;
		switch (ReadArchive(full, found, &files, &problem)) {
		case ARCHIVE_NOT_GRF:
			Say(reason, reason_size, "%s in %s is not a GRF archive.", name.c_str(), path);
			return 0;
		case ARCHIVE_UNREADABLE:
			Say(reason, reason_size, "%s in %s cannot be read: %s.", name.c_str(), path, problem.c_str());
			return 0;
		case ARCHIVE_NAMES_UNKNOWN:
			namesUnknown = true;
			break;
		case ARCHIVE_READ:
			break;
		}
		present.push_back(name);
	}
	if (present.empty()) {
		if (fromIni)
			Say(reason, reason_size, "The folder %s holds none of the archives its DATA.INI names (%s).", path,
			    ArchiveListText(archives).c_str());
		else if (g_archives.empty())
			Say(reason, reason_size, "The folder %s holds no data.grf, the archive the client reads "
			    "(a DATA.INI in the folder may name others, such as rdata.grf).", path);
		else
			Say(reason, reason_size, "The folder %s holds no %s.", path, ArchiveListText(archives).c_str());
		return 0;
	}
	// The files the start-up cannot do without, loose or in an archive (the
	// names of a 0x1xx archive are not read: it may hold them).
	std::string missing;
	size_t missingCount = 0;
	for (size_t i = 0; i < kRequiredCount; ++i) {
		if (found[i] || namesUnknown || !FindFile(folder, kRequired[i].looseName).empty())
			continue;
		missing += missing.empty() ? "" : "; ";
		missing += std::string(kRequired[i].looseName) + " (" + kRequired[i].what + ")";
		++missingCount;
	}
	if (missingCount) {
		Say(reason, reason_size, "The folder %s lacks %s %s, neither loose in the folder nor in %s%s.", path,
		    missingCount == 1 ? "a file the client needs to start:" : "files the client needs to start:",
		    missing.c_str(), ArchiveListText(present).c_str(),
		    files ? "" : present.size() == 1 ? " (which holds no files)" : " (which hold no files)");
		return 0;
	}
	return 1;
}

extern "C" int bf_prefs_set_game_folder(const char *path, char *reason, size_t reason_size)
{
	char why[1024];
	if (!bf_prefs_game_folder_valid(path, why, sizeof why)) {
		Say(reason, reason_size, "%s", why);
		bf_tracef(BFT_ERR, "prefs: game folder refused: %s", why);
		return 0;
	}
	bf_settings *s = bf_settings_shared();
	if (!bf_settings_set_string(s, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER, path)) {
		Say(reason, reason_size, "The setting could not be written.");
		return 0;
	}
	if (bf_settings_save(s) < 0)
		bf_tracef(BFT_ERR, "prefs: settings could not be saved to %s", bf_settings_path(s));
	BF_TRACE(BFT_UI, "prefs: game folder set to %s", path);
	bf_prefs_internal_notify(BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER);
	return 1;
}

// ---- The open panel -------------------------------------------------------

@interface BFPrefsFolderValidator : NSObject <NSOpenSavePanelDelegate>
@end

@implementation BFPrefsFolderValidator
- (BOOL)panel:(id)sender validateURL:(NSURL *)url error:(NSError **)outError
{
	char why[1024];
	if (bf_prefs_game_folder_valid(url.fileSystemRepresentation, why, sizeof why))
		return YES;
	if (outError)
		*outError = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadUnknownError userInfo:@{
			NSLocalizedDescriptionKey: @"This folder cannot be used as the game folder.",
			NSLocalizedRecoverySuggestionErrorKey: Ns(why) ?: @"",
		}];
	return NO;
}
@end

NSOpenPanel *bf_prefs_internal_make_panel(const char *reason, id<NSOpenSavePanelDelegate> *keep)
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	panel.canChooseDirectories = YES;
	panel.canChooseFiles = NO;
	panel.allowsMultipleSelection = NO;
	panel.canCreateDirectories = NO;
	panel.resolvesAliases = YES;
	panel.prompt = @"Choose";
	panel.title = @"Choose the Game Folder";
	NSString *app = Ns(bf_sys_app_name()) ?: @"";
	NSString *what = [NSString stringWithFormat:
		@"%@ needs the folder that holds the game's archives (%s) and its data folder. Nothing in it is changed.",
		app, g_archives.empty() ? "data.grf, or those its DATA.INI names" : ArchiveListText(g_archives).c_str()];
	NSString *why = Ns(reason);
	panel.message = why.length ? [NSString stringWithFormat:@"%@\n%@", why, what] : what;
	// Start where the default lies, or in the home folder.
	NSString *games = [NSHomeDirectory() stringByAppendingPathComponent:@"Games"];
	BOOL isDir = NO;
	if ([[NSFileManager defaultManager] fileExistsAtPath:games isDirectory:&isDir] && isDir)
		panel.directoryURL = [NSURL fileURLWithPath:games isDirectory:YES];
	BFPrefsFolderValidator *validator = [BFPrefsFolderValidator new];
	panel.delegate = validator;
	*keep = validator;
	return panel;
}

extern "C" int bf_prefs_ask_game_folder(const char *reason, char *out, size_t out_size)
{
	if (!bf_sys_alerts_enabled() || !bf_sys_is_main_thread()) {
		bf_tracef(BFT_ERR, "prefs: no usable game folder and no way to ask for one (%s): %s",
		          bf_sys_is_main_thread() ? "no window server or BF_NO_ALERTS" : "not on the main thread",
		          reason ? reason : "");
		return -1;
	}
	int result = 0;
	@autoreleasepool {
		[NSApplication sharedApplication];
		if (NSApp.activationPolicy == NSApplicationActivationPolicyProhibited)
			[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp activate];
		id<NSOpenSavePanelDelegate> keep = nil;
		NSOpenPanel *panel = bf_prefs_internal_make_panel(reason, &keep);
		NSModalResponse answer = [panel runModal];
		if (answer != NSModalResponseOK || !panel.URL) {
			BF_TRACE(BFT_UI, "prefs: game-folder chooser cancelled");
			return 0;
		}
		const char *path = panel.URL.fileSystemRepresentation;
		size_t n = strlen(path);
		if (!out || n + 1 > out_size) {
			bf_tracef(BFT_ERR, "prefs: chosen game folder does not fit the buffer: %s", path);
			return -1;
		}
		char why[1024];
		if (!bf_prefs_set_game_folder(path, why, sizeof why))
			return -1;   // the panel validated it; only a race or a write failure gets here
		bf_file_set_game_folder(path);
		memcpy(out, path, n + 1);
		result = 1;
	}
	return result;
}

extern "C" int bf_prefs_choose_game_folder(char *out, size_t out_size)
{
	const char *configured = bf_file_location(BF_LOCATION_GAME);
	char why[1024];
	if (configured && bf_prefs_game_folder_valid(configured, why, sizeof why)) {
		size_t n = strlen(configured);
		if (!out || n + 1 > out_size) {
			bf_tracef(BFT_ERR, "prefs: game folder does not fit the buffer: %s", configured);
			return -1;
		}
		memcpy(out, configured, n + 1);
		BF_TRACE(BFT_UI, "prefs: game folder %s is valid", configured);
		return 1;
	}
	if (!configured)
		Say(why, sizeof why, "No game folder is set.");
	bf_tracef(BFT_ERR, "prefs: configured game folder is not usable: %s", why);
	return bf_prefs_ask_game_folder(why, out, out_size);
}

// ---- UI scale ---------------------------------------------------------------

extern "C" int bf_prefs_ui_scale(void)
{
	int32_t v = 0;
	if (bf_settings_get_int(bf_settings_shared(), BF_PREFS_DISPLAY, BF_PREFS_UI_SCALE, &v))
		return v < 1 ? 1 : v > 3 ? 3 : v;
	double backing = 1.0;
	@autoreleasepool {
		NSScreen *screen = [NSScreen mainScreen];
		if (screen)
			backing = screen.backingScaleFactor;
	}
	int s = (int)(backing + 0.5);
	return s < 1 ? 1 : s > 3 ? 3 : s;
}
