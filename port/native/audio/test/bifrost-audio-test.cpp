// bifrost-audio-test: checks the audio module without game data, and with
// it when BF_CLIENT_DIR (default ~/Games/<app>) holds BGM/01.mp3 and
// data.grf. One line per check; exit status 1 on any failure.
//
//   bifrost-audio-test            all sections
//   BF_AUDIO_TEST_ALL=1           also decode every readable .wav in the archives
//   BF_TRACE=snd                  the module's trace lines

#include "bf_audio.h"
#include "bf_trace.h"
#include "grf.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <vector>

#ifndef BF_APP_NAME
#error BF_APP_NAME comes from port/name.cmake
#endif

extern "C" int bf_audio_header_c_check(void);
int bf_audio_header_cxx14_check();

static int g_failures = 0;

static void line(const char *status, const char *fmt, va_list ap)
{
	printf("%-7s ", status);
	vprintf(fmt, ap);
	printf("\n");
	fflush(stdout);
}

static bool check(bool ok, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	line(ok ? "ok" : "FAIL", fmt, ap);
	va_end(ap);
	if (!ok) g_failures++;
	return ok;
}

static void info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	line("info", fmt, ap);
	va_end(ap);
}

static void skipped(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	line("skipped", fmt, ap);
	va_end(ap);
}

static void msleep(int ms) { usleep(ms * 1000); }

// ---- synthetic WAVs -------------------------------------------------------

static void put32(std::vector<uint8_t> &v, uint32_t x) { for (int i = 0; i < 4; ++i) v.push_back((uint8_t)(x >> (8 * i))); }
static void put16(std::vector<uint8_t> &v, uint16_t x) { v.push_back((uint8_t)x); v.push_back((uint8_t)(x >> 8)); }

// The sample a synthetic WAV holds at frame i, channel c, as the decoder must
// return it (16-bit).
static int16_t pattern(int bits, int64_t i, int c)
{
	if (bits == 8) {
		uint8_t u = (uint8_t)((i * 7 + c * 50) & 255);
		return (int16_t)((u - 128) * 256);
	}
	return (int16_t)(((i * 263 + c * 1000) % 65536) - 32768);
}

