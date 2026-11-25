#include "common.h"
#include "Wave.h"
#include "../base/File.h"
#include "../device/Sound.h"


int g_isSoundOn = 1;

using std::string;


void	PlayWave(const char *waveFileName, float x, float y, float z, int volumeMaxDist, int volumeMinDist, float vFactor)
{
	if (!g_soundMode)
		return;
	if (!g_isSoundOn)
		return;
	CWave *	wave = (CWave *) (g_resMgr().Get(waveFileName, false));
#if defined(BF_PORT)
	// names the voice PlayDigital may start with the file and its distance,
	// for the `sound` part of the control socket
	bf_audio_tag_next_voice(waveFileName, _sqrt(x * x + y * y + z * z));
#endif
	wave->Play(x, y, z, volumeMaxDist, volumeMinDist, vFactor);
#if defined(BF_PORT)
	bf_audio_tag_next_voice(NULL, 0);	// out of range or no free handle: no voice took it
#endif
}


static CWave CWaveInstance(Exemplar(), "wav", "wav\\");
CWave :: CWave()
	: CRes()
{
	m_fileImage = NULL;
	m_fileLength = 0;
	m_sound = NULL;
	m_soundFailed = false;
}

void	CWave :: Reset()
{
	// the sound goes with the wave (a voice still playing it plays to its end)
	bf_audio_sound_destroy(m_sound);
	m_sound = NULL;
	m_soundFailed = false;
	if (m_fileImage)
		delete [] m_fileImage;
	m_fileImage = NULL;
	m_fileLength = 0;
}

bool	CWave :: Load(const char *fName)
{
	CFile fp;

	if (!fp.Open(fName, CFile::modeRead)) {
#if !defined(BF_PORT)	// missing: counted by CRes::OnLoadError after both names
		ErrorMsg((string("Wave :: Cannot find File : ") + string(fName)).c_str());
#endif
		return false;
	}

	BYTE* tmpFileImage = NULL;
	DWORD fileLength = fp.GetLength();
	Reset();
	tmpFileImage = new BYTE[fileLength];
	fp.Read(tmpFileImage, fileLength);
	fp.Close();

	// PCM, ADPCM and MP3 alike: the sound module decodes the image on the
	// first play (Miles decompressed ADPCM and MP3 here)
	m_fileImage = tmpFileImage;
	m_fileLength = fileLength;
	return true;
}

// The sound made from the file image, or NULL when it cannot be decoded
// (traced and counted once per load).
bf_audio_sound*	CWave :: GetSound()
{
	if (m_sound || m_soundFailed || !m_fileImage)
		return m_sound;
	m_sound = bf_audio_sound_create_from_memory(m_fileImage, m_fileLength);
	if (!m_sound) {
		m_soundFailed = true;
		bf_count("sound.undecodable_waves", 1);
		BF_TRACE(BFT_SND, "sound: %s (%u bytes) cannot be played", GetName(), m_fileLength);
		return NULL;
	}
	delete [] m_fileImage;	// decoded: the sound holds the samples
	m_fileImage = NULL;
	return m_sound;
}

void	CWave :: Play(float &x, float &y, float &z, int volumeMaxDist, int volumeMinDist, float vFactor)
{
	switch (g_soundMode) {
		case 2:
			Play3DDigital(GetSound(), x, y, z, volumeMaxDist, volumeMinDist, vFactor);
			break;
		case 1:
			PlayDigital(GetSound(), x, y, z, volumeMaxDist, volumeMinDist, vFactor);
			break;
	}
}
