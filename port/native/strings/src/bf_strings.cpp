#include "bf_strings.h"
#include "bf_trace.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {

struct Entry {
	std::string english;
	bool translated = false;
};

std::mutex g_lock;
std::unordered_map<std::string, Entry> g_table;
std::map<std::string, long long> g_fallbacks;   // ordered, so reports are stable
long long g_lookups = 0, g_hits = 0, g_fallbackCount = 0;
int g_translated = 0;
bool g_loadTried = false;

bool HasHighByte(const char *s) {
	for (; *s; ++s)
		if ((unsigned char)*s >= 0x80) return true;
	return false;
}

int HexVal(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

bool FromHex(const std::string &hex, std::string *out) {
	if (hex.empty() || (hex.size() & 1)) return false;
	out->clear();
	for (size_t i = 0; i < hex.size(); i += 2) {
		int a = HexVal(hex[i]), b = HexVal(hex[i + 1]);
		if (a < 0 || b < 0) return false;
		out->push_back((char)(a * 16 + b));
	}
	return true;
}

// Windows-1252 bytes 0x80..0x9f as Unicode code points (0 = undefined in 1252).
const unsigned short kCp1252High[32] = {
	0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
	0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178};

bool ToCp1252(unsigned cp, char *out) {
	if (cp < 0x80 || (cp >= 0xA0 && cp <= 0xFF)) { *out = (char)cp; return true; }
	for (int i = 0; i < 32; ++i)
		if (kCp1252High[i] && kCp1252High[i] == cp) { *out = (char)(0x80 + i); return true; }
	return false;
}

// The English column: escapes undone, UTF-8 turned into Windows-1252 bytes.
bool Convert(const std::string &in, std::string *out) {
	out->clear();
	size_t i = 0;
	while (i < in.size()) {
		unsigned char c = (unsigned char)in[i];
		unsigned cp;
		if (c == '\\' && i + 1 < in.size()) {
			char e = in[i + 1];
			cp = e == 'n' ? '\n' : e == 't' ? '\t' : e == 'r' ? '\r' : e == '\\' ? '\\' : 0;
			if (!cp) { cp = '\\'; i += 1; } else i += 2;
		} else if (c < 0x80) {
			cp = c; i += 1;
		} else {
			int n = (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : -1;
			if (n < 0 || i + n >= in.size()) return false;
			cp = c & (0x3F >> n);
			for (int k = 1; k <= n; ++k) {
				unsigned char d = (unsigned char)in[i + k];
				if ((d & 0xC0) != 0x80) return false;
				cp = (cp << 6) | (d & 0x3F);
			}
			i += 1 + n;
		}
		char b;
		if (!ToCp1252(cp, &b)) return false;
		out->push_back(b);
	}
	return true;
}

int LoadLocked(const char *path) {
	g_table.clear();
	g_translated = 0;
	g_loadTried = true;
	FILE *f = fopen(path, "rb");
	if (!f) {
		BF_TRACE(BFT_ERR, "strings: cannot read %s", path);
		return -1;
	}
	std::string text;
	char buf[8192];
	size_t n;
	while ((n = fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
	fclose(f);
	if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3);

	int lineNo = 0;
	size_t pos = 0;
	while (pos <= text.size()) {
		size_t end = text.find('\n', pos);
		if (end == std::string::npos) end = text.size();
		std::string line = text.substr(pos, end - pos);
		pos = end + 1;
		++lineNo;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty() || line[0] == '#') continue;
		size_t t1 = line.find('\t');
		std::string hex = line.substr(0, t1), english;
		if (t1 != std::string::npos) {
			size_t t2 = line.find('\t', t1 + 1);
			english = line.substr(t1 + 1, t2 == std::string::npos ? std::string::npos : t2 - t1 - 1);
		}
		std::string key;
		if (!FromHex(hex, &key)) {
			BF_TRACE(BFT_ERR, "strings: %s:%d: bad hex key", path, lineNo);
			continue;
		}
		Entry e;
		if (!english.empty()) {
			if (Convert(english, &e.english)) {
				e.translated = true;
			} else {
				BF_TRACE(BFT_ERR, "strings: %s:%d: English text is not Windows-1252, entry ignored", path, lineNo);
			}
		}
		auto it = g_table.find(key);
		if (it != g_table.end() && it->second.translated) --g_translated;
		g_table[key] = e;
		if (e.translated) ++g_translated;
	}
	BF_TRACE(BFT_UI, "strings: %s: %d originals, %d translated", path, (int)g_table.size(), g_translated);
	return (int)g_table.size();
}

void AutoLoadLocked() {
	if (g_loadTried) return;
	std::string path;
	const char *file = getenv("BF_STRINGS_FILE");
	if (file && *file) {
		path = file;
	} else {
		const char *dir = getenv("BF_CLIENT_DIR");
		path = (dir && *dir) ? std::string(dir) + "/data/bf_strings.tsv" : "data/bf_strings.tsv";
	}
	LoadLocked(path.c_str());
}

std::string HexOf(const std::string &s, size_t max) {
	static const char *d = "0123456789abcdef";
	std::string h;
	for (size_t i = 0; i < s.size() && i < max; ++i) {
		h.push_back(d[(unsigned char)s[i] >> 4]);
		h.push_back(d[(unsigned char)s[i] & 15]);
	}
	if (s.size() > max) h += "...";
	return h;
}

}  // namespace

extern "C" {

int bf_strings_load(const char *path) {
	std::lock_guard<std::mutex> g(g_lock);
	return LoadLocked(path);
}

void bf_strings_clear(void) {
	std::lock_guard<std::mutex> g(g_lock);
	g_table.clear();
	g_translated = 0;
	g_loadTried = false;
}

const char *bf_tr(const char *original) {
	if (!original) return nullptr;
	if (!HasHighByte(original)) return original;
	std::lock_guard<std::mutex> g(g_lock);
	AutoLoadLocked();
	++g_lookups;
	std::string key(original);
	auto it = g_table.find(key);
	if (it != g_table.end() && it->second.translated) {
		++g_hits;
		return it->second.english.c_str();
	}
	++g_fallbackCount;
	if (++g_fallbacks[key] == 1)
		BF_TRACE(BFT_UI, "strings: no translation (%s) for %s", it == g_table.end() ? "not in table" : "not translated yet",
		         HexOf(key, 48).c_str());
	bf_count("strings.fallback", 1);
	return original;
}

void bf_strings_get_stats(bf_strings_stats *out) {
	std::lock_guard<std::mutex> g(g_lock);
	out->lookups = g_lookups;
	out->hits = g_hits;
	out->fallbacks = g_fallbackCount;
	out->entries = (int)g_table.size();
	out->translated = g_translated;
	out->distinct_fallbacks = (int)g_fallbacks.size();
}

int bf_strings_each_fallback(void (*fn)(const char *, long long, void *), void *user) {
	std::lock_guard<std::mutex> g(g_lock);
	for (const auto &kv : g_fallbacks) fn(kv.first.c_str(), kv.second, user);
	return (int)g_fallbacks.size();
}

void bf_strings_reset_counts(void) {
	std::lock_guard<std::mutex> g(g_lock);
	g_lookups = g_hits = g_fallbackCount = 0;
	g_fallbacks.clear();
}

}  // extern "C"
