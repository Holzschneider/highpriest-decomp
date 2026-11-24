/*
 * bf_file: the game folder, the write locations, and file access by the
 * client's own path names.
 *
 * A client path is what the client holds: bytes, backslashes or slashes,
 * relative to the game folder, in any letter case, with Korean names in
 * CP949. It is translated here:
 *
 *   - "\" and "/" both separate; empty and "." components are dropped,
 *     ".." goes up but never above the folder it is relative to;
 *   - a leading drive letter ("C:") or a leading separator is dropped: the
 *     path is relative to the game folder;
 *   - an absolute POSIX path that lies inside the game folder or a write
 *     location is taken as relative to it; any other absolute POSIX path is
 *     used as it is;
 *   - non-ASCII components are decoded as CP949 under the game folder and as
 *     the user code page (bf_file_set_user_codepage) under a write location,
 *     through CoreFoundation, and compared in NFC: the file system may hold
 *     either NFC or NFD;
 *   - every component is found case-insensitively; directory listings are
 *     cached and rescanned when a directory's modification time changes.
 *
 * The game folder is read-only. A client path is sent to a write location
 * when its first component is one the client writes (the table is in
 * README.md and bf_file_write_location): screenshots go to Pictures, chat
 * logs and reports to Documents, the client's other files to Application
 * Support. Reading such a path looks in the write location first and in the
 * game folder second. Opening any other path for writing fails: the
 * function returns its failure value, errno is EACCES, and a BFT_ERR trace
 * line names the path.
 *
 * Locations, relative to the home folder:
 *   game folder       bf_file_set_game_folder, else $BF_CLIENT_DIR, else ~/Games/<name>
 *   support           ~/Library/Application Support/<name>
 *   pictures          ~/Pictures/<name>
 *   documents         ~/Documents/<name>
 * <name> is BF_APP_NAME from port/name.cmake. $BF_HOME, when set, replaces the
 * home folder for the three write locations (tests write under the build
 * directory with it); the default game folder always uses the real home.
 *
 * Every function may be called from any thread. Paths returned as
 * `const char *` stay valid until the process ends.
 */
#ifndef BF_FILE_H
#define BF_FILE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Locations ---------------------------------------------------------- */

typedef enum bf_location {
	BF_LOCATION_GAME      = 0,   /* the read-only game folder */
	BF_LOCATION_SUPPORT   = 1,   /* ~/Library/Application Support/<name> */
	BF_LOCATION_PICTURES  = 2,   /* ~/Pictures/<name> */
	BF_LOCATION_DOCUMENTS = 3    /* ~/Documents/<name> */
} bf_location;

/*
 * The folder's absolute UTF-8 path, without a trailing slash. A write
 * location is created (with its parents) on first use; the game folder is
 * never created. NULL if a write location cannot be created.
 */
const char *bf_file_location(bf_location location);

/* The home folder in use for the write locations ($BF_HOME or the real one). */
const char *bf_file_home(void);

/*
 * Sets the game folder explicitly (an absolute UTF-8 path), overriding the
 * setting and the environment; NULL returns to them. Clears the directory
 * cache. Call before the first file access.
 */
void bf_file_set_game_folder(const char *path);

/*
 * The code page of names the client makes from user text (chat log titles,
 * character and server names) inside the write locations: 949 (default),
 * 1252, or 65001 for UTF-8. Names in the game folder are always CP949.
 */
void bf_file_set_user_codepage(unsigned codepage);

/*
 * Which location a client path's writes go to: BF_LOCATION_GAME when the
 * path is not one the client writes (a write to it is refused).
 */
bf_location bf_file_write_location(const char *client_path);

/* ---- Path translation ---------------------------------------------------- */

enum {
	BF_RESOLVE_READ  = 0,   /* the existing file or folder; fails if absent */
	BF_RESOLVE_WRITE = 1    /* the place to write; folders are created; refused in the game folder */
};

/*
 * Translates a client path to an absolute UTF-8 POSIX path in out.
 * Returns the length written, or -1: absent (ENOENT), refused (EACCES), out
 * too small (ENAMETOOLONG), or undecodable name (EILSEQ). This is what an
 * fopen or _mkdir adapter calls before the C library.
 */
int bf_file_resolve(const char *client_path, int mode, char *out, size_t out_size);

/* ---- Open files ---------------------------------------------------------- */

typedef struct bf_file bf_file;

