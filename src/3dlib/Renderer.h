#ifndef HIGHPRIEST_3DLIB_RENDERER_H
#define HIGHPRIEST_3DLIB_RENDERER_H

#include "Prim.h"
#include "Device.h"
#include "Texture.h"
#include <list>

class CBitmapRes;



#define		RF_TYPE_MASK		0x00FF
#define		RF_EFFECT_MASK		0xFF00

#define		RF_NULL				0x0000

#define		RF_ALPHA			0x0001
#define		RF_EMISSIVE			0x0002
#define		RF_ALPHAOP			0x0004
#define		RF_ALPHAOP_OM		0x0008
#define		RF_ALPHAOP_OM_2		0x0010
#define		RF_ALPHA_OM_SHADOW	0x0020
#define		RF_ALPHA_OM_NODEPTH	0x0040
#define		RF_ALPHAOP_OM_4		0x0080

#define		RF_DEPTHWRITE		0x0100
#define		RF_NODEPTHCHECK		0x0200
#define		RF_BUMPMAP			0x0400
#define		RF_GROUND			0x0800
#define		RF_LIGHT			0x1000
#define		RF_PC				0x2000
#define     RP_GROUND_PASS2     0x4000
#define		RF_WATER			0x8000

#define		RF_NORMAL			(RF_NULL)
#define		RF_SPRITE			(RF_ALPHA | RF_DEPTHWRITE)
#define		RF_SPRITEPC			(RF_ALPHA | RF_DEPTHWRITE | RF_PC)
#define		RF_EFFECT			(RF_ALPHA | RF_ALPHAOP)
#define		PF_SPRSHADOW		(RF_ALPHA | RF_ALPHA_OM_SHADOW)
#define		RF_EFFECT_OM_3		(RF_ALPHA | RF_ALPHA_OM_NODEPTH)
#define		RF_EFFECT_OM		(RF_ALPHA | RF_ALPHAOP_OM )
#define		RF_EFFECT_OM_2		(RF_ALPHA | RF_ALPHAOP_OM_2)
#define		RF_NODEPTHWRITE		(RF_NODEPTHCHECK)
#define		RF_UI				(RF_ALPHA | RF_NODEPTHCHECK)
#define		RP_WATER			(RF_WATER | RF_ALPHA )
#define		RF_EFFECT_OM_4		(RF_ALPHA | RF_ALPHAOP_OM_4)

enum RfLmType { RF_LM_LIGHTMAP, RF_LM_BUMPMAP };
const	float	FARPLANE = 1500.0f;
const	float	NEARPLANE = 10.0f;
const	float	HFOV = 15.0f;
const	float	VFOV = 15.0f;

struct SprImg;
struct CMotion;
class CSprRes;

struct RPRaw {
public:
	void DrawPri(IDirect3DDevice7* device);
	void DrawPriIndexed(IDirect3DDevice7* device);
	matrix world;
	CDynamicVB* vb;
	int vertStart;
	int numVerts;
	unsigned short* indices;
	unsigned long numIndices;
	CTexture* tex;
	CTexture* tex2;
};

struct RPFace {
public:
	RPFace(tlvertex3d* vert, int numVert);
	void SetGeomInfo(int vertId, const vector4d& src);
	void SetGeomInfo(int vertId, const tlvertex3d& src);
	void InitSpecular();
	void DrawPri(IDirect3DDevice7* device);
	tlvertex3d* verts;
	int numVerts;
	CTexture* tex;
	unsigned short* indices;
	unsigned long numIndices;
	unsigned long srcAlphaMode;
	unsigned long destAlphaMode;
	unsigned long mtPreset;
	_D3DPRIMITIVETYPE primType;
};

struct RPTriFace {
public:
	tlvertex3d verts[3];
	CTexture* tex;
	unsigned long srcAlphaMode;
	unsigned long destAlphaMode;
};

struct RPQuadFace : public RPFace {
public:
	RPQuadFace();
	RPQuadFace(const RPQuadFace&);
	const RPQuadFace& operator=(const RPQuadFace& rp);
	tlvertex3d m_verts[4];
};

struct RPMesh : public RPFace {
public:
	RPMesh();
	~RPMesh();
	void AllocVerts(int numVerts);
};

struct RPLmFace {
public:
	void InitSpecular();
	void DrawPri(IDirect3DDevice7* device);
	lmtlvertex3d* lmverts;
	int numVerts;
	unsigned short* indices;
	unsigned long numIndices;
	CTexture* tex;
	CTexture* tex2;
	_D3DPRIMITIVETYPE primType;
};

