#include "bf_coverage.h"
#include "bf_coverage_internal.h"
#include "bf_strings.h"
#include "bf_trace.h"

#include <CoreFoundation/CoreFoundation.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

namespace {

struct Record {
	std::string text, drawn, hex, table, where;
	long long count = 0;
};

// key: (kind, the text as drawn or the placeholder's table + what)
std::mutex g_lock;
std::map<std::pair<std::string, std::string>, Record> g_records;

// ---- classification -------------------------------------------------------

bool IsTrail(unsigned char d) {
	return (d >= 0x41 && d <= 0x5A) || (d >= 0x61 && d <= 0x7A) || (d >= 0x81 && d <= 0xFE);
}

// Every byte >= 0x80 in a CP949 pair, and the pairs hold Hangul: one KS X 1001
// syllable (lead B0..C8) or compatibility jamo (lead A4), or two syllables of
// the UHC extension (lead 81..C6 with a trail below A1), whose trail bytes can
// be ASCII letters and so turn up in Western text more easily.
bool LooksKorean(const unsigned char *s, size_t n) {
	int strong = 0, weak = 0;
	for (size_t i = 0; i < n;) {
		unsigned char c = s[i];
		if (c < 0x80) { ++i; continue; }
		if (c == 0x80 || c == 0xFF || i + 1 >= n || !IsTrail(s[i + 1])) return false;
		unsigned char d = s[i + 1];
		if (c >= 0xA1 && d >= 0xA1) {
			if ((c >= 0xB0 && c <= 0xC8) || c == 0xA4) ++strong;
		} else if (c <= 0xC6) {
			++weak;
		}
		i += 2;
	}
	return strong >= 1 || weak >= 2;
}

bool IsHangul(unsigned u) {
	return (u >= 0xAC00 && u <= 0xD7A3) || (u >= 0x1100 && u <= 0x11FF) || (u >= 0x3130 && u <= 0x318F);
}

// Windows-1252 bytes 0x80..0x9f as Unicode (0: undefined, which Windows maps to
// the C1 control of the same value).
const unsigned short kCp1252High[32] = {
	0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
	0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178};

bool To1252(unsigned u, unsigned char *out) {
	if (u < 0x80 || (u >= 0xA0 && u <= 0xFF)) { *out = (unsigned char)u; return true; }
	for (int i = 0; i < 32; ++i)
		if ((kCp1252High[i] && kCp1252High[i] == u) || (!kCp1252High[i] && u == 0x80u + i)) {
			*out = (unsigned char)(0x80 + i);
			return true;
		}
	return false;
}

size_t Units(const unsigned short *t, int count) {
	if (count >= 0) return (size_t)count;
	size_t n = 0;
	while (t[n]) ++n;
	return n;
}

size_t Bytes(const char *t, int count) {
	return count >= 0 ? (size_t)count : strlen(t);
}

// UTF-16 -> the Windows-1252 bytes it was converted from; false when a unit is
// not a Windows-1252 character.
bool Back1252(const unsigned short *t, size_t n, std::string *out) {
	out->clear();
	for (size_t i = 0; i < n; ++i) {
		unsigned char b;
		if (!To1252(t[i], &b)) return false;
		out->push_back((char)b);
	}
	return true;
}

int ClassifyBytes(const unsigned char *s, size_t n) {
	size_t i = 0;
	while (i < n && s[i] < 0x80) ++i;
	if (i == n) return BF_COVERAGE_ENGLISH;
	return LooksKorean(s, n) ? BF_COVERAGE_KOREAN : BF_COVERAGE_ENGLISH;
}

int ClassifyUnits(const unsigned short *t, size_t n, std::string *bytes) {
	bool high = false, hangul = false, outside = false;
	for (size_t i = 0; i < n; ++i) {
		unsigned u = t[i];
		if (u < 0x80) continue;
		high = true;
		if (IsHangul(u)) hangul = true;
		unsigned char b;
		if (!To1252(u, &b)) outside = true;
	}
	if (!high) return BF_COVERAGE_ENGLISH;
	if (hangul) return BF_COVERAGE_KOREAN;
	if (outside) return BF_COVERAGE_FOREIGN;
	std::string tmp;
	if (!bytes) bytes = &tmp;
	Back1252(t, n, bytes);
	return ClassifyBytes((const unsigned char *)bytes->data(), bytes->size());
}

// ---- conversions for the report ----------------------------------------------

std::string FromCF(CFStringRef s) {
	if (!s) return std::string();
	CFIndex len = CFStringGetLength(s);
	CFIndex max = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;
	std::string out((size_t)max, '\0');
	if (!CFStringGetCString(s, &out[0], max, kCFStringEncodingUTF8)) out.clear();
	out.resize(strlen(out.c_str()));
	return out;
}

std::string Decode(const std::string &bytes, CFStringEncoding enc) {
	CFStringRef s = CFStringCreateWithBytes(nullptr, (const UInt8 *)bytes.data(), (CFIndex)bytes.size(), enc, false);
	if (!s && enc != kCFStringEncodingWindowsLatin1)
		s = CFStringCreateWithBytes(nullptr, (const UInt8 *)bytes.data(), (CFIndex)bytes.size(),
		                            kCFStringEncodingWindowsLatin1, false);
	std::string out = FromCF(s);
	if (s) CFRelease(s);
	return out;
}

std::string Utf16ToUtf8(const unsigned short *t, size_t n) {
	CFStringRef s = CFStringCreateWithCharacters(nullptr, (const UniChar *)t, (CFIndex)n);
	std::string out = FromCF(s);
	if (s) CFRelease(s);
	return out;
}

std::string EncodeCp949(const unsigned short *t, size_t n) {
	CFStringRef s = CFStringCreateWithCharacters(nullptr, (const UniChar *)t, (CFIndex)n);
	std::string out;
	if (s) {
		CFIndex used = 0;
		CFRange r = CFRangeMake(0, CFStringGetLength(s));
		CFStringGetBytes(s, r, kCFStringEncodingDOSKorean, '?', false, nullptr, 0, &used);
		out.resize((size_t)used);
		CFStringGetBytes(s, r, kCFStringEncodingDOSKorean, '?', false, (UInt8 *)&out[0], used, &used);
		CFRelease(s);
	}
	return out;
}

std::string Hex(const std::string &s) {
	static const char *d = "0123456789abcdef";
	std::string h;
	for (unsigned char c : s) {
		h.push_back(d[c >> 4]);
		h.push_back(d[c & 15]);
	}
	return h;
}

// Windows-1252 or UTF-8 text from the client, as UTF-8.
std::string AsUtf8(const char *s) {
	if (!s) return std::string();
	CFStringRef u = CFStringCreateWithCString(nullptr, s, kCFStringEncodingUTF8);
	if (!u) u = CFStringCreateWithCString(nullptr, s, kCFStringEncodingWindowsLatin1);
	std::string out = u ? FromCF(u) : std::string(s);
	if (u) CFRelease(u);
	return out;
}

// ---- call sites ----------------------------------------------------------------

// The demangled name without its parameter list: "UIPartyWnd::OnDraw".
std::string Demangled(const char *name) {
	int status = 0;
	char *d = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	std::string out = (status == 0 && d) ? d : name;
	free(d);
	static const std::string anon = "(anonymous namespace)::";
	size_t from = 0;
	while (out.compare(from, anon.size(), anon) == 0) from += anon.size();
	size_t paren = out.find('(', from);
	if (paren != std::string::npos && paren > from) out.erase(paren);
	return out;
}

// Frames inside the drawing path itself say nothing about who drew.
bool Helper(const std::string &sym) {
	static const char *skip[] = {"bf_coverage", "TextOutCp", "TextOutW", "TextOutA", "TextOutWith", "DrawDC::",
	                             "SingleLangTextOut", "_textout", "PolishText", "_sigtramp", "TextOutVertical",
	                             "(anonymous namespace)::Add", "(anonymous namespace)::Where", "bf_port_missing"};
	for (const char *s : skip)
		if (sym.find(s) != std::string::npos) return true;
	return false;
}

std::string Where() {
	void *frames[32];
	int n = backtrace(frames, 32);
	std::string out;
	int kept = 0;
	for (int i = 1; i < n && kept < 3; ++i) {
		std::string sym = bf_coverage_symbol(frames[i]);
		if (Helper(sym)) continue;
		if (!out.empty()) out += " < ";
		out += sym;
		++kept;
	}
	return out;
}

void Add(const char *kind, const std::string &key, long long times, Record seed, bool want_where) {
	std::lock_guard<std::mutex> g(g_lock);
	auto it = g_records.find(std::make_pair(std::string(kind), key));
	if (it == g_records.end()) {
		if (want_where) seed.where = Where();
		seed.count = 0;
		it = g_records.emplace(std::make_pair(std::string(kind), key), seed).first;
		BF_TRACE(BFT_UI, "coverage: %s: %s%s%s", kind, it->second.text.c_str(),
		         it->second.where.empty() ? "" : " at ", it->second.where.c_str());
		bf_count("coverage.entries", 1);
	}
	it->second.count += times;
}

// ---- the report ----------------------------------------------------------------

struct Collected {
	std::string kind;
	Record r;
};

void CollectFallback(const std::string &original, long long count, const std::string &site, void *user) {
	auto *out = (std::vector<Collected> *)user;
	Collected c;
	c.kind = "untranslated";
	c.r.text = Decode(original, kCFStringEncodingDOSKorean);
	c.r.hex = Hex(original);
	c.r.where = site;
	c.r.count = count;
	out->push_back(c);
}

std::vector<Collected> Collect() {
	std::vector<Collected> all;
	bf_strings_each_fallback_site(CollectFallback, &all);
	std::lock_guard<std::mutex> g(g_lock);
	for (const auto &kv : g_records) all.push_back(Collected{kv.first.first, kv.second});
	return all;
}

void JsonString(std::string *o, const std::string &s) {
	o->push_back('"');
	for (unsigned char c : s) {
		switch (c) {
		case '"': *o += "\\\""; break;
		case '\\': *o += "\\\\"; break;
		case '\n': *o += "\\n"; break;
		case '\r': *o += "\\r"; break;
		case '\t': *o += "\\t"; break;
		default:
			if (c < 0x20) {
				char b[8];
				snprintf(b, sizeof b, "\\u%04x", c);
				*o += b;
			} else {
				o->push_back((char)c);
			}
		}
	}
	o->push_back('"');
}

}  // namespace

