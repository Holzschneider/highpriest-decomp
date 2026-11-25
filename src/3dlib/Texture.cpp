#include "common.h"
#include "Texture.h"
#include "Device.h"
#include "D3dutil.h"
#include "Renderer.h"
#include "../resource/Sprite.h"

#include <windows.h>
#include <ddraw.h>

float CTexture::m_uOffset = 0.5f;
float CTexture::m_vOffset = 0.5f;

extern CRenderer *g_renderer;
extern int g_isMipmap;

// A texture is a texture of port/native/gfx (M11.9), registered with the
// render target (bf_render.h). Its CPU copy, in the texture's own format, is
// what the client's Lock wrote into; mip levels are made by the module after
// every change. The four formats the device offered by EnumTextureFormats are
// the module's: A1R5G5B5, A4R4G4B4, R5G6B5, A8R8G8B8.
static bool GfxFormat(PixelFormat pf, bf_gfx_format* format)
{
	switch (pf) {
	case PF_A1R5G5B5: *format = BF_GFX_FORMAT_A1R5G5B5; return true;
	case PF_A4R4G4B4: *format = BF_GFX_FORMAT_A4R4G4B4; return true;
	case PF_R5G6B5:   *format = BF_GFX_FORMAT_R5G6B5;   return true;
	case PF_A8R8G8B8: *format = BF_GFX_FORMAT_A8R8G8B8; return true;
	default: return false;
	}
}

// DirectDraw's rectangle rule: inside the surface and not empty.
static bool RectInside(const RECT& r, DWORD w, DWORD h)
{
	return r.left >= 0 && r.top >= 0 && r.right > r.left && r.bottom > r.top &&
		   (DWORD)r.right <= w && (DWORD)r.bottom <= h;
}


CSurface :: CSurface(DWORD w, DWORD h)
:	m_pddsSurface(NULL)
,	m_gfxTex(NULL)
{
	Create(w, h);
}

CSurface :: CSurface(DWORD w, DWORD h, IDirectDrawSurface7 * surface)
:	m_w(w), m_h(h), m_pddsSurface(surface), m_gfxTex(NULL)
{
}

CSurface :: ~CSurface()
{
	SAFE_RELEASE(m_pddsSurface);
	bf_render_texture_destroy(m_gfxTex);
	m_gfxTex = NULL;
}

void	CSurface :: Update(int x, int y, int width, int height, DWORD *image, bool blackkey, int lPitch)
{
	if(!m_pddsSurface) return;
	if(width == 0) return;
	if(height == 0) return;

	if(lPitch == 0) lPitch = width;

	RECT rtDest = { x, y, x+width, y+height };
	DWORD argb;
	DDSURFACEDESC2 ddsd = { sizeof(DDSURFACEDESC2), 0 };
	if(FAILED(m_pddsSurface->Lock(&rtDest, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL)))
		return;

	DWORD maskKey = ((0 >> g_3dDevice.GetGShiftL()) << g_3dDevice.GetGShiftR())
				  + ((255 >> g_3dDevice.GetRShiftL()) << g_3dDevice.GetRShiftR())
				  + ((255 >> g_3dDevice.GetBShiftL()) << g_3dDevice.GetBShiftR());

	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16) {
		WORD* surface;
		surface = (WORD*)ddsd.lpSurface;
		for(int i=0; i<height; ++i) {
			for(int j=0; j<width; ++j) {
				argb = image[j];
				DWORD color = ((((argb >> 8) & 0xff) >> g_3dDevice.GetGShiftL()) << g_3dDevice.GetGShiftR())
							+ ((((argb >> 16) & 0xff) >> g_3dDevice.GetRShiftL()) << g_3dDevice.GetRShiftR())
							+ (((int)(argb & 0xff) >> g_3dDevice.GetBShiftL()) << g_3dDevice.GetBShiftR());
				if((WORD)color != (WORD)maskKey)
					surface[j] = (WORD)color;
			}
			surface = (WORD*)((BYTE*)surface + ddsd.lPitch);
			image += lPitch;
		}
	} else {
		DWORD* surface = (DWORD*)ddsd.lpSurface;
		for(int i=0; i<height; ++i) {
			for(int j=0; j<width; ++j) {
				DWORD color = image[j];
				if((color & 0xffffff) != 0xff00ff)
					surface[j] = color;
			}
			surface = (DWORD*)((BYTE*)surface + ddsd.lPitch);
			image += lPitch;
		}
	}

	m_pddsSurface->Unlock(NULL);
}

