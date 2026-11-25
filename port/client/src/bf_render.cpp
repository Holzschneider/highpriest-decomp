// The client's render target on port/native/gfx (bf_render.h, M11.9): the
// device, the texture registry and the stage bindings, the lazy lock, the
// part present. The rules are those the DirectDraw/Direct3D adapter of stage A
// (port/compat/d3d, deleted in M11.9) applied, moved here unchanged.
#include "bf_render.h"

#include "bf_trace.h"

#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <set>
#include <vector>

namespace {

struct Render {
	bf_gfx_device *gfx = nullptr;
	// Bumped by every change of the target (draws, colour clears, writes,
	// restores, write-backs), so that the lock buffer is known to be current.
	unsigned long long serial = 1;
	std::set<const bf_gfx_texture *> textures;   // registered, alive
	bf_gfx_texture *bound[2] = {nullptr, nullptr};
	bool orphan[2] = {false, false};             // bound[i] was destroyed by its owner
	bf_gfx_texture *part_tex = nullptr;          // the part present's source
	long long locks = 0, readbacks = 0, writebacks = 0, deferred = 0;
};

// Never destroyed: the client's statics (CTexMgr, the renderer's caches)
// release their textures in any order at exit.
Render &R()
{
	static Render *r = new Render;
	return *r;
}

void changed() { ++R().serial; }

// ---- the lazy lock -----------------------------------------------------------
//
// The lock returns a buffer whose pages are inaccessible. The first access
// faults; the handler reads the target into the buffer and makes it readable.
// The first write then faults once more; the handler makes it writable and
// marks it written. Unlock writes the frame back only when it was written. An
// untouched lock costs two mprotect calls and reads nothing back.
//
// The fault handler is installed (for SIGBUS and SIGSEGV) at each lock if it
// is not the current handler, and chains every fault outside the buffer to
// the handler it replaced, so that a crash reporter installed before or after
// keeps working. Under a debugger the faults stop the program as
// EXC_BAD_ACCESS: in lldb, `settings set platform.plugin.darwin.ignored-exceptions
// EXC_BAD_ACCESS` (before `run`) and `process handle -s false -p true SIGBUS SIGSEGV`
// let them through.

enum State { IDLE, LOCKED_NONE, LOCKED_READ, LOCKED_RW };

struct Buffer {
	uint8_t *mem = nullptr;
	size_t size = 0;           // mapped bytes (page multiple)
	int w = 0, h = 0;
	int pitch = 0;
	volatile State state = IDLE;
	bool written = false;
	unsigned long long serial = 0;   // the target's serial right after the last write-back
};
Buffer s_buf;

struct sigaction s_old_bus, s_old_segv;
bool s_old_bus_valid = false, s_old_segv_valid = false;

bool read_back()
{
	Render &r = R();
	if (!r.gfx) return false;
	bf_gfx_rect q{0, 0, s_buf.w, s_buf.h};
	r.readbacks++;
	return bf_gfx_read_pixels(r.gfx, BF_GFX_SPACE_LOGICAL, &q, s_buf.mem, s_buf.pitch) == BF_GFX_OK;
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
	// and the process ends as it would have without the handler.
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

void drop_buffer()
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

bool ensure_buffer(int w, int h)
{
	if (s_buf.mem && s_buf.w == w && s_buf.h == h) return true;
	drop_buffer();
	size_t page = size_t(getpagesize());
	int pitch = w * 4;
	size_t bytes = size_t(pitch) * size_t(h);
	size_t size = (bytes + page - 1) / page * page;
	void *m = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (m == MAP_FAILED) {
		bf_tracef(BFT_ERR, "render: cannot map a %dx%d render-target buffer", w, h);
		return false;
	}
	s_buf.mem = (uint8_t *)m;
	s_buf.size = size;
	s_buf.w = w;
	s_buf.h = h;
	s_buf.pitch = pitch;
	return true;
}

// The lock buffer when it still holds the target as written back (nothing has
// changed the target since): the pixels without a read-back. NULL otherwise.
const uint8_t *buffer_current(int *pitch)
{
	if (!s_buf.mem || s_buf.state != IDLE || s_buf.serial != R().serial) return nullptr;
	int w, h;
	bf_render_size(&w, &h);
	if (w != s_buf.w || h != s_buf.h) return nullptr;
	*pitch = s_buf.pitch;
	return s_buf.mem;
}

} // namespace

// ---- device -----------------------------------------------------------------------

extern "C" void bf_render_set_device(bf_gfx_device *dev)
{
	Render &r = R();
	if (r.gfx == dev) return;
	if (r.gfx) {
		drop_buffer();
		for (int i = 0; i < 2; ++i) {
			bf_gfx_set_texture(r.gfx, i, nullptr);
			r.bound[i] = nullptr;
			r.orphan[i] = false;
		}
		for (const bf_gfx_texture *t : r.textures)
			bf_gfx_texture_destroy(const_cast<bf_gfx_texture *>(t));
		r.textures.clear();
		bf_gfx_texture_destroy(r.part_tex);
		r.part_tex = nullptr;
	}
	r.gfx = dev;
	changed();
}

extern "C" bf_gfx_device *bf_render_device(void) { return R().gfx; }

extern "C" void bf_render_size(int *width, int *height)
{
	*width = *height = 0;
	if (R().gfx) bf_gfx_get_size(R().gfx, width, height, nullptr, nullptr, nullptr);
}

// ---- textures ----------------------------------------------------------------------

extern "C" bf_gfx_texture *bf_render_texture_create(int width, int height, bf_gfx_format format, int mip_levels)
{
	Render &r = R();
	if (!r.gfx) return nullptr;
	bf_gfx_texture *t = bf_gfx_texture_create(r.gfx, width, height, format, mip_levels);
	if (t) r.textures.insert(t);
	return t;
}

extern "C" int bf_render_texture_valid(const bf_gfx_texture *tex)
{
	if (!tex) return 0;
	Render &r = R();
	if (!r.textures.count(tex)) return 0;
	for (int i = 0; i < 2; ++i)
		if (r.bound[i] == tex && r.orphan[i]) return 0;
	return 1;
}

extern "C" void bf_render_texture_destroy(bf_gfx_texture *tex)
{
	Render &r = R();
	if (!tex || !r.textures.count(tex)) return;
	bool held = false;
	for (int i = 0; i < 2; ++i)
		if (r.bound[i] == tex) {
			r.orphan[i] = true;   // the stage keeps it, as Direct3D 7 kept its reference
			held = true;
		}
	if (held) {
		r.deferred++;
		return;
	}
	r.textures.erase(tex);
	bf_gfx_texture_destroy(tex);
}

extern "C" void bf_render_bind_texture(int stage, bf_gfx_texture *tex)
{
	Render &r = R();
	if (stage < 0 || stage > 1) return;
	bf_gfx_texture *old = r.bound[stage];
	bool orphan = r.orphan[stage];
	r.bound[stage] = tex;
	r.orphan[stage] = old == tex ? orphan : false;
	if (old && orphan && old != tex) {
		int other = 1 - stage;
		if (r.bound[other] == old) {
			r.orphan[other] = true;   // still held by the other stage
		} else {
			r.textures.erase(old);
			bf_gfx_texture_destroy(old);
		}
	}
	if (r.gfx) bf_gfx_set_texture(r.gfx, stage, tex);
}

// ---- drawing ---------------------------------------------------------------------------

extern "C" int bf_render_draw(const bf_gfx_state *state, bf_gfx_prim prim, bf_gfx_vertex_format format,
                              const void *vertices, int vertex_count, const uint16_t *indices, int index_count)
{
	Render &r = R();
	if (!r.gfx) return BF_GFX_E_STATE;
	bf_gfx_set_state(r.gfx, state);
	bf_gfx_set_texture(r.gfx, 0, r.bound[0]);
	bf_gfx_set_texture(r.gfx, 1, r.bound[1]);
	int rc = indices ? bf_gfx_draw_indexed(r.gfx, prim, format, vertices, vertex_count, indices, index_count)
	                 : bf_gfx_draw(r.gfx, prim, format, vertices, vertex_count);
	changed();
	return rc;
}

extern "C" int bf_render_clear(unsigned flags, uint32_t color, float depth, const bf_gfx_rect *rect)
{
	Render &r = R();
	if (!r.gfx) return BF_GFX_E_STATE;
	int rc = bf_gfx_clear(r.gfx, flags, color, depth, rect);
	if (flags & BF_GFX_CLEAR_COLOR) changed();
	return rc;
}

extern "C" int bf_render_write_pixels(const bf_gfx_rect *rect, const void *pixels, int pitch, unsigned flags)
{
	Render &r = R();
	if (!r.gfx) return BF_GFX_E_STATE;
	int rc = bf_gfx_write_pixels(r.gfx, BF_GFX_SPACE_LOGICAL, rect, pixels, pitch, flags);
	changed();
	return rc;
}

extern "C" int bf_render_read_pixels(const bf_gfx_rect *rect, void *pixels, int pitch)
{
	Render &r = R();
	if (!r.gfx) return BF_GFX_E_STATE;
	return bf_gfx_read_pixels(r.gfx, BF_GFX_SPACE_LOGICAL, rect, pixels, pitch);
}

extern "C" void bf_render_begin_scene(void)
{
	if (R().gfx) bf_gfx_begin_frame(R().gfx);
}

extern "C" void bf_render_end_scene(void)
{
	if (R().gfx) bf_gfx_end_frame(R().gfx);   // submit without waiting
}

extern "C" int bf_render_backup(void)
{
	if (!R().gfx) return BF_GFX_E_STATE;
	return bf_gfx_backup_frame(R().gfx);
}

extern "C" int bf_render_restore(void)
{
	if (!R().gfx) return BF_GFX_E_STATE;
	int rc = bf_gfx_restore_frame(R().gfx);
	changed();
	return rc;
}

// ---- the lazy lock --------------------------------------------------------------------

extern "C" void *bf_render_lock(int write_only, int *pitch)
{
	int w, h;
	bf_render_size(&w, &h);
	if (w <= 0 || h <= 0 || s_buf.state != IDLE) return nullptr;
	if (!ensure_buffer(w, h)) return nullptr;
	R().locks++;
	s_buf.written = false;
	if (write_only) {
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

extern "C" void bf_render_unlock(void)
{
	if (s_buf.state == IDLE) return;
	bool written = s_buf.written;
	s_buf.state = IDLE;
	mprotect(s_buf.mem, s_buf.size, PROT_READ | PROT_WRITE);
	Render &r = R();
	if (written && r.gfx) {
		bf_gfx_rect q{0, 0, s_buf.w, s_buf.h};
		bf_gfx_write_pixels(r.gfx, BF_GFX_SPACE_LOGICAL, &q, s_buf.mem, s_buf.pitch, 0);
		r.writebacks++;
		changed();
		s_buf.serial = r.serial;
	}
}

extern "C" int bf_render_is_locked(void) { return s_buf.state != IDLE; }

// ---- presenting ------------------------------------------------------------------------

extern "C" int bf_render_present(void)
{
	Render &r = R();
	if (!r.gfx) return BF_GFX_E_STATE;
	int rc = bf_gfx_present(r.gfx);
	if (rc != BF_GFX_OK && rc != BF_GFX_E_STATE) {
		static bool once;
		if (!once) {
			once = true;
			bf_tracef(BFT_GFX, "render: present failed (%d)", rc);
		}
	}
	return rc;
}

// A part of the target shown in the whole window: DirectDraw stretched a Blt
// to the primary from the source rectangle to the destination (the client
// area). The window shows the whole target, so the part is drawn stretched
// over the target first, filtered linearly as the hardware blitters of the
// time did. CBink::BinkShowNextFrame does this: the movie at the top left of
// the back buffer, shown in the whole window. The part's pixels come from
// the lock buffer when the client has just written them there (no read-back).
extern "C" int bf_render_present_part(int x, int y, int w, int h)
{
	Render &r = R();
	if (!r.gfx) return BF_GFX_E_STATE;
	int tw, th;
	bf_render_size(&tw, &th);
	bool whole = x == 0 && y == 0 && w == tw && h == th;
	if (whole || x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > tw || y + h > th)
		return bf_render_present();

	std::vector<uint32_t> spx;
	const void *src;
	int src_pitch;
	int bp;
	if (const uint8_t *buf = buffer_current(&bp)) {
		src = buf + size_t(y) * size_t(bp) + size_t(x) * 4;
		src_pitch = bp;
	} else {
		spx.assign(size_t(w) * size_t(h), 0);
		bf_gfx_rect q{x, y, w, h};
		if (bf_gfx_read_pixels(r.gfx, BF_GFX_SPACE_LOGICAL, &q, spx.data(), 0) != BF_GFX_OK)
			return bf_render_present();
		src = spx.data();
		src_pitch = w * 4;
	}
	int pw = 0, ph = 0;
	if (r.part_tex) bf_gfx_texture_get_info(r.part_tex, &pw, &ph, nullptr, nullptr);
	if (!r.part_tex || pw != w || ph != h) {
		bf_gfx_texture_destroy(r.part_tex);
		r.part_tex = bf_gfx_texture_create(r.gfx, w, h, BF_GFX_FORMAT_A8R8G8B8, 1);
		if (!r.part_tex) return bf_render_present();
	}
	bf_gfx_texture_update(r.part_tex, 0, 0, w, h, src, src_pitch);

	bf_gfx_state st;
	bf_gfx_state_init(&st);
	st.stage[0].color_op = BF_GFX_OP_SELECT_ARG1;
	st.stage[0].color_arg1 = BF_GFX_ARG_TEXTURE;
	st.stage[0].alpha_op = BF_GFX_OP_SELECT_ARG1;
	st.stage[0].alpha_arg1 = BF_GFX_ARG_DIFFUSE;
	st.stage[0].address_u = st.stage[0].address_v = BF_GFX_ADDRESS_CLAMP;
	st.stage[0].mag_filter = st.stage[0].min_filter = BF_GFX_FILTER_LINEAR;
	st.stage[1].color_op = st.stage[1].alpha_op = BF_GFX_OP_DISABLE;
	st.depth_enable = 0;
	st.depth_write = 0;
	st.cull = BF_GFX_CULL_NONE;
	bf_gfx_set_state(r.gfx, &st);
	bf_gfx_set_texture(r.gfx, 0, r.part_tex);
	bf_gfx_set_texture(r.gfx, 1, nullptr);
	struct V { float x, y, z, rhw; uint32_t diffuse, specular; float u, v; };
	// Direct3D 7 pixel centres: the quad's edges at -0.5 cover the target exactly.
	V v[4] = {{-0.5f, -0.5f, 0, 1, 0xFFFFFFFFu, 0, 0, 0},
	          {tw - 0.5f, -0.5f, 0, 1, 0xFFFFFFFFu, 0, 1, 0},
	          {-0.5f, th - 0.5f, 0, 1, 0xFFFFFFFFu, 0, 0, 1},
	          {tw - 0.5f, th - 0.5f, 0, 1, 0xFFFFFFFFu, 0, 1, 1}};
	bf_gfx_draw(r.gfx, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4);
	// The client's own bindings go back with its next draw (bf_render_draw).
	bf_gfx_set_texture(r.gfx, 0, r.bound[0]);
	bf_gfx_set_texture(r.gfx, 1, r.bound[1]);
	changed();
	bf_count("render.part_presents", 1);
	return bf_render_present();
}

extern "C" void bf_render_get_stats(long long *locks, long long *readbacks, long long *writebacks, long long *deferred)
{
	Render &r = R();
	if (locks) *locks = r.locks;
	if (readbacks) *readbacks = r.readbacks;
	if (writebacks) *writebacks = r.writebacks;
	if (deferred) *deferred = r.deferred;
}