struct RPLmQuadFace : public RPLmFace {
public:
	RPLmQuadFace();
	RPLmQuadFace(const RPLmQuadFace&);
	const RPLmQuadFace& operator=(const RPLmQuadFace& rp);
	lmtlvertex3d m_lmverts[4];
};

struct RPSprite {
	float top, left, bottom, right, oow, oowup;
	DWORD rgba;
	float angle;
	int cacheTexNum;
	float u, v;
	SprImg* spr;
};

struct FogPara {
	DWORD mode;
	DWORD color;
	BOOL useRange;
	float density;
	float start, end;
};

class CDC {
public:
	virtual ~CDC() {}
	virtual void Resize(int, int) = 0;
	virtual bool GetDC(HDC__**) = 0;
	virtual void ReleaseDC(HDC__*) = 0;
	virtual void Update(int, int, int, int, unsigned long*, bool) = 0;
	virtual void CopyRect(int, int, int, int, CDC*) = 0;
	virtual void BltSprite(int, int, CSprRes*, CMotion*, unsigned long*) = 0;
	virtual void BltTexture2(int, int, CTexture*, int, int, int, int, int, int, int) = 0;
	virtual void ClearSurface(tagRECT*, unsigned long) = 0;
	virtual void DrawSurface(int, int, int, int, unsigned long) = 0;
};

class CDCSurface : public CDC {
public:
	CDCSurface(unsigned long w, unsigned long h, IDirectDrawSurface7* surface);
	CDCSurface(unsigned long w, unsigned long h);
	virtual ~CDCSurface();
	virtual void Resize(int w, int h);
	virtual bool GetDC(HDC__** hdc);
	virtual void ReleaseDC(HDC__* hdc);
	virtual void Update(int x, int y, int width, int height, unsigned long* image, bool SkipColorKey);
	virtual void CopyRect(int x, int y, int width, int height, CDC* surface);
	virtual void BltSprite(int x, int y, CSprRes* sprRes, CMotion* curMotion, unsigned long* palette) {}
	virtual void BltTexture2(int x, int y, CTexture* src, int srcx, int srcy, int width, int height, int xflip, int zoomx, int zoomy);
	virtual void ClearSurface(tagRECT* rect, unsigned long color);
	virtual void DrawSurface(int x, int y, int width, int height, unsigned long color);
protected:
	CSurface m_surface;
};

class CDCBitmap : public CDC {
public:
	CDCBitmap(unsigned long w, unsigned long h);
	virtual ~CDCBitmap();
	virtual void Resize(int w, int h);
	virtual bool GetDC(HDC__** hdc);
	virtual void ReleaseDC(HDC__* hdc);
	virtual void Update(int x, int y, int width, int height, unsigned long* imgData, bool SkipColorKey);
	virtual void CopyRect(int x, int y, int width, int height, CDC* surface);
	virtual void BltSprite(int x, int y, CSprRes* sprRes, CMotion* curMotion, unsigned long* palette);
	virtual void BltTexture2(int x, int y, CTexture* src, int srcx, int srcy, int width, int height, int xflip, int zoomx, int zoomy);
	virtual void ClearSurface(tagRECT* rect, unsigned long color);
	virtual void DrawSurface(int x, int y, int w, int h, unsigned long color);
protected:
	void UpdateSurface();
	void CreateDCSurface(unsigned long w, unsigned long h);
protected:
	unsigned long m_w;
	unsigned long m_h;
	HDC__* m_dc;
	HBITMAP__* m_bitmap;
	HBITMAP__* m_bitmapOld;
	unsigned long* m_image;
	unsigned char m_dirty;
	std::list<CTexture *> m_textureList;
	typedef std::list<CTexture *> texList;
	typedef std::list<CTexture *>::iterator texListIter;
};

class CTexMgr {
public:
	CTexMgr();
	~CTexMgr();
	CTexture* CreateTexture(int w, int h, unsigned long* pf, PixelFormat surface, bool blackkey);
	CTexture* CreateTexture(unsigned long w, unsigned long h, PixelFormat pf, IDirectDrawSurface7* surface);
	void ReCreateTexture(CTexture* tex, int w, int h, unsigned long* imgData);
	CTexture* GetTexture(const char* fName, bool blackkey);
	void DestroyTexture(CTexture* t);
	void DestroyAllTexture();
	void ReloadAllTexture();
	void UnloadRarelyUsedTexture();
	CTexture* GetDummyTexture() { return &s_dummy_texture; }
private:
	CTexture* FindTexture(const char* fName);
	CTexture* LoadTexture(const char* fName, bool blackkey);
	void AddTexture(CTexture* t);
	static CTexture s_dummy_texture;
private:
	std::map<char const *, CTexture *, CharPrtLess, std::allocator<CTexture *> > m_texTable;
};

