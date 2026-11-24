// The game folder, the write locations, path translation and file access.

#include "bf_file.h"
#include "bf_settings.h"
#include "bf_trace.h"

#include <CoreFoundation/CoreFoundation.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct bf_file {
	int fd;
	std::string path;
};

struct bf_file_mapping {
	void *data;
	uint64_t size;
};

struct bf_file_find {
	std::vector<bf_file_info> entries;
	size_t next;
};

namespace {

// ---- State ------------------------------------------------------------------

std::recursive_mutex g_lock;

std::string g_explicitGameFolder;
bool g_hasExplicitGameFolder = false;
std::string g_gameFolder;          // resolved on first use
bool g_gameFolderReady = false;
std::string g_home;
std::string g_locations[4];
bool g_locationReady[4];
unsigned g_userCodepage = 949;

struct Listing {
	struct timespec modified;
	std::unordered_map<std::string, std::string> byFoldedName;   // folded -> name on disk
};
std::unordered_map<std::string, Listing> g_listings;

// ---- Strings ------------------------------------------------------------------

bool IsAscii(const char *s, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		if ((unsigned char)s[i] >= 0x80)
			return false;
	return true;
}

CFStringEncoding EncodingFor(unsigned codepage)
{
	switch (codepage) {
	case 949:   return kCFStringEncodingDOSKorean;
	case 1252:  return kCFStringEncodingWindowsLatin1;
	case 65001: return kCFStringEncodingUTF8;
	default: {
		CFStringEncoding e = CFStringConvertWindowsCodepageToEncoding(codepage);
		return e == kCFStringEncodingInvalidId ? kCFStringEncodingWindowsLatin1 : e;
	}
	}
}

std::string ToUtf8(CFStringRef s)
{
	CFIndex length = CFStringGetLength(s);
	CFIndex capacity = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
	std::vector<char> buffer((size_t)capacity);
	if (!CFStringGetCString(s, buffer.data(), capacity, kCFStringEncodingUTF8))
		return std::string();
	return std::string(buffer.data());
}

// Marks a path that native code built: UTF-8 when it is valid UTF-8, else the user code page.
const unsigned kUtf8ThenUser = 0xFFFF;

// Client bytes in a code page -> UTF-8 in NFC. False if they cannot be decoded.
bool Decode(const std::string &bytes, unsigned codepage, std::string *out)
{
	if (IsAscii(bytes.data(), bytes.size())) {
		*out = bytes;
		return true;
	}
	if (codepage == kUtf8ThenUser) {
		CFStringRef utf8 = CFStringCreateWithBytes(NULL, (const UInt8 *)bytes.data(), (CFIndex)bytes.size(),
		                                           kCFStringEncodingUTF8, false);
		if (utf8) {
			CFMutableStringRef m = CFStringCreateMutableCopy(NULL, 0, utf8);
			CFRelease(utf8);
			CFStringNormalize(m, kCFStringNormalizationFormC);
			*out = ToUtf8(m);
			CFRelease(m);
			return !out->empty();
		}
		codepage = g_userCodepage;
	}
	CFStringRef s = CFStringCreateWithBytes(NULL, (const UInt8 *)bytes.data(), (CFIndex)bytes.size(),
	                                        EncodingFor(codepage), false);
	if (!s && codepage != 1252) {
		bf_tracef(BFT_FILE, "file: name not valid in code page %u, read as 1252", codepage);
		s = CFStringCreateWithBytes(NULL, (const UInt8 *)bytes.data(), (CFIndex)bytes.size(),
		                            kCFStringEncodingWindowsLatin1, false);
	}
	if (!s)
		s = CFStringCreateWithBytes(NULL, (const UInt8 *)bytes.data(), (CFIndex)bytes.size(),
		                            kCFStringEncodingISOLatin1, false);
	if (!s)
		return false;
	CFMutableStringRef m = CFStringCreateMutableCopy(NULL, 0, s);
	CFRelease(s);
	CFStringNormalize(m, kCFStringNormalizationFormC);
	*out = ToUtf8(m);
	CFRelease(m);
	return !out->empty();
}

// UTF-8 (any normalisation) -> client bytes; '?' for what the code page lacks.
std::string Encode(const std::string &utf8, unsigned codepage)
{
	if (IsAscii(utf8.data(), utf8.size()))
		return utf8;
	CFStringRef s = CFStringCreateWithBytes(NULL, (const UInt8 *)utf8.data(), (CFIndex)utf8.size(),
	                                        kCFStringEncodingUTF8, false);
	if (!s)
		return utf8;
	CFMutableStringRef m = CFStringCreateMutableCopy(NULL, 0, s);
	CFRelease(s);
	CFStringNormalize(m, kCFStringNormalizationFormC);
	CFRange range = CFRangeMake(0, CFStringGetLength(m));
	CFIndex used = 0;
	CFStringGetBytes(m, range, EncodingFor(codepage), '?', false, NULL, 0, &used);
	std::string out((size_t)used, 0);
	CFStringGetBytes(m, range, EncodingFor(codepage), '?', false, (UInt8 *)&out[0], used, &used);
	CFRelease(m);
	return out;
}

// The comparison key of a UTF-8 name: NFC, case folded.
std::string Fold(const std::string &utf8)
{
	if (IsAscii(utf8.data(), utf8.size())) {
		std::string out = utf8;
		for (size_t i = 0; i < out.size(); ++i)
			if (out[i] >= 'A' && out[i] <= 'Z')
				out[i] = (char)(out[i] - 'A' + 'a');
		return out;
	}
	CFStringRef s = CFStringCreateWithBytes(NULL, (const UInt8 *)utf8.data(), (CFIndex)utf8.size(),
	                                        kCFStringEncodingUTF8, false);
	if (!s)
		return utf8;
	CFMutableStringRef m = CFStringCreateMutableCopy(NULL, 0, s);
	CFRelease(s);
	CFStringNormalize(m, kCFStringNormalizationFormC);
	CFStringFold(m, kCFCompareCaseInsensitive, NULL);
	CFStringNormalize(m, kCFStringNormalizationFormC);
	std::string out = ToUtf8(m);
	CFRelease(m);
	return out;
}

std::u32string Utf32(const std::string &utf8)
{
	std::u32string out;
	for (size_t i = 0; i < utf8.size();) {
		unsigned char c = (unsigned char)utf8[i];
		char32_t cp;
		int n;
		if (c < 0x80)      { cp = c; n = 1; }
		else if (c < 0xE0) { cp = c & 0x1F; n = 2; }
		else if (c < 0xF0) { cp = c & 0x0F; n = 3; }
		else               { cp = c & 0x07; n = 4; }
		for (int k = 1; k < n && i + k < utf8.size(); ++k)
			cp = (cp << 6) | ((unsigned char)utf8[i + k] & 0x3F);
		out.push_back(cp);
		i += n;
	}
	return out;
}

bool Match(const char32_t *p, const char32_t *pe, const char32_t *s, const char32_t *se)
{
	while (p < pe) {
		if (*p == U'*') {
			while (p < pe && *p == U'*')
				++p;
			if (p == pe)
				return true;
			for (const char32_t *t = s; t <= se; ++t)
				if (Match(p, pe, t, se))
					return true;
			return false;
		}
		if (s == se)
			return false;
		if (*p != U'?' && *p != *s)
			return false;
		++p;
		++s;
	}
	return s == se;
}

// Wildcard match of folded names, with FindFirstFile's "*.*" meaning everything.
bool WildcardMatch(const std::string &foldedPattern, const std::string &foldedName)
{
	if (foldedPattern == "*.*" || foldedPattern == "*")
		return true;
	std::u32string p = Utf32(foldedPattern), s = Utf32(foldedName);
	return Match(p.data(), p.data() + p.size(), s.data(), s.data() + s.size());
}

bool HasWildcard(const std::string &s)
{
	return s.find_first_of("*?") != std::string::npos;
}

bool StartsWithFolder(const std::string &path, const std::string &root, std::string *rest)
{
	if (root.empty() || path.size() < root.size())
		return false;
	if (strncasecmp(path.c_str(), root.c_str(), root.size()) != 0)
		return false;
	if (path.size() == root.size()) {
		*rest = std::string();
		return true;
	}
	char c = path[root.size()];
	if (c != '/' && c != '\\')
		return false;
	*rest = path.substr(root.size() + 1);
	return true;
}

std::string StripTrailingSlash(std::string s)
{
	while (s.size() > 1 && s[s.size() - 1] == '/')
		s.erase(s.size() - 1);
	return s;
}

// Splits on both separators, drops empty and "." parts, applies "..". False if ".." leaves the root.
bool Split(const std::string &path, std::vector<std::string> *parts)
{
	parts->clear();
	size_t i = 0;
	while (i <= path.size()) {
		size_t j = path.find_first_of("/\\", i);
		if (j == std::string::npos)
			j = path.size();
		std::string part = path.substr(i, j - i);
		if (part == "..") {
			if (parts->empty())
				return false;
			parts->pop_back();
		} else if (!part.empty() && part != ".") {
			parts->push_back(part);
		}
		i = j + 1;
	}
	return true;
}

// ---- Locations ------------------------------------------------------------------

std::string RealHome()
{
	struct passwd *pw = getpwuid(getuid());
	if (pw && pw->pw_dir)
		return pw->pw_dir;
	const char *home = getenv("HOME");
	return home ? home : "/tmp";
}

const std::string &Home()
{
	if (g_home.empty()) {
		const char *override = getenv("BF_HOME");
		g_home = StripTrailingSlash(override && *override ? override : RealHome());
	}
	return g_home;
}

bool MakeFolders(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) == 0)
		return S_ISDIR(st.st_mode);
	size_t slash = path.rfind('/');
	if (slash != std::string::npos && slash > 0 && !MakeFolders(path.substr(0, slash)))
		return false;
	return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

