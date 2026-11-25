// The Visual C++ runtime names the client calls that the C library of macOS
// does not have (M3.2), declared by bf_prefix.h (itoa, _ltoa, strlwr, ...):
// the real functions.
//
// And the C library's file calls on client paths (bf_crt.h): the functions
// behind the macros fopen, access, remove and rename, on port/native/sys
// (bf_file.h). They were the win32 adapter's until M11.3; call sites that
// move to bf_file.h directly no longer need them. bifrost-crt-test
// (port/client/test) calls both as the client does.
#include "bf_types.h"
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bf_file.h"
#include "bf_trace.h"

namespace {

// Visual C++'s _itoa: radix 2..36, a minus sign only in radix 10, other
// radices print the value's two's complement as unsigned.
char *IntToText(long long value, unsigned long long as_unsigned, char *buffer, int radix)
{
	if (!buffer)
		return buffer;
	if (radix < 2 || radix > 36) {
		buffer[0] = 0;
		return buffer;
	}
	char digits[72];
	int n = 0;
	bool negative = radix == 10 && value < 0;
	unsigned long long u = negative ? (unsigned long long)(-(value + 1)) + 1 : as_unsigned;
	do {
		int d = (int)(u % (unsigned)radix);
		digits[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
		u /= (unsigned)radix;
	} while (u);
	char *p = buffer;
	if (negative)
		*p++ = '-';
	while (n)
		*p++ = digits[--n];
	*p = 0;
	return buffer;
}

char *MapAscii(char *text, int (*fn)(int))
{
	if (!text)
		return text;
	for (char *p = text; *p; ++p) {
		unsigned char c = (unsigned char)*p;
		if (c < 0x80)   // the "C" locale: ASCII letters only, CP949 bytes untouched
			*p = (char)fn(c);
	}
	return text;
}

}  // namespace

extern "C" {

char *itoa(int value, char *buffer, int radix)
{
	return IntToText(value, (unsigned int)value, buffer, radix);
}

char *_itoa(int value, char *buffer, int radix)
{
	return IntToText(value, (unsigned int)value, buffer, radix);
}

// `long` of Visual C++ is 32 bits; bf_prefix.h declares it with int.
char *_ltoa(int value, char *buffer, int radix)
{
	return IntToText(value, (unsigned int)value, buffer, radix);
}

char *strlwr(char *text) { return MapAscii(text, tolower); }
char *_strlwr(char *text) { return MapAscii(text, tolower); }
char *strupr(char *text) { return MapAscii(text, toupper); }
char *_strupr(char *text) { return MapAscii(text, toupper); }

unsigned int _lrotl(unsigned int value, int shift)
{
	shift &= 31;
	return shift ? (value << shift) | (value >> (32 - shift)) : value;
}

unsigned int _lrotr(unsigned int value, int shift)
{
	shift &= 31;
	return shift ? (value >> shift) | (value << (32 - shift)) : value;
}

}  // extern "C"

// ---- The C library on client paths (bf_crt.h) ----------------------------------
// Reads find the file in the game folder (or in a write location first);
// writes go to the write locations and are refused in the game folder (EACCES).

extern "C" FILE *bf_crt_fopen(const char *path, const char *mode)
{
	return bf_file_fopen(path, mode);
}

extern "C" int bf_crt_access(const char *path, int mode)
{
	bf_file_info info;
	if (!path || !bf_file_stat(path, &info)) {
		errno = ENOENT;
		return -1;
	}
	if ((mode & W_OK) && info.read_only) {
		errno = EACCES;
		return -1;
	}
	return 0;
}

extern "C" int bf_crt_remove(const char *path)
{
	bf_file_info info;
	if (path && bf_file_stat(path, &info) && info.is_folder)
		return bf_file_remove_folder(path);
	return bf_file_remove(path);
}

extern "C" int bf_crt_rename(const char *from, const char *to)
{
	return bf_file_rename(from, to);
}