struct CacheInfo {
	CacheInfo() { memset(this, 0, sizeof(CacheInfo)); }
	DWORD id;
	DWORD palID;
	DWORD lastTime;
	float tu, tv;
	float tu1, tv1;
	DWORD x;
	DWORD y;
};

struct CacheSurface {
public:
	CTexture* tex;
	int w;
	int h;
	int texWidth;
	int texHeight;
	std::vector<CacheInfo> info;
	int isLocked;
	unsigned long lastTime;
};

struct UVRECT {
	UVRECT() {}
	UVRECT(float newU1, float newV1, float newU2, float newV2);
	float u1, v1;
	float u2, v2;
};

class CRenderer {
public:
	CRenderer();
	~CRenderer();
	void SetSize(int cx, int cy);
	void SetPixelFormat(PixelFormat pf);
	void SetTexture(CTexture* tex);
	void SetLmapTexture(CTexture* tex);
	void ReleaseCacheSurfaces();
	void CreateUnusedCacheSurfaces();
	void ReleaseUnusedCacheSurfaces();
	void UnloadRarelyUsedCaches();
	void DestroyAllRPList();
	void SetLookAt(vector3d& from, vector3d& at, vector3d& up);
	void SetLight(vector3d& from, vector3d& at, vector3d& up);
	void Clear(int clearScreen);
	void ClearBackground();
	void SetBackgroundColor(unsigned long color) { m_nClearColor = color; }
	bool DrawScene();
	int Lock();
	void Unlock();
	void Flip(bool vertSync);
	void BackupFrame();
	void RestoreFrame();
	int SaveBmp(const char* fName, CBitmapRes* bitmap);
	int SaveJPG(const char* fName, CBitmapRes* bitmap);
	void ScreenEffect();
	void DrawAgeRate();
	void DrawBattleFieldIcon();
	void ProjectVertex(const vector3d& src, const matrix& vtm, tlvertex3d* x);
	void ProjectVertex(const vector3d& src, const matrix& vtm, float* x, float* y, float* oow);
	float GetPixelRatio(float oow);
	float GetScreenXFactor();
	float GetScreenYFactor();
	void BuildRay(int sx, int sy, const vector3d& from, const matrix& invViewMatrix, ray3d* ray);
	RPFace& BorrowNullRP();
	RPQuadFace& BorrowQuadRP();
	RPLmQuadFace& BorrowLmQuadRP();
	tlvertex3d* BorrowVerts(unsigned int vertCount);
	void ResetBorrowedRP();
	void AddRP(RPTriFace* rp, int renderFlag);
	void AddRP(RPFace* rp, int renderFlag);
	void AddRawRP(RPRaw* rp, int renderFlag);
	void AddLmRP(RPLmFace* rp, RfLmType renderFlag);
	void FlushRenderList();
	CTexture* AddSpriteIndex(SprImg& img, unsigned long pal[], CacheInfo* info);
	CTexture* GetSpriteIndex(SprImg& img, unsigned long palID, CacheInfo* info);
	void DoFog();
	void FogSwitch(int clearScreen);
	int FogFactor(float);
	void SetMultiTextureMode(int clearScreen);
	int TextOutScreen(int x, int y, const char* text, unsigned long colorRef, int fontHeight, char fontPrint);
	void ShowInfo();
	void StartFPSCount();
	tagSIZE* GetTextSize(const char* text, int textLen, int fontType, int fontHeight, bool bold);
private:
	void FlushFaceList();
	void FlushLMGroundList();
	void FlushLMLightList();
	void FlushAlphaDepthList();
	void FlushAlphaList();
	void FlushAlphaNoDepthList();
	void FlushEmissiveDepthList();
	void FlushEmissiveList();
	void FlushEmissiveNoDepthList();
	void FlushRawList();
	void FlushAlphaRawList();
	void FlushEmissiveRawList();
	void FlushEffectList();
	void FlushBumpList();
	void FlushLmList();
public:
	float m_hpc;
	float m_vpc;
	float m_hratio;
	float m_vratio;
	float m_aspectRatio;
	float m_screenXFactor;
	float m_screenYFactor;
	int m_xoffset;
	int m_yoffset;
	int m_width;
	int m_height;
	int m_halfWidth;
	int m_halfHeight;
	int m_curFrame;
	int m_bRGBBitCount;
	unsigned long m_fpsFrameCount;
	unsigned long m_fpsStartTick;
	int m_isFoggy;
	int m_fogChanged;
	int m_isVertexFog;
	CTexture* m_oldTexture;
	CTexture* m_oldLmapTexture;
	float m_guardBandLeft;
	float m_guardBandRight;
	float m_guardBandTop;
	float m_guardBandBottom;
	int m_isShowInfo;
	vector3d m_eyeVector;
	unsigned long m_nClearColor;
private:
	friend class C3dDevice;
	IDirect3DDevice7* m_device;
	IDirectDrawSurface7* m_lpSurface;
	unsigned long m_dwScreenWidth;
	unsigned long m_dwScreenHeight;
	PixelFormat m_pf;
	void* m_lpSurfacePtr;
	long m_lPitch;
	struct RPRawTexPrtLess : public std::binary_function<const RPRaw*, const RPRaw*, bool> {
	public:
		bool operator()(const RPRaw*, const RPRaw*) const;
	};

