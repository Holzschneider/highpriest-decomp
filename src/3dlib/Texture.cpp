#include "common.h"
#include "Texture.h"
#include "Device.h"
#include "D3dutil.h"
#include "Renderer.h"
#include "../resource/Sprite.h"

#include "bf_types.h"

#include <vector>

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


// ---- surfaces (M11.9) -------------------------------------------------------
// The pixel rules of the DirectDraw surfaces of stage A (port/compat/d3d):
// a surface reads and writes as 0xAARRGGBB, channels widened by bit
// replication; a format without alpha reads alpha 255. A Blt that is not a
// fast path copies through ARGB with nearest stretching and the source
// colour key. The render target has no colour key; an offscreen surface's is
// magenta (CSurface::Create); a texture's only in R5G6B5 (CTexture::Create).

struct SurfaceFormat {
	int bytes;
	DWORD a, r, g, b;
};

static const SurfaceFormat FRAMEBUFFER_FORMAT = { 4, 0, 0x00FF0000, 0x0000FF00, 0x000000FF };

static SurfaceFormat FormatOf(const CSurface* s, PixelFormat pf)
{
	SurfaceFormat f = FRAMEBUFFER_FORMAT;
	if (!s->GetGfxTexture())
		return f;
	switch (pf) {
	case PF_A1R5G5B5: { SurfaceFormat t = { 2, 0x8000, 0x7C00, 0x03E0, 0x001F }; return t; }
	case PF_A4R4G4B4: { SurfaceFormat t = { 2, 0xF000, 0x0F00, 0x00F0, 0x000F }; return t; }
	case PF_R5G6B5:   { SurfaceFormat t = { 2, 0, 0xF800, 0x07E0, 0x001F }; return t; }
	default:          { SurfaceFormat t = { 4, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF }; return t; }
	}
}

static int MaskShift(DWORD m) { int n = 0; if (!m) return 0; while (!(m & 1)) { m >>= 1; ++n; } return n; }
static int MaskBits(DWORD m) { int n = 0; while (m) { n += (int)(m & 1); m >>= 1; } return n; }

// v has n significant bits; repeat them to fill 8 bits (v << 3 | v >> 2 for 5).
static DWORD Widen(DWORD v, int n)
{
	if (n <= 0) return 0;
	if (n >= 8) return (v >> (n - 8)) & 0xFF;
	DWORD out = 0;
	for (int filled = 0; filled < 8; filled += n) {
		int shift = 8 - filled - n;
		out |= shift >= 0 ? v << shift : v >> -shift;
	}
	return out & 0xFF;
}

static DWORD ChannelIn(DWORD raw, DWORD mask) { return Widen((raw & mask) >> MaskShift(mask), MaskBits(mask)); }
static DWORD ChannelOut(DWORD c8, DWORD mask)
{
	int n = MaskBits(mask);
	if (!n) return 0;
	DWORD v = n >= 8 ? c8 << (n - 8) : c8 >> (8 - n);
	return (v << MaskShift(mask)) & mask;
}

static DWORD ToArgb(DWORD raw, const SurfaceFormat& f)
{
	DWORD a = f.a ? ChannelIn(raw, f.a) : 255;
	return a << 24 | ChannelIn(raw, f.r) << 16 | ChannelIn(raw, f.g) << 8 | ChannelIn(raw, f.b);
}

static DWORD FromArgb(DWORD argb, const SurfaceFormat& f)
{
	DWORD v = ChannelOut((argb >> 16) & 0xFF, f.r) | ChannelOut((argb >> 8) & 0xFF, f.g) | ChannelOut(argb & 0xFF, f.b);
	if (f.a) v |= ChannelOut(argb >> 24, f.a);
	return v;
}

static DWORD LoadRaw(const BYTE* p, int bytes) { return bytes == 2 ? *(const WORD*)p : *(const DWORD*)p; }
static void StoreRaw(BYTE* p, int bytes, DWORD v) { if (bytes == 2) *(WORD*)p = (WORD)v; else *(DWORD*)p = v; }

// The source colour key of the surface, if it has one: its low value (= high).
static bool ColorKey(const CSurface* s, PixelFormat pf, DWORD* key)
{
	if (s->GetGfxTexture()) {
		*key = 0xF81F;   // DDSD_CKSRCBLT of an R5G6B5 texture: its B | R masks
		return pf == PF_R5G6B5;
	}
	*key = 0x00FF00FF;   // an offscreen surface's: B | R masks
	return true;
}

static bool KeyMatch(DWORD raw, const SurfaceFormat& f, DWORD key)
{
	DWORD m = f.r | f.g | f.b;
	if (f.bytes < 4) m &= (1u << (f.bytes * 8)) - 1;
	return (raw & m) == (key & m);
}

