// bifrost-compat-bink-test: the client's Bink call sequences (src/movie/
// CBink.cpp: InitOpenning, OpenningMain's loop of BinkWait and
// BinkShowNextFrame, Pause, Break's BinkGoto, CloseOpenning) through the SDK
// names of the adapter, drawing through the DirectDraw adapter offscreen. One
// line per check; the exit status is the number of failures.
//
// The clock is the test's (bf_clock_set_source), so the timing checks are
// exact; the sound engine runs OFFLINE, so voices start without a device.
// Movies: the public sample movies under <game folder>/test/bink/samples/
// (original.bik has sound) and data\video\ad\01.bik if it was extracted
// there; without them the movie checks say "skipped".

#include <windows.h>
#include <ddraw.h>

#include "bf_bink_sdk.h"
#include "bf_compat_bink.h"

#include "bf_audio.h"
#include "bf_bink.h"
#include "bf_clock.h"
#include "bf_compat_d3d.h"
#include "bf_file.h"
#include "bf_gfx.h"
#include "bf_trace.h"


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <string>
#include <vector>

static int g_failures = 0;

static void check(bool ok, const char *what, const char *detail = "")
{
	printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", what, *detail ? ": " : "", detail);
	if (!ok)
		++g_failures;
}

static std::string fmt(const char *f, ...)
{
	char b[512];
	va_list ap;
	va_start(ap, f);
	vsnprintf(b, sizeof b, f, ap);
	va_end(ap);
	return b;
}

// ---- fakes of the gdi adapter's surface-DC functions (the d3d adapter imports them)
extern "C" HDC bf_compat_gdi_dc_for_bitmap(void *, int, int, int, int) { return nullptr; }
extern "C" BOOL bf_compat_gdi_dc_take_dirty(HDC, RECT *out) { *out = RECT{0, 0, 0, 0}; return FALSE; }
extern "C" BOOL WINAPI DeleteDC(HDC) { return TRUE; }

// ---- the test's clock
static uint64_t g_now = 1000000;
static uint64_t now_us(void *) { return g_now; }
static void sleep_us(void *, uint64_t us) { g_now += us; }

// ---- C3dDevice: a primary and a 3D back buffer, 640x480 (C3dDevice::Init, windowed)
static bf_gfx_device *g_gfx;
static IDirectDraw7 *g_dd;
static IDirectDrawSurface7 *g_front, *g_back;
static const int W = 640, H = 480;

static bool device_init()
{
	bf_gfx_desc gd = {nullptr, W, H, 1};
	g_gfx = bf_gfx_create(&gd);
	if (!g_gfx)
		return false;
	bf_compat_d3d_set_device(g_gfx);
	if (FAILED(DirectDrawCreateEx(NULL, (VOID **)&g_dd, IID_IDirectDraw7, NULL)))
		return false;
	g_dd->SetCooperativeLevel((HWND)0x1234, DDSCL_NORMAL);
	DDSURFACEDESC2 d;
	memset(&d, 0, sizeof d);
	d.dwSize = sizeof d;
	d.dwFlags = DDSD_CAPS;
	d.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (FAILED(g_dd->CreateSurface(&d, &g_front, NULL)))
		return false;
	d.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	d.dwWidth = W;
	d.dwHeight = H;
	d.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	return SUCCEEDED(g_dd->CreateSurface(&d, &g_back, NULL));
}

static std::vector<uint32_t> read_target(int x, int y, int w, int h)
{
	std::vector<uint32_t> v((size_t)w * h);
	bf_gfx_rect r{x, y, w, h};
	bf_gfx_read_pixels(g_gfx, BF_GFX_SPACE_LOGICAL, &r, v.data(), 0);
	for (uint32_t &p : v)
		p |= 0xFF000000u;
	return v;
}

