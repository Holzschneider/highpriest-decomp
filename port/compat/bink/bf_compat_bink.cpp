// The Bink SDK names the client calls (port/compat/include/bf_bink_sdk.h) on
// the Bink reader (port/native/formats/bink), the sound module and the sys
// clock. Stage A only (M12.4a); M11 deletes it.
//
// Timing: frame n (1-based) is due n-1 frame periods after frame 1 was
// decoded; BinkWait compares that with bf_clock_us. Frames are never skipped:
// a client that falls behind shows every frame late (the reader decodes a
// 512x384 frame in a few milliseconds).
//
// Sound: the first track of the movie is decoded completely at BinkOpen, from
// a second reader handle with the picture turned off, and played as one
// sound-module voice started with frame 1. Pause and BinkGoto stop the voice
// and start the rest of the track from the movie's position. The voice is
// tagged with the movie's name, so the control socket's `sound` part shows it.
#include <windows.h>
#include <ddraw.h>

#include "bf_bink_sdk.h"
#include "bf_compat_bink.h"

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

namespace {

struct Movie {
	bf_bink *reader = nullptr;
	std::string name;
	bf_bink_info info{};
	bool started = false;        // frame 1 decoded: the clock runs
	uint64_t base_us = 0;        // when frame 1 was (or would have been) shown
	uint64_t paused_us = 0;      // when BinkPause(1) came
	// The first sound track, whole.
	std::vector<int16_t> pcm;
	int channels = 0;
	int rate = 0;
	bf_audio_voice voice = 0;
	bf_audio_sound *sound = nullptr;   // the part playing now
};

std::mutex g_lock;                 // the state below, read by the control socket
bf_compat_bink_state g_state{};
bool g_sound_system = false;       // BinkSetSoundSystem was called
std::string g_error = "";

double Fps(const bf_bink_info &in) { return in.rate_den ? (double)in.rate_num / in.rate_den : 0; }

uint64_t FrameUs(const Movie &m, unsigned frame1)   // offset of a 1-based frame from frame 1
{
	if (!m.info.rate_num || frame1 < 1)
		return 0;
	return (uint64_t)(frame1 - 1) * 1000000ull * m.info.rate_den / m.info.rate_num;
}

Movie *Of(HBINK b) { return b ? (Movie *)b->bf : nullptr; }

void Publish(HBINK b)
{
	Movie *m = Of(b);
	std::lock_guard<std::mutex> g(g_lock);
	g_state.paused = b->Paused;
	g_state.frame = (int)b->LastFrameNum;
	g_state.voice = m ? m->voice : 0;
}

void StopSound(Movie &m)
{
	if (m.voice)
		bf_audio_voice_stop(m.voice);
	m.voice = 0;
	if (m.sound)
		bf_audio_sound_destroy(m.sound);   // a voice still sounding plays to its end; it was stopped
	m.sound = nullptr;
}

// Starts the track from the movie time `at_us` (from frame 1).
void StartSound(Movie &m, uint64_t at_us)
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
void ReadSound(Movie &m, const std::vector<uint8_t> &bytes)
{
	if (!g_sound_system || m.info.audio_tracks < 1)
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
		bf_audio_start(nullptr);   // the client starts Miles later; it adopts this engine
}

HBINK Open(std::vector<uint8_t> &bytes, const char *name, unsigned flags)
{
	bf_bink *r = bf_bink_open_memory(bytes.data(), bytes.size());
	if (!r) {
		g_error = std::string(name) + " is not a Bink 1 movie the reader accepts";
		BF_TRACE(BFT_GFX, "bink: %s", g_error.c_str());
		return nullptr;
	}
	Movie *m = new Movie;
	m->reader = r;
	m->name = name;
	bf_bink_get_info(r, &m->info);
	ReadSound(*m, bytes);

	BINK *b = new BINK();
	b->Width = m->info.width;
	b->Height = m->info.height;
	b->Frames = m->info.frames;
	b->FrameNum = 1;
	b->LastFrameNum = 0;
	b->FrameRate = m->info.rate_num;
	b->FrameRateDiv = m->info.rate_den;
	b->OpenFlags = flags;
	b->Size = (unsigned)bytes.size();
	b->bf = m;
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
	return b;
}

int SurfaceFormat(unsigned flags, int *bytes)
{
	switch (flags & BINKSURFACEMASK) {
	case BINKSURFACE32: *bytes = 4; return BF_BINK_BGRA8888;
	case BINKSURFACE565: *bytes = 2; return BF_BINK_RGB565;
	case BINKSURFACE555:
	case BINKSURFACE5551: *bytes = 2; return BF_BINK_XRGB1555;
	default: return -1;
	}
}

}  // namespace