void	CSurface :: ClearSurface(RECT* rect, DWORD color)
{
	if (!m_pddsSurface) return;

	DDBLTFX bltfx;
	bltfx.dwSize = sizeof(DDBLTFX);

	bltfx.dwFillColor = (((((color >> 8) & 0xff) >> g_3dDevice.GetGShiftL()) << g_3dDevice.GetGShiftR()) + ((((color >> 16) & 0xff) >> g_3dDevice.GetBShiftL()) << g_3dDevice.GetBShiftR()) + (((int)(color & 0xff) >> g_3dDevice.GetRShiftL()) << g_3dDevice.GetRShiftR())) & 0xffff;

	m_pddsSurface->Blt(rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &bltfx);
}

void	CSurface :: DrawSurface(int x, int y, int width, int height, DWORD color)
{
	int rwidth = g_3dDevice.GetWidth();
	RECT r;
	r.right  = width;
	r.left   = 0;
	r.top    = 0;
	r.bottom = height;

	if (x + width > rwidth)
		r.right = rwidth - x;
	int rheight = g_3dDevice.GetHeight();
	if (y + height > rheight)
		r.bottom = rheight - y;

	if (x < 0) { r.left = -x; x = 0; }
	if (y < 0) { r.top  = -y; y = 0; }

	if (r.right > r.left && r.bottom > r.top)
		g_3dDevice.GetRenderSurface()->BltFast(x, y, m_pddsSurface, &r, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY);
}

void	CSurface :: DrawSurfaceStretch(int x, int y, int width, int height)
{
	RECT dest = { 0, 0, width, height };
	g_3dDevice.GetRenderSurface()->Blt(&dest, m_pddsSurface, NULL, DDBLT_WAIT, NULL );
}

bool CSurface::Create(DWORD w, DWORD h)
{
	DDSURFACEDESC2 ddsd;

	m_w = w + ((-(int)(w & 3)) & 3);
	m_h = h + ((-(int)(h & 3)) & 3);

	SAFE_RELEASE(m_pddsSurface);

	ZeroMemory(&ddsd.dwHeight, sizeof(ddsd) - 8);
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_CKSRCBLT;
	ddsd.dwWidth = m_w;
	ddsd.dwHeight = m_h;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.ddpfPixelFormat = g_3dDevice.GetFrameBufferDesc()->ddpfPixelFormat;
	ddsd.ddckCKSrcBlt.dwColorSpaceLowValue =
		ddsd.ddpfPixelFormat.dwBBitMask | ddsd.ddpfPixelFormat.dwRBitMask;
	ddsd.ddckCKSrcBlt.dwColorSpaceHighValue =
		ddsd.ddpfPixelFormat.dwBBitMask | ddsd.ddpfPixelFormat.dwRBitMask;

	if (FAILED(g_3dDevice.GetDirectDraw()->CreateSurface(&ddsd, &m_pddsSurface, NULL)))
		return false;
	return true;
}

CSurfaceWallpaper :: CSurfaceWallpaper(DWORD w, DWORD h)
:	CSurface(w, h)
{
}

void	CSurfaceWallpaper :: Update(int x, int y, int width, int height, DWORD *imgData, bool blackkey, int lPitch)
{
	int w = width;
	int h = height;
	CSurface	imageSurface(w, h);
	imageSurface.Update(0, 0, w, h, imgData, false, 0);
	m_pddsSurface->Blt(NULL, imageSurface.GetDDSurface(), NULL, DDBLT_WAIT, NULL);
}

CTexture :: CTexture(DWORD w, DWORD h, PixelFormat pf)
:	m_lockCnt(0), m_timeStamp(0)
{
	m_texName[0] = 0;

	if( pf==PF_BUMP ) CreateBump(w, h);
	else {
		Create(w, h, pf);
		SetUVAdjust(w, h);
	}
}

CTexture :: CTexture(DWORD w, DWORD h, PixelFormat pf, bf_gfx_texture * surface)
:	m_pf(pf), m_lockCnt(0), m_timeStamp(0)
{
	m_w = w;
	m_h = h;

	if( pf==PF_BUMP ) CreateBump(w, h, surface);
	else {
		m_gfxTex = surface;
	}
}

