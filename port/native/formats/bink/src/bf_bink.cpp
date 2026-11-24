// Bink container: header, frame index, per-frame audio and video records,
// and the public functions of bf_bink.h.
#include <stdio.h>

#include <algorithm>
#include <memory>

#include "bf_bink_internal.h"
#include "bf_trace.h"

using namespace bf_bink_impl;

namespace {

const uint32_t kFlagAlpha = 0x00100000;
const uint16_t kAudioStereo = 0x2000;
const uint16_t kAudioDct = 0x1000;
const size_t kPad = 16;   // zero bytes after every record handed to a bit reader

uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

} // namespace

struct bf_bink {
	std::vector<uint8_t> file;
	bf_bink_info info{};
	std::vector<uint32_t> offsets;   // frames + 1 entries, key flag removed
	std::vector<uint8_t> key;
	Video video;
	std::vector<std::unique_ptr<Audio>> audio;
	std::vector<uint8_t> record;     // the current record, padded
	int shown = -1;                  // frame in the planes
	uint32_t next = 0;               // frame decode_next reads
	bool video_enabled = true;       // bf_bink_set_video_enabled
	bf_bink_stats stats{};
};

namespace {

bool parse(bf_bink *b)
{
	const std::vector<uint8_t> &f = b->file;
	if (f.size() < 44) {
		BF_TRACE(BFT_FILE, "bink: file too short (%zu bytes)\n", f.size());
		return false;
	}
	const uint8_t *h = f.data();
	if (h[0] == 'K' && h[1] == 'B' && h[2] == '2') {
		BF_TRACE(BFT_FILE, "bink: Bink 2 (KB2%c) is not supported\n", h[3]);
		return false;
	}
	if (h[0] != 'B' || h[1] != 'I' || h[2] != 'K') {
		BF_TRACE(BFT_FILE, "bink: not a Bink file\n");
		return false;
	}
	bf_bink_info &in = b->info;
	in.revision = h[3];
	if (in.revision < 'c' || in.revision > 'k' || in.revision == 'j') {
		BF_TRACE(BFT_FILE, "bink: revision '%c' is not supported\n", h[3]);
		return false;
	}
	uint32_t file_size = rd32(h + 4) + 8;
	in.frames = rd32(h + 8);
	in.largest_frame = rd32(h + 12);
	in.width = rd32(h + 20);
	in.height = rd32(h + 24);
	in.rate_num = rd32(h + 28);
	in.rate_den = rd32(h + 32);
	in.video_flags = rd32(h + 36);
	uint32_t tracks = rd32(h + 40);
	in.has_alpha = (in.video_flags & kFlagAlpha) ? 1 : 0;
	if (in.frames == 0 || in.frames > 1000000 || in.width == 0 || in.width > 7680 ||
	    in.height == 0 || in.height > 4800 || in.rate_num == 0 || in.rate_den == 0 || tracks > 8) {
		BF_TRACE(BFT_FILE, "bink: implausible header (%ux%u, %u frames, %u tracks)\n",
		         in.width, in.height, in.frames, tracks);
		return false;
	}
	if (file_size > f.size())
		BF_TRACE(BFT_FILE, "bink: file is %zu bytes, header says %u\n", f.size(), file_size);
	in.audio_tracks = (int)tracks;
	size_t at = 44;
	if (in.revision == 'k')
		at += 4;
	size_t need = at + tracks * 12 + ((size_t)in.frames + 1) * 4;
	if (need > f.size()) {
		BF_TRACE(BFT_FILE, "bink: header runs past the end\n");
		return false;
	}
	std::vector<uint32_t> max_size(tracks);
	for (uint32_t t = 0; t < tracks; ++t, at += 4)
		max_size[t] = rd32(h + at);
	for (uint32_t t = 0; t < tracks; ++t, at += 4) {
		uint16_t rate = rd16(h + at);
		uint16_t flags = rd16(h + at + 2);
		std::unique_ptr<Audio> a(new Audio);
		int channels = (flags & kAudioStereo) ? 2 : 1;
		bool ok = a->init(in.revision, rate, channels, (flags & kAudioDct) != 0);
		a->enabled = ok;
		a->info.sample_rate = rate;
		a->info.channels = channels;
		a->info.uses_dct = (flags & kAudioDct) ? 1 : 0;
		a->info.max_packet_samples = max_size[t] / (2 * channels);
		b->audio.push_back(std::move(a));
	}
	for (uint32_t t = 0; t < tracks; ++t, at += 4)
		b->audio[t]->info.id = rd32(h + at);

	b->offsets.resize(in.frames + 1);
	b->key.resize(in.frames);
	for (uint32_t i = 0; i <= in.frames; ++i, at += 4) {
		uint32_t v = rd32(h + at);
		if (i < in.frames)
			b->key[i] = v & 1;
		b->offsets[i] = v & ~1u;
	}
	// The last entry is the end of the file. A file cut short keeps the
	// frames that are complete.
	for (uint32_t i = 0; i < in.frames; ++i) {
		if (b->offsets[i] < at || b->offsets[i + 1] < b->offsets[i]) {
			BF_TRACE(BFT_FILE, "bink: bad frame index at frame %u\n", i);
			return false;
		}
		if (b->offsets[i + 1] > f.size()) {
			if (i == 0) {
				BF_TRACE(BFT_FILE, "bink: the first frame is past the end of the file\n");
				return false;
			}
			BF_TRACE(BFT_FILE, "bink: file cut short: %u of %u frames\n", i, in.frames);
			in.frames = i;
			b->offsets.resize(i + 1);
			b->key.resize(i);
			break;
		}
	}
	b->video.init(in.revision, in.width, in.height, in.has_alpha != 0);
	b->record.resize(std::max<size_t>(in.largest_frame, 64) + kPad);
	return true;
}

// Decodes frame b->next; returns 1, 0 at the end, -1 if damaged.
int decode_frame(bf_bink *b, bool want_audio)
{
	if (b->next >= b->info.frames)
		return 0;
	uint32_t i = b->next++;
	const uint8_t *p = b->file.data() + b->offsets[i];
	size_t size = b->offsets[i + 1] - b->offsets[i];
	int result = 1;

	for (size_t t = 0; t < b->audio.size(); ++t) {
		if (size < 4) {
			BF_TRACE(BFT_GFX, "bink: frame %u: record ends in the audio\n", i);
			return -1;
		}
		uint32_t len = rd32(p);
		if (len > size - 4) {
			BF_TRACE(BFT_SND, "bink: frame %u: audio packet longer than the frame\n", i);
			b->stats.audio_errors++;
			return -1;
		}
		Audio &a = *b->audio[t];
		if (want_audio && a.enabled && len >= 4) {
			if (b->record.size() < len + kPad)
				b->record.resize(len + kPad);
			memcpy(b->record.data(), p + 4, len);
			memset(b->record.data() + len, 0, kPad);
			uint64_t before = a.produced();
			if (!a.decode_packet(b->record.data(), len, len + kPad))
				b->stats.audio_errors++;
			b->stats.audio_packets++;
			b->stats.audio_frames += a.produced() - before;
		}
		p += 4 + len;
		size -= 4 + len;
	}

	if (!b->video_enabled) {
		// Sound only: the planes keep the last picture decoded.
		b->shown = (int)i;
		b->stats.frames_decoded++;
		return result;
	}
	if (b->record.size() < size + kPad)
		b->record.resize(size + kPad);
	memcpy(b->record.data(), p, size);
	memset(b->record.data() + size, 0, kPad);
	if (!b->video.decode(b->record.data(), size, size + kPad)) {
		BF_TRACE(BFT_GFX, "bink: frame %u: video damaged\n", i);
		b->stats.video_errors++;
		result = -1;
	}
	b->shown = (int)i;
	b->stats.frames_decoded++;
	return result;
}

inline uint8_t clamp8(int v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); }

} // namespace

