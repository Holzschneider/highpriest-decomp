// The intro movie's player (bf_movie.h): what src/movie/CBink.cpp calls since
// M11.10, on the Bink reader, the sound module and the sys clock. The rules
// are those of the Bink adapter of M12.4a (port/compat/bink, deleted in
// M11.10), unchanged: timing, sound, the state for the control socket, the
// trace lines.

#include "bf_movie.h"

#include "bf_audio.h"
#include "bf_bink.h"
#include "bf_clock.h"
#include "bf_file.h"
#include "bf_trace.h"

#include <stdio.h>
#include <string.h>

#include <mutex>
#include <string>
#include <vector>

struct bf_movie {
	bf_bink *reader = nullptr;
	std::string name;
	bf_bink_info info{};
	unsigned frame = 1;          // the frame to be shown next, 1-based
	unsigned last_frame = 0;     // the frame last decoded, 1-based (0 before the first)
	int paused = 0;
	bool started = false;        // frame 1 decoded: the clock runs
	uint64_t base_us = 0;        // when frame 1 was (or would have been) shown
	uint64_t paused_us = 0;      // when the pause came
	// The first sound track, whole.
	std::vector<int16_t> pcm;
	int channels = 0;
	int rate = 0;
	bf_audio_voice voice = 0;
	bf_audio_sound *sound = nullptr;   // the part playing now
};

namespace {

std::mutex g_lock;                 // the state below, read by the control socket
bf_movie_state g_state{};
bool g_sound = false;              // bf_movie_use_sound
std::string g_error = "";

double Fps(const bf_bink_info &in) { return in.rate_den ? (double)in.rate_num / in.rate_den : 0; }

uint64_t FrameUs(const bf_movie &m, unsigned frame1)   // offset of a 1-based frame from frame 1
{
	if (!m.info.rate_num || frame1 < 1)
		return 0;
	return (uint64_t)(frame1 - 1) * 1000000ull * m.info.rate_den / m.info.rate_num;
}

void Publish(const bf_movie &m)
{
	std::lock_guard<std::mutex> g(g_lock);
	g_state.paused = m.paused;
	g_state.frame = (int)m.last_frame;
	g_state.voice = m.voice;
}

void StopSound(bf_movie &m)
{
	if (m.voice)
		bf_audio_voice_stop(m.voice);
	m.voice = 0;
	if (m.sound)
		bf_audio_sound_destroy(m.sound);   // a voice still sounding plays to its end; it was stopped
	m.sound = nullptr;
}

// Starts the track from the movie time `at_us` (from frame 1).
void StartSound(bf_movie &m, uint64_t at_us)
{
	StopSound(m);
	if (m.pcm.empty() || m.channels <= 0)
		return;
	int64_t total = (int64_t)(m.pcm.size() / (size_t)m.channels);
	int64_t from = (int64_t)(at_us * (uint64_t)m.rate / 1000000ull);
	if (from >= total)
		return;
	m.sound = bf_audio_sound_create(m.pcm.data() + (size_t)from * (size_t)m.channels, total - from, m.channels, m.rate);
	if (!m.sound)
		return;
	bf_audio_tag_next_voice(m.name.c_str(), -1);
	m.voice = bf_audio_play(m.sound, 1.0f, 0.0f, 0);
	BF_TRACE(BFT_SND, "bink: %s: sound from %.3f s, voice %u", m.name.c_str(), at_us / 1e6, (unsigned)m.voice);
}

// Reads the first sound track completely, from a second handle without picture.
void ReadSound(bf_movie &m, const std::vector<uint8_t> &bytes)
{
	if (!g_sound || m.info.audio_tracks < 1)
		return;
	bf_bink_track_info tr;
	if (bf_bink_get_track(m.reader, 0, &tr) != 0)
		return;
	bf_bink *a = bf_bink_open_memory(bytes.data(), bytes.size());
	if (!a)
		return;
	bf_bink_set_video_enabled(a, 0);
	for (int t = 1; t < m.info.audio_tracks; ++t)
		bf_bink_set_audio_enabled(a, t, 0);
	m.channels = tr.channels;
	m.rate = (int)tr.sample_rate;
	while (bf_bink_decode_next(a) != 0) {
		size_t n = bf_bink_audio_available(a, 0);
		size_t at = m.pcm.size();
		m.pcm.resize(at + n * (size_t)tr.channels);
		bf_bink_audio_read(a, 0, m.pcm.data() + at, n);
	}
	bf_bink_close(a);
	// The decoder yields up to one block more sound than picture; keep the
	// picture's length (bf_bink.h, "Length").
	size_t keep = (size_t)((double)m.info.frames / Fps(m.info) * m.rate + 0.5) * (size_t)m.channels;
	if (Fps(m.info) > 0 && m.pcm.size() > keep)
		m.pcm.resize(keep);
	if (bf_audio_mode() < 0)
		bf_audio_start(nullptr);   // the client starts its sound later; it adopts this engine
}

bf_movie *Open(std::vector<uint8_t> &bytes, const char *name)
{
	bf_bink *r = bf_bink_open_memory(bytes.data(), bytes.size());
	if (!r) {
		g_error = std::string(name) + " is not a Bink 1 movie the reader accepts";
		BF_TRACE(BFT_GFX, "bink: %s", g_error.c_str());
		return nullptr;
	}
	bf_movie *m = new bf_movie;
	m->reader = r;
	m->name = name;
	bf_bink_get_info(r, &m->info);
	ReadSound(*m, bytes);
	{
		std::lock_guard<std::mutex> g(g_lock);
		g_state.playing = 1;
		g_state.paused = 0;
		snprintf(g_state.file, sizeof g_state.file, "%s", name);
		g_state.width = (int)m->info.width;
		g_state.height = (int)m->info.height;
		g_state.frames = (int)m->info.frames;
		g_state.frame = 0;
		g_state.fps = Fps(m->info);
		g_state.tracks = m->info.audio_tracks;
		g_state.sound = !m->pcm.empty();
		g_state.voice = 0;
		g_state.sound_seconds = m->rate ? (double)m->pcm.size() / m->channels / m->rate : 0;
		g_state.opened++;
	}
	BF_TRACE(BFT_GFX, "bink: open %s: %ux%u, %u frames at %.3f fps, %d sound track(s)%s", name, m->info.width,
	         m->info.height, m->info.frames, Fps(m->info), m->info.audio_tracks,
	         m->pcm.empty() ? (m->info.audio_tracks ? ", sound off" : "") : ", first track played");
	bf_count("bink.opened", 1);
	return m;
}

int ReaderFormat(int format, int *bytes)
{
	switch (format) {
	case BF_MOVIE_BGRA8888: *bytes = 4; return BF_BINK_BGRA8888;
	case BF_MOVIE_RGB565: *bytes = 2; return BF_BINK_RGB565;
	case BF_MOVIE_XRGB1555: *bytes = 2; return BF_BINK_XRGB1555;
	default: return -1;
	}
}

}  // namespace

