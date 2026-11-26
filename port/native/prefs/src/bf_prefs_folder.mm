// bf_prefs: game-folder validation and the start-up chooser.

#import <AppKit/AppKit.h>

#include "bf_prefs.h"
#include "bf_prefs_internal.h"
#include "bf_sys.h"
#include "bf_trace.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace {

std::vector<std::string> g_archives;   // empty: any *.grf

void Say(char *reason, size_t size, const char *fmt, ...)
{
	if (!reason || size == 0)
		return;
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(reason, size, fmt, ap);
	va_end(ap);
}

bool EndsWithGrf(const char *name)
{
	size_t n = strlen(name);
	return n > 4 && strcasecmp(name + n - 4, ".grf") == 0;
}

bool Listed(const char *name)
{
	if (g_archives.empty())
		return EndsWithGrf(name);
	for (const std::string &a : g_archives)
		if (strcasecmp(a.c_str(), name) == 0)
			return true;
	return false;
}

// "data.grf or rdata.grf", or "a .grf archive" for any.
std::string ArchiveListText(bool article = true)
{
	if (g_archives.empty())
		return article ? "a .grf archive" : ".grf archive";
	std::string s;
	for (size_t i = 0; i < g_archives.size(); ++i) {
		if (i)
			s += i + 1 == g_archives.size() ? " or " : ", ";
		s += g_archives[i];
	}
	return s;
}

// 1 header good, 0 not a GRF, -1 unreadable (errno).
int GrfHeader(const std::string &path)
{
	int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -1;
	char head[16] = {};
	ssize_t got = read(fd, head, sizeof head);
	int err = errno;
	close(fd);
	if (got < 0) {
		errno = err;
		return -1;
	}
	return got >= 15 && memcmp(head, "Master of Magic", 15) == 0 ? 1 : 0;
}

NSString *Ns(const char *utf8)
{
	return utf8 ? [NSString stringWithUTF8String:utf8] : nil;
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
	std::vector<std::string> found;
	while (struct dirent *e = readdir(dir)) {
		if (e->d_name[0] == '.')
			continue;
		if (Listed(e->d_name))
			found.push_back(e->d_name);
	}
	closedir(dir);
	if (found.empty()) {
		Say(reason, reason_size, "The folder %s holds no %s.", path, ArchiveListText(false).c_str());
		return 0;
	}
	std::string firstBad;
	int firstErr = 0;
	for (const std::string &name : found) {
		std::string full = std::string(path) + "/" + name;
		struct stat fs;
		if (stat(full.c_str(), &fs) != 0 || !S_ISREG(fs.st_mode))
			continue;
		int h = GrfHeader(full);
		if (h == 1)
			return 1;
		if (firstBad.empty()) {
			firstBad = name;
			firstErr = h < 0 ? errno : 0;
		}
	}
	if (firstBad.empty())
		Say(reason, reason_size, "The folder %s holds no %s that is a file.", path, ArchiveListText(false).c_str());
	else if (firstErr)
		Say(reason, reason_size, "%s in %s cannot be read: %s.", firstBad.c_str(), path, strerror(firstErr));
	else
		Say(reason, reason_size, "%s in %s is not a GRF archive.", firstBad.c_str(), path);
	return 0;
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
		@"%@ needs the folder that holds the game's archives (%s). Nothing in it is changed.",
		app, ArchiveListText().c_str()];
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
