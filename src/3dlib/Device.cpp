#include "common.h"
#include "../device/Sound.h"
#include "Device.h"
#include "D3dutil.h"
#include "Renderer.h"
#include "Texture.h"

#include "../base/Util.h"
#include "../resource/Bitmap.h"

#include "bf_types.h"
#include <stdio.h>
#include "../pdb_shared.h"




#ifndef PACK16BIT1555
#define PACK16BIT1555(a, r, g, b) \
	((((a) & 0x80) << 8) + (((r) & 0xF8) << 7) + (((g) & 0xF8) << 2) + (((b) & 0xF8) >> 3))
#endif




extern bool    g_isSupportLMTex;
extern int     g_isMipmap;
extern Registry2 g_registry;

bool g_isSupportLMTex = true;
C3dDevice g_3dDevice;
// Textures get mip levels (the trilinear option; DDSD_MIPMAPCOUNT and
// DDSCAPS_MIPMAP of the 2008 client's texture descriptions).
int g_isMipmap = 0;


extern int WINDOW_HEIGHT;
extern int WINDOW_WIDTH;
extern int WINDOW_X_OFFSET;
extern int WINDOW_Y_OFFSET;


#include "bf_input.h"

// The render target is the gfx target (bf_render.h), in 32-bit X8R8G8B8: the
// display mode, the primary surface and the back buffer DirectDraw gave the
// 2008 client are that one target (M11.9).
static const DWORD FRAMEBUFFER_RMASK = 0x00FF0000;
static const DWORD FRAMEBUFFER_GMASK = 0x0000FF00;
static const DWORD FRAMEBUFFER_BMASK = 0x000000FF;
static const DWORD FRAMEBUFFER_AMASK = 0;
static const DWORD FRAMEBUFFER_BITS  = 32;

// The back buffer is made at the client's size (its client area, or the
// size it resizes to); a gfx target of another size follows it, keeping the
// UI scale.
static void FitTarget(DWORD w, DWORD h)
{
	int tw, th, scale;
	bf_gfx_device* gfx = bf_render_device();
	if (!gfx)
		return;
	bf_gfx_get_size(gfx, &tw, &th, &scale, NULL, NULL);
	if ((DWORD)tw == w && (DWORD)th == h)
		return;
	BF_TRACE(BFT_GFX, "device: render target %ux%u asked, gfx is %dx%d: resizing gfx", (unsigned)w, (unsigned)h, tw, th);
	if (bf_gfx_resize(gfx, (int)w, (int)h, scale) != BF_GFX_OK)
		bf_tracef(BFT_ERR, "device: bf_gfx_resize(%u, %u, %d) failed", (unsigned)w, (unsigned)h, scale);
}

// user32's SetRect.
static void SetRectangle(RECT* r, int left, int top, int right, int bottom)
{
	r->left = left;
	r->top = top;
	r->right = right;
	r->bottom = bottom;
}

void GDIFlip()
{
	// A message box over the game: the render target's lock is let go first
	// (DirectDraw's FlipToGDISurface did nothing more on a window).
	if (g_3dDevice.IsInitialized())
		bf_render_unlock();
}

C3dDevice :: C3dDevice()
{
	m_hWnd             = NULL;
	m_dwRenderWidth    = 0L;
	m_dwRenderHeight   = 0L;
	m_bInitialized     = FALSE;
	m_bHasBackup       = FALSE;
	bf_gfx_state_init(&m_state);
	m_bIsFullscreen    = FALSE;
	m_bIsGDIObject     = FALSE;
	m_pfRShiftR = 0, m_pfRShiftL = 8;
	m_pfGShiftR = 0, m_pfGShiftL = 8;
	m_pfBShiftR = 0, m_pfBShiftL = 8;
	m_pfAShiftR = 0, m_pfAShiftL = 8;
	m_pfBitCount = 0;
	m_pfRBitMask = 0;
	m_pfGBitMask = 0;
	m_pfBBitMask = 0;
	m_pfABitMask = 0;
}

C3dDevice :: ~C3dDevice()
{
}

HRESULT C3dDevice :: DestroyObjects()
{
	// Nothing is counted any more (the DirectDraw and Direct3D objects'
	// references were): the buffers are the gfx target, the textures the
	// client's. Bound stages are let go.
	if (m_bInitialized) {
		bf_render_unlock();
		bf_render_bind_texture(0, NULL);
		bf_render_bind_texture(1, NULL);
	}
	m_bInitialized = FALSE;
	m_bHasBackup = FALSE;

	return S_OK;
}

