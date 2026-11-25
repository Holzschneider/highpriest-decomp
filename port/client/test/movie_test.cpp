// bifrost-movie-test: the movie player (port/client/include/bf_movie.h)
// through the client's call sequences (src/movie/CBink.cpp: InitOpenning,
// OpenningMain's loop of bf_movie_wait and BinkShowNextFrame, Pause, Break's
// goto, CloseOpenning), copying into a buffer of its own. One line per
// check; the exit status is the number of failures. Moved from the test of
// the Bink adapter (port/compat/bink, deleted in M11.10); the window's
// stretch of the movie is the DirectDraw adapter's and is not checked here.
//
// The clock is the test's (bf_clock_set_source), so the timing checks are
// exact; the sound engine runs OFFLINE, so voices start without a device.
// Movies: the public sample movies under <game folder>/test/bink/samples/
// (original.bik has sound) and data\video\ad\01.bik if it was extracted
// there; without them the movie checks say "skipped".

#include "bf_movie.h"

#include "bf_audio.h"
#include "bf_bink.h"
#include "bf_clock.h"
#include "bf_file.h"
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

static std::string fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
static std::string fmt(const char *f, ...)
{
	char b[512];
	va_list ap;
	va_start(ap, f);
	vsnprintf(b, sizeof b, f, ap);
	va_end(ap);
	return b;
}

// ---- the test's clock
static uint64_t g_now = 1000000;
static uint64_t now_us(void *) { return g_now; }
static void sleep_us(void *, uint64_t us) { g_now += us; }

// The client's back buffer: 640x480, 32-bit (C3dDevice's, X8R8G8B8).
static const int W = 640, H = 480;
static std::vector<uint32_t> g_back((size_t)W * H);

// CBink::BinkShowNextFrame as written, minus the Blt to the window.
// before_blt receives the movie's rectangle of the back buffer.
static int show_next_frame(bf_movie *movie, std::vector<uint32_t> *before_blt)
{
	bf_movie_decode(movie);
	bf_movie_copy(movie, g_back.data(), W * 4, H, 0, 0, BF_MOVIE_BGRA8888);
	if (before_blt) {
		bf_movie_state st;
		bf_movie_get_state(&st);
		before_blt->clear();
		for (int y = 0; y < st.height; ++y)
			for (int x = 0; x < st.width; ++x)
				before_blt->push_back(g_back[(size_t)y * W + x] | 0xFF000000u);
	}
	if (bf_movie_frame(movie) == bf_movie_frames(movie))
		return 0;
	bf_movie_next(movie);
	return 1;
}

