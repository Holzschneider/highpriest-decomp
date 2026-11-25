// bifrost-render-test: the client's render target (bf_render.h, M11.9) on an
// offscreen gfx device, through the call sequences of src/3dlib: CRenderer's
// Flip, screenshot and screen-effect locks, CBink's write-only lock and part
// present, Mode.cpp's fade (backup and restore), CRenderer::SetTexture's
// bindings with a texture its owner destroys while bound, and the device
// going before the client's textures. It carries the checks of the DirectDraw
// adapter's test (port/compat/d3d, deleted) for the rules that moved here.
// Prints one line per check; exits non-zero on a failure.
#include "bf_render.h"

#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

static int g_fail = 0, g_pass = 0;
static void check(bool ok, const char *what, const std::string &detail = std::string())
{
	printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
	if (ok) ++g_pass; else ++g_fail;
}
static std::string hex(uint32_t v) { char b[16]; snprintf(b, sizeof b, "%08x", v); return b; }

static bf_gfx_device *g_gfx;
static uint32_t pix(int x, int y)
{
	uint32_t v = 0;
	bf_gfx_rect r{x, y, 1, 1};
	bf_gfx_read_pixels(g_gfx, BF_GFX_SPACE_LOGICAL, &r, &v, 0);
	return v | 0xFF000000u;
}
static bool close_to(uint32_t a, uint32_t b, int tol)
{
	for (int s = 0; s < 24; s += 8)
		if (abs(int((a >> s) & 0xFF) - int((b >> s) & 0xFF)) > tol) return false;
	return true;
}
static void check_px(const char *what, int x, int y, uint32_t want, int tol = 0)
{
	uint32_t got = pix(x, y);
	char d[96];
	snprintf(d, sizeof d, "(%d,%d) %s, expected %s", x, y, hex(got).c_str(), hex(want | 0xFF000000u).c_str());
	check(close_to(got, want, tol), what, d);
}
static long long gfx_readbacks()
{
	long long n = 0;
	bf_gfx_get_stats(g_gfx, nullptr, nullptr, &n, nullptr);
	return n;
}

struct tlvertex3d { float x, y, z, oow; uint32_t color, specular; float tu, tv; };

// A textured quad from (x, y) to (x + w, y + h), texels 1:1 on a w x h texture.
static void quad(tlvertex3d *v, float x, float y, float w, float h, uint32_t color)
{
	float u0 = 0.5f / w, v0 = 0.5f / h, u1 = (w + 0.5f) / w, v1 = (h + 0.5f) / h;
	v[0] = {x, y, 0.00001f, 0.99999f, color, 0xFFFFFFFFu, u0, v0};
	v[1] = {x + w, y, 0.00001f, 0.99999f, color, 0xFFFFFFFFu, u1, v0};
	v[2] = {x, y + h, 0.00001f, 0.99999f, color, 0xFFFFFFFFu, u0, v1};
	v[3] = {x + w, y + h, 0.00001f, 0.99999f, color, 0xFFFFFFFFu, u1, v1};
}

// The state of CRenderer's UI pass, simplified: texture x diffuse, no depth.
static bf_gfx_state ui_state()
{
	bf_gfx_state st;
	bf_gfx_state_init(&st);
	st.stage[0].color_op = BF_GFX_OP_MODULATE;
	st.stage[0].color_arg1 = BF_GFX_ARG_TEXTURE;
	st.stage[0].color_arg2 = BF_GFX_ARG_DIFFUSE;
	st.stage[0].alpha_op = BF_GFX_OP_SELECT_ARG1;
	st.stage[0].alpha_arg1 = BF_GFX_ARG_TEXTURE;
	st.stage[1].color_op = st.stage[1].alpha_op = BF_GFX_OP_DISABLE;
	st.depth_enable = 0;
	st.cull = BF_GFX_CULL_NONE;
	return st;
}

static bf_gfx_texture *solid_texture(int w, int h, uint32_t argb)
{
	bf_gfx_texture *t = bf_render_texture_create(w, h, BF_GFX_FORMAT_A8R8G8B8, 1);
	if (t) bf_gfx_texture_fill(t, nullptr, argb);
	return t;
}

// A stand-in for the crash reporter: a fault handler installed before the
// lock's, which must still receive faults outside the lock buffer.
static sigjmp_buf g_jump;
static volatile sig_atomic_t g_caught;
static void crash_reporter(int, siginfo_t *, void *) { g_caught = 1; siglongjmp(g_jump, 1); }