HRESULT C3dDevice :: Init(HWND hwnd, const void* pDriverGUID, const void* pDeviceGUID,
						  DisplayMode* pMode, DWORD dwFlags)
{
	HRESULT hr;

	m_hWnd = hwnd;
	if (NULL == m_hWnd || NULL == pDeviceGUID ||
		(NULL == pMode && (dwFlags & D3DFW_FULLSCREEN)))
		return E_INVALIDARG;

	m_bIsFullscreen = (dwFlags & D3DFW_FULLSCREEN) ? TRUE : FALSE;

	if (m_bIsFullscreen) {
		// SetWindowLong(WS_POPUP), ShowWindow(SW_MAXIMIZE): the window's state is macOS's.
		BF_TRACE(BFT_GFX, "device: full screen asked; the window keeps its state");
	}

	hr = CreateEnvironment(pDriverGUID, pDeviceGUID, pMode, dwFlags);
	if (FAILED(hr)) {
		DestroyObjects();
		if (E_FAIL == hr)
			return D3DFWERR_INITIALIZATIONFAILED;
	}

	if (!m_bInitialized)
		return hr;

	SetRenderState((D3DRENDERSTATETYPE)22, 1);
	SetRenderState((D3DRENDERSTATETYPE)23, 4);
	SetRenderState((D3DRENDERSTATETYPE)4, TRUE);
	SetRenderState((D3DRENDERSTATETYPE)25, 7);
	SetRenderState((D3DRENDERSTATETYPE)24, 0xf0);
	SetRenderState((D3DRENDERSTATETYPE)41, TRUE);
	SetRenderState((D3DRENDERSTATETYPE)7, TRUE);
	SetRenderState((D3DRENDERSTATETYPE)14, TRUE);
	SetRenderState((D3DRENDERSTATETYPE)19, 5);
	SetRenderState((D3DRENDERSTATETYPE)20, 6);

	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)11, 0);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)1, 4);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)2, 2);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)3, 0);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)4, 4);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)5, 2);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)6, 0);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)11, 1);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)1, 18);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)4, 1);
	SetTextureStageState(2, (D3DTEXTURESTAGESTATETYPE)1, 1);
	SetTextureStageState(2, (D3DTEXTURESTAGESTATETYPE)4, 1);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)14, 1);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)17, 2);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)16, 2);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)17, 2);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)16, 2);
	SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)12, 3);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)12, 4);
	SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)15, 0xff000000);

	return hr;
}

void C3dDevice :: EnableClipper(BOOL bEnable) {

	// The primary surface's clipper for the window: the gfx target is the
	// window's, there is nothing to clip.
}

HRESULT C3dDevice :: CreateEnvironment(const void* pDriverGUID, const void* pDeviceGUID,
									   DisplayMode* pMode, DWORD dwFlags)
{
	HRESULT hr;

	hr = CreateDirectDraw(pDriverGUID, dwFlags);
	if (FAILED(hr))
		return hr;

	if (FAILED(hr = CreateBuffers(pMode, dwFlags)))
		return hr;

	if (!pDeviceGUID)
		return S_OK;

	// The 3D device (any device GUID: the one gfx device), with the Direct3D 7
	// defaults; its depth buffer is the gfx target's (32-bit float), so the
	// z-buffer surface and the viewport of the whole target are not made.
	bf_gfx_state_init(&m_state);
	bf_render_bind_texture(0, NULL);
	bf_render_bind_texture(1, NULL);
	m_bInitialized = TRUE;

	return S_OK;
}

HRESULT C3dDevice :: CreateDirectDraw(const void* pDriverGUID, DWORD dwFlags)
{
	// Any driver GUID (or none) is the one gfx device, which the start-up has
	// made (bf_port_start_graphics); the display mode is its target's.
	if (!bf_render_device())
		return D3DFWERR_NODIRECTDRAW;

	m_windowBitCount = FRAMEBUFFER_BITS;

	if (m_bIsFullscreen)
		BF_TRACE(BFT_GFX, "device: exclusive full-screen requested; the application module owns full screen");
	return S_OK;
}

