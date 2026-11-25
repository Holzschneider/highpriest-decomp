/*
 * bf_movie.h: the intro movie's player, what src/movie/CBink.cpp calls
 * (M11.10, in place of the Bink SDK names of the deleted adapter
 * port/compat/bink). Port code in port/client/src/bf_movie.cpp, on the Bink
 * reader (port/native/formats/bink), the sound module (port/native/audio)
 * and the sys clock and file lookup (port/native/sys).
 *
 * Timing: frame n (1-based) is due n-1 frame periods after frame 1 was
 * decoded; bf_movie_wait compares that with bf_clock_us. Frames are never
 * skipped: a client that falls behind shows every frame late.
 *
 * Sound: when the sound was asked for (bf_movie_use_sound), the movie's first
 * track is decoded completely at open, cut to the picture's length, and
 * played as one sound-module voice started with frame 1, tagged with the
 * movie's name (the control socket's `sound` part shows it). Pause and
 * bf_movie_goto stop it and start the rest of the track from the movie's
 * position. When the engine has not been started yet (the intro plays before
 * the client starts its sound), the player starts it; the client adopts it.
 *
 * Trace (gfx): "bink: open <name>: WxH, N frames at F fps, T sound track(s)
 * [, first track played | , sound off]", "bink: close <name> after frame n
 * of N (ended|stopped)", "bink: <name> not found in the game folder".
 * Counters: bink.opened, bink.missing, bink.damaged_frames.
 *
 * One movie is played at a time, from the client's main thread;
 * bf_movie_get_state may be called from any thread.
 */
#ifndef BF_MOVIE_H
#define BF_MOVIE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_movie bf_movie;

/* Pixel formats of bf_movie_copy (the Bink SDK's BINKSURFACE32, 565 and
   555/5551, converted by the reader). */
enum {
	BF_MOVIE_BGRA8888 = 0,   /* 32-bit X8R8G8B8 in memory order B, G, R, A */
	BF_MOVIE_RGB565 = 1,     /* 16-bit 5:6:5 */
	BF_MOVIE_XRGB1555 = 2    /* 16-bit 1:5:5:5 */
};

/* Whether movies opened from now on play their sound (BinkSetSoundSystem). */
void bf_movie_use_sound(int on);

/* Opens a movie: name is looked up in the game folder (an absolute path is
   used as it is). NULL, with a trace line and bf_movie_error(), when the
   file is absent or not a Bink 1 movie the reader takes. */
bf_movie *bf_movie_open(const char *name);

/* Opens a movie held in memory (the bytes are copied); name is for the
   trace and the control socket's `movie` part. */
bf_movie *bf_movie_open_memory(const void *data, size_t size, const char *name);

/* Stops the sound and frees the movie; NULL is ignored. */
void bf_movie_close(bf_movie *movie);

/* Decodes frame bf_movie_frame() (BinkDoFrame). The first frame decoded
   starts the clock and the sound. */
void bf_movie_decode(bf_movie *movie);

/* The next frame to show is one on; after the last frame the first, as the
   SDK (BinkNextFrame). */
void bf_movie_next(bf_movie *movie);

/* Non-zero while it is too early to show frame bf_movie_frame() and while
   paused (BinkWait). 0 for NULL. */
int bf_movie_wait(bf_movie *movie);

/* Converts the decoded picture into dest at (x, y): pitch bytes per row,
   height rows, the width following from pitch and the format
   (BinkCopyToBuffer). 0, or -1 for a format not listed above (traced once)
   or a NULL argument. */
int bf_movie_copy(bf_movie *movie, void *dest, int pitch, unsigned height, unsigned x, unsigned y, int format);

/* The next frame decoded is frame (1-based, clamped to the movie); the
   movie's clock and the sound follow (BinkGoto). */
void bf_movie_goto(bf_movie *movie, unsigned frame);

/* 1 stops the picture's clock and the sound, 0 goes on. Returns the new
   state (BinkPause). */
int bf_movie_pause(bf_movie *movie, int pause);

/* The movie's number of frames; the frame to be shown next (1-based); the
   frame last decoded (1-based, 0 before the first). 0 for NULL. */
unsigned bf_movie_frames(const bf_movie *movie);
unsigned bf_movie_frame(const bf_movie *movie);
unsigned bf_movie_last_frame(const bf_movie *movie);

/* The last failure as text ("" before any). */
const char *bf_movie_error(void);

/* What the control socket's `movie` part and event report
   (port/client/src/bf_control_movie.cpp, PROTOCOL.md). */
typedef struct bf_movie_state {
	int  playing;           /* 1 while a movie is open */
	int  paused;
	char file[512];         /* the movie open now, or the last one; "" before any */
	int  width, height;
	int  frames;            /* the movie's number of frames */
	int  frame;             /* the frame last decoded, 1-based; 0 before the first */
	double fps;
	int  tracks;            /* sound tracks in the file */
	int  sound;             /* 1 when the first track was handed to the sound module */
	unsigned voice;         /* its bf_audio voice (0: none, or the engine refused) */
	double sound_seconds;   /* length of that track */
	long long opened;       /* movies opened since start */
	long long closed;       /* movies closed since start */
	int  ended;             /* of the last movie closed: 1 shown to its last frame, 0 stopped before */
	int  last_frame;        /* of the last movie closed: the frame last decoded */
} bf_movie_state;

void bf_movie_get_state(bf_movie_state *out);

#ifdef __cplusplus
}
#endif

#endif
