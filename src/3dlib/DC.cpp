#include "common.h"
#include "Renderer.h"
#include "Device.h"
#include "../resource/Action.h"
#include "../resource/Sprite.h"

#include <windows.h>
#include <ddraw.h>

typedef std::list<CTexture *>::iterator     texListIter;


#ifndef INVERT_COLOR
#define INVERT_COLOR(color)     ((color&0xFF000000) + ((color&0xFF)<<16) + (color&0xFF00) + ((color>>16)&0xFF))
#endif
#ifndef PACK555TO32
#define PACK555TO32(argb)       ( 0xFF000000 + ((argb&0x7C00)<<9) + ((argb&0x03E0)<<6) + ((argb&0x0001F)<<3) )
#endif




CDCSurface :: CDCSurface(DWORD w, DWORD h)
:	m_surface(w,h)
{
}


CDCSurface :: CDCSurface(DWORD w, DWORD h, IDirectDrawSurface7 * surface)
:	m_surface(w,h, surface)
{
}

CDCSurface :: ~CDCSurface()
{
}

bool	CDCSurface :: GetDC(HDC__** hdc)
{
	IDirectDrawSurface7 * pddsSurface = m_surface.GetDDSurface();

	if(pddsSurface)
		return SUCCEEDED(pddsSurface->GetDC(hdc)) ? true : false;
	else
		return false;
}

void	CDCSurface :: ReleaseDC(HDC__* hdc)
{
	if (m_surface.GetDDSurface())
		m_surface.GetDDSurface()->ReleaseDC(hdc);
}

void	CDCSurface :: ClearSurface(tagRECT* rect, DWORD color)
{
	m_surface.ClearSurface(rect, color);
}

void	CDCSurface :: Resize(int w, int h)
{
	if((DWORD)w > m_surface.m_w || (DWORD)h > m_surface.m_h)
		m_surface.Create(w, h);
}

void	CDCSurface :: CopyRect(int x, int y, int width, int height, CDC* surface)
{
	RECT r;
	r.left = 0;
	r.top = 0;
	r.right = width;
	r.bottom = height;

	m_surface.GetDDSurface()->BltFast(x, y, ((CDCSurface*)surface)->m_surface.GetDDSurface(), &r, DDBLTFAST_SRCCOLORKEY|DDBLTFAST_WAIT);
}

void	CDCSurface :: BltTexture2(int x, int y, CTexture *src, int srcx, int srcy, int width, int height, int xflip, int zoomx, int zoomy)
{
	if(!src) return;

	IDirectDrawSurface7 *	pddsSurface = m_surface.GetDDSurface();
	if(!pddsSurface) return;

	int						i, j;
	IDirectDrawSurface7 *	srcSurface;
	WORD					*lpSrcSurfacePtr;
	int						lSrcPitch;
	int						r, g, b;
	DWORD					color;
	WORD					*lpDestSurfacePtr;
	int						lDestPitch;

	srcSurface = src->GetDDSurface();
	if(!srcSurface) return;

	DDSURFACEDESC2	srcDesc = { sizeof(DDSURFACEDESC2), 0 };
	if(FAILED(srcSurface->Lock(NULL, &srcDesc, DDLOCK_WAIT, NULL))) return;

	lpSrcSurfacePtr = (WORD*)srcDesc.lpSurface;
	lSrcPitch = srcDesc.lPitch / 2;

	DDSURFACEDESC2	destDesc = { sizeof(DDSURFACEDESC2), 0 };
	if(FAILED(pddsSurface->Lock(NULL, &destDesc, DDLOCK_WAIT, NULL))) {
		srcSurface->Unlock(NULL);
		return;
	}

	lpDestSurfacePtr = (WORD*)destDesc.lpSurface;
	lDestPitch = destDesc.lPitch / 2;

	if(src->GetPixelFormat() == PF_A1R5G5B5) {
		if(xflip) {
				for(i=0; i<height; i++) {
					for(j=width-1; j>=0; j--) {
					color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));
					if(color&0x8000) {
						r = (int(0x7c00 & color) >> 7);
						g = (int(0x03e0 & color) >> 2);
						b = (int(0x001f & color) << 3);
						for(int zy=0; zy<zoomy; ++zy) {
							for(int zx=0; zx<zoomx; ++zx) {
								color = g_3dDevice.EncodePixel(r, g, b);
								*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + ((width-1)-j)*zoomx + zx)) = WORD(color);
							}
						}
					}
				}
			}
		} else {
				for(i=0; i<height; i++) {
					for(j=0; j<width; j++) {
					color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));
					if(color&0x8000) {
						r = (int(0x7c00 & color) >> 7);
						g = (int(0x03e0 & color) >> 2);
						b = (int(0x001f & color) << 3);
						for(int zy=0; zy<zoomy; ++zy) {
							for(int zx=0; zx<zoomx; ++zx) {
								if(lDestPitch * (y + i*zoomy + zy) + (x + j*zoomx + zx) > 0) {
									color = g_3dDevice.EncodePixel(r, g, b);
									*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + j*zoomx + zx)) = WORD(color);
								}
							}
						}
					}
				}
			}
		}
	}

	pddsSurface->Unlock(NULL);
	srcSurface->Unlock(NULL);
}