std::string bf_coverage_symbol(const void *addr) {
	Dl_info info;
	char buf[64];
	if (!addr || !dladdr(addr, &info)) {
		snprintf(buf, sizeof buf, "%p", addr);
		return buf;
	}
	std::string out;
	uintptr_t base;
	if (info.dli_sname) {
		out = Demangled(info.dli_sname);
		base = (uintptr_t)info.dli_saddr;
	} else {
		const char *f = info.dli_fname ? strrchr(info.dli_fname, '/') : nullptr;
		out = f ? f + 1 : (info.dli_fname ? info.dli_fname : "?");
		base = (uintptr_t)info.dli_fbase;
	}
	snprintf(buf, sizeof buf, "+0x%lx", (unsigned long)((uintptr_t)addr - base));
	return out + buf;
}

extern "C" {

int bf_coverage_classify(int codepage, const void *text, int count) {
	if (!text || count == 0) return BF_COVERAGE_ENGLISH;
	if (codepage == BF_COVERAGE_UTF16) {
		const unsigned short *t = (const unsigned short *)text;
		return ClassifyUnits(t, Units(t, count), nullptr);
	}
	const char *t = (const char *)text;
	return ClassifyBytes((const unsigned char *)t, Bytes(t, count));
}

int bf_coverage_drawn(int codepage, const void *text, int count) {
	if (!text || count == 0) return BF_COVERAGE_ENGLISH;
	Record r;
	int kind;
	std::string key;
	if (codepage == BF_COVERAGE_UTF16) {
		const unsigned short *t = (const unsigned short *)text;
		size_t n = Units(t, count);
		size_t i = 0;
		while (i < n && t[i] < 0x80) ++i;
		if (i == n) return BF_COVERAGE_ENGLISH;   // ASCII: the common case, nothing more to do
		std::string bytes;
		kind = ClassifyUnits(t, n, &bytes);
		if (kind == BF_COVERAGE_ENGLISH) return kind;
		r.drawn = Utf16ToUtf8(t, n);
		if (kind == BF_COVERAGE_KOREAN && !bytes.empty()) {
			// Korean bytes drawn through a Western font: show what they say.
			r.text = Decode(bytes, kCFStringEncodingDOSKorean);
			r.hex = Hex(bytes);
		} else {
			r.text = r.drawn;
			r.hex = Hex(EncodeCp949(t, n));
		}
		key = r.drawn;
	} else {
		const char *t = (const char *)text;
		size_t n = Bytes(t, count);
		size_t i = 0;
		while (i < n && (unsigned char)t[i] < 0x80) ++i;
		if (i == n) return BF_COVERAGE_ENGLISH;
		std::string bytes(t, n);
		kind = ClassifyBytes((const unsigned char *)t, n);
		if (kind == BF_COVERAGE_ENGLISH) return kind;
		r.text = Decode(bytes, kCFStringEncodingDOSKorean);
		if (codepage != 949) r.drawn = Decode(bytes, kCFStringEncodingWindowsLatin1);
		r.hex = Hex(bytes);
		key = r.hex;
	}
	if (r.drawn == r.text) r.drawn.clear();
	Add(kind == BF_COVERAGE_KOREAN ? "korean" : "foreign", key, 1, r, true);
	return kind;
}

void bf_coverage_placeholder(const char *table, const char *what) {
	Record r;
	r.table = table ? table : "?";
	r.text = AsUtf8(what);
	Add("placeholder", r.table + "\t" + r.text, 1, r, true);
}

int bf_coverage_each(void (*fn)(const bf_coverage_entry *, void *), void *user) {
	std::vector<Collected> all = Collect();
	for (const Collected &c : all) {
		bf_coverage_entry e;
		e.kind = c.kind.c_str();
		e.text = c.r.text.c_str();
		e.drawn = c.r.drawn.empty() ? nullptr : c.r.drawn.c_str();
		e.hex = c.r.hex.c_str();
		e.table = c.r.table.empty() ? nullptr : c.r.table.c_str();
		e.where = c.r.where.c_str();
		e.count = c.r.count;
		fn(&e, user);
	}
	return (int)all.size();
}

int bf_coverage_count(void) {
	bf_strings_stats st;
	bf_strings_get_stats(&st);
	std::lock_guard<std::mutex> g(g_lock);
	return (int)g_records.size() + st.distinct_fallbacks;
}

int bf_coverage_write(const char *path) {
	std::vector<Collected> all = Collect();
	std::map<std::string, int> per;
	for (const Collected &c : all) per[c.kind]++;
	bf_strings_stats st;
	bf_strings_get_stats(&st);
	std::string o;
	char b[256];
	snprintf(b, sizeof b,
	         "{\n  \"entries\": %d,\n  \"untranslated\": %d,\n  \"placeholder\": %d,\n  \"korean\": %d,\n"
	         "  \"foreign\": %d,\n  \"strings\": {\"lookups\": %lld, \"hits\": %lld, \"fallbacks\": %lld, "
	         "\"entries\": %d, \"translated\": %d},\n  \"list\": [",
	         (int)all.size(), per["untranslated"], per["placeholder"], per["korean"], per["foreign"], st.lookups,
	         st.hits, st.fallbacks, st.entries, st.translated);
	o += b;
	for (size_t i = 0; i < all.size(); ++i) {
		const Collected &c = all[i];
		o += i ? ",\n    {" : "\n    {";
		o += "\"kind\": ";
		JsonString(&o, c.kind);
		o += ", \"text\": ";
		JsonString(&o, c.r.text);
		o += ", \"drawn\": ";
		if (c.r.drawn.empty()) o += "null"; else JsonString(&o, c.r.drawn);
		o += ", \"hex\": ";
		JsonString(&o, c.r.hex);
		o += ", \"table\": ";
		if (c.r.table.empty()) o += "null"; else JsonString(&o, c.r.table);
		o += ", \"where\": ";
		JsonString(&o, c.r.where);
		snprintf(b, sizeof b, ", \"count\": %lld}", c.r.count);
		o += b;
	}
	o += all.empty() ? "]\n}\n" : "\n  ]\n}\n";
	FILE *f = fopen(path, "wb");
	if (!f) {
		BF_TRACE(BFT_ERR, "coverage: cannot write %s", path);
		return -1;
	}
	bool ok = fwrite(o.data(), 1, o.size(), f) == o.size();
	ok = fclose(f) == 0 && ok;
	if (!ok) {
		BF_TRACE(BFT_ERR, "coverage: writing %s failed", path);
		return -1;
	}
	BF_TRACE(BFT_UI, "coverage: %d entries written to %s", (int)all.size(), path);
	return 0;
}

void bf_coverage_reset(void) {
	bf_strings_reset_counts();
	std::lock_guard<std::mutex> g(g_lock);
	g_records.clear();
}

}  // extern "C"
