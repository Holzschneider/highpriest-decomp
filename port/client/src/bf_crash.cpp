// The client's crash handler on macOS (technical plan 3.3, "Protection";
// implementation plan M2.5). It replaces src/ExceptionHandler.cpp, the crash
// dialog and minidump writer, which is out of the build.
//
// At start-up (a static constructor, so nothing in src/ has to call it) it
// installs a handler for the signals of a crash. The handler writes the
// signal, the faulting address and a backtrace through the trace (category
// err, which is on by default), then restores the default action and raises
// the signal again, so that the system's crash report and a debugger still
// see the original fault.
// Compiled with the client's flags (bf_client_flags in CMakeLists.txt).
#include "bf_types.h"
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include "bf_trace.h"

namespace {

const int kSignals[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP };

void bf_crash_handler(int sig, siginfo_t* info, void* context)
{
	static volatile sig_atomic_t entered = 0;
	if (!entered) {
		entered = 1;
		void* frames[64];
		int count = backtrace(frames, 64);
		void* pc = NULL;
		void* lr = NULL;
		if (context) {
			ucontext_t* uc = (ucontext_t*)context;
			pc = (void*)uc->uc_mcontext->__ss.__pc;
			lr = (void*)uc->uc_mcontext->__ss.__lr;
		}
		bf_tracef(BFT_ERR, "crash: signal %d (%s) at address %p, pc %p, lr %p, %d frames", sig,
			strsignal(sig), info ? info->si_addr : NULL, pc, lr, count);
		char** names = backtrace_symbols(frames, count);
		for (int i = 0; i < count; ++i) {
			if (names)
				bf_tracef(BFT_ERR, "crash:   %s", names[i]);
			else
				bf_tracef(BFT_ERR, "crash:   #%d %p", i, frames[i]);
		}
		bf_trace_dump_counters();
	}
	// SA_RESETHAND restored the default action; raise again to end the
	// process the way the system reports it.
	raise(sig);
}

struct InstallCrashHandler {
	InstallCrashHandler()
	{
		if (getenv("BF_NO_CRASH_HANDLER"))
			return;
		// An alternate stack, so that a stack overflow is reported too.
		static char altStack[64 * 1024];
		stack_t ss;
		ss.ss_sp = altStack;
		ss.ss_size = sizeof(altStack);
		ss.ss_flags = 0;
		sigaltstack(&ss, NULL);

		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_sigaction = bf_crash_handler;
		sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
		sigemptyset(&sa.sa_mask);
		for (size_t i = 0; i < sizeof(kSignals) / sizeof(kSignals[0]); ++i)
			sigaction(kSignals[i], &sa, NULL);
	}
} installCrashHandler;

}
