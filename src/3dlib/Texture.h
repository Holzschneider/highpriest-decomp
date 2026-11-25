#ifndef HIGHPRIEST_3DLIB_TEXTURE_H
#define HIGHPRIEST_3DLIB_TEXTURE_H

#include "Device.h"
#include "bf_text.h"

struct SprImg;
class CSurface;
class CTexture;

class CSurface {
	friend class CDCSurface;
public:
	CSurface() : m_isTarget(0), m_bits(0), m_gfxTex(0), m_dcBits(0) {}
	CSurface(unsigned int w, unsigned int h, int renderTarget);   // the render target (M11.9)
	CSurface(unsigned int w, unsigned int h);
	virtual ~CSurface();
	virtual void Update(int x, int y, int width, int height, unsigned int* image, bool blackkey, int lPitch);
	virtual void ClearSurface(tagRECT* rect, unsigned int color);
	virtual void DrawSurface(int x, int y, int width, int height, unsigned int color);
	virtual void DrawSurfaceStretch(int x, int y, int width, int height);
	bf_gfx_texture* GetGfxTexture() const { return m_gfxTex; }   // a texture's (CTexture), else NULL
	int IsRenderTarget() const { return m_isTarget; }
	bool Create(unsigned int w, unsigned int h);
	// The surface's DC (CDCSurface, M11.9): a canvas over a staging bitmap;
	// what is drawn on it goes into the surface on ReleaseDC.
	bool GetDC(bf_text_canvas** canvas);
	void ReleaseDC(bf_text_canvas* canvas);
#if defined(BF_PORT)
	unsigned int GetSurfaceWidth() const { return m_w; }    // the UI pass's texel test (Renderer.cpp)
	unsigned int GetSurfaceHeight() const { return m_h; }
#endif
protected:
	unsigned int m_w;
	unsigned int m_h;
	// What a DirectDraw surface was (M11.9): the render target (the gfx
	// target), an offscreen surface (32-bit X8R8G8B8 in memory, source colour
	// key magenta), or a texture of port/native/gfx (CTexture).
	int m_isTarget;
	unsigned int* m_bits;
	bf_gfx_texture* m_gfxTex;
	unsigned int* m_dcBits;
	bf_text_canvas m_dcCanvas;
	friend bool BfReadSurface(const CSurface* s, const tagRECT& r, unsigned int* out, unsigned char* keyed);
	friend bool BfWriteSurface(CSurface* s, const tagRECT& r, const unsigned int* px, const unsigned char* skip);
	friend bool BfBltSurface(CSurface* dst, const tagRECT& dr, const CSurface* src, const tagRECT& sr, bool keysrc);
};

// The pixel paths of the surfaces (Texture.cpp, M11.9).
bool BfReadSurface(const CSurface* s, const tagRECT& r, unsigned int* out, unsigned char* keyed);
bool BfWriteSurface(CSurface* s, const tagRECT& r, const unsigned int* px, const unsigned char* skip);
bool BfBltSurface(CSurface* dst, const tagRECT& dr, const CSurface* src, const tagRECT& sr, bool keysrc);

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
	void UpdateStamp() { m_timeStamp = bf_clock_ticks(); }
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
