// A small strict JSON reader and writer (RFC 8259), written for the control
// socket: no comments, no trailing commas, no leading zeros, escapes and
// surrogate pairs checked, nesting limited.

#include "bf_control_json.h"

#include <charconv>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace bfctl {

namespace {

const int kMaxDepth = 64;

// Length of the UTF-8 sequence at p (at most n bytes), or 0 if invalid.
size_t Utf8Seq(const unsigned char *p, size_t n, uint32_t *cp)
{
	unsigned c = p[0];
	if (c < 0x80) { *cp = c; return 1; }
	size_t len;
	uint32_t v, min;
	if (c >= 0xc2 && c <= 0xdf) { len = 2; v = c & 0x1f; min = 0x80; }
	else if (c >= 0xe0 && c <= 0xef) { len = 3; v = c & 0x0f; min = 0x800; }
	else if (c >= 0xf0 && c <= 0xf4) { len = 4; v = c & 0x07; min = 0x10000; }
	else return 0;
	if (n < len) return 0;
	for (size_t i = 1; i < len; i++) {
		if ((p[i] & 0xc0) != 0x80) return 0;
		v = (v << 6) | (p[i] & 0x3f);
	}
	if (v < min || v > 0x10ffff || (v >= 0xd800 && v <= 0xdfff)) return 0;
	*cp = v;
	return len;
}

void AppendUtf8(std::string &out, uint32_t cp)
{
	if (cp < 0x80) out += (char)cp;
	else if (cp < 0x800) { out += (char)(0xc0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3f)); }
	else if (cp < 0x10000) {
		out += (char)(0xe0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3f));
		out += (char)(0x80 | (cp & 0x3f));
	} else {
		out += (char)(0xf0 | (cp >> 18)); out += (char)(0x80 | ((cp >> 12) & 0x3f));
		out += (char)(0x80 | ((cp >> 6) & 0x3f)); out += (char)(0x80 | (cp & 0x3f));
	}
}

class Parser {
public:
	Parser(const char *p, size_t n) : m_p(p), m_end(p + n), m_begin(p) {}

	bool Document(bf_control_value *out, std::string *error)
	{
		Space();
		if (!Value(out, 0)) { Report(error); return false; }
		Space();
		if (m_p != m_end) { m_err = "text after the value"; Report(error); return false; }
		return true;
	}

private:
	const char *m_p, *m_end, *m_begin;
	const char *m_err = nullptr;

	void Report(std::string *error)
	{
		if (!error) return;
		char buf[96];
		snprintf(buf, sizeof buf, "%s at byte %ld", m_err ? m_err : "syntax error", (long)(m_p - m_begin));
		*error = buf;
	}

	bool Fail(const char *what) { if (!m_err) m_err = what; return false; }

	void Space()
	{
		while (m_p < m_end && (*m_p == ' ' || *m_p == '\t' || *m_p == '\n' || *m_p == '\r'))
			m_p++;
	}

	bool Literal(const char *word)
	{
		size_t n = strlen(word);
		if ((size_t)(m_end - m_p) < n || memcmp(m_p, word, n) != 0)
			return Fail("unknown literal");
		m_p += n;
		return true;
	}

	bool Value(bf_control_value *v, int depth)
	{
		if (m_p >= m_end) return Fail("unexpected end");
		switch (*m_p) {
		case '{': return Object(v, depth + 1);
		case '[': return Array(v, depth + 1);
		case '"': v->type = BF_CONTROL_T_STRING; return String(&v->s);
		case 't': v->type = BF_CONTROL_T_BOOL; v->b = true; return Literal("true");
		case 'f': v->type = BF_CONTROL_T_BOOL; v->b = false; return Literal("false");
		case 'n': v->type = BF_CONTROL_T_NULL; return Literal("null");
		default:
			if (*m_p == '-' || (*m_p >= '0' && *m_p <= '9'))
				return Number(v);
			return Fail("unexpected character");
		}
	}

	bool Object(bf_control_value *v, int depth)
	{
		if (depth > kMaxDepth) return Fail("nested too deeply");
		v->type = BF_CONTROL_T_OBJECT;
		m_p++;
		Space();
		if (m_p < m_end && *m_p == '}') { m_p++; return true; }
		for (;;) {
			Space();
			if (m_p >= m_end || *m_p != '"') return Fail("expected a key");
			std::string key;
			if (!String(&key)) return false;
			Space();
			if (m_p >= m_end || *m_p != ':') return Fail("expected ':'");
			m_p++;
			Space();
			v->keys.push_back(std::move(key));
			v->items.emplace_back();
			if (!Value(&v->items.back(), depth)) return false;
			Space();
			if (m_p >= m_end) return Fail("unexpected end");
			if (*m_p == ',') { m_p++; continue; }
			if (*m_p == '}') { m_p++; return true; }
			return Fail("expected ',' or '}'");
		}
	}

