/*
 * Lookup of the client's displayed Korean literals (M9.2, technical plan 3.5).
 *
 * About 70 texts in the client's code bypass msgStringTable.txt: billing notices,
 * pet states, name-balloon brackets, error boxes. The source keeps the original
 * literal, a CP949 byte string; the step that wires this module later changes
 * each of those sites (port/data/strings/callsites.tsv) from
 *
 *     sprintf(buffer, "<korean>", ...)      to      sprintf(buffer, bf_tr("<korean>"), ...)
 *
 * bf_tr() returns the English text (Windows-1252 bytes) the table holds for those
 * exact original bytes, or the original itself when there is none, so a missing
 * translation shows Korean and never breaks anything.
 *
 * The table is a text file, port/data/strings/translation.tsv, copied by
 * port/data/assemble.py into the game folder's data/ as bf_strings.tsv, and read
 * at run time: a translation changes without a rebuild. Format: one line per
 * original, "<original bytes in hex> TAB <English> TAB <anything>"; '#' lines
 * are comments; the file is UTF-8, the English column holds characters of
 * Windows-1252 only and is converted when loaded; \\, \n, \t, \r are escapes.
 * An empty English column is "not translated yet".
 *
 * Every fallback is counted (counter "strings.fallback" through bf_count, a
 * BFT_UI trace line the first time each distinct original falls back, and the
 * statistics below) for the coverage report of M9.5.
 *
 * Threads: any. Strings returned by bf_tr stay valid until the next
 * bf_strings_load or bf_strings_clear.
 *
 * No Windows types, no client headers; C header usable from C++14.
 */
#ifndef BF_STRINGS_H
#define BF_STRINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Reads the table at path (replacing any table loaded before; counters stay).
 * Returns the number of originals the file lists (translated or not), or -1 when
 * the file cannot be read (a BFT_ERR trace line says why; lookups then fall back).
 * Lines that cannot be used (bad hex, text outside Windows-1252) are skipped with
 * a BFT_ERR line; the English of such a line counts as missing.
 */
int bf_strings_load(const char *path);

/* Drops the table and forgets that a load was attempted. Counters stay. */
void bf_strings_clear(void);

/*
 * The translation of original (a CP949 byte string, as it is in the source), or
 * original itself. NULL gives NULL. A string with no byte >= 0x80 is returned
 * as it is and counts for nothing: such strings are never keys.
 *
 * If no table was loaded yet, the first call loads $BF_STRINGS_FILE, else
 * $BF_CLIENT_DIR/data/bf_strings.tsv (or data/bf_strings.tsv when BF_CLIENT_DIR
 * is not set) and traces once if there is none.
 */
const char *bf_tr(const char *original);

typedef struct {
	long long lookups;            /* bf_tr calls on strings that could be keys */
	long long hits;               /* answered with a translation */
	long long fallbacks;          /* answered with the original */
	int       entries;            /* originals the loaded table lists */
	int       translated;         /* of them, with an English text */
	int       distinct_fallbacks; /* different originals that fell back */
} bf_strings_stats;

void bf_strings_get_stats(bf_strings_stats *out);

/*
 * Calls fn once per original that fell back, with how often. Returns how many.
 * fn must not call back into this module.
 */
int bf_strings_each_fallback(void (*fn)(const char *original, long long count, void *user), void *user);

/* Zeroes the counters and forgets the fallbacks (start of a measured session). */
void bf_strings_reset_counts(void);

#ifdef __cplusplus
}
#endif

#endif
