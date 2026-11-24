// The render-target lock, lazy (technical plan 3.3).
//
// CRenderer::Flip locks and unlocks the render target every frame without
// touching a pixel; screenshots and the screen effect lock it and read (and
// write) the frame. A Direct3D 7 lock has no way to say which, so the adapter
// finds out from the accesses themselves: the lock returns a buffer whose
// pages are inaccessible. The first access faults; the handler reads the
// target into the buffer and makes it readable. The first write then faults
// once more; the handler makes it writable and marks it written. Unlock writes
// the frame back only when it was written. An untouched lock costs two
// mprotect calls and reads nothing back.
//
// The fault handler is installed (for SIGBUS and SIGSEGV) at each lock if it
// is not the current handler, and chains every fault outside the buffer to
// the handler it replaced, so a crash reporter installed before or after the
// adapter keeps working. Under a debugger the faults stop the program as
// EXC_BAD_ACCESS: in lldb, `settings set platform.plugin.darwin.ignored-exceptions
// EXC_BAD_ACCESS` (before `run`) and `process handle -s false -p true SIGBUS SIGSEGV`
// let them through.
#include "d3d_internal.h"

#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

namespace bfd3d {

namespace {

enum State { IDLE, LOCKED_NONE, LOCKED_READ, LOCKED_RW };

struct Buffer {
	uint8_t *mem = nullptr;
	size_t size = 0;           // mapped bytes (page multiple)
	int w = 0, h = 0;
	LONG pitch = 0;
	volatile State state = IDLE;
	bool written = false;
	unsigned long long serial = 0;   // target_serial right after the last write-back
};
Buffer s_buf;

struct sigaction s_old_bus, s_old_segv;
bool s_old_bus_valid = false, s_old_segv_valid = false;

bool read_back()
{
	Global &G = g();
	if (!G.gfx) return false;
	bf_gfx_rect r{0, 0, s_buf.w, s_buf.h};
	G.target_readbacks++;
	return bf_gfx_read_pixels(G.gfx, BF_GFX_SPACE_LOGICAL, &r, s_buf.mem, int(s_buf.pitch)) == BF_GFX_OK;
}

void chain(int sig, siginfo_t *info, void *uc)
{
	struct sigaction *old = sig == SIGBUS ? &s_old_bus : &s_old_segv;
	bool valid = sig == SIGBUS ? s_old_bus_valid : s_old_segv_valid;
	if (valid && (old->sa_flags & SA_SIGINFO) && old->sa_sigaction) {
		old->sa_sigaction(sig, info, uc);
		return;
	}
	if (valid && old->sa_handler != SIG_DFL && old->sa_handler != SIG_IGN) {
		old->sa_handler(sig);
		return;
	}
	// Default action: restore it and return; the instruction faults again
	// and the process ends as it would have without the adapter.
	struct sigaction dfl;
	memset(&dfl, 0, sizeof dfl);
	dfl.sa_handler = SIG_DFL;
	sigaction(sig, &dfl, nullptr);
}

void on_fault(int sig, siginfo_t *info, void *uc)
{
	uint8_t *a = (uint8_t *)info->si_addr;
	if (s_buf.state != IDLE && s_buf.mem && a >= s_buf.mem && a < s_buf.mem + s_buf.size) {
		if (s_buf.state == LOCKED_NONE) {
			mprotect(s_buf.mem, s_buf.size, PROT_READ | PROT_WRITE);
			read_back();
			mprotect(s_buf.mem, s_buf.size, PROT_READ);
			s_buf.state = LOCKED_READ;
			return;
		}
		if (s_buf.state == LOCKED_READ) {
			mprotect(s_buf.mem, s_buf.size, PROT_READ | PROT_WRITE);
			s_buf.written = true;
			s_buf.state = LOCKED_RW;
			return;
		}
	}
	chain(sig, info, uc);
}

void ensure_handler(int sig, struct sigaction *old, bool *valid)
{
	struct sigaction cur;
	if (sigaction(sig, nullptr, &cur) != 0) return;
	if ((cur.sa_flags & SA_SIGINFO) && cur.sa_sigaction == on_fault) return;
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_sigaction = on_fault;
	sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig, &sa, old) == 0) *valid = true;
}

bool ensure_buffer(int w, int h)
{
	if (s_buf.mem && s_buf.w == w && s_buf.h == h) return true;
	target_drop_buffer();
	size_t page = size_t(getpagesize());
	LONG pitch = LONG(w) * 4;
	size_t bytes = size_t(pitch) * size_t(h);
	size_t size = (bytes + page - 1) / page * page;
	void *m = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (m == MAP_FAILED) {
		bf_tracef(BFT_ERR, "d3d: cannot map a %dx%d render-target buffer", w, h);
		return false;
	}
	s_buf.mem = (uint8_t *)m;
	s_buf.size = size;
	s_buf.w = w;
	s_buf.h = h;
	s_buf.pitch = pitch;
	return true;
}

} // namespace

void *target_lock(DWORD flags, LONG *pitch)
{
	int w, h;
	target_size(&w, &h);
	if (w <= 0 || h <= 0 || s_buf.state != IDLE) return nullptr;
	if (!ensure_buffer(w, h)) return nullptr;
	g().target_locks++;
	s_buf.written = false;
	if (flags & DDLOCK_WRITEONLY) {
		// Contents are undefined for a write-only lock: no read-back, and the
		// frame is written back on unlock.
		s_buf.written = true;
		s_buf.state = LOCKED_RW;
	} else {
		ensure_handler(SIGBUS, &s_old_bus, &s_old_bus_valid);
		ensure_handler(SIGSEGV, &s_old_segv, &s_old_segv_valid);
		s_buf.state = LOCKED_NONE;
		mprotect(s_buf.mem, s_buf.size, PROT_NONE);
	}
	*pitch = s_buf.pitch;
	return s_buf.mem;
}

void target_unlock()
{
	if (s_buf.state == IDLE) return;
	bool written = s_buf.written;
	s_buf.state = IDLE;
	mprotect(s_buf.mem, s_buf.size, PROT_READ | PROT_WRITE);
	Global &G = g();
	if (written && G.gfx) {
		bf_gfx_rect r{0, 0, s_buf.w, s_buf.h};
		bf_gfx_write_pixels(G.gfx, BF_GFX_SPACE_LOGICAL, &r, s_buf.mem, int(s_buf.pitch), 0);
		G.target_writebacks++;
		target_changed();
		s_buf.serial = G.target_serial;
	}
}

const uint8_t *target_buffer_current(int *w, int *h, LONG *pitch)
{
	if (!s_buf.mem || s_buf.state != IDLE || s_buf.serial != g().target_serial) return nullptr;
	*w = s_buf.w;
	*h = s_buf.h;
	*pitch = s_buf.pitch;
	return s_buf.mem;
}

bool target_is_locked() { return s_buf.state != IDLE; }

void target_drop_buffer()
{
	if (!s_buf.mem) return;
	s_buf.state = IDLE;
	munmap(s_buf.mem, s_buf.size);
	s_buf.mem = nullptr;
	s_buf.size = 0;
	s_buf.w = s_buf.h = 0;
	s_buf.pitch = 0;
	s_buf.written = false;
	s_buf.serial = 0;
}

} // namespace bfd3d
