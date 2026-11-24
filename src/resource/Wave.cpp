#include "common.h"
#include "Wave.h"
#include "../base/File.h"
#include "../device/Sound.h"


int g_isSoundOn = 1;

#if defined(BF_PORT)
// Names the voice PlayDigital may start with the file and its distance, for
// the `sound` part of the control socket (port/native/audio, bf_audio.h).
extern "C" void bf_audio_tag_next_voice(const char *label, float distance);
// Drops the sound the Miles adapter decoded from an image the client frees
// itself (port/compat/miles).
extern "C" void bf_miles_forget_image(const void *image);
#endif

using std::string;


void	PlayWave(const char *waveFileName, float x, float y, float z, int volumeMaxDist, int volumeMinDist, float vFactor)
{
	if (!g_soundMode)
		return;
	if (!g_isSoundOn)
		return;
	CWave *	wave = (CWave *) (g_resMgr().Get(waveFileName, false));
#if defined(BF_PORT)
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
	m_isDecompress = false;
}

void	CWave :: Reset()
{
	if (m_isDecompress) {
		if (m_fileImage)
			AIL_mem_free_lock(m_fileImage);
	} else {
#if defined(BF_PORT)
		// not freed through Miles, so the adapter would keep the sound made
		// from it: every unloaded wave stayed decoded (the soak's leak)
		if (m_fileImage)
			bf_miles_forget_image(m_fileImage);
#endif
		if (m_fileImage)
			delete m_fileImage;
	}
	m_isDecompress = false;
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

	AILSOUNDINFO info;
	int type = AIL_file_type(tmpFileImage, fileLength);
	switch (type) {
		case AILFILETYPE_PCM_WAV:
			m_fileImage = tmpFileImage;
			break;

		case AILFILETYPE_ADPCM_WAV:
			m_isDecompress = true;
			AIL_WAV_info(tmpFileImage, &info);
			AIL_decompress_ADPCM(&info, (void**)&m_fileImage, 0);
			delete [] tmpFileImage;
			break;

		case AILFILETYPE_MPEG_L3_AUDIO:
			m_isDecompress = true;
			AIL_decompress_ASI(tmpFileImage, fileLength, fName, (void**)&m_fileImage, 0, 0);
			delete [] tmpFileImage;
			break;

		default:
			m_fileImage = tmpFileImage;
			break;
	}
	return true;
}

void	CWave :: Play(float &x, float &y, float &z, int volumeMaxDist, int volumeMinDist, float vFactor)
{
	switch (g_soundMode) {
		case 2:
			Play3DDigital(m_fileImage, x, y, z, volumeMaxDist, volumeMinDist, vFactor);
			break;
		case 1:
			PlayDigital(m_fileImage, x, y, z, volumeMaxDist, volumeMinDist, vFactor);
			break;
	}
}
