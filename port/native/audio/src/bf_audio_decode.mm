// WAV and MP3 decoding through AudioToolbox (AudioFile + ExtAudioFile), with
// a small built-in reader for PCM WAV files whose RIFF header AudioToolbox
// refuses. Stateless apart from the counters; any thread.

#include "bf_audio.h"
#include "bf_audio_internal.h"
#include "bf_trace.h"

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <atomic>
#include <vector>

std::atomic<long long> bf_audio_n_decodes(0);
std::atomic<long long> bf_audio_n_decode_failures(0);
std::atomic<long long> bf_audio_n_decode_fallbacks(0);

namespace {

uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

// What the first bytes say: the kind, and for a RIFF WAVE the fmt chunk and
// where its data chunk is (clipped to the bytes present).
struct Sniff {
	int kind = BF_AUDIO_KIND_UNKNOWN;
	int tag = 0, channels = 0, rate = 0, bits = 0, align = 0;
	size_t data_at = 0, data_len = 0;
	bool have_data = false;
};

bool mp3_sync(const uint8_t *p, size_t n)
{
	// Frame sync (11 bits), MPEG version not "reserved", layer III.
	return n >= 4 && p[0] == 0xFF && (p[1] & 0xE0) == 0xE0 && (p[1] & 0x18) != 0x08 && (p[1] & 0x06) == 0x02;
}

Sniff sniff(const void *data, size_t size)
{
	Sniff s;
	const uint8_t *p = (const uint8_t *)data;
	if (!p || size < 4)
		return s;
	if (size >= 12 && memcmp(p, "RIFF", 4) == 0 && memcmp(p + 8, "WAVE", 4) == 0) {
		s.kind = BF_AUDIO_KIND_OTHER_WAV;
		size_t at = 12;
		bool have_fmt = false;
		while (at + 8 <= size) {
			uint32_t len = rd32(p + at + 4);
			if (memcmp(p + at, "fmt ", 4) == 0 && at + 8 + 16 <= size) {
				s.tag = rd16(p + at + 8);
				s.channels = rd16(p + at + 10);
				s.rate = (int)rd32(p + at + 12);
				s.align = rd16(p + at + 20);
				s.bits = rd16(p + at + 22);
				if (s.tag == 0xFFFE && len >= 40 && at + 8 + 26 <= size)
					s.tag = rd16(p + at + 8 + 24);   // WAVE_FORMAT_EXTENSIBLE: the sub-format GUID's first two bytes
				have_fmt = true;
			} else if (memcmp(p + at, "data", 4) == 0) {
				s.data_at = at + 8;
				s.data_len = len;
				if (s.data_at > size) s.data_at = size;
				if (s.data_len > size - s.data_at) s.data_len = size - s.data_at;
				s.have_data = true;
				break;
			}
			if (len > size)   // a broken size: no further chunk can be found
				break;
			at += 8 + (size_t)len + (len & 1);
		}
		if (have_fmt) {
			if (s.tag == 1) s.kind = BF_AUDIO_KIND_PCM_WAV;
			else if (s.tag == 2 || s.tag == 0x11) s.kind = BF_AUDIO_KIND_ADPCM_WAV;
		}
		return s;
	}
	if (size >= 10 && memcmp(p, "ID3", 3) == 0) {
		// Skip the tag (synch-safe size) and confirm a frame follows.
		size_t tag = 10 + (((size_t)(p[6] & 0x7f) << 21) | ((p[7] & 0x7f) << 14) | ((p[8] & 0x7f) << 7) | (p[9] & 0x7f));
		if (p[5] & 0x10) tag += 10;
		if (tag >= size || mp3_sync(p + tag, size - tag) || tag + 4 > size)
			s.kind = BF_AUDIO_KIND_MP3;   // a tag that runs past what we were given still means MP3
		else {
			// Some encoders pad after the tag; look a little further.
			for (size_t i = tag; i + 4 <= size && i < tag + 4096; ++i)
				if (mp3_sync(p + i, size - i)) { s.kind = BF_AUDIO_KIND_MP3; break; }
		}
		if (s.kind == BF_AUDIO_KIND_MP3) s.tag = 0x55;
		return s;
	}
	if (mp3_sync(p, size)) {
		s.kind = BF_AUDIO_KIND_MP3;
		s.tag = 0x55;
	}
	return s;
}

struct Mem { const uint8_t *p; size_t n; };

OSStatus mem_read(void *client, SInt64 pos, UInt32 want, void *buffer, UInt32 *got)
{
	Mem *m = (Mem *)client;
	if (pos < 0) { *got = 0; return kAudioFilePositionError; }
	if ((uint64_t)pos >= m->n) { *got = 0; return noErr; }
	size_t n = m->n - (size_t)pos;
	if (n > want) n = want;
	memcpy(buffer, m->p + pos, n);
	*got = (UInt32)n;
	return noErr;
}

SInt64 mem_size(void *client) { return (SInt64)((Mem *)client)->n; }

// An open AudioToolbox reader over memory or a file.
struct Reader {
	Mem mem = { nullptr, 0 };
	AudioFileID file = nullptr;
	ExtAudioFileRef ext = nullptr;
	AudioStreamBasicDescription src = {};
	int64_t frames = 0;