HRESULT C3dDevice :: CreateBuffers(DisplayMode* pddsd, DWORD dwFlags)
{
	if (dwFlags & D3DFW_FULLSCREEN) {
		SetRectangle(&m_rcViewportRect, 0, 0, pddsd->width, pddsd->height);
		m_rcScreenRect = m_rcViewportRect;
		m_dwRenderWidth = m_rcViewportRect.right;
		m_dwRenderHeight = m_rcViewportRect.bottom;

		// SetDisplayMode: the window keeps its size; the primary's back
		// buffer is the gfx target as it is.
		BF_TRACE(BFT_GFX, "device: SetDisplayMode(%u, %u, %u) accepted; the window keeps its size",
				 (unsigned)m_dwRenderWidth, (unsigned)m_dwRenderHeight, (unsigned)pddsd->bitCount);
	} else {
		int w, h;
		bf_input_client_size(&w, &h);   // the client area, logical ("screen" coordinates are the client's)
		SetRectangle(&m_rcViewportRect, 0, 0, w, h);
		m_rcScreenRect = m_rcViewportRect;
		m_dwRenderWidth = m_rcViewportRect.right;
		m_dwRenderHeight = m_rcViewportRect.bottom;

		if (dwFlags & D3DFW_BACKBUFFER)
			FitTarget(m_dwRenderWidth, m_dwRenderHeight);
	}

	m_pfRShiftR = 0, m_pfRShiftL = 8;
	m_pfGShiftR = 0, m_pfGShiftL = 8;
	m_pfBShiftR = 0, m_pfBShiftL = 8;
	m_pfAShiftR = 0, m_pfAShiftL = 8;
	m_pfABitMask = FRAMEBUFFER_AMASK;
	m_pfRBitMask = FRAMEBUFFER_RMASK;
	m_pfGBitMask = FRAMEBUFFER_GMASK;
	m_pfBBitMask = FRAMEBUFFER_BMASK;
	m_pfBitCount = FRAMEBUFFER_BITS;
	DWORD mask;
	for (mask = m_pfRBitMask; mask && !(mask & 1); mask >>= 1)
		m_pfRShiftR++;
	for (; mask; mask >>= 1)
		m_pfRShiftL--;
	for (mask = m_pfGBitMask; mask && !(mask & 1); mask >>= 1)
		m_pfGShiftR++;
	for (; mask; mask >>= 1)
		m_pfGShiftL--;
	for (mask = m_pfBBitMask; mask && !(mask & 1); mask >>= 1)
		m_pfBShiftR++;
	for (; mask; mask >>= 1)
		m_pfBShiftL--;
	for (mask = m_pfABitMask; mask && !(mask & 1); mask >>= 1)
		m_pfAShiftR++;
	for (; mask; mask >>= 1)
		m_pfAShiftL--;

	// The backup surface: the gfx module's backup of the target (BackupFrame).
	m_bHasBackup = FALSE;
	return S_OK;
}

void	C3dDevice :: DestroyRenderer(CRenderer *rc)
{
	delete	rc;
}

HRESULT C3dDevice :: FlipToGDISurface(BOOL bDrawFrame)
{
	// FlipToGDISurface, DrawMenuBar and RedrawWindow for a message box over
	// the full screen: macOS draws the window's frame and menu.
	return S_OK;
}

HRESULT C3dDevice :: CloneFrame()
{
	if (!m_bInitialized)
		return D3DFWERR_NOTINITIALIZED;

	// The window into the back buffer: the target holds the frame already.
	return S_OK;
}

HRESULT C3dDevice :: ShowFrame(bool vertSync)
{
	if (!m_bInitialized)
		return D3DFWERR_NOTINITIALIZED;

	// Flip, or the Blt of the viewport into the window (the window shows the
	// whole target, so a viewport smaller than it is stretched over it);
	// present paces on the display, WaitForVerticalBlank is not needed.
	if (vertSync && m_bIsFullscreen && !m_bIsGDIObject) {
		bf_render_present();
		return S_OK;
	}

	bf_render_present_part(m_rcViewportRect.left, m_rcViewportRect.top,
						   m_rcViewportRect.right - m_rcViewportRect.left,
						   m_rcViewportRect.bottom - m_rcViewportRect.top);
	return S_OK;
}

// A clear of the render size, clipped to the target (Direct3D 7 clipped it
// to the viewport, which is the render size); the whole target is cleared
// as such, which also clears the native pixels past the last logical one.
static HRESULT ClearRender(unsigned flags, DWORD color, DWORD w, DWORD h)
{
	int tw, th;
	bf_render_size(&tw, &th);
	int x1 = (int)w < tw ? (int)w : tw;
	int y1 = (int)h < th ? (int)h : th;
	if (x1 <= 0 || y1 <= 0)
		return S_OK;
	bf_gfx_rect r = { 0, 0, x1, y1 };
	bf_render_clear(flags, color, 1.0f, (x1 == tw && y1 == th) ? NULL : &r);
	return S_OK;
}

