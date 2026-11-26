/*
 * bf_audio: sound for the macOS port. AVAudioEngine voices for sound
 * effects, one looping music stream read from a file, and WAV and MP3
 * decoding through AudioToolbox.
 *
 * This header is the contract the client's sound code is written against:
 * src/device/Sound.cpp and src/resource/Wave.cpp call it directly since
 * M11.6 (before, the Miles adapter of port/compat did; MILES-USAGE.md says
 * which AIL_* call became which function here). It is a C header usable from
 * C and C++14.
 *
 * Threads. Decoding functions keep no state and may be called from any
 * thread at the same time. Every other function may be called from any
 * thread too: the client's music fade runs on its own thread and calls the
 * stream volume and pause functions, so the module serialises all engine,
 * voice and stream calls with one internal lock. No callback ever reaches
 * the caller.
 *
 * Failure is a return value (0 or -1 as documented) plus a trace line in the
 * BFT_SND category of port/instrument/bf_trace.h, and a counter. Nothing
 * aborts and nothing shows an alert. With no output device the module runs
 * silent: calls succeed, voices are counted but finish at once, the stream
 * keeps its file name and state but makes no sound.
 */
#ifndef BF_AUDIO_H
#define BF_AUDIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------
 * Decoding
 *
 * Decoded audio is always signed 16-bit interleaved PCM at the source's own
 * sample rate and channel count, which is what Miles handed the client. An
 * 8-bit (unsigned) source sample v becomes (v - 128) * 256.
 * --------------------------------------------------------------------- */

/* What a block of bytes holds. The values equal Miles' AILFILETYPE_* on
 * purpose, so AIL_file_type can return them unchanged. */
enum {
	BF_AUDIO_KIND_UNKNOWN   = 0,
	BF_AUDIO_KIND_PCM_WAV   = 1,   /* RIFF WAVE, format tag 1 (or extensible PCM) */
	BF_AUDIO_KIND_ADPCM_WAV = 2,   /* RIFF WAVE, format tag 2 (MS) or 0x11 (IMA) */
	BF_AUDIO_KIND_OTHER_WAV = 3,   /* RIFF WAVE with any other format tag */
	BF_AUDIO_KIND_MP3       = 13   /* MPEG-1/2 layer III, with or without an ID3 tag */
};

typedef struct bf_audio_info {
	int     kind;          /* BF_AUDIO_KIND_* */
	int     format_tag;    /* the WAVE format tag; 0x55 for a bare MP3 */
	int     sample_rate;   /* Hz */
	int     channels;      /* 1 or 2 (more are decoded too, but voices take 1 or 2) */
	int     bits;          /* bits per source sample; 0 for MP3 */
	int64_t frames;        /* decoded length in frames */
	int     frames_exact;  /* 1 for WAV; 0 for MP3, whose length is the
	                          decoder's estimate: bf_audio_decode may return
	                          fewer or more (never more than max_frames) */
	int     builtin;       /* 1 when AudioToolbox refused the data and the
	                          module's own PCM WAV reader decoded it (a broken
	                          RIFF header; counted as a decode fallback) */
} bf_audio_info;

/* Looks at the first bytes only. Never fails; UNKNOWN when unsure. */
int bf_audio_file_kind(const void *data, size_t size);

/* Fills *info for a WAV or MP3 held in memory. 0, or -1 when the data cannot
 * be decoded (counted as a decode failure). */
int bf_audio_query(const void *data, size_t size, bf_audio_info *info);

/* Decodes into dst, which holds max_frames * channels samples. Returns the
 * number of frames written, or -1 (counted as a decode failure). */
int64_t bf_audio_decode(const void *data, size_t size, int16_t *dst, int64_t max_frames);

/* Query and decode in one: returns a buffer of *frames * info->channels
 * samples, to be released with bf_audio_free, or NULL on failure. Use this
 * for MP3, whose exact length is only known after decoding. */
int16_t *bf_audio_decode_alloc(const void *data, size_t size, bf_audio_info *info, int64_t *frames);
void bf_audio_free(void *buffer);

/* The same for a file (UTF-8 path). */
int bf_audio_query_file(const char *path, bf_audio_info *info);
int64_t bf_audio_decode_file(const char *path, int16_t *dst, int64_t max_frames);

/* ------------------------------------------------------------------------
 * Engine
 * --------------------------------------------------------------------- */

enum {
	BF_AUDIO_MODE_DEVICE  = 0,   /* the default output device, in real time */
	BF_AUDIO_MODE_OFFLINE = 1,   /* no device: the caller pulls samples with bf_audio_render */
	BF_AUDIO_MODE_SILENT  = 2    /* no device and no rendering: everything is counted only */
};

#define BF_AUDIO_MAX_VOICES      64
#define BF_AUDIO_DEFAULT_VOICES  48   /* the client's MAXSAMPLE */