const std::string &GameFolder()
{
	if (!g_gameFolderReady) {
		std::string folder;
		const char *env = getenv("BF_CLIENT_DIR");
		char fromSettings[1024];
		if (g_hasExplicitGameFolder) {
			folder = g_explicitGameFolder;
		} else if (env && *env) {
			folder = env;
		} else if (bf_settings_get_string(bf_settings_shared(), BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER,
		                                  fromSettings, sizeof(fromSettings), NULL) == 1 && fromSettings[0]) {
			folder = fromSettings;
		} else {
			folder = RealHome() + "/Games/" BF_APP_NAME;
		}
		if (folder.size() > 1 && folder[0] == '~' && folder[1] == '/')
			folder = RealHome() + folder.substr(1);
		g_gameFolder = StripTrailingSlash(folder);
		g_gameFolderReady = true;
		BF_TRACE(BFT_FILE, "file: game folder %s", g_gameFolder.c_str());
	}
	return g_gameFolder;
}

// The path of a write location, whether or not it exists yet.
std::string LocationPath(int i)
{
	static const char *const kParts[4] = {
		NULL, "/Library/Application Support/", "/Pictures/", "/Documents/"
	};
	return Home() + kParts[i] + BF_APP_NAME;
}

const char *Location(bf_location location)
{
	if (location == BF_LOCATION_GAME)
		return GameFolder().c_str();
	int i = (int)location;
	if (i < 0 || i > 3)
		return NULL;
	if (!g_locationReady[i]) {
		std::string path = LocationPath(i);
		if (!MakeFolders(path)) {
			bf_tracef(BFT_ERR, "file: cannot create %s: %s", path.c_str(), strerror(errno));
			return NULL;
		}
		g_locations[i] = path;
		g_locationReady[i] = true;
	}
	return g_locations[i].c_str();
}

