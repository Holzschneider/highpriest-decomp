#ifndef HIGHPRIEST_3DGROUND_H
#define HIGHPRIEST_3DGROUND_H

#include "common.h"
#include "3dlib/Prim.h"
#include "3dlib/Renderer.h"
#include "resource/Ground.h"
#include "resource/Attr.h"
extern	vector2d	g_uv_all[4];

struct CLightmap;
class C3dGround;
class C3dGround15;
struct CGndCell;
struct CGndSurface;
class CLightmapMgr;
struct lmvertex3d;

struct lmvertex3d {
public:
	vector3d wvert;
	COLOR argbs;
	vector2d uv;
	vector2d uv2;
};

struct CGndSurface {
public:
	CGndSurface() : alpha(0), tex(NULL) {}
	lmvertex3d vertex[4];
	int offset;
	CTexture* tex;
	CTexture* lmtex;
	CLightmap* lmap;
	COLOR argb;
	unsigned int alpha;
	int MtlId;
};

struct CGndCell {
public:
	CGndCell() : top(NULL), right(NULL), front(NULL) { cellColor[0] = cellColor[1] = cellColor[2] = 255; }
	float h[4];
	vector3d watervert[4];
	CGndSurface* top;
	CGndSurface* right;
	CGndSurface* front;
	int renderSignature;
	unsigned char cellColor[3];
};

struct CLightmap
{
	BYTE brightObj[3];
	CTexture* surface;
	texCoor coor[4];
	COLOR intensity[4];
};

class CLightmapMgr {
public:
	CLightmapMgr();
	~CLightmapMgr();
	void Create(CGndRes* gnd);
	void Reset();
	std::vector<CLightmap> m_lightmaps;
	std::vector<CTexture *> m_lmSurfaces;
	int m_numLightmaps;
	int m_numLmSurfaces;
};

struct LightmapRGBIntensity {
	float rIntensity;
	float gIntensity;
	float bIntensity;
};

struct CellNormal {
public:
	vector3d top[2];
	vector3d front;
	vector3d right;
};

class C3dGround {
public:
	C3dGround();
	virtual ~C3dGround() {}
	virtual void Create(int w, int h) {}
	virtual void AssignGnd(CGndRes* gnd, vector3d& lightDir, vector3d& diffuseCol, vector3d& ambientCol) {}
	virtual void Reset() {}
	virtual void FlushGround(const matrix& wtm) {}
	virtual void RenderLod(const matrix& wtm, const tagRECT& area, bool needClip) {}
	virtual void Render(const matrix& wtm, const tagRECT& area, bool needClip) {}
	virtual void GetHeightMinMax(const tagRECT& area, float& topHeight, float& bottomHeight) {}
	virtual void CheckAlpha(const ray3d& ray, int x, int y, float nearest) {}
	virtual bool IsNewVer() { return false; }
	virtual void GetHeightMinMax(int x, int y, float& topHeight, float& bottomHeight) {}
	virtual void CalcLmIntensityFloatRGBAll() {}
	virtual int GetLmIntensityFloatRGBByGatCoor(int cx, int cy, float& r, float& g, float& b) { return 0; }
	void ConvertToCellCoor(float x, float z, int& cx, int& cy);
	void UpdateWater();
	void SetWaterInfo(float waterLevel, int textureType, int textureAnimSpeed, int pitch, int speed, float height);
	virtual void RenderAttrTile(const matrix& wtm, int attrX, int attrY, unsigned int color) {}
	virtual void SetGndRiseAmount(float amount, int x, int y) {}
	void RenderGroundTile(const matrix& wtm, int attrX, int attrY, unsigned int color, CTexture* tex, vector2d* uv);
	virtual bool GetGroundVertics(vector3d* nwvert, int attrX, int attrY);
	CTexture* GetWaterTexture();
protected:
	friend struct SceneGraphNode;
	friend class CWorld;
	C3dAttr* m_attr;
	int m_width;
	int m_height;
	float m_zoom;
	CLightmapMgr m_lightmapMgr;
	int m_numSurfaces;
	float m_waterLevel;
	int m_texAnimCycle;
	int m_wavePitch;
	int m_waveSpeed;
	int m_waterSet;
	float m_waveHeight;
	CTexture* m_waterTex;
	CTexture* m_pBumpMap;
	int m_waterCnt;
	int m_waterOffset;
	int m_isNewVer;
};

class C3dGround15 : public C3dGround {
public:
	virtual void Create(int w, int h);
	virtual void AssignGnd(CGndRes* gnd, vector3d& light, vector3d& diffuseCol, vector3d& ambientCol);
	virtual void Reset();
	virtual void FlushGround(const matrix& wtm);
	virtual void RenderLod(const matrix& wtm, const tagRECT& area, bool needClip);
	virtual void Render(const matrix& wtm, const tagRECT& area, bool needClip);
	virtual void CheckAlpha(const ray3d& ray, int x, int y, float nearest);
	virtual void CalcLmIntensityFloatRGBAll();
	virtual void GetHeightMinMax(const tagRECT& x, float& y, float& topHeight);
	virtual void GetHeightMinMax(int x, int y, float& topHeight, float& bottomHeight);
	virtual int GetLmIntensityFloatRGBByGatCoor(int cx, int cy, float& r, float& g, float& b);
	void GetBrightObjVal(vector3d v, int* rgb);
	virtual bool IsNewVer() { return true; }
private:
	CGndCell* GetCell15(int x, int y)
	{
		return &m_V15Cells[y * m_width + x];
	}
	void GetLmIntensityRGBByGatCoor(int cx, int cy, unsigned char& in, unsigned char& rI, unsigned char& gI, unsigned char& bI);
	virtual bool GetGroundVertics(vector3d* nwvert, int attrX, int attrY);
	void RenderNew(const matrix& wtm, CGndSurface* surface, bool needClip);
	void RenderNewLod(const matrix& wtm, CGndSurface* surface, bool needClip);
	virtual void RenderAttrTile(const matrix& wtm, int attrX, int attrY, unsigned int color);
	void SetLmIntensityFloatRGBByGatCoor(int cx, int cy, float& r, float& g, float& b);
private:
	std::vector<CGndSurface> m_V15surfaces;
	std::vector<LightmapRGBIntensity> m_lmRGBIntensities;
	std::vector<CGndCell> m_V15Cells;
};

#endif
