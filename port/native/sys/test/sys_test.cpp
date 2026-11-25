// bifrost-sys-test: one line per check, non-zero exit on a failure.
//
// Writes only below its own build directory (BF_HOME is pointed there before
// the module is used). Checks on the real game folder read BF_CLIENT_DIR
// (default ~/Games/<name>) and are skipped when it is absent.

#include "bf_sys.h"
#include "bf_trace.h"

#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

#include <atomic>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_passes = 0;
int g_skips = 0;

void Check(bool ok, const char *name, const char *detail = "")
{
	if (ok) {
		++g_passes;
		printf("ok    %s\n", name);
	} else {
		++g_failures;
		printf("FAIL  %s%s%s\n", name, detail[0] ? ": " : "", detail);
	}
	fflush(stdout);
}

__attribute__((unused)) void Skip(const char *name, const char *why)
{
	++g_skips;
	printf("skip  %s: %s\n", name, why);
	fflush(stdout);
}

std::string Format(const char *format, ...) __attribute__((format(printf, 1, 2)));
std::string Format(const char *format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	return buffer;
}

int RemoveEntry(const char *path, const struct stat *, int, struct FTW *)
{
	return remove(path);
}

void RemoveTree(const std::string &path)
{
	nftw(path.c_str(), RemoveEntry, 16, FTW_DEPTH | FTW_PHYS);
}