// The client's own write paths, by their first component.
struct WriteRule {
	const char *name;
	bf_location location;
	bool dropName;   // the component itself is not kept below the location
};

const WriteRule kWriteRules[] = {
	{ "ScreenShot",         BF_LOCATION_PICTURES,  true  },   // Mode.cpp ScreenShot()
	{ "Chat",               BF_LOCATION_DOCUMENTS, false },   // chat saves, GM autosave
	{ "Chat_BM",            BF_LOCATION_DOCUMENTS, false },   // battle-message chat saves
	{ "Whisper",            BF_LOCATION_DOCUMENTS, false },   // whisper window saves
	{ "Report",             BF_LOCATION_DOCUMENTS, false },   // chat reports
	{ "emblem",             BF_LOCATION_DOCUMENTS, false },   // guild emblems the player supplies
	{ "PingLog.txt",        BF_LOCATION_DOCUMENTS, false },
	{ "TraceAI.txt",        BF_LOCATION_DOCUMENTS, false },   // homunculus/mercenary AI trace
	{ "ProhibitionLog.txt", BF_LOCATION_DOCUMENTS, false },
	{ "ProhibitionLog.gd",  BF_LOCATION_SUPPORT,   false },
	{ "_tmpEmblem",         BF_LOCATION_SUPPORT,   false },   // emblem cache from the server
	{ "_tmpskin_",          BF_LOCATION_SUPPORT,   false },   // unpacked UI skins
	{ "notice_patch.txt",   BF_LOCATION_SUPPORT,   false },
	{ "notice_check.txt",   BF_LOCATION_SUPPORT,   false },
	{ "notice_read.txt",    BF_LOCATION_SUPPORT,   false },
	{ "savedata",           BF_LOCATION_SUPPORT,   false },
};