void	CDCSurface :: Update(int x, int y, int width, int height, DWORD *image, bool SkipColorKey)
{

	m_surface.Update(x, y, width, height, image, SkipColorKey, 0);
}

void	CDCSurface :: DrawSurface(int x, int y, int width, int height, DWORD color)
{
	m_surface.DrawSurface(x, y, width, height, color);
}

CDCBitmap :: CDCBitmap(DWORD w, DWORD h)
{
	m_dc = CreateCompatibleDC(NULL);

	sAssert(m_dc != NULL);

	CreateDCSurface(w, h);
}

CDCBitmap :: ~CDCBitmap()
{
	for(texListIter iter = m_textureList.begin(); iter != m_textureList.end(); ++iter) delete (*iter);
	m_textureList.clear();
	SelectObject(m_dc, m_bitmapOld);
	DeleteObject(m_bitmap);
	DeleteDC(m_dc);
}

bool	CDCBitmap :: GetDC(HDC__** hdc)
{
	m_dirty = true;

	*hdc =  m_dc;

	return true;
}

void	CDCBitmap :: ReleaseDC(HDC__* hdc)
{
	sAssert(m_dc == hdc);
}

void	CDCBitmap :: ClearSurface(tagRECT* rect, DWORD color)
{
	if(m_image == NULL) return;

	m_dirty = true;

	DWORD fillColor = INVERT_COLOR(color);

	if(rect) {
		sAssert(rect->left >= 0 && (DWORD)rect->left <= m_w);
		sAssert(rect->right >= 0 && (DWORD)rect->right <= m_w);
		sAssert(rect->top >= 0 && (DWORD)rect->top <= m_h);
		sAssert(rect->bottom >= 0 && (DWORD)rect->bottom <= m_h);

		for(int i=rect->top; i<rect->bottom; ++i) {
			for(int j=rect->left; j<rect->right; ++j) {
				m_image[i*m_w + j] = fillColor;
			}
		}
	} else {
		for(int i=0; i<m_h; ++i) {
			for(int j=0; j<m_w; ++j) {
				m_image[i*m_w + j] = fillColor;
			}
		}
	}
}

void	CDCBitmap :: CopyRect(int x, int y, int width, int height, CDC* surface)
{
	if(m_image == NULL) return;
	if((DWORD)x >= m_w) return;
	if((DWORD)y >= m_h) return;
	if((DWORD)(x+width) > m_w) width = m_w - x;
	if((DWORD)(y+height) > m_h) height = m_h - y;

	m_dirty = true;

	DWORD* dest = m_image + y*m_w + x;
	DWORD* src = ((CDCBitmap*)surface)->m_image;
	int srcPitch = ((CDCBitmap*)surface)->m_w;

	for(int i=0; i<height; ++i)
	{
		if ( dest == NULL )
			break;
		if ( src == NULL )
			break;
		for(int j=0; j<width; ++j)
		{
#ifndef GDI_PLUS
			if(src[j] != 0xFFFF00FF) dest[j] = src[j];
#else
			dest[j] = src[j];
#endif
		}
		src += srcPitch;
		dest += m_w;
	}
}