// CBink::BinkShowNextFrame as written, with the client rect 640x480 at (0,0).
// before_blt receives the back buffer's movie rectangle as the copy left it.
static int show_next_frame(HBINK bink, std::vector<uint32_t> *before_blt)
{
	BinkDoFrame(bink);
	DDSURFACEDESC2 ddsd;
	memset(&ddsd, 0, sizeof ddsd);
	ddsd.dwSize = sizeof ddsd;
	RECT src = {0, 0, 0x200, 0x15b};
	RECT dest = {0, 0, W, H};
	int surface_type = BinkDDSurfaceType(g_back);
	if (FAILED(g_back->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL)))
		return 0;
	BinkCopyToBuffer(bink, ddsd.lpSurface, ddsd.lPitch, ddsd.dwHeight, 0, 0, surface_type);
	g_back->Unlock(NULL);
	if (before_blt)
		*before_blt = read_target(0, 0, (int)bink->Width, (int)bink->Height);
	g_front->Blt(&dest, g_back, &src, DDBLT_WAIT, NULL);
	if (bink->FrameNum == bink->Frames)
		return 0;
	BinkNextFrame(bink);
	return 1;
}

// The picture of a frame (0-based) as BGRA, from a reader of its own.
static std::vector<uint32_t> reference_frame(const std::string &path, int frame, int *w, int *h)
{
	std::vector<uint32_t> v;
	bf_bink *b = bf_bink_open_file(path.c_str());
	if (!b)
		return v;
	bf_bink_info in;
	bf_bink_get_info(b, &in);
	bf_bink_seek(b, (uint32_t)frame);
	bf_bink_decode_next(b);
	*w = (int)in.width;
	*h = (int)in.height;
	v.resize((size_t)in.width * in.height);
	bf_bink_copy_pixels(b, v.data(), (int)in.width * 4, (int)in.width, (int)in.height, 0, 0, BF_BINK_BGRA8888);
	for (uint32_t &p : v)
		p |= 0xFF000000u;
	bf_bink_close(b);
	return v;
}

static int max_of(int a, int b) { return a > b ? a : b; }

static bool exists(const std::string &p)
{
	struct stat st;
	return stat(p.c_str(), &st) == 0;
}

static int voices_labelled(const char *label)
{
	bf_audio_voice_record r[BF_AUDIO_RECENT_VOICES];
	int n = bf_audio_recent_voices(0, r, BF_AUDIO_RECENT_VOICES);
	int k = 0;
	for (int i = 0; i < n; ++i)
		k += r[i].started && strcmp(r[i].label, label) == 0;
	return k;
}

// OpenningMain's loop: wait, show; the sleep advances the test's clock.
static int play(HBINK bink, int stop_after, int pause_at, uint64_t *paused_wait, uint64_t *elapsed)
{
	uint64_t t0 = g_now;
	int shown = 0;
	for (;;) {
		if (!BinkWait(bink)) {
			++shown;
			if (!show_next_frame(bink, nullptr) || shown == stop_after)
				break;
			if (shown == pause_at) {
				BinkPause(bink, 1);
				uint64_t p0 = g_now;
				bool waited = true;
				for (int i = 0; i < 100; ++i) {   // 100 ms in pause: BinkWait holds
					waited = waited && BinkWait(bink);
					bf_clock_sleep_us(1000);
				}
				*paused_wait = waited ? g_now - p0 : 0;
				BinkPause(bink, 0);
			}
		} else {
			bf_clock_sleep_us(500);   // BinkGoodSleepUs(500), in effect
		}
	}
	*elapsed = g_now - t0;
	return shown;
}

