// bifrost-compat-miles-test: the client's Miles call sequences (InitMSS,
// PlayDigital, PlayStream, PauseStream, CloseStream, UnInitMSS in
// src/device/Sound.cpp; CWave::Load and CWave::Reset in src/resource/Wave.cpp)
// run through the AIL_* names of the adapter. One line per check; the exit
// status is the number of failures.
//
// The engine is started in OFFLINE mode before AIL_open_digital_driver,
// which adopts it, so that the samples produced can be rendered and looked
// at. The stream part uses BGM/01.mp3 and BGM/08.mp3 from the game folder
// ($BF_CLIENT_DIR or ~/Games/<name>) and says "skipped" without them; a
// generated WAV file proves the looping in any case. The last part opens the
// real device the way the client does (sound may play).

#include "../../../src/device/Mss.h"

#include "bf_audio.h"
#include "bf_file.h"
#include "bf_trace.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

static int g_failures = 0;

static void check(bool ok, const char *what, ...)
{
	va_list ap;
	va_start(ap, what);
	printf("%s ", ok ? "ok  " : "FAIL");
	vprintf(what, ap);
	printf("\n");
	va_end(ap);
	if (!ok)
		++g_failures;
}

static void put16(std::vector<uint8_t> &v, uint16_t x) { v.push_back((uint8_t)x); v.push_back((uint8_t)(x >> 8)); }
static void put32(std::vector<uint8_t> &v, uint32_t x) { put16(v, (uint16_t)x); put16(v, (uint16_t)(x >> 16)); }

