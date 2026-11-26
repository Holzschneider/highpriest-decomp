/*
 * Instrumentation for the macOS port: categorised tracing, packet capture,
 * event counters and a record of compatibility functions that are still
 * stubs. It depends on the C library only, so game code, the compatibility
 * layer and standalone probes can all include it.
 *
 * Everything is off until the environment turns it on:
 *
 *   BF_TRACE=net,pkt,file     categories to print (names below; "all")
 *   BF_TRACE_FILE=path        write the trace there instead of stderr
 *   BF_PKT_LOG=path           append every packet as one text line
 *   BF_ASSERT=abort           stop in the debugger on a failed sAssert
 *
 * A second sink, the rolling trace file, is opened by the program itself
 * (bf_trace_open_rolling; the client's debug build does, bf_main.cpp).
 *
 * A disabled BF_TRACE costs one load and one branch.
 */
#ifndef BF_TRACE_H
#define BF_TRACE_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BFT_ERR   = 1 << 0,   /* ErrorMsg, failed asserts; on by default */
	BFT_GAME  = 1 << 1,   /* the game's own Trace() calls */
	BFT_NET   = 1 << 2,   /* connect, disconnect, socket errors */
	BFT_PKT   = 1 << 3,   /* one line per packet sent or received */
	BFT_FILE  = 1 << 4,   /* file lookups: archive or folder, found or not */
	BFT_RES   = 1 << 5,   /* resource manager loads */
	BFT_GFX   = 1 << 6,   /* device, textures, frames */
	BFT_SND   = 1 << 7,
	BFT_INPUT = 1 << 8,
	BFT_UI    = 1 << 9,
	BFT_MODE  = 1 << 10,  /* mode and sub-mode changes */
	BFT_W32   = 1 << 11,  /* calls into the Win32 compatibility layer */
	BFT_STUB  = 1 << 12   /* first call of each unimplemented function; on by default */
};

/* The categories any sink prints: the stream's (BF_TRACE) and the rolling file's. */
extern unsigned bf_trace_mask;

/* Reads the environment. Called on first use; calling it again re-reads. */
void bf_trace_init(void);

void bf_tracef(unsigned category, const char *format, ...)
#if defined(__GNUC__)
	__attribute__((format(printf, 2, 3)))
#endif
	;
void bf_tracev(unsigned category, const char *format, va_list args);

/* A hex dump of at most 256 bytes. */
void bf_trace_hex(unsigned category, const char *label, const void *data, int size);

/*
 * One packet. direction is 'S' or 'R'; the first two bytes are the header.
 * Writes an BFT_PKT line and, with BF_PKT_LOG set, a capture line:
 *   <milliseconds> <S|R> <header> <size> <name> <hex bytes> [<connection>]
 */
void bf_trace_packet(char direction, const void *data, int size);

/*
 * The same with a name and a connection. name NULL takes the packet namer's;
 * "raw" marks bytes read outside the packet framing (the account id the
 * character and zone servers send first). connection, when not NULL, is
 * written as a seventh field: the server role and the connection's serial in
 * the process, "account/1", "char/2", "zone/3" (device/Connection.cpp).
 * Readers of the six-field format can ignore it; BF_PKT_REPLAY uses it to
 * find where each connection begins.
 */
void bf_trace_packet_ex(char direction, const void *data, int size, const char *name, const char *connection);

/* Supplies names for packet headers (see port/tools/gen_packet_names.py). */
void bf_trace_set_packet_namer(const char *(*namer)(int header));

/* Counts an event under a name; bf_trace_dump_counters prints the totals. */
void bf_count(const char *name, long long amount);
/* The totals, to the stream (stderr or BF_TRACE_FILE) and the rolling file. */
void bf_trace_dump_counters(void);
/*
 * The same at an orderly exit: to the stream only when BF_TRACE asked for a
 * category beyond err and stub (as before the rolling file existed), and to
 * the rolling file when one is open.
 */
void bf_trace_dump_counters_at_exit(void);

/*
 * The rolling trace file: a sink beside the stream, with its own categories
 * (mask; bf_trace_mask becomes the union). Each call opens
 * <folder>/trace-<YYYYmmdd-HHMMSS>-<pid>.txt (local time); when a file
 * reaches max_bytes, it is closed and the next part,
 * trace-<YYYYmmdd-HHMMSS>-<pid>-<n>.txt, opened. After each open only the
 * newest keep files named trace-*.txt in folder are left (by modification
 * time, then name); the others are deleted. folder must exist. Returns the
 * path of the file opened, valid until the next call, or NULL with a BFT_ERR
 * line on the stream. bf_trace_init leaves it open and keeps its categories.
 */
const char *bf_trace_open_rolling(const char *folder, unsigned mask, int keep, long long max_bytes);

/* Reports an unimplemented function once per call site. */
void bf_stub(const char *function, const char *file, int line);

/* Reports a failed assertion; stops in the debugger with BF_ASSERT=abort. */
void bf_assert_failed(const char *expression, const char *message, const char *file, int line);

#ifdef __cplusplus
}
#endif

#define BF_TRACE(category, ...) \
	do { if (bf_trace_mask & (category)) bf_tracef((category), __VA_ARGS__); } while (0)

#define BF_STUB() bf_stub(__func__, __FILE__, __LINE__)

#endif