HRESULT	C3dDevice :: Clear(DWORD color)
{
	return	ClearRender(BF_GFX_CLEAR_COLOR|BF_GFX_CLEAR_DEPTH, color, m_dwRenderWidth, m_dwRenderHeight);
}

HRESULT	C3dDevice :: ClearZBuffer()
{
	return	ClearRender(BF_GFX_CLEAR_DEPTH, 0, m_dwRenderWidth, m_dwRenderHeight);
}

HRESULT C3dDevice :: RestoreSurfaces()
{
	Trace("HRESULT C3dDevice::RestoreSurfaces()\n");
	return S_OK;   // Metal loses no surfaces
}

#if defined(BF_PORT)
// The port's resize path (M4.3a): the window's logical size has changed and
// the graphics device has been resized to it (port/native/view). The back
// buffer and the backup are made anew at the new size, as CreateBuffers does
// at start-up; the depth buffer is the gfx target's. The original fixed the
// size at start-up.
HRESULT C3dDevice :: BfResize(DWORD w, DWORD h)
{
	if (!m_bInitialized || m_bIsFullscreen)
		return D3DFWERR_NOTINITIALIZED;
	if (w == m_dwRenderWidth && h == m_dwRenderHeight)
		return S_OK;

	FitTarget(w, h);
	m_bHasBackup = FALSE;

	SetRectangle(&m_rcViewportRect, 0, 0, w, h);
	int cw, ch;
	bf_input_client_size(&cw, &ch);
	SetRectangle(&m_rcScreenRect, 0, 0, cw, ch);
	m_dwRenderWidth = w;
	m_dwRenderHeight = h;
	return S_OK;
}
#endif

CDC *	C3dDevice :: CreateDCBuffer(DWORD w, DWORD h)
{
	CDC * surface;

	surface = new CDCBitmap(w, h);

	surface->ClearSurface(NULL, 0xFFFF00FF);

	return surface;
}

CSurface *	C3dDevice :: CreateWallPaper(DWORD w, DWORD h)
{
	CSurface* surface;

	if(w <= m_dwMaxTextureWidth && h <= m_dwMaxTextureHeight) {
		surface = g_texMgr.CreateTexture(w, h, PF_A1R5G5B5, NULL);
	} else {
		surface = new CSurfaceWallpaper(m_dwRenderWidth, m_dwRenderHeight);
	}

	return surface;
}

void	C3dDevice :: ConvertPalette(SYSTEMPALETTE* destPal, const PALETTEENTRY* srcPal, int palCnt)
{


	static DWORD transPal = PACK16BIT1555(0, COLORKEY_R, COLORKEY_G, COLORKEY_B);

	const PALETTEENTRY* srcPalEnd = srcPal+palCnt;

	while(srcPal != srcPalEnd)
	{

		if(srcPal->peRed>=0xFE && srcPal->peGreen<0x04 && srcPal->peBlue>=0xFE)
			*destPal = transPal;
		else {
			*destPal = PACK16BIT1555(0xFF, srcPal->peRed, srcPal->peGreen, srcPal->peBlue);
		}
		++srcPal;
		++destPal;
	}
}