	~Reader()
	{
		if (ext) ExtAudioFileDispose(ext);
		if (file) AudioFileClose(file);
	}

	bool finish(OSStatus st, const char *what)
	{
		if (st != noErr) {
			BF_TRACE(BFT_SND, "audio: %s failed (OSStatus %d)\n", what, (int)st);
			return false;
		}
		UInt32 sz = sizeof src;
		st = ExtAudioFileGetProperty(ext, kExtAudioFileProperty_FileDataFormat, &sz, &src);
		if (st != noErr || src.mChannelsPerFrame == 0 || src.mSampleRate <= 0) {
			BF_TRACE(BFT_SND, "audio: no usable data format (OSStatus %d)\n", (int)st);
			return false;
		}
		SInt64 len = 0;
		sz = sizeof len;
		if (ExtAudioFileGetProperty(ext, kExtAudioFileProperty_FileLengthFrames, &sz, &len) == noErr)
			frames = len;
		AudioStreamBasicDescription c = {};
		c.mSampleRate = src.mSampleRate;
		c.mFormatID = kAudioFormatLinearPCM;
		c.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		c.mChannelsPerFrame = src.mChannelsPerFrame;
		c.mBitsPerChannel = 16;
		c.mBytesPerFrame = c.mBytesPerPacket = 2 * src.mChannelsPerFrame;
		c.mFramesPerPacket = 1;
		st = ExtAudioFileSetProperty(ext, kExtAudioFileProperty_ClientDataFormat, sizeof c, &c);
		if (st != noErr) {
			BF_TRACE(BFT_SND, "audio: no 16-bit client format (OSStatus %d)\n", (int)st);
			return false;
		}
		return true;
	}

	bool open_memory(const void *data, size_t size, int kind)
	{
		mem.p = (const uint8_t *)data;
		mem.n = size;
		AudioFileTypeID hint = kind == BF_AUDIO_KIND_MP3 ? kAudioFileMP3Type : kAudioFileWAVEType;
		OSStatus st = AudioFileOpenWithCallbacks(&mem, mem_read, nullptr, mem_size, nullptr, hint, &file);
		if (st != noErr) {
			file = nullptr;
			BF_TRACE(BFT_SND, "audio: AudioFileOpenWithCallbacks failed (OSStatus %d)\n", (int)st);
			return false;
		}
		return finish(ExtAudioFileWrapAudioFileID(file, false, &ext), "ExtAudioFileWrapAudioFileID");
	}

	bool open_path(const char *path)
	{
		@autoreleasepool {
			NSString *s = [NSString stringWithUTF8String:path];
			if (!s) return false;
			NSURL *url = [NSURL fileURLWithPath:s];
			return finish(ExtAudioFileOpenURL((__bridge CFURLRef)url, &ext), "ExtAudioFileOpenURL");
		}
	}

