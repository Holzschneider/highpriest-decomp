// The Miles adapter (step M7.2a): the AIL_* names of mss32 that the client
// calls, on port/native/audio. port/native/audio/MILES-USAGE.md is the
// specification; README.md here has the table of names.
//
// Stage A only: M11 deletes this file and moves src/device/Sound.cpp and
// src/resource/Wave.cpp onto bf_audio.h.
//
// Handles. The client never looks inside a handle, so they are tokens, not
// pointers: a sample handle is (generation << 8 | slot + 1) and a stream
// handle a serial number. A released or stale handle is recognised and
// ignored, which matters for the stream: the client's fade thread may call
// with the handle of a stream the main thread has just closed.
//
// Threads. One adapter lock guards the handle tables and the sound cache.
// bf_audio calls made under it cannot call back, so this cannot deadlock.

#include "../../../src/device/Mss.h"

#include "bf_audio.h"
#include "bf_file.h"
#include "bf_trace.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mutex>
#include <string>
#include <unordered_map>

namespace {

const int kMaxSlots = 64;   // the client allocates at most MAXSAMPLE (48)

struct Slot {
	bool           used = false;
	uint32_t       generation = 0;
	const void    *image = nullptr;   // the file image of AIL_set_sample_file
	bf_audio_voice voice = 0;
	S32            volume = 127;      // Miles' default sample volume
};

// The sound cache: file image -> its length and its decoded sound.
struct Image {
	size_t          length = 0;      // 0: not known (RIFF size + 8 is used)
	bf_audio_sound *sound = nullptr;
	bool            failed = false;  // the sound could not be made; do not retry
};

struct State {
	std::mutex mutex;
	bool       driver_open = false;
	Slot       slots[kMaxSlots];
	std::unordered_map<const void *, Image> images;
	std::unordered_map<const void *, const void *> data_to_image;  // AIL_WAV_info's data_ptr -> image
	uintptr_t  stream = 0;          // the only stream token that acts; 0 when none
	uintptr_t  next_stream = 0;
	S32        preferences[256] = {};
	char       redist[_MAX_PATH] = {};
};

// Never destroyed: AIL_shutdown may run from atexit, after static destructors.
State &state()
{
	static State *s = new State;
	return *s;
}

char g_driver_token;   // HDIGDRIVER is the address of this byte

HDIGDRIVER driver_handle() { return reinterpret_cast<HDIGDRIVER>(&g_driver_token); }

Slot *slot_of(State &s, HSAMPLE h)
{
	uintptr_t t = reinterpret_cast<uintptr_t>(h);
	uintptr_t index = (t & 0xff);
	if (index == 0 || index > (uintptr_t)kMaxSlots)
		return nullptr;
	Slot &slot = s.slots[index - 1];
	if (!slot.used || slot.generation != (uint32_t)(t >> 8))
		return nullptr;
	return &slot;
}

uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

// The length of an image whose length was never recorded: the RIFF size + 8.
size_t length_of(State &s, const void *image)
{
	auto it = s.images.find(image);
	if (it != s.images.end() && it->second.length)
		return it->second.length;
	const uint8_t *p = static_cast<const uint8_t *>(image);
	if (memcmp(p, "RIFF", 4) == 0)
		return (size_t)le32(p + 4) + 8;
	return 0;
}

void forget_sound(Image &entry)
{
	bf_audio_sound_destroy(entry.sound);   // voices still playing it play to their end
	entry.sound = nullptr;
	entry.failed = false;
}

bf_audio_sound *sound_for(State &s, const void *image)
{
	if (!image)
		return nullptr;
	Image &entry = s.images[image];
	if (entry.sound || entry.failed)
		return entry.sound;
	size_t length = entry.length ? entry.length : length_of(s, image);
	if (length)
		entry.sound = bf_audio_sound_create_from_memory(image, length);
	if (!entry.sound) {
		entry.failed = true;
		bf_count("miles.undecodable_images", 1);
		BF_TRACE(BFT_SND, "miles: the image at %p (%zu bytes) cannot be played", image, length);
	}
	return entry.sound;
}

void stop_slot(Slot &slot)
{
	if (slot.voice)
		bf_audio_voice_stop(slot.voice);
	slot.voice = 0;
}

float to_unit(S32 v)
{
	if (v < 0) v = 0;
	if (v > 127) v = 127;
	return (float)v / 127.0f;
}

// One PCM WAV image (44-byte header and the samples) in one malloc block,
// as AIL_decompress_* hand it out and AIL_mem_free_lock frees it.
void *make_pcm_wav(const int16_t *samples, int64_t frames, int channels, int rate, size_t *size)
{
	size_t bytes = (size_t)frames * (size_t)channels * 2;
	uint8_t *p = static_cast<uint8_t *>(malloc(44 + bytes));
	if (!p)
		return nullptr;
	auto put32 = [](uint8_t *d, uint32_t v) { d[0] = (uint8_t)v; d[1] = (uint8_t)(v >> 8); d[2] = (uint8_t)(v >> 16); d[3] = (uint8_t)(v >> 24); };
	auto put16 = [](uint8_t *d, uint16_t v) { d[0] = (uint8_t)v; d[1] = (uint8_t)(v >> 8); };
	memcpy(p, "RIFF", 4);
	put32(p + 4, (uint32_t)(36 + bytes));
	memcpy(p + 8, "WAVEfmt ", 8);
	put32(p + 16, 16);
	put16(p + 20, 1);
	put16(p + 22, (uint16_t)channels);
	put32(p + 24, (uint32_t)rate);
	put32(p + 28, (uint32_t)(rate * channels * 2));
	put16(p + 32, (uint16_t)(channels * 2));
	put16(p + 34, 16);
	memcpy(p + 36, "data", 4);
	put32(p + 40, (uint32_t)bytes);
	memcpy(p + 44, samples, bytes);
	*size = 44 + bytes;
	return p;
}

// Decodes an image and stores the PCM WAV image in *out (and its length in
// *outsize). 1 on success, 0 on failure with *out = NULL.
S32 decompress(const void *image, size_t length, void **out, U32 *outsize, const char *what)
{
	if (out)
		*out = nullptr;
	if (outsize)
		*outsize = 0;
	if (!image || !length || !out) {
		BF_TRACE(BFT_SND, "miles: %s: no image", what);
		bf_count("miles.decompress_failures", 1);
		return 0;
	}
	bf_audio_info info;
	int64_t frames = 0;
	int16_t *pcm = bf_audio_decode_alloc(image, length, &info, &frames);
	if (!pcm) {
		BF_TRACE(BFT_SND, "miles: %s: the image at %p (%zu bytes) cannot be decoded", what, image, length);
		bf_count("miles.decompress_failures", 1);
		return 0;
	}
	size_t size = 0;
	void *wav = make_pcm_wav(pcm, frames, info.channels, info.sample_rate, &size);
	bf_audio_free(pcm);
	if (!wav)
		return 0;
	State &s = state();
	{
		std::lock_guard<std::mutex> lock(s.mutex);
		Image &entry = s.images[wav];   // a fresh block may sit where a freed image was
		forget_sound(entry);
		entry.length = size;
	}
	*out = wav;
	if (outsize)
		*outsize = (U32)size;
	return 1;
}

// Runs fn under the lock when token is the open stream; otherwise nothing.
template <typename Fn>
void with_stream(HSTREAM token, Fn fn)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	uintptr_t t = reinterpret_cast<uintptr_t>(token);
	if (t && t == s.stream)
		fn(s);
	else if (t)
		bf_count("miles.stale_stream_calls", 1);
}

} // namespace

