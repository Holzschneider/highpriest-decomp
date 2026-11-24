// Test of the string lookup: exact-bytes match, fallback, counting, conversion.
// Runs without game data. Prints one line per check; exits non-zero on a failure.
#include "bf_strings.h"
#include "bf_trace.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static int g_failed = 0;

static void Check(bool ok, const char *what) {
	printf("%s: %s\n", ok ? "ok" : "FAIL", what);
	if (!ok) ++g_failed;
}

static std::string Hex(const char *s) {
	static const char *d = "0123456789abcdef";
	std::string h;
	for (; *s; ++s) {
		h.push_back(d[(unsigned char)*s >> 4]);
		h.push_back(d[(unsigned char)*s & 15]);
	}
	return h;
}

static void WriteFile(const std::string &path, const std::string &text) {
	FILE *f = fopen(path.c_str(), "wb");
	fwrite(text.data(), 1, text.size(), f);
	fclose(f);
}

struct Collect {
	int n = 0;
	long long total = 0;
	std::string first;
};

static void Each(const char *original, long long count, void *user) {
	Collect *c = (Collect *)user;
	if (c->n++ == 0) c->first = original;
	c->total += count;
}

int main() {
	bf_trace_init();
	const std::string dir = BF_STRINGS_OUTPUT_DIR;

	// CP949 originals as they sit in the source: "\xb9\xe8\xb0\xed\xc7\xc4" and a printf format.
	const char *kHungry = "\xb9\xe8\xb0\xed\xc7\xc4";
	const char *kTotal = "Total : %d \xc7\xd1\xc4\xda\xc0\xce";
	const char *kEmpty = "\xba\xb8\xc5\xeb";
	const char *kNl = "\xb0\xa1\n\xb3\xaa";
	const char *kEuro = "\xb4\xd9";

	std::string table = "# comment line\n\n";
	table += Hex(kHungry) + "\tHungry\t\xEB\xB0\xB0\xEA\xB3\xA0\xED\x8C\x8C\n";
	table += Hex(kTotal) + "\tTotal: %d H-Coins\tkorean ignored\r\n";
	table += Hex(kEmpty) + "\t\tnot translated yet\n";
	table += Hex(kNl) + "\tline one\\nline two\\ttab \\\\ done\n";
	table += Hex(kEuro) + "\tCaf\xC3\xA9 \xE2\x82\xAC" "5 \xE2\x80\x9Cquoted\xE2\x80\x9D\n";
	table += "zz\tbad hex\n";                         // skipped
	table += Hex("\xb0\xa1\xb0\xa2") + "\t\xE7\x8C\xAB\n";   // text outside Windows-1252: skipped as a translation
	WriteFile(dir + "/strings_test_table.tsv", table);

	// Before any table: a lookup falls back to the original, with the table missing.
	bf_strings_clear();
	setenv("BF_STRINGS_FILE", (dir + "/does_not_exist.tsv").c_str(), 1);
	Check(bf_tr(kHungry) == kHungry, "no table: original returned (same pointer)");
	bf_strings_stats st;
	bf_strings_get_stats(&st);
	Check(st.lookups == 1 && st.fallbacks == 1 && st.hits == 0 && st.entries == 0, "no table: counted as a fallback");
	Check(bf_strings_load((dir + "/does_not_exist.tsv").c_str()) == -1, "load of a missing file returns -1");
	unsetenv("BF_STRINGS_FILE");

	int n = bf_strings_load((dir + "/strings_test_table.tsv").c_str());
	Check(n == 6, "load: 6 originals listed (bad hex line skipped)");
	bf_strings_get_stats(&st);
	Check(st.entries == 6 && st.translated == 4, "load: 4 of them translated (empty and non-1252 entries are not)");

	bf_strings_reset_counts();
	const char *r = bf_tr(kHungry);
	Check(strcmp(r, "Hungry") == 0 && r != kHungry, "exact bytes: translation returned");
	Check(strcmp(bf_tr(kTotal), "Total: %d H-Coins") == 0, "format string: translation returned, CRLF line handled");
	Check(strcmp(bf_tr(kNl), "line one\nline two\ttab \\ done") == 0, "escapes \\n \\t \\\\ undone");
	Check(strcmp(bf_tr(kEuro), "Caf\xE9 \x80""5 \x93quoted\x94") == 0, "UTF-8 converted to Windows-1252 bytes");

	// A string that merely starts with, ends with or contains an original does not match.
	std::string longer = std::string(kHungry) + " ";
	Check(bf_tr(longer.c_str()) == longer.c_str(), "original plus one byte: no match, original returned");
	std::string shorter(kHungry, strlen(kHungry) - 1);
	Check(bf_tr(shorter.c_str()) == shorter.c_str(), "original minus one byte: no match");
	Check(bf_tr(kEmpty) == kEmpty, "entry without English: original returned");
	Check(bf_tr("\xb0\xa1\xb0\xa2") != nullptr && strcmp(bf_tr("\xb0\xa1\xb0\xa2"), "\xb0\xa1\xb0\xa2") == 0,
	      "entry whose English is outside Windows-1252: original returned");
	Check(bf_tr("\xb0\xa1\xb0\xa2\xb0\xa3") != nullptr, "unknown original: returned, not NULL");

	// Counting. Lookups so far: 4 hits; fallbacks: longer, shorter, kEmpty, 1252-bad twice, unknown = 6.
	bf_strings_get_stats(&st);
	Check(st.hits == 4, "count: 4 hits");
	Check(st.fallbacks == 6 && st.lookups == 10, "count: 6 fallbacks of 10 lookups");
	Check(st.distinct_fallbacks == 5, "count: 5 distinct originals fell back (the 1252 one twice)");
	Collect c;
	Check(bf_strings_each_fallback(Each, &c) == 5 && c.n == 5 && c.total == 6, "each_fallback visits the 5 originals with their counts");

	// Strings with no high byte are not keys: returned as they are, counted for nothing.
	long long before = st.lookups;
	const char *ascii = "Total: %d";
	Check(bf_tr(ascii) == ascii, "ASCII string passes through unchanged");
	bf_strings_get_stats(&st);
	Check(st.lookups == before, "ASCII string is not counted");
	Check(bf_tr(nullptr) == nullptr, "NULL gives NULL");

	bf_strings_reset_counts();
	bf_strings_get_stats(&st);
	Check(st.lookups == 0 && st.hits == 0 && st.fallbacks == 0 && st.distinct_fallbacks == 0 && st.entries == 6,
	      "reset_counts zeroes counters, keeps the table");

	// Reload replaces the table.
	WriteFile(dir + "/strings_test_table2.tsv", Hex(kHungry) + "\tStarving\n");
	Check(bf_strings_load((dir + "/strings_test_table2.tsv").c_str()) == 1, "reload: 1 original");
	Check(strcmp(bf_tr(kHungry), "Starving") == 0 && bf_tr(kTotal) == kTotal, "reload: the new table replaces the old");

	// The committed table (all displayed originals, English still empty at M9.2): loads, every lookup
	// falls back to the identical bytes and is counted once.
	FILE *probe = fopen(BF_STRINGS_TABLE, "rb");
	if (!probe) {
		printf("skipped: committed translation.tsv not found\n");
	} else {
		fclose(probe);
		bf_strings_clear();
		bf_strings_reset_counts();
		int entries = bf_strings_load(BF_STRINGS_TABLE);
		bf_strings_get_stats(&st);
		Check(entries > 50, "committed table loads (more than 50 originals)");
		int translated = st.translated;
		// Walk the file again to find each key and look it up.
		FILE *f = fopen(BF_STRINGS_TABLE, "rb");
		char line[4096];
		int keys = 0, bad = 0;
		while (fgets(line, sizeof line, f)) {
			if (line[0] == '#' || line[0] == '\n') continue;
			char *tab = strchr(line, '\t');
			if (!tab) continue;
			std::string orig;
			for (char *p = line; p < tab; p += 2) {
				auto hv = [](char ch) { return ch <= '9' ? ch - '0' : ch - 'a' + 10; };
				orig.push_back((char)(hv(p[0]) * 16 + hv(p[1])));
			}
			++keys;
			const char *got = bf_tr(orig.c_str());
			if (translated == 0 && strcmp(got, orig.c_str()) != 0) ++bad;
		}
		fclose(f);
		bf_strings_get_stats(&st);
		Check(keys == entries, "committed table: every listed original was looked up");
		Check(translated > 0 || (bad == 0 && st.fallbacks == keys && st.distinct_fallbacks == keys),
		      "committed table: untranslated originals fall back to identical bytes, each counted");
	}

	printf("%s\n", g_failed ? "FAILED" : "all passed");
	return g_failed ? 1 : 0;
}