void	CTexture :: ClearSurface(RECT* rect, DWORD color)
{
	if (!bf_render_texture_valid(m_gfxTex)) return;

	DWORD fillColor = (((((color >> 8) & 0xff) >> g_3dDevice.GetGShiftL()) << g_3dDevice.GetGShiftR()) + ((((color >> 16) & 0xff) >> g_3dDevice.GetBShiftL()) << g_3dDevice.GetBShiftR()) + (((int)(color & 0xff) >> g_3dDevice.GetRShiftL()) << g_3dDevice.GetRShiftR())) & 0xffff;

	RECT whole = { 0, 0, (LONG)m_w, (LONG)m_h };
	RECT r = rect ? *rect : whole;
	if (!RectInside(r, m_w, m_h)) return;
	bf_gfx_rect q = { r.left, r.top, r.right - r.left, r.bottom - r.top };
	bf_gfx_texture_fill(m_gfxTex, &q, fillColor);
}

int	CTexture :: Lock()
{
	UpdateStamp();
	return ++m_lockCnt;
}

int	CTexture :: Unlock()
{
	return --m_lockCnt;
}

void	CTexture :: SetUVAdjust(DWORD width, DWORD height)
{
	while(width > m_w) width = width/2;
	while(height > m_h) height = height/2;

	m_updateWidth = width;
	m_updateHeight = height;
}

void CTexture::UpdateMipmap(tagRECT &src)
{
	// The lower mip levels were made from level 0 by a Blt each; the gfx
	// module makes them after every change of level 0 (bf_gfx.h).
}

void	CTexture :: Update(int x, int y, int width, int height, DWORD *image, bool blackkey, int lPitch)
{
	SetUVAdjust(width, height);
	m_blackkey = blackkey;
	if(!bf_render_texture_valid(m_gfxTex)) return;
	if(image == NULL) return;

	if(lPitch == 0) lPitch = width;

	int iter_x, iter_y;
	for(iter_x = 1; width / iter_x > (int)m_w; ++iter_x);
	for(iter_y = 1; height / iter_y > (int)m_h; ++iter_y);

	x = x / iter_x;
	y = y / iter_y;
	width = width / iter_x;
	height = height / iter_y;

	if(width == 0 || height == 0) return;

	RECT rtDest = { x, y, x+width, y+height };
	if(!RectInside(rtDest, m_w, m_h)) return;

	int pitch;
	BYTE* base = (BYTE*)bf_gfx_texture_lock(m_gfxTex, &pitch);
	if(!base) return;
	int bytes = (m_pf == PF_A8R8G8B8) ? 4 : 2;
	BYTE* lpSurface = base + y * pitch + x * bytes;

	DWORD color;
	WORD* surface;
	DWORD* surface32;
	int i, j, imageIter;
	switch(m_pf)
	{
	case PF_A1R5G5B5:
		surface = (WORD*)lpSurface;
		for(i = 0; i < height; ++i)
		{
			for(j = 0, imageIter = 0; j < width; ++j, imageIter += iter_x)
			{
				color = image[imageIter];
				color = ((color >> 6) & 0x03e0) +
						((color >> 3) & 0x001f) +
						((color >> 16) & 0x8000) +
						((color >> 9) & 0x7c00);

				if(color == 0xfc1f)
					surface[j] = 0;
				else if(m_blackkey && (color & 0x7fff) == 0)
					surface[j] = 0;
				else
					surface[j] = (WORD)color;
			}
			surface = (WORD*)((BYTE*)surface + pitch);
			image += lPitch * iter_y;
		}
		break;
	case PF_A4R4G4B4:
		surface = (WORD*)lpSurface;
		for(i = 0; i < height; ++i)
		{
			for(j = 0, imageIter = 0; j < width; ++j, imageIter += iter_x)
			{
				color = ((image[imageIter] >> 16) & 0xf000) +
						((image[imageIter] >> 12) & 0x0f00) +
						((image[imageIter] >> 8) & 0x00f0) +
						((image[imageIter] >> 4) & 0x000f);

				if((color & 0x0fff) == 0x0f0f)
					surface[j] = 0;
				else if(m_blackkey && (color & 0x0fff) == 0)
					surface[j] = 0;
				else
					surface[j] = (WORD)color;
			}
			surface = (WORD*)((BYTE*)surface + pitch);
			image += lPitch * iter_y;
		}
		break;
	case PF_R5G6B5:
		surface = (WORD*)lpSurface;
		for(i = 0; i < height; ++i)
		{
			for(j = 0, imageIter = 0; j < width; ++j, imageIter += iter_x)
			{
				color = image[imageIter];
				color = ((color >> 5) & 0x07e0) +
						((color >> 3) & 0x001f) +
						((color >> 8) & 0xf800);

				if(!m_blackkey)
					surface[j] = (WORD)color;
				else if(color != 0xf81f)
					surface[j] = (WORD)color;
			}
			surface = (WORD*)((BYTE*)surface + pitch);
			image += lPitch * iter_y;
		}
		break;
	case PF_A8R8G8B8:
		surface32 = (DWORD*)lpSurface;
		for(i = 0; i < height; ++i)
		{
			for(j = 0, imageIter = 0; j < width; ++j, imageIter += iter_x)
			{
				if(m_blackkey && image[imageIter] == 0)
					surface32[j] = 0;
				else
					surface32[j] = image[imageIter];
			}
			surface32 = (DWORD*)((BYTE*)surface32 + pitch);
			image += lPitch * iter_y;
		}
		break;
	}

	bf_gfx_rect changed = { x, y, width, height };
	bf_gfx_texture_unlock(m_gfxTex, &changed);

	RECT srcRect = { 0, 0, m_w, m_h };
	UpdateMipmap(srcRect);
}

