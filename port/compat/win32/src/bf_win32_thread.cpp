// Time (kernel32, winmm) on bf_clock; threads and critical sections on
// bf_sys. InterlockedIncrement and the other Interlocked* functions are the
// Wine headers' inline functions (compiler atomics on the LONG).
#include "bf_win32_internal.h"

#include <mmsystem.h>
#include <process.h>

#include <atomic>
#include <string.h>
#include <time.h>

using namespace bf_win32;

// ---- Time -----------------------------------------------------------------------

DWORD WINAPI timeGetTime(void)
{
	return bf_clock_ms();
}

DWORD WINAPI GetTickCount(void)
{
	return bf_clock_ms();
}

MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS caps, UINT size)
{
	if (!caps || size < sizeof(TIMECAPS))
		return TIMERR_NOCANDO;
	caps->wPeriodMin = 1;
	caps->wPeriodMax = 1000000;
	return TIMERR_NOERROR;
}

// The clock has a resolution far below a millisecond already.
MMRESULT WINAPI timeBeginPeriod(UINT period)
{
	return period ? TIMERR_NOERROR : TIMERR_NOCANDO;
}

MMRESULT WINAPI timeEndPeriod(UINT period)
{
	return period ? TIMERR_NOERROR : TIMERR_NOCANDO;
}

BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *frequency)
{
	if (!frequency)
		return FALSE;
	frequency->QuadPart = 1000000;
	return TRUE;
}

BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *counter)
{
	if (!counter)
		return FALSE;
	counter->QuadPart = (LONGLONG)bf_clock_us();
	return TRUE;
}

VOID WINAPI Sleep(DWORD ms)
{
	bf_clock_sleep_ms(ms);
}

namespace {
void ToSystemTime(const bf_clock_date &d, LPSYSTEMTIME out)
{
	out->wYear = (WORD)d.year;
	out->wMonth = (WORD)d.month;
	out->wDayOfWeek = (WORD)d.weekday;
	out->wDay = (WORD)d.day;
	out->wHour = (WORD)d.hour;
	out->wMinute = (WORD)d.minute;
	out->wSecond = (WORD)d.second;
	out->wMilliseconds = (WORD)d.millisecond;
}
}  // namespace

VOID WINAPI GetLocalTime(LPSYSTEMTIME out)
{
	if (!out)
		return;
	bf_clock_date d;
	bf_clock_local_time(&d);
	ToSystemTime(d, out);
}

VOID WINAPI GetSystemTime(LPSYSTEMTIME out)
{
	if (!out)
		return;
	bf_clock_date d;
	bf_clock_utc_time(&d);
	ToSystemTime(d, out);
}