CRenderer *	C3dDevice :: CreateRenderer()
{
	PixelFormat pixelFormat;

	if (m_pfRBitMask == 0xf800) {
		pixelFormat = PF_R5G6B5;
	} else if (m_pfRBitMask == 0x7c00) {
		pixelFormat = PF_R5G5B5;
	} else if (m_pfRBitMask == 0x1f) {
		ErrorMsg("BGR 565 MODE. NOT SUPPORTED");
		pixelFormat = PF_R5G6B5;
	} else if (m_pfBitCount == 32) {
		pixelFormat = PF_A8R8G8B8;
	} else {
		ErrorMsg("NOT SUPPORTED MODE");
		pixelFormat = PF_UNSUPPORTED;
	}

	m_bSupportBltStretch = TRUE;   // DDCAPS_BLTSTRETCH: a part of the target is stretched over the window

	// The device's caps (port/native/gfx): textures of 1 to its largest
	// edge without an aspect limit; two texture stages with
	// MODULATEALPHA_ADDCOLOR, so the single-pass lightmap path; trilinear
	// filtering (LINEARMIPLINEAR) when the option asks for it.
	if (m_bInitialized)
	{
		m_dwMinTextureWidth			=	1;
		m_dwMinTextureHeight		=	1;
		m_dwMaxTextureWidth			=	bf_gfx_max_texture_size(bf_render_device());
		m_dwMaxTextureHeight		=	bf_gfx_max_texture_size(bf_render_device());
		m_dwMaxTextureAspectRatio	=	0;
		g_isSupportLMTex = true;

		g_isMipmap = g_registry.m_trilinearFilter ? 1 : 0;
	}
	m_bSupportTextureSurface = (1024 <= m_dwMaxTextureWidth) && (1024 <= m_dwMaxTextureHeight);

	CRenderer *	newRenderer = new CRenderer;
	newRenderer->SetPixelFormat(pixelFormat);
	newRenderer->m_device = this;
	newRenderer->SetSize(m_dwRenderWidth, m_dwRenderHeight);


	if (newRenderer->m_guardBandLeft == 0) {
		newRenderer->m_guardBandLeft	=	-640;
		newRenderer->m_guardBandRight	=	newRenderer->m_width+640;
		newRenderer->m_guardBandTop		=	-480;
		newRenderer->m_guardBandBottom	=	newRenderer->m_height+480;
	} else {
		newRenderer->m_guardBandLeft	=	-16384.0f;   // the gfx module clips at any position
		newRenderer->m_guardBandRight	=	16384.0f;
		newRenderer->m_guardBandTop		=	-16384.0f;
		newRenderer->m_guardBandBottom	=	16384.0f;
	}

#if !defined(BF_PORT)
	TestScreen();
#endif
	// The calibration picks a texel offset of 0.5 on any 32-bit target; the
	// port's renderer has fixed half-pixel offsets, and that is CTexture's
	// default (technical plan 3.3).

	return  newRenderer;
}

void	C3dDevice :: AdjustTextureSize(DWORD* w, DWORD* h)
{
	DWORD width, height;

	for (width = 1; width < *w; width <<= 1);
	width = max(width, m_dwMinTextureWidth);
	width = min(width, m_dwMaxTextureWidth);

	for (height = 1; height < *h; height <<= 1);
	height = max(height, m_dwMinTextureHeight);
	height = min(height, m_dwMaxTextureHeight);

	if(m_dwMaxTextureAspectRatio) {
		if(width/height > m_dwMaxTextureAspectRatio)
			height = width / m_dwMaxTextureAspectRatio;
		if(height/width > m_dwMaxTextureAspectRatio)
			width = height / m_dwMaxTextureAspectRatio;
	}

	*w = width;
	*h = height;
}