void	CTexture :: UpdateSprite(int x, int y, int width, int height, SprImg &img, SYSTEMPALETTE* pal)
{
	if(!bf_render_texture_valid(m_gfxTex))
		return;

	int pitch;
	BYTE* base = (BYTE*)bf_gfx_texture_lock(m_gfxTex, &pitch);
	if(!base)
		return;

	int keyCnt = 0;
	BYTE* src = img.m_8bitImage;
	WORD* surface = (WORD*)(base + pitch * y) + x;
	int i;
	for(i = 0; i < img.height; ++i)
	{
		int j;
		for(j = 0; j < img.width; ++j)
		{
			if(keyCnt)
			{
				surface[j] = 0;
				--keyCnt;
			}
			else
			{
				if(*src)
				{
					surface[j] = (WORD)pal[*src];
				}
				else
				{
					surface[j] = 0;
					keyCnt = *(++src) - 1;
				}
				++src;
			}
		}

		memset(surface+j, 0, (width-j)*sizeof(WORD));

		surface = (WORD*)((BYTE*)surface + pitch);
	}

	for(; i < height; ++i)
	{
		memset(surface, 0, width*sizeof(WORD));

		surface = (WORD*)((BYTE*)surface + pitch);
	}

	bf_gfx_texture_unlock(m_gfxTex, NULL);

	RECT srcRect = { x, y, x+width, y+height };
	UpdateMipmap(srcRect);
}

void	CTexture :: DrawSurface(int x, int y, int w, int h, DWORD color)
{
	vector2d xy[2] = { vector2d(x,y), vector2d(x+w,y+h) };
	vector2d uv[2] = { vector2d(m_uOffset/m_w, m_vOffset/m_h), vector2d((w + m_uOffset)/m_w, (h + m_vOffset)/m_h) };

	RPQuadFace& rpFace = g_renderer->BorrowQuadRP();

	rpFace.tex = this;

	tlvertex3d*	tlvert = rpFace.m_verts;

	tlvert[0] = tlvertex3d(xy[0].x,	xy[0].y, 0.00001f, 0.99999f, color, 0xffffffff, uv[0].x, uv[0].y);
	tlvert[1] = tlvertex3d(xy[1].x, xy[0].y, 0.00001f, 0.99999f, color, 0xffffffff, uv[1].x, uv[0].y);
	tlvert[2] = tlvertex3d(xy[0].x,	xy[1].y, 0.00001f, 0.99999f, color, 0xffffffff, uv[0].x, uv[1].y);
	tlvert[3] = tlvertex3d(xy[1].x,	xy[1].y, 0.00001f, 0.99999f, color, 0xffffffff, uv[1].x, uv[1].y);

	g_renderer->AddRP(&rpFace, RF_UI);
}

void	CTexture :: DrawSurfaceStretch(int x, int y, int w, int h)
{
	vector2d xy[2] = { vector2d(x,y), vector2d(x+w,y+h) };
	vector2d uv[2] = { vector2d(m_uOffset/m_w, m_vOffset/m_h), vector2d((m_updateWidth + m_uOffset)/m_w, (m_updateHeight + m_vOffset)/m_h) };

	RPQuadFace& rpFace = g_renderer->BorrowQuadRP();

	rpFace.tex = this;

	tlvertex3d*	tlvert = rpFace.m_verts;

	tlvert[0] = tlvertex3d(xy[0].x,	xy[0].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[0].x, uv[0].y);
	tlvert[1] = tlvertex3d(xy[1].x, xy[0].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[1].x, uv[0].y);
	tlvert[2] = tlvertex3d(xy[0].x,	xy[1].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[0].x, uv[1].y);
	tlvert[3] = tlvertex3d(xy[1].x,	xy[1].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[1].x, uv[1].y);

	g_renderer->AddRP(&rpFace, RF_UI);
}

