// Code-page conversion for bf_text. CoreFoundation knows the Windows code
// pages; it is asked once per code page to fill a table (every single byte,
// every lead/trail pair), and the conversions then run on the tables. That
// keeps them fast, makes undefined bytes predictable instead of failing
// the whole string, and allows the reverse direction without CF's lossy
// mode.

#include "bf_text.h"
#include "bf_text_internal.h"
#include "bf_trace.h"

#include <CoreFoundation/CoreFoundation.h>

#include <string.h>

#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace {

struct CodePage {
	int number = 0;
	bool dbcs = false;
	bool lead[256] = {};
	uint16_t single[256] = {};
	std::vector<uint16_t> pairs;                  // [lead * 256 + trail], 0 = undefined
	std::unordered_map<uint16_t, uint16_t> back;  // UTF-16 unit -> byte or pair
};

std::mutex g_lock;
std::map<int, std::unique_ptr<CodePage>> g_pages;

bool IsDbcsLead(int cp, unsigned b)
{
	switch (cp) {
	case 932: return (b >= 0x81 && b <= 0x9F) || (b >= 0xE0 && b <= 0xFC);
	case 936: case 949: case 950: return b >= 0x81 && b <= 0xFE;
	default: return false;
	}
}

CFStringEncoding EncodingFor(int cp)
{
	if (cp == 949) return kCFStringEncodingDOSKorean;
	if (cp == 1252) return kCFStringEncodingWindowsLatin1;
	return CFStringConvertWindowsCodepageToEncoding((UInt32)cp);
}

// The single UTF-16 unit `n` bytes decode to, or 0.
uint16_t Decode(CFStringEncoding enc, const uint8_t *bytes, int n)
{
	CFStringRef s = CFStringCreateWithBytes(NULL, bytes, n, enc, false);
	if (!s) return 0;
	uint16_t u = 0;
	if (CFStringGetLength(s) == 1) u = CFStringGetCharacterAtIndex(s, 0);
	CFRelease(s);
	return u;
}

CodePage *Build(int cp)
{
	CFStringEncoding enc = EncodingFor(cp);
	if (enc == kCFStringEncodingInvalidId || !CFStringIsEncodingAvailable(enc)) return NULL;

	std::unique_ptr<CodePage> page(new CodePage);
	page->number = cp;
	for (int b = 0; b < 256; ++b) page->lead[b] = IsDbcsLead(cp, b);
	page->dbcs = page->lead[0x81];

	for (int b = 1; b < 256; ++b) {
		if (page->lead[b]) continue;
		uint8_t byte = (uint8_t)b;
		uint16_t u = Decode(enc, &byte, 1);
		if (!u) u = (cp == 1252) ? (uint16_t)b : 0xFFFD;  // Windows maps 0x81, 0x8D ... to U+0081 ...
		page->single[b] = u;
	}

	if (page->dbcs) {
		page->pairs.assign(256 * 256, 0);
		for (int l = 0x81; l < 256; ++l) {
			if (!page->lead[l]) continue;
			for (int t = 0x40; t < 256; ++t) {
				uint8_t pair[2] = { (uint8_t)l, (uint8_t)t };
				uint16_t u = Decode(enc, pair, 2);
				if (u && u != 0xFFFD) page->pairs[l * 256 + t] = u;
			}
		}
	}

	// The reverse map: single bytes first so that they win over pairs.
	for (int b = 1; b < 256; ++b)
		if (!page->lead[b] && page->single[b] != 0xFFFD) page->back.emplace(page->single[b], (uint16_t)b);
	for (size_t i = 0; i < page->pairs.size(); ++i)
		if (page->pairs[i]) page->back.emplace(page->pairs[i], (uint16_t)i);

	return page.release();
}

const CodePage *Get(int cp)
{
	std::lock_guard<std::mutex> hold(g_lock);
	auto it = g_pages.find(cp);
	if (it != g_pages.end()) return it->second.get();
	CodePage *page = Build(cp);
	if (!page) bf_tracef(BFT_ERR, "text: code page %d is not available", cp);
	g_pages[cp].reset(page);  // a failure is remembered too
	return page;
}

int ByteLength(const char *text, int len)
{
	if (len >= 0) return len;
	return text ? (int)strlen(text) : 0;
}

int Utf16Length(const uint16_t *text, int len)
{
	if (len >= 0) return len;
	int n = 0;
	if (text) while (text[n]) ++n;
	return n;
}

