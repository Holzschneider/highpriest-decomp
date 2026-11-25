/*
 * The C library's file functions on the client's paths (step M3.1a, area
 * win32; included by bf_prefix.h, so every game unit sees it).
 *
 * The client opens files with fopen, tests them with access and makes
 * folders with _mkdir, passing its own path names: backslashes, any letter
 * case, CP949, relative to the game folder or below g_baseDir3 (which
 * GetCurrentDirectory filled with the game folder). In game units the names
 * below are macros for functions of port/compat/win32 that translate the path
 * with bf_file_resolve and then call the C library: reads find the file in
 * the game folder (or in a write location first), writes go to the write
 * locations and are refused in the game folder (EACCES).
 *
 * Port code (the adapters, compiled with BF_COMPAT_ADAPTER by
 * bf_compat_flags) and the native modules keep the C library's own names.
 *
 * The macros are object-like, so every use of the name in a game unit is
 * renamed the same way, the standard headers' too (std::remove, the
 * algorithm, becomes std::bf_crt_remove in declaration and call alike). This
 * header is included after <stdio.h>, <unistd.h> and <sys/stat.h>, so the C
 * library's own declarations keep their names.
 */
#ifndef BF_WIN32_CRT_H
#define BF_WIN32_CRT_H

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE *bf_crt_fopen(const char *path, const char *mode);
int   bf_crt_access(const char *path, int mode);
int   bf_crt_remove(const char *path);
int   bf_crt_rename(const char *from, const char *to);
int   bf_crt_unlink(const char *path);
int   bf_crt_rmdir(const char *path);
int   bf_crt_chmod(const char *path, int mode);

/* The Visual C++ names macOS lacks (also declared by direct.h). */
int   _mkdir(const char *path);
char *_getcwd(char *buffer, int size);

#ifdef __cplusplus
}
#endif

#ifndef _S_IREAD
#define _S_IREAD  0x0100
#define _S_IWRITE 0x0080
#endif

#if !defined(BF_COMPAT_ADAPTER)
#define fopen  bf_crt_fopen
#define access bf_crt_access
#define remove bf_crt_remove
#define rename bf_crt_rename
#define unlink bf_crt_unlink
#define rmdir  bf_crt_rmdir
#endif

/* As io.h has them; through the macros above in game units. */
#define _access access
#define _unlink unlink
#define _rmdir  rmdir
#define _chmod  bf_crt_chmod

#endif
