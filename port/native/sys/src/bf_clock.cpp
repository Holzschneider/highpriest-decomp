// The one clock of the process: mach_absolute_time, or an installed source.

#include "bf_clock.h"

#include <mach/mach_time.h>
#include <sched.h>
#include <sys/time.h>
#include <time.h>

#include <atomic>
#include <mutex>

namespace {

struct RealClock {
	uint64_t start;
	mach_timebase_info_data_t timebase;
	RealClock()
	{
		mach_timebase_info(&timebase);
		start = mach_absolute_time();
	}
	uint64_t Microseconds() const
	{
		uint64_t ticks = mach_absolute_time() - start;
		// numer/denom converts ticks to nanoseconds; split to avoid overflow.
		uint64_t ns = (ticks / timebase.denom) * timebase.numer
		            + (ticks % timebase.denom) * timebase.numer / timebase.denom;
		return ns / 1000;
	}
};

RealClock &Real()
{
	static RealClock clock;
	return clock;
}

// Initialised when the module is loaded, so "since start" means process start.
const RealClock &g_startup = Real();

std::mutex g_sourceLock;
bf_clock_source g_source;
std::atomic<bool> g_hasSource(false);

void RealSleep(uint64_t us)
{
	if (us == 0) {
		sched_yield();
		return;
	}
	struct timespec request, remaining;
	request.tv_sec = (time_t)(us / 1000000);
	request.tv_nsec = (long)(us % 1000000) * 1000;
	while (nanosleep(&request, &remaining) != 0)
		request = remaining;
}

void Fill(bf_clock_date *out, const struct tm &t, int ms)
{
	out->year = t.tm_year + 1900;
	out->month = t.tm_mon + 1;
	out->weekday = t.tm_wday;
	out->day = t.tm_mday;
	out->hour = t.tm_hour;
	out->minute = t.tm_min;
	out->second = t.tm_sec;
	out->millisecond = ms;
}

} // namespace

extern "C" uint64_t bf_clock_us(void)
{
	(void)g_startup;
	if (g_hasSource.load(std::memory_order_acquire)) {
		std::lock_guard<std::mutex> lock(g_sourceLock);
		if (g_hasSource.load(std::memory_order_relaxed))
			return g_source.now_us(g_source.context);
	}
	return Real().Microseconds();
}

extern "C" uint32_t bf_clock_ms(void)
{
	return (uint32_t)(bf_clock_us() / 1000);
}

extern "C" uint32_t bf_clock_ticks(void)
{
	return bf_clock_ms() + BF_CLOCK_TICKS_AT_START;
}

extern "C" void bf_clock_sleep_us(uint64_t us)
{
	if (g_hasSource.load(std::memory_order_acquire)) {
		bf_clock_source source;
		{
			std::lock_guard<std::mutex> lock(g_sourceLock);
			source = g_source;
		}
		if (g_hasSource.load(std::memory_order_relaxed)) {
			if (source.sleep_us)
				source.sleep_us(source.context, us);
			return;
		}
	}
	RealSleep(us);
}

extern "C" void bf_clock_sleep_ms(uint32_t ms)
{
	bf_clock_sleep_us((uint64_t)ms * 1000);
}

extern "C" void bf_clock_set_source(const bf_clock_source *source)
{
	std::lock_guard<std::mutex> lock(g_sourceLock);
	if (source && source->now_us) {
		g_source = *source;
		g_hasSource.store(true, std::memory_order_release);
	} else {
		g_hasSource.store(false, std::memory_order_release);
	}
}

extern "C" void bf_clock_local_time(bf_clock_date *out)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	struct tm t;
	localtime_r(&now.tv_sec, &t);
	Fill(out, t, (int)(now.tv_usec / 1000));
}

extern "C" void bf_clock_utc_time(bf_clock_date *out)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	struct tm t;
	gmtime_r(&now.tv_sec, &t);
	Fill(out, t, (int)(now.tv_usec / 1000));
}
