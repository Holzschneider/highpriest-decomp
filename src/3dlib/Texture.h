#ifndef HIGHPRIEST_3DLIB_TEXTURE_H
#define HIGHPRIEST_3DLIB_TEXTURE_H

#include "Device.h"

struct SprImg;
class CSurface;
class CTexture;

class CSurface {
	friend class CDCSurface;
public:
	CSurface() : m_pddsSurface(0), m_gfxTex(0) {}
	CSurface(unsigned int w, unsigned int h, IDirectDrawSurface7* surface);
	CSurface(unsigned int w, unsigned int h);
	virtual ~CSurface();
	virtual void Update(int x, int y, int width, int height, unsigned int* image, bool blackkey, int lPitch);
	virtual void ClearSurface(tagRECT* rect, unsigned int color);
	virtual void DrawSurface(int x, int y, int width, int height, unsigned int color);
	virtual void DrawSurfaceStretch(int x, int y, int width, int height);
	IDirectDrawSurface7* GetDDSurface() const { return m_pddsSurface; }
	bf_gfx_texture* GetGfxTexture() const { return m_gfxTex; }   // a texture's (CTexture), else NULL
	bool Create(unsigned int w, unsigned int h);
#if defined(BF_PORT)
	unsigned int GetSurfaceWidth() const { return m_w; }    // the UI pass's texel test (Renderer.cpp)
	unsigned int GetSurfaceHeight() const { return m_h; }
#endif
protected:
	unsigned int m_w;
	unsigned int m_h;
	IDirectDrawSurface7* m_pddsSurface;
	bf_gfx_texture* m_gfxTex;
};

class CSurfaceWallpaper : public CSurface {
public:
	CSurfaceWallpaper(unsigned int w, unsigned int h);
	virtual void Update(int x, int y, int width, int height, unsigned int* imgData, bool blackkey, int lPitch);
};

class CTexture : public CSurface {
	friend class CTexMgr;
public:
	virtual void Update(int x, int y, int width, int height, unsigned int* image, bool blackkey, int lPitch);
	void UpdateSprite(int x, int y, int width, int height, SprImg& img, unsigned int* pal);
	virtual void ClearSurface(tagRECT* rect, unsigned int color);
	virtual void DrawSurface(int x, int y, int w, int h, unsigned int color);
	virtual void DrawSurfaceStretch(int x, int y, int w, int h);
	float GetUAdjust() { return float(m_updateWidth) / float(m_w); }
	float GetVAdjust() { return float(m_updateHeight) / float(m_h); }
	PixelFormat GetPixelFormat() const { return m_pf; }
	static void SetUVOffset(float uOffset, float vOffset){ m_uOffset = uOffset; m_vOffset = vOffset; }
	int Lock();
	int Unlock();
	int GetLockCount() const { return m_lockCnt; }
	void UpdateStamp() { extern unsigned int __stdcall bf_clock_ticks(); m_timeStamp = bf_clock_ticks(); }
	void SetName(const char* texName){ strcpy(m_texName, texName); }
	bool CopyTexture(CTexture* srcTex, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
protected:
	CTexture(unsigned int w, unsigned int h, PixelFormat pf, bf_gfx_texture* surface);
	CTexture(unsigned int w, unsigned int h, PixelFormat pf);
	bool Create(unsigned int w, unsigned int h, PixelFormat pf);
	bool CreateBump(unsigned int w, unsigned int iDv, bf_gfx_texture* h);
	bool CreateBump(unsigned int w, unsigned int iDv);
	void SetUVAdjust(unsigned int width, unsigned int height);
	void UpdateMipmap(tagRECT& src);
	friend class C3dDevice;
	friend class CRenderer;
	static float m_uOffset;
	static float m_vOffset;
	PixelFormat m_pf;
	unsigned char m_blackkey;
	unsigned int m_updateWidth;
	unsigned int m_updateHeight;
	char m_texName[256];
	int m_lockCnt;
	unsigned int m_timeStamp;
};

#endif
