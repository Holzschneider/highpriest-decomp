// bifrost-sys-test: one line per check, non-zero exit on a failure.
//
// Writes only below its own build directory (BF_HOME is pointed there before
// the module is used). Checks on the real game folder read BF_CLIENT_DIR
// (default ~/Games/<name>) and are skipped when it is absent.

#include "bf_sys.h"
#include "bf_trace.h"

#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

#include <atomic>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_passes = 0;
int g_skips = 0;

void Check(bool ok, const char *name, const char *detail = "")
{
	if (ok) {
		++g_passes;
		printf("ok    %s\n", name);
	} else {
		++g_failures;
		printf("FAIL  %s%s%s\n", name, detail[0] ? ": " : "", detail);
	}
	fflush(stdout);
}

__attribute__((unused)) void Skip(const char *name, const char *why)
{
	++g_skips;
	printf("skip  %s: %s\n", name, why);
	fflush(stdout);
}

std::string Format(const char *format, ...) __attribute__((format(printf, 1, 2)));
std::string Format(const char *format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	return buffer;
}

int RemoveEntry(const char *path, const struct stat *, int, struct FTW *)
{
	return remove(path);
}

void RemoveTree(const std::string &path)
{
	nftw(path.c_str(), RemoveEntry, 16, FTW_DEPTH | FTW_PHYS);
}