enum {
	BF_FILE_READ      = 1 << 0,
	BF_FILE_WRITE     = 1 << 1,
	BF_FILE_CREATE    = 1 << 2,   /* create if absent (OPEN_ALWAYS) */
	BF_FILE_TRUNCATE  = 1 << 3,   /* with CREATE: CREATE_ALWAYS; alone: TRUNCATE_EXISTING */
	BF_FILE_EXCLUSIVE = 1 << 4,   /* with CREATE: fail if present (CREATE_NEW) */
	BF_FILE_APPEND    = 1 << 5,   /* every write goes to the end */
	BF_FILE_NATIVE    = 1 << 6    /* the path is already an absolute POSIX path: no translation
	                                 (writes into the game folder are still refused) */
};

/* Opens a file. Without WRITE the file must exist. NULL and errno on failure. */
bf_file *bf_file_open(const char *client_path, unsigned flags);

/* Bytes read or written (short at the end of the file), or -1. */
int64_t bf_file_read(bf_file *file, void *buffer, size_t size);
int64_t bf_file_write(bf_file *file, const void *buffer, size_t size);

/* whence: 0 = begin, 1 = current, 2 = end (as SEEK_* and FILE_BEGIN...). New position or -1. */
int64_t bf_file_seek(bf_file *file, int64_t offset, int whence);
int64_t bf_file_tell(bf_file *file);
int64_t bf_file_size(bf_file *file);

/* Moves the end of the file to the current position (SetEndOfFile). 0 or -1. */
int bf_file_truncate_here(bf_file *file);

/* The absolute POSIX path the file was opened at. */
const char *bf_file_path(bf_file *file);

void bf_file_close(bf_file *file);

/*
 * Reads a whole file into a buffer from malloc, followed by at least 8 zero
 * bytes (the client's parsers read past the end). NULL if absent; *size gets
 * the length without the padding. Free with bf_file_free.
 */
void *bf_file_read_all(const char *client_path, size_t *size);
void bf_file_free(void *data);

/* ---- Read-only mappings --------------------------------------------------- */

typedef struct bf_file_mapping bf_file_mapping;

/* Maps a whole file read-only (the GRF archives: CreateFileMapping + MapViewOfFile). */
bf_file_mapping *bf_file_map(const char *client_path);
/* Maps the file behind an open handle. The mapping outlives the handle. */
bf_file_mapping *bf_file_map_open_file(bf_file *file);
const void *bf_file_mapping_data(const bf_file_mapping *mapping);
uint64_t bf_file_mapping_size(const bf_file_mapping *mapping);
void bf_file_unmap(bf_file_mapping *mapping);

/* ---- Folders, existence, attributes ------------------------------------- */

typedef struct bf_file_info {
	char     name[260];        /* last component, in the client's bytes (CP949 or the user code page; '?' if unrepresentable) */
	char     name_utf8[1024];  /* the same in UTF-8 as on disk */
	uint64_t size;
	int64_t  modified;         /* seconds since 1970 */
	int      is_folder;
	int      read_only;        /* 1 for everything in the game folder */
} bf_file_info;

/* 1 if the path exists (file or folder) and info (may be NULL) is filled, 0 if not. */
int bf_file_stat(const char *client_path, bf_file_info *info);

typedef struct bf_file_find bf_file_find;

/*
 * Lists a folder: the last component of pattern may hold the wildcards "*"
 * and "?", matched without regard to case; "*.*" matches every name, as in
 * FindFirstFile. "." and ".." are never returned. A pattern without
 * wildcards finds that one entry. NULL if nothing matches (ENOENT).
 */
bf_file_find *bf_file_find_first(const char *client_pattern, bf_file_info *info);
/* 1 and info filled, or 0 when there is nothing more. */
int bf_file_find_next(bf_file_find *find, bf_file_info *info);
void bf_file_find_close(bf_file_find *find);

/* Creates a folder (with parents) in a write location: 1 created, 0 existed, -1 error. */
int bf_file_make_folder(const char *client_path);
/* Removes a file in a write location. 0 or -1. */
int bf_file_remove(const char *client_path);
/* Renames within the write locations. 0 or -1. */
int bf_file_rename(const char *from_client_path, const char *to_client_path);

/* Forgets every cached directory listing. */
void bf_file_flush_cache(void);

#ifdef __cplusplus
}
#endif

#endif