extern "C" {

void *BinkOpenDirectSound(uintptr_t) { return nullptr; }

int BinkSetSoundSystem(BINKSNDSYSOPEN open, uintptr_t)
{
	g_sound_system = open != nullptr;
	return 1;
}

HBINK BinkOpen(const char *name, unsigned int flags)
{
	if (!name) {
		g_error = "no name";
		return nullptr;
	}
	if (flags & BINKFROMMEMORY) {
		// The SDK reads the size from the header: bytes 4..7 are the file size less 8.
		const uint8_t *p = (const uint8_t *)name;
		uint32_t size = (uint32_t)p[4] | (uint32_t)p[5] << 8 | (uint32_t)p[6] << 16 | (uint32_t)p[7] << 24;
		return BfBinkOpenMemory(p, (size_t)size + 8, "(memory)");
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
	return Open(bytes, name, flags);
}

HBINK BfBinkOpenMemory(const void *data, size_t size, const char *name)
{
	if (!data || size < 44) {
		g_error = "no movie in memory";
		return nullptr;
	}
	std::vector<uint8_t> bytes((const uint8_t *)data, (const uint8_t *)data + size);
	return Open(bytes, name ? name : "(memory)", BINKFROMMEMORY);
}

int BinkDoFrame(HBINK b)
{
	Movie *m = Of(b);
	if (!m)
		return 0;
	unsigned want = b->FrameNum ? b->FrameNum - 1 : 0;   // 0-based
	if (bf_bink_frame(m->reader) != (int)want) {
		if (bf_bink_frame(m->reader) + 1 != (int)want)
			bf_bink_seek(m->reader, want);
		if (bf_bink_decode_next(m->reader) < 0)
			bf_count("bink.damaged_frames", 1);
	}
	b->LastFrameNum = b->FrameNum;
	if (!m->started) {
		// The clock starts with the first picture, and the sound with it.
		m->started = true;
		m->base_us = bf_clock_us() - FrameUs(*m, b->FrameNum);
		if (!b->Paused)
			StartSound(*m, FrameUs(*m, b->FrameNum));
	}
	Publish(b);
	return 0;
}

void BinkNextFrame(HBINK b)
{
	if (!b)
		return;
	if (b->FrameNum < b->Frames)
		b->FrameNum++;
	else
		BinkGoto(b, 1, 0);   // the SDK loops
}

int BinkWait(HBINK b)
{
	Movie *m = Of(b);
	if (!m)
		return 0;
	if (b->Paused)
		return 1;
	if (!m->started)
		return 0;
	return bf_clock_us() < m->base_us + FrameUs(*m, b->FrameNum) ? 1 : 0;
}

void BinkClose(HBINK b)
{
	Movie *m = Of(b);
	if (!m)
		return;
	StopSound(*m);
	bool ended = b->LastFrameNum >= b->Frames;
	BF_TRACE(BFT_GFX, "bink: close %s after frame %u of %u (%s)", m->name.c_str(), b->LastFrameNum, b->Frames,
	         ended ? "ended" : "stopped");
	{
		std::lock_guard<std::mutex> g(g_lock);
		g_state.playing = 0;
		g_state.paused = 0;
		g_state.voice = 0;
		g_state.closed++;
		g_state.ended = ended;
		g_state.last_frame = (int)b->LastFrameNum;
		g_state.frame = (int)b->LastFrameNum;
	}
	bf_bink_close(m->reader);
	delete m;
	delete b;
}

int BinkPause(HBINK b, int pause)
{
	Movie *m = Of(b);
	if (!m)
		return 0;
	pause = pause != 0;
	if (pause == b->Paused)
		return pause;
	b->Paused = pause;
	uint64_t now = bf_clock_us();
	if (pause) {
		m->paused_us = now;
		StopSound(*m);
	} else if (m->started) {
		m->base_us += now - m->paused_us;   // the movie's clock stood still
		StartSound(*m, now - m->base_us);
	}
	BF_TRACE(BFT_GFX, "bink: %s %s at frame %u", m->name.c_str(), pause ? "paused" : "resumed", b->LastFrameNum);
	Publish(b);
	return pause;
}

int BinkCopyToBuffer(HBINK b, void *dest, int destpitch, unsigned int destheight, unsigned int destx,
                     unsigned int desty, unsigned int flags)
{
	Movie *m = Of(b);
	if (!m || !dest || destpitch <= 0)
		return -1;
	int bytes = 0;
	int format = SurfaceFormat(flags, &bytes);
	if (format < 0) {
		static bool said = false;
		if (!said)
			BF_TRACE(BFT_GFX, "bink: surface type %u is not supported", flags & BINKSURFACEMASK);
		said = true;
		return -1;
	}
	return bf_bink_copy_pixels(m->reader, dest, destpitch, destpitch / bytes, (int)destheight, (int)destx,
	                           (int)desty, format);
}

void BinkGoto(HBINK b, unsigned int frame, int)
{
	Movie *m = Of(b);
	if (!m || !b->Frames)
		return;
	if (frame < 1)
		frame = 1;
	if (frame > b->Frames)
		frame = b->Frames;
	b->FrameNum = frame;
	// Positioned now; DoFrame decodes from the key frame. The movie time of
	// that frame is now.
	if (m->started) {
		uint64_t now = b->Paused ? m->paused_us : bf_clock_us();
		m->base_us = now - FrameUs(*m, frame);
		if (!b->Paused)
			StartSound(*m, FrameUs(*m, frame));
	}
}

int BinkDDSurfaceType(void *lpDDS)
{
	IDirectDrawSurface7 *s = (IDirectDrawSurface7 *)lpDDS;
	if (!s)
		return -1;
	DDPIXELFORMAT pf;
	memset(&pf, 0, sizeof pf);
	pf.dwSize = sizeof pf;
	if (FAILED(s->GetPixelFormat(&pf)) || !(pf.dwFlags & DDPF_RGB))
		return -1;
	if (pf.dwRGBBitCount == 32 && pf.dwRBitMask == 0xFF0000 && pf.dwBBitMask == 0xFF)
		return BINKSURFACE32;
	if (pf.dwRGBBitCount == 16 && pf.dwGBitMask == 0x07E0)
		return BINKSURFACE565;
	if (pf.dwRGBBitCount == 16 && pf.dwGBitMask == 0x03E0)
		return (pf.dwFlags & DDPF_ALPHAPIXELS) ? BINKSURFACE5551 : BINKSURFACE555;
	return -1;
}

const char *BinkGetError(void) { return g_error.c_str(); }

void bf_compat_bink_get_state(bf_compat_bink_state *out)
{
	if (!out)
		return;
	std::lock_guard<std::mutex> g(g_lock);
	*out = g_state;
}

}  // extern "C"