bool C3dDevice :: TestScreen()
{
#if defined(BF_PORT)
	// The calibration picks a texel offset of 0.5 on any 32-bit target; the
	// gfx module's half-pixel offset is fixed and that is CTexture's default
	// (technical plan 3.3). Not called (CreateRenderer).
	return true;
#else
	DWORD dwBuffer[16] = { 0, 0, 0xffffffff, 0xffffffff,
		0, 0, 0xffffffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0, 0,
		0xffffffff, 0xffffffff, 0, 0 };

	CSurface screen(m_dwRenderWidth, m_dwRenderHeight);
	CTexture test(4, 4, PF_A1R5G5B5);
	test.Update(0, 0, 4, 4, dwBuffer, false, 0);

	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
	SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	m_pd3dDevice->SetTexture(0, test.m_pddsSurface);

	DWORD i = 0, width = m_dwRenderWidth >> 2;
	DWORD height = m_dwRenderHeight >> 2;
	int difVal = -1024;
	int difOff = 0x8080;

	while (i < 0x10000) {
		DDSURFACEDESC2 srcDesc;
		DWORD j;
		int byteCount;
		m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
							0, 1.0f, 0);
		if (FAILED(m_pd3dDevice->BeginScene()))
			return false;

		for (j = 0; j < width * height; ++j) {
			if (int(i + j) > 0xffff)
				break;

			int x = int(j % width) * 4;
			int y = int(j / width) * 4;
			float tu = float(int(i + j) % 256) * 0.00390625f;
			float tv = float(int(i + j) / 256) * 0.00390625f;

			vector2d xy[2] = { vector2d(float(x), float(y)), vector2d(float(x + 4), float(y + 4)) };
			vector2d uv[2] = { vector2d(tu / test.m_w, tv / test.m_h), vector2d((tu + 4) / test.m_w, (tv + 4) / test.m_h) };
			tlvertex3d tlvert[4] = {
				tlvertex3d(xy[0].x, xy[0].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[0].x, uv[0].y),
				tlvertex3d(xy[0].x, xy[1].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[0].x, uv[1].y),
				tlvertex3d(xy[1].x, xy[0].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[1].x, uv[0].y),
				tlvertex3d(xy[1].x, xy[1].y, 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, uv[1].x, uv[1].y)
			};

			m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, tlvert, 4, 0);
		}

		m_pd3dDevice->EndScene();
		screen.GetDDSurface()->Blt(NULL, m_pddsRenderTarget, NULL, DDBLT_WAIT, NULL);

		srcDesc.dwSize = sizeof(srcDesc);
		srcDesc.dwFlags = 0;
		ZeroMemory(&srcDesc.dwHeight, sizeof(srcDesc) - 8);
		if (FAILED(screen.GetDDSurface()->Lock(NULL, &srcDesc, DDLOCK_WAIT, NULL)))
			return false;

		byteCount = srcDesc.ddpfPixelFormat.dwRGBBitCount >> 3;
		for (j = 0; j < width * height; ++j) {
			if (int(i + j) > 0xffff)
				break;

			int white = 0;
			int black = 0;

			for (int k = 0; k < 16; ++k) {
				int x = int(j % width) * 4 + k % 4;
				int y = int(j / width) * 4 + k / 4;
				DWORD pixel;
				if (byteCount == 2) {
					pixel = *(WORD*)((BYTE*)srcDesc.lpSurface + y * srcDesc.lPitch + x * 2);
				} else {
					pixel = *(DWORD*)((BYTE*)srcDesc.lpSurface + y * srcDesc.lPitch + x * byteCount);
				}

				if (dwBuffer[k])
					white += DecodePixelB((WORD)pixel) +
						DecodePixelG((WORD)pixel) +
						DecodePixelR((WORD)pixel);
				else
					black += DecodePixelB((WORD)pixel) +
						DecodePixelG((WORD)pixel) +
						DecodePixelR((WORD)pixel);
			}

			if (white >= 0x1680 && black <= 0x180) {
				int diff = white - black;
				if (diff > difVal) {
					difVal = diff;
					difOff = int(i + j);
				}
			}
		}
		screen.GetDDSurface()->Unlock(NULL);

		i += width * height;
	}

	CTexture::m_uOffset = float(difOff % 256) * 0.00390625f;
	CTexture::m_vOffset = float(difOff / 256) * 0.00390625f;
	SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	return true;
#endif
}

int C3dDevice :: DecodePixelR(WORD color) const
{
	return ((color & m_pfRBitMask) >> m_pfRShiftR) << m_pfRShiftL;
}

int C3dDevice :: DecodePixelG(WORD color) const
{
	return ((color & m_pfGBitMask) >> m_pfGShiftR) << m_pfGShiftL;
}

int C3dDevice :: DecodePixelB(WORD color) const
{
	return ((color & m_pfBBitMask) >> m_pfBShiftR) << m_pfBShiftL;
}

// The whole target into the backup surface and back: the gfx module's
// backup, never read back (Mode.cpp's fade).
void C3dDevice :: BackupFrame()
{
	if (m_bInitialized) {
		if (bf_render_backup() == BF_GFX_OK)
			m_bHasBackup = TRUE;
	}
}

void C3dDevice :: RestoreFrame()
{
	if (m_bInitialized) {
		if (m_bHasBackup)
			bf_render_restore();
		else
			ClearRender(BF_GFX_CLEAR_COLOR, 0xFF000000, m_dwRenderWidth, m_dwRenderHeight);   // the backup surface as made: black
	}
}

void C3dDevice :: EnableMipmap()
{
	g_isMipmap = g_registry.m_trilinearFilter ? 1 : 0;   // the device filters LINEARMIPLINEAR
}

CDynamicVB :: CDynamicVB()
{
	m_vertCount = 0;
	m_vertOffset = 0;
	m_vertSize = 0;
	m_fvf = 0;
	m_pVB = 0;
}

CDynamicVB :: ~CDynamicVB()
{
	Release();
}

void	CDynamicVB :: Release()
{
	SAFE_DELETE_ARRAY(m_pVB);
}

bool CDynamicVB::Init(const DWORD theFVF, DWORD theVertexCount, DWORD theVertexSize)
{
	Release();

	m_vertCount = theVertexCount;
	m_vertSize = theVertexSize;
	m_vertOffset = 0;
	m_fvf = theFVF;
	m_pVB = new unsigned char[theVertexCount * theVertexSize];
	memset(m_pVB, 0, theVertexCount * theVertexSize);
	return true;
}
bool CDynamicVB::Lock(DWORD vertCount, void** ppLockedData)
{
	if (!m_pVB)
		return false;

	if (vertCount > m_vertCount)
		return false;

	if (m_vertOffset + vertCount <= m_vertCount) {
		*ppLockedData = m_pVB + m_vertSize * m_vertOffset;
		m_vertOffset += vertCount;
		return true;
	} else {
		*ppLockedData = m_pVB;
		m_vertOffset = vertCount;
		return true;
	}
}

