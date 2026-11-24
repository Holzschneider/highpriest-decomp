#include "common.h"
#include "EZeffect.h"
#include "../base/Res.h"


#include "../3dlib/D3dutil.h"
#include "../3dlib/Device.h"
#include "../3dlib/Renderer.h"




#ifndef KAC_MAXANITEXFRAME
#define KAC_MAXANITEXFRAME  110
#endif
#ifndef KSTR_VERSION
#define KSTR_VERSION        0x0094
#endif








static CEZeffectRes CEZeffectResInstance(Exemplar(), "str", "texture\\effect\\");
KAC_LAYER :: KAC_LAYER()
{
}

KAC_LAYER :: ~KAC_LAYER()
{
	for (CTexture** tex = m_tex; tex < m_tex + KAC_MAXANITEXFRAME; ++tex)
		if (*tex) (*tex)->Unlock();
}

CTexture * KAC_LAYER :: GetTexture(int iTex)
{
	sAssert(iTex < cTex);

	if (iTex < cTex)
	{
		if (m_tex[iTex] == NULL) {
			char texName[256] = "effect\\";
			strcat(texName, m_texName[iTex]);
			if (strchr(texName, '.') == NULL) strcat(texName, ".bmp");
			m_tex[iTex] = g_texMgr.GetTexture(texName, true);
			m_tex[iTex]->Lock();
		}

		return m_tex[iTex];
	} else {
		return g_texMgr.GetDummyTexture();
	}
}

CEZeffectRes :: CEZeffectRes()
{
	m_nMaxLayer = 0;
	memset(&m_aniClips, 0, sizeof(m_aniClips));
	m_fp.Close();
}

void	CEZeffectRes :: Reset()
{
	memset(&m_aniClips, 0, sizeof(m_aniClips));
	m_fp.Close();
}

bool	CEZeffectRes :: Load(const char *fName)
{

	if (!m_fp.Open(fName, CFile::modeRead))
	{
#if !defined(BF_PORT)	// missing: counted by CRes::OnLoadError after both names
		ErrorMsg((std::string("Str::Cannot find File : ") + std::string(fName)).c_str());
#endif
		Reset();
		return	false;
	}

	int			iLayer;
	int			iTex;
	DWORD		dwVer, dwSig;


	m_fp.Read(&dwSig, sizeof(DWORD));
	m_fp.Read(&dwVer, sizeof(DWORD));

	m_fp.Read(&m_aniClips.cFrame, sizeof(int));
	m_fp.Read(&m_aniClips.nFPS, sizeof(int));
	m_fp.Read(&m_aniClips.cLayer, sizeof(int));

	m_fp.Seek(sizeof(DWORD) * 4, SEEK_CUR);

	if (dwVer != KSTR_VERSION) {
#if defined(BF_PORT)
		bf_port_missing(BFT_RES, "res.unusable", "%s: str version 0x%x", fName, (unsigned)dwVer);
#else
		ErrorMsg("LoadAniClip: Version is incorrect");
#endif
		Reset();
		return false;
	}

	for (iLayer = 0; iLayer < m_aniClips.cLayer; iLayer++)
	{

		KAC_LAYER& pLayer = m_aniClips.aLayer[iLayer];

		m_fp.Read(&pLayer.cTex, sizeof(int));

		sAssert(pLayer.cTex < KAC_MAXANITEXFRAME);

		for (iTex = 0; iTex < pLayer.cTex; iTex++) {
			pLayer.m_texName[iTex] = (const char*)m_fp.GetCurBuf(128);
		}

		for (; iTex < KAC_MAXANITEXFRAME; ++iTex) pLayer.m_texName[iTex] = "1.bmp";

		m_fp.Read(&pLayer.cAniKey, sizeof(int));
		if (pLayer.cAniKey > 0) {
			pLayer.aAniKey = (KAC_KEYFRAME*)m_fp.GetCurBuf(sizeof(KAC_KEYFRAME) * pLayer.cAniKey);
		}
	}

	return true;
}