// The picture of a frame (0-based) as BGRA, from a reader of its own.
static std::vector<uint32_t> reference_frame(const std::string &path, int frame)
{
	std::vector<uint32_t> v;
	bf_bink *b = bf_bink_open_file(path.c_str());
	if (!b)
		return v;
	bf_bink_info in;
	bf_bink_get_info(b, &in);
	bf_bink_seek(b, (uint32_t)frame);
	bf_bink_decode_next(b);
	v.resize((size_t)in.width * in.height);
	bf_bink_copy_pixels(b, v.data(), (int)in.width * 4, (int)in.width, (int)in.height, 0, 0, BF_BINK_BGRA8888);
	for (uint32_t &p : v)
		p |= 0xFF000000u;
	bf_bink_close(b);
	return v;
}

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
static int play(bf_movie *movie, int stop_after, int pause_at, uint64_t *paused_wait, uint64_t *elapsed)
{
	uint64_t t0 = g_now;
	int shown = 0;
	for (;;) {
		if (!bf_movie_wait(movie)) {
			++shown;
			if (!show_next_frame(movie, nullptr) || shown == stop_after)
				break;
			if (shown == pause_at) {
				bf_movie_pause(movie, 1);
				uint64_t p0 = g_now;
				bool waited = true;
				for (int i = 0; i < 100; ++i) {   // 100 ms in pause: the wait holds
					waited = waited && bf_movie_wait(movie);
					bf_clock_sleep_us(1000);
				}
				*paused_wait = waited ? g_now - p0 : 0;
				bf_movie_pause(movie, 0);
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
	bf_movie *none = bf_movie_open("no_such_movie.bik");
	check(!none && strstr(bf_movie_error(), "not found"), "an absent movie gives NULL", bf_movie_error());
	char junk[64] = "not a movie";
	check(!bf_movie_open_memory(junk, sizeof junk, "junk"), "a buffer that is no movie is refused");
	bf_movie_state st;
	bf_movie_get_state(&st);
	check(st.opened == 0 && !st.playing, "nothing counts as opened");
	bf_movie_close(nullptr);
	check(bf_movie_wait(nullptr) == 0 && bf_movie_frames(nullptr) == 0
			&& bf_movie_copy(nullptr, g_back.data(), W * 4, H, 0, 0, BF_MOVIE_BGRA8888) == -1,
		"NULL handles are harmless");

	std::string samples = std::string(bf_file_location(BF_LOCATION_GAME)) + "/test/bink/samples";
	std::string movie = samples + "/original.bik";
	if (!exists(movie)) {
		printf("skipped movie checks: %s absent\n", movie.c_str());
		printf("%d failure(s)\n", g_failures);
		return g_failures;
	}

	// ---- InitOpenning, OpenningMain, CloseOpenning: the whole movie
	bf_movie_use_sound(1);
	bf_movie *b = bf_movie_open(movie.c_str());
	bf_movie_get_state(&st);
	check(b && bf_movie_frames(b) == 280 && st.width == 512 && st.height == 384 && bf_movie_frame(b) == 1,
	      "open: original.bik, 512x384, 280 frames, frame 1 next",
	      b ? fmt("%dx%d, %u frames", st.width, st.height, bf_movie_frames(b)).c_str() : bf_movie_error());
	if (!b)
		return g_failures + 1;
	check(st.playing && st.opened == 1 && st.frames == 280 && st.sound && st.sound_seconds > 11.1
	          && st.sound_seconds < 11.3,
	      "state: playing, sound track read ahead, cut to the picture's length",
	      fmt("%.3f s of sound", st.sound_seconds).c_str());
	check(bf_movie_copy(b, g_back.data(), W * 4, H, 0, 0, 7) == -1, "an unknown pixel format is refused");
	check(bf_movie_wait(b) == 0, "wait: the first frame is due at once");

	// Frame 1 drawn: the back buffer holds the picture at 1:1.
	std::vector<uint32_t> got;
	show_next_frame(b, &got);
	std::vector<uint32_t> want = reference_frame(movie, 0);
	check(!want.empty() && got == want, "frame 1 is copied into the back buffer exactly");
	bf_movie_get_state(&st);
	check(st.voice != 0 && voices_labelled(movie.c_str()) == 1, "the sound starts with frame 1, tagged with the movie",
	      fmt("voice %u", st.voice).c_str());
	check(bf_movie_wait(b) == 1, "wait: frame 2 is not due yet");
	g_now += 40000;   // 25 fps
	check(bf_movie_wait(b) == 0, "wait: frame 2 is due 40 ms after frame 1");

	uint64_t paused = 0, elapsed = 0;
	int shown = 1 + play(b, 0, 99, &paused, &elapsed);
	check(shown == 280 && bf_movie_last_frame(b) == 280, "every frame is shown, the last one ends the loop",
	      fmt("%d shown", shown).c_str());
	// 279 periods after frame 1 (1 already waited), plus the pause.
	uint64_t expect = 278 * 40000 + paused;
	check(paused >= 100000 && elapsed >= expect && elapsed <= expect + 1000,
	      "the movie takes its length plus the pause",
	      fmt("%.3f s, expected %.3f s (pause %.3f s)", elapsed / 1e6, expect / 1e6, paused / 1e6).c_str());
	check(voices_labelled(movie.c_str()) == 2, "pause stops the sound, resuming starts the rest");

	bf_movie_close(b);
	bf_movie_get_state(&st);
	check(!st.playing && st.closed == 1 && st.ended && st.last_frame == 280,
	      "close: the movie counts as ended at frame 280");

	// ---- Break: a key stops it early; 'g' jumps to frame 1500
	b = bf_movie_open(movie.c_str());
	play(b, 10, 0, &paused, &elapsed);
	bf_movie_goto(b, 1500);
	check(bf_movie_frame(b) == 280, "goto 1500 is clamped to the last frame");
	bf_movie_decode(b);
	check(bf_movie_last_frame(b) == 280, "the next decode decodes the last frame");
	bf_movie_goto(b, 5);
	bf_movie_decode(b);
	check(bf_movie_last_frame(b) == 5, "goto back to frame 5");
	bf_movie_close(b);
	bf_movie_get_state(&st);
	check(st.closed == 2 && !st.ended && st.last_frame == 5, "closed at frame 5: not ended");
	b = bf_movie_open(movie.c_str());
	play(b, 140, 0, &paused, &elapsed);
	show_next_frame(b, &got);   // decodes frame 141 (the loop stopped after showing 140)
	std::vector<uint32_t> want141 = reference_frame(movie, 140);
	check(!want141.empty() && got == want141, "frame 141 after 140 shown is copied exactly");
	bf_movie_close(b);
	bf_movie_get_state(&st);
	check(st.closed == 3 && !st.ended && st.last_frame == 141, "stopped after 141 frames: not ended",
	      fmt("last frame %d", st.last_frame).c_str());

	// ---- movies from memory (BF_INTRO_MOVIE from the archives)
	std::vector<uint8_t> bytes;
	if (FILE *fp = fopen(movie.c_str(), "rb")) {
		fseek(fp, 0, SEEK_END);
		bytes.resize((size_t)ftell(fp));
		fseek(fp, 0, SEEK_SET);
		if (fread(bytes.data(), 1, bytes.size(), fp) != bytes.size())
			bytes.clear();
		fclose(fp);
	}
	b = bf_movie_open_memory(bytes.data(), bytes.size(), "data\\video\\test.bik");
	bf_movie_get_state(&st);
	check(b && bf_movie_frames(b) == 280 && strcmp(st.file, "data\\video\\test.bik") == 0,
	      "a movie from memory keeps its name for the state");
	bf_movie_close(b);

	// ---- without sound no voice starts
	bf_movie_use_sound(0);
	int before = voices_labelled(movie.c_str());
	b = bf_movie_open(movie.c_str());
	play(b, 3, 0, &paused, &elapsed);
	bf_movie_get_state(&st);
	check(!st.sound && voices_labelled(movie.c_str()) == before, "no sound asked for: the movie plays silent");
	bf_movie_close(b);

	// A movie without sound: 01.bik, if extracted next to the samples.
	std::string ad = samples + "/../01.bik";
	if (exists(ad)) {
		b = bf_movie_open(ad.c_str());
		check(b && bf_movie_frames(b) == 450, "01.bik opens: 450 frames");
		bf_movie_close(b);
	}

	bf_audio_stop();
	printf("%d failure(s)\n", g_failures);
	return g_failures;
}
