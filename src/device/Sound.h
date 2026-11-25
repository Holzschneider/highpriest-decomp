#ifndef HIGHPRIEST_DEVICE_SOUND_H
#define HIGHPRIEST_DEVICE_SOUND_H





#include "bf_audio.h"	// port/native/audio (M11.6: the client calls the sound module, not Miles)



const int	VOLUMEMAXDIST = 250;
const int	VOLUMEMINDIST = 40;

extern	bool		InitMSS();
extern	void		UnInitMSS();
extern	void		CloseStream();
extern	void		PlayStream(const char *streamFileName);
extern	void		PauseStream(int bgmIsPaused);
extern	bool		PlayDigital(bf_audio_sound *sound, float &x, float &y, float &z, int volumeMaxDist=VOLUMEMAXDIST, int volumeMinDist=VOLUMEMINDIST, float vFactor=1.0f);
extern	bool		Play3DDigital(bf_audio_sound *sound, float &x, float &y, float &z, int volumeMaxDist=VOLUMEMAXDIST, int volumeMinDist=VOLUMEMINDIST, float vFactor=1.0f);
extern	void		PauseDigitalSound(BOOL IsPause);
extern	void		EndDigitalSound();
extern	void		SetRoomEffect(int val);
extern	void		SetListenerOrient(float roty);
extern	void		SetStreamVolume(int volume);
extern	void		Set2DVolume(int volume);
extern	void		Set3DVolume(int volume);
extern	int			GetStreamVolume();
extern	int			Get2DVolume();
extern	int			Get3DVolume();

const	int MAXSAMPLE = 48;
const	int MAX3DSAMPLE = 48;

// EAX's generic room, the only room type the client asks for (GameMode.cpp).
const	int ENVIRONMENT_GENERIC = 0;

// One of the client's sample handles: the voice it started last and that
// voice's volume (Miles' 0..127).
struct	CMssSample {
		bool			allocated;
		bf_audio_voice	voice;
		int				volume;
};

class	CMssVariables {
	public:
		CMssVariables();

	public:
		bool			hDigital;		// the sound module is started
		bool			hDigitalStream;	// a music file is open
		CMssSample		hSamples[MAXSAMPLE];
		int				streamVolume;
		int				digital2DVolume;
		int				digital3DVolume;
		char			streamFileName[256];
		int				volumeMaxDist;
		int				volumeMinDist;
		int				digitalrate;
		int				digitalbits;
		int				digitalchannels;
		int				numSamples;

		int				roomType;
		int				num3DSamples;
};

extern	CMssVariables	g_mssVar;

#endif
extern unsigned int g_soundMode;
