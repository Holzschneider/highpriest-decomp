#ifndef HIGHPRIEST_RESOURCE_EZEFFECT_H
#define HIGHPRIEST_RESOURCE_EZEFFECT_H

#include "../common.h"
#include "../base/File.h"
#include "../base/Res.h"

class CTexture;
class CEZeffectRes;
struct KAC_LAYER;
struct KANICLIP;

struct KAC_XFORMDATA {
	float x;
	float y;
	float u;
	float v;
	float us;
	float vs;
	float u2;
	float v2;
	float us2;
	float vs2;
	float ax[4];
	float ay[4];
	float aniframe;
	DWORD anitype;
	float anidelta;
	float rz;
	float crR;
	float crG;
	float crB;
	float crA;
	DWORD srcalpha;
	DWORD destalpha;
	DWORD mtpreset;
};

typedef struct {
	int iFrame;
	DWORD dwType;
	KAC_XFORMDATA XformData;
} KAC_KEYFRAME;

struct KAC_LAYER {
public:
	KAC_LAYER();
	~KAC_LAYER();
	CTexture* GetTexture(int iTex);
	int cTex;
	int iCurAniFrame;
	CTexture* m_tex[110];
	const char* m_texName[110];
	int cAniKey;
	KAC_KEYFRAME* aAniKey;
};

struct KANICLIP {
public:
	int nFPS;
	int cFrame;
	int cLayer;
	int cEndLayer;
	KAC_LAYER aLayer[60];
};

class CEZeffectRes : public CRes {
public:
	DECLARE_RESOURCE(CEZeffectRes)
	CEZeffectRes();
	virtual bool Load(const char* fName);
	virtual void Reset();





	KANICLIP m_aniClips;
	int m_nMaxLayer;
protected:
	CFile m_fp;
};

#endif
