/*
 * bf_bink_sdk.h: the part of RAD's Bink SDK (binkw32, version 1.5) that the
 * client calls, as the port provides it (port/compat/bink, on the Bink reader
 * port/native/formats/bink). src/movie/CBink.cpp includes this under BF_PORT
 * instead of the SDK's RAD.H and BINK.H, which do not know the platform.
 *
 * Written for the port from the names the client uses; the values of the
 * constants are the SDK's, so the client's call sites stay as they are.
 * Stage A only: M11 changes CBink to call port/native/formats/bink directly.
 */
#ifndef BF_BINK_SDK_H
#define BF_BINK_SDK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The movie. The client reads Frames and FrameNum; the first fields are in
   the SDK's order and meaning, the rest is the adapter's. */
typedef struct BINK {
	unsigned int Width;        /* picture size in pixels */
	unsigned int Height;
	unsigned int Frames;       /* number of frames */
	unsigned int FrameNum;     /* the frame to be shown next, 1-based */
	unsigned int LastFrameNum; /* the frame last decoded, 1-based (0 before the first) */
	unsigned int FrameRate;    /* frames per second = FrameRate / FrameRateDiv */
	unsigned int FrameRateDiv;
	unsigned int ReadError;
	unsigned int OpenFlags;
	unsigned int BinkType;
	unsigned int Size;         /* bytes of the file */
	unsigned int FrameSize;
	unsigned int SndSize;
	int Paused;                /* BinkPause */
	void *bf;                  /* the adapter's own state */
} BINK;
typedef BINK *HBINK;

/* BinkOpen flags the port knows (the SDK's values). */
#define BINKFROMMEMORY   0x04000000L   /* name points at the whole file in memory (the bytes are copied) */

/* BinkGoto flags. */
#define BINKGOTOQUICK    1

/* Pixel formats of BinkCopyToBuffer and BinkDDSurfaceType (the SDK's
   numbers); the port converts to 32-bit, 5:6:5 and 5:5:5. */
#define BINKSURFACE32    3
#define BINKSURFACE5551  8
#define BINKSURFACE555   9
#define BINKSURFACE565   10
#define BINKSURFACEMASK  15

/* The sound system. The port plays a movie's first sound track through the
   sound module (port/native/audio) once any sound system was chosen. */
typedef void *(*BINKSNDSYSOPEN)(uintptr_t param);
void *BinkOpenDirectSound(uintptr_t param);
int BinkSetSoundSystem(BINKSNDSYSOPEN open, uintptr_t param);
#define BinkSoundUseDirectSound(lpDS) BinkSetSoundSystem(BinkOpenDirectSound, (uintptr_t)(lpDS))

/* Opens a movie: name is looked up in the game folder (an absolute path is
   used as it is); with BINKFROMMEMORY it is the file's bytes. NULL when the
   file is absent or not a movie the reader accepts (traced). */
HBINK BinkOpen(const char *name, unsigned int flags);

/* Decodes frame FrameNum. 0. */
int BinkDoFrame(HBINK bink);

/* Moves FrameNum on by one; after the last frame back to the first, as the SDK. */
void BinkNextFrame(HBINK bink);

/* Non-zero while it is too early to show frame FrameNum (and while paused). */
int BinkWait(HBINK bink);

void BinkClose(HBINK bink);

/* pause 1 stops picture and sound, 0 goes on. Returns the new state. */
int BinkPause(HBINK bink, int pause);

/* Converts the decoded picture into dest at (x, y); dest is pitch bytes per
   row and destheight rows; the width follows from pitch and the format
   (flags & BINKSURFACEMASK). 0, or -1 for a format the port lacks. */
int BinkCopyToBuffer(HBINK bink, void *dest, int destpitch, unsigned int destheight, unsigned int destx,
                     unsigned int desty, unsigned int flags);

/* The next frame decoded is `frame` (1-based); the sound follows. */
void BinkGoto(HBINK bink, unsigned int frame, int flags);

/* The BINKSURFACE* format of a DirectDraw 7 surface (IDirectDrawSurface7 *),
   -1 when there is none. */
int BinkDDSurfaceType(void *lpDDS);

/* The last error as text. */
const char *BinkGetError(void);

/* Port only: opens a movie held in memory (the bytes are copied); name is
   for the trace and the control socket's `movie` part. */
HBINK BfBinkOpenMemory(const void *data, size_t size, const char *name);

#ifdef __cplusplus
}
#endif

#endif