static std::vector<uint8_t> make_wav(int bits, int channels, int rate, int64_t frames, bool tone = false, double freq = 440)
{
	std::vector<uint8_t> v;
	uint32_t align = channels * bits / 8, data = (uint32_t)(frames * align);
	v.insert(v.end(), { 'R', 'I', 'F', 'F' });
	put32(v, 36 + data);
	v.insert(v.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
	put32(v, 16);
	put16(v, 1);
	put16(v, (uint16_t)channels);
	put32(v, rate);
	put32(v, rate * align);
	put16(v, (uint16_t)align);
	put16(v, (uint16_t)bits);
	v.insert(v.end(), { 'd', 'a', 't', 'a' });
	put32(v, data);
	for (int64_t i = 0; i < frames; ++i)
		for (int c = 0; c < channels; ++c) {
			int16_t s = tone ? (int16_t)(16000 * sin(2 * M_PI * freq * i / rate)) : pattern(bits, i, c);
			if (bits == 8) v.push_back((uint8_t)((s >> 8) + 128));
			else put16(v, (uint16_t)s);
		}
	return v;
}


// IMA ADPCM (WAVE format 0x11), mono, from the public algorithm: the
// encoder's own reconstruction is what any correct decoder must return.
static std::vector<uint8_t> make_ima_wav(int rate, const std::vector<int16_t> &in, std::vector<int16_t> &expect, int64_t &frames)
{
	static const int steps[89] = { 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
		73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
		724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
		4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
		24623, 27086, 29794, 32767 };
	static const int adjust[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
	const int align = 256, per_block = (align - 4) * 2 + 1;
	size_t blocks = in.size() / per_block;
	frames = (int64_t)(blocks * per_block);
	std::vector<uint8_t> data;
	expect.clear();
	int index = 0;
	for (size_t b = 0; b < blocks; ++b) {
		const int16_t *x = in.data() + b * per_block;
		int pred = x[0];
		put16(data, (uint16_t)(int16_t)pred);
		data.push_back((uint8_t)index);
		data.push_back(0);
		expect.push_back((int16_t)pred);
		uint8_t byte = 0;
		for (int i = 1; i < per_block; ++i) {
			int step = steps[index], diff = x[i] - pred, code = 0;
			if (diff < 0) { code = 8; diff = -diff; }
			int vp = step >> 3;
			if (diff >= step) { code |= 4; diff -= step; vp += step; }
			step >>= 1;
			if (diff >= step) { code |= 2; diff -= step; vp += step; }
			step >>= 1;
			if (diff >= step) { code |= 1; vp += step; }
			pred += (code & 8) ? -vp : vp;
			pred = pred < -32768 ? -32768 : (pred > 32767 ? 32767 : pred);
			index += adjust[code & 7];
			index = index < 0 ? 0 : (index > 88 ? 88 : index);
			expect.push_back((int16_t)pred);
			if (i & 1) byte = (uint8_t)code;
			else data.push_back((uint8_t)(byte | (code << 4)));
		}
	}
	std::vector<uint8_t> v;
	v.insert(v.end(), { 'R', 'I', 'F', 'F' });
	put32(v, (uint32_t)(4 + 28 + 12 + 8 + data.size()));
	v.insert(v.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
	put32(v, 20);
	put16(v, 0x11);
	put16(v, 1);
	put32(v, rate);
	put32(v, rate * align / per_block);
	put16(v, align);
	put16(v, 4);
	put16(v, 2);
	put16(v, per_block);
	v.insert(v.end(), { 'f', 'a', 'c', 't' });
	put32(v, 4);
	put32(v, (uint32_t)frames);
	v.insert(v.end(), { 'd', 'a', 't', 'a' });
	put32(v, (uint32_t)data.size());
	v.insert(v.end(), data.begin(), data.end());
	return v;
}

static bf_audio_sound *tone(int rate, int channels, double seconds, double freq, double amp = 0.5)
{
	int64_t n = (int64_t)(rate * seconds);
	std::vector<int16_t> pcm((size_t)n * channels);
	for (int64_t i = 0; i < n; ++i)
		for (int c = 0; c < channels; ++c)
			pcm[i * channels + c] = (int16_t)(32767 * amp * sin(2 * M_PI * freq * i / rate));
	return bf_audio_sound_create(pcm.data(), n, channels, rate);
}

static bf_audio_counters counters()
{
	bf_audio_counters c;
	bf_audio_get_counters(&c);
	return c;
}

static void test_decode()
{
	struct Case { int bits, channels, rate; } cases[] = { { 8, 1, 11025 }, { 16, 2, 22050 }, { 16, 1, 44100 } };
	for (const Case &k : cases) {
		int64_t frames = k.rate;   // one second
		std::vector<uint8_t> wav = make_wav(k.bits, k.channels, k.rate, frames);
		bf_audio_info in;
		int q = bf_audio_query(wav.data(), wav.size(), &in);
		check(q == 0 && in.kind == BF_AUDIO_KIND_PCM_WAV && in.format_tag == 1 && in.sample_rate == k.rate
		      && in.channels == k.channels && in.bits == k.bits && in.frames == frames && in.frames_exact && !in.builtin,
		      "query %d-bit %s %d Hz: kind %d, %d Hz, %d ch, %d bit, %lld frames", k.bits,
		      k.channels == 1 ? "mono" : "stereo", k.rate, in.kind, in.sample_rate, in.channels, in.bits, (long long)in.frames);
		std::vector<int16_t> pcm((size_t)frames * k.channels + 16, 0x7777);
		int64_t n = bf_audio_decode(wav.data(), wav.size(), pcm.data(), frames);
		int64_t bad = 0;
		for (int64_t i = 0; i < frames; ++i)
			for (int c = 0; c < k.channels; ++c)
				if (pcm[i * k.channels + c] != pattern(k.bits, i, c)) bad++;
		check(n == frames && bad == 0 && pcm[frames * k.channels] == 0x7777,
		      "decode %d-bit %s %d Hz: %lld frames, %lld wrong samples, no overrun", k.bits,
		      k.channels == 1 ? "mono" : "stereo", k.rate, (long long)n, (long long)bad);
		int64_t half = bf_audio_decode(wav.data(), wav.size(), pcm.data(), frames / 2);
		check(half == frames / 2, "decode stops at max_frames (%lld)", (long long)half);
	}

	// A broken header the game's files are known for: RIFF and data sizes
	// larger than the file. Decoded by AudioToolbox or the built-in reader.
	std::vector<uint8_t> wav = make_wav(16, 1, 22050, 2205);
	wav[4] = wav[5] = wav[6] = 0xff;
	wav[40] = wav[41] = 0xff;
	int64_t frames = 0;
	bf_audio_info in;
	int16_t *pcm = bf_audio_decode_alloc(wav.data(), wav.size(), &in, &frames);
	check(pcm && frames == 2205 && pcm[100] == pattern(16, 100, 0),
	      "oversized RIFF and data sizes: %lld frames%s", (long long)frames, pcm && in.builtin ? " (built-in reader)" : "");
	bf_audio_free(pcm);

	// A RIFF size of 0: AudioToolbox refuses it, the built-in reader takes it.
	wav = make_wav(8, 1, 11025, 1000);
	wav[4] = wav[5] = wav[6] = wav[7] = 0;
	long long fallbacks = counters().decode_fallbacks;
	pcm = bf_audio_decode_alloc(wav.data(), wav.size(), &in, &frames);
	bool same = pcm && frames == 1000;
	for (int i = 0; same && i < 1000; ++i) same = pcm[i] == pattern(8, i, 0);
	check(same && in.builtin && counters().decode_fallbacks == fallbacks + 1,
	      "RIFF size 0: decoded by the built-in reader, %lld frames, values match, counted", (long long)frames);
	bf_audio_free(pcm);

	// IMA ADPCM, as Wave.cpp's ADPCM branch would meet it.
	std::vector<int16_t> src(22050), expect;
	for (size_t i = 0; i < src.size(); ++i) src[i] = (int16_t)(12000 * sin(2 * M_PI * 440 * i / 22050.0) + 3000 * sin(2 * M_PI * 1250 * i / 22050.0));
	int64_t imaframes = 0;
	wav = make_ima_wav(22050, src, expect, imaframes);
	pcm = bf_audio_decode_alloc(wav.data(), wav.size(), &in, &frames);
	int64_t off = 0;
	for (int64_t i = 0; pcm && i < std::min(frames, imaframes); ++i)
		if (abs(pcm[i] - expect[i]) > 1) off++;
	check(pcm && in.kind == BF_AUDIO_KIND_ADPCM_WAV && in.format_tag == 0x11 && frames == imaframes && off == 0,
	      "IMA ADPCM WAV: kind %d, %lld of %lld frames, %lld samples off by more than 1", in.kind, (long long)frames,
	      (long long)imaframes, (long long)off);
	bf_audio_free(pcm);

	long long fails = counters().decode_failures;
	const char junk[] = "this is not a sound file at all";
	check(bf_audio_query(junk, sizeof junk, &in) == -1 && bf_audio_file_kind(junk, sizeof junk) == BF_AUDIO_KIND_UNKNOWN
	      && counters().decode_failures == fails + 1, "junk is refused and counted as a decode failure");
	uint8_t mp3head[4] = { 0xff, 0xfb, 0x90, 0x00 };
	check(bf_audio_file_kind(mp3head, 4) == BF_AUDIO_KIND_MP3 && bf_audio_file_kind("ID3\x03\0\0\0\0\0\x10", 10) == BF_AUDIO_KIND_MP3,
	      "MP3 frame sync and ID3 tag are recognised");
	check(bf_audio_header_c_check() == (int)sizeof(bf_audio_counters) && bf_audio_header_cxx14_check() == 13,
	      "the header compiles as C and as C++14");
}

// ---- offline rendering ------------------------------------------------------

struct Level { double l, r; };

static Level rms(const std::vector<float> &b, size_t from = 0)
{
	double l = 0, r = 0;
	size_t n = 0;
	for (size_t i = from * 2; i + 1 < b.size(); i += 2, ++n) { l += b[i] * b[i]; r += b[i + 1] * b[i + 1]; }
	if (!n) return { 0, 0 };
	return { sqrt(l / n), sqrt(r / n) };
}

static std::vector<float> render(double seconds)
{
	int64_t n = (int64_t)(bf_audio_sample_rate() * seconds);
	std::vector<float> b((size_t)n * 2);
	int64_t got = bf_audio_render(b.data(), n);
	if (got != n) check(false, "render %lld frames returned %lld", (long long)n, (long long)got);
	return b;
}

// Completion is reported by AVFoundation on its own thread; give it a moment.
static bool wait_done(bf_audio_voice v)
{
	for (int i = 0; i < 50 && bf_audio_voice_playing(v); ++i) msleep(4);
	return !bf_audio_voice_playing(v);
}

static std::string g_builddir;

static void test_offline()
{
	bf_audio_config cfg = { BF_AUDIO_MODE_OFFLINE, 4, 44100 };
	int mode = bf_audio_start(&cfg);
	if (!check(mode == BF_AUDIO_MODE_OFFLINE && bf_audio_sample_rate() == 44100, "offline engine starts (mode %d, %d Hz)", mode, bf_audio_sample_rate()))
		return;
	check(bf_audio_start(&cfg) == -1, "a second start is refused");
	bf_audio_counters c0 = counters();

	Level quiet = rms(render(0.1));
	check(quiet.l == 0 && quiet.r == 0, "nothing playing renders silence");

	bf_audio_sound *s = tone(22050, 1, 0.5, 441);
	bf_audio_voice v = bf_audio_play(s, 1, 0, 0);
	Level centre = rms(render(0.1));
	check(v != 0 && bf_audio_voice_playing(v) && centre.l > 0.1 && centre.r > 0.1,
	      "a voice produces samples: rms L %.3f R %.3f", centre.l, centre.r);
	bf_audio_voice_stop(v);
	check(!bf_audio_voice_playing(v), "stop ends the voice");
	Level after = rms(render(0.05), 512);
	check(after.l < 1e-4 && after.r < 1e-4, "a stopped voice is silent: rms %.5f", after.l);

	v = bf_audio_play(s, 1, -1, 0);
	Level left = rms(render(0.2), 4096);
	check(left.l > 0.1 && left.r < 0.01 * left.l, "pan -1 is left only: rms L %.3f R %.4f", left.l, left.r);
	bf_audio_voice_set_pan(v, 1);
	Level right = rms(render(0.2), 4096);
	check(right.r > 0.1 && right.l < 0.01 * right.r, "pan changed while playing to +1: rms L %.4f R %.3f", right.l, right.r);
	bf_audio_stop_all_voices();

	v = bf_audio_play(s, 0.5f, 0, 0);
	Level halfv = rms(render(0.1));
	double ratio = halfv.l / centre.l;
	check(ratio > 0.4 && ratio < 0.6, "volume 0.5 scales the level by %.3f", ratio);
	bf_audio_voice_set_volume(v, 0);
	Level zero = rms(render(0.1), 1024);
	check(zero.l < 1e-3, "volume set to 0 while playing: rms %.5f", zero.l);
	bf_audio_stop_all_voices();

	bf_audio_set_effects_volume(0);
	v = bf_audio_play(s, 1, 0, 0);
	Level master0 = rms(render(0.1));
	bf_audio_set_effects_volume(1);
	Level master1 = rms(render(0.1), 1024);
	check(master0.l < 1e-3 && master1.l > 0.1, "effects master volume 0 then 1: rms %.4f then %.3f", master0.l, master1.l);
	bf_audio_stop_all_voices();

	bf_audio_sound *shortone = tone(22050, 1, 0.1, 441);
	v = bf_audio_play(shortone, 1, 0, 0);
	render(0.3);
	check(wait_done(v), "a non-looping voice ends after its length has rendered");
	v = bf_audio_play(shortone, 1, 0, 1);
	std::vector<float> looped = render(0.5);
	Level tail = rms(looped, (size_t)(0.4 * 44100));
	check(bf_audio_voice_playing(v) && tail.l > 0.1, "a looping 0.1 s voice still sounds after 0.5 s: rms %.3f", tail.l);
	bf_audio_voice_stop(v);

	bf_audio_sound *wide = tone(48000, 2, 0.2, 1000);
	long long rc = counters().conversions;
	v = bf_audio_play(wide, 1, 0, 0);
	Level st = rms(render(0.1));
	check(st.l > 0.1 && st.r > 0.1 && counters().conversions == rc + 1,
	      "a 48 kHz stereo sound plays: rms L %.3f R %.3f (converted once)", st.l, st.r);
	bf_audio_stop_all_voices();

	// After the engine has run a while a voice must still start at once (the
	// rate converter's own delay is about 64 frames) and, converted from
	// 22050 Hz, keep its pitch.
	render(5.0);
	bf_audio_sound *pitch = tone(22050, 1, 0.5, 441);
	v = bf_audio_play(pitch, 1, 0, 0);
	std::vector<float> pb = render(0.6);
	int first = -1, last = -1, crossings = 0;
	for (size_t i = 0; i < pb.size() / 2; ++i)
		if (fabsf(pb[2 * i]) > 0.01f) { if (first < 0) first = (int)i; last = (int)i; }
	for (int i = first; first >= 0 && i < last; ++i)
		if (pb[2 * i] < 0 && pb[2 * i + 2] >= 0) crossings++;
	double hz = last > first ? crossings / ((last - first) / 44100.0) : 0;
	check(first >= 0 && first < 128 && fabs((last - first) / 44100.0 - 0.5) < 0.01 && fabs(hz - 441) < 3,
	      "after 5 s a 22050 Hz voice starts at frame %d, lasts %.3f s, %.1f Hz", first, (last - first) / 44100.0, hz);
	bf_audio_sound_destroy(pitch);

	// Stealing: four voices, all busy with one-second sounds.
	bf_audio_sound *longone = tone(22050, 1, 1.0, 441);
	bf_audio_voice vs[4];
	for (int i = 0; i < 4; ++i) { vs[i] = bf_audio_play(longone, 1, 0, 0); render(0.01); }
	check(bf_audio_voices_playing() == 4, "four voices sound at once (%d)", bf_audio_voices_playing());
	long long stolen = counters().voices_stolen;
	bf_audio_voice fifth = bf_audio_play(longone, 1, 0, 0);
	check(fifth && counters().voices_stolen == stolen + 1 && !bf_audio_voice_playing(vs[0]) && bf_audio_voice_playing(vs[1])
	      && bf_audio_voice_playing(fifth), "a fifth voice steals the oldest one");
	bf_audio_stop_all_voices();
	for (int i = 0; i < 4; ++i) vs[i] = bf_audio_play(shortone, 1, 0, 1);
	long long rejected = counters().voices_rejected;
	bf_audio_voice none = bf_audio_play(shortone, 1, 0, 0);
	check(none == 0 && counters().voices_rejected == rejected + 1 && bf_audio_voices_playing() == 4,
	      "looping voices are never stolen: the fifth request is refused");
	bf_audio_stop_all_voices();

	// The stream, on a generated file: loop count, passes counted, volume.
	std::string path = g_builddir + "/bf-audio-stream-test.wav";
	std::vector<uint8_t> wav = make_wav(16, 2, 22050, 4410, true, 330);   // 0.2 s
	FILE *f = fopen(path.c_str(), "wb");
	if (f) { fwrite(wav.data(), 1, wav.size(), f); fclose(f); }
	check(bf_audio_stream_open(path.c_str()) == 0 && bf_audio_stream_status() == BF_AUDIO_STREAM_OPEN
	      && fabs(bf_audio_stream_duration() - 0.2) < 1e-6, "stream opens a generated 0.2 s file (%.3f s)", bf_audio_stream_duration());
	bf_audio_stream_set_loop_count(3);
	bf_audio_stream_start();
	std::vector<float> sb = render(0.5);
	for (int i = 0; i < 100 && bf_audio_stream_status() != BF_AUDIO_STREAM_DONE; ++i) { render(0.05); msleep(2); }
	bf_audio_counters c = counters();
	Level sl = rms(sb);
	check(c.stream_status == BF_AUDIO_STREAM_DONE && c.stream_loops == 3 && sl.l > 0.1 && path == c.stream_file,
	      "loop count 3: %d passes played, status %d, rms %.3f", c.stream_loops, c.stream_status, sl.l);
	bf_audio_stream_set_loop_count(0);
	bf_audio_stream_start();
	render(1.0);
	for (int i = 0; i < 20; ++i) { render(0.01); msleep(2); }
	c = counters();
	check(c.stream_status == BF_AUDIO_STREAM_PLAYING && c.stream_loops >= 4,
	      "loop count 0 loops forever: %d passes after 1.2 s", c.stream_loops);
	{
		// The position within the pass goes up and falls back when the next pass begins.
		double p0 = bf_audio_stream_position();
		bool wrapped = false, inside = p0 >= 0 && p0 <= 0.2 + 1e-6;
		double prev = p0;
		for (int i = 0; i < 12; ++i) {
			render(0.03);
			double p = bf_audio_stream_position();
			inside = inside && p >= 0 && p <= 0.2 + 1e-6;
			if (p < prev) wrapped = true;
			prev = p;
		}
		check(inside && wrapped, "stream position stays within the 0.2 s pass and falls back on a loop (last %.3f s)", prev);
	}
	bf_audio_stream_set_volume(0.5f);
	Level s05 = rms(render(0.2), 1024);
	bf_audio_set_music_volume(0);
	Level m0 = rms(render(0.3), (size_t)(0.2 * 44100));   // the mixer ramps its output volume
	bf_audio_set_music_volume(1);
	bf_audio_stream_set_volume(1);
	Level s1 = rms(render(0.2), 1024);
	check(fabs(s05.l / s1.l - 0.5) < 0.1 && m0.l < 1e-3, "stream volume 0.5 scales by %.3f; music master 0 mutes: rms %.4f", s05.l / s1.l, m0.l);
	bf_audio_stream_pause(1);
	Level paused = rms(render(0.1), 1024);
	bf_audio_stream_pause(0);
	Level resumed = rms(render(0.1), 1024);
	check(paused.l < 1e-4 && resumed.l > 0.1 && bf_audio_stream_status() == BF_AUDIO_STREAM_PLAYING,
	      "pause silences the stream, resume continues it");
	bf_audio_stream_close();
	check(counters().stream_file[0] == 0 && bf_audio_stream_status() == BF_AUDIO_STREAM_NONE, "close forgets the file");
	long long sf = counters().stream_failures;
	check(bf_audio_stream_open((g_builddir + "/no-such-file.mp3").c_str()) == -1 && counters().stream_failures == sf + 1,
	      "a missing stream file is refused and counted");
	unlink(path.c_str());

	c = counters();
	check(c.voices_started - c0.voices_started == 17, "voices started counted: %lld", c.voices_started - c0.voices_started);
	bf_audio_sound_destroy(s);
	bf_audio_sound_destroy(shortone);
	bf_audio_sound_destroy(wide);
	bf_audio_sound_destroy(longone);
	bf_audio_stop();
	check(bf_audio_mode() == -1, "offline engine stops");
}

// A voice starts at the volume and pan given to bf_audio_play from its first
// sample, also on a slot whose last sound was loud, panned elsewhere or had
// its volume changed while playing (AUMultiChannelMixer smooths its inputs'
// volume and pan, so a reused player used to start at the old values for
// about 15 ms). The sounds are at the engine rate, so output frame i is the
// sound's frame i times a gain: for a mono sound the mixer's equal-power pan
// law, cos and sin of (pan + 1) pi / 4; for a stereo one its balance law.
static void test_start_volume()
{
	bf_audio_config cfg = { BF_AUDIO_MODE_OFFLINE, 1, 44100 };   // one voice: every play reuses it
	if (!check(bf_audio_start(&cfg) == BF_AUDIO_MODE_OFFLINE, "start volume: offline engine with one voice starts"))
		return;
	const int rate = 44100, win = rate / 200;   // 5 ms
	const int64_t n = rate / 5;                 // 0.2 s
	std::vector<int16_t> m((size_t)n), st((size_t)n * 2);
	for (int64_t i = 0; i < n; ++i) {
		m[i] = (int16_t)(16000 * sin(2 * M_PI * 1000 * i / rate));
		st[2 * i] = st[2 * i + 1] = m[i];
	}
	bf_audio_sound *mono = bf_audio_sound_create(m.data(), n, 1, rate);
	bf_audio_sound *stereo = bf_audio_sound_create(st.data(), n, 2, rate);
	// The gain of channel c in frames [from, from + count).
	auto gain = [&](const std::vector<float> &b, int c, int64_t from, int64_t count) {
		double o = 0, in = 0;
		for (int64_t i = from; i < from + count; ++i) {
			double x = m[i] / 32768.0;
			o += b[2 * i + c] * b[2 * i + c];
			in += x * x;
		}
		return in > 0 ? sqrt(o / in) : 0.0;
	};
	struct Case {
		const char *what;
		bf_audio_sound *first; float v1, p1, vmid; bool ends;   // vmid >= 0: set while the first one plays
		bf_audio_sound *second; float v2, p2;
	};
	const Case cases[] = {
		{ "loud then quiet, stopped", mono, 1, 0, -1, false, mono, 0.25f, 0 },
		{ "loud then quiet, the loud one ended", mono, 1, 0, -1, true, mono, 0.1f, 0 },
		{ "quiet then loud", mono, 0.1f, 0, -1, false, mono, 1, 0 },
		{ "loud left then quiet right", mono, 1, -1, -1, false, mono, 0.5f, 1 },
		{ "loud right then quiet at -0.5", mono, 1, 1, -1, false, mono, 0.3f, -0.5f },
		{ "loud stereo then quiet stereo at 0.5", stereo, 1, 0, -1, false, stereo, 0.25f, 0.5f },
		{ "loud mono then quiet stereo", mono, 1, 0, -1, false, stereo, 0.2f, 0 },
		{ "volume lowered while playing, then a louder sound", mono, 1, 0, 0.05f, false, mono, 0.5f, 0 },
	};
	for (const Case &k : cases) {
		bf_audio_voice v = bf_audio_play(k.first, k.v1, k.p1, 0);
		render(0.05);
		if (k.vmid >= 0) { bf_audio_voice_set_volume(v, k.vmid); render(0.05); }
		if (k.ends) render(0.3);
		else bf_audio_voice_stop(v);
		bf_audio_play(k.second, k.v2, k.p2, 0);
		std::vector<float> b = render(0.05);
		double want[2];
		if (k.second == mono) {
			want[0] = k.v2 * cos((k.p2 + 1) * M_PI / 4);
			want[1] = k.v2 * sin((k.p2 + 1) * M_PI / 4);
		} else {
			want[0] = k.v2 * (k.p2 > 0 ? 1 - k.p2 : 1);
			want[1] = k.v2 * (k.p2 < 0 ? 1 + k.p2 : 1);
		}
		double first[2] = { gain(b, 0, 0, win), gain(b, 1, 0, win) };
		double later[2] = { gain(b, 0, 1000, 1000), gain(b, 1, 1000, 1000) };
		bool ok = true;
		for (int c = 0; c < 2; ++c) {
			if (want[c] < 1e-6) ok = ok && first[c] < 1e-4 && later[c] < 1e-4;
			else ok = ok && fabs(first[c] / want[c] - 1) < 0.01 && fabs(later[c] / want[c] - 1) < 0.01;
		}
		check(ok, "start volume: %s: first 5 ms L %.4f R %.4f, later L %.4f R %.4f, expected L %.4f R %.4f",
		      k.what, first[0], first[1], later[0], later[1], want[0], want[1]);
		bf_audio_stop_all_voices();
	}
	// A change while playing is a 5 ms ramp, not a step.
	bf_audio_voice v = bf_audio_play(mono, 1, 0, 0);
	render(0.05);
	bf_audio_voice_set_volume(v, 0.2f);
	std::vector<float> r = render(0.02);
	double g = cos(M_PI / 4), w1 = gain(r, 0, 0, win / 2) / g, w2 = gain(r, 0, win / 2, win / 2) / g, w3 = gain(r, 0, win, win) / g;
	check(w1 < 0.95 && w1 > w2 && w2 > 0.21 && fabs(w3 - 0.2) < 0.002,
	      "start volume: volume 1 -> 0.2 while playing ramps: %.3f, %.3f in the first 5 ms, then %.3f", w1, w2, w3);
	bf_audio_stop_all_voices();
	// The effects volume also applies from a sound's first sample.
	bf_audio_play(mono, 1, 0, 0);
	render(0.05);
	bf_audio_stop_all_voices();
	bf_audio_set_effects_volume(0.2f);
	bf_audio_play(mono, 0.5f, 0, 0);
	std::vector<float> b = render(0.05);
	double first = gain(b, 0, 0, win), want = 0.1 * cos(M_PI / 4);
	check(fabs(first / want - 1) < 0.01, "start volume: effects volume 0.2, voice 0.5: first 5 ms %.4f, expected %.4f", first, want);
	bf_audio_set_effects_volume(1);
	bf_audio_stop();
	bf_audio_sound_destroy(mono);
	bf_audio_sound_destroy(stereo);
}

// The ring of recent voices and the tags: started and refused calls, the tag
// used by one call only, the newest kept when there are more.
static void test_recent()
{
	bf_audio_config cfg = { BF_AUDIO_MODE_SILENT, 4, 0 };
	check(bf_audio_start(&cfg) == BF_AUDIO_MODE_SILENT, "recent voices: silent engine starts");
	bf_audio_sound *s = tone(22050, 1, 0.25, 441);
	bf_audio_voice_record r[BF_AUDIO_RECENT_VOICES];
	int n0 = bf_audio_recent_voices(0, r, BF_AUDIO_RECENT_VOICES);
	long long last = n0 ? r[n0 - 1].serial : 0;
	bf_audio_tag_next_voice("effect\\near.wav", 12.5f);
	bf_audio_voice a = bf_audio_play(s, 100 / 127.0f, 0, 0);
	bf_audio_voice b = bf_audio_play(s, 2.0f, -3.0f, 0);          // untagged, clamped
	bf_audio_tag_next_voice("refused.wav", 3);
	bf_audio_play(nullptr, 1, 0, 0);                               // refused
	bf_audio_tag_next_voice("dropped.wav", 300);
	bf_audio_tag_next_voice(nullptr, 0);                           // cleared before use
	bf_audio_play(s, 0.25f, 0.5f, 1);
	int n = bf_audio_recent_voices(last, r, BF_AUDIO_RECENT_VOICES);
	check(n == 4 && r[0].serial == last + 1 && r[3].serial == last + 4, "recent voices: 4 new records after serial %lld (%d)", last, n);
	if (n == 4) {
		check(r[0].voice == a && r[0].started && !strcmp(r[0].label, "effect\\near.wav") && r[0].distance == 12.5f
		      && fabsf(r[0].volume - 100 / 127.0f) < 1e-6 && fabs(r[0].duration - 0.25) < 1e-3,
		      "recent voices: the tagged call keeps label, distance, volume %.4f and duration %.3f", r[0].volume, r[0].duration);
		check(r[1].voice == b && r[1].label[0] == 0 && r[1].distance == -1 && r[1].volume == 1 && r[1].pan == -1,
		      "recent voices: the next call is untagged, volume and pan clamped");
		check(!r[2].started && r[2].voice == 0 && !strcmp(r[2].label, "refused.wav") && r[2].duration == 0,
		      "recent voices: a refused call is recorded with its tag");
		check(r[3].started && r[3].label[0] == 0 && r[3].loop && r[3].pan == 0.5f, "recent voices: a cleared tag is not used");
	}
	for (int i = 0; i < BF_AUDIO_RECENT_VOICES + 5; ++i) bf_audio_play(s, 0.5f, 0, 0);
	n = bf_audio_recent_voices(0, r, BF_AUDIO_RECENT_VOICES);
	check(n == BF_AUDIO_RECENT_VOICES && r[n - 1].serial == last + 4 + BF_AUDIO_RECENT_VOICES + 5
	      && r[0].serial == r[n - 1].serial - BF_AUDIO_RECENT_VOICES + 1, "recent voices: the ring keeps the newest %d", n);
	bf_audio_voice_record two[2];
	n = bf_audio_recent_voices(0, two, 2);
	check(n == 2 && two[1].serial == r[BF_AUDIO_RECENT_VOICES - 1].serial, "recent voices: a short buffer gets the newest");
	check(bf_audio_stream_position() == 0, "recent voices: no stream position in silent mode");
	bf_audio_sound_destroy(s);
	bf_audio_stop();
}

static void test_silent()
{
	bf_audio_config cfg = { BF_AUDIO_MODE_SILENT, 0, 0 };
	check(bf_audio_start(&cfg) == BF_AUDIO_MODE_SILENT, "silent mode starts");
	bf_audio_sound *s = tone(22050, 1, 0.5, 441);
	long long started = counters().voices_started;
	bf_audio_voice v = bf_audio_play(s, 1, 0, 0);
	check(v != 0 && !bf_audio_voice_playing(v) && counters().voices_started == started + 1,
	      "silent: a voice is counted and finishes at once");
	std::string path = g_builddir + "/bf-audio-silent.wav";
	std::vector<uint8_t> wav = make_wav(16, 1, 22050, 2205);
	FILE *f = fopen(path.c_str(), "wb");
	if (f) { fwrite(wav.data(), 1, wav.size(), f); fclose(f); }
	bf_audio_stream_open(path.c_str());
	bf_audio_stream_set_loop_count(0);
	bf_audio_stream_start();
	bf_audio_counters c = counters();
	check(c.stream_status == BF_AUDIO_STREAM_PLAYING && path == c.stream_file, "silent: the stream keeps its file name and state");
	unlink(path.c_str());
	bf_audio_sound_destroy(s);
	bf_audio_stop();
}

// ---- the output device -------------------------------------------------------

static std::string g_client;

static void test_device()
{
	int mode = bf_audio_start(nullptr);
	if (mode != BF_AUDIO_MODE_DEVICE) {
		check(mode == BF_AUDIO_MODE_SILENT && counters().device_failures > 0, "no output device: running silent, counted");
		skipped("device playback: no output device");
	} else {
		info("device: mixing at %d Hz", bf_audio_sample_rate());
		bf_audio_sound *s[5];
		bf_audio_voice v[5];
		long long started = counters().voices_started;
		for (int i = 0; i < 5; ++i) {
			s[i] = tone(22050, 1, 0.6, 330 * pow(1.25, i), 0.1);
			v[i] = bf_audio_play(s[i], 0.5f, -1 + 0.5f * i, 0);
		}
		msleep(50);
		check(bf_audio_voices_playing() == 5 && counters().voices_started == started + 5,
		      "device: five voices sound at once (%d playing)", bf_audio_voices_playing());
		bf_audio_voice_set_volume(v[0], 0.1f);
		msleep(200);
		bf_audio_voice_stop(v[4]);
		check(!bf_audio_voice_playing(v[4]) && bf_audio_voices_playing() == 4, "device: stop one, four left");
		msleep(800);
		check(bf_audio_voices_playing() == 0, "device: the others end on their own (%d playing)", bf_audio_voices_playing());
		for (int i = 0; i < 5; ++i) bf_audio_sound_destroy(s[i]);
	}

	std::string bgm = g_client + "/BGM/01.mp3";
	struct stat st;
	if (stat(bgm.c_str(), &st) != 0) {
		skipped("music stream: %s not found", bgm.c_str());
	} else {
		bf_audio_info in;
		check(bf_audio_query_file(bgm.c_str(), &in) == 0 && in.kind == BF_AUDIO_KIND_MP3,
		      "BGM/01.mp3 is MP3: %d Hz, %d ch, about %lld frames", in.sample_rate, in.channels, (long long)in.frames);
		bf_audio_stream_set_volume(1);
		int ok = bf_audio_stream_open(bgm.c_str());
		bf_audio_stream_set_loop_count(0);
		bf_audio_stream_start();
		bf_audio_counters c = counters();
		check(ok == 0 && c.stream_status == BF_AUDIO_STREAM_PLAYING && bgm == c.stream_file && c.stream_duration > 60,
		      "stream: %s, %.1f s, looping", c.stream_file, c.stream_duration);
		// The client's fade runs on its own thread: fade out, pause, resume, fade in.
		std::atomic<bool> faded(false);
		std::thread fade([&] {
			for (int a = 100; a > 0; --a) { bf_audio_stream_set_volume(a / 100.0f); msleep(5); }
			bf_audio_stream_pause(1);
			msleep(100);
			bf_audio_stream_pause(0);
			for (int a = 0; a <= 100; ++a) { bf_audio_stream_set_volume(a / 100.0f); msleep(5); }
			faded = true;
		});
		msleep(2000);
		fade.join();
		c = counters();
		check(faded && c.stream_status == BF_AUDIO_STREAM_PLAYING && fabs(bf_audio_stream_volume() - 1) < 1e-6,
		      "stream: two seconds played with a fade out and in from another thread");
		bf_audio_stream_close();
	}
	bf_audio_stop();
}

// ---- the game's own sounds ---------------------------------------------------

static std::string printable(const std::string &s)
{
	std::string o;
	char b[8];
	for (unsigned char ch : s) {
		if (ch >= 32 && ch < 127) o += (char)ch;
		else { snprintf(b, sizeof b, "\\x%02x", ch); o += b; }
	}
	return o;
}

static bool is_wav(const std::string &n)
{
	if (n.size() < 4) return false;
	std::string e = n.substr(n.size() - 4);
	for (char &ch : e) ch = (char)tolower((unsigned char)ch);
	return e == ".wav";
}

static void survey(const char *label, Grf &g, std::vector<const GrfEntry *> &wavs, bool all)
{
	std::map<std::string, int> formats;
	int decoded = 0, des = 0, broken = 0, failed = 0, builtin = 0, mismatched = 0;
	size_t n = wavs.size(), stride = all ? 1 : std::max<size_t>(1, n / 20), visited = 0;
	bool sounded_checked = false;
	for (size_t off = 0; off < stride && (all || decoded < 20); ++off)
		for (size_t k = off; k < n && (all || decoded < 20); k += stride) {
			const GrfEntry &e = *wavs[k];
			visited++;
			if (e.des()) { des++; continue; }
			std::vector<uint8_t> data;
			std::string err;
			if (!g.read(e, data, &err)) { broken++; if (!all) info("%s: %s", printable(e.name).c_str(), err.c_str()); continue; }
			bf_audio_info in;
			int64_t frames = 0;
			int16_t *pcm = bf_audio_decode_alloc(data.data(), data.size(), &in, &frames);
			if (!pcm) {
				failed++;
				info("cannot decode %s (%zu bytes, kind %d)", printable(e.name).c_str(), data.size(), bf_audio_file_kind(data.data(), data.size()));
				continue;
			}
			decoded++;
			if (in.builtin) builtin++;
			bf_audio_info q;
			if (bf_audio_query(data.data(), data.size(), &q) != 0 || (q.frames_exact && q.frames != frames)) mismatched++;
			char key[96];
			snprintf(key, sizeof key, "kind %d tag %d, %d-bit %s %d Hz", in.kind, in.format_tag, in.bits,
			         in.channels == 1 ? "mono" : "stereo", in.sample_rate);
			formats[key]++;
			if (!all)
				info("%-48s %6lld frames %5.2f s  %s", printable(e.name).c_str(), (long long)frames,
				     (double)frames / in.sample_rate, key);
			if (!sounded_checked && !all) {
				// Play the first one offline and look at what came out.
				bf_audio_sound *s = bf_audio_sound_create(pcm, frames, in.channels, in.sample_rate);
				bf_audio_config cfg = { BF_AUDIO_MODE_OFFLINE, 4, 44100 };
				bf_audio_start(&cfg);
				bf_audio_play(s, 1, 0, 0);
				Level l = rms(render(bf_audio_sound_duration(s) + 0.05));
				bf_audio_stop();
				bf_audio_sound_destroy(s);
				check(l.l > 0, "%s: a real sound renders samples offline (rms %.4f)", printable(e.name).c_str(), l.l);
				sounded_checked = true;
			}
			bf_audio_free(pcm);
		}
	check(failed == 0 && mismatched == 0 && (all || decoded == 20 || decoded == (int)(n - des)),
	      "%s: %d decoded (%d by the built-in reader), %d failed, %d DES-flagged skipped, %d unreadable, %d cut short in the archive, of %zu visited / %zu .wav entries",
	      label, decoded, builtin, failed, des, broken, g.truncated, visited, n);
	for (auto &f : formats) info("  %4d x %s", f.second, f.first.c_str());
}

static void test_grf(bool all)
{
	const char *names[] = { "data.grf", "rdata.grf" };
	for (int i = 0; i < (all ? 2 : 1); ++i) {
		std::string path = g_client + "/" + names[i];
		struct stat st;
		if (stat(path.c_str(), &st) != 0) { skipped("real sounds: %s not found", path.c_str()); continue; }
		Grf g;
		std::string err;
		if (!check(g.open(path.c_str(), &err), "%s opens: %s", names[i], err.empty() ? "ok" : err.c_str())) continue;
		std::vector<const GrfEntry *> wavs;
		for (const GrfEntry &e : g.entries)
			if (is_wav(e.name)) wavs.push_back(&e);
		std::sort(wavs.begin(), wavs.end(), [](const GrfEntry *a, const GrfEntry *b) { return a->name < b->name; });
		survey(names[i], g, wavs, all);
	}
}

int main(int argc, char **argv)
{
	bf_trace_init();
	std::string self = argv[0];
	size_t slash = self.rfind('/');
	g_builddir = slash == std::string::npos ? "." : self.substr(0, slash);
	const char *dir = getenv("BF_CLIENT_DIR");
	g_client = dir && *dir ? dir : std::string(getenv("HOME") ? getenv("HOME") : "") + "/Games/" + BF_APP_NAME;
	bool all = getenv("BF_AUDIO_TEST_ALL") && *getenv("BF_AUDIO_TEST_ALL") == '1';
	(void)argc;

	test_decode();
	test_offline();
	test_start_volume();
	test_silent();
	test_recent();
	test_device();
	test_grf(false);
	if (all) test_grf(true);

	bf_audio_counters c = counters();
	info("counters: %lld voices started, %lld stolen, %lld rejected, %lld sounds converted; %lld decodes, %lld failures, %lld fallbacks; "
	     "%lld streams opened, %lld failed; %lld device failures",
	     c.voices_started, c.voices_stolen, c.voices_rejected, c.conversions, c.decodes, c.decode_failures,
	     c.decode_fallbacks, c.streams_opened, c.stream_failures, c.device_failures);
	printf("%s: %d failure%s\n", g_failures ? "FAILED" : "PASSED", g_failures, g_failures == 1 ? "" : "s");
	return g_failures ? 1 : 0;
}