void	CDynamicVB :: Unlock()
{
}

void CDynamicVB :: DrawPri(C3dDevice* device, DWORD vertCount)
{
	device->DrawPrimitiveVB(D3DPT_TRIANGLELIST, this, m_vertOffset - vertCount, vertCount, 0);
}

// ---- the device (M11.9) ---------------------------------------------------
// What the Direct3D 7 device of stage A (port/compat/d3d) did with the
// client's calls: render and stage states translated into one bf_gfx_state
// by their numbers, which the gfx enumerations share (bf_gfx.h); states with
// no effect on pre-transformed vertices (lighting, fog table, perspective,
// colour keys of textures, whose key colours CTexture::Update writes as
// alpha 0) and stages 2-7 change nothing. Each draw hands the state and the
// bound textures to gfx (bf_render_draw).

int C3dDevice :: BeginScene()
{
	// Always succeeds: a scene left open by a failed frame must not stop
	// every later frame (the client brackets each flush with Begin/EndScene).
	bf_render_begin_scene();
	return S_OK;
}

int C3dDevice :: EndScene()
{
	bf_render_end_scene();
	return S_OK;
}

static unsigned char StateValue(DWORD v, DWORD lo, DWORD hi, unsigned char keep)
{
	return v >= lo && v <= hi ? (unsigned char)v : keep;
}

void C3dDevice :: SetRenderState(D3DRENDERSTATETYPE type, DWORD v)
{
	bf_gfx_state &st = m_state;
	switch (type) {
	case D3DRENDERSTATE_ZENABLE: st.depth_enable = v != D3DZB_FALSE; break;
	case D3DRENDERSTATE_ZWRITEENABLE: st.depth_write = v != 0; break;
	case D3DRENDERSTATE_ALPHATESTENABLE: st.alpha_test_enable = v != 0; break;
	case D3DRENDERSTATE_SRCBLEND: st.src_blend = StateValue(v, 1, 13, st.src_blend); break;
	case D3DRENDERSTATE_DESTBLEND: st.dst_blend = StateValue(v, 1, 13, st.dst_blend); break;
	case D3DRENDERSTATE_CULLMODE: st.cull = StateValue(v, 1, 3, st.cull); break;
	case D3DRENDERSTATE_ZFUNC: st.depth_func = StateValue(v, 1, 8, st.depth_func); break;
	case D3DRENDERSTATE_ALPHAREF: st.alpha_ref = (unsigned char)(v & 0xFF); break;
	case D3DRENDERSTATE_ALPHAFUNC: st.alpha_func = StateValue(v, 1, 8, st.alpha_func); break;
	case D3DRENDERSTATE_ALPHABLENDENABLE: st.blend_enable = v != 0; break;
	case D3DRENDERSTATE_FOGENABLE: st.fog_enable = v != 0; break;
	case D3DRENDERSTATE_SPECULARENABLE: st.specular_enable = v != 0; break;
	case D3DRENDERSTATE_FOGCOLOR: st.fog_color = v; break;
	case D3DRENDERSTATE_TEXTUREFACTOR: st.texture_factor = v; break;
	default: break;   // no effect on pre-transformed vertices (port/native/gfx/D3D-USAGE.md 3)
	}
}

