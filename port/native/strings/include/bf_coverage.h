/*
 * Coverage of the English client during a session (M9.5).
 *
 * Three things are collected while the client runs, each entry once per
 * distinct text, with how often it happened and where:
 *
 *   untranslated  a bf_tr() lookup that fell back to the Korean original
 *                 (bf_strings.h); `where` is the caller of bf_tr.
 *   placeholder   a table lookup that fell back to a placeholder name
 *                 (`?<id>` items and jobs, a message id the table lacks, a
 *                 map with no display name, ...), reported by the client
 *                 through bf_coverage_placeholder.
 *   korean        text the client draws (the gdi adapter's TextOut) that is
 *                 Korean: CP949 bytes that decode as Hangul, whether the font
 *                 draws them as Hangul (code page 949) or as Windows-1252
 *                 mojibake (code page 1252, "°¡").
 *   foreign       drawn text with characters Windows-1252 does not have and
 *                 that are not Hangul (CJK, symbols of a Korean font).
 *
 * A session whose report has no entry is the M9 criterion. The report is
 * written as JSON by bf_coverage_write (the client does so at exit when
 * BF_COVERAGE names a file) and read through bf_coverage_each (the control
 * socket's `coverage` part).
 *
 * Threads: any. No Windows types, no client headers; C header usable from
 * C++14.
 */
#ifndef BF_COVERAGE_H
#define BF_COVERAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* bf_coverage_drawn's code page for UTF-16 text (count in 16-bit units). */
enum { BF_COVERAGE_UTF16 = 1200 };

/* What a drawn text is: 0 Windows-1252 English (or empty), 1 Korean, 2 foreign. */
enum { BF_COVERAGE_ENGLISH = 0, BF_COVERAGE_KOREAN = 1, BF_COVERAGE_FOREIGN = 2 };

/*
 * Classifies a text as the client hands it to a drawing call. codepage is the
 * one the bytes are drawn in (949, 1252, any other byte code page), or
 * BF_COVERAGE_UTF16. Bytes are Korean when every byte >= 0x80 is part of a
 * CP949 pair and the pairs hold Hangul (one KS X 1001 syllable or jamo, or two
 * of the UHC extension's syllables, whose trail bytes can be ASCII letters).
 * UTF-16 is Korean when it holds Hangul; otherwise, when every unit is a
 * Windows-1252 character, its bytes are tested as above (Korean bytes drawn
 * through a Western font). count < 0: up to the terminating zero.
 */
int bf_coverage_classify(int codepage, const void *text, int count);

/*
 * The drawing hook: classifies the text and records it if it is not English.
 * The first time a distinct text is seen, the call stack is read for `where`
 * (the first frames outside the drawing helpers, as symbol+offset). ASCII text
 * returns at once. Returns the class.
 */
int bf_coverage_drawn(int codepage, const void *text, int count);

/*
 * A table lookup that fell back to a placeholder. table names the table
 * (`item`, `job`, `skill`, `msg`, `map`, ...); what says which entry and what
 * was shown instead (UTF-8 or Windows-1252 English). Recorded once per
 * (table, what); repeated calls count.
 */
void bf_coverage_placeholder(const char *table, const char *what);

typedef struct {
	const char *kind;    /* "untranslated", "placeholder", "korean", "foreign" */
	const char *text;    /* UTF-8: the Korean text decoded from CP949, the placeholder's description */
	const char *drawn;   /* UTF-8: what was drawn (the mojibake), or NULL when it equals text */
	const char *hex;     /* the original bytes in hex (CP949), or "" */
	const char *table;   /* placeholders: the table; else NULL */
	const char *where;   /* call site: "symbol+0x12 < symbol+0x40 ...", or "" */
	long long count;
} bf_coverage_entry;

/*
 * Calls fn for every entry, in a stable order (kind, then text). Includes the
 * bf_tr fallbacks of bf_strings.h. fn must not call back into this module.
 * Returns the number of entries.
 */
int bf_coverage_each(void (*fn)(const bf_coverage_entry *entry, void *user), void *user);

/* The number of entries (0: the session is covered). */
int bf_coverage_count(void);

/*
 * Writes the report as JSON:
 *   {"entries": N, "untranslated": n, "placeholder": n, "korean": n, "foreign": n,
 *    "strings": {"lookups", "hits", "fallbacks", "entries", "translated"},
 *    "list": [{"kind", "text", "drawn", "hex", "table", "where", "count"}, ...]}
 * Returns 0, or -1 when the file cannot be written (traced).
 */
int bf_coverage_write(const char *path);

/* Forgets every entry (and the bf_tr fallbacks: bf_strings_reset_counts). */
void bf_coverage_reset(void);

#ifdef __cplusplus
}
#endif

#endif