bool	CTexture :: Create(DWORD w, DWORD h, PixelFormat pf)
{
	m_w = w;
	m_h = h;
	m_pf = pf;
	g_3dDevice.AdjustTextureSize(&m_w, &m_h);

	bf_render_texture_destroy(m_gfxTex);
	m_gfxTex = NULL;

	if (pf != PF_A1R5G5B5 && pf != PF_A4R4G4B4 && pf != PF_A8R8G8B8 && pf != PF_R5G6B5)
		ErrorMsg("Create Texture::Unsupported pixelformat");

	bf_gfx_format format;
	if (!GfxFormat(pf, &format)) {
		ErrorMsg(bf_tr("GFC \xb8\xa6 \xb1\xb8\xb5\xbf\xc7\xd2 \xbc\xf6 \xbe\xf8\xb4\xc2 "
			"\xba\xf1\xb5\xf0\xbf\xc0\xbd\xc3\xbd\xba\xc5\xdb\xc0\xd4\xb4\xcf\xb4\xd9"));
		bf_tracef(BFT_ERR, "texture: %ux%u in pixel format %d: no such texture format", (unsigned)m_w, (unsigned)m_h, (int)pf);
		return false;
	}

	int mipMapCount = 1;
	while ((1 << mipMapCount) < m_w &&
		   (1 << mipMapCount) < m_h)
		++mipMapCount;

	m_gfxTex = bf_render_texture_create(m_w, m_h, format, g_isMipmap ? mipMapCount : 1);
	if (!m_gfxTex)
		return false;
	return true;
}

// Bump maps (DDPF_BUMPDUDV) are not a format of the gfx module, nor were they
// one the stage-A device offered: as on a card without BUMPENVMAP, the two
// messages, and no texture.
bool	CTexture :: CreateBump(DWORD w, DWORD h)
{
	bf_port_alert("notsuppot", "D3DTEXOPCAPS_BUMPENVMAP", BF_ALERT_INFO, BF_ALERT_BUTTONS_OK);
	bf_port_alert("notsuppot", "D3DTEXOPCAPS_BUMPENVMAPLUMINANCE", BF_ALERT_INFO, BF_ALERT_BUTTONS_OK);

	m_w = w;
	m_h = h;
	m_pf = PF_BUMP;
	g_3dDevice.AdjustTextureSize(&m_w, &m_h);
	bf_render_texture_destroy(m_gfxTex);
	m_gfxTex = NULL;

	bf_tracef(BFT_GFX, "texture: no bump-map format (WATERBUMPMAP %ux%u)", (unsigned)m_w, (unsigned)m_h);
	return false;
}

bool	CTexture :: CreateBump(DWORD w, DWORD h, bf_gfx_texture * pSurface)
{
	bf_port_alert("notsuppot", "D3DTEXOPCAPS_BUMPENVMAP", BF_ALERT_INFO, BF_ALERT_BUTTONS_OK);
	bf_port_alert("notsuppot", "D3DTEXOPCAPS_BUMPENVMAPLUMINANCE", BF_ALERT_INFO, BF_ALERT_BUTTONS_OK);

	m_w = w;
	m_h = h;
	m_pf = PF_BUMP;
	g_3dDevice.AdjustTextureSize(&m_w, &m_h);
	bf_render_texture_destroy(m_gfxTex);
	m_gfxTex = NULL;

	if (pSurface == NULL)
		return false;

	bf_tracef(BFT_GFX, "texture: no bump-map format (WATERBUMPMAP %ux%u from a height map)", (unsigned)m_w, (unsigned)m_h);
	return false;
}

bool	CTexture :: CopyTexture( CTexture *srcTex, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh )
{
	RECT srcRect = { sx, sy, sx + sw, sy + sh };
	RECT dstRect = { dx, dy, dx + dw, dy + dh };

	if (!bf_render_texture_valid(m_gfxTex) || !bf_render_texture_valid(srcTex->m_gfxTex))
		return false;
	if (!RectInside(dstRect, m_w, m_h) || !RectInside(srcRect, srcTex->m_w, srcTex->m_h))
		return false;

	bf_gfx_rect a = { dx, dy, dw, dh }, b = { sx, sy, sw, sh };
	if(bf_gfx_texture_copy(m_gfxTex, &a, srcTex->m_gfxTex, &b) == BF_GFX_OK) {
		RECT rect = { 0, 0, m_w, m_h };
		UpdateMipmap(rect);
		return true;
	}
	return false;
}
