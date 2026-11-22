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
	wave->Play(x, y, z, volumeMaxDist, volumeMinDist, vFactor);
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
		if (m_fileImage)
			delete m_fileImage;
	}
	m_isDecompress = false;
}

bool	CWave :: Load(const char *fName)
{
	CFile fp;

	if (!fp.Open(fName, CFile::modeRead)) {
		ErrorMsg((string("Wave :: Cannot find File : ") + string(fName)).c_str());
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