static PixelFormat SurfacePixelFormat(const CSurface* s)
{
	return s->GetGfxTexture() ? static_cast<const CTexture*>(s)->GetPixelFormat() : PF_A8R8G8B8;
}

// Reads a rectangle as 0xAARRGGBB; keyed (optional) receives 1 for pixels
// that match the source colour key.
bool BfReadSurface(const CSurface* s, const RECT& r, DWORD* out, BYTE* keyed)
{
	int w = r.right - r.left, h = r.bottom - r.top;
	if (keyed) memset(keyed, 0, w * h);
	if (s->m_isTarget) {
		bf_gfx_rect q = { r.left, r.top, w, h };
		return bf_render_read_pixels(&q, out, 0) == BF_GFX_OK;   // the target has no colour key
	}
	PixelFormat pf = SurfacePixelFormat(s);
	SurfaceFormat f = FormatOf(s, pf);
	DWORD key;
	bool hasKey = ColorKey(s, pf, &key);
	const BYTE* base;
	int pitch;
	if (s->m_bits) {
		base = (const BYTE*)s->m_bits;
		pitch = s->m_w * 4;
	} else if (bf_render_texture_valid(s->m_gfxTex)) {
		base = (const BYTE*)bf_gfx_texture_lock(s->m_gfxTex, &pitch);
		if (!base) return false;
	} else {
		return false;
	}
	for (int y = 0; y < h; ++y) {
		const BYTE* row = base + (r.top + y) * pitch + r.left * f.bytes;
		for (int x = 0; x < w; ++x) {
			DWORD raw = LoadRaw(row + x * f.bytes, f.bytes);
			out[y * w + x] = ToArgb(raw, f);
			if (keyed && hasKey) keyed[y * w + x] = KeyMatch(raw, f, key);
		}
	}
	if (!s->m_bits) {
		bf_gfx_rect none = { 0, 0, 0, 0 };
		bf_gfx_texture_unlock(s->m_gfxTex, &none);
	}
	return true;
}

// Writes 0xAARRGGBB pixels into a rectangle; skip (optional) leaves pixels
// with a non-zero entry untouched.
bool BfWriteSurface(CSurface* s, const RECT& r, const DWORD* px, const BYTE* skip)
{
	int w = r.right - r.left, h = r.bottom - r.top;
	if (s->m_isTarget) {
		std::vector<DWORD> tmp(px, px + w * h);
		for (size_t i = 0; i < tmp.size(); ++i)
			tmp[i] = (skip && skip[i]) ? 0 : (tmp[i] | 0xFF000000u);
		bf_gfx_rect q = { r.left, r.top, w, h };
		return bf_render_write_pixels(&q, &tmp[0], 0, skip ? BF_GFX_WRITE_SKIP_TRANSPARENT : 0) == BF_GFX_OK;
	}
	PixelFormat pf = SurfacePixelFormat(s);
	SurfaceFormat f = FormatOf(s, pf);
	BYTE* base;
	int pitch;
	if (s->m_bits) {
		base = (BYTE*)s->m_bits;
		pitch = s->m_w * 4;
	} else if (bf_render_texture_valid(s->m_gfxTex)) {
		base = (BYTE*)bf_gfx_texture_lock(s->m_gfxTex, &pitch);
		if (!base) return false;
	} else {
		return false;
	}
	for (int y = 0; y < h; ++y) {
		BYTE* row = base + (r.top + y) * pitch + r.left * f.bytes;
		for (int x = 0; x < w; ++x) {
			if (skip && skip[y * w + x]) continue;
			StoreRaw(row + x * f.bytes, f.bytes, FromArgb(px[y * w + x], f));
		}
	}
	if (!s->m_bits) {
		bf_gfx_rect q = { r.left, r.top, w, h };
		bf_gfx_texture_unlock(s->m_gfxTex, &q);
	}
	return true;
}

static void SurfaceSize(const CSurface* s, DWORD* w, DWORD* h)
{
	if (s->GetGfxTexture() || !s->IsRenderTarget()) {
		*w = s->GetSurfaceWidth();
		*h = s->GetSurfaceHeight();
	} else {
		int tw, th;
		bf_render_size(&tw, &th);
		*w = tw;
		*h = th;
	}
}