	bool Array(bf_control_value *v, int depth)
	{
		if (depth > kMaxDepth) return Fail("nested too deeply");
		v->type = BF_CONTROL_T_ARRAY;
		m_p++;
		Space();
		if (m_p < m_end && *m_p == ']') { m_p++; return true; }
		for (;;) {
			Space();
			v->items.emplace_back();
			if (!Value(&v->items.back(), depth)) return false;
			Space();
			if (m_p >= m_end) return Fail("unexpected end");
			if (*m_p == ',') { m_p++; continue; }
			if (*m_p == ']') { m_p++; return true; }
			return Fail("expected ',' or ']'");
		}
	}

	bool Hex4(uint32_t *out)
	{
		if (m_end - m_p < 4) return Fail("short \\u escape");
		uint32_t v = 0;
		for (int i = 0; i < 4; i++) {
			char c = m_p[i];
			v <<= 4;
			if (c >= '0' && c <= '9') v |= c - '0';
			else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
			else return Fail("bad \\u escape");
		}
		m_p += 4;
		*out = v;
		return true;
	}

	bool String(std::string *out)
	{
		m_p++;  // opening quote
		for (;;) {
			if (m_p >= m_end) return Fail("unterminated string");
			unsigned char c = (unsigned char)*m_p;
			if (c == '"') { m_p++; return true; }
			if (c < 0x20) return Fail("control character in string");
			if (c != '\\') {
				uint32_t cp;
				size_t len = Utf8Seq((const unsigned char *)m_p, m_end - m_p, &cp);
				if (!len) return Fail("invalid UTF-8");
				out->append(m_p, len);
				m_p += len;
				continue;
			}
			m_p++;
			if (m_p >= m_end) return Fail("unterminated escape");
			char e = *m_p++;
			switch (e) {
			case '"': *out += '"'; break;
			case '\\': *out += '\\'; break;
			case '/': *out += '/'; break;
			case 'b': *out += '\b'; break;
			case 'f': *out += '\f'; break;
			case 'n': *out += '\n'; break;
			case 'r': *out += '\r'; break;
			case 't': *out += '\t'; break;
			case 'u': {
				uint32_t cp;
				if (!Hex4(&cp)) return false;
				if (cp >= 0xdc00 && cp <= 0xdfff) return Fail("lone low surrogate");
				if (cp >= 0xd800 && cp <= 0xdbff) {
					uint32_t lo;
					if (m_end - m_p < 2 || m_p[0] != '\\' || m_p[1] != 'u') return Fail("lone high surrogate");
					m_p += 2;
					if (!Hex4(&lo)) return false;
					if (lo < 0xdc00 || lo > 0xdfff) return Fail("bad surrogate pair");
					cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
				}
				AppendUtf8(*out, cp);
				break;
			}
			default:
				return Fail("unknown escape");
			}
		}
	}

	bool Number(bf_control_value *v)
	{
		const char *start = m_p;
		bool integer = true;
		if (*m_p == '-') m_p++;
		if (m_p >= m_end) return Fail("bad number");
		if (*m_p == '0') {
			m_p++;
		} else if (*m_p >= '1' && *m_p <= '9') {
			while (m_p < m_end && *m_p >= '0' && *m_p <= '9') m_p++;
		} else {
			return Fail("bad number");
		}
		if (m_p < m_end && *m_p == '.') {
			integer = false;
			m_p++;
			if (m_p >= m_end || *m_p < '0' || *m_p > '9') return Fail("bad fraction");
			while (m_p < m_end && *m_p >= '0' && *m_p <= '9') m_p++;
		}
		if (m_p < m_end && (*m_p == 'e' || *m_p == 'E')) {
			integer = false;
			m_p++;
			if (m_p < m_end && (*m_p == '+' || *m_p == '-')) m_p++;
			if (m_p >= m_end || *m_p < '0' || *m_p > '9') return Fail("bad exponent");
			while (m_p < m_end && *m_p >= '0' && *m_p <= '9') m_p++;
		}
		std::string text(start, m_p - start);
		v->type = BF_CONTROL_T_NUMBER;
		v->d = strtod(text.c_str(), nullptr);
		if (!std::isfinite(v->d)) return Fail("number out of range");
		if (integer) {
			int64_t i;
			auto r = std::from_chars(text.data(), text.data() + text.size(), i);
			if (r.ec == std::errc() && r.ptr == text.data() + text.size()) {
				v->isInt = true;
				v->i = i;
			}
		}
		return true;
	}
};

} // namespace

bool ValidUtf8(const char *p, size_t n)
{
	const unsigned char *u = (const unsigned char *)p;
	size_t i = 0;
	while (i < n) {
		uint32_t cp;
		size_t len = Utf8Seq(u + i, n - i, &cp);
		if (!len) return false;
		i += len;
	}
	return true;
}

