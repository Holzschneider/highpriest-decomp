#ifndef HIGHPRIEST_3DLIB_TEXTURE_H
#define HIGHPRIEST_3DLIB_TEXTURE_H

#include "Device.h"

struct SprImg;
class CSurface;
class CTexture;

class CSurface {
	friend class CDCSurface;
public:
	CSurface() : m_pddsSurface(0) {}
	CSurface(unsigned long w, unsigned long h, IDirectDrawSurface7* surface);
	CSurface(unsigned long w, unsigned long h);
	virtual ~CSurface();
	virtual void Update(int x, int y, int width, int height, unsigned long* image, bool blackkey, int lPitch);
	virtual void ClearSurface(tagRECT* rect, unsigned long color);
	virtual void DrawSurface(int x, int y, int width, int height, unsigned long color);
	virtual void DrawSurfaceStretch(int x, int y, int width, int height);
	IDirectDrawSurface7* GetDDSurface() const { return m_pddsSurface; }
	bool Create(unsigned long w, unsigned long h);
protected:
	unsigned long m_w;
	unsigned long m_h;
	IDirectDrawSurface7* m_pddsSurface;
};

class CSurfaceWallpaper : public CSurface {
public:
	CSurfaceWallpaper(unsigned long w, unsigned long h);
	virtual void Update(int x, int y, int width, int height, unsigned long* imgData, bool blackkey, int lPitch);
};

class CTexture : public CSurface {
	friend class CTexMgr;
public:
	virtual void Update(int x, int y, int width, int height, unsigned long* image, bool blackkey, int lPitch);
	void UpdateSprite(int x, int y, int width, int height, SprImg& img, unsigned long* pal);
	virtual void ClearSurface(tagRECT* rect, unsigned long color);
	virtual void DrawSurface(int x, int y, int w, int h, unsigned long color);
	virtual void DrawSurfaceStretch(int x, int y, int w, int h);
	float GetUAdjust() { return float(m_updateWidth) / float(m_w); }
	float GetVAdjust() { return float(m_updateHeight) / float(m_h); }
	PixelFormat GetPixelFormat() const { return m_pf; }
	static void SetUVOffset(float uOffset, float vOffset){ m_uOffset = uOffset; m_vOffset = vOffset; }
	long Lock();
	long Unlock();
	long GetLockCount() const { return m_lockCnt; }
	void UpdateStamp() { extern unsigned long __stdcall timeGetTime(); m_timeStamp = timeGetTime(); }
	void SetName(const char* texName){ strcpy(m_texName, texName); }
	bool CopyTexture(CTexture* srcTex, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
protected:
	CTexture(unsigned long w, unsigned long h, PixelFormat pf, IDirectDrawSurface7* surface);
	CTexture(unsigned long w, unsigned long h, PixelFormat pf);
	bool Create(unsigned long w, unsigned long h, PixelFormat pf);
	bool CreateBump(unsigned long w, unsigned long iDv, IDirectDrawSurface7* h);
	bool CreateBump(unsigned long w, unsigned long iDv);
	void SetUVAdjust(unsigned long width, unsigned long height);
	void UpdateMipmap(tagRECT& src);
	friend class C3dDevice;
	friend class CRenderer;
	static float m_uOffset;
	static float m_vOffset;
	PixelFormat m_pf;
	unsigned char m_blackkey;
	unsigned long m_updateWidth;
	unsigned long m_updateHeight;
	char m_texName[256];
	long m_lockCnt;
	unsigned long m_timeStamp;
};

#endif