const WriteRule *RuleFor(const std::string &firstPart)
{
	for (size_t i = 0; i < sizeof(kWriteRules) / sizeof(kWriteRules[0]); ++i)
		if (strcasecmp(firstPart.c_str(), kWriteRules[i].name) == 0)
			return &kWriteRules[i];
	return NULL;
}

// ---- Lookup ------------------------------------------------------------------

const Listing *ListFolder(const std::string &folder)
{
	struct stat st;
	if (stat(folder.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
		return NULL;
	std::unordered_map<std::string, Listing>::iterator it = g_listings.find(folder);
	if (it != g_listings.end() && it->second.modified.tv_sec == st.st_mtimespec.tv_sec
	    && it->second.modified.tv_nsec == st.st_mtimespec.tv_nsec)
		return &it->second;
	DIR *dir = opendir(folder.c_str());
	if (!dir)
		return NULL;
	Listing listing;
	listing.modified = st.st_mtimespec;
	while (struct dirent *entry = readdir(dir)) {
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		listing.byFoldedName[Fold(name)] = name;
	}
	closedir(dir);
	BF_TRACE(BFT_FILE, "file: listed %s (%zu entries)", folder.c_str(), listing.byFoldedName.size());
	Listing &stored = g_listings[folder];
	stored = listing;
	return &stored;
}

// The name on disk of an entry of folder matching name (UTF-8), or false.
// The listing gives the name as it is on disk, so resolved paths do not depend
// on the case the client wrote; the direct lstat catches what folding misses
// on a volume with other rules.
bool FindEntry(const std::string &folder, const std::string &name, std::string *actual)
{
	const Listing *listing = ListFolder(folder);
	if (!listing)
		return false;
	std::unordered_map<std::string, std::string>::const_iterator it = listing->byFoldedName.find(Fold(name));
	if (it != listing->byFoldedName.end()) {
		*actual = it->second;
		return true;
	}
	struct stat st;
	std::string direct = folder + "/" + name;
	if (lstat(direct.c_str(), &st) == 0) {
		*actual = name;
		return true;
	}
	return false;
}

enum Mode { kRead, kWrite, kWriteNoCreate };

// Walks parts below root. In write mode the last part may be new, and
// missing folders before it are created (unless kWriteNoCreate).
bool Walk(const std::string &root, const std::vector<std::string> &parts, size_t first,
          unsigned codepage, Mode mode, std::string *out)
{
	std::string current = root;
	if (mode == kRead && first >= parts.size()) {
		struct stat st;
		if (stat(root.c_str(), &st) != 0) {
			errno = ENOENT;
			return false;
		}
	}
	for (size_t i = first; i < parts.size(); ++i) {
		std::string name;
		if (!Decode(parts[i], codepage, &name)) {
			errno = EILSEQ;
			return false;
		}
		std::string actual;
		if (FindEntry(current, name, &actual)) {
			current += "/" + actual;
			continue;
		}
		if (mode == kRead) {
			errno = ENOENT;
			return false;
		}
		current += "/" + name;
		if (i + 1 < parts.size()) {
			if (mode == kWriteNoCreate) {
				errno = ENOENT;
				return false;
			}
			if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST)
				return false;
		}
	}
	*out = current;
	return true;
}

struct Resolved {
	std::string path;
	bf_location location;   // where it ended; GAME also for foreign absolute paths
	bool foreign;           // an absolute path outside every known folder
};

bool Resolve(const char *clientPath, Mode mode, Resolved *result)
{
	if (!clientPath) {
		errno = EINVAL;
		return false;
	}
	std::string path = clientPath;
	result->foreign = false;
	const std::string &game = GameFolder();
	std::string rest;

	if (!path.empty() && path[0] == '/') {
		for (int i = 1; i <= 3; ++i) {
			std::string root = LocationPath(i);
			if (StartsWithFolder(path, root, &rest)) {
				std::vector<std::string> parts;
				if (!Split(rest, &parts)) {
					errno = ENOENT;
					return false;
				}
				if (mode != kRead && !Location((bf_location)i)) {
					errno = EIO;
					return false;
				}
				result->location = (bf_location)i;
				return Walk(root, parts, 0, kUtf8ThenUser, mode, &result->path);
			}
		}
		if (StartsWithFolder(path, game, &rest)) {
			path = rest;   // relative to the game folder from here on
		} else {
			if (mode != kRead)
				BF_TRACE(BFT_FILE, "file: write outside the known folders: %s", path.c_str());
			result->path = path;
			result->location = BF_LOCATION_GAME;
			result->foreign = true;
			if (mode == kRead) {
				struct stat st;
				if (stat(path.c_str(), &st) != 0)
					return false;
			}
			return true;
		}
	} else if (path.size() >= 2 && ((path[0] | 0x20) >= 'a' && (path[0] | 0x20) <= 'z') && path[1] == ':') {
		path = path.substr(2);
	}

	std::vector<std::string> parts;
	if (!Split(path, &parts)) {
		errno = ENOENT;
		return false;
	}
	const WriteRule *rule = parts.empty() ? NULL : RuleFor(parts[0]);
	if (rule) {
		std::string root = LocationPath((int)rule->location);
		if (mode != kRead && !Location(rule->location)) {
			errno = EIO;
			return false;
		}
		size_t first = rule->dropName ? 1 : 0;
		if (mode != kRead) {
			result->location = rule->location;
			return Walk(root, parts, first, g_userCodepage, mode, &result->path);
		}
		if (Walk(root, parts, first, g_userCodepage, kRead, &result->path)) {
			result->location = rule->location;
			return true;
		}
		// not written yet: the game folder may ship one (notices, emblems)
	} else if (mode != kRead) {
		bf_tracef(BFT_ERR, "file: refused to write into the game folder: %s", clientPath);
		bf_count("file.write_refused", 1);
		errno = EACCES;
		return false;
	}
	result->location = BF_LOCATION_GAME;
	return Walk(game, parts, 0, 949, kRead, &result->path);
}

bool InsideGameFolder(const std::string &nativePath)
{
	std::string rest;
	return StartsWithFolder(nativePath, GameFolder(), &rest);
}

unsigned CodepageOf(bf_location location)
{
	return location == BF_LOCATION_GAME ? 949 : g_userCodepage;
}

void FillInfo(const std::string &path, const struct stat &st, bf_location location, bool foreign,
              bf_file_info *info)
{
	size_t slash = path.rfind('/');
	std::string utf8 = slash == std::string::npos ? path : path.substr(slash + 1);
	std::string client = Encode(utf8, CodepageOf(location));
	strlcpy(info->name, client.c_str(), sizeof(info->name));
	strlcpy(info->name_utf8, utf8.c_str(), sizeof(info->name_utf8));
	info->size = S_ISDIR(st.st_mode) ? 0 : (uint64_t)st.st_size;
	info->modified = (int64_t)st.st_mtimespec.tv_sec;
	info->is_folder = S_ISDIR(st.st_mode) ? 1 : 0;
	if (foreign)
		info->read_only = access(path.c_str(), W_OK) == 0 ? 0 : 1;
	else
		info->read_only = location == BF_LOCATION_GAME ? 1 : 0;
}

bool ReadFully(int fd, void *buffer, size_t size)
{
	char *p = (char *)buffer;
	while (size > 0) {
		ssize_t n = read(fd, p, size > (1u << 30) ? (1u << 30) : size);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return false;
		p += n;
		size -= (size_t)n;
	}
	return true;
}

} // namespace