extern "C" {

void bf_movie_use_sound(int on)
{
	g_sound = on != 0;
}

bf_movie *bf_movie_open(const char *name)
{
	if (!name) {
		g_error = "no name";
		return nullptr;
	}
	char path[1024];
	if (name[0] == '/')
		snprintf(path, sizeof path, "%s", name);
	else if (bf_file_resolve(name, BF_RESOLVE_READ, path, sizeof path) < 0) {
		g_error = std::string(name) + " not found in the game folder";
		BF_TRACE(BFT_GFX, "bink: %s", g_error.c_str());
		bf_count("bink.missing", 1);
		return nullptr;
	}
	std::vector<uint8_t> bytes;
	FILE *fp = fopen(path, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		long n = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if (n > 0) {
			bytes.resize((size_t)n);
			if (fread(bytes.data(), 1, (size_t)n, fp) != (size_t)n)
				bytes.clear();
		}
		fclose(fp);
	}
	if (bytes.empty()) {
		g_error = std::string(path) + " cannot be read";
		BF_TRACE(BFT_GFX, "bink: %s", g_error.c_str());
		bf_count("bink.missing", 1);
		return nullptr;
	}
	return Open(bytes, name);
}

bf_movie *bf_movie_open_memory(const void *data, size_t size, const char *name)
{
	if (!data || size < 44) {
		g_error = "no movie in memory";
		return nullptr;
	}
	std::vector<uint8_t> bytes((const uint8_t *)data, (const uint8_t *)data + size);
	return Open(bytes, name ? name : "(memory)");
}

void bf_movie_decode(bf_movie *m)
{
	if (!m)
		return;
	unsigned want = m->frame ? m->frame - 1 : 0;   // 0-based
	if (bf_bink_frame(m->reader) != (int)want) {
		if (bf_bink_frame(m->reader) + 1 != (int)want)
			bf_bink_seek(m->reader, want);
		if (bf_bink_decode_next(m->reader) < 0)
			bf_count("bink.damaged_frames", 1);
	}
	m->last_frame = m->frame;
	if (!m->started) {
		// The clock starts with the first picture, and the sound with it.
		m->started = true;
		m->base_us = bf_clock_us() - FrameUs(*m, m->frame);
		if (!m->paused)
			StartSound(*m, FrameUs(*m, m->frame));
	}
	Publish(*m);
}

void bf_movie_next(bf_movie *m)
{
	if (!m)
		return;
	if (m->frame < m->info.frames)
		m->frame++;
	else
		bf_movie_goto(m, 1);   // the SDK loops
}

int bf_movie_wait(bf_movie *m)
{
	if (!m)
		return 0;
	if (m->paused)
		return 1;
	if (!m->started)
		return 0;
	return bf_clock_us() < m->base_us + FrameUs(*m, m->frame) ? 1 : 0;
}

void bf_movie_close(bf_movie *m)
{
	if (!m)
		return;
	StopSound(*m);
	bool ended = m->last_frame >= m->info.frames;
	BF_TRACE(BFT_GFX, "bink: close %s after frame %u of %u (%s)", m->name.c_str(), m->last_frame, m->info.frames,
	         ended ? "ended" : "stopped");
	{
		std::lock_guard<std::mutex> g(g_lock);
		g_state.playing = 0;
		g_state.paused = 0;
		g_state.voice = 0;
		g_state.closed++;
		g_state.ended = ended;
		g_state.last_frame = (int)m->last_frame;
		g_state.frame = (int)m->last_frame;
	}
	bf_bink_close(m->reader);
	delete m;
}

int bf_movie_pause(bf_movie *m, int pause)
{
	if (!m)
		return 0;
	pause = pause != 0;
	if (pause == m->paused)
		return pause;
	m->paused = pause;
	uint64_t now = bf_clock_us();
	if (pause) {
		m->paused_us = now;
		StopSound(*m);
	} else if (m->started) {
		m->base_us += now - m->paused_us;   // the movie's clock stood still
		StartSound(*m, now - m->base_us);
	}
	BF_TRACE(BFT_GFX, "bink: %s %s at frame %u", m->name.c_str(), pause ? "paused" : "resumed", m->last_frame);
	Publish(*m);
	return pause;
}

int bf_movie_copy(bf_movie *m, void *dest, int pitch, unsigned height, unsigned x, unsigned y, int format)
{
	if (!m || !dest || pitch <= 0)
		return -1;
	int bytes = 0;
	int reader = ReaderFormat(format, &bytes);
	if (reader < 0) {
		static bool said = false;
		if (!said)
			BF_TRACE(BFT_GFX, "bink: surface format %d is not supported", format);
		said = true;
		return -1;
	}
	return bf_bink_copy_pixels(m->reader, dest, pitch, pitch / bytes, (int)height, (int)x, (int)y, reader);
}

void bf_movie_goto(bf_movie *m, unsigned frame)
{
	if (!m || !m->info.frames)
		return;
	if (frame < 1)
		frame = 1;
	if (frame > m->info.frames)
		frame = m->info.frames;
	m->frame = frame;
	// Positioned now; decode starts from the key frame. The movie time of
	// that frame is now.
	if (m->started) {
		uint64_t now = m->paused ? m->paused_us : bf_clock_us();
		m->base_us = now - FrameUs(*m, frame);
		if (!m->paused)
			StartSound(*m, FrameUs(*m, frame));
	}
}

unsigned bf_movie_frames(const bf_movie *m) { return m ? m->info.frames : 0; }
unsigned bf_movie_frame(const bf_movie *m) { return m ? m->frame : 0; }
unsigned bf_movie_last_frame(const bf_movie *m) { return m ? m->last_frame : 0; }

const char *bf_movie_error(void) { return g_error.c_str(); }

void bf_movie_get_state(bf_movie_state *out)
{
	if (!out)
		return;
	std::lock_guard<std::mutex> g(g_lock);
	*out = g_state;
}

}  // extern "C"