// Appends code point c as UTF-16.
void PutUtf16(std::vector<uint16_t> &out, uint32_t c)
{
	if (c >= 0x10000 && c <= 0x10FFFF) {
		c -= 0x10000;
		out.push_back((uint16_t)(0xD800 + (c >> 10)));
		out.push_back((uint16_t)(0xDC00 + (c & 0x3FF)));
	} else {
		out.push_back((uint16_t)c);
	}
}

void DecodeUtf8(const char *text, int len, std::vector<uint16_t> &out)
{
	const uint8_t *s = (const uint8_t *)text;
	int i = 0;
	while (i < len) {
		uint32_t c = s[i];
		int extra = c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : -1;
		if (extra < 0) { PutUtf16(out, 0xFFFD); ++i; continue; }
		c &= extra == 0 ? 0x7F : extra == 1 ? 0x1F : extra == 2 ? 0x0F : 0x07;
		bool ok = true;
		for (int k = 1; k <= extra; ++k) {
			if (i + k >= len || (s[i + k] & 0xC0) != 0x80) { ok = false; break; }
			c = (c << 6) | (s[i + k] & 0x3F);
		}
		if (!ok) { PutUtf16(out, 0xFFFD); ++i; continue; }
		PutUtf16(out, c);
		i += extra + 1;
	}
}

void EncodeUtf8(const uint16_t *text, int len, std::vector<char> &out)
{
	for (int i = 0; i < len; ++i) {
		uint32_t c = text[i];
		if (c >= 0xD800 && c < 0xDC00 && i + 1 < len && text[i + 1] >= 0xDC00 && text[i + 1] < 0xE000) {
			c = 0x10000 + ((c - 0xD800) << 10) + (text[i + 1] - 0xDC00);
			++i;
		} else if (c >= 0xD800 && c < 0xE000) {
			c = 0xFFFD;
		}
		if (c < 0x80) out.push_back((char)c);
		else if (c < 0x800) { out.push_back((char)(0xC0 | (c >> 6))); out.push_back((char)(0x80 | (c & 0x3F))); }
		else if (c < 0x10000) {
			out.push_back((char)(0xE0 | (c >> 12))); out.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
			out.push_back((char)(0x80 | (c & 0x3F)));
		} else {
			out.push_back((char)(0xF0 | (c >> 18))); out.push_back((char)(0x80 | ((c >> 12) & 0x3F)));
			out.push_back((char)(0x80 | ((c >> 6) & 0x3F))); out.push_back((char)(0x80 | (c & 0x3F)));
		}
	}
}

template <typename T>
int Deliver(const std::vector<T> &v, T *out, int out_max)
{
	if (out) {
		int n = (int)v.size() < out_max ? (int)v.size() : out_max;
		for (int i = 0; i < n; ++i) out[i] = v[i];
	}
	return (int)v.size();
}

} // namespace

// ---- internal ------------------------------------------------------------

int bf_text_decode(int codepage, const void *text, int len, std::vector<uint16_t> &units, std::vector<int> *offsets)
{
	units.clear();
	if (offsets) offsets->clear();
	if (codepage == BF_TEXT_CP_UTF16) {
		const uint16_t *s = (const uint16_t *)text;
		int n = Utf16Length(s, len);
		units.assign(s, s + n);
		if (offsets) for (int i = 0; i < n; ++i) offsets->push_back(i);
		return 0;
	}
	const char *s = (const char *)text;
	int n = ByteLength(s, len);
	if (codepage == BF_TEXT_CP_UTF8) {
		// Offsets for UTF-8 are per unit of the decoded text; good enough for
		// the callers that pass UTF-8 (tools and tests, not the client).
		DecodeUtf8(s, n, units);
		if (offsets) for (size_t i = 0; i < units.size(); ++i) offsets->push_back((int)i);
		return 0;
	}
	const CodePage *page = Get(codepage);
	if (!page) return -1;
	const uint8_t *b = (const uint8_t *)s;
	for (int i = 0; i < n;) {
		if (offsets) offsets->push_back(i);
		if (b[i] == 0) { units.push_back(0); ++i; continue; }
		if (page->lead[b[i]]) {
			if (i + 1 < n && b[i + 1] != 0) {
				uint16_t u = b[i + 1] >= 0x40 ? page->pairs[b[i] * 256 + b[i + 1]] : 0;
				units.push_back(u ? u : (uint16_t)0xFFFD);
				i += 2;
			} else {
				units.push_back(0xFFFD);
				++i;
			}
			continue;
		}
		units.push_back(page->single[b[i]]);
		++i;
	}
	return 0;
}

// ---- public --------------------------------------------------------------