extern "C" {

// ------------------------------------------------------------ start-up

char FAR *AILCALL AIL_set_redist_directory(char const FAR *dir)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	if (dir)
		strlcpy(s.redist, dir, sizeof s.redist);
	return s.redist;
}

S32 AILCALL AIL_startup(void)
{
	return 1;
}

S32 AILCALL AIL_set_preference(U32 number, S32 value)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	if (number >= sizeof s.preferences / sizeof s.preferences[0])
		return 0;
	S32 old = s.preferences[number];
	s.preferences[number] = value;
	return old;
}

HDIGDRIVER AILCALL AIL_open_digital_driver(U32 frequency, S32 bits, S32 channel, U32 flags)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	if (s.driver_open)
		return driver_handle();
	// An engine that is already running (a test that wants OFFLINE) is
	// adopted. Otherwise the device; SILENT when there is none is still a
	// driver, so that the client keeps counting voices and the stream.
	if (bf_audio_mode() < 0) {
		bf_audio_config config = { BF_AUDIO_MODE_DEVICE, BF_AUDIO_DEFAULT_VOICES, 0 };
		int mode = bf_audio_start(&config);
		if (mode < 0) {
			BF_TRACE(BFT_SND, "miles: AIL_open_digital_driver: the sound engine did not start");
			return 0;
		}
		BF_TRACE(BFT_SND, "miles: AIL_open_digital_driver(%u, %d, %d): engine mode %d at %d Hz",
		         (unsigned)frequency, (int)bits, (int)channel, mode, bf_audio_sample_rate());
	}
	s.driver_open = true;
	return driver_handle();
}

