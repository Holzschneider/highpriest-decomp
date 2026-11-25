/*
 * Included ahead of every game source in the macOS build (-include).
 * It supplies what Visual C++ 6.0 and its C runtime provided implicitly and
 * the Windows headers do not declare.
 */
#ifndef BF_PREFIX_H
#define BF_PREFIX_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* The Windows headers must not pull in their own Winsock declarations: the
   game's socket calls are BSD sockets (<sys/socket.h>, M11.2). */
#define _WINSOCKAPI_
#define _WINSOCK2API_
#define __WINE_WINSOCKAPI_STDLIB_H

/* Calling conventions do not exist on arm64. */
#define __stdcall
#define __cdecl
#define __fastcall
#define _stdcall
#define _cdecl
#define __export

#define __int64 long long
#define _int64 long long
#define __forceinline inline
#define __inline inline
#define _inline inline

#define _MAX_PATH 260
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strcmpi strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _strdup strdup

#ifdef __cplusplus
extern "C" {
#endif
char *itoa(int value, char *buffer, int radix);
char *_itoa(int value, char *buffer, int radix);
char *_ltoa(int value, char *buffer, int radix);
char *strlwr(char *text);
char *strupr(char *text);
char *_strlwr(char *text);
char *_strupr(char *text);
unsigned int _lrotl(unsigned int value, int shift);
unsigned int _lrotr(unsigned int value, int shift);
unsigned int _controlfp(unsigned int value, unsigned int mask);
#ifdef __cplusplus
}
#endif

/* The one clock (port/native/sys): bf_clock_ticks is the client's tick
   count, bf_clock_sleep_ms its sleep (M11.1). */
#include "bf_clock.h"

/* The C library's file names on client paths: game folder, write locations
   (bf_crt.h, port/client/src/bf_crt.cpp). */
#include "bf_crt.h"

#ifdef __cplusplus
/* time_t is 32 bits in Visual C++ 6.0 and 64 bits here. The structure fields
   that travel on the wire stay 32-bit int (M2.2); these overloads let the
   game's calls on them keep their shape: the C library is called with a
   64-bit copy and the 32-bit value is what the caller sees. The value is
   moved byte-wise: the fields sit in packed packet structures (MAIL_LIST's
   DeleteTime at offset 69), where an int load is misaligned. */
#include <time.h>
#include <type_traits>
template <class T> inline typename std::enable_if<std::is_same<T, int>::value, time_t>::type time(T *value) { time_t now; ::time(&now); if (value) { int v = (int)now; __builtin_memcpy((void *)value, &v, sizeof v); } return now; }
template <class T> inline typename std::enable_if<std::is_same<typename std::remove_const<T>::type, int>::value, struct tm *>::type localtime(T *value) { static time_t copy; int v; __builtin_memcpy(&v, (const void *)value, sizeof v); copy = (time_t)v; return ::localtime(&copy); }
#endif

/* Names the DirectX 7 SDK gives its enumerations and the Wine headers do not. */
#define _D3DPRIMITIVETYPE D3DPRIMITIVETYPE

#endif