// Blt and BltFast between two surfaces that no fast path covers: the source
// as ARGB, nearest stretch, the source colour key, into the destination.
bool BfBltSurface(CSurface* dst, const RECT& dr, const CSurface* src, const RECT& sr, bool keysrc)
{
	DWORD dw0, dh0, sw0, sh0;
	SurfaceSize(dst, &dw0, &dh0);
	SurfaceSize(src, &sw0, &sh0);
	if (!RectInside(dr, dw0, dh0) || !RectInside(sr, sw0, sh0))
		return false;
	int dw = dr.right - dr.left, dh = dr.bottom - dr.top, sw = sr.right - sr.left, sh = sr.bottom - sr.top;
	std::vector<DWORD> spx(sw * sh);
	std::vector<BYTE> skey(sw * sh);
	if (!BfReadSurface(src, sr, &spx[0], keysrc ? &skey[0] : NULL))
		return false;
	std::vector<DWORD> dpx(dw * dh);
	std::vector<BYTE> dskip(dw * dh);
	for (int y = 0; y < dh; ++y) {
		int sy = (int)((long long)y * sh / dh);
		for (int x = 0; x < dw; ++x) {
			int sx = (int)((long long)x * sw / dw);
			dpx[y * dw + x] = spx[sy * sw + sx];
			dskip[y * dw + x] = keysrc ? skey[sy * sw + sx] : 0;
		}
	}
	return BfWriteSurface(dst, dr, &dpx[0], keysrc ? &dskip[0] : NULL);
}

CSurface :: CSurface(DWORD w, DWORD h)
:	m_isTarget(0)
,	m_bits(NULL)
,	m_gfxTex(NULL)
,	m_dcBits(NULL)
{
	Create(w, h);
}

CSurface :: CSurface(DWORD w, DWORD h, int renderTarget)
:	m_w(w), m_h(h), m_isTarget(renderTarget), m_bits(NULL), m_gfxTex(NULL), m_dcBits(NULL)
{
}

CSurface :: ~CSurface()
{
	delete [] m_bits;
	m_bits = NULL;
	delete [] m_dcBits;
	m_dcBits = NULL;
	bf_render_texture_destroy(m_gfxTex);
	m_gfxTex = NULL;
}

void	CSurface :: Update(int x, int y, int width, int height, DWORD *image, bool blackkey, int lPitch)
{
	if(!m_isTarget && !m_bits) return;
	if(width == 0) return;
	if(height == 0) return;

	if(lPitch == 0) lPitch = width;

	RECT rtDest = { x, y, x+width, y+height };
	DWORD sw, sh;
	SurfaceSize(this, &sw, &sh);
	if(!RectInside(rtDest, sw, sh))
		return;

	// 32-bit: every pixel but the magenta key is written.
	if(m_bits) {
		DWORD* surface = (DWORD*)m_bits + y * m_w + x;
		for(int i=0; i<height; ++i) {
			for(int j=0; j<width; ++j) {
				DWORD color = image[j];
				if((color & 0xffffff) != 0xff00ff)
					surface[j] = color;
			}
			surface += m_w;
			image += lPitch;
		}
	} else {
		std::vector<DWORD> px(width * height);
		std::vector<BYTE> skip(width * height);
		for(int i=0; i<height; ++i) {
			for(int j=0; j<width; ++j) {
				DWORD color = image[j];
				px[i * width + j] = color;
				skip[i * width + j] = (color & 0xffffff) == 0xff00ff;
			}
			image += lPitch;
		}
		BfWriteSurface(this, rtDest, &px[0], &skip[0]);
	}
}

void	CSurface :: ClearSurface(RECT* rect, DWORD color)
{
	if (!m_isTarget && !m_bits) return;

	DWORD fillColor = (((((color >> 8) & 0xff) >> g_3dDevice.GetGShiftL()) << g_3dDevice.GetGShiftR()) + ((((color >> 16) & 0xff) >> g_3dDevice.GetBShiftL()) << g_3dDevice.GetBShiftR()) + (((int)(color & 0xff) >> g_3dDevice.GetRShiftL()) << g_3dDevice.GetRShiftR())) & 0xffff;

	DWORD sw, sh;
	SurfaceSize(this, &sw, &sh);
	RECT whole = { 0, 0, (LONG)sw, (LONG)sh };
	RECT r = rect ? *rect : whole;
	if (!RectInside(r, sw, sh)) return;

	// A colour fill with the raw value in the surface's format (X8R8G8B8).
	if (m_isTarget) {
		bf_gfx_rect q = { r.left, r.top, r.right - r.left, r.bottom - r.top };
		bf_render_clear(BF_GFX_CLEAR_COLOR, ToArgb(fillColor, FRAMEBUFFER_FORMAT), 1.0f, &q);
	} else {
		for (LONG i = r.top; i < r.bottom; ++i)
			for (LONG j = r.left; j < r.right; ++j)
				m_bits[i * m_w + j] = fillColor;
	}
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

	if (r.right > r.left && r.bottom > r.top) {
		// BltFast onto the render target with the source colour key.
		CSurface target(g_3dDevice.GetWidth(), g_3dDevice.GetHeight(), TRUE);
		RECT d = { x, y, x + (r.right - r.left), y + (r.bottom - r.top) };
		BfBltSurface(&target, d, this, r, true);
	}
}