static void close_driver_locked(State &s)
{
	for (Slot &slot : s.slots) {
		if (slot.used) {
			stop_slot(slot);
			slot.used = false;
			++slot.generation;
		}
	}
	if (s.stream)
		bf_audio_stream_close();
	s.stream = 0;
	bf_audio_stop();
	s.driver_open = false;
}

void AILCALL AIL_close_digital_driver(HDIGDRIVER dig)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	if (!s.driver_open || dig != driver_handle())
		return;
	close_driver_locked(s);
}

void AILCALL AIL_shutdown(void)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	if (s.driver_open)
		close_driver_locked(s);
}

// ------------------------------------------------------------ sample handles

HSAMPLE AILCALL AIL_allocate_sample_handle(HDIGDRIVER dig)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	if (!s.driver_open || dig != driver_handle())
		return 0;
	for (int i = 0; i < kMaxSlots; ++i) {
		Slot &slot = s.slots[i];
		if (slot.used)
			continue;
		uint32_t generation = slot.generation;
		slot = Slot();
		slot.used = true;
		slot.generation = generation & 0xffffff;
		return reinterpret_cast<HSAMPLE>((uintptr_t)slot.generation << 8 | (uintptr_t)(i + 1));
	}
	BF_TRACE(BFT_SND, "miles: AIL_allocate_sample_handle: all %d handles in use", kMaxSlots);
	return 0;
}

void AILCALL AIL_release_sample_handle(HSAMPLE h)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	Slot *slot = slot_of(s, h);
	if (!slot)
		return;
	stop_slot(*slot);
	slot->used = false;
	slot->generation = (slot->generation + 1) & 0xffffff;
}

U32 AILCALL AIL_sample_status(HSAMPLE h)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	Slot *slot = slot_of(s, h);
	if (!slot)
		return SMP_FREE;
	return bf_audio_voice_playing(slot->voice) ? SMP_PLAYING : SMP_DONE;
}

void AILCALL AIL_init_sample(HSAMPLE h)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	Slot *slot = slot_of(s, h);
	if (!slot)
		return;
	stop_slot(*slot);
	slot->image = nullptr;
	slot->volume = 127;
}

S32 AILCALL AIL_set_sample_file(HSAMPLE h, void const FAR *file_image, S32 block)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	Slot *slot = slot_of(s, h);
	if (!slot)
		return 0;
	stop_slot(*slot);
	slot->image = nullptr;
	if (!sound_for(s, file_image))
		return 0;
	slot->image = file_image;
	return 1;
}

void AILCALL AIL_set_sample_volume(HSAMPLE h, S32 volume)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	Slot *slot = slot_of(s, h);
	if (!slot)
		return;
	slot->volume = volume < 0 ? 0 : (volume > 127 ? 127 : volume);
	if (slot->voice)
		bf_audio_voice_set_volume(slot->voice, to_unit(slot->volume));
}

void AILCALL AIL_start_sample(HSAMPLE h)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	Slot *slot = slot_of(s, h);
	if (!slot)
		return;
	stop_slot(*slot);
	// Looked up again: the image may have been reloaded since set_sample_file.
	bf_audio_sound *sound = sound_for(s, slot->image);
	if (!sound)
		return;
	slot->voice = bf_audio_play(sound, to_unit(slot->volume), 0.0f, 0);
}

void AILCALL AIL_end_sample(HSAMPLE h)
{
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	Slot *slot = slot_of(s, h);
	if (slot)
		stop_slot(*slot);
}

// ------------------------------------------------------------ decoding

S32 AILCALL AIL_file_type(void const FAR *data, U32 size)
{
	if (!data)
		return AILFILETYPE_UNKNOWN;
	int kind = bf_audio_file_kind(data, size);
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	// CWave::Load calls this for every image it loads: an entry left at this
	// address belongs to a freed image.
	Image &entry = s.images[data];
	forget_sound(entry);
	entry.length = size;
	return kind;
}