// FILETIME (100 ns since 1601, UTC) to the calendar, UTC.
BOOL WINAPI FileTimeToSystemTime(const FILETIME *file_time, LPSYSTEMTIME out)
{
	if (!file_time || !out) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	uint64_t t = ((uint64_t)file_time->dwHighDateTime << 32) | file_time->dwLowDateTime;
	if (t >= 0x8000000000000000ULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	time_t seconds = (time_t)(t / 10000000ULL) - (time_t)11644473600LL;
	struct tm tm;
	if (!gmtime_r(&seconds, &tm)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	out->wYear = (WORD)(tm.tm_year + 1900);
	out->wMonth = (WORD)(tm.tm_mon + 1);
	out->wDayOfWeek = (WORD)tm.tm_wday;
	out->wDay = (WORD)tm.tm_mday;
	out->wHour = (WORD)tm.tm_hour;
	out->wMinute = (WORD)tm.tm_min;
	out->wSecond = (WORD)tm.tm_sec;
	out->wMilliseconds = (WORD)((t / 10000ULL) % 1000ULL);
	return TRUE;
}

// ---- Threads ------------------------------------------------------------------
//
// A thread handle is a registered object holding the bf_sys thread. Thread
// ids come from one counter, for the threads made here (given before they
// start, as CreateThread reports them) and for the others on first ask.

namespace {

std::atomic<DWORD> g_nextThreadId(1);
thread_local DWORD t_threadId = 0;

struct Thread : Object {
	Thread() : Object(kThread) {}
	~Thread() override { bf_sys_thread_release(thread); }
	bf_sys_thread *thread = nullptr;
	DWORD id = 0;
};

struct Start {
	LPTHREAD_START_ROUTINE routine;
	unsigned (*crt_routine)(void *);
	void *argument;
	DWORD id;
};

uint32_t Trampoline(void *p)
{
	Start start = *(Start *)p;
	delete (Start *)p;
	t_threadId = start.id;
	return start.routine ? (uint32_t)start.routine(start.argument) : (uint32_t)start.crt_routine(start.argument);
}

HANDLE StartThread(LPTHREAD_START_ROUTINE routine, unsigned (*crt_routine)(void *), void *argument, DWORD flags,
                   DWORD *id_out)
{
	if (flags & CREATE_SUSPENDED) {
		BF_STUB();   // never used by the client: the thread starts at once
	}
	Start *start = new Start{routine, crt_routine, argument, g_nextThreadId++};
	DWORD id = start->id;
	bf_sys_thread *thread = bf_sys_thread_create(Trampoline, start, NULL);
	if (!thread) {
		delete start;
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	Thread *object = new Thread;
	object->thread = thread;
	object->id = id;
	if (id_out)
		*id_out = id;
	BF_TRACE(BFT_W32, "thread %u started", (unsigned)id);
	return Register(object);
}

}  // namespace

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack, LPTHREAD_START_ROUTINE routine,
                           LPVOID argument, DWORD flags, LPDWORD id)
{
	(void)attributes;
	(void)stack;
	if (!routine) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	return StartThread(routine, NULL, argument, flags, id);
}

uintptr_t _beginthreadex(void *security, unsigned stack, unsigned (*routine)(void *), void *argument,
                         unsigned flags, unsigned *id)
{
	(void)security;
	(void)stack;
	if (!routine) {
		errno = EINVAL;
		return 0;
	}
	DWORD tid = 0;
	HANDLE handle = StartThread(NULL, routine, argument, flags, &tid);
	if (id)
		*id = tid;
	return (uintptr_t)handle;
}

DWORD WINAPI WaitForSingleObject(HANDLE handle, DWORD ms)
{
	Thread *thread = (Thread *)Lookup(handle, kThread);
	if (!thread) {
		if (handle)
			BF_TRACE(BFT_W32, "WaitForSingleObject(%p): not a thread", handle);
		return WAIT_FAILED;
	}
	return bf_sys_thread_wait(thread->thread, ms == INFINITE ? BF_SYS_WAIT_FOREVER : ms, NULL) ? WAIT_OBJECT_0
	                                                                                          : WAIT_TIMEOUT;
}

BOOL WINAPI GetExitCodeThread(HANDLE handle, LPDWORD code)
{
	Thread *thread = (Thread *)Lookup(handle, kThread);
	if (!thread || !code)
		return FALSE;
	uint32_t result = 0;
	*code = bf_sys_thread_wait(thread->thread, 0, &result) ? (DWORD)result : STILL_ACTIVE;
	return TRUE;
}

HANDLE WINAPI GetCurrentThread(void)
{
	return (HANDLE)~(ULONG_PTR)1;
}

DWORD WINAPI GetCurrentThreadId(void)
{
	if (!t_threadId)
		t_threadId = g_nextThreadId++;
	return t_threadId;
}

// Priorities apply to the calling thread (bf_sys sets only its own); the
// client sets only its own (GameWorld's loader thread).
BOOL WINAPI SetThreadPriority(HANDLE handle, INT priority)
{
	if (handle != GetCurrentThread()) {
		Thread *thread = (Thread *)Lookup(handle, kThread);
		if (!thread)
			return FALSE;
		if (thread->id != GetCurrentThreadId()) {
			BF_TRACE(BFT_W32, "SetThreadPriority(%d) on another thread: ignored", priority);
			return TRUE;
		}
	}
	if (priority < THREAD_PRIORITY_LOWEST)
		priority = THREAD_PRIORITY_LOWEST;
	if (priority > THREAD_PRIORITY_HIGHEST)
		priority = THREAD_PRIORITY_HIGHEST;
	bf_sys_thread_set_priority(priority);
	return TRUE;
}

// ---- Critical sections ------------------------------------------------------------
// The bf_sys mutex (recursive, as a critical section) lives in LockSemaphore.
// A zeroed section that was never initialised gets one on first entry.

namespace {
bf_sys_mutex *MutexOf(CRITICAL_SECTION *section)
{
	std::atomic<bf_sys_mutex *> *slot = reinterpret_cast<std::atomic<bf_sys_mutex *> *>(&section->LockSemaphore);
	bf_sys_mutex *mutex = slot->load(std::memory_order_acquire);
	if (mutex)
		return mutex;
	bf_sys_mutex *fresh = bf_sys_mutex_create();
	if (slot->compare_exchange_strong(mutex, fresh, std::memory_order_acq_rel))
		return fresh;
	bf_sys_mutex_destroy(fresh);
	return mutex;
}
}  // namespace

void WINAPI InitializeCriticalSection(CRITICAL_SECTION *section)
{
	if (!section)
		return;
	memset(section, 0, sizeof(*section));
	section->LockSemaphore = (HANDLE)bf_sys_mutex_create();
}

BOOL WINAPI InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *section, DWORD spin)
{
	(void)spin;
	InitializeCriticalSection(section);
	return TRUE;
}

void WINAPI EnterCriticalSection(CRITICAL_SECTION *section)
{
	bf_sys_mutex_lock(MutexOf(section));
	++section->RecursionCount;
}

BOOL WINAPI TryEnterCriticalSection(CRITICAL_SECTION *section)
{
	if (!bf_sys_mutex_try_lock(MutexOf(section)))
		return FALSE;
	++section->RecursionCount;
	return TRUE;
}

void WINAPI LeaveCriticalSection(CRITICAL_SECTION *section)
{
	--section->RecursionCount;
	bf_sys_mutex_unlock(MutexOf(section));
}

void WINAPI DeleteCriticalSection(CRITICAL_SECTION *section)
{
	if (!section)
		return;
	bf_sys_mutex *mutex = (bf_sys_mutex *)section->LockSemaphore;
	if (mutex)
		bf_sys_mutex_destroy(mutex);
	memset(section, 0, sizeof(*section));
}
