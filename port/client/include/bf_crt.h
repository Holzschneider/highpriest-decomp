/*
 * The C library's file functions on the client's paths (step M3.1a; included
 * by bf_prefix.h, so every game unit sees it).
 *
 * The client opens files with fopen, tests them with access and removes and
 * renames them (Lua's os.remove and os.rename among the callers), passing its
 * own path names: backslashes, any letter case, CP949, relative to the game
 * folder or below g_baseDir3 (the game folder). In game units these names are
 * macros for functions of port/client/src/bf_crt.cpp on port/native/sys
 * (bf_file.h): reads find the file in the game folder (or in a write location
 * first), writes go to the write locations and are refused in the game folder
 * (EACCES).
 *
 * A call site that leaves these names calls bf_file.h itself (bf_file_fopen,
 * bf_file_stat for access, bf_file_make_folder, bf_file_remove,
 * bf_file_remove_folder, bf_file_rename), as base/File.cpp, base/GPak.cpp,
 * base/Util.cpp and skin.cpp do since M11.3; M11.11 dropped the names no
 * compiled call used any more (unlink, rmdir, _mkdir, _getcwd, _chmod and
 * their io.h spellings).
 *
 * Port code (compiled with BF_PORT_CODE by bf_port_flags) and the native
 * modules keep the C library's own names.
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

#ifdef __cplusplus
}
#endif

#if !defined(BF_PORT_CODE)
#define fopen  bf_crt_fopen
#define access bf_crt_access
#define remove bf_crt_remove
#define rename bf_crt_rename
#endif

#endif