	struct RPFaceTexPrtLess : public std::binary_function<const RPFace*, const RPFace*, bool> {
	public:
		bool operator()(const RPFace*, const RPFace*) const;
	};

	struct RPLmFaceTexPrtLess : public std::binary_function<const RPLmFace*, const RPLmFace*, bool> {
	public:
		bool operator()(const RPLmFace* left, const RPLmFace* right) const
		{
			if (left->tex == right->tex)
				return left->tex2 < right->tex2;
			else
				return left->tex < right->tex;
		}
	};

	typedef std::vector<RPRaw *> rp_raw_map;
	typedef rp_raw_map::iterator rp_raw_map_iter;
	typedef std::vector<RPFace *> rp_tex_map;
	typedef rp_tex_map::iterator rp_tex_map_iter;
	typedef std::vector<RPMesh *> rp_mesh_map;
	typedef rp_mesh_map::iterator rp_mesh_map_iter;
	typedef std::vector<RPLmFace *> rp_lmtex_map;
	typedef rp_lmtex_map::iterator rp_lmtex_map_iter;
	typedef std::pair<float, RPFace *> rp_float_pair;
	typedef std::vector<rp_float_pair> rp_rp_map;
	typedef rp_rp_map::iterator rp_rp_map_iter;
	std::vector<RPFace *> m_rpFaceList;
	std::vector<RPFace *> m_rpLMGroundList;
	std::vector<RPFace *> m_rpLMLightList;
	std::vector<std::pair<float, RPFace *> > m_rpAlphaDepthList;
	std::vector<std::pair<float, RPFace *> > m_rpAlphaList;
	std::vector<std::pair<float, RPFace *> > m_rpAlphaNoDepthList;
	std::vector<std::pair<float, RPFace *> > m_rpEmissiveDepthList;
	std::vector<std::pair<float, RPFace *> > m_rpEmissiveList;
	std::vector<std::pair<float, RPFace *> > m_rpEmissiveNoDepthList;
	std::vector<RPRaw *> m_rpRawList;
	std::vector<RPRaw *> m_rpRawAlphaList;
	std::vector<RPRaw *> m_rpRawEmissiveList;
	std::vector<RPFace *> m_rpAlphaOPList;
	std::vector<RPLmFace *> m_rpLmList;
	std::vector<RPLmFace *> m_rpBumpFaceList;
	typedef std::list<CacheSurface> cache_surface_vector;
	typedef cache_surface_vector::iterator cache_surface_vector_iter;
	typedef std::vector<CacheInfo> cache_info_vector;
	typedef cache_info_vector::iterator cache_info_vector_iter;
	typedef std::list<CTexture *> unused_cache_surface_list;
	typedef unused_cache_surface_list::iterator unused_cache_surface_list_iter;
	std::list<CacheSurface> m_cacheSurfaces[16];
	std::list<CTexture *> m_unusedCacheSurfaces;
	std::list<RPFace> m_rpNullFaceList;
	std::list<RPFace>::iterator m_rpNullFaceListIter;
	std::list<RPQuadFace> m_rpQuadFaceList;
	std::list<RPQuadFace>::iterator m_rpQuadFaceListIter;
	std::list<RPLmQuadFace> m_rpLmQuadFaceList;
	std::list<RPLmQuadFace>::iterator m_rpLmQuadFaceListIter;
	std::vector<tlvertex3d> m_vertBuffer;
};
extern FogPara g_fogPara;

inline UVRECT::UVRECT(float newU1, float newV1, float newU2, float newV2)
	: u1(newU1), v1(newV1), u2(newU2), v2(newV2) {}

extern CRenderer* g_renderer;


#endif
extern CTexMgr g_texMgr;
extern float g_gradient;
extern float g_slope;
extern float g_shadowSlope;
DWORD GetSpecularFromOow(float oow);
float GetZFromOow(float oow);
float	GetOowFromZ(float z);