bool ParseJson(const char *p, size_t n, bf_control_value *out, std::string *error)
{
	*out = bf_control_value();
	Parser parser(p, n);
	return parser.Document(out, error);
}

bool JsonEqual(const bf_control_value &a, const bf_control_value &b)
{
	if (a.type != b.type) return false;
	switch (a.type) {
	case BF_CONTROL_T_NULL: return true;
	case BF_CONTROL_T_BOOL: return a.b == b.b;
	case BF_CONTROL_T_NUMBER:
		if (a.isInt && b.isInt) return a.i == b.i;
		return a.d == b.d;
	case BF_CONTROL_T_STRING: return a.s == b.s;
	case BF_CONTROL_T_ARRAY:
		if (a.items.size() != b.items.size()) return false;
		for (size_t i = 0; i < a.items.size(); i++)
			if (!JsonEqual(a.items[i], b.items[i])) return false;
		return true;
	case BF_CONTROL_T_OBJECT: {
		if (a.items.size() != b.items.size()) return false;
		for (size_t i = 0; i < a.items.size(); i++) {
			const bf_control_value *other = bf_control_value_get(&b, a.keys[i].c_str());
			if (!other || !JsonEqual(a.items[i], *other)) return false;
		}
		return true;
	}
	}
	return false;
}

void AppendString(std::string &out, const char *p, size_t n)
{
	static const char hex[] = "0123456789abcdef";
	const unsigned char *u = (const unsigned char *)p;
	out += '"';
	size_t i = 0;
	while (i < n) {
		unsigned c = u[i];
		if (c < 0x80) {
			switch (c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			default:
				if (c < 0x20 || c == 0x7f) {
					out += "\\u00";
					out += hex[c >> 4];
					out += hex[c & 15];
				} else {
					out += (char)c;
				}
			}
			i++;
			continue;
		}
		uint32_t cp;
		size_t len = Utf8Seq(u + i, n - i, &cp);
		if (!len) {
			out += "\xef\xbf\xbd";   // U+FFFD
			i++;
		} else {
			out.append(p + i, len);
			i += len;
		}
	}
	out += '"';
}

void AppendValue(std::string &out, const bf_control_value &v)
{
	switch (v.type) {
	case BF_CONTROL_T_BOOL: out += v.b ? "true" : "false"; break;
	case BF_CONTROL_T_NUMBER: {
		char buf[40];
		if (v.isInt) {
			auto r = std::to_chars(buf, buf + sizeof buf, v.i);
			out.append(buf, r.ptr - buf);
		} else {
			auto r = std::to_chars(buf, buf + sizeof buf, v.d);
			out.append(buf, r.ptr - buf);
		}
		break;
	}
	case BF_CONTROL_T_STRING: AppendString(out, v.s.data(), v.s.size()); break;
	case BF_CONTROL_T_ARRAY:
		out += '[';
		for (size_t i = 0; i < v.items.size(); i++) {
			if (i) out += ',';
			AppendValue(out, v.items[i]);
		}
		out += ']';
		break;
	case BF_CONTROL_T_OBJECT:
		out += '{';
		for (size_t i = 0; i < v.items.size(); i++) {
			if (i) out += ',';
			AppendString(out, v.keys[i].data(), v.keys[i].size());
			out += ':';
			AppendValue(out, v.items[i]);
		}
		out += '}';
		break;
	default: out += "null"; break;
	}
}

} // namespace bfctl

// ------------------------------------------------------------ writer state

void bf_control_writer::Reset(Mode mode)
{
	out.clear();
	stack.clear();
	broken = false;
	problem.clear();
	if (mode == kFields)
		stack.push_back(Frame{'o', 1, false});   // members before ours exist
	else
		stack.push_back(Frame{'t', 0, false});
}

bool bf_control_writer::Complete() const
{
	if (broken || stack.size() != 1) return false;
	const Frame &f = stack.back();
	if (f.kind == 'o') return !f.haveKey;
	return f.count == 1;
}

void bf_control_writer::Fail(const char *what)
{
	if (!broken) {
		broken = true;
		problem = what;
	}
}

bool bf_control_writer::BeforeValue()
{
	if (broken) return false;
	if (stack.empty()) { Fail("value after the end"); return false; }
	Frame &f = stack.back();
	switch (f.kind) {
	case 'o':
		if (!f.haveKey) { Fail("value without a key"); return false; }
		f.haveKey = false;
		f.count++;
		return true;
	case 'a':
		if (f.count++) out += ',';
		return true;
	default:
		if (f.count) { Fail("second value"); return false; }
		f.count++;
		return true;
	}
}