int main()
{
	setenv("BF_NO_ALERTS", "1", 0);
	bf_clock_source clock = {now_us, sleep_us, nullptr};
	bf_clock_set_source(&clock);
	bf_audio_config offline = {BF_AUDIO_MODE_OFFLINE, 48, 48000};
	check(bf_audio_start(&offline) == BF_AUDIO_MODE_OFFLINE, "the offline sound engine starts");

	// ---- missing data: the client goes on
	HBINK none = BinkOpen("no_such_movie.bik", 0);
	check(!none && strstr(BinkGetError(), "not found"), "BinkOpen of an absent movie returns NULL",
	      BinkGetError());
	char junk[64] = "not a movie";
	check(!BfBinkOpenMemory(junk, sizeof junk, "junk"), "a buffer that is no movie is refused");
	bf_compat_bink_state st;
	bf_compat_bink_get_state(&st);
	check(st.opened == 0 && !st.playing, "nothing counts as opened");
	check(BinkWait(nullptr) == 0 && BinkDDSurfaceType(nullptr) == -1, "NULL handles are harmless");

	if (!device_init()) {
		printf("FAIL no Metal device\n");
		return 1;
	}
	check(BinkDDSurfaceType(g_back) == BINKSURFACE32, "BinkDDSurfaceType: the back buffer is BINKSURFACE32");

	std::string samples = std::string(bf_file_location(BF_LOCATION_GAME)) + "/test/bink/samples";
	std::string movie = samples + "/original.bik";
	if (!exists(movie)) {
		printf("skipped movie checks: %s absent\n", movie.c_str());
		printf("%d failure(s)\n", g_failures);
		return g_failures;
	}

	// ---- InitOpenning, OpenningMain, CloseOpenning: the whole movie
	BinkSoundUseDirectSound(0);
	HBINK b = BinkOpen(movie.c_str(), 0);
	check(b && b->Frames == 280 && b->Width == 512 && b->Height == 384 && b->FrameNum == 1,
	      "BinkOpen: original.bik, 512x384, 280 frames, FrameNum 1",
	      b ? fmt("%ux%u, %u frames", b->Width, b->Height, b->Frames).c_str() : BinkGetError());
	if (!b)
		return g_failures + 1;
	bf_compat_bink_get_state(&st);
	check(st.playing && st.opened == 1 && st.frames == 280 && st.sound && st.sound_seconds > 11.1
	          && st.sound_seconds < 11.3,
	      "state: playing, sound track read ahead, cut to the picture's length",
	      fmt("%.3f s of sound", st.sound_seconds).c_str());
	check(BinkWait(b) == 0, "BinkWait: the first frame is due at once");

	// Frame 1 drawn: the back buffer holds the picture at 1:1 before the Blt.
	std::vector<uint32_t> got;
	show_next_frame(b, &got);
	int rw = 0, rh = 0;
	std::vector<uint32_t> want = reference_frame(movie, 0, &rw, &rh);
	check(!want.empty() && got == want, "frame 1 is copied into the back buffer exactly");
	std::vector<uint32_t> win;
	bf_compat_bink_get_state(&st);
	check(st.voice != 0 && voices_labelled(movie.c_str()) == 1, "the sound starts with frame 1, tagged with the movie",
	      fmt("voice %u", st.voice).c_str());
	check(BinkWait(b) == 1, "BinkWait: frame 2 is not due yet");
	g_now += 40000;   // 25 fps
	check(BinkWait(b) == 0, "BinkWait: frame 2 is due 40 ms after frame 1");

	uint64_t paused = 0, elapsed = 0;
	int shown = 1 + play(b, 0, 99, &paused, &elapsed);
	check(shown == 280 && b->LastFrameNum == 280, "every frame is shown, the last one ends the loop",
	      fmt("%d shown", shown).c_str());
	// 279 periods after frame 1 (1 already waited), plus the pause.
	uint64_t expect = 278 * 40000 + paused;
	check(paused >= 100000 && elapsed >= expect && elapsed <= expect + 1000,
	      "the movie takes its length plus the pause",
	      fmt("%.3f s, expected %.3f s (pause %.3f s)", elapsed / 1e6, expect / 1e6, paused / 1e6).c_str());
	check(voices_labelled(movie.c_str()) == 2, "pause stops the sound, resuming starts the rest");

	BinkClose(b);
	bf_compat_bink_get_state(&st);
	check(!st.playing && st.closed == 1 && st.ended && st.last_frame == 280,
	      "BinkClose: the movie counts as ended at frame 280");

	// ---- Break: a key stops it early; 'g' jumps with BinkGoto(1500, QUICK)
	b = BinkOpen(movie.c_str(), 0);
	play(b, 10, 0, &paused, &elapsed);
	BinkGoto(b, 1500, BINKGOTOQUICK);
	check(b->FrameNum == 280, "BinkGoto(1500) is clamped to the last frame");
	BinkDoFrame(b);
	check(b->LastFrameNum == 280, "the next BinkDoFrame decodes the last frame");
	BinkGoto(b, 5, 0);
	BinkDoFrame(b);
	check(b->LastFrameNum == 5, "BinkGoto back to frame 5");
	BinkClose(b);
	bf_compat_bink_get_state(&st);
	check(st.closed == 2 && !st.ended && st.last_frame == 5, "closed at frame 5: not ended");
	b = BinkOpen(movie.c_str(), 0);
	play(b, 140, 0, &paused, &elapsed);
	// The window shows frame 140's rectangle 512x347 (the client's source
	// rectangle) stretched over 640x480: 25 points against the picture.
	want = reference_frame(movie, 139, &rw, &rh);
	win = read_target(0, 0, W, H);
	int close_pts = 0;
	std::string worst;
	for (int i = 0; i < 5; ++i)
		for (int j = 0; j < 5; ++j) {
			int X = 64 + i * 128, Y = 48 + j * 96;
			int sx = X * 512 / W, sy = Y * 347 / H;
			uint32_t a = win[(size_t)Y * W + X], r = want[(size_t)sy * rw + sx];
			int diff = 0;
			for (int s = 0; s < 24; s += 8)
				diff = max_of(diff, abs((int)((a >> s) & 255) - (int)((r >> s) & 255)));
			close_pts += diff <= 32;
			if (diff > 32)
				worst = fmt("(%d,%d) %08x vs %08x", X, Y, a, r);
		}
	check(close_pts >= 22, "the window shows the movie's rectangle stretched over it (frame 140, 25 points)",
	      fmt("%d of 25 close %s", close_pts, worst.c_str()).c_str());
	BinkClose(b);
	bf_compat_bink_get_state(&st);
	check(st.closed == 3 && !st.ended && st.last_frame == 140, "stopped after 140 frames: not ended",
	      fmt("last frame %d", st.last_frame).c_str());

	// ---- BINKFROMMEMORY and the port's BfBinkOpenMemory
	std::vector<uint8_t> bytes;
	if (FILE *fp = fopen(movie.c_str(), "rb")) {
		fseek(fp, 0, SEEK_END);
		bytes.resize((size_t)ftell(fp));
		fseek(fp, 0, SEEK_SET);
		fread(bytes.data(), 1, bytes.size(), fp);
		fclose(fp);
	}
	b = BinkOpen((const char *)bytes.data(), BINKFROMMEMORY);
	check(b && b->Frames == 280, "BinkOpen with BINKFROMMEMORY reads the size from the header");
	if (b)
		BinkClose(b);
	b = BfBinkOpenMemory(bytes.data(), bytes.size(), "data\\video\\test.bik");
	bf_compat_bink_get_state(&st);
	check(b && strcmp(st.file, "data\\video\\test.bik") == 0, "BfBinkOpenMemory keeps the name for the state");
	if (b)
		BinkClose(b);

	// ---- without a sound system no voice starts
	BinkSetSoundSystem(nullptr, 0);
	int before = voices_labelled(movie.c_str());
	b = BinkOpen(movie.c_str(), 0);
	play(b, 3, 0, &paused, &elapsed);
	bf_compat_bink_get_state(&st);
	check(!st.sound && voices_labelled(movie.c_str()) == before, "no sound system: the movie plays silent");
	BinkClose(b);

	// A movie without sound: 01.bik, if extracted next to the samples.
	std::string ad = samples + "/../01.bik";
	if (exists(ad)) {
		b = BinkOpen(ad.c_str(), 0);
		check(b && b->Frames == 450, "01.bik opens: 450 frames");
		if (b)
			BinkClose(b);
	}

	g_back->Release();
	g_front->Release();
	g_dd->Release();
	bf_compat_d3d_set_device(nullptr);
	bf_gfx_destroy(g_gfx);
	bf_audio_stop();
	printf("%d failure(s)\n", g_failures);
	return g_failures;
}
