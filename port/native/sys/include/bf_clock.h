/*
 * bf_clock: the one clock of the process.
 *
 * Nothing else in the port reads the time. Every timeGetTime, GetTickCount,
 * QueryPerformanceCounter, Sleep and GetLocalTime of the client ends here,
 * through the adapters in port/compat and later directly.
 *
 * The real source is mach_absolute_time, counted from the moment the module
 * is loaded. A replacement source can be installed (bf_clock_set_source) so
 * that an agent can advance the client by fixed logic steps: the elapsed
 * clock and the sleeps then follow that source, and nothing else changes.
 * The calendar time (bf_clock_local_time) always comes from the system.
 *
 * Every function may be called from any thread.
 */
#ifndef BF_CLOCK_H
#define BF_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Microseconds since the process started (or since the installed source's zero). */
uint64_t bf_clock_us(void);

/* Milliseconds since start, wrapping at 2^32 like timeGetTime. */
uint32_t bf_clock_ms(void);

/*
 * Blocks the calling thread for at least ms milliseconds (or microseconds).
 * 0 yields the processor, as Sleep(0) does. With a source installed, the
 * source's sleep is called instead (it may return at once).
 */
void bf_clock_sleep_ms(uint32_t ms);
void bf_clock_sleep_us(uint64_t us);

/*
 * A replacement clock. now_us must not go backwards; sleep_us may be NULL,
 * in which case a sleep returns at once. The structure is copied.
 * Passing NULL restores the real clock. Install it before other threads
 * start reading the clock if their readings must agree from the start.
 */
typedef struct bf_clock_source {
	uint64_t (*now_us)(void *context);
	void     (*sleep_us)(void *context, uint64_t us);
	void     *context;
} bf_clock_source;

void bf_clock_set_source(const bf_clock_source *source);

/* The calendar, with the fields of a Win32 SYSTEMTIME. */
typedef struct bf_clock_date {
	int year;          /* e.g. 2026 */
	int month;         /* 1-12 */
	int weekday;       /* 0 = Sunday */
	int day;           /* 1-31 */
	int hour, minute, second, millisecond;
} bf_clock_date;

void bf_clock_local_time(bf_clock_date *out);   /* GetLocalTime */
void bf_clock_utc_time(bf_clock_date *out);     /* GetSystemTime */

#ifdef __cplusplus
}
#endif

#endif