extern "C" {

bf_bink *bf_bink_open_memory(const void *data, size_t size)
{
	if (!data || size == 0)
		return nullptr;
	std::unique_ptr<bf_bink> b(new bf_bink);
	b->file.assign((const uint8_t *)data, (const uint8_t *)data + size);
	if (!parse(b.get()))
		return nullptr;
	return b.release();
}

bf_bink *bf_bink_open_file(const char *path)
{
	if (!path)
		return nullptr;
	FILE *fp = fopen(path, "rb");
	if (!fp) {
		BF_TRACE(BFT_FILE, "bink: cannot open %s\n", path);
		return nullptr;
	}
	std::unique_ptr<bf_bink> b(new bf_bink);
	fseek(fp, 0, SEEK_END);
	long n = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (n > 0) {
		b->file.resize((size_t)n);
		if (fread(b->file.data(), 1, (size_t)n, fp) != (size_t)n)
			b->file.clear();
	}
	fclose(fp);
	if (!parse(b.get())) {
		BF_TRACE(BFT_FILE, "bink: %s not opened\n", path);
		return nullptr;
	}
	return b.release();
}

void bf_bink_close(bf_bink *bink) { delete bink; }

void bf_bink_get_info(const bf_bink *bink, bf_bink_info *info)
{
	if (bink && info)
		*info = bink->info;
}

int bf_bink_get_track(const bf_bink *bink, int track, bf_bink_track_info *info)
{
	if (!bink || !info || track < 0 || track >= (int)bink->audio.size())
		return -1;
	*info = bink->audio[track]->info;
	return 0;
}

int bf_bink_decode_next(bf_bink *bink)
{
	return bink ? decode_frame(bink, true) : -1;
}

int bf_bink_frame(const bf_bink *bink) { return bink ? bink->shown : -1; }

int bf_bink_is_keyframe(const bf_bink *bink, uint32_t frame)
{
	return bink && frame < bink->info.frames ? bink->key[frame] : 0;
}

int bf_bink_seek(bf_bink *bink, uint32_t frame)
{
	if (!bink)
		return -1;
	frame = std::min(frame, bink->info.frames - 1);
	uint32_t start = frame;
	while (start > 0 && !bink->key[start])
		--start;
	// Continuing from the frame shown is as good as starting at the key frame.
	if (bink->shown >= 0 && (uint32_t)bink->shown >= start && (uint32_t)bink->shown < frame)
		bink->next = (uint32_t)bink->shown + 1;
	else
		bink->next = start;
	int result = 0;
	while (bink->next < frame)
		if (decode_frame(bink, false) < 0)
			result = -1;
	for (auto &a : bink->audio)
		a->reset();
	return result;
}

const uint8_t *bf_bink_plane(const bf_bink *bink, int plane, int *pitch, int *width, int *height)
{
	if (!bink || plane < 0 || plane > 3 || (plane == 3 && !bink->info.has_alpha))
		return nullptr;
	const Plane &p = bink->video.plane(plane);
	if (pitch)
		*pitch = p.stride;
	if (width)
		*width = p.width;
	if (height)
		*height = p.height;
	return p.cur.data();
}

int bf_bink_copy_pixels(const bf_bink *bink, void *dest, int pitch, int dest_width,
                        int dest_height, int x, int y, int format)
{
	if (!bink || !dest || bink->shown < 0 || format < 0 || format > 2)
		return -1;
	const Plane &py = bink->video.plane(0);
	const Plane &pu = bink->video.plane(1);
	const Plane &pv = bink->video.plane(2);
	const Plane *pa = bink->info.has_alpha ? &bink->video.plane(3) : nullptr;
	const bool full = bink->info.revision == 'k';
	const int w = (int)bink->info.width, h = (int)bink->info.height;
	for (int r = 0; r < h; ++r) {
		int dy = y + r;
		if (dy < 0 || dy >= dest_height)
			continue;
		const uint8_t *ly = py.cur.data() + (size_t)r * py.stride;
		const uint8_t *lu = pu.cur.data() + (size_t)(r >> 1) * pu.stride;
		const uint8_t *lv = pv.cur.data() + (size_t)(r >> 1) * pv.stride;
		const uint8_t *la = pa ? pa->cur.data() + (size_t)r * pa->stride : nullptr;
		uint8_t *row = (uint8_t *)dest + (ptrdiff_t)dy * pitch;
		for (int c = 0; c < w; ++c) {
			int dx = x + c;
			if (dx < 0 || dx >= dest_width)
				continue;
			// BT.601, 16.16 fixed point.
			int Y = ly[c], U = lu[c >> 1] - 128, V = lv[c >> 1] - 128;
			int yy, R, G, B;
			if (full) {
				yy = Y << 16;
				R = yy + 91881 * V;
				G = yy - 22554 * U - 46802 * V;
				B = yy + 116130 * U;
			} else {
				yy = (Y - 16) * 76309;
				R = yy + 104597 * V;
				G = yy - 25675 * U - 53279 * V;
				B = yy + 132201 * U;
			}
			uint8_t r8 = clamp8((R + 32768) >> 16), g8 = clamp8((G + 32768) >> 16), b8 = clamp8((B + 32768) >> 16);
			if (format == BF_BINK_BGRA8888) {
				uint8_t *px = row + dx * 4;
				px[0] = b8;
				px[1] = g8;
				px[2] = r8;
				px[3] = la ? la[c] : 255;
			} else if (format == BF_BINK_RGB565) {
				uint16_t v = (uint16_t)(((r8 >> 3) << 11) | ((g8 >> 2) << 5) | (b8 >> 3));
				memcpy(row + dx * 2, &v, 2);
			} else {
				uint16_t v = (uint16_t)(0x8000 | ((r8 >> 3) << 10) | ((g8 >> 3) << 5) | (b8 >> 3));
				memcpy(row + dx * 2, &v, 2);
			}
		}
	}
	return 0;
}

size_t bf_bink_audio_available(const bf_bink *bink, int track)
{
	if (!bink || track < 0 || track >= (int)bink->audio.size())
		return 0;
	return bink->audio[track]->available();
}

size_t bf_bink_audio_read(bf_bink *bink, int track, int16_t *dest, size_t max_frames)
{
	if (!bink || !dest || track < 0 || track >= (int)bink->audio.size())
		return 0;
	return bink->audio[track]->read(dest, max_frames);
}

void bf_bink_set_audio_enabled(bf_bink *bink, int track, int enabled)
{
	if (bink && track >= 0 && track < (int)bink->audio.size())
		bink->audio[track]->enabled = enabled != 0 && bink->audio[track]->ready;
}

void bf_bink_set_video_enabled(bf_bink *bink, int enabled)
{
	if (bink)
		bink->video_enabled = enabled != 0;
}

void bf_bink_get_stats(const bf_bink *bink, bf_bink_stats *stats)
{
	if (bink && stats)
		*stats = bink->stats;
}

} // extern "C"