	// Reads up to max frames (all when max < 0) into dst, or into grow when
	// dst is null. Returns frames read or -1.
	int64_t read(int16_t *dst, int64_t max, std::vector<int16_t> *grow)
	{
		const int ch = (int)src.mChannelsPerFrame;
		int64_t done = 0;
		std::vector<int16_t> chunk;
		for (;;) {
			UInt32 want = 8192;
			if (max >= 0) {
				if (done >= max) break;
				if (max - done < want) want = (UInt32)(max - done);
			}
			int16_t *out;
			if (dst) out = dst + done * ch;
			else { chunk.resize((size_t)want * ch); out = chunk.data(); }
			AudioBufferList abl;
			abl.mNumberBuffers = 1;
			abl.mBuffers[0].mNumberChannels = (UInt32)ch;
			abl.mBuffers[0].mDataByteSize = want * 2 * ch;
			abl.mBuffers[0].mData = out;
			UInt32 n = want;
			OSStatus st = ExtAudioFileRead(ext, &n, &abl);
			if (st != noErr) {
				BF_TRACE(BFT_SND, "audio: ExtAudioFileRead failed (OSStatus %d)\n", (int)st);
				return done > 0 ? done : -1;   // keep what was decoded before a damaged tail
			}
			if (n == 0) break;
			if (grow) grow->insert(grow->end(), chunk.begin(), chunk.begin() + (size_t)n * ch);
			done += n;
		}
		return done;
	}
};

bool builtin_ok(const Sniff &s)
{
	return s.kind == BF_AUDIO_KIND_PCM_WAV && s.have_data && (s.bits == 8 || s.bits == 16)
		&& s.channels >= 1 && s.channels <= 8 && s.rate > 0 && s.align == s.channels * s.bits / 8;
}

int64_t builtin_decode(const void *data, const Sniff &s, int16_t *dst, int64_t max, std::vector<int16_t> *grow)
{
	const uint8_t *p = (const uint8_t *)data + s.data_at;
	int64_t frames = (int64_t)(s.data_len / s.align);
	if (max >= 0 && frames > max) frames = max;
	size_t n = (size_t)frames * s.channels;
	if (grow) { grow->resize(n); dst = grow->data(); }
	if (s.bits == 8)
		for (size_t i = 0; i < n; ++i) dst[i] = (int16_t)((p[i] - 128) * 256);
	else
		for (size_t i = 0; i < n; ++i) dst[i] = (int16_t)rd16(p + 2 * i);
	return frames;
}

void fill_info(bf_audio_info *info, const Sniff &s, const Reader *r)
{
	memset(info, 0, sizeof *info);
	info->kind = s.kind;
	info->format_tag = s.tag;
	if (r) {
		info->sample_rate = (int)(r->src.mSampleRate + 0.5);
		info->channels = (int)r->src.mChannelsPerFrame;
		info->bits = s.kind == BF_AUDIO_KIND_MP3 ? 0 : (s.bits ? s.bits : (int)r->src.mBitsPerChannel);
		info->frames = r->frames;
		info->frames_exact = s.kind != BF_AUDIO_KIND_MP3;
	} else {
		info->sample_rate = s.rate;
		info->channels = s.channels;
		info->bits = s.bits;
		info->frames = (int64_t)(s.data_len / s.align);
		info->frames_exact = 1;
		info->builtin = 1;
	}
}

int fail(const char *what)
{
	bf_audio_n_decode_failures++;
	bf_count("audio.decode_failures", 1);
	BF_TRACE(BFT_SND, "audio: cannot decode %s\n", what);
	return -1;
}

void ok(bool fallback)
{
	bf_audio_n_decodes++;
	if (fallback) {
		bf_audio_n_decode_fallbacks++;
		bf_count("audio.decode_fallbacks", 1);
	}
}

// The shared path for memory: AudioToolbox first, the built-in reader when
// AudioToolbox refuses a PCM WAV.
int64_t decode_memory(const void *data, size_t size, bf_audio_info *info, int16_t *dst, int64_t max, std::vector<int16_t> *grow)
{
	Sniff s = sniff(data, size);
	if (s.kind == BF_AUDIO_KIND_UNKNOWN)
		return fail("data of unknown kind");
	{
		Reader r;
		if (r.open_memory(data, size, s.kind)) {
			if (info) fill_info(info, s, &r);
			if (!dst && !grow) { ok(false); return r.frames; }
			int64_t n = r.read(dst, max, grow);
			if (n >= 0) {
				if (info && n > 0) info->frames = info->frames_exact ? info->frames : n;
				ok(false);
				return n;
			}
		}
	}
	if (builtin_ok(s)) {
		BF_TRACE(BFT_SND, "audio: AudioToolbox refused a PCM WAV; built-in reader used\n");
		if (info) fill_info(info, s, nullptr);
		int64_t n = (!dst && !grow) ? (int64_t)(s.data_len / s.align) : builtin_decode(data, s, dst, max, grow);
		ok(true);
		return n;
	}
	return fail(s.kind == BF_AUDIO_KIND_MP3 ? "MP3 data" : "WAV data");
}

Sniff sniff_file(const char *path)
{
	Sniff s;
	FILE *f = fopen(path, "rb");
	if (!f) return s;
	std::vector<uint8_t> head(65536);
	size_t n = fread(head.data(), 1, head.size(), f);
	fclose(f);
	return sniff(head.data(), n);
}

} // namespace