typedef struct bf_audio_config {
	int mode;           /* BF_AUDIO_MODE_*; DEVICE falls back to SILENT when
	                       there is no output device or the engine will not start */
	int max_voices;     /* 1..BF_AUDIO_MAX_VOICES; 0 means BF_AUDIO_DEFAULT_VOICES */
	int offline_rate;   /* OFFLINE only: the rendering rate; 0 means 44100 */
} bf_audio_config;

/* Starts the engine. config may be NULL (DEVICE, 48 voices). Returns the mode
 * that is now running (a DEVICE request may return SILENT, counted as a
 * device failure), or -1 when already started or for an invalid config. */
int bf_audio_start(const bf_audio_config *config);

/* Stops every voice and the stream and releases the engine. Sounds stay
 * valid. Safe to call when not started. */
void bf_audio_stop(void);

/* The running mode, or -1 when not started. */
int bf_audio_mode(void);

/* The rate the engine mixes at (the device's, or offline_rate); 0 when not
 * started or silent. */
int bf_audio_sample_rate(void);

/* Master volumes, 0..1, applied on top of every voice's or the stream's own
 * volume. Kept across bf_audio_stop/start. */
void  bf_audio_set_effects_volume(float volume);
float bf_audio_effects_volume(void);
void  bf_audio_set_music_volume(float volume);
float bf_audio_music_volume(void);

/* Mute: in DEVICE mode the output is silenced (the main mixer's output
 * volume 0) while everything else runs as it would: voices, the stream,
 * positions and counters are unchanged. May be called before bf_audio_start;
 * kept across bf_audio_stop/start. For instrumented test runs (--mute). */
void bf_audio_set_muted(int muted);
int  bf_audio_muted(void);

/* OFFLINE only: renders the next frames of the mix into dst as interleaved
 * stereo float (2 * frames values). Returns frames rendered, or -1 when the
 * engine is not in OFFLINE mode or rendering failed. Voice completion is
 * noticed during rendering, so a non-looping voice is no longer playing once
 * its length has been rendered. */
int64_t bf_audio_render(float *dst, int64_t frames);

/* ------------------------------------------------------------------------
 * Sounds and voices
 * --------------------------------------------------------------------- */

/* A decoded sample ready to play. It is kept as float at its own rate and
 * converted to the engine's rate the first time it plays (the converted copy
 * is kept with the sound). May be created before bf_audio_start. */
typedef struct bf_audio_sound bf_audio_sound;

/* From interleaved 16-bit PCM; channels 1 or 2, sample_rate 1000..192000.
 * The samples are copied. NULL on invalid arguments. */
bf_audio_sound *bf_audio_sound_create(const int16_t *samples, int64_t frames, int channels, int sample_rate);

/* Decodes a WAV or MP3 held in memory (bf_audio_decode_alloc) and creates a
 * sound from it. NULL on failure (counted as a decode failure). */
bf_audio_sound *bf_audio_sound_create_from_memory(const void *data, size_t size);

/* Releases the sound. Voices still playing it play to their end. NULL is
 * ignored. */
void bf_audio_sound_destroy(bf_audio_sound *sound);

double bf_audio_sound_duration(const bf_audio_sound *sound);   /* seconds */
int64_t bf_audio_sound_frames(const bf_audio_sound *sound);

/* A playing voice. 0 is never a valid voice. A handle goes stale when its
 * voice ends or is reused; calls on a stale handle do nothing and
 * bf_audio_voice_playing returns 0. */
typedef uint32_t bf_audio_voice;

/* Plays sound with volume 0..1 (scaled by the effects master volume) and pan
 * -1 (left) .. 0 (centre) .. 1 (right), once or looping until stopped.
 *
 * Voice cap and stealing rule: at most max_voices voices sound at once. When
 * all are busy, the oldest non-looping voice (the one started first) is
 * stopped and reused, counted as stolen. Looping voices are never stolen; when
 * every voice is looping the request is refused: 0 is returned and counted as
 * rejected. In SILENT mode the voice is counted as started and finishes at
 * once. Returns 0 also when the engine is not started or sound is NULL. */
bf_audio_voice bf_audio_play(bf_audio_sound *sound, float volume, float pan, int loop);

void bf_audio_voice_stop(bf_audio_voice voice);
int  bf_audio_voice_playing(bf_audio_voice voice);
void bf_audio_voice_set_volume(bf_audio_voice voice, float volume);
void bf_audio_voice_set_pan(bf_audio_voice voice, float pan);

/* Stops every voice (not the stream). */
void bf_audio_stop_all_voices(void);

/* Voices that are sounding now. */
int bf_audio_voices_playing(void);

/* ------------------------------------------------------------------------
 * The music stream: one file at a time, decoded while it plays
 * --------------------------------------------------------------------- */

enum {
	BF_AUDIO_STREAM_NONE    = 0,   /* no file open */
	BF_AUDIO_STREAM_OPEN    = 1,   /* opened, not started */
	BF_AUDIO_STREAM_PLAYING = 2,
	BF_AUDIO_STREAM_PAUSED  = 3,
	BF_AUDIO_STREAM_DONE    = 4    /* played its loop count to the end */
};

