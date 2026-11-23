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
   game's socket calls go to BSD sockets through compat/include/winsock.h. */
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

/* Names the DirectX 7 SDK gives its enumerations and the Wine headers do not. */
#define _D3DPRIMITIVETYPE D3DPRIMITIVETYPE

#endif
