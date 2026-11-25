/*
 * bf_compat_bink.h: the port's side of the Bink adapter (the SDK names the
 * client calls are in port/compat/include/bf_bink_sdk.h). What the control
 * socket's `movie` part and event report (port/client/src/bf_control_movie.cpp).
 */
#ifndef BF_COMPAT_BINK_H
#define BF_COMPAT_BINK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_compat_bink_state {
	int  playing;           /* 1 while a movie is open (BinkOpen .. BinkClose) */
	int  paused;            /* BinkPause */
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
} bf_compat_bink_state;

void bf_compat_bink_get_state(bf_compat_bink_state *out);

#ifdef __cplusplus
}
#endif

#endif
