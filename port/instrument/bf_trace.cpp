#include "bf_trace.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

// Errors and stub reports are printed even when BF_TRACE is unset.
unsigned bf_trace_mask = BFT_ERR | BFT_STUB;

namespace {

struct Category { const char *name; unsigned bit; };
const Category kCategories[] = {
	{ "err", BFT_ERR }, { "game", BFT_GAME }, { "net", BFT_NET }, { "pkt", BFT_PKT },
	{ "file", BFT_FILE }, { "res", BFT_RES }, { "gfx", BFT_GFX }, { "snd", BFT_SND },
	{ "input", BFT_INPUT }, { "ui", BFT_UI }, { "mode", BFT_MODE }, { "w32", BFT_W32 },
	{ "stub", BFT_STUB }, { "all", ~0u },
};

// The worker threads of the game (model loading, music fades) trace too.
std::recursive_mutex g_lock;
bool g_ready = false;
FILE *g_out = NULL;
unsigned g_outMask = BFT_ERR | BFT_STUB;   // the stream's categories (BF_TRACE)

// The rolling file (bf_trace_open_rolling).
struct Rolling {
	FILE *file = NULL;
	unsigned mask = 0;
	std::string folder, base, path;   // base: trace-<date>-<pid>
	int part = 1, keep = 10;
	long long bytes = 0, max = 0;
};
Rolling g_roll;
FILE *g_packetLog = NULL;
bool g_abortOnAssert = false;
const char *(*g_packetNamer)(int) = NULL;
std::map<std::string, long long> g_counters;
std::set<std::string> g_stubsSeen;
struct timespec g_start;

const char *CategoryName(unsigned category)
{
	for (size_t i = 0; i < sizeof(kCategories) / sizeof(kCategories[0]); ++i)
		if (kCategories[i].bit == category)
			return kCategories[i].name;
	return "?";
}

long long Milliseconds()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec - g_start.tv_sec) * 1000LL + (now.tv_nsec - g_start.tv_nsec) / 1000000;
}

void EnsureReady()
{
	if (!g_ready)
		bf_trace_init();
}

// Leaves the newest g_roll.keep files named trace-*.txt in the folder.
void PruneRolling()
{
	DIR *dir = opendir(g_roll.folder.c_str());
	if (!dir)
		return;
	struct Entry { struct timespec mtime; std::string name; };
	std::vector<Entry> files;
	while (struct dirent *e = readdir(dir)) {
		size_t n = strlen(e->d_name);
		if (n < 10 || strncmp(e->d_name, "trace-", 6) != 0 || strcmp(e->d_name + n - 4, ".txt") != 0)
			continue;
		struct stat st;
		std::string path = g_roll.folder + "/" + e->d_name;
		if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode))
			files.push_back(Entry{st.st_mtimespec, e->d_name});
	}
	closedir(dir);
	std::sort(files.begin(), files.end(), [](const Entry &a, const Entry &b) {   // newest first
		if (a.mtime.tv_sec != b.mtime.tv_sec)
			return a.mtime.tv_sec > b.mtime.tv_sec;
		if (a.mtime.tv_nsec != b.mtime.tv_nsec)
			return a.mtime.tv_nsec > b.mtime.tv_nsec;
		return a.name > b.name;
	});
	for (size_t i = g_roll.keep > 0 ? (size_t)g_roll.keep : 1; i < files.size(); ++i)
		if (files[i].name != g_roll.path.substr(g_roll.folder.size() + 1))
			unlink((g_roll.folder + "/" + files[i].name).c_str());
}

// Opens part g_roll.part of the rolling file; false (on the stream) if it cannot.
bool OpenRollingPart()
{
	char name[64];
	if (g_roll.part > 1)
		snprintf(name, sizeof name, "-%d", g_roll.part);
	else
		name[0] = 0;
	std::string path = g_roll.folder + "/" + g_roll.base + name + ".txt";
	FILE *file = fopen(path.c_str(), "w");
	if (!file) {
		fprintf(g_out ? g_out : stderr, "[%8lld err  ] trace: cannot write the rolling trace %s\n", Milliseconds(),
		        path.c_str());
		return false;
	}
	g_roll.file = file;
	g_roll.path = path;
	g_roll.bytes = 0;
	PruneRolling();
	return true;
}