void	CSurface :: DrawSurfaceStretch(int x, int y, int width, int height)
{
	RECT dest = { 0, 0, width, height };
	DWORD sw, sh;
	SurfaceSize(this, &sw, &sh);
	RECT src = { 0, 0, (LONG)sw, (LONG)sh };
	CSurface target(g_3dDevice.GetWidth(), g_3dDevice.GetHeight(), TRUE);
	BfBltSurface(&target, dest, this, src, false);
}

bool CSurface::Create(DWORD w, DWORD h)
{
	m_w = w + ((-(int)(w & 3)) & 3);
	m_h = h + ((-(int)(h & 3)) & 3);

	// An offscreen surface in the frame buffer's format (32-bit X8R8G8B8),
	// in memory, zeroed; its source colour key is magenta.
	delete [] m_bits;
	m_bits = NULL;
	if (m_w == 0 || m_h == 0)
		return false;
	m_bits = new DWORD[m_w * m_h];
	memset(m_bits, 0, m_w * m_h * 4);
	return true;
}

bool CSurface :: GetDC(bf_text_canvas** canvas)
{
	// The DC is a bf_text_canvas (port/native/text) over a 32-bit staging
	// bitmap, pre-filled with opaque pixels (alpha 255); the client's DrawDC
	// draws on it with bf_text_canvas_draw, which writes 0x00RRGGBB, so the
	// pixels with alpha 0 inside the canvas's drawn rectangle are exactly
	// what was drawn, and on ReleaseDC only they go into the surface.
	// Nothing is read back, not even from the render target.
	*canvas = NULL;
	if (m_dcBits || (!m_isTarget && !m_bits && !bf_render_texture_valid(m_gfxTex)))
		return false;
	DWORD w, h;
	SurfaceSize(this, &w, &h);
	if (m_isTarget && bf_render_is_locked())
		return false;
	m_dcBits = new DWORD[w * h];
	for (DWORD i = 0; i < w * h; ++i)
		m_dcBits[i] = 0xFF000000u;
	memset(&m_dcCanvas, 0, sizeof(m_dcCanvas));   // a fresh DC: black text, nothing drawn
	m_dcCanvas.bitmap.pixels = m_dcBits;
	m_dcCanvas.bitmap.width = (int)w;
	m_dcCanvas.bitmap.height = (int)h;
	m_dcCanvas.bitmap.stride = (int)w * 4;
	m_dcCanvas.bitmap.bottom_up = 0;
	*canvas = &m_dcCanvas;
	return true;
}

void CSurface :: ReleaseDC(bf_text_canvas* canvas)
{
	if (!m_dcBits || canvas != &m_dcCanvas)
		return;
	DWORD w, h;
	SurfaceSize(this, &w, &h);
	RECT d = { m_dcCanvas.left, m_dcCanvas.top, m_dcCanvas.right, m_dcCanvas.bottom };
	if (m_dcCanvas.dirty && d.right > d.left && d.bottom > d.top) {
		if (d.left < 0) d.left = 0;
		if (d.top < 0) d.top = 0;
		if (d.right > (LONG)w) d.right = w;
		if (d.bottom > (LONG)h) d.bottom = h;
		int dw = d.right - d.left, dh = d.bottom - d.top;
		if (dw > 0 && dh > 0) {
			std::vector<DWORD> px(dw * dh);
			std::vector<BYTE> skip(dw * dh);
			for (int y = 0; y < dh; ++y)
				for (int x = 0; x < dw; ++x) {
					DWORD v = m_dcBits[(d.top + y) * w + (d.left + x)];
					skip[y * dw + x] = (v >> 24) != 0;
					px[y * dw + x] = v | 0xFF000000u;
				}
			BfWriteSurface(this, d, &px[0], &skip[0]);
		}
	}
	delete [] m_dcBits;
	m_dcBits = NULL;
	memset(&m_dcCanvas, 0, sizeof(m_dcCanvas));
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
	RECT all = { 0, 0, (LONG)m_w, (LONG)m_h };
	RECT src = { 0, 0, (LONG)imageSurface.GetSurfaceWidth(), (LONG)imageSurface.GetSurfaceHeight() };
	BfBltSurface(this, all, &imageSurface, src, false);
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