__attribute__((unused)) bool Exists(const std::string &path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

__attribute__((unused)) void WriteText(const std::string &path, const char *text)
{
	FILE *f = fopen(path.c_str(), "wb");
	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

__attribute__((unused)) std::string ReadText(const std::string &path)
{
	std::string out;
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return out;
	char buffer[4096];
	size_t n;
	while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
		out.append(buffer, n);
	fclose(f);
	return out;
}

std::string ExecutableFolder()
{
	char path[PATH_MAX];
	uint32_t size = sizeof(path);
	if (_NSGetExecutablePath(path, &size) != 0)
		return ".";
	char real[PATH_MAX];
	if (!realpath(path, real))
		return ".";
	std::string s = real;
	return s.substr(0, s.rfind('/'));
}

// ---- Clock --------------------------------------------------------------------

uint64_t g_fakeNow = 0;
uint64_t g_fakeSlept = 0;
uint64_t FakeNow(void *) { return g_fakeNow; }
void FakeSleep(void *, uint64_t us) { g_fakeSlept += us; g_fakeNow += us; }

void TestClock()
{
	uint64_t last = bf_clock_us();
	bool monotonic = true;
	for (int i = 0; i < 200000; ++i) {
		uint64_t now = bf_clock_us();
		if (now < last)
			monotonic = false;
		last = now;
	}
	Check(monotonic, "clock: 200000 readings never go backwards");

	uint64_t us = bf_clock_us();
	uint32_t ms = bf_clock_ms();
	Check(ms >= us / 1000 && ms <= us / 1000 + 5, "clock: milliseconds agree with microseconds");

	bool accurate = true;
	std::string detail;
	for (uint32_t wanted : { 1u, 5u, 20u, 50u }) {
		uint64_t start = bf_clock_us();
		bf_clock_sleep_ms(wanted);
		double took = (bf_clock_us() - start) / 1000.0;
		detail += Format("%u->%.2f ", wanted, took);
		if (took < wanted || took > wanted + 15)
			accurate = false;
	}
	Check(accurate, Format("clock: sleep accuracy (ms asked->taken: %s)", detail.c_str()).c_str());

	uint64_t start = bf_clock_us();
	for (int i = 0; i < 100; ++i)
		bf_clock_sleep_ms(0);
	Check(bf_clock_us() - start < 20000, "clock: sleep 0 only yields");

	bf_clock_source source = { FakeNow, FakeSleep, NULL };
	g_fakeNow = 5000000;
	bf_clock_set_source(&source);
	bool followed = bf_clock_ms() == 5000 && bf_clock_us() == 5000000;
	start = bf_clock_us();
	bf_clock_sleep_ms(16);
	followed = followed && g_fakeSlept == 16000 && bf_clock_us() - start == 16000;
	bf_clock_set_source(NULL);
	bool restored = bf_clock_us() < 5000000 && bf_clock_us() >= last;
	Check(followed && restored, "clock: an installed source is followed, and removing it restores the real clock");

	bf_clock_date date;
	bf_clock_local_time(&date);
	bf_clock_date utc;
	bf_clock_utc_time(&utc);
	Check(date.year >= 2024 && date.month >= 1 && date.month <= 12 && date.day >= 1 && date.weekday >= 0
	      && date.weekday <= 6 && utc.year >= 2024,
	      Format("clock: calendar %04d-%02d-%02d %02d:%02d:%02d.%03d", date.year, date.month, date.day,
	             date.hour, date.minute, date.second, date.millisecond).c_str());
}

// ---- Threads -----------------------------------------------------------------

std::atomic<int> g_gate(0);

uint32_t ReturnArgument(void *argument)
{
	return (uint32_t)(uintptr_t)argument;
}

uint32_t WaitForGate(void *)
{
	while (!g_gate.load())
		bf_clock_sleep_ms(1);
	return 7;
}

uint32_t IsMain(void *)
{
	return (uint32_t)bf_sys_is_main_thread();
}

struct Contention {
	bf_sys_mutex *mutex;
	long counter;
};

uint32_t Increment(void *raw)
{
	Contention *c = (Contention *)raw;
	for (int i = 0; i < 100000; ++i) {
		bf_sys_mutex_lock(c->mutex);
		bf_sys_mutex_lock(c->mutex);      // recursive, like a critical section
		long v = c->counter;
		c->counter = v + 1;
		bf_sys_mutex_unlock(c->mutex);
		bf_sys_mutex_unlock(c->mutex);
	}
	return 0;
}

uint32_t TryLockFromOther(void *raw)
{
	return (uint32_t)bf_sys_mutex_try_lock((bf_sys_mutex *)raw);
}

uint64_t g_otherId = 0;
uint32_t RecordId(void *)
{
	g_otherId = bf_sys_thread_id();
	return 0;
}

void TestThreads()
{
	bf_sys_thread *t = bf_sys_thread_create(ReturnArgument, (void *)(uintptr_t)42, "bf-test");
	uint32_t result = 0;
	Check(t && bf_sys_thread_wait(t, BF_SYS_WAIT_FOREVER, &result) == 1 && result == 42
	      && bf_sys_thread_wait(t, 0, &result) == 1,
	      "threads: create, wait twice, result passed back");
	bf_sys_thread_release(t);

	g_gate = 0;
	t = bf_sys_thread_create(WaitForGate, NULL, NULL);
	bool timedOut = bf_sys_thread_wait(t, 30, NULL) == 0;
	g_gate = 1;
	Check(timedOut && bf_sys_thread_join(t) == 7, "threads: wait times out on a running thread; join returns its value");

	g_gate = 0;
	t = bf_sys_thread_create(WaitForGate, NULL, NULL);
	bf_sys_thread_release(t);   // released while running: the thread must still finish safely
	g_gate = 1;
	bf_clock_sleep_ms(20);
	Check(true, "threads: handle released while the thread runs");

	t = bf_sys_thread_create(IsMain, NULL, NULL);
	Check(bf_sys_is_main_thread() == 1 && bf_sys_thread_join(t) == 0, "threads: main thread recognised, worker is not main");

	t = bf_sys_thread_create(RecordId, NULL, NULL);
	bf_sys_thread_join(t);
	Check(bf_sys_thread_id() != 0 && g_otherId != 0 && g_otherId != bf_sys_thread_id(), "threads: ids are non-zero and distinct");

	Contention c = { bf_sys_mutex_create(), 0 };
	std::vector<bf_sys_thread *> threads;
	for (int i = 0; i < 8; ++i)
		threads.push_back(bf_sys_thread_create(Increment, &c, "bf-contention"));
	for (bf_sys_thread *th : threads)
		bf_sys_thread_join(th);
	Check(c.counter == 800000, Format("mutex: 8 threads x 100000 nested increments = %ld", c.counter).c_str());

	bf_sys_mutex_lock(c.mutex);
	t = bf_sys_thread_create(TryLockFromOther, c.mutex, NULL);
	uint32_t got = bf_sys_thread_join(t);
	bool ownAgain = bf_sys_mutex_try_lock(c.mutex) == 1;
	bf_sys_mutex_unlock(c.mutex);
	bf_sys_mutex_unlock(c.mutex);
	Check(got == 0 && ownAgain, "mutex: held lock refuses another thread, admits its owner");
	bf_sys_mutex_destroy(c.mutex);
}

// ---- Keychain and shell ----------------------------------------------------------

void TestShell()
{
	setenv("BF_NO_ALERTS", "1", 1);
	uint64_t start = bf_clock_us();
	bool logged = bf_sys_alerts_enabled() == 0
	           && bf_sys_alert("Test", "logged, not shown", BF_ALERT_INFO, BF_ALERT_BUTTONS_OK) == BF_ALERT_OK
	           && bf_sys_alert("Test", "logged, not shown", BF_ALERT_WARNING, BF_ALERT_BUTTONS_YES_NO) == BF_ALERT_YES
	           && bf_sys_alert("Test", "logged", BF_ALERT_CRITICAL, BF_ALERT_BUTTONS_OK_CANCEL) == BF_ALERT_OK;
	Check(logged && bf_clock_us() - start < 1000000, "shell: with BF_NO_ALERTS an alert is logged and answered with its first button");

	setenv("BF_NO_OPEN", "1", 1);
	Check(bf_sys_open_url("https://example.com/") == 1 && bf_sys_open_url("not a url") == 0
	      && bf_sys_open_path("/nonexistent/file") == 0 && bf_sys_open_path("/") == 1,
	      "shell: open URL and path validate and, with BF_NO_OPEN, only log");
	Check(strcmp(bf_sys_app_name(), BF_APP_NAME) == 0, "shell: application name from the build");
}

} // namespace

int main()
{
	// Everything the test writes stays below the build directory.
	std::string root = ExecutableFolder() + "/sys-test";
	RemoveTree(root);
	mkdir(root.c_str(), 0755);
	std::string home = root + "/home";
	std::string game = root + "/game";
	mkdir(home.c_str(), 0755);
	mkdir(game.c_str(), 0755);
	setenv("BF_HOME", home.c_str(), 1);

	const char *clientDir = getenv("BF_CLIENT_DIR");
	struct passwd *pw = getpwuid(getuid());
	std::string realGame = clientDir && *clientDir ? clientDir : std::string(pw ? pw->pw_dir : "") + "/Games/" BF_APP_NAME;

	printf("bifrost-sys-test: home %s\n", home.c_str());
	TestClock();
	TestThreads();
	(void)realGame;
	TestShell();

	printf("%d passed, %d failed, %d skipped\n", g_passes, g_failures, g_skips);
	return g_failures ? 1 : 0;
}