void bf_control_writer::Key(const char *k, size_t n)
{
	if (broken) return;
	if (stack.empty() || stack.back().kind != 'o' || stack.back().haveKey) {
		Fail("key outside an object");
		return;
	}
	if (stack.back().count) out += ',';
	bfctl::AppendString(out, k, n);
	out += ':';
	stack.back().haveKey = true;
}

void bf_control_writer::Raw(const std::string &json)
{
	if (BeforeValue()) out += json;
}

void bf_control_writer::Begin(char kind)
{
	if (!BeforeValue()) return;
	out += kind == 'o' ? '{' : '[';
	stack.push_back(Frame{kind, 0, false});
}

void bf_control_writer::End(char kind)
{
	if (broken) return;
	if (stack.size() < 2 || stack.back().kind != kind || stack.back().haveKey) {
		Fail("unbalanced end");
		return;
	}
	stack.pop_back();
	out += kind == 'o' ? '}' : ']';
}

// -------------------------------------------------------------- C entries

extern "C" {

int bf_control_value_type(const bf_control_value *v)
{
	return v ? v->type : BF_CONTROL_T_ABSENT;
}

const bf_control_value *bf_control_value_get(const bf_control_value *object, const char *key)
{
	if (!object || !key || object->type != BF_CONTROL_T_OBJECT) return nullptr;
	for (size_t i = object->keys.size(); i-- > 0;)
		if (object->keys[i] == key) return &object->items[i];
	return nullptr;
}

int bf_control_value_int(const bf_control_value *v, int64_t *out)
{
	if (!v || v->type != BF_CONTROL_T_NUMBER || !v->isInt) return 0;
	if (out) *out = v->i;
	return 1;
}

int bf_control_value_number(const bf_control_value *v, double *out)
{
	if (!v || v->type != BF_CONTROL_T_NUMBER) return 0;
	if (out) *out = v->d;
	return 1;
}

int bf_control_value_bool(const bf_control_value *v, int *out)
{
	if (!v || v->type != BF_CONTROL_T_BOOL) return 0;
	if (out) *out = v->b ? 1 : 0;
	return 1;
}

const char *bf_control_value_string(const bf_control_value *v, size_t *length)
{
	if (!v || v->type != BF_CONTROL_T_STRING) return nullptr;
	if (length) *length = v->s.size();
	return v->s.c_str();
}

size_t bf_control_value_count(const bf_control_value *v)
{
	if (!v || (v->type != BF_CONTROL_T_ARRAY && v->type != BF_CONTROL_T_OBJECT)) return 0;
	return v->items.size();
}

const bf_control_value *bf_control_value_at(const bf_control_value *v, size_t index)
{
	if (index >= bf_control_value_count(v)) return nullptr;
	return &v->items[index];
}

const char *bf_control_value_key_at(const bf_control_value *v, size_t index)
{
	if (!v || v->type != BF_CONTROL_T_OBJECT || index >= v->keys.size()) return nullptr;
	return v->keys[index].c_str();
}

void bf_control_write_key(bf_control_writer *w, const char *key)
{
	if (w) w->Key(key ? key : "", key ? strlen(key) : 0);
}

void bf_control_write_null(bf_control_writer *w)
{
	if (w) w->Raw("null");
}

void bf_control_write_bool(bf_control_writer *w, int value)
{
	if (w) w->Raw(value ? "true" : "false");
}

void bf_control_write_int(bf_control_writer *w, int64_t value)
{
	if (!w) return;
	char buf[24];
	auto r = std::to_chars(buf, buf + sizeof buf, value);
	w->Raw(std::string(buf, r.ptr - buf));
}

void bf_control_write_number(bf_control_writer *w, double value)
{
	if (!w) return;
	if (!std::isfinite(value)) { w->Raw("null"); return; }
	char buf[40];
	auto r = std::to_chars(buf, buf + sizeof buf, value);
	w->Raw(std::string(buf, r.ptr - buf));
}

void bf_control_write_string_n(bf_control_writer *w, const char *utf8, size_t length)
{
	if (!w) return;
	if (!utf8) { w->Raw("null"); return; }
	std::string s;
	bfctl::AppendString(s, utf8, length);
	w->Raw(s);
}

void bf_control_write_string(bf_control_writer *w, const char *utf8)
{
	bf_control_write_string_n(w, utf8, utf8 ? strlen(utf8) : 0);
}

void bf_control_write_begin_object(bf_control_writer *w) { if (w) w->Begin('o'); }
void bf_control_write_end_object(bf_control_writer *w)   { if (w) w->End('o'); }
void bf_control_write_begin_array(bf_control_writer *w)  { if (w) w->Begin('a'); }
void bf_control_write_end_array(bf_control_writer *w)    { if (w) w->End('a'); }

void bf_control_write_value(bf_control_writer *w, const bf_control_value *v)
{
	if (!w) return;
	std::string s;
	if (v) bfctl::AppendValue(s, *v); else s = "null";
	w->Raw(s);
}

} // extern "C"
