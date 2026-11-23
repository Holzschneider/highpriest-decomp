/*
 * Instrumentation for the macOS port: categorised tracing, packet capture,
 * event counters and a record of compatibility functions that are still
 * stubs. It depends on the C library only, so game code, the compatibility
 * layer and standalone probes can all include it.
 *
 * Everything is off until the environment turns it on:
 *
 *   HP_TRACE=net,pkt,file     categories to print (names below; "all")
 *   HP_TRACE_FILE=path        write the trace there instead of stderr
 *   HP_PKT_LOG=path           append every packet as one text line
 *   HP_ASSERT=abort           stop in the debugger on a failed sAssert
 *
 * A disabled HP_TRACE costs one load and one branch.
 */
#ifndef HP_TRACE_H
#define HP_TRACE_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	HPT_ERR   = 1 << 0,   /* ErrorMsg, failed asserts; on by default */
	HPT_GAME  = 1 << 1,   /* the game's own Trace() calls */
	HPT_NET   = 1 << 2,   /* connect, disconnect, socket errors */
	HPT_PKT   = 1 << 3,   /* one line per packet sent or received */
	HPT_FILE  = 1 << 4,   /* file lookups: archive or folder, found or not */
	HPT_RES   = 1 << 5,   /* resource manager loads */
	HPT_GFX   = 1 << 6,   /* device, textures, frames */
	HPT_SND   = 1 << 7,
	HPT_INPUT = 1 << 8,
	HPT_UI    = 1 << 9,
	HPT_MODE  = 1 << 10,  /* mode and sub-mode changes */
	HPT_W32   = 1 << 11,  /* calls into the Win32 compatibility layer */
	HPT_STUB  = 1 << 12   /* first call of each unimplemented function; on by default */
};

extern unsigned hp_trace_mask;

/* Reads the environment. Called on first use; calling it again re-reads. */
void hp_trace_init(void);

void hp_tracef(unsigned category, const char *format, ...)
#if defined(__GNUC__)
	__attribute__((format(printf, 2, 3)))
#endif
	;
void hp_tracev(unsigned category, const char *format, va_list args);

/* A hex dump of at most 256 bytes. */
void hp_trace_hex(unsigned category, const char *label, const void *data, int size);

/*
 * One packet. direction is 'S' or 'R'; the first two bytes are the header.
 * Writes an HPT_PKT line and, with HP_PKT_LOG set, a capture line:
 *   <milliseconds> <S|R> <header> <size> <name> <hex bytes>
 */
void hp_trace_packet(char direction, const void *data, int size);

/* Supplies names for packet headers (see port/tools/gen_packet_names.py). */
void hp_trace_set_packet_namer(const char *(*namer)(int header));

/* Counts an event under a name; hp_trace_dump_counters prints the totals. */
void hp_count(const char *name, long long amount);
void hp_trace_dump_counters(void);

/* Reports an unimplemented function once per call site. */
void hp_stub(const char *function, const char *file, int line);

/* Reports a failed assertion; stops in the debugger with HP_ASSERT=abort. */
void hp_assert_failed(const char *expression, const char *message, const char *file, int line);

#ifdef __cplusplus
}
#endif

#define HP_TRACE(category, ...) \
	do { if (hp_trace_mask & (category)) hp_tracef((category), __VA_ARGS__); } while (0)

#define HP_STUB() hp_stub(__func__, __FILE__, __LINE__)

#endif
