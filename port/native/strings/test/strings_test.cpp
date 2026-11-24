// Test of the string lookup: exact-bytes match, fallback, counting, conversion.
// Runs without game data. Prints one line per check; exits non-zero on a failure.
#include "bf_coverage.h"
#include "bf_strings.h"
#include "bf_trace.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

static std::string ReadFile(const std::string &path) {
	std::string text;
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) return text;
	char buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
	fclose(f);
	return text;
}

struct CovEntry {
	std::string kind, text, drawn, hex, table, where;
	long long count;
};

static void EachCov(const bf_coverage_entry *e, void *user) {
	((std::vector<CovEntry> *)user)->push_back(CovEntry{e->kind, e->text, e->drawn ? e->drawn : "", e->hex,
	                                                     e->table ? e->table : "", e->where, e->count});
}

static const CovEntry *FindCov(const std::vector<CovEntry> &v, const char *kind) {
	for (const CovEntry &e : v)
		if (e.kind == kind) return &e;
	return nullptr;
}

// Stands for the client's drawing call: the coverage hook reads the stack from here.
__attribute__((noinline)) static int DrawSomething(int codepage, const void *text, int count) {
	return bf_coverage_drawn(codepage, text, count);
}

// The coverage report (M9.5): classification, recording, the JSON file.
static void CoverageTests(const std::string &dir) {
	bf_coverage_reset();
	bf_strings_clear();
	Check(bf_coverage_count() == 0, "coverage: empty after reset");

	// Classification of byte strings.
	const char *kGa = "\xb0\xa1";                      // 가, KS X 1001
	const char *kSentence = "\xc6\xc4\xc6\xbc \xb8\xde\xbd\xc3\xc1\xf6";   // 파티 메시지
	Check(bf_coverage_classify(1252, "Hello, world", -1) == BF_COVERAGE_ENGLISH, "classify: ASCII is English");
	Check(bf_coverage_classify(1252, "Ren\xe9" "e and Zo\xeb", -1) == BF_COVERAGE_ENGLISH,
	      "classify: Windows-1252 accents are English");
	Check(bf_coverage_classify(1252, "\x80 5, caf\xe9, na\xef" "ve", -1) == BF_COVERAGE_ENGLISH,
	      "classify: euro sign and accents are English");
	Check(bf_coverage_classify(949, kGa, -1) == BF_COVERAGE_KOREAN, "classify: one KS X 1001 syllable is Korean");
	Check(bf_coverage_classify(1252, kSentence, -1) == BF_COVERAGE_KOREAN,
	      "classify: Korean bytes are Korean in any code page");
	Check(bf_coverage_classify(1252, "\xc4pfel", -1) == BF_COVERAGE_ENGLISH,
	      "classify: one UHC pair with an ASCII trail (\xc4p) is not enough");
	Check(bf_coverage_classify(1252, "Lv 3 \xb0\xa1", 5) == BF_COVERAGE_ENGLISH, "classify: count limits the bytes");

	// UTF-16: Hangul drawn as Hangul, Korean bytes drawn through a Western font, foreign characters.
	const unsigned short kHangul16[] = {0xD30C, 0xD2F0, 0};                  // 파티
	const unsigned short kMojibake16[] = {0x00C6, 0x00C4, 0x00C6, 0x00BC, 0};  // C6 C4 C6 BC as 1252
	const unsigned short kAccents16[] = {'R', 'e', 'n', 0x00E9, 'e', 0x20AC, 0};
	const unsigned short kKanji16[] = {0x65E5, 0x672C, 0};
	Check(bf_coverage_classify(BF_COVERAGE_UTF16, kHangul16, -1) == BF_COVERAGE_KOREAN, "classify: UTF-16 Hangul");
	Check(bf_coverage_classify(BF_COVERAGE_UTF16, kMojibake16, -1) == BF_COVERAGE_KOREAN,
	      "classify: UTF-16 of CP949 bytes read as Windows-1252 is Korean");
	Check(bf_coverage_classify(BF_COVERAGE_UTF16, kAccents16, -1) == BF_COVERAGE_ENGLISH,
	      "classify: UTF-16 accents and the euro sign are English");
	Check(bf_coverage_classify(BF_COVERAGE_UTF16, kKanji16, -1) == BF_COVERAGE_FOREIGN,
	      "classify: UTF-16 CJK is foreign");

	// Recording: English is not recorded; Korean once per text, counted, with the drawn mojibake and a call site.
	DrawSomething(BF_COVERAGE_UTF16, kAccents16, -1);
	DrawSomething(1252, "plain", -1);
	Check(bf_coverage_count() == 0, "drawn: English leaves no entry");
	for (int i = 0; i < 3; ++i) DrawSomething(BF_COVERAGE_UTF16, kMojibake16, -1);
	DrawSomething(949, kGa, -1);
	DrawSomething(BF_COVERAGE_UTF16, kKanji16, -1);
	bf_coverage_placeholder("item", "item 31337: shown as ?31337");
	bf_coverage_placeholder("item", "item 31337: shown as ?31337");
	std::vector<CovEntry> v;
	int n = bf_coverage_each(EachCov, &v);
	Check(n == 4 && bf_coverage_count() == 4, "coverage: 4 entries (2 Korean, 1 foreign, 1 placeholder)");
	const CovEntry *moj = nullptr;
	for (const CovEntry &e : v)
		if (e.kind == "korean" && e.hex == "c6c4c6bc") moj = &e;
	Check(moj && moj->text == "\xED\x8C\x8C\xED\x8B\xB0" && moj->drawn == "\xC3\x86\xC3\x84\xC3\x86\xC2\xBC" && moj->count == 3,
	      "drawn mojibake: decoded as 파티, drawn as ÆÄÆ¼, counted 3 times");
	Check(moj && moj->where.find("DrawSomething") != std::string::npos, "drawn: the call site names the caller");
	const CovEntry *ph = FindCov(v, "placeholder");
	Check(ph && ph->table == "item" && ph->count == 2, "placeholder: once per entry, counted");
	Check(FindCov(v, "foreign") != nullptr, "foreign text recorded");

	// bf_tr fallbacks belong to the report, with the caller of bf_tr.
	WriteFile(dir + "/coverage_table.tsv", "b0a1\tGa\n");
	bf_strings_load((dir + "/coverage_table.tsv").c_str());
	bf_tr(kGa);
	bf_tr(kSentence);
	v.clear();
	bf_coverage_each(EachCov, &v);
	const CovEntry *un = FindCov(v, "untranslated");
	Check(un && un->text == "\xED\x8C\x8C\xED\x8B\xB0 \xEB\xA9\x94\xEC\x8B\x9C\xEC\xA7\x80" && un->count == 1,
	      "untranslated: the bf_tr fallback, decoded");
	Check(un && un->where.find("CoverageTests") != std::string::npos, "untranslated: the caller of bf_tr");

	// The JSON file.
	std::string path = dir + "/coverage_test.json";
	Check(bf_coverage_write(path.c_str()) == 0, "report written");
	std::string json = ReadFile(path);
	Check(json.find("\"entries\": 5") != std::string::npos && json.find("\"korean\": 2") != std::string::npos &&
	          json.find("\"untranslated\": 1") != std::string::npos && json.find("\"table\": \"item\"") != std::string::npos,
	      "report: counts per kind and the entries");
	bf_coverage_reset();
	Check(bf_coverage_count() == 0, "reset forgets everything");
	Check(bf_coverage_write(path.c_str()) == 0 && ReadFile(path).find("\"list\": []") != std::string::npos,
	      "an empty session writes an empty list");
	Check(bf_coverage_write((dir + "/no/such/folder/x.json").c_str()) == -1, "unwritable path: -1");
	bf_strings_clear();
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

	CoverageTests(dir);

	printf("%s\n", g_failed ? "FAILED" : "all passed");
	return g_failed ? 1 : 0;
}
