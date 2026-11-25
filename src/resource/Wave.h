#ifndef HIGHPRIEST_RESOURCE_WAVE_H
#define HIGHPRIEST_RESOURCE_WAVE_H

#include "../base/Res.h"
#include "../device/Sound.h"

class CWave;

class CWave : public CRes {
public:
	DECLARE_RESOURCE(CWave)
	CWave();
	virtual bool Load(const char* fName);
	virtual void Reset();
	void Play(float& x, float& y, float& z, int volumeMaxDist, int volumeMinDist, float vFactor);
	void Play3d(float, int);
	void PlayLooped();
private:
	bf_audio_sound* GetSound();
	unsigned char* m_fileImage;
	unsigned int m_fileLength;
	bf_audio_sound* m_sound;	// made from m_fileImage on the first play (port/native/audio)
	bool m_soundFailed;		// m_fileImage cannot be decoded: not tried again
};

void	PlayWave(const char *waveFileName, float x=0, float y=0, float z=0, int volumeMaxDist=VOLUMEMAXDIST, int volumeMinDist=VOLUMEMINDIST, float vFactor=1.0f);

extern	BOOL	g_isSoundOn;

#endif