// After a line: the next part once the file is full.
void RollIfFull()
{
	if (!g_roll.file || g_roll.max <= 0 || g_roll.bytes < g_roll.max)
		return;
	fprintf(g_roll.file, "[%8lld mode ] trace: %lld bytes, continued in part %d\n", Milliseconds(), g_roll.bytes,
	        g_roll.part + 1);
	fclose(g_roll.file);
	g_roll.file = NULL;
	++g_roll.part;
	OpenRollingPart();
}

} // namespace

void bf_trace_init(void)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);

	if (!g_ready)
		clock_gettime(CLOCK_MONOTONIC, &g_start);
	g_ready = true;

	unsigned mask = BFT_ERR | BFT_STUB;
	if (const char *spec = getenv("BF_TRACE")) {
		std::string list(spec);
		size_t begin = 0;
		while (begin <= list.size()) {
			size_t end = list.find(',', begin);
			if (end == std::string::npos)
				end = list.size();
			std::string word = list.substr(begin, end - begin);
			bool known = word.empty();
			for (size_t i = 0; i < sizeof(kCategories) / sizeof(kCategories[0]); ++i)
				if (word == kCategories[i].name) {
					mask |= kCategories[i].bit;
					known = true;
				}
			if (!known)
				fprintf(stderr, "BF_TRACE: unknown category '%s'\n", word.c_str());
			begin = end + 1;
		}
	}
	g_outMask = mask;
	bf_trace_mask = mask | (g_roll.file ? g_roll.mask : 0);

	if (g_out && g_out != stderr)
		fclose(g_out);
	g_out = stderr;
	if (const char *path = getenv("BF_TRACE_FILE")) {
		if (FILE *file = fopen(path, "w"))
			g_out = file;
		else
			fprintf(stderr, "BF_TRACE_FILE: cannot write %s\n", path);
	}

	if (g_packetLog) {
		fclose(g_packetLog);
		g_packetLog = NULL;
	}
	if (const char *path = getenv("BF_PKT_LOG")) {
		g_packetLog = fopen(path, "a");
		if (!g_packetLog)
			fprintf(stderr, "BF_PKT_LOG: cannot write %s\n", path);
	}

	const char *onAssert = getenv("BF_ASSERT");
	g_abortOnAssert = onAssert && !strcmp(onAssert, "abort");
}

void bf_tracev(unsigned category, const char *format, va_list args)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	if (!(bf_trace_mask & category))
		return;

	long long ms = Milliseconds();
	size_t length = strlen(format);
	bool newline = !length || format[length - 1] != '\n';
	if (g_outMask & category) {
		va_list copy;
		va_copy(copy, args);
		fprintf(g_out, "[%8lld %-5s] ", ms, CategoryName(category));
		vfprintf(g_out, format, copy);
		va_end(copy);
		if (newline)
			fputc('\n', g_out);
		fflush(g_out);
	}
	if (g_roll.file && (g_roll.mask & category)) {
		va_list copy;
		va_copy(copy, args);
		int a = fprintf(g_roll.file, "[%8lld %-5s] ", ms, CategoryName(category));
		int b = vfprintf(g_roll.file, format, copy);
		va_end(copy);
		if (newline)
			fputc('\n', g_roll.file);
		fflush(g_roll.file);
		g_roll.bytes += (a > 0 ? a : 0) + (b > 0 ? b : 0) + (newline ? 1 : 0);
		RollIfFull();
	}
}

void bf_tracef(unsigned category, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	bf_tracev(category, format, args);
	va_end(args);
}

void bf_trace_hex(unsigned category, const char *label, const void *data, int size)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	if (!(bf_trace_mask & category))
		return;

	const unsigned char *bytes = static_cast<const unsigned char *>(data);
	int shown = size < 256 ? size : 256;
	std::string line;
	char cell[4];
	for (int i = 0; i < shown; ++i) {
		snprintf(cell, sizeof(cell), "%02x ", bytes[i]);
		line += cell;
	}
	bf_tracef(category, "%s (%d bytes)%s %s", label, size, shown < size ? " first 256:" : ":", line.c_str());
}

