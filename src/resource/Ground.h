#ifndef HIGHPRIEST_RESOURCE_GROUND_H
#define HIGHPRIEST_RESOURCE_GROUND_H



#include "../common.h"
#include "../base/File.h"
#include "../base/Res.h"
#include "../3dlib/Prim.h"
#include "../3dlib/Renderer.h"

const int WATERTEXTURESIZE = 4;
const float WATERTEXUV = 1.0f / float(WATERTEXTURESIZE);
const	int LMAPWIDTH = 8;
const	int LMAPHEIGHT = 8;
const	int LMAPIDATASIZE = (LMAPHEIGHT * LMAPWIDTH * (int)sizeof(BYTE));
const	int LMAPSDATASIZE = (LMAPHEIGHT * LMAPWIDTH * (int)sizeof(BYTE) * 3);

struct CLMInfo {
	BYTE idata[8][8];
	BYTE sdata[8][8][3];
};

struct LMIndex {
	DWORD a, r, g, b;
};

struct ColorChannel {
	BYTE m_buffer[40];

	void GetChannel(BYTE channel[64]);
	void SetChannel(BYTE channel[64]);

	BYTE GetBuffer(int offset);
	void SetBuffer(int offset, BYTE value);
};

struct GndSurfaceFmt {
	float u[4];
	float v[4];
	short textureId;
	unsigned short lightmapId;
	COLOR argb;
};

struct GndCellFmt17 {
	float height[4];
	int topSurfaceId;
	int frontSurfaceId;
	int rightSurfaceId;
};

class CGndRes : public CRes {
public:
	CGndRes();
	DECLARE_RESOURCE(CGndRes)
	virtual bool Load(const char* fName);
	virtual void Reset();

	bool LoadGnd();

	void GetLMInfo(int n, CLMInfo* lminfo);

	const char* GetTextureName(int n) { return m_texNameTable[n]; }

	GndSurfaceFmt* GetSurface(int n) { return &m_surface[n]; }

	const GndCellFmt17& GetCells(int n) { return m_V17cells[n]; }

	BOOL m_newVer;
	BYTE m_verMajor, m_verMinor;
	int m_width, m_height;
	float m_zoom;
	int m_numTexture;
	int m_numSurface;
	int m_numLightmap;

protected:
	std::vector<const char*> m_texNameTable;

	CLMInfo* m_lminfo;

	LMIndex* m_lmindex;
	ColorChannel* m_colorchannel;

	GndSurfaceFmt* m_surface;
	GndCellFmt17* m_V17cells;

	CFile m_fp;
#if defined(BF_PORT)
	// The records above point into the file buffer, where they start at any
	// offset (the texture names before them are 80 bytes each, the lightmap
	// header 16): on arm64 they are copied when misaligned (Ground.cpp).
	std::vector<LMIndex> m_bfLmIndex;
	std::vector<GndSurfaceFmt> m_bfSurfaces;
	std::vector<GndCellFmt17> m_bfCells;
#endif
};

#endif