__attribute__((unused)) bool Exists(const std::string &path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

__attribute__((unused)) void WriteText(const std::string &path, const char *text)
{
	FILE *f = fopen(path.c_str(), "wb");
	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

__attribute__((unused)) std::string ReadText(const std::string &path)
{
	std::string out;
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return out;
	char buffer[4096];
	size_t n;
	while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
		out.append(buffer, n);
	fclose(f);
	return out;
}

// UTF-8 in NFD, as the Finder would write a Korean name.
std::string Nfd(const char *utf8)
{
	CFStringRef s = CFStringCreateWithCString(NULL, utf8, kCFStringEncodingUTF8);
	CFMutableStringRef m = CFStringCreateMutableCopy(NULL, 0, s);
	CFRelease(s);
	CFStringNormalize(m, kCFStringNormalizationFormD);
	char buffer[1024];
	CFStringGetCString(m, buffer, sizeof(buffer), kCFStringEncodingUTF8);
	CFRelease(m);
	return buffer;
}

std::string ExecutableFolder()
{
	char path[PATH_MAX];
	uint32_t size = sizeof(path);
	if (_NSGetExecutablePath(path, &size) != 0)
		return ".";
	char real[PATH_MAX];
	if (!realpath(path, real))
		return ".";
	std::string s = real;
	return s.substr(0, s.rfind('/'));
}

std::string Resolve(const char *clientPath, int mode)
{
	char out[2048];
	if (bf_file_resolve(clientPath, mode, out, sizeof(out)) < 0)
		return std::string();
	return out;
}

const char kKoreanUtf8[] = "\xEC\x9C\xA0\xEC\xA0\x80\xEC\x9D\xB8\xED\x84\xB0\xED\x8E\x98\xEC\x9D\xB4\xEC\x8A\xA4"; // 유저인터페이스
const char kKoreanCp949[] = "\xc0\xaf\xc0\xfa\xc0\xce\xc5\xcd\xc6\xe4\xc0\xcc\xbd\xba";

// ---- Clock --------------------------------------------------------------------

uint64_t g_fakeNow = 0;
uint64_t g_fakeSlept = 0;
uint64_t FakeNow(void *) { return g_fakeNow; }
void FakeSleep(void *, uint64_t us) { g_fakeSlept += us; g_fakeNow += us; }

void TestClock()
{
	uint64_t last = bf_clock_us();
	bool monotonic = true;
	for (int i = 0; i < 200000; ++i) {
		uint64_t now = bf_clock_us();
		if (now < last)
			monotonic = false;
		last = now;
	}
	Check(monotonic, "clock: 200000 readings never go backwards");

	uint64_t us = bf_clock_us();
	uint32_t ms = bf_clock_ms();
	Check(ms >= us / 1000 && ms <= us / 1000 + 5, "clock: milliseconds agree with microseconds");
	uint32_t ticks = bf_clock_ticks();
	Check(ticks - ms >= 3600000u && ticks - ms <= 3600005u, "clock: ticks are milliseconds plus an hour");

	bool accurate = true;
	std::string detail;
	for (uint32_t wanted : { 1u, 5u, 20u, 50u }) {
		uint64_t start = bf_clock_us();
		bf_clock_sleep_ms(wanted);
		double took = (bf_clock_us() - start) / 1000.0;
		detail += Format("%u->%.2f ", wanted, took);
		if (took < wanted || took > wanted + 15)
			accurate = false;
	}
	Check(accurate, Format("clock: sleep accuracy (ms asked->taken: %s)", detail.c_str()).c_str());

	uint64_t start = bf_clock_us();
	for (int i = 0; i < 100; ++i)
		bf_clock_sleep_ms(0);
	Check(bf_clock_us() - start < 20000, "clock: sleep 0 only yields");

	bf_clock_source source = { FakeNow, FakeSleep, NULL };
	g_fakeNow = 5000000;
	bf_clock_set_source(&source);
	bool followed = bf_clock_ms() == 5000 && bf_clock_us() == 5000000 && bf_clock_ticks() == 3605000u;
	start = bf_clock_us();
	bf_clock_sleep_ms(16);
	followed = followed && g_fakeSlept == 16000 && bf_clock_us() - start == 16000;
	bf_clock_set_source(NULL);
	bool restored = bf_clock_us() < 5000000 && bf_clock_us() >= last;
	Check(followed && restored, "clock: an installed source is followed, and removing it restores the real clock");

	bf_clock_date date;
	bf_clock_local_time(&date);
	bf_clock_date utc;
	bf_clock_utc_time(&utc);
	Check(date.year >= 2024 && date.month >= 1 && date.month <= 12 && date.day >= 1 && date.weekday >= 0
	      && date.weekday <= 6 && utc.year >= 2024,
	      Format("clock: calendar %04d-%02d-%02d %02d:%02d:%02d.%03d", date.year, date.month, date.day,
	             date.hour, date.minute, date.second, date.millisecond).c_str());
}

// ---- Threads -----------------------------------------------------------------

std::atomic<int> g_gate(0);

uint32_t ReturnArgument(void *argument)
{
	return (uint32_t)(uintptr_t)argument;
}

uint32_t WaitForGate(void *)
{
	while (!g_gate.load())
		bf_clock_sleep_ms(1);
	return 7;
}

uint32_t IsMain(void *)
{
	return (uint32_t)bf_sys_is_main_thread();
}

struct Contention {
	bf_sys_mutex *mutex;
	long counter;
};

uint32_t Increment(void *raw)
{
	Contention *c = (Contention *)raw;
	for (int i = 0; i < 100000; ++i) {
		bf_sys_mutex_lock(c->mutex);
		bf_sys_mutex_lock(c->mutex);      // recursive, like a critical section
		long v = c->counter;
		c->counter = v + 1;
		bf_sys_mutex_unlock(c->mutex);
		bf_sys_mutex_unlock(c->mutex);
	}
	return 0;
}

uint32_t TryLockFromOther(void *raw)
{
	return (uint32_t)bf_sys_mutex_try_lock((bf_sys_mutex *)raw);
}

uint64_t g_otherId = 0;
uint32_t RecordId(void *)
{
	g_otherId = bf_sys_thread_id();
	return 0;
}

void TestThreads()
{
	bf_sys_thread *t = bf_sys_thread_create(ReturnArgument, (void *)(uintptr_t)42, "bf-test");
	uint32_t result = 0;
	Check(t && bf_sys_thread_wait(t, BF_SYS_WAIT_FOREVER, &result) == 1 && result == 42
	      && bf_sys_thread_wait(t, 0, &result) == 1,
	      "threads: create, wait twice, result passed back");
	bf_sys_thread_release(t);

	g_gate = 0;
	t = bf_sys_thread_create(WaitForGate, NULL, NULL);
	bool timedOut = bf_sys_thread_wait(t, 30, NULL) == 0;
	g_gate = 1;
	Check(timedOut && bf_sys_thread_join(t) == 7, "threads: wait times out on a running thread; join returns its value");

	g_gate = 0;
	t = bf_sys_thread_create(WaitForGate, NULL, NULL);
	bf_sys_thread_release(t);   // released while running: the thread must still finish safely
	g_gate = 1;
	bf_clock_sleep_ms(20);
	Check(true, "threads: handle released while the thread runs");

	t = bf_sys_thread_create(IsMain, NULL, NULL);
	Check(bf_sys_is_main_thread() == 1 && bf_sys_thread_join(t) == 0, "threads: main thread recognised, worker is not main");

	t = bf_sys_thread_create(RecordId, NULL, NULL);
	bf_sys_thread_join(t);
	Check(bf_sys_thread_id() != 0 && g_otherId != 0 && g_otherId != bf_sys_thread_id(), "threads: ids are non-zero and distinct");

	Contention c = { bf_sys_mutex_create(), 0 };
	std::vector<bf_sys_thread *> threads;
	for (int i = 0; i < 8; ++i)
		threads.push_back(bf_sys_thread_create(Increment, &c, "bf-contention"));
	for (bf_sys_thread *th : threads)
		bf_sys_thread_join(th);
	Check(c.counter == 800000, Format("mutex: 8 threads x 100000 nested increments = %ld", c.counter).c_str());

	bf_sys_mutex_lock(c.mutex);
	t = bf_sys_thread_create(TryLockFromOther, c.mutex, NULL);
	uint32_t got = bf_sys_thread_join(t);
	bool ownAgain = bf_sys_mutex_try_lock(c.mutex) == 1;
	bf_sys_mutex_unlock(c.mutex);
	bf_sys_mutex_unlock(c.mutex);
	Check(got == 0 && ownAgain, "mutex: held lock refuses another thread, admits its owner");
	bf_sys_mutex_destroy(c.mutex);
}

// ---- Locations and files --------------------------------------------------------

void TestLocations(const std::string &home)
{
	const char *support = bf_file_location(BF_LOCATION_SUPPORT);
	const char *pictures = bf_file_location(BF_LOCATION_PICTURES);
	const char *documents = bf_file_location(BF_LOCATION_DOCUMENTS);
	Check(std::string(bf_file_home()) == home, "locations: BF_HOME replaces the home folder");
	Check(support && std::string(support) == home + "/Library/Application Support/" BF_APP_NAME && Exists(support),
	      Format("locations: support folder created at %s", support ? support : "(null)").c_str());
	Check(pictures && std::string(pictures) == home + "/Pictures/" BF_APP_NAME && Exists(pictures),
	      "locations: pictures folder created");
	Check(documents && std::string(documents) == home + "/Documents/" BF_APP_NAME && Exists(documents),
	      "locations: documents folder created");
}

void TestLookup(const std::string &game)
{
	// A game folder like the real one: mixed case, a Korean name stored in NFD.
	std::string korean = Nfd(kKoreanUtf8);
	std::string texture = game + "/Data/Texture/" + korean;
	mkdir((game + "/Data").c_str(), 0755);
	mkdir((game + "/Data/Texture").c_str(), 0755);
	mkdir(texture.c_str(), 0755);
	WriteText(texture + "/Bgi_Temp.BMP", "BM-test");
	WriteText(game + "/Data/clientinfo.xml", "<clientinfo/>");
	WriteText(game + "/data.grf", "Master of Magic\0fake");
	bf_file_set_game_folder(game.c_str());
	Check(std::string(bf_file_location(BF_LOCATION_GAME)) == game, "lookup: game folder set");

	std::string hit = Resolve("data\\clientinfo.xml", BF_RESOLVE_READ);
	Check(hit == game + "/Data/clientinfo.xml" || (!hit.empty() && Exists(hit)),
	      Format("lookup: backslashes and lower case find Data/clientinfo.xml (%s)", hit.c_str()).c_str());

	hit = Resolve("DATA/CLIENTINFO.XML", BF_RESOLVE_READ);
	Check(!hit.empty() && Exists(hit), "lookup: upper case with slashes");

	hit = Resolve("C:\\data\\..\\data\\.\\clientinfo.xml", BF_RESOLVE_READ);
	Check(!hit.empty() && Exists(hit), "lookup: drive letter, '.' and '..' handled");

	hit = Resolve((game + "\\data\\clientinfo.xml").c_str(), BF_RESOLVE_READ);
	Check(!hit.empty() && Exists(hit), "lookup: absolute game-folder prefix with a backslash tail");

	errno = 0;
	Check(Resolve("..\\outside.txt", BF_RESOLVE_READ).empty(), "lookup: '..' cannot leave the game folder");

	std::string cp949Path = std::string("data\\texture\\") + kKoreanCp949 + "\\bgi_temp.bmp";
	hit = Resolve(cp949Path.c_str(), BF_RESOLVE_READ);
	Check(!hit.empty() && ReadText(hit) == "BM-test",
	      Format("lookup: CP949 name finds the NFD folder %s", hit.c_str()).c_str());

	errno = 0;
	bool missing = Resolve("data\\texture\\nothere.bmp", BF_RESOLVE_READ).empty() && errno == ENOENT;
	Check(missing, "lookup: an absent file fails with ENOENT");

	// Enumeration goes through the folded directory listing, whatever the volume's case rules.
	bf_file_info info;
	bf_file_find *find = bf_file_find_first((std::string("DATA\\TEXTURE\\") + kKoreanCp949 + "\\B?I_*.bmp").c_str(), &info);
	Check(find && strcmp(info.name_utf8, "Bgi_Temp.BMP") == 0 && info.read_only == 1 && info.size == 7
	      && !bf_file_find_next(find, &info),
	      "find: wildcard pattern in other case finds the one file, read-only");
	bf_file_find_close(find);

	find = bf_file_find_first("data\\texture\\*.*", &info);
	Check(find && info.is_folder && strcmp(info.name, kKoreanCp949) == 0,
	      "find: a folder's Korean name comes back in CP949 bytes");
	bf_file_find_close(find);

	Check(bf_file_find_first("data\\*.nothing", &info) == NULL, "find: no match gives NULL");
	Check(bf_file_stat("DATA\\Texture", &info) == 1 && info.is_folder, "stat: folder found case-insensitively");

	size_t size = 0;
	char *data = (char *)bf_file_read_all("data\\clientinfo.xml", &size);
	Check(data && size == 13 && strcmp(data, "<clientinfo/>") == 0 && data[size + 7] == 0,
	      "read_all: contents and zero padding");
	bf_file_free(data);

	bf_file_mapping *map = bf_file_map("DATA.GRF");
	Check(map && bf_file_mapping_size(map) >= 15 && memcmp(bf_file_mapping_data(map), "Master of Magic", 15) == 0,
	      "mmap: a test archive's header");
	bf_file_unmap(map);
}

void TestRefusal(const std::string &game)
{
	errno = 0;
	bf_file *f = bf_file_open("data\\new.txt", BF_FILE_WRITE | BF_FILE_CREATE | BF_FILE_TRUNCATE);
	Check(!f && errno == EACCES && !Exists(game + "/Data/new.txt"), "refusal: a write into the game folder is refused");
	errno = 0;
	f = bf_file_open((game + "/data.grf").c_str(), BF_FILE_READ | BF_FILE_WRITE | BF_FILE_NATIVE);
	Check(!f && errno == EACCES, "refusal: also through a native absolute path");
	errno = 0;
	Check(Resolve("data\\clientinfo.xml", BF_RESOLVE_WRITE).empty() && errno == EACCES,
	      "refusal: resolve for writing refuses existing game files");
	Check(bf_file_make_folder("data\\newfolder") == -1 && !Exists(game + "/Data/newfolder"),
	      "refusal: no folders made in the game folder");
	Check(bf_file_remove("data.grf") == -1 && Exists(game + "/data.grf"), "refusal: nothing removed from the game folder");
}

void TestWritePaths(const std::string &game)
{
	std::string pictures = bf_file_location(BF_LOCATION_PICTURES);
	std::string documents = bf_file_location(BF_LOCATION_DOCUMENTS);
	std::string support = bf_file_location(BF_LOCATION_SUPPORT);

	Check(bf_file_write_location("ScreenShot\\screen000.jpg") == BF_LOCATION_PICTURES
	      && bf_file_write_location("chat\\Chat.txt") == BF_LOCATION_DOCUMENTS
	      && bf_file_write_location("_tmpEmblem\\a.ebm") == BF_LOCATION_SUPPORT
	      && bf_file_write_location("data\\x") == BF_LOCATION_GAME,
	      "write paths: the table sends screenshots, chat and caches to their places");

	// CMode::ScreenShot: access(<game>\ScreenShot), and when that fails
	// _mkdir, giving up unless it reports the folder made. On a first run
	// the pictures folder does not exist yet.
	rmdir(pictures.c_str());
	std::string shots = game + "\\ScreenShot";
	bf_file_info shotInfo;
	bool absent = !bf_file_stat(shots.c_str(), &shotInfo);
	int made = bf_file_make_folder(shots.c_str());
	bool there = bf_file_stat(shots.c_str(), &shotInfo) && shotInfo.is_folder;
	Check(absent && made == 1 && there && bf_file_make_folder(shots.c_str()) == 0 && Exists(pictures),
	      Format("write paths: _mkdir <game>\\ScreenShot makes Pictures/<name> (%d %d %d)", absent, made, there).c_str());

	bf_file *f = bf_file_open((game + "\\ScreenShot\\screen000.jpg").c_str(),
	                          BF_FILE_WRITE | BF_FILE_CREATE | BF_FILE_TRUNCATE);
	bool wrote = f && bf_file_write(f, "jpeg", 4) == 4;
	bf_file_close(f);
	Check(wrote && ReadText(pictures + "/screen000.jpg") == "jpeg" && !Exists(game + "/ScreenShot"),
	      "write paths: <game>\\ScreenShot\\screen000.jpg lands in Pictures");

	Check(bf_file_make_folder("Chat") == 1 && bf_file_make_folder("Chat") == 0 && Exists(documents + "/Chat"),
	      "write paths: _mkdir Chat creates Documents/<name>/Chat once");
	f = bf_file_open("Chat\\Chat.txt", BF_FILE_WRITE | BF_FILE_CREATE | BF_FILE_TRUNCATE);
	wrote = f && bf_file_write(f, "hello", 5) == 5;
	bf_file_close(f);
	bf_file_info info;
	Check(wrote && ReadText(documents + "/Chat/Chat.txt") == "hello" && bf_file_stat("chat\\chat.txt", &info) == 1
	      && info.read_only == 0 && info.size == 5,
	      "write paths: a chat log goes to Documents and is found again for reading");

	f = bf_file_open("Report\\Someone.txt", BF_FILE_WRITE | BF_FILE_CREATE);   // folder made on demand
	bf_file_close(f);
	Check(f && Exists(documents + "/Report/Someone.txt"), "write paths: missing folders are made on demand");

	bf_file_make_folder("_tmpEmblem");
	f = bf_file_open("_tmpEmblem\\srv_1_2.ebm", BF_FILE_WRITE | BF_FILE_CREATE);
	bf_file_close(f);
	bf_file_find *find = bf_file_find_first("_tmpemblem\\srv_1_*.ebm", &info);
	Check(find && strcmp(info.name, "srv_1_2.ebm") == 0 && Exists(support + "/_tmpEmblem/srv_1_2.ebm"),
	      "write paths: emblem cache in Application Support, listed by wildcard");
	bf_file_find_close(find);
	Check(bf_file_rename("_tmpEmblem\\srv_1_2.ebm", "_tmpEmblem\\srv_1_3.ebm") == 0
	      && bf_file_remove("_TMPEMBLEM\\srv_1_3.ebm") == 0 && !Exists(support + "/_tmpEmblem/srv_1_3.ebm"),
	      "write paths: rename and remove inside a write location");

	// Names the client makes from user text, in the user code page.
	bf_file_set_user_codepage(1252);
	f = bf_file_open("Whisper\\J\xf6hn.txt", BF_FILE_WRITE | BF_FILE_CREATE);
	bf_file_close(f);
	find = bf_file_find_first("Whisper\\*.txt", &info);
	Check(f && Exists(documents + "/Whisper/J\xc3\xb6hn.txt") && find && strcmp(info.name, "J\xf6hn.txt") == 0,
	      "write paths: a CP1252 name is stored as UTF-8 and listed back as CP1252");
	bf_file_find_close(find);
	bf_file_set_user_codepage(949);
	Check(bf_file_user_codepage() == 949, "write paths: the user code page reads back");

	bf_file_make_folder("_tmpskin_\\empty");
	Check(bf_file_remove_folder("_tmpskin_\\EMPTY") == 0 && !Exists(support + "/_tmpskin_/empty")
	      && bf_file_remove_folder("data") == -1 && errno == EACCES,
	      "write paths: an empty folder is removed in a write location, refused in the game folder");

	// Paths native code built (UTF-8) under a write location stay UTF-8.
	std::string nativeKorean = documents + "/Chat/" + kKoreanUtf8 + ".txt";
	f = bf_file_open(nativeKorean.c_str(), BF_FILE_WRITE | BF_FILE_CREATE);
	bf_file_close(f);
	Check(f && Exists(nativeKorean) && bf_file_stat(nativeKorean.c_str(), &info) == 1,
	      "write paths: an absolute UTF-8 path in a write location is taken as UTF-8");

	// File handle operations.
	f = bf_file_open("savedata\\test.bin", BF_FILE_READ | BF_FILE_WRITE | BF_FILE_CREATE | BF_FILE_TRUNCATE);
	bool ops = f && bf_file_write(f, "0123456789", 10) == 10 && bf_file_size(f) == 10 && bf_file_seek(f, 4, 0) == 4;
	char buffer[16] = { 0 };
	ops = ops && bf_file_read(f, buffer, 3) == 3 && memcmp(buffer, "456", 3) == 0 && bf_file_tell(f) == 7;
	ops = ops && bf_file_seek(f, -2, 2) == 8 && bf_file_read(f, buffer, 10) == 2 && bf_file_read(f, buffer, 10) == 0;
	ops = ops && bf_file_seek(f, 5, 0) == 5 && bf_file_truncate_here(f) == 0 && bf_file_size(f) == 5;
	bf_file_close(f);
	f = bf_file_open("savedata\\test.bin", BF_FILE_APPEND | BF_FILE_WRITE);
	ops = ops && f && bf_file_write(f, "X", 1) == 1;
	bf_file_close(f);
	ops = ops && ReadText(support + "/savedata/test.bin") == "01234X";
	errno = 0;
	f = bf_file_open("savedata\\test.bin", BF_FILE_WRITE | BF_FILE_CREATE | BF_FILE_EXCLUSIVE);
	ops = ops && !f && errno == EEXIST;
	Check(ops, "files: write, size, seek, read, short read at the end, truncate, append, exclusive create");

	errno = 0;
	f = bf_file_open("data\\texture", BF_FILE_READ);
	Check(!f && errno == EISDIR, "files: a folder cannot be opened as a file");
}

uint32_t ResolveMany(void *raw)
{
	const std::string *expected = (const std::string *)raw;
	for (int i = 0; i < 2000; ++i) {
		std::string got = Resolve(i % 2 ? "DATA\\CLIENTINFO.XML" : "data/clientinfo.xml", BF_RESOLVE_READ);
		if (got != *expected)
			return 1;
	}
	return 0;
}

void TestConcurrentLookups()
{
	std::string expected = Resolve("data\\clientinfo.xml", BF_RESOLVE_READ);
	std::vector<bf_sys_thread *> threads;
	for (int i = 0; i < 8; ++i)
		threads.push_back(bf_sys_thread_create(ResolveMany, &expected, "bf-lookup"));
	uint32_t failures = 0;
	for (bf_sys_thread *t : threads)
		failures += bf_sys_thread_join(t);
	Check(failures == 0, "lookup: 8 threads x 2000 lookups agree");
}

void TestRealGameFolder(const std::string &folder)
{
	if (!Exists(folder + "/data.grf")) {
		Skip("game folder: data.grf header", ("no data.grf in " + folder).c_str());
		Skip("game folder: listing *.grf", "no game folder");
		Skip("game folder: bgm listing", "no game folder");
		return;
	}
	bf_file_set_game_folder(folder.c_str());
	bf_file_mapping *map = bf_file_map("data.grf");
	Check(map && bf_file_mapping_size(map) > 46 && memcmp(bf_file_mapping_data(map), "Master of Magic", 15) == 0,
	      Format("game folder: mmap of data.grf (%.1f GB) reads \"Master of Magic\"",
	             map ? bf_file_mapping_size(map) / 1e9 : 0.0).c_str());
	if (map) {
		// touch the last page too: the whole archive is mapped
		volatile unsigned char last = ((const unsigned char *)bf_file_mapping_data(map))[bf_file_mapping_size(map) - 1];
		(void)last;
	}
	bf_file_unmap(map);

	bf_file_info info;
	int grfs = 0;
	bf_file_find *find = bf_file_find_first("*.GRF", &info);
	for (bool more = find != NULL; more; more = bf_file_find_next(find, &info) != 0)
		++grfs;
	bf_file_find_close(find);
	Check(grfs >= 2, Format("game folder: *.GRF lists %d archives", grfs).c_str());

	int mp3 = 0;
	find = bf_file_find_first("bgm\\*.mp3", &info);
	for (bool more = find != NULL; more; more = bf_file_find_next(find, &info) != 0)
		++mp3;
	bf_file_find_close(find);
	if (Exists(folder + "/BGM"))
		Check(mp3 > 0, Format("game folder: bgm\\*.mp3 lists %d files", mp3).c_str());
	else
		Skip("game folder: bgm listing", "no BGM folder");
}

// ---- Settings ------------------------------------------------------------------

uint32_t SetMany(void *raw)
{
	bf_settings *s = (bf_settings *)raw;
	char name[32];
	snprintf(name, sizeof(name), "t%llu", (unsigned long long)bf_sys_thread_id());
	for (int i = 0; i < 500; ++i)
		bf_settings_set_int(s, "Client/Stress", name, i);
	return bf_settings_int(s, "Client/Stress", name, -1) == 499 ? 0 : 1;
}

void TestSettings(const std::string &home, const std::string &fakeGame)
{
	std::string path = home + "/settings-test.plist";
	unlink(path.c_str());
	bf_settings *s = bf_settings_open_file(path.c_str());

	int32_t width = 0;
	char provider[64];
	Check(bf_settings_get_int(s, "Client", "WIDTH", &width) == 1 && width == 1024
	      && bf_settings_int(s, "Client", "NUMSAMPLETYPE", -1) == 1
	      && bf_settings_get_string(s, "Client", "PROVIDERNAME", provider, sizeof(provider), NULL) == 1
	      && bf_settings_type(s, "Client", "GUIDDEVICE") == BF_SETTING_DATA
	      && bf_settings_int(s, "Client\\Option", "streamVolume", 0) == 100,
	      "settings: Setup_HP defaults (WIDTH 1024, 32 voices, provider, GUIDs, Option volumes)");
	Check(bf_settings_option(s, BF_OPT_SKIP_INTRO) == 1 && bf_settings_option(s, BF_OPT_FREE_CAMERA) == 1
	      && bf_settings_option(s, BF_OPT_LAUNCH_ARGUMENTS) == 1 && bf_settings_option(s, BF_OPT_ATTACK_WITHOUT_CTRL) == 0,
	      "settings: convenience options default on, on, on, off");
	Check(bf_settings_has_group(s, "Client") && bf_settings_has_group(s, "client/option")
	      && !bf_settings_has_group(s, "Client/UIRectInfo"),
	      "settings: groups exist from the defaults (RegOpenKeyEx succeeds before Setup)");

	unsigned char guid[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	bool set = bf_settings_set_int(s, "Client", "width", 1280)
	        && bf_settings_set_int(s, "Client/UIRectInfo", "ITEMWNDINFO.X", -5)
	        && bf_settings_set_int(s, "Client/UIRectInfo", "M_MINIMAPARGB", (int32_t)0xFF102030u)
	        && bf_settings_set_string(s, "Client/Option", "SKINNAME", "default")
	        && bf_settings_set_data(s, "Client", "GUIDDEVICE", guid, sizeof(guid))
	        && bf_settings_set_string(s, "Client/BookMark/Local \xc3\x96sterreich/Hero", "BOOKMARK0", "prt_fild08 \xc3\xa9t\xc3\xa9")
	        && bf_settings_set_option(s, BF_OPT_ATTACK_WITHOUT_CTRL, 1)
	        && bf_settings_set_account_name(s, "bftest");
	Check(set, "settings: set int, string, data and option, creating groups");
	Check(bf_settings_save(s) == 1 && bf_settings_save(s) == 0, "settings: save writes once, then has nothing to do");
	bf_settings_close(s);

	std::string text = ReadText(path);
	Check(text.find("<key>FormatVersion</key>") != std::string::npos && text.find("<key>WIDTH</key>") != std::string::npos
	      && text.find("HEIGHT") == std::string::npos && text.find("<key>UIRectInfo</key>") != std::string::npos,
	      "settings: plist holds FormatVersion and set values only (WIDTH spelled as the default, no HEIGHT)");

	s = bf_settings_open_file(path.c_str());
	unsigned char back[16] = { 0 };
	size_t length = 0;
	char name[64];
	char bookmark[64];
	Check(bf_settings_int(s, "CLIENT", "Width", 0) == 1280 && bf_settings_int(s, "Client", "HEIGHT", 0) == 768
	      && bf_settings_int(s, "Client/UIRectInfo", "ITEMWNDINFO.X", 0) == -5
	      && (uint32_t)bf_settings_int(s, "Client/UIRectInfo", "M_MINIMAPARGB", 0) == 0xFF102030u
	      && bf_settings_get_data(s, "Client", "GUIDDEVICE", back, sizeof(back), &length) == 1 && length == 16
	      && memcmp(back, guid, 16) == 0
	      && bf_settings_get_string(s, "Client/BookMark/Local \xc3\x96sterreich/Hero", "BOOKMARK0", bookmark, sizeof(bookmark), NULL) == 1
	      && strcmp(bookmark, "prt_fild08 \xc3\xa9t\xc3\xa9") == 0
	      && bf_settings_option(s, BF_OPT_ATTACK_WITHOUT_CTRL) == 1
	      && bf_settings_account_name(s, name, sizeof(name)) == 1 && strcmp(name, "bftest") == 0,
	      "settings: a fresh object reads every value back from the plist, names without regard to case");

	char tiny[4];
	Check(bf_settings_get_string(s, "Client/Option", "SKINNAME", tiny, sizeof(tiny), &length) == -1 && length == 7
	      && bf_settings_get_data(s, "Client", "GUIDDEVICE", NULL, 0, &length) == 1 && length == 16,
	      "settings: a short buffer reports the length needed");

	Check(bf_settings_remove(s, "Client", "WIDTH") == 1 && bf_settings_int(s, "Client", "WIDTH", 0) == 1024,
	      "settings: a removed value reads as its default again");
	Check(bf_settings_remove(s, "Client/BookMark", NULL) == 1 && !bf_settings_has_group(s, "Client/BookMark/Local \xc3\x96sterreich"),
	      "settings: a group is removed with everything in it (RegDeleteKey)");

	Check(bf_settings_set_int(s, "Client", "Option", 1) == 0, "settings: a value cannot replace a group");
	Check(bf_settings_set_string(s, "Client", "BAD", "\xc0\xaf") == 0, "settings: a string that is not UTF-8 is refused");
	Check(bf_settings_create_group(s, "Client/NameBaloon/srv") == 1 && bf_settings_has_group(s, "Client/NameBaloon/srv"),
	      "settings: create group (RegCreateKeyEx)");

	int count = 0;
	bool sawOwn = false, sawDefault = false;
	for (int i = 0;; ++i) {
		char n[64];
		bf_setting_type type = bf_settings_value_at(s, "Client/Option", i, n, sizeof(n));
		if (type == BF_SETTING_NONE)
			break;
		++count;
		if (strcmp(n, "SKINNAME") == 0 && type == BF_SETTING_STRING) sawOwn = true;
		if (strcmp(n, "isEffectOn") == 0 && type == BF_SETTING_INT) sawDefault = true;
	}
	Check(count == 6 && sawOwn && sawDefault, Format("settings: enumeration lists stored and default values (%d)", count).c_str());

	std::vector<bf_sys_thread *> threads;
	for (int i = 0; i < 4; ++i)
		threads.push_back(bf_sys_thread_create(SetMany, s, "bf-settings"));
	uint32_t failures = 0;
	for (bf_sys_thread *t : threads)
		failures += bf_sys_thread_join(t);
	Check(failures == 0 && bf_settings_save(s) == 1, "settings: 4 threads set values at once, then save");

	bf_settings_reset(s);
	Check(bf_settings_int(s, "Client", "WIDTH", 0) == 1024 && !bf_settings_has_group(s, "Client/UIRectInfo"),
	      "settings: reset leaves the defaults");
	bf_settings_close(s);

	// The shared object lives in Application Support under BF_HOME.
	bf_settings *shared = bf_settings_shared();
	std::string expected = std::string(bf_file_location(BF_LOCATION_SUPPORT)) + "/Settings.plist";
	Check(shared == bf_settings_open(NULL) && expected == bf_settings_path(shared),
	      Format("settings: the shared object is %s", bf_settings_path(shared)).c_str());
	bf_settings *profile = bf_settings_open("second");
	Check(profile != shared && profile == bf_settings_open("second")
	      && std::string(bf_settings_path(profile)).find("Settings-second.plist") != std::string::npos,
	      "settings: a profile gets its own object and file");

	// The game folder can come from the settings when the environment names none.
	const char *env = getenv("BF_CLIENT_DIR");
	std::string saved = env ? env : "";
	unsetenv("BF_CLIENT_DIR");
	bf_settings_set_string(shared, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER, fakeGame.c_str());
	bf_file_set_game_folder(NULL);
	std::string fromSettings = bf_file_location(BF_LOCATION_GAME);
	setenv("BF_CLIENT_DIR", "/nonexistent/from-env", 1);
	bf_file_set_game_folder(NULL);
	std::string fromEnv = bf_file_location(BF_LOCATION_GAME);
	if (env)
		setenv("BF_CLIENT_DIR", saved.c_str(), 1);
	else
		unsetenv("BF_CLIENT_DIR");
	bf_settings_remove(shared, BF_SETTINGS_PATHS, BF_SETTING_GAME_FOLDER);
	bf_file_set_game_folder(NULL);
	Check(fromSettings == fakeGame && fromEnv == "/nonexistent/from-env",
	      "settings: game folder from Paths/GameFolder, BF_CLIENT_DIR wins over it");
	bf_file_set_game_folder(fakeGame.c_str());
}

// ---- Keychain and shell ----------------------------------------------------------

void TestKeychain()
{
	const char *account = "bifrost-sys-test";
	if (!bf_sys_keychain_available()) {
		Skip("keychain: store, read, replace, short buffer, delete",
		     "the login keychain cannot be reached from this session (locked, or no window server)");
		return;
	}
	bf_sys_keychain_delete(account);
	char out[64];
	bool ok = bf_sys_keychain_store(account, "first secret") == 1
	       && bf_sys_keychain_read(account, out, sizeof(out)) == 1 && strcmp(out, "first secret") == 0
	       && bf_sys_keychain_store(account, "second") == 1
	       && bf_sys_keychain_read(account, out, sizeof(out)) == 1 && strcmp(out, "second") == 0
	       && bf_sys_keychain_read(account, out, 3) == -1
	       && bf_sys_keychain_delete(account) == 1
	       && bf_sys_keychain_read(account, out, sizeof(out)) == 0
	       && bf_sys_keychain_delete(account) == 0;
	Check(ok, "keychain: store, read, replace, short buffer, delete (service " BF_APP_NAME ")");
}

void TestShell()
{
	setenv("BF_NO_ALERTS", "1", 1);
	uint64_t start = bf_clock_us();
	bool logged = bf_sys_alerts_enabled() == 0
	           && bf_sys_alert("Test", "logged, not shown", BF_ALERT_INFO, BF_ALERT_BUTTONS_OK) == BF_ALERT_OK
	           && bf_sys_alert("Test", "logged, not shown", BF_ALERT_WARNING, BF_ALERT_BUTTONS_YES_NO) == BF_ALERT_YES
	           && bf_sys_alert("Test", "logged", BF_ALERT_CRITICAL, BF_ALERT_BUTTONS_OK_CANCEL) == BF_ALERT_OK;
	Check(logged && bf_clock_us() - start < 1000000, "shell: with BF_NO_ALERTS an alert is logged and answered with its first button");

	setenv("BF_NO_OPEN", "1", 1);
	Check(bf_sys_open_url("https://example.com/") == 1 && bf_sys_open_url("not a url") == 0
	      && bf_sys_open_path("/nonexistent/file") == 0 && bf_sys_open_path("/") == 1,
	      "shell: open URL and path validate and, with BF_NO_OPEN, only log");
	Check(strcmp(bf_sys_app_name(), BF_APP_NAME) == 0, "shell: application name from the build");
}

} // namespace

int main()
{
	// Everything the test writes stays below the build directory.
	std::string root = ExecutableFolder() + "/sys-test";
	RemoveTree(root);
	mkdir(root.c_str(), 0755);
	std::string home = root + "/home";
	std::string game = root + "/game";
	mkdir(home.c_str(), 0755);
	mkdir(game.c_str(), 0755);
	setenv("BF_HOME", home.c_str(), 1);

	const char *clientDir = getenv("BF_CLIENT_DIR");
	struct passwd *pw = getpwuid(getuid());
	std::string realGame = clientDir && *clientDir ? clientDir : std::string(pw ? pw->pw_dir : "") + "/Games/" BF_APP_NAME;

	printf("bifrost-sys-test: home %s\n", home.c_str());
	TestClock();
	TestThreads();
	TestLocations(home);
	TestLookup(game);
	TestRefusal(game);
	TestWritePaths(game);
	TestConcurrentLookups();
	TestSettings(home, game);
	TestKeychain();
	TestShell();
	TestRealGameFolder(realGame);

	printf("%d passed, %d failed, %d skipped\n", g_passes, g_failures, g_skips);
	return g_failures ? 1 : 0;
}