// ---- Locations ------------------------------------------------------------------

extern "C" const char *bf_file_location(bf_location location)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	return Location(location);
}

extern "C" const char *bf_file_home(void)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	return Home().c_str();
}

extern "C" void bf_file_set_game_folder(const char *path)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	g_hasExplicitGameFolder = path != NULL;
	g_explicitGameFolder = path ? path : "";
	g_gameFolderReady = false;
	g_listings.clear();
}

extern "C" void bf_file_set_user_codepage(unsigned codepage)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	g_userCodepage = codepage;
}

extern "C" unsigned bf_file_user_codepage(void)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	return g_userCodepage;
}

extern "C" bf_location bf_file_write_location(const char *client_path)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	std::string path = client_path ? client_path : "";
	std::string rest;
	if (!path.empty() && path[0] == '/') {
		for (int i = 1; i <= 3; ++i)
			if (StartsWithFolder(path, LocationPath(i), &rest))
				return (bf_location)i;
		if (!StartsWithFolder(path, GameFolder(), &rest))
			return BF_LOCATION_GAME;
		path = rest;
	} else if (path.size() >= 2 && path[1] == ':') {
		path = path.substr(2);
	}
	std::vector<std::string> parts;
	if (!Split(path, &parts) || parts.empty())
		return BF_LOCATION_GAME;
	const WriteRule *rule = RuleFor(parts[0]);
	return rule ? rule->location : BF_LOCATION_GAME;
}