/* Opens a file (UTF-8 path; the caller resolves BGM\xx.mp3 under the game
 * folder) and closes any stream before it. Any format AudioToolbox reads.
 * The loop count is reset to 1 and the stream's volume is kept. 0, or -1
 * when the file cannot be opened (counted as a stream failure; the stream
 * is then NONE). Works in SILENT mode too. */
int bf_audio_stream_open(const char *path);

/* How many times the file plays: 0 loops forever (Miles' convention), n plays
 * it n times. Applies from the next bf_audio_stream_start. */
void bf_audio_stream_set_loop_count(int count);

/* Starts (or restarts from the beginning) the open stream. 0, or -1 when
 * none is open. */
int bf_audio_stream_start(void);

/* paused 1 pauses a playing stream, 0 resumes a paused one; otherwise
 * nothing happens. */
void bf_audio_stream_pause(int paused);

/* Stops the stream and forgets its file. */
void bf_audio_stream_close(void);

/* The stream's own volume, 0..1, scaled by the music master volume. Kept
 * across stream_open. */
void  bf_audio_stream_set_volume(float volume);
float bf_audio_stream_volume(void);

int    bf_audio_stream_status(void);     /* BF_AUDIO_STREAM_* */
double bf_audio_stream_duration(void);   /* seconds of one pass; 0 without a file */

/* ------------------------------------------------------------------------
 * Counters, for tests
 *
 * Kept since the process started (or since bf_audio_reset_counters); they
 * survive bf_audio_stop. The main ones are also added to the bf_count
 * counters of port/instrument under "audio.*".
 * --------------------------------------------------------------------- */

typedef struct bf_audio_counters {
	int       mode;              /* bf_audio_mode() */
	int       max_voices;
	int       voices_playing;    /* bf_audio_voices_playing() */
	long long voices_started;
	long long voices_stolen;
	long long voices_rejected;   /* every voice looping, engine not started, or no sound */
	long long conversions;       /* sounds converted to the engine rate (once per sound and rate) */
	long long decodes;           /* successful decodes (query and decode count once each) */
	long long decode_failures;
	long long decode_fallbacks;  /* decoded by the built-in PCM reader */
	long long device_failures;   /* DEVICE requested, SILENT running */
	long long engine_restarts;   /* after an output device change */
	long long streams_opened;
	long long stream_failures;
	int       stream_status;     /* bf_audio_stream_status() */
	int       stream_loops;      /* passes of the current file played to their end */
	double    stream_duration;   /* bf_audio_stream_duration() */
	char      stream_file[1024]; /* the current stream's path as given; "" when none */
} bf_audio_counters;

void bf_audio_get_counters(bf_audio_counters *out);
void bf_audio_reset_counters(void);

/* Seconds into the current pass of the stream (0 .. duration): how far the
 * output has played it, so it falls back towards 0 when a loop begins again.
 * 0 when no file is open, before the start, and in SILENT mode, where
 * nothing is played. */
double bf_audio_stream_position(void);

/* ------------------------------------------------------------------------
 * Recent voices, for tests
 *
 * The module keeps a record of the last BF_AUDIO_RECENT_VOICES calls of
 * bf_audio_play (started or refused) with what the caller passed. A caller
 * that knows more than the module (a file name, a distance) tags the next
 * call on its own thread with bf_audio_tag_next_voice; the tag is used by
 * that one call and then forgotten. Untagged records have label "" and
 * distance -1. bf_audio_reset_counters does not clear the ring; serials keep
 * counting.
 * --------------------------------------------------------------------- */

#define BF_AUDIO_RECENT_VOICES 64

typedef struct bf_audio_voice_record {
	long long      serial;     /* 1 for the first call since the process started, then +1 per call */
	bf_audio_voice voice;      /* the handle returned; 0 when refused */
	int            started;    /* 1 when a voice was started (in SILENT mode too) */
	float          volume;     /* as passed, clamped to 0..1 (before the effects master volume) */
	float          pan;        /* as passed, clamped to -1..1 */
	int            loop;
	double         duration;   /* of the sound, seconds; 0 without one */
	float          distance;   /* from the tag; -1 when untagged */
	char           label[256]; /* from the tag (bytes as given, cut at 255); "" when untagged */
} bf_audio_voice_record;

/* Tags the next bf_audio_play on the calling thread. label NULL clears a tag
 * that was not used. The label is copied. */
void bf_audio_tag_next_voice(const char *label, float distance);

/* Copies the records with a serial above after_serial, oldest first, at most
 * max of them (the newest max when there are more). Returns the number
 * copied. after_serial 0 asks for all that are kept. */
int bf_audio_recent_voices(long long after_serial, bf_audio_voice_record *out, int max);

#ifdef __cplusplus
}
#endif

#endif
