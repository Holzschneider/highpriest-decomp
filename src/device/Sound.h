#ifndef HIGHPRIEST_DEVICE_SOUND_H
#define HIGHPRIEST_DEVICE_SOUND_H





#include "Mss.h"



const int	VOLUMEMAXDIST = 250;
const int	VOLUMEMINDIST = 40;

extern	bool		InitMSS();
extern	void		UnInitMSS();
extern	void		CloseStream();
extern	void		PlayStream(const char *streamFileName);
extern	void		PauseStream(int bgmIsPaused);
extern	bool		PlayDigital(BYTE *fileImage, float &x, float &y, float &z, int volumeMaxDist=VOLUMEMAXDIST, int volumeMinDist=VOLUMEMINDIST, float vFactor=1.0f);
extern	bool		Play3DDigital(BYTE *fileImage, float &x, float &y, float &z, int volumeMaxDist=VOLUMEMAXDIST, int volumeMinDist=VOLUMEMINDIST, float vFactor=1.0f);
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
const	int MAX_PROVIDERS = 64;
const	int MAX3DSAMPLE = 48;

class	CMssVariables {
	public:
		CMssVariables();

	public:
		HDIGDRIVER		hDigital;
		HSTREAM			hDigitalStream;
		HSAMPLE			hSamples[MAXSAMPLE];
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

		char *			provider_names[MAX_PROVIDERS];
		HPROVIDER		provider_handles[MAX_PROVIDERS];
		HPROVIDER		provider_default;
		HPROENUM		enum3D;
		H3DSAMPLE		h3DSamples[MAX3DSAMPLE];
		H3DPOBJECT		hListener;
		int				usingEAX;
		int				roomType;
		int				num3DSamples;
};

extern	CMssVariables	g_mssVar;

#endif
extern unsigned long g_soundMode;
