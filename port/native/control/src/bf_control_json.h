// JSON values, the parser and the writer of the control module. Internal.
#ifndef BF_CONTROL_JSON_H
#define BF_CONTROL_JSON_H

#include "bf_control.h"

#include <string>
#include <vector>

struct bf_control_value {
	int type = BF_CONTROL_T_NULL;
	bool b = false;
	bool isInt = false;      // a number written without fraction or exponent, fitting int64
	int64_t i = 0;
	double d = 0;
	std::string s;           // string value
	std::vector<bf_control_value> items;   // array elements or object member values
	std::vector<std::string> keys;         // object member keys, parallel to items
};

namespace bfctl {

// Checks that the bytes are UTF-8 (no overlongs, no surrogates, at most U+10FFFF).
bool ValidUtf8(const char *p, size_t n);

// Parses exactly one JSON value (surrounding whitespace allowed). On failure
// returns false and a message for people in *error.
bool ParseJson(const char *p, size_t n, bf_control_value *out, std::string *error);

// Deep equality; numbers by value, objects regardless of member order.
bool JsonEqual(const bf_control_value &a, const bf_control_value &b);

// Appends a JSON string literal; invalid UTF-8 becomes U+FFFD.
void AppendString(std::string &out, const char *p, size_t n);

// Serialises a value.
void AppendValue(std::string &out, const bf_control_value &v);

} // namespace bfctl

struct bf_control_writer {
	enum Mode { kFields, kValue };
	struct Frame { char kind; int count; bool haveKey; };  // kind 'o' object, 'a' array, 't' top

	std::string out;
	std::vector<Frame> stack;
	bool broken = false;
	std::string problem;

	// kFields: the writer continues an object whose first members are
	// already written (a reply or an event); kValue: exactly one value.
	void Reset(Mode mode);
	bool Complete() const;     // a well-formed result
	void Fail(const char *what);

	bool BeforeValue();        // false if a value may not be written here
	void Key(const char *k, size_t n);
	void Raw(const std::string &json);   // a value already serialised
	void Begin(char kind);
	void End(char kind);
};

#endif