void	CDCBitmap :: BltSprite(int x, int y, CSprRes * sprRes, CMotion * curMotion, unsigned int * palette)
{
	int ox, oy;

	for (int a = 0; a < curMotion->numClips; a++)
	{
		CSprClip *	clip = curMotion->GetClip(a);

		SprImg * img = sprRes->m_sprites[clip->clipType][clip->sprIndex];

		sAssert(clip->clipType==0);

		int	xStart, yStart;

		if(a == 0)
		{
			ox = clip->x + (img->width / 2);
			oy = clip->y + (img->height / 2);
			xStart	= x - img->width / 2;
			yStart	= y - img->height / 2;
		}
		else
		{
			xStart	= x - (ox - clip->x);
			yStart	= y - (oy - clip->y);
		}

		int	xLen	= min(xStart+img->width,m_w) - max(xStart, 0);
		int	yLen	= min(yStart+img->height,m_h) - max(yStart, 0);

		int	xSkip	= img->width - xLen;
		int	srcSkip	= max(-yStart, 0) * img->width + max(-xStart, 0);

		int		keyCnt		= 0;
		BYTE*	srcIter		= img->m_8bitImage;
		DWORD*	destIter	= m_image + max(yStart,0)*m_w + max(xStart, 0);

		int i,j;


		for(i=0; i<srcSkip; ++i)
		{

			if(keyCnt)
			{
				--keyCnt;
			}
			else
			{
				if(!*srcIter) keyCnt = *(++srcIter) - 1;
				++srcIter;
			}
		}

		if(clip->flags == 0)
		{
			for(i=0; i<yLen; ++i)
			{

				for(j=0; j<xLen; ++j)
				{

					if(keyCnt)
					{
						--keyCnt;
					}
					else
					{
						if(!*srcIter) keyCnt = *(++srcIter) - 1;
						else destIter[j] = PACK555TO32(palette[*srcIter]);
						++srcIter;
					}
				}

				for(j=0; j<xSkip; ++j)
				{

					if(keyCnt)
					{
						--keyCnt;
					}
					else
					{
						if(!*srcIter) keyCnt = *(++srcIter) - 1;
						++srcIter;
					}
				}

				destIter += m_w;
			}
		}
		else
		{
			for(i=0; i<yLen; ++i)
			{

				for(j=0; j<xLen; ++j)
				{

					if(keyCnt)
					{
						--keyCnt;
					}
					else
					{
						if(!*srcIter) keyCnt = *(++srcIter) - 1;
						else destIter[xLen-1-j] = PACK555TO32(palette[*srcIter]);
						++srcIter;
					}
				}

				for(j=0; j<xSkip; ++j)
				{

					if(keyCnt)
					{
						--keyCnt;
					}
					else
					{
						if(!*srcIter) keyCnt = *(++srcIter) - 1;
						++srcIter;
					}
				}

				destIter += m_w;
			}
		}
	}
}