extern "C" {

int bf_audio_file_kind(const void *data, size_t size)
{
	return sniff(data, size).kind;
}

int bf_audio_query(const void *data, size_t size, bf_audio_info *info)
{
	bf_audio_info tmp;
	return decode_memory(data, size, info ? info : &tmp, nullptr, -1, nullptr) < 0 ? -1 : 0;
}

int64_t bf_audio_decode(const void *data, size_t size, int16_t *dst, int64_t max_frames)
{
	if (!dst || max_frames < 0)
		return fail("into no buffer");
	return decode_memory(data, size, nullptr, dst, max_frames, nullptr);
}

int16_t *bf_audio_decode_alloc(const void *data, size_t size, bf_audio_info *info, int64_t *frames)
{
	bf_audio_info tmp;
	if (!info) info = &tmp;
	std::vector<int16_t> pcm;
	int64_t n = decode_memory(data, size, info, nullptr, -1, &pcm);
	if (n < 0) return nullptr;
	info->frames = n;
	if (frames) *frames = n;
	int16_t *out = (int16_t *)malloc(pcm.size() * 2 + 2);   // +2: never malloc(0)
	if (!out) return nullptr;
	memcpy(out, pcm.data(), pcm.size() * 2);
	return out;
}

void bf_audio_free(void *buffer)
{
	free(buffer);
}

int bf_audio_query_file(const char *path, bf_audio_info *info)
{
	if (!path) return fail("a null path");
	Reader r;
	if (!r.open_path(path)) return fail(path);
	if (info) {
		Sniff s = sniff_file(path);
		if (s.kind == BF_AUDIO_KIND_UNKNOWN)
			s.kind = r.src.mFormatID == kAudioFormatMPEGLayer3 ? BF_AUDIO_KIND_MP3 : BF_AUDIO_KIND_UNKNOWN;
		fill_info(info, s, &r);
	}
	ok(false);
	return 0;
}

int64_t bf_audio_decode_file(const char *path, int16_t *dst, int64_t max_frames)
{
	if (!path || !dst || max_frames < 0) return fail("a file into no buffer");
	Reader r;
	if (!r.open_path(path)) return fail(path);
	int64_t n = r.read(dst, max_frames, nullptr);
	if (n < 0) return fail(path);
	ok(false);
	return n;
}

} // extern "C"