void bf_trace_set_packet_namer(const char *(*namer)(int header))
{
	g_packetNamer = namer;
}

void bf_trace_packet(char direction, const void *data, int size)
{
	bf_trace_packet_ex(direction, data, size, NULL, NULL);
}

void bf_trace_packet_ex(char direction, const void *data, int size, const char *name, const char *connection)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	if (size < 2 || (!(bf_trace_mask & BFT_PKT) && !g_packetLog))
		return;

	const unsigned char *bytes = static_cast<const unsigned char *>(data);
	int header = bytes[0] | (bytes[1] << 8);
	if (!name)
		name = g_packetNamer ? g_packetNamer(header) : NULL;
	if (!name)
		name = "?";
	if (connection && !*connection)
		connection = NULL;

	BF_TRACE(BFT_PKT, "%c 0x%04x %-32s %d bytes%s%s", direction, header, name, size, connection ? " on " : "",
	         connection ? connection : "");

	if (g_packetLog) {
		fprintf(g_packetLog, "%lld %c 0x%04x %d %s ", Milliseconds(), direction, header, size, name);
		for (int i = 0; i < size; ++i)
			fprintf(g_packetLog, "%02x", bytes[i]);
		if (connection)
			fprintf(g_packetLog, " %s", connection);
		fputc('\n', g_packetLog);
		fflush(g_packetLog);
	}
}

void bf_count(const char *name, long long amount)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	g_counters[name] += amount;
}

namespace {

void DumpCounters(bool stream)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	for (std::map<std::string, long long>::const_iterator it = g_counters.begin(); it != g_counters.end(); ++it) {
		if (stream)
			fprintf(g_out, "[counter] %-40s %lld\n", it->first.c_str(), it->second);
		if (g_roll.file) {
			int n = fprintf(g_roll.file, "[counter] %-40s %lld\n", it->first.c_str(), it->second);
			g_roll.bytes += n > 0 ? n : 0;
		}
	}
	if (stream)
		fflush(g_out);
	if (g_roll.file) {
		fflush(g_roll.file);
		RollIfFull();
	}
}

} // namespace

void bf_trace_dump_counters(void)
{
	DumpCounters(true);
}

void bf_trace_dump_counters_at_exit(void)
{
	DumpCounters((g_outMask & ~(unsigned)(BFT_ERR | BFT_STUB)) != 0);
}

const char *bf_trace_open_rolling(const char *folder, unsigned mask, int keep, long long max_bytes)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	if (g_roll.file) {
		fclose(g_roll.file);
		g_roll.file = NULL;
	}
	time_t now = time(NULL);
	struct tm local;
	localtime_r(&now, &local);
	char base[64];
	strftime(base, sizeof base, "trace-%Y%m%d-%H%M%S", &local);
	snprintf(base + strlen(base), sizeof base - strlen(base), "-%d", (int)getpid());
	g_roll.folder = folder ? folder : "";
	g_roll.base = base;
	g_roll.mask = mask;
	g_roll.keep = keep;
	g_roll.max = max_bytes;
	g_roll.part = 1;
	if (g_roll.folder.empty() || !OpenRollingPart()) {
		bf_trace_mask = g_outMask;
		return NULL;
	}
	bf_trace_mask = g_outMask | g_roll.mask;
	return g_roll.path.c_str();
}

void bf_stub(const char *function, const char *file, int line)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	char site[512];
	snprintf(site, sizeof(site), "%s:%d", file, line);
	bf_count((std::string("stub.") + function).c_str(), 1);
	if (g_stubsSeen.insert(site).second)
		bf_tracef(BFT_STUB, "unimplemented: %s (%s)", function, site);
}

void bf_assert_failed(const char *expression, const char *message, const char *file, int line)
{
	bf_count("assert.failed", 1);
	bf_tracef(BFT_ERR, "assertion failed: %s%s%s (%s:%d)", expression,
		message ? " - " : "", message ? message : "", file, line);
	if (g_abortOnAssert)
		__builtin_debugtrap();
}