#ifdef _DX9
void	CDCBitmap :: BltTexture2(int x, int y, CTexture *src, int srcx, int srcy, int width, int height, int xflip, int zoomx, int zoomy)
{
	if(!src) return;
	if(m_image == NULL) return;

	m_dirty = true;

	int						i, j;
	IDirectDrawSurface7 *	srcSurface;
	WORD					*lpSrcSurfacePtr;
	int						lSrcPitch;
	int						r, g, b;
	DWORD					color;

	DWORD*					lpDestSurfacePtr = m_image;
	int						lDestPitch = m_w;

	srcSurface = src->GetDDSurface();

	if(!srcSurface) return;
	D3DLOCKED_RECT LockedRect;
	if(FAILED(srcSurface->LockRect(0, &LockedRect,NULL,0))) return;
	lpSrcSurfacePtr = (WORD *)LockedRect.pBits;
	lSrcPitch = LockedRect.Pitch / 2;









	switch (src->GetPixelFormat())
	{
		case	PF_A1R5G5B5:
			if (xflip)
			{
				for (i = 0; i < height; i++)
				{
					for (j = width - 1; j >= 0; j--)
					{
						color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));

						if(color&0x8000)
						{

							r = (int(0x7c00 & color) >> 7);
							g = (int(0x03e0 & color) >> 2);
							b = (int(0x001f & color) << 3);

							for (int zy=0; zy<zoomy; ++zy)
							{
								for (int zx=0; zx<zoomx; ++zx)
								{
									*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + ((width-1)-j)*zoomx + zx)) = PACK32BIT8888(255, r, g, b);
								}
							}
						}
					}
				}
			}
			else
			{
				for (i = 0; i < height; i++)
				{
					for (j = 0; j < width; j++)
					{
						color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));

						if(color&0x8000)
						{
							r = (int(0x7c00 & color) >> 7);
							g = (int(0x03e0 & color) >> 2);
							b = (int(0x001f & color) << 3);

							if ( (lpDestSurfacePtr + lDestPitch * (y + i*zoomy + (zoomy-1)) + (x + j*zoomx + (zoomx-1))) != NULL )
							{
								for (int zy=0; zy<zoomy; ++zy)
								{
									for (int zx=0; zx<zoomx; ++zx)
										*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + j*zoomx + zx)) = PACK32BIT8888(255, r, g, b);
								}
							}
						}
					}
				}
			}
			break;


		case PF_A4R4G4B4:
			if (xflip)
			{
				int a = 0;
				for (i = 0; i < height; i++)
				{
					for (j = width - 1; j >= 0; j--)
					{
						color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));


						{
							a = ((color >> 12)&0xF )* 0x11;
							r = ((color >> 8)&0xF )* 0x11;
							g = ((color >> 4)&0xF )* 0x11;
							b = ((color >> 0)&0xF )* 0x11;

							for (int zy=0; zy<zoomy; ++zy)
							{
								for (int zx=0; zx<zoomx; ++zx)
								{
									*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + ((width-1)-j)*zoomx + zx)) = PACK32BIT8888(a, r, g, b);
								}
							}
						}
					}
				}
			}
			else
			{
				int a = 0;
				for (i = 0; i < height; i++)
				{
					for (j = 0; j < width; j++)
					{
						color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));


						{
							a = ((color >> 12)&0xF )* 0x11;
							r = ((color >> 8)&0xF )* 0x11;
							g = ((color >> 4)&0xF )* 0x11;
							b = ((color >> 0)&0xF )* 0x11;

							if ( (lpDestSurfacePtr + lDestPitch * (y + i*zoomy + (zoomy-1)) + (x + j*zoomx + (zoomx-1))) != NULL )
							{
								for (int zy=0; zy<zoomy; ++zy)
								{
									for (int zx=0; zx<zoomx; ++zx)
										*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + j*zoomx + zx)) = PACK32BIT8888(a, r, g, b);
								}
							}
						}
					}
				}
			}
			break ;
	}
	srcSurface->UnlockRect(0);
}
#else
void	CDCBitmap :: BltTexture2(int x, int y, CTexture *src, int srcx, int srcy, int width, int height, int xflip, int zoomx, int zoomy)
{
	if(!src) return;
	DWORD*					lpDestSurfacePtr = m_image;
	if(lpDestSurfacePtr == NULL) return;

	m_dirty = true;

	int						i, j;
	IDirectDrawSurface7 *	srcSurface;
	WORD					*lpSrcSurfacePtr;
	int						lSrcPitch;
	int						r, g, b;
	DWORD					color;
	int						lDestPitch = m_w;

	srcSurface = src->GetDDSurface();
	if(!srcSurface) return;

	DDSURFACEDESC2	srcDesc = { sizeof(DDSURFACEDESC2), 0 };
	if(FAILED(srcSurface->Lock(NULL, &srcDesc, DDLOCK_WAIT, NULL))) return;

	lpSrcSurfacePtr = (WORD*)srcDesc.lpSurface;
	lSrcPitch = srcDesc.lPitch / 2;

	if(src->GetPixelFormat() == PF_A1R5G5B5) {
		if(xflip) {
			for(i=0; i<height; i++) {
				for(j=width-1; j>=0; j--) {
					color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));
					if(color&0x8000) {
						r = (int(0x7c00 & color) >> 7);
						g = (int(0x03e0 & color) >> 2);
						b = (int(0x001f & color) << 3);
						for(int zy=0; zy<zoomy; ++zy) {
							for(int zx=0; zx<zoomx; ++zx) {
								*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + ((width-1)-j)*zoomx + zx)) = PACK32BIT8888(255, r, g, b);
							}
						}
					}
				}
			}
		} else {
			for(i=0; i<height; i++) {
				for(j=0; j<width; j++) {
					color = *(lpSrcSurfacePtr + lSrcPitch * (i + srcy) + (j + srcx));
					if(color&0x8000) {
						r = (int(0x7c00 & color) >> 7);
						g = (int(0x03e0 & color) >> 2);
						b = (int(0x001f & color) << 3);
						if((lpDestSurfacePtr + lDestPitch * (y + i*zoomy + (zoomy-1)) + (x + j*zoomx + (zoomx-1))) != NULL) {
							for(int zy=0; zy<zoomy; ++zy) {
								for(int zx=0; zx<zoomx; ++zx)
									*(lpDestSurfacePtr + lDestPitch * (y + i*zoomy + zy) + (x + j*zoomx + zx)) = PACK32BIT8888(255, r, g, b);
							}
						}
					}
				}
			}
		}
	}
	srcSurface->Unlock(NULL);
}
#endif

