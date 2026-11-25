/*
 * bf_clock: the one clock of the process.
 *
 * Nothing else in the port reads the time. Every timeGetTime, GetTickCount,
 * QueryPerformanceCounter, Sleep and GetLocalTime of the 2008 client ends
 * here; since M11.1 the client calls these functions directly.
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

/* Milliseconds since start, wrapping at 2^32. */
uint32_t bf_clock_ms(void);

/*
 * The client's tick count (its timeGetTime and GetTickCount): bf_clock_ms()
 * plus one hour, wrapping at 2^32 (after 48.7 more days). A system clock
 * never starts near 0, and the client subtracts its expiry times from it
 * unsigned (the sprite cache's EXPIRETIME / 4 = 3750 ms in
 * CRenderer::AddSpriteIndex), which on a clock starting at 0 gives 0 for the
 * first 3750 ms and then never finds a free slot. Follows an installed source.
 */
#define BF_CLOCK_TICKS_AT_START (3600u * 1000u)
uint32_t bf_clock_ticks(void);

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

/*
 * The calendar, UTC, of a moment given in milliseconds since 1970 (negative
 * before): what FileTimeToSystemTime gives for a time on the wire. 0 when the
 * system cannot represent the year (out is then left as it was), else 1.
 */
int bf_clock_utc_date(int64_t unix_ms, bf_clock_date *out);

#ifdef __cplusplus
}
#endif

#endif