extern "C" int bf_file_resolve(const char *client_path, int mode, char *out, size_t out_size)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	Resolved r;
	if (!Resolve(client_path, mode == BF_RESOLVE_WRITE ? kWrite : kRead, &r)) {
		BF_TRACE(BFT_FILE, "file: %s not resolved: %s", client_path ? client_path : "(null)", strerror(errno));
		return -1;
	}
	if (r.path.size() + 1 > out_size) {
		errno = ENAMETOOLONG;
		return -1;
	}
	memcpy(out, r.path.c_str(), r.path.size() + 1);
	return (int)r.path.size();
}

// ---- Open files ------------------------------------------------------------------

extern "C" bf_file *bf_file_open(const char *client_path, unsigned flags)
{
	std::string path;
	bool write = (flags & (BF_FILE_WRITE | BF_FILE_APPEND)) != 0;
	{
		std::lock_guard<std::recursive_mutex> guard(g_lock);
		if (flags & BF_FILE_NATIVE) {
			if (!client_path) {
				errno = EINVAL;
				return NULL;
			}
			path = client_path;
			if (write && InsideGameFolder(path)) {
				bf_tracef(BFT_ERR, "file: refused to write into the game folder: %s", client_path);
				bf_count("file.write_refused", 1);
				errno = EACCES;
				return NULL;
			}
		} else {
			Resolved r;
			if (!Resolve(client_path, write ? kWrite : kRead, &r)) {
				BF_TRACE(BFT_FILE, "file: open %s failed: %s", client_path ? client_path : "(null)", strerror(errno));
				return NULL;
			}
			path = r.path;
		}
	}

	int oflags = O_CLOEXEC;
	if (write)
		oflags |= (flags & BF_FILE_READ) ? O_RDWR : O_WRONLY;
	else
		oflags |= O_RDONLY;
	if (flags & BF_FILE_CREATE)
		oflags |= O_CREAT;
	if ((flags & BF_FILE_TRUNCATE) && write)
		oflags |= O_TRUNC;
	if ((flags & BF_FILE_EXCLUSIVE) && (flags & BF_FILE_CREATE))
		oflags |= O_EXCL;
	if (flags & BF_FILE_APPEND)
		oflags |= O_APPEND;

	int fd = open(path.c_str(), oflags, 0644);
	if (fd < 0) {
		int error = errno;
		BF_TRACE(BFT_FILE, "file: open %s: %s", path.c_str(), strerror(error));
		errno = error;
		return NULL;
	}
	struct stat st;
	if (fstat(fd, &st) == 0 && S_ISDIR(st.st_mode)) {
		close(fd);
		errno = EISDIR;
		return NULL;
	}
	bf_file *file = new bf_file;
	file->fd = fd;
	file->path = path;
	return file;
}

extern "C" int64_t bf_file_read(bf_file *file, void *buffer, size_t size)
{
	if (!file)
		return -1;
	char *p = (char *)buffer;
	size_t total = 0;
	while (total < size) {
		size_t chunk = size - total;
		if (chunk > (1u << 30))
			chunk = 1u << 30;
		ssize_t n = read(file->fd, p + total, chunk);
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0)
			return total ? (int64_t)total : -1;
		if (n == 0)
			break;
		total += (size_t)n;
	}
	return (int64_t)total;
}

extern "C" int64_t bf_file_write(bf_file *file, const void *buffer, size_t size)
{
	if (!file)
		return -1;
	const char *p = (const char *)buffer;
	size_t total = 0;
	while (total < size) {
		ssize_t n = write(file->fd, p + total, size - total);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return total ? (int64_t)total : -1;
		total += (size_t)n;
	}
	return (int64_t)total;
}

