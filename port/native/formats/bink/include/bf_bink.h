/*
 * bf_bink: a decoder for Bink 1 movies (RAD Game Tools' "BIK" files), the
 * format of the client's intro movie (openning.bik, played by
 * src/movie/CBink.cpp). Written from public descriptions of the format and
 * the game's own files; see ../FORMAT.md for what is established.
 *
 * Covers container revisions 'c' to 'i' and 'k' of Bink 1, video (YUV 4:2:0,
 * optional alpha plane) and both audio codecs (RDFT and DCT). Bink 2 ("KB2")
 * and the oldest revision 'b' are refused at open.
 *
 * A movie is decoded in order, one frame per call. Each decoded frame leaves
 * its picture in the handle's planes (read them, or convert with
 * bf_bink_copy_pixels) and appends the frame's sound to one queue per audio
 * track, read with bf_bink_audio_read. Timing is the caller's: the module
 * reads no clock; the frame rate is in the info.
 *
 * Threads: a handle is used by one thread at a time; different handles are
 * independent. Nothing here must run on the main thread.
 *
 * Failure is a return value plus a trace line (port/instrument/bf_trace.h,
 * categories GFX for video, SND for audio, FILE for opening).
 */
#ifndef BF_BINK_H
#define BF_BINK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_bink bf_bink;

typedef struct bf_bink_info {
	uint32_t revision;       /* the fourth signature byte, e.g. 'i' */
	uint32_t width;          /* picture size in pixels */
	uint32_t height;
	uint32_t frames;         /* number of frames */
	uint32_t rate_num;       /* frames per second = rate_num / rate_den */
	uint32_t rate_den;
	uint32_t video_flags;    /* header flags; bit 20 alpha, bit 17 grey */
	uint32_t largest_frame;  /* largest frame record in bytes */
	int      has_alpha;      /* 1 if plane 3 (alpha) is coded */
	int      audio_tracks;   /* 0 to 8 */
} bf_bink_info;

typedef struct bf_bink_track_info {
	uint32_t id;             /* track id from the header (BinkSetSoundTrack) */
	uint32_t sample_rate;    /* Hz */
	int      channels;       /* 1 or 2 */
	int      uses_dct;       /* 1: DCT codec, 0: RDFT codec */
	uint32_t max_packet_samples; /* header's largest decoded packet, in sample frames */
} bf_bink_track_info;

typedef struct bf_bink_stats {
	uint32_t frames_decoded;   /* frames passed through bf_bink_decode_next */
	uint32_t video_errors;     /* frames whose video bitstream was damaged */
	uint32_t audio_packets;    /* audio packets decoded, all tracks */
	uint32_t audio_errors;     /* audio packets that ended in a bitstream error */
	uint64_t audio_frames;     /* sample frames produced, all tracks */
} bf_bink_stats;

/* Pixel formats for bf_bink_copy_pixels, as bytes in memory (little endian). */
enum {
	BF_BINK_BGRA8888 = 0,    /* B, G, R, A=255 (or the alpha plane) */
	BF_BINK_RGB565   = 1,    /* 16-bit 5:6:5 */
	BF_BINK_XRGB1555 = 2     /* 16-bit 1:5:5:5, top bit set */
};

/*
 * Opens a movie. The file is read completely into memory. Returns NULL on a
 * missing file or a header the decoder does not accept.
 */
bf_bink *bf_bink_open_file(const char *path);

/* Opens a movie held in memory; the bytes are copied. */
bf_bink *bf_bink_open_memory(const void *data, size_t size);

void bf_bink_close(bf_bink *bink);

void bf_bink_get_info(const bf_bink *bink, bf_bink_info *info);

/* Returns 0 and fills *track for track 0..audio_tracks-1, else -1. */
int bf_bink_get_track(const bf_bink *bink, int track, bf_bink_track_info *info);

/*
 * Decodes the next frame: its picture into the planes and its audio into the
 * track queues. Returns 1 when a frame was decoded, 0 at the end of the
 * movie, -1 when the frame's video was damaged (the planes then hold what
 * could be decoded, the rest copied from the previous frame; decoding may
 * continue with the next frame).
 */
int bf_bink_decode_next(bf_bink *bink);

/* The 0-based number of the frame in the planes, -1 before the first. */
int bf_bink_frame(const bf_bink *bink);

/* 1 if the frame (0-based) is a key frame. */
int bf_bink_is_keyframe(const bf_bink *bink, uint32_t frame);

/*
 * Positions the movie so that the next bf_bink_decode_next returns `frame`
 * (0-based; clamped to the last frame). Decodes from the key frame at or
 * before it, discarding that sound; clears the audio queues. Returns 0, or -1
 * if a frame on the way was damaged.
 */
int bf_bink_seek(bf_bink *bink, uint32_t frame);

/*
 * The decoded planes. plane 0 is Y (width x height), 1 is Cb (U) and 2 is
 * Cr (V) (each (width+1)/2 x (height+1)/2), 3 is alpha (as Y; NULL without
 * alpha). Values are 8-bit; Y and chroma are video range (16..235/240) for
 * revisions before 'k', full range for 'k'. Pointers stay valid until the
 * next decode, seek or close.
 */
const uint8_t *bf_bink_plane(const bf_bink *bink, int plane, int *pitch, int *width, int *height);

/*
 * Converts the current picture (BT.601) into dest at (x, y), clipping to
 * dest_width x dest_height. pitch is in bytes. Returns 0, or -1 on bad
 * arguments or before the first frame.
 */
int bf_bink_copy_pixels(const bf_bink *bink, void *dest, int pitch, int dest_width,
                        int dest_height, int x, int y, int format);

/* Sample frames waiting in the track's queue. */
size_t bf_bink_audio_available(const bf_bink *bink, int track);

/*
 * Moves up to max_frames sample frames of signed 16-bit PCM, interleaved by
 * channel, out of the track's queue. Returns the number moved.
 */
size_t bf_bink_audio_read(bf_bink *bink, int track, int16_t *dest, size_t max_frames);

/* Turns decoding of a track's sound on (default) or off. */
void bf_bink_set_audio_enabled(bf_bink *bink, int track, int enabled);

void bf_bink_get_stats(const bf_bink *bink, bf_bink_stats *stats);

#ifdef __cplusplus
}
#endif

#endif