S32 AILCALL AIL_WAV_info(void const FAR *data, AILSOUNDINFO FAR *info)
{
	if (!data || !info)
		return 0;
	memset(info, 0, sizeof *info);
	State &s = state();
	size_t length;
	{
		std::lock_guard<std::mutex> lock(s.mutex);
		length = length_of(s, data);
	}
	const uint8_t *p = static_cast<const uint8_t *>(data);
	if (length < 12 || memcmp(p, "RIFF", 4) != 0 || memcmp(p + 8, "WAVE", 4) != 0)
		return 0;
	const uint8_t *fmt = nullptr, *chunk = nullptr;
	uint32_t chunk_len = 0;
	for (size_t at = 12; at + 8 <= length;) {
		uint32_t n = le32(p + at + 4);
		if (memcmp(p + at, "fmt ", 4) == 0 && n >= 16 && at + 8 + 16 <= length)
			fmt = p + at + 8;
		else if (memcmp(p + at, "data", 4) == 0) {
			chunk = p + at + 8;
			chunk_len = (uint32_t)(n <= length - at - 8 ? n : length - at - 8);
			break;
		}
		at += 8 + (size_t)n + (n & 1);
	}
	if (!fmt || !chunk)
		return 0;
	bf_audio_info a;
	int64_t frames = bf_audio_query(data, length, &a) == 0 ? a.frames : 0;
	info->format = le16(fmt);
	info->data_ptr = chunk;
	info->initial_ptr = chunk;
	info->data_len = chunk_len;
	info->channels = le16(fmt + 2);
	info->rate = le32(fmt + 4);
	info->block_size = le16(fmt + 12);
	info->bits = le16(fmt + 14);
	info->samples = (U32)frames;
	std::lock_guard<std::mutex> lock(s.mutex);
	s.data_to_image[chunk] = data;
	return 1;
}

S32 AILCALL AIL_decompress_ADPCM(AILSOUNDINFO const FAR *info, void FAR *FAR *outdata, U32 FAR *outsize)
{
	const void *image = nullptr;
	size_t length = 0;
	if (info) {
		State &s = state();
		std::lock_guard<std::mutex> lock(s.mutex);
		auto it = s.data_to_image.find(info->data_ptr);
		if (it != s.data_to_image.end()) {
			image = it->second;
			length = length_of(s, image);
			s.data_to_image.erase(it);
		}
	}
	if (!image && info)
		BF_TRACE(BFT_SND, "miles: AIL_decompress_ADPCM: data at %p came from no AIL_WAV_info", info->data_ptr);
	return decompress(image, length, outdata, outsize, "AIL_decompress_ADPCM");
}

S32 AILCALL AIL_decompress_ASI(void const FAR *indata, U32 insize, char const FAR *filename_ext,
                               void FAR *FAR *wav, U32 FAR *wavsize, AILLENGTHYCB callback)
{
	return decompress(indata, insize, wav, wavsize, "AIL_decompress_ASI");
}

void AILCALL AIL_mem_free_lock(void FAR *ptr)
{
	if (!ptr)
		return;
	{
		State &s = state();
		std::lock_guard<std::mutex> lock(s.mutex);
		auto it = s.images.find(ptr);
		if (it != s.images.end()) {
			forget_sound(it->second);
			s.images.erase(it);
		}
	}
	free(ptr);
}

// ------------------------------------------------------------ the stream

HSTREAM AILCALL AIL_open_stream(HDIGDRIVER dig, char const FAR *filename, S32 stream_mem)
{
	if (!filename)
		return 0;
	// "bgm\01.mp3" relative to the game folder, any letter case (BGM/).
	char path[2048];
	if (bf_file_resolve(filename, BF_RESOLVE_READ, path, sizeof path) < 0) {
		// Handed on as it is, so that the module counts the failure.
		const char *game = bf_file_location(BF_LOCATION_GAME);
		std::string p = filename[0] == '/' ? std::string() : std::string(game ? game : ".") + "/";
		for (const char *c = filename; *c; ++c)
			p += *c == '\\' ? '/' : *c;
		strlcpy(path, p.c_str(), sizeof path);
	}
	State &s = state();
	std::lock_guard<std::mutex> lock(s.mutex);
	if (!s.driver_open || dig != driver_handle()) {
		BF_TRACE(BFT_SND, "miles: AIL_open_stream(%s) with no driver open", filename);
		return 0;
	}
	s.stream = 0;
	if (bf_audio_stream_open(path) != 0) {
		BF_TRACE(BFT_SND, "miles: AIL_open_stream: %s (%s) cannot be opened", filename, path);
		return 0;
	}
	if (++s.next_stream == 0)
		++s.next_stream;
	s.stream = s.next_stream;
	return reinterpret_cast<HSTREAM>(s.stream);
}