static std::vector<uint8_t> riff(int tag, int channels, int rate, int bits, int align, const std::vector<uint8_t> &extra_fmt,
                                 const std::vector<uint8_t> &data)
{
	std::vector<uint8_t> v = { 'R', 'I', 'F', 'F' };
	put32(v, (uint32_t)(4 + 8 + 16 + extra_fmt.size() + 8 + data.size()));
	v.insert(v.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
	put32(v, (uint32_t)(16 + extra_fmt.size()));
	put16(v, (uint16_t)tag);
	put16(v, (uint16_t)channels);
	put32(v, (uint32_t)rate);
	put32(v, (uint32_t)(rate * align));
	put16(v, (uint16_t)align);
	put16(v, (uint16_t)bits);
	v.insert(v.end(), extra_fmt.begin(), extra_fmt.end());
	v.insert(v.end(), { 'd', 'a', 't', 'a' });
	put32(v, (uint32_t)data.size());
	v.insert(v.end(), data.begin(), data.end());
	return v;
}

static std::vector<int16_t> sine(int rate, double seconds, double hz, double amplitude)
{
	std::vector<int16_t> s((size_t)(rate * seconds));
	for (size_t i = 0; i < s.size(); ++i)
		s[i] = (int16_t)lround(amplitude * 32767 * sin(2 * M_PI * hz * (double)i / rate));
	return s;
}

static std::vector<uint8_t> pcm16_wav(int rate, const std::vector<int16_t> &s)
{
	std::vector<uint8_t> data;
	for (int16_t x : s)
		put16(data, (uint16_t)x);
	return riff(1, 1, rate, 16, 2, {}, data);
}

static std::vector<uint8_t> pcm8_wav(int rate, const std::vector<int16_t> &s)
{
	std::vector<uint8_t> data;
	for (int16_t x : s)
		data.push_back((uint8_t)((x >> 8) + 128));
	return riff(1, 1, rate, 8, 1, {}, data);
}

// IMA ADPCM, mono, 256-byte blocks: the public encoder algorithm.
static std::vector<uint8_t> ima_wav(int rate, const std::vector<int16_t> &in, int64_t *frames_out)
{
	static const int steps[89] = { 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55,
		60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494,
		544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
		3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289,
		16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 };
	static const int adjust[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
	const int align = 256, per_block = (align - 4) * 2 + 1;
	size_t blocks = in.size() / per_block;
	std::vector<uint8_t> data;
	int index = 0;
	for (size_t b = 0; b < blocks; ++b) {
		const int16_t *x = &in[b * per_block];
		int pred = x[0];
		put16(data, (uint16_t)(int16_t)pred);
		data.push_back((uint8_t)index);
		data.push_back(0);
		uint8_t low = 0;
		for (int i = 1; i < per_block; ++i) {
			int step = steps[index], diff = x[i] - pred, code = 0, delta = step >> 3;
			if (diff < 0) { code = 8; diff = -diff; }
			for (int bit = 4; bit; bit >>= 1, step >>= 1)
				if (diff >= step) { code |= bit; diff -= step; delta += step; }
			pred += (code & 8) ? -delta : delta;
			pred = pred < -32768 ? -32768 : (pred > 32767 ? 32767 : pred);
			index += adjust[code & 7];
			index = index < 0 ? 0 : (index > 88 ? 88 : index);
			if (i & 1) low = (uint8_t)code;
			else data.push_back((uint8_t)(low | code << 4));
		}
	}
	*frames_out = (int64_t)(blocks * per_block);
	std::vector<uint8_t> extra;
	put16(extra, 2);
	put16(extra, (uint16_t)per_block);
	// riff() writes byte rate = rate * align; close enough for a test image.
	return riff(0x11, 1, rate, 4, align, extra, data);
}

// What CWave::Load does with the bytes of a file: an image from new[],
// decompressed when it is ADPCM or MP3. *decompressed tells CWave::Reset
// which free to use.
static BYTE *wave_load(const std::vector<uint8_t> &file, bool *decompressed, int *type_out, U32 *outsize = 0)
{
	BYTE *tmp = new BYTE[file.size()];
	memcpy(tmp, file.data(), file.size());
	BYTE *image = 0;
	*decompressed = false;
	AILSOUNDINFO info;
	int type = AIL_file_type(tmp, (U32)file.size());
	*type_out = type;
	switch (type) {
	case AILFILETYPE_PCM_WAV:
		image = tmp;
		break;
	case AILFILETYPE_ADPCM_WAV:
		*decompressed = true;
		AIL_WAV_info(tmp, &info);
		AIL_decompress_ADPCM(&info, (void **)&image, outsize);
		delete[] tmp;
		break;
	case AILFILETYPE_MPEG_L3_AUDIO:
		*decompressed = true;
		AIL_decompress_ASI(tmp, (U32)file.size(), "x.mp3", (void **)&image, outsize, 0);
		delete[] tmp;
		break;
	default:
		image = tmp;
		break;
	}
	return image;
}

static void wave_reset(BYTE *image, bool decompressed)
{
	if (decompressed)
		AIL_mem_free_lock(image);
	else
		delete[] image;
}

static HDIGDRIVER g_dig = 0;
static HSAMPLE g_samples[48];
static int g_num_samples = 16;

// PlayDigital's loop: the first handle that is SMP_DONE gets the sound.
static bool play_digital(BYTE *image, int volume)
{
	for (int i = 0; i < g_num_samples; i++) {
		if (g_samples[i] && AIL_sample_status(g_samples[i]) == SMP_DONE) {
			AIL_init_sample(g_samples[i]);
			AIL_set_sample_file(g_samples[i], image, 0);
			AIL_set_sample_volume(g_samples[i], volume);
			AIL_start_sample(g_samples[i]);
			return true;
		}
	}
	return false;
}

// Renders seconds of the offline mix; returns the peak of either channel and
// the largest left/right difference.
static float render(double seconds, float *lr_diff = 0, double *rms = 0)
{
	int rate = bf_audio_sample_rate();
	int64_t left = (int64_t)(seconds * rate);
	std::vector<float> buf(2 * 1024);
	float peak = 0, diff = 0;
	double sum = 0;
	int64_t n = 0;
	while (left > 0) {
		int64_t want = left < 1024 ? left : 1024;
		int64_t got = bf_audio_render(buf.data(), want);
		if (got <= 0)
			break;
		for (int64_t i = 0; i < got; ++i) {
			float l = buf[2 * i], r = buf[2 * i + 1];
			peak = fmaxf(peak, fmaxf(fabsf(l), fabsf(r)));
			diff = fmaxf(diff, fabsf(l - r));
			sum += (double)l * l;
		}
		n += got;
		left -= got;
	}
	if (lr_diff) *lr_diff = diff;
	if (rms) *rms = n ? sqrt(sum / n) : 0;
	return peak;
}

static bf_audio_counters counters()
{
	bf_audio_counters c;
	bf_audio_get_counters(&c);
	return c;
}

static bool ends_with(const char *s, const char *tail)
{
	size_t a = strlen(s), b = strlen(tail);
	return a >= b && strcasecmp(s + a - b, tail) == 0;
}

static std::string out_dir()
{
	const char *d = getenv("BF_TEST_OUT");
	char buf[2048];
	if (d && *d == '/')
		return d;
	return getcwd(buf, sizeof buf) ? std::string(buf) : std::string("/tmp");
}

static void init_mss_2d()
{
	// InitMSS with m_soundMode 1: one channel, 22050 Hz, 16 bit.
	AIL_set_redist_directory(".\\");
	check(AIL_startup() != 0, "AIL_startup");
	AIL_set_preference(DIG_USE_WAVEOUT, 0);
	g_dig = AIL_open_digital_driver(22050, 16, 1, AIL_OPEN_DIGITAL_FORCE_PREFERENCE);
	check(g_dig != 0, "AIL_open_digital_driver returns a driver (engine mode %d)", bf_audio_mode());
	bool all = true, distinct = true, done = true;
	for (int i = 0; i < g_num_samples; i++) {
		g_samples[i] = AIL_allocate_sample_handle(g_dig);
		all = all && g_samples[i];
		done = done && AIL_sample_status(g_samples[i]) == SMP_DONE;
		for (int j = 0; j < i; ++j)
			distinct = distinct && g_samples[i] != g_samples[j];
	}
	check(all && distinct, "%d sample handles allocated, all distinct", g_num_samples);
	check(done, "a new sample handle is SMP_DONE");
}

static void uninit_mss()
{
	for (int i = 0; i < 48; i++) {
		if (g_samples[i]) {
			AIL_release_sample_handle(g_samples[i]);
			g_samples[i] = 0;
		}
	}
	AIL_close_digital_driver(g_dig);
	g_dig = 0;
	AIL_shutdown();
}

int main()
{
	setvbuf(stdout, 0, _IONBF, 0);
	bf_audio_reset_counters();

	// ---- start-up, on an OFFLINE engine the driver adopts
	bf_audio_config offline = { BF_AUDIO_MODE_OFFLINE, BF_AUDIO_DEFAULT_VOICES, 44100 };
	check(bf_audio_start(&offline) == BF_AUDIO_MODE_OFFLINE, "the offline engine starts");
	init_mss_2d();
	check(bf_audio_mode() == BF_AUDIO_MODE_OFFLINE, "AIL_open_digital_driver adopts the running engine");
	check(counters().max_voices >= 48, "the engine has %d voices, at least the client's 48 handles", counters().max_voices);
	{
		HPROENUM e = HPROENUM_FIRST;
		HPROVIDER p = 0;
		char *name = 0;
		check(AIL_enumerate_3D_providers(&e, &p, &name) == 0 && p == 0, "AIL_enumerate_3D_providers: no 3D provider");
	}

	// ---- CWave::Load on generated images
	std::vector<int16_t> tone = sine(22050, 0.25, 440, 0.5);
	std::vector<uint8_t> pcm = pcm16_wav(22050, tone);
	std::vector<uint8_t> pcm8 = pcm8_wav(11025, sine(11025, 0.2, 300, 0.5));
	int64_t ima_frames = 0;
	std::vector<uint8_t> ima = ima_wav(22050, sine(22050, 0.3, 500, 0.5), &ima_frames);
	std::vector<uint8_t> junk(300, 0x5a);

	bool dec_pcm, dec_ima, dec_junk;
	int type;
	BYTE *img_pcm = wave_load(pcm, &dec_pcm, &type);
	check(type == AILFILETYPE_PCM_WAV && !dec_pcm && img_pcm, "AIL_file_type: 16-bit PCM WAV is AILFILETYPE_PCM_WAV (%d)", type);
	BYTE *img_junk = wave_load(junk, &dec_junk, &type);
	check(type == AILFILETYPE_UNKNOWN, "AIL_file_type: junk is AILFILETYPE_UNKNOWN (%d)", type);
	check(AIL_file_type(ima.data(), (U32)ima.size()) == AILFILETYPE_ADPCM_WAV, "AIL_file_type: IMA ADPCM WAV is AILFILETYPE_ADPCM_WAV");
	{
		AILSOUNDINFO info;
		int ok = AIL_WAV_info(ima.data(), &info);
		const uint8_t *d = (const uint8_t *)info.data_ptr;
		check(ok && info.format == 0x11 && info.rate == 22050 && info.channels == 1 && info.bits == 4 &&
		          info.block_size == 256 && (int64_t)info.samples == ima_frames && d > ima.data() &&
		          d + info.data_len == ima.data() + ima.size() && info.initial_ptr == info.data_ptr,
		      "AIL_WAV_info on IMA ADPCM: format 0x%x, %u Hz, %d ch, %d bits, block %u, %u samples (%lld), data_len %u",
		      (int)info.format, (unsigned)info.rate, (int)info.channels, (int)info.bits, (unsigned)info.block_size,
		      (unsigned)info.samples, (long long)ima_frames, (unsigned)info.data_len);
	}
	U32 ima_size = 0;
	BYTE *img_ima = wave_load(ima, &dec_ima, &type, &ima_size);
	{
		bf_audio_info in;
		bool ok = img_ima && ima_size > 44 && AIL_file_type(img_ima, ima_size) == AILFILETYPE_PCM_WAV &&
		          bf_audio_query(img_ima, ima_size, &in) == 0 && in.bits == 16 && in.sample_rate == 22050 &&
		          in.channels == 1 && in.frames == ima_frames;
		check(dec_ima && ok, "AIL_decompress_ADPCM returns a 16-bit PCM WAV image of %lld frames (%u bytes)",
		      (long long)ima_frames, (unsigned)ima_size);
	}
	{
		// The client passes 0 for the size (Wave.cpp); and an ASI path on an 8-bit WAV.
		AILSOUNDINFO info;
		void *out = (void *)1;
		AIL_WAV_info(ima.data(), &info);
		check(AIL_decompress_ADPCM(&info, &out, 0) == 1 && out, "AIL_decompress_ADPCM with a null size pointer");
		AIL_mem_free_lock(out);
		U32 size = 0;
		out = 0;
		int r = AIL_decompress_ASI(pcm8.data(), (U32)pcm8.size(), "x.wav", &out, &size, 0);
		bf_audio_info in;
		check(r == 1 && out && bf_audio_query(out, size, &in) == 0 && in.bits == 16 && in.sample_rate == 11025 &&
		          in.frames == 2205,
		      "AIL_decompress_ASI on an 8-bit WAV gives a 16-bit PCM WAV image (%u bytes)", (unsigned)size);
		AIL_mem_free_lock(out);
		out = (void *)1;
		check(AIL_decompress_ASI(junk.data(), (U32)junk.size(), "x.mp3", &out, &size, 0) == 0 && out == 0,
		      "AIL_decompress_ASI on junk fails with a null image");
		AILSOUNDINFO bogus = {};
		bogus.data_ptr = junk.data();
		out = (void *)1;
		check(AIL_decompress_ADPCM(&bogus, &out, 0) == 0 && out == 0,
		      "AIL_decompress_ADPCM on data no AIL_WAV_info described fails with a null image");
	}

	// ---- PlayDigital
	long long decodes0 = counters().decodes;
	long long started0 = counters().voices_started;
	check(play_digital(img_pcm, 127), "PlayDigital finds a free handle");
	check(AIL_sample_status(g_samples[0]) == SMP_PLAYING, "AIL_sample_status: SMP_PLAYING after AIL_start_sample");
	check(counters().voices_started == started0 + 1, "the module counted one voice started");
	float diff = 1;
	float peak127 = render(0.1, &diff);
	check(fabsf(peak127 - 0.5f * 0.7071f) < 0.01f, "the sound is rendered at volume 127 (peak %.3f: amplitude 0.5, mono centred at -3 dB per side)", peak127);
	check(diff < 1e-4f, "centred: left and right are equal (largest difference %.6f)", diff);
	render(0.3);
	check(AIL_sample_status(g_samples[0]) == SMP_DONE, "AIL_sample_status: SMP_DONE once its 0.25 s are rendered");
	check(play_digital(img_pcm, 32), "PlayDigital again, volume 32");
	// The reused voice starts at its own volume: the level is measured over
	// its first 5 ms (a 440 Hz tone reaches its peak in them).
	float peak32 = render(0.005);
	check(fabsf(peak32 / peak127 - 32.0f / 127) < 0.02f, "volume 32 against 127, first 5 ms: peak ratio %.3f (expected %.3f)",
	      peak32 / peak127, 32.0f / 127);
	render(0.075);
	render(0.3);
	check(counters().decodes == decodes0 + 1, "the image was decoded once for two plays (the sound cache)");
	{
		// AIL_set_sample_volume while it plays.
		play_digital(img_pcm, 127);
		render(0.05);
		AIL_set_sample_volume(g_samples[0], 16);
		render(0.005);   // a change while playing ramps over 5 ms
		float p = render(0.05);
		check(fabsf(p / peak127 - 16.0f / 127) < 0.02f, "AIL_set_sample_volume while playing: ratio %.3f (expected %.3f)",
		      p / peak127, 16.0f / 127);
		render(0.3);
	}
	// A reload at the same address (AIL_file_type) drops the cached sound.
	AIL_file_type(img_pcm, (U32)pcm.size());
	play_digital(img_pcm, 127);
	check(counters().decodes == decodes0 + 2, "AIL_file_type on a known address makes the next play decode again");
	render(0.3);
	// The decompressed ADPCM image plays.
	play_digital(img_ima, 127);
	double rms = 0;
	render(0.1, 0, &rms);
	check(rms > 0.05, "the decompressed ADPCM image plays (rms %.3f)", rms);
	render(0.4);
	// A junk image: AIL_set_sample_file fails, nothing plays.
	started0 = counters().voices_started;
	AIL_init_sample(g_samples[0]);
	check(AIL_set_sample_file(g_samples[0], img_junk, 0) == 0, "AIL_set_sample_file on junk returns 0");
	AIL_start_sample(g_samples[0]);
	check(AIL_sample_status(g_samples[0]) == SMP_DONE && counters().voices_started == started0,
	      "AIL_start_sample after a failed AIL_set_sample_file starts nothing");
	AIL_init_sample(g_samples[0]);
	check(AIL_set_sample_file(g_samples[0], 0, 0) == 0, "AIL_set_sample_file on a null image returns 0");
	// Every handle busy: the client drops the sound itself, nothing is stolen.
	{
		bf_audio_counters c0 = counters();
		int played = 0;
		for (int i = 0; i < g_num_samples; ++i)
			played += play_digital(img_pcm, 100);
		bool dropped = !play_digital(img_pcm, 100);
		bf_audio_counters c1 = counters();
		check(played == g_num_samples && dropped && c1.voices_playing == g_num_samples &&
		          c1.voices_stolen == c0.voices_stolen && c1.voices_rejected == c0.voices_rejected,
		      "%d sounds play; the next finds no SMP_DONE handle (dropped by the client, %lld stolen, %lld rejected)",
		      played, c1.voices_stolen - c0.voices_stolen, c1.voices_rejected - c0.voices_rejected);
		// EndDigitalSound
		for (int i = 0; i < 48; i++)
			if (g_samples[i])
				AIL_end_sample(g_samples[i]);
		bool all_done = true;
		for (int i = 0; i < g_num_samples; ++i)
			all_done = all_done && AIL_sample_status(g_samples[i]) == SMP_DONE;
		check(all_done && counters().voices_playing == 0, "AIL_end_sample on every handle: all SMP_DONE, none playing");
	}

	// ---- the stream on a generated file: looping, volume, pause, close
	std::string loop_file = out_dir() + "/compat-miles-loop.wav";
	{
		std::vector<uint8_t> w = pcm16_wav(22050, sine(22050, 0.2, 330, 0.5));
		FILE *f = fopen(loop_file.c_str(), "wb");
		check(f && fwrite(w.data(), 1, w.size(), f) == w.size() && fclose(f) == 0, "wrote %s", loop_file.c_str());
	}
	HSTREAM stream = 0;
	{
		// PlayStream(name): CloseStream, open, volume, loop count 0, start.
		HSTREAM none = 0;
		AIL_close_stream(none);
		AIL_pause_stream(none, 1);
		AIL_stream_volume(none);   // PauseStream does this with no stream open
		stream = AIL_open_stream(g_dig, loop_file.c_str(), 0);
		check(stream != 0, "AIL_open_stream on a WAV file by absolute path");
		AIL_set_stream_volume(stream, 100);
		AIL_set_stream_loop_count(stream, 0);
		AIL_start_stream(stream);
		check(counters().stream_status == BF_AUDIO_STREAM_PLAYING && AIL_stream_volume(stream) == 100,
		      "the stream plays at volume %d", (int)AIL_stream_volume(stream));
		double r = 0;
		render(0.7, 0, &r);
		bf_audio_counters c = counters();
		check(r > 0.05 && c.stream_loops >= 3 && c.stream_status == BF_AUDIO_STREAM_PLAYING,
		      "loop count 0 loops: %d passes of 0.2 s in 0.7 s, still playing (rms %.3f)", c.stream_loops, r);
		AIL_pause_stream(stream, 1);
		check(counters().stream_status == BF_AUDIO_STREAM_PAUSED, "AIL_pause_stream(1) pauses");
		float p = render(0.1);
		check(p < 1e-4f, "a paused stream is silent (peak %.6f)", p);
		AIL_pause_stream(stream, 0);
		check(counters().stream_status == BF_AUDIO_STREAM_PLAYING, "AIL_pause_stream(0) resumes");
		AIL_close_stream(stream);
		check(counters().stream_status == BF_AUDIO_STREAM_NONE, "AIL_close_stream closes");
		// The stale handle after close does nothing.
		AIL_set_stream_volume(stream, 5);
		AIL_start_stream(stream);
		AIL_pause_stream(stream, 0);
		AIL_set_stream_loop_count(stream, 3);
		AIL_close_stream(stream);
		check(counters().stream_status == BF_AUDIO_STREAM_NONE && lroundf(bf_audio_stream_volume() * 127) == 100,
		      "calls on the closed handle are no-ops (status %d, volume %ld)", counters().stream_status,
		      lroundf(bf_audio_stream_volume() * 127));
		// A stale handle while a newer stream is open does not touch it.
		HSTREAM newer = AIL_open_stream(g_dig, loop_file.c_str(), 0);
		AIL_start_stream(newer);
		AIL_set_stream_volume(stream, 7);
		AIL_pause_stream(stream, 1);
		check(newer && newer != stream && counters().stream_status == BF_AUDIO_STREAM_PLAYING &&
		          AIL_stream_volume(newer) == 100,
		      "a stale handle does not act on the stream opened after it");
		// The fade thread against the main thread: volume steps and pauses on
		// a handle that the main thread keeps closing and replacing.
		std::atomic<HSTREAM> shared(newer);
		std::atomic<bool> stop(false);
		std::thread fade([&] {
			int v = 0;
			while (!stop) {
				HSTREAM h = shared.load();
				AIL_set_stream_volume(h, v++ & 127);
				AIL_pause_stream(h, v & 1);
				AIL_stream_volume(h);
			}
		});
		for (int i = 0; i < 40; ++i) {
			HSTREAM old = shared.load();
			AIL_close_stream(old);
			HSTREAM h = AIL_open_stream(g_dig, loop_file.c_str(), 0);
			AIL_set_stream_volume(h, 100);
			AIL_set_stream_loop_count(h, 0);
			AIL_start_stream(h);
			shared = h;
			render(0.01);
		}
		stop = true;
		fade.join();
		check(counters().stream_status != BF_AUDIO_STREAM_NONE, "40 reopenings under a fade thread on stale handles");
		AIL_close_stream(shared.load());
	}
	{
		long long failures = counters().stream_failures;
		HSTREAM h = AIL_open_stream(g_dig, "bgm\\no-such-file.mp3", 0);
		check(h == 0 && counters().stream_failures == failures + 1,
		      "AIL_open_stream on a missing file returns 0, counted as a stream failure");
		AIL_set_stream_volume(h, 50);
		AIL_set_stream_loop_count(h, 0);
		AIL_start_stream(h);   // PlayStream goes on after its non-fatal assertion
		check(counters().stream_status == BF_AUDIO_STREAM_NONE, "the calls PlayStream makes after a failed open do nothing");
	}

	// ---- the game's music
	char bgm[2048];
	if (bf_file_resolve("bgm\\01.mp3", BF_RESOLVE_READ, bgm, sizeof bgm) < 0) {
		printf("skipped the game's music: no BGM/01.mp3 in the game folder (%s)\n", bf_file_location(BF_LOCATION_GAME));
	} else {
		HSTREAM h = AIL_open_stream(g_dig, "bgm\\01.mp3", 0);
		AIL_set_stream_volume(h, 100);
		AIL_set_stream_loop_count(h, 0);
		AIL_start_stream(h);
		bf_audio_counters c = counters();
		check(h && c.stream_status == BF_AUDIO_STREAM_PLAYING && ends_with(c.stream_file, "/BGM/01.mp3"),
		      "AIL_open_stream(\"bgm\\\\01.mp3\") plays %s (%.0f s)", c.stream_file, c.stream_duration);
		double r = 0;
		render(1.0, 0, &r);
		check(r > 0.001, "BGM/01.mp3 renders (rms %.4f)", r);
		// PlayStream("bgm\\08.mp3"): CloseStream first.
		AIL_close_stream(h);
		HSTREAM h8 = AIL_open_stream(g_dig, "bgm\\08.mp3", 0);
		AIL_start_stream(h8);
		c = counters();
		check(h8 && h8 != h && ends_with(c.stream_file, "/BGM/08.mp3") && c.stream_status == BF_AUDIO_STREAM_PLAYING,
		      "then bgm\\08.mp3: %s", c.stream_file);
		AIL_close_stream(h8);
	}
	{
		// The MP3 branch of CWave::Load, on the start of a music file.
		FILE *f = bgm[0] ? fopen(bgm, "rb") : 0;
		if (!f) {
			printf("skipped AIL_decompress_ASI on MP3: no game music\n");
		} else {
			std::vector<uint8_t> mp3(256 * 1024);
			mp3.resize(fread(mp3.data(), 1, mp3.size(), f));
			fclose(f);
			bool dec;
			U32 size = 0;
			BYTE *img = wave_load(mp3, &dec, &type, &size);
			bf_audio_info in;
			check(type == AILFILETYPE_MPEG_L3_AUDIO && dec && img && bf_audio_query(img, size, &in) == 0 &&
			          in.kind == BF_AUDIO_KIND_PCM_WAV && in.frames > 44100,
			      "AIL_decompress_ASI on 256 KB of BGM/01.mp3: a PCM WAV image of %lld frames at %d Hz",
			      (long long)(img ? in.frames : 0), img ? in.sample_rate : 0);
			if (img) {
				play_digital(img, 127);
				double r = 0;
				render(0.5, 0, &r);
				check(r > 0.001, "the decompressed MP3 image plays (rms %.4f)", r);
				AIL_end_sample(g_samples[0]);
				wave_reset(img, dec);
			}
		}
	}

	// ---- the 3D functions report "none"
	{
		check(AIL_open_3D_provider(1) != M3D_NOERR, "AIL_open_3D_provider fails");
		check(AIL_3D_speaker_type(1) == -1 && AIL_3D_room_type(1) == -1, "AIL_3D_speaker_type, AIL_3D_room_type: -1");
		check(AIL_open_3D_listener(1) == 0 && AIL_allocate_3D_sample_handle(1) == 0, "no 3D listener, no 3D sample handle");
		check(AIL_3D_sample_status(0) == SMP_DONE && AIL_set_3D_sample_file(0, img_pcm) == 0,
		      "AIL_3D_sample_status SMP_DONE, AIL_set_3D_sample_file 0");
		AIL_set_3D_speaker_type(1, 0);
		AIL_set_3D_room_type(1, 0);
		AIL_set_3D_sample_effects_level(0, 0.0f);
		AIL_set_3D_position(0, 1, 2, 3);
		AIL_set_3D_orientation(0, -1, -2, -3, 0, 1, 0);
		AIL_set_3D_sample_volume(0, 100);
		AIL_set_3D_sample_distances(0, 250, 40);
		AIL_start_3D_sample(0);
		AIL_end_3D_sample(0);
		AIL_release_3D_sample_handle(0);
		AIL_close_3D_listener(0);
		AIL_close_3D_provider(1);
		check(true, "the other 3D functions return");
	}

	// ---- CWave::Reset, UnInitMSS
	wave_reset(img_pcm, dec_pcm);
	wave_reset(img_ima, dec_ima);
	wave_reset(img_junk, dec_junk);
	HSAMPLE released = g_samples[0];
	uninit_mss();
	check(bf_audio_mode() == -1, "AIL_close_digital_driver stops the engine");
	check(AIL_sample_status(released) == SMP_FREE, "a released handle is SMP_FREE");
	AIL_start_sample(released);
	AIL_shutdown();   // again, as atexit would
	check(true, "AIL_shutdown twice");

	// ---- the device, as the client opens it (sound may play)
	{
		init_mss_2d();
		int mode = bf_audio_mode();
		check(mode == BF_AUDIO_MODE_DEVICE || mode == BF_AUDIO_MODE_SILENT, "the driver started the engine itself (mode %d)", mode);
		std::vector<uint8_t> beep = pcm16_wav(22050, sine(22050, 0.3, 660, 0.3));
		bool dec;
		BYTE *img = wave_load(beep, &dec, &type);
		long long started = counters().voices_started;
		check(play_digital(img, 100) && counters().voices_started == started + 1, "a sound started on the %s",
		      mode == BF_AUDIO_MODE_DEVICE ? "device" : "silent engine");
		usleep(400 * 1000);
		check(AIL_sample_status(g_samples[0]) == SMP_DONE, "and finished after 0.3 s");
		uninit_mss();
		wave_reset(img, dec);
	}

	unlink(loop_file.c_str());
	bf_trace_dump_counters();
	printf("%s: %d failures\n", g_failures ? "FAIL" : "PASS", g_failures);
	return g_failures ? 1 : 0;
}