extern "C" int bf_text_codepage_for_charset(int charset)
{
	switch (charset) {
	case BF_TEXT_CHARSET_SHIFTJIS:    return 932;
	case BF_TEXT_CHARSET_HANGUL:      return 949;
	case BF_TEXT_CHARSET_GB2312:      return 936;
	case BF_TEXT_CHARSET_CHINESEBIG5: return 950;
	case BF_TEXT_CHARSET_GREEK:       return 1253;
	case BF_TEXT_CHARSET_TURKISH:     return 1254;
	case BF_TEXT_CHARSET_RUSSIAN:     return 1251;
	case BF_TEXT_CHARSET_VIETNAMESE:  return 1258;
	case BF_TEXT_CHARSET_HEBREW:      return 1255;
	case BF_TEXT_CHARSET_ARABIC:      return 1256;
	case BF_TEXT_CHARSET_BALTIC:      return 1257;
	case BF_TEXT_CHARSET_THAI:        return 874;
	case BF_TEXT_CHARSET_EASTEUROPE:  return 1250;
	default:                          return 1252;
	}
}

extern "C" int bf_text_is_lead_byte(int codepage, unsigned char byte)
{
	return IsDbcsLead(codepage, byte) ? 1 : 0;
}

extern "C" int bf_text_char_length(int codepage, const char *text, int len)
{
	if (!text || len == 0 || text[0] == 0) return 0;
	bool more = len < 0 || len >= 2;
	if (IsDbcsLead(codepage, (unsigned char)text[0]) && more && text[1] != 0) return 2;
	if (codepage == BF_TEXT_CP_UTF8) {
		unsigned c = (unsigned char)text[0];
		int n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
		int i = 1;
		while (i < n && (len < 0 || i < len) && ((unsigned char)text[i] & 0xC0) == 0x80) ++i;
		return i;
	}
	return 1;
}

extern "C" int bf_text_char_prev(int codepage, const char *text, int at)
{
	if (!text || at <= 0) return 0;
	int pos = 0, prev = 0;
	while (pos < at) {
		int n = bf_text_char_length(codepage, text + pos, -1);
		if (n == 0) n = 1;
		prev = pos;
		pos += n;
	}
	return prev;
}

extern "C" int bf_text_to_utf16(int codepage, const char *text, int len, uint16_t *out, int out_max)
{
	if (codepage == BF_TEXT_CP_UTF16) return -1;
	std::vector<uint16_t> units;
	if (bf_text_decode(codepage, text, len, units, NULL) < 0) return -1;
	return Deliver(units, out, out_max);
}

extern "C" int bf_text_from_utf16(int codepage, const uint16_t *text, int len, char *out, int out_max)
{
	int n = Utf16Length(text, len);
	std::vector<char> bytes;
	if (codepage == BF_TEXT_CP_UTF8) {
		EncodeUtf8(text, n, bytes);
		return Deliver(bytes, out, out_max);
	}
	if (codepage == BF_TEXT_CP_UTF16) return -1;
	const CodePage *page = Get(codepage);
	if (!page) return -1;
	for (int i = 0; i < n; ++i) {
		uint16_t u = text[i];
		if (u == 0) { bytes.push_back(0); continue; }
		if (u >= 0xD800 && u < 0xDC00 && i + 1 < n) ++i;  // a pair no code page holds
		auto it = (u >= 0xD800 && u < 0xE000) ? page->back.end() : page->back.find(u);
		if (it == page->back.end()) { bytes.push_back('?'); continue; }
		if (it->second > 0xFF) bytes.push_back((char)(it->second >> 8));
		bytes.push_back((char)(it->second & 0xFF));
	}
	return Deliver(bytes, out, out_max);
}

extern "C" int bf_text_to_utf8(int codepage, const char *text, int len, char *out, int out_max)
{
	std::vector<uint16_t> units;
	if (codepage == BF_TEXT_CP_UTF16 || bf_text_decode(codepage, text, len, units, NULL) < 0) return -1;
	std::vector<char> bytes;
	EncodeUtf8(units.data(), (int)units.size(), bytes);
	return Deliver(bytes, out, out_max);
}

extern "C" int bf_text_from_utf8(int codepage, const char *utf8, int len, char *out, int out_max)
{
	std::vector<uint16_t> units;
	DecodeUtf8(utf8, ByteLength(utf8, len), units);
	if (codepage == BF_TEXT_CP_UTF16) return -1;
	return bf_text_from_utf16(codepage, units.data(), (int)units.size(), out, out_max);
}