void C3dDevice :: SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD v)
{
	if (stage >= 2) {
		if (type == D3DTSS_COLOROP && v != D3DTOP_DISABLE) {
			static bool once;
			if (!once) {
				once = true;
				BF_TRACE(BFT_GFX, "device: texture stage %u enabled; the gfx module has two", (unsigned)stage);
			}
		}
		return;
	}
	bf_gfx_stage_state &s = m_state.stage[stage];
	switch (type) {
	case D3DTSS_COLOROP: s.color_op = StateValue(v, 1, 24, BF_GFX_OP_DISABLE); break;
	case D3DTSS_ALPHAOP: s.alpha_op = StateValue(v, 1, 24, BF_GFX_OP_DISABLE); break;
	case D3DTSS_COLORARG1: s.color_arg1 = (unsigned char)(v & 0x3F); break;
	case D3DTSS_COLORARG2: s.color_arg2 = (unsigned char)(v & 0x3F); break;
	case D3DTSS_ALPHAARG1: s.alpha_arg1 = (unsigned char)(v & 0x3F); break;
	case D3DTSS_ALPHAARG2: s.alpha_arg2 = (unsigned char)(v & 0x3F); break;
	case D3DTSS_TEXCOORDINDEX: s.texcoord_index = (v & 0xFFFF) ? 1 : 0; break;
	case D3DTSS_ADDRESS: s.address_u = s.address_v = StateValue(v, 1, 4, s.address_u); break;
	case D3DTSS_ADDRESSU: s.address_u = StateValue(v, 1, 4, s.address_u); break;
	case D3DTSS_ADDRESSV: s.address_v = StateValue(v, 1, 4, s.address_v); break;
	case D3DTSS_BORDERCOLOR: s.border_color = v; break;
	case D3DTSS_MAGFILTER: s.mag_filter = v == D3DTFG_POINT ? BF_GFX_FILTER_NEAREST : BF_GFX_FILTER_LINEAR; break;
	case D3DTSS_MINFILTER: s.min_filter = v == D3DTFN_POINT ? BF_GFX_FILTER_NEAREST : BF_GFX_FILTER_LINEAR; break;
	case D3DTSS_MIPFILTER: s.mip_filter = StateValue(v, 1, 3, s.mip_filter); break;
	default: break;   // bump-mapping and texture-transform states: unused
	}
}

void C3dDevice :: SetTexture(DWORD stage, bf_gfx_texture* tex)
{
	if (stage < 2)
		bf_render_bind_texture((int)stage, tex);   // stages 2-7 draw nothing
}

int C3dDevice :: Draw(D3DPRIMITIVETYPE type, DWORD fvf, const void* verts, DWORD numVerts, const WORD* indices, DWORD numIndices)
{
	if (!m_bInitialized)
		return E_FAIL;
	if (!verts)
		return E_INVALIDARG;
	// The two layouts the gfx module draws: tlvertex3d and lmtlvertex3d.
	bf_gfx_vertex_format format;
	if (fvf == (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1))
		format = BF_GFX_VERTEX_TL1;
	else if (fvf == (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2))
		format = BF_GFX_VERTEX_TL2;
	else {
		static bool once;
		if (!once) {
			once = true;
			BF_TRACE(BFT_GFX, "device: draw with FVF 0x%x skipped: only pre-transformed tlvertex3d/lmtlvertex3d are drawn", (unsigned)fvf);
		}
		return S_OK;
	}
	if ((DWORD)type < D3DPT_POINTLIST || (DWORD)type > D3DPT_TRIANGLEFAN)
		return E_INVALIDARG;
	if (numVerts > 0xFFFF && indices)
		return E_INVALIDARG;
	return bf_render_draw(&m_state, (bf_gfx_prim)type, format, verts, (int)numVerts, indices, (int)numIndices) == BF_GFX_OK ? S_OK : E_FAIL;
}

int C3dDevice :: DrawPrimitive(D3DPRIMITIVETYPE type, DWORD fvf, void* verts, DWORD numVerts, DWORD flags)
{
	return Draw(type, fvf, verts, numVerts, NULL, 0);
}

int C3dDevice :: DrawIndexedPrimitive(D3DPRIMITIVETYPE type, DWORD fvf, void* verts, DWORD numVerts, WORD* indices, DWORD numIndices, DWORD flags)
{
	if (!indices)
		return E_INVALIDARG;
	return Draw(type, fvf, verts, numVerts, indices, numIndices);
}

int C3dDevice :: DrawPrimitiveVB(D3DPRIMITIVETYPE type, CDynamicVB* vb, DWORD startVert, DWORD numVerts, DWORD flags)
{
	if (!vb || !vb->GetVertices() || startVert + numVerts > vb->GetSize())
		return E_INVALIDARG;
	return Draw(type, vb->GetFVF(), vb->GetVertices() + startVert * vb->GetVertexSize(), numVerts, NULL, 0);
}

int C3dDevice :: DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE type, CDynamicVB* vb, DWORD startVert, DWORD numVerts, WORD* indices, DWORD numIndices, DWORD flags)
{
	if (!vb || !vb->GetVertices() || !indices || startVert + numVerts > vb->GetSize())
		return E_INVALIDARG;
	return Draw(type, vb->GetFVF(), vb->GetVertices() + startVert * vb->GetVertexSize(), numVerts, indices, numIndices);
}
