#include "hp_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <map>
#include <mutex>
#include <set>
#include <string>

// Errors and stub reports are printed even when HP_TRACE is unset.
unsigned hp_trace_mask = HPT_ERR | HPT_STUB;

namespace {

struct Category { const char *name; unsigned bit; };
const Category kCategories[] = {
	{ "err", HPT_ERR }, { "game", HPT_GAME }, { "net", HPT_NET }, { "pkt", HPT_PKT },
	{ "file", HPT_FILE }, { "res", HPT_RES }, { "gfx", HPT_GFX }, { "snd", HPT_SND },
	{ "input", HPT_INPUT }, { "ui", HPT_UI }, { "mode", HPT_MODE }, { "w32", HPT_W32 },
	{ "stub", HPT_STUB }, { "all", ~0u },
};

// The worker threads of the game (model loading, music fades) trace too.
std::recursive_mutex g_lock;
bool g_ready = false;
FILE *g_out = NULL;
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
		hp_trace_init();
}

} // namespace

void hp_trace_init(void)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);

	if (!g_ready)
		clock_gettime(CLOCK_MONOTONIC, &g_start);
	g_ready = true;

	unsigned mask = HPT_ERR | HPT_STUB;
	if (const char *spec = getenv("HP_TRACE")) {
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
				fprintf(stderr, "HP_TRACE: unknown category '%s'\n", word.c_str());
			begin = end + 1;
		}
	}
	hp_trace_mask = mask;

	if (g_out && g_out != stderr)
		fclose(g_out);
	g_out = stderr;
	if (const char *path = getenv("HP_TRACE_FILE")) {
		if (FILE *file = fopen(path, "w"))
			g_out = file;
		else
			fprintf(stderr, "HP_TRACE_FILE: cannot write %s\n", path);
	}

	if (g_packetLog) {
		fclose(g_packetLog);
		g_packetLog = NULL;
	}
	if (const char *path = getenv("HP_PKT_LOG")) {
		g_packetLog = fopen(path, "a");
		if (!g_packetLog)
			fprintf(stderr, "HP_PKT_LOG: cannot write %s\n", path);
	}

	const char *onAssert = getenv("HP_ASSERT");
	g_abortOnAssert = onAssert && !strcmp(onAssert, "abort");
}

void hp_tracev(unsigned category, const char *format, va_list args)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	if (!(hp_trace_mask & category))
		return;

	fprintf(g_out, "[%8lld %-5s] ", Milliseconds(), CategoryName(category));
	vfprintf(g_out, format, args);
	size_t length = strlen(format);
	if (!length || format[length - 1] != '\n')
		fputc('\n', g_out);
	fflush(g_out);
}

void hp_tracef(unsigned category, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	hp_tracev(category, format, args);
	va_end(args);
}

void hp_trace_hex(unsigned category, const char *label, const void *data, int size)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	if (!(hp_trace_mask & category))
		return;

	const unsigned char *bytes = static_cast<const unsigned char *>(data);
	int shown = size < 256 ? size : 256;
	std::string line;
	char cell[4];
	for (int i = 0; i < shown; ++i) {
		snprintf(cell, sizeof(cell), "%02x ", bytes[i]);
		line += cell;
	}
	hp_tracef(category, "%s (%d bytes)%s %s", label, size, shown < size ? " first 256:" : ":", line.c_str());
}

void hp_trace_set_packet_namer(const char *(*namer)(int header))
{
	g_packetNamer = namer;
}

void hp_trace_packet(char direction, const void *data, int size)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	if (size < 2 || (!(hp_trace_mask & HPT_PKT) && !g_packetLog))
		return;

	const unsigned char *bytes = static_cast<const unsigned char *>(data);
	int header = bytes[0] | (bytes[1] << 8);
	const char *name = g_packetNamer ? g_packetNamer(header) : NULL;
	if (!name)
		name = "?";

	HP_TRACE(HPT_PKT, "%c 0x%04x %-32s %d bytes", direction, header, name, size);

	if (g_packetLog) {
		fprintf(g_packetLog, "%lld %c 0x%04x %d %s ", Milliseconds(), direction, header, size, name);
		for (int i = 0; i < size; ++i)
			fprintf(g_packetLog, "%02x", bytes[i]);
		fputc('\n', g_packetLog);
		fflush(g_packetLog);
	}
}

void hp_count(const char *name, long long amount)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	g_counters[name] += amount;
}

void hp_trace_dump_counters(void)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	EnsureReady();
	for (std::map<std::string, long long>::const_iterator it = g_counters.begin(); it != g_counters.end(); ++it)
		fprintf(g_out, "[counter] %-40s %lld\n", it->first.c_str(), it->second);
	fflush(g_out);
}

void hp_stub(const char *function, const char *file, int line)
{
	std::lock_guard<std::recursive_mutex> hold(g_lock);
	char site[512];
	snprintf(site, sizeof(site), "%s:%d", file, line);
	hp_count((std::string("stub.") + function).c_str(), 1);
	if (g_stubsSeen.insert(site).second)
		hp_tracef(HPT_STUB, "unimplemented: %s (%s)", function, site);
}

void hp_assert_failed(const char *expression, const char *message, const char *file, int line)
{
	hp_count("assert.failed", 1);
	hp_tracef(HPT_ERR, "assertion failed: %s%s%s (%s:%d)", expression,
		message ? " - " : "", message ? message : "", file, line);
	if (g_abortOnAssert)
		__builtin_debugtrap();
}
