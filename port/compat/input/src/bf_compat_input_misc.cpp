// The rest of user32 the client calls: wsprintf, the code-page aware
// character stepping (on port/native/text) and SetRect.
#include "bf_compat_input_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bf_text.h"

namespace {

int CodePage(WORD cp)
{
	// CP_ACP, CP_OEMCP, CP_MACCP and CP_THREAD_ACP: the client's Windows-1252.
	return cp <= 3 ? BF_TEXT_CP_WESTERN : cp;
}

// wsprintf's format in C99 terms: its `l` is 32 bits (Windows' long), `h`
// before s/c means narrow, which C's printf has as plain; the rest is the same.
std::string Format(const char *fmt)
{
	std::string out;
	for (const char *p = fmt; *p; p++) {
		out += *p;
		if (*p != '%')
			continue;
		if (p[1] == '%') {
			out += *++p;
			continue;
		}
		p++;
		while (*p && strchr("-+ #0", *p))
			out += *p++;
		while (*p && ((*p >= '0' && *p <= '9') || *p == '.' || *p == '*'))
			out += *p++;
		if (*p == 'l' && p[1] != 'l')
			p++;                            // 32-bit: the argument is an int
		else if (*p == 'h' && (p[1] == 's' || p[1] == 'c'))
			p++;
		if (!*p)
			break;
		out += *p;
	}
	return out;
}

}  // namespace

extern "C" {

INT WINAPI wvsprintfA(LPSTR buffer, LPCSTR format, __ms_va_list args)
{
	if (!buffer || !format)
		return 0;
	// wsprintf writes at most 1024 characters and the NUL.
	int n = vsnprintf(buffer, 1025, Format(format).c_str(), args);
	if (n < 0) {
		buffer[0] = 0;
		return 0;
	}
	return n > 1024 ? 1024 : n;
}

INT WINAPIV wsprintfA(LPSTR buffer, LPCSTR format, ...)
{
	va_list args;
	va_start(args, format);
	INT n = wvsprintfA(buffer, format, args);
	va_end(args);
	return n;
}

LPSTR WINAPI CharNextExA(WORD codepage, LPCSTR text, DWORD flags)
{
	if (!text || !*text)
		return (LPSTR)text;
	return (LPSTR)(text + bf_text_char_length(CodePage(codepage), text, -1));
}

LPSTR WINAPI CharPrevExA(WORD codepage, LPCSTR start, LPCSTR current, DWORD flags)
{
	if (!start || !current || current <= start)
		return (LPSTR)start;
	return (LPSTR)(start + bf_text_char_prev(CodePage(codepage), start, (int)(current - start)));
}

LPSTR WINAPI CharNextA(LPCSTR text)
{
	return CharNextExA(0, text, 0);
}

LPSTR WINAPI CharPrevA(LPCSTR start, LPCSTR current)
{
	return CharPrevExA(0, start, current, 0);
}

BOOL WINAPI SetRect(LPRECT rect, INT left, INT top, INT right, INT bottom)
{
	if (!rect)
		return FALSE;
	rect->left = left;
	rect->top = top;
	rect->right = right;
	rect->bottom = bottom;
	return TRUE;
}

BOOL WINAPI CopyRect(LPRECT dst, const RECT *src)
{
	if (!dst || !src)
		return FALSE;
	*dst = *src;
	return TRUE;
}

}  // extern "C"