void	CDCBitmap :: Resize(int w, int h)
{
	if((DWORD)w > m_w || (DWORD)h > m_h) {
		SelectObject(m_dc, m_bitmapOld);
		DeleteObject(m_bitmap);
		CreateDCSurface(w, h);
	}
}

void	CDCBitmap :: Update(int x, int y, int width, int height, DWORD *imgData, bool SkipColorKey)
{
	if(m_image == NULL) return;
	if(imgData == NULL) return;

	m_dirty = true;

	for(int i=0; i<height; ++i)
	{
		if ( m_image[(i+y)*m_w + x] == NULL)
			break;
		for(int j=0; j<width; ++j)
		{
			DWORD color = imgData[i*width + j];

#ifndef GDI_PLUS
			if(!SkipColorKey || color!=0xFFFF00FF)
				m_image[(i+y)*m_w + (j+x)] = color;
#else
			if(!SkipColorKey)
				m_image[(i+y)*m_w + (j+x)] = color;
#endif
		}
	}
}

void	CDCBitmap :: DrawSurface(int x, int y, int w, int h, DWORD color)
{
	if(m_image == NULL) return;

	UpdateSurface();

	if(m_textureList.empty()) return;

	texListIter iter = m_textureList.begin();
	for(int iter_h=0; iter_h<h; iter_h+=256) {
		for(int iter_w=0; iter_w<w; iter_w+=256) {
			(*iter)->DrawSurface(x+iter_w, y+iter_h, (w-iter_w)>256 ? 256 : w-iter_w, (h-iter_h)>256 ? 256 : h-iter_h, color);
			++iter;
		}
	}
}

void	CDCBitmap :: CreateDCSurface(DWORD w, DWORD h)
{
	m_dirty = true;

	m_w = w;
	m_h = h;

	for(texListIter iter = m_textureList.begin(); iter != m_textureList.end(); ++iter) delete (*iter);
	m_textureList.clear();

	BITMAPINFO bitmapinfo;

	bitmapinfo.bmiHeader.biSize				= sizeof(bitmapinfo.bmiHeader);
	bitmapinfo.bmiHeader.biWidth			= m_w;
	bitmapinfo.bmiHeader.biHeight			= -(int)m_h;
	bitmapinfo.bmiHeader.biPlanes			= 1;
	bitmapinfo.bmiHeader.biBitCount			= 32;
	bitmapinfo.bmiHeader.biCompression		= BI_RGB;
	bitmapinfo.bmiHeader.biSizeImage		= 0;
	bitmapinfo.bmiHeader.biXPelsPerMeter	= 100;
	bitmapinfo.bmiHeader.biYPelsPerMeter	= 100;
	bitmapinfo.bmiHeader.biClrUsed			= 0;
	bitmapinfo.bmiHeader.biClrImportant		= 0;

	m_bitmap = CreateDIBSection(NULL, &bitmapinfo, DIB_RGB_COLORS, (PVOID*)&m_image, NULL, 0);

	if(m_bitmap == NULL) m_image = NULL;

	m_bitmapOld = (HBITMAP)SelectObject(m_dc, m_bitmap);
}


void	CDCBitmap :: UpdateSurface()
{
	if(m_dirty) {
		if(m_textureList.empty()) {
			for(int h=0; h<m_h; h+=256) {
				for(int w=0; w<m_w; w+=256) {
					m_textureList.push_back(g_texMgr.CreateTexture((m_w-w)>256 ? 256 : m_w-w, (m_h-h)>256 ? 256 : m_h-h, PF_A1R5G5B5, NULL));
				}
			}
		}

		int count = m_w*m_h*4;
		BYTE* image = ((BYTE*)m_image) + 3;

		for(int i=0; i<count; i+=4) image[i] = 0xFF;

		texListIter iter = m_textureList.begin();
		for(int h=0; h<m_h; h+=256) {
			for(int w=0; w<m_w; w+=256) {
				(*iter)->Update(0, 0, (m_w-w)>256 ? 256 : m_w-w, (m_h-h)>256 ? 256 : m_h-h, m_image+m_w*h+w, false, m_w);
				++iter;
			}
		}

		m_dirty = false;
	}
}