extern "C" int64_t bf_file_seek(bf_file *file, int64_t offset, int whence)
{
	if (!file)
		return -1;
	int w = whence == 1 ? SEEK_CUR : whence == 2 ? SEEK_END : SEEK_SET;
	off_t at = lseek(file->fd, (off_t)offset, w);
	return at < 0 ? -1 : (int64_t)at;
}

extern "C" int64_t bf_file_tell(bf_file *file)
{
	return bf_file_seek(file, 0, 1);
}

extern "C" int64_t bf_file_size(bf_file *file)
{
	struct stat st;
	if (!file || fstat(file->fd, &st) != 0)
		return -1;
	return (int64_t)st.st_size;
}

extern "C" int bf_file_truncate_here(bf_file *file)
{
	int64_t at = bf_file_tell(file);
	if (at < 0)
		return -1;
	return ftruncate(file->fd, (off_t)at) == 0 ? 0 : -1;
}

extern "C" const char *bf_file_path(bf_file *file)
{
	return file ? file->path.c_str() : NULL;
}

extern "C" void bf_file_close(bf_file *file)
{
	if (file) {
		close(file->fd);
		delete file;
	}
}

extern "C" void *bf_file_read_all(const char *client_path, size_t *size)
{
	bf_file *file = bf_file_open(client_path, BF_FILE_READ);
	if (!file)
		return NULL;
	int64_t length = bf_file_size(file);
	char *data = length < 0 ? NULL : (char *)malloc((size_t)length + 8);
	if (!data || !ReadFully(file->fd, data, (size_t)length)) {
		int error = errno;
		free(data);
		bf_file_close(file);
		errno = error ? error : EIO;
		return NULL;
	}
	memset(data + length, 0, 8);
	bf_file_close(file);
	if (size)
		*size = (size_t)length;
	return data;
}

extern "C" void bf_file_free(void *data)
{
	free(data);
}

// ---- Mappings ------------------------------------------------------------------

extern "C" bf_file_mapping *bf_file_map_open_file(bf_file *file)
{
	if (!file)
		return NULL;
	struct stat st;
	if (fstat(file->fd, &st) != 0)
		return NULL;
	bf_file_mapping *mapping = new bf_file_mapping;
	mapping->size = (uint64_t)st.st_size;
	mapping->data = NULL;
	if (mapping->size) {
		void *data = mmap(NULL, (size_t)mapping->size, PROT_READ, MAP_PRIVATE, file->fd, 0);
		if (data == MAP_FAILED) {
			bf_tracef(BFT_ERR, "file: mmap %s: %s", file->path.c_str(), strerror(errno));
			delete mapping;
			return NULL;
		}
		mapping->data = data;
	}
	return mapping;
}

extern "C" bf_file_mapping *bf_file_map(const char *client_path)
{
	bf_file *file = bf_file_open(client_path, BF_FILE_READ);
	if (!file)
		return NULL;
	bf_file_mapping *mapping = bf_file_map_open_file(file);
	bf_file_close(file);
	return mapping;
}

extern "C" const void *bf_file_mapping_data(const bf_file_mapping *mapping)
{
	return mapping ? mapping->data : NULL;
}

extern "C" uint64_t bf_file_mapping_size(const bf_file_mapping *mapping)
{
	return mapping ? mapping->size : 0;
}

extern "C" void bf_file_unmap(bf_file_mapping *mapping)
{
	if (mapping) {
		if (mapping->data)
			munmap(mapping->data, (size_t)mapping->size);
		delete mapping;
	}
}

// ---- Folders ------------------------------------------------------------------

extern "C" int bf_file_stat(const char *client_path, bf_file_info *info)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	Resolved r;
	if (!Resolve(client_path, kRead, &r))
		return 0;
	struct stat st;
	if (stat(r.path.c_str(), &st) != 0)
		return 0;
	if (info)
		FillInfo(r.path, st, r.location, r.foreign, info);
	return 1;
}

