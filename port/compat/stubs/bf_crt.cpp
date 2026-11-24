// The Visual C++ runtime names the client calls that the C library of macOS
// does not have (M3.2). Declared by bf_prefix.h (itoa, _ltoa, strlwr, ...)
// and process.h (spawnl). These are the real functions, except spawnl,
// whose two callers start Windows programs (runzip.exe for skins,
// findhack.exe for GameGuard) and get "not started".
#include <windows.h>
#include <process.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

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

intptr_t spawnl(int mode, const char *path, const char *arg0, ...)
{
	(void)mode;
	(void)arg0;
	BF_STUB();
	bf_tracef(BFT_W32, "spawnl(%s): Windows programs are not started", path ? path : "");
	errno = ENOENT;
	return -1;
}