void AILCALL AIL_close_stream(HSTREAM stream)
{
	with_stream(stream, [](State &s) {
		bf_audio_stream_close();
		s.stream = 0;
	});
}

void AILCALL AIL_start_stream(HSTREAM stream)
{
	with_stream(stream, [](State &) { bf_audio_stream_start(); });
}

void AILCALL AIL_pause_stream(HSTREAM stream, S32 onoff)
{
	with_stream(stream, [onoff](State &) { bf_audio_stream_pause(onoff ? 1 : 0); });
}

void AILCALL AIL_set_stream_volume(HSTREAM stream, S32 volume)
{
	with_stream(stream, [volume](State &) { bf_audio_stream_set_volume(to_unit(volume)); });
}

void AILCALL AIL_set_stream_loop_count(HSTREAM stream, S32 count)
{
	with_stream(stream, [count](State &) { bf_audio_stream_set_loop_count(count < 0 ? 0 : count); });
}

// PauseStream calls this with 0 when no stream is open; the result is unused.
S32 AILCALL AIL_stream_volume(HSTREAM stream)
{
	return (S32)lroundf(bf_audio_stream_volume() * 127.0f);
}

// ------------------------------------------------------------ 3D: no provider
// Reached only with g_soundMode == 2, which WinMain never sets. With no
// provider, Play3DDigital falls back to PlayDigital.

S32 AILCALL AIL_enumerate_3D_providers(HPROENUM FAR *next, HPROVIDER FAR *dest, C8 FAR *FAR *name)
{
	return 0;
}

M3DRESULT AILCALL AIL_open_3D_provider(HPROVIDER lib)
{
	return M3D_INTERNAL_ERR;
}

void AILCALL AIL_close_3D_provider(HPROVIDER lib) {}

S32 AILCALL AIL_3D_speaker_type(HPROVIDER lib)
{
	return -1;
}

void AILCALL AIL_set_3D_speaker_type(HPROVIDER lib, S32 speaker_type) {}

S32 AILCALL AIL_3D_room_type(HPROVIDER lib)
{
	return -1;
}

void AILCALL AIL_set_3D_room_type(HPROVIDER lib, S32 room_type) {}

H3DPOBJECT AILCALL AIL_open_3D_listener(HPROVIDER lib)
{
	return 0;
}

void AILCALL AIL_close_3D_listener(H3DPOBJECT listener) {}

H3DSAMPLE AILCALL AIL_allocate_3D_sample_handle(HPROVIDER lib)
{
	return 0;
}

void AILCALL AIL_release_3D_sample_handle(H3DSAMPLE S) {}

U32 AILCALL AIL_3D_sample_status(H3DSAMPLE S)
{
	return SMP_DONE;
}

S32 AILCALL AIL_set_3D_sample_file(H3DSAMPLE S, void const FAR *file_image)
{
	return 0;
}

void AILCALL AIL_set_3D_position(H3DPOBJECT obj, F32 X, F32 Y, F32 Z) {}

void AILCALL AIL_set_3D_orientation(H3DPOBJECT obj, F32 X_face, F32 Y_face, F32 Z_face, F32 X_up, F32 Y_up, F32 Z_up) {}

void AILCALL AIL_set_3D_sample_volume(H3DSAMPLE S, S32 volume) {}

void AILCALL AIL_set_3D_sample_distances(H3DSAMPLE S, F32 max_dist, F32 min_dist) {}

void AILCALL AIL_set_3D_sample_effects_level(H3DSAMPLE S, F32 effects_level) {}

void AILCALL AIL_start_3D_sample(H3DSAMPLE S) {}

void AILCALL AIL_end_3D_sample(H3DSAMPLE S) {}

} // extern "C"