extern "C" bf_file_find *bf_file_find_first(const char *client_pattern, bf_file_info *info)
{
	if (!client_pattern) {
		errno = EINVAL;
		return NULL;
	}
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	std::string pattern = client_pattern;
	size_t cut = pattern.find_last_of("/\\");
	std::string folderPart = cut == std::string::npos ? std::string() : pattern.substr(0, cut);
	std::string last = cut == std::string::npos ? pattern : pattern.substr(cut + 1);

	bf_file_find *find = new bf_file_find;
	find->next = 0;

	if (!HasWildcard(last)) {
		Resolved r;
		struct stat st;
		if (!last.empty() && Resolve(client_pattern, kRead, &r) && stat(r.path.c_str(), &st) == 0) {
			bf_file_info entry;
			FillInfo(r.path, st, r.location, r.foreign, &entry);
			find->entries.push_back(entry);
		}
	} else {
		Resolved r;
		bool found;
		if (folderPart.empty()) {
			r.path = GameFolder();
			r.location = BF_LOCATION_GAME;
			r.foreign = false;
			found = true;
		} else {
			found = Resolve(folderPart.c_str(), kRead, &r);
		}
		std::string foldedPattern;
		if (found) {
			std::string decoded;
			if (Decode(last, CodepageOf(r.location), &decoded))
				foldedPattern = Fold(decoded);
			else
				found = false;
		}
		DIR *dir = found ? opendir(r.path.c_str()) : NULL;
		if (dir) {
			std::map<std::string, bf_file_info> sorted;
			while (struct dirent *e = readdir(dir)) {
				std::string name = e->d_name;
				if (name == "." || name == "..")
					continue;
				if (!WildcardMatch(foldedPattern, Fold(name)))
					continue;
				std::string full = r.path + "/" + name;
				struct stat st;
				if (stat(full.c_str(), &st) != 0)
					continue;
				bf_file_info entry;
				FillInfo(full, st, r.location, r.foreign, &entry);
				sorted[name] = entry;
			}
			closedir(dir);
			for (std::map<std::string, bf_file_info>::iterator it = sorted.begin(); it != sorted.end(); ++it)
				find->entries.push_back(it->second);
		}
	}

	if (find->entries.empty()) {
		delete find;
		errno = ENOENT;
		return NULL;
	}
	if (info)
		*info = find->entries[0];
	find->next = 1;
	return find;
}

extern "C" int bf_file_find_next(bf_file_find *find, bf_file_info *info)
{
	if (!find || find->next >= find->entries.size())
		return 0;
	if (info)
		*info = find->entries[find->next];
	++find->next;
	return 1;
}

extern "C" void bf_file_find_close(bf_file_find *find)
{
	delete find;
}

extern "C" int bf_file_make_folder(const char *client_path)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	Resolved r;
	struct stat st;
	// Whether it is there is asked before resolving for writing: that
	// creates a write location on its first use, and a path that names the
	// location itself (<game>\ScreenShot is Pictures/<name>) would then be
	// reported as existing on the call that made it (CMode::ScreenShot gives
	// up when _mkdir fails, so the first screenshot was lost).
	if (Resolve(client_path, kRead, &r) && stat(r.path.c_str(), &st) == 0)
		return S_ISDIR(st.st_mode) ? 0 : (errno = EEXIST, -1);
	if (!Resolve(client_path, kWrite, &r))
		return -1;
	if (stat(r.path.c_str(), &st) == 0)
		return S_ISDIR(st.st_mode) ? 1 : (errno = EEXIST, -1);
	if (!MakeFolders(r.path))
		return -1;
	return 1;
}

extern "C" int bf_file_remove(const char *client_path)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	Resolved r;
	if (!Resolve(client_path, kWriteNoCreate, &r))
		return -1;
	return unlink(r.path.c_str()) == 0 ? 0 : -1;
}

extern "C" int bf_file_remove_folder(const char *client_path)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	Resolved r;
	if (!Resolve(client_path, kWriteNoCreate, &r))
		return -1;
	if (rmdir(r.path.c_str()) != 0)
		return -1;
	g_listings.erase(r.path);
	return 0;
}

extern "C" int bf_file_rename(const char *from_client_path, const char *to_client_path)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	Resolved from, to;
	if (!Resolve(from_client_path, kWriteNoCreate, &from) || !Resolve(to_client_path, kWrite, &to))
		return -1;
	return rename(from.path.c_str(), to.path.c_str()) == 0 ? 0 : -1;
}

extern "C" void bf_file_flush_cache(void)
{
	std::lock_guard<std::recursive_mutex> guard(g_lock);
	g_listings.clear();
}