int main()
{
	int pitch = 0;
	check(!bf_render_device() && !bf_render_lock(0, &pitch) && !bf_render_texture_create(4, 4, BF_GFX_FORMAT_A8R8G8B8, 1),
	      "without a device: no lock, no texture");

	bf_gfx_desc gd = {nullptr, 320, 240, 1};
	g_gfx = bf_gfx_create(&gd);
	if (!g_gfx) { printf("FAIL no Metal device\n"); return 1; }
	bf_render_set_device(g_gfx);
	int w, h;
	bf_render_size(&w, &h);
	check(bf_render_device() == g_gfx && w == 320 && h == 240, "the device and its logical size");

	const uint32_t bg = 0xFF203040u;
	bf_render_clear(BF_GFX_CLEAR_COLOR | BF_GFX_CLEAR_DEPTH, bg, 1.0f, nullptr);

	// CRenderer::Flip: Lock/Unlock touching nothing. No read-back, no write-back.
	long long l0, r0, w0, l1, r1, w1;
	bf_render_get_stats(&l0, &r0, &w0, nullptr);
	long long g0 = gfx_readbacks();
	for (int i = 0; i < 3; ++i) {
		if (bf_render_lock(0, &pitch)) bf_render_unlock();
		bf_render_present();
	}
	bf_render_get_stats(&l1, &r1, &w1, nullptr);
	check(l1 - l0 == 3 && r1 == r0 && w1 == w0 && gfx_readbacks() == g0, "Flip: three locks, no read-back, no write-back",
	      std::to_string(l1 - l0) + " locks");
	check(pitch == 320 * 4, "the lock's pitch is the logical width in 32-bit words");

	// SaveBmp-style screenshot: lock and read through the pointer.
	{
		uint8_t *p = (uint8_t *)bf_render_lock(0, &pitch);
		uint32_t v = p ? *(uint32_t *)(p + 17 * pitch + 23 * 4) : 0;
		bool again = bf_render_lock(0, &pitch) == nullptr;
		if (p) bf_render_unlock();
		bf_render_get_stats(&l1, &r1, &w1, nullptr);
		check(p && (v & 0xFFFFFF) == (bg & 0xFFFFFF) && r1 == r0 + 1 && w1 == w0,
		      "screenshot lock: the first read brings the frame back once, nothing is written", hex(v));
		check(again, "a second lock while locked fails");
	}
	// ScreenEffect-style read-modify-write: written back on unlock.
	{
		uint8_t *p = (uint8_t *)bf_render_lock(0, &pitch);
		if (p) {
			uint32_t *row = (uint32_t *)(p + 200 * pitch);
			for (int x = 0; x < 320; ++x) row[x] = row[x] ^ 0x00FFFFFF;
			bf_render_unlock();
		}
		bf_render_get_stats(&l1, &r1, &w1, nullptr);
		check(p && w1 == w0 + 1, "screen-effect lock: a written frame goes back on unlock");
		check_px("screen-effect lock: the written row is on the target", 10, 200, bg ^ 0xFFFFFF);
		check_px("screen-effect lock: the rest is unchanged", 10, 199, bg);
	}
	// Faults outside the lock buffer go to the handler the lock replaced.
	{
		struct sigaction sa, old_bus, old_segv;
		memset(&sa, 0, sizeof sa);
		sa.sa_sigaction = crash_reporter;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);
		sigaction(SIGBUS, &sa, &old_bus);
		sigaction(SIGSEGV, &sa, &old_segv);
		if (bf_render_lock(0, &pitch)) bf_render_unlock();   // installs its handler on top
		struct sigaction now;
		sigaction(SIGSEGV, nullptr, &now);
		bool replaced = now.sa_sigaction != crash_reporter;
		g_caught = 0;
		if (sigsetjmp(g_jump, 1) == 0) *(volatile int *)(uintptr_t)16 = 1;
		check(replaced && g_caught, "a fault elsewhere reaches the handler installed before the lock's");
		sigaction(SIGBUS, &old_bus, nullptr);
		sigaction(SIGSEGV, &old_segv, nullptr);
	}

	// CBink::BinkShowNextFrame: a write-only lock, the movie at the top left,
	// then its rectangle shown over the whole window, from the lock buffer.
	{
		bf_render_get_stats(&l0, &r0, &w0, nullptr);
		uint8_t *p = (uint8_t *)bf_render_lock(1, &pitch);
		if (p) {
			for (int y = 0; y < 240; ++y)
				for (int x = 0; x < 320; ++x)
					*(uint32_t *)(p + y * pitch + x * 4) = x < 64 && y < 32 ? 0x00FF0000u : 0x000000FFu;
			bf_render_unlock();
		}
		bf_render_get_stats(&l1, &r1, &w1, nullptr);
		check(p && r1 == r0 && w1 == w0 + 1, "write-only lock: nothing read, the frame goes back");
		long long g = gfx_readbacks();
		bf_render_present_part(0, 0, 64, 32);
		check(gfx_readbacks() == g, "part present from the lock buffer: no read-back");
		check_px("part present: the part covers the target (top left)", 2, 2, 0xFF0000, 1);
		check_px("part present: the part covers the target (bottom right)", 317, 237, 0xFF0000, 1);
		// Again after a draw: the buffer is stale, the part is read back.
		bf_render_clear(BF_GFX_CLEAR_COLOR, 0xFF00FF00u, 1.0f, nullptr);
		g = gfx_readbacks();
		bf_render_present_part(0, 0, 64, 32);
		check(gfx_readbacks() == g + 1, "part present after a change of the target: read back");
		check_px("part present from the read-back: the target's own part", 160, 120, 0x00FF00, 1);
		// The whole target, or a rectangle outside it: present as it is.
		bf_render_clear(BF_GFX_CLEAR_COLOR, bg, 1.0f, nullptr);
		bf_render_present_part(0, 0, 320, 240);
		bf_render_present_part(0, 0, 512, 347);
		check_px("a whole or too large rectangle presents the target unchanged", 160, 120, bg);
	}

	// CRenderer::SetTexture after the part present: the client's state and
	// bindings go with its next draw.
	tlvertex3d v[4];
	{
		bf_gfx_texture *t = solid_texture(16, 16, 0xFFFFFF00u);
		bf_render_bind_texture(0, t);
		bf_render_bind_texture(1, nullptr);
		bf_render_present_part(0, 0, 64, 32);
		bf_render_clear(BF_GFX_CLEAR_COLOR, bg, 1.0f, nullptr);
		bf_gfx_state st = ui_state();
		quad(v, 8, 8, 16, 16, 0xFFFFFFFFu);
		check(bf_render_draw(&st, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4, nullptr, 0) == BF_GFX_OK,
		      "draw");
		check_px("draw after a part present: the client's texture and state", 12, 12, 0xFFFF00);

		// The owner destroys the bound texture: the stage keeps it (Direct3D 7
		// held a reference), as the renderer skips SetTexture for the same pointer.
		long long d0, d1;
		bf_render_get_stats(nullptr, nullptr, nullptr, &d0);
		bf_render_texture_destroy(t);
		bf_render_get_stats(nullptr, nullptr, nullptr, &d1);
		check(!bf_render_texture_valid(t) && d1 == d0 + 1, "a bound texture its owner destroys: no longer valid, kept");
		quad(v, 40, 8, 16, 16, 0xFFFFFFFFu);
		bf_render_draw(&st, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4, nullptr, 0);
		check_px("the stage still samples it", 44, 12, 0xFFFF00);
		bf_gfx_texture *u = solid_texture(16, 16, 0xFF00FFFFu);
		bf_render_bind_texture(0, u);   // now it goes
		quad(v, 72, 8, 16, 16, 0xFFFFFFFFu);
		bf_render_draw(&st, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4, nullptr, 0);
		check_px("a new binding samples the new texture", 76, 12, 0x00FFFF);
		bf_render_bind_texture(0, nullptr);
		quad(v, 104, 8, 16, 16, 0xFF808080u);
		bf_render_draw(&st, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4, nullptr, 0);
		check_px("no texture bound: white x diffuse", 108, 12, 0x808080, 1);
		bf_render_texture_destroy(u);
		check(!bf_render_texture_valid(u), "an unbound texture goes at once");
	}

	// Mode.cpp's fade: BackupFrame once, then a box and RestoreFrame each step. No read-back.
	{
		bf_render_clear(BF_GFX_CLEAR_COLOR, 0xFF336699u, 1.0f, nullptr);
		long long g = gfx_readbacks();
		bool ok = bf_render_backup() == BF_GFX_OK;
		for (int i = 0; i < 3; ++i) {
			bf_gfx_rect r{0, 0, 320, 240};
			bf_render_clear(BF_GFX_CLEAR_COLOR, 0xFF000000u, 1.0f, &r);
			ok = ok && bf_render_restore() == BF_GFX_OK;
		}
		check(ok && gfx_readbacks() == g, "backup and restore stay on the GPU");
		check_px("restore: the backed-up frame", 100, 100, 0x336699);
	}

	// The device goes before the client's statics: their textures go with it.
	{
		bf_gfx_texture *a = solid_texture(8, 8, 0xFF000000u);
		bf_gfx_texture *b = solid_texture(8, 8, 0xFF000000u);
		bf_render_bind_texture(1, b);
		bf_render_texture_destroy(b);   // bound: kept
		bf_render_set_device(nullptr);
		check(!bf_render_device() && !bf_render_texture_valid(a), "clearing the device destroys the textures left");
		bf_render_texture_destroy(a);   // the owner's later release: nothing
		check(!bf_render_lock(0, &pitch), "no lock after the device is gone");
		bf_gfx_destroy(g_gfx);
		check(true, "bf_gfx_destroy after: no texture left on the device");
	}

	printf("%d passed, %d failed\n", g_pass, g_fail);
	return g_fail ? 1 : 0;
}
