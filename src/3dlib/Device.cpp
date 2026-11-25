#include "common.h"
#include "../device/Sound.h"
#include "Device.h"
#include "D3dutil.h"
#include "Renderer.h"
#include "Texture.h"

#include "../base/Util.h"
#include "../resource/Bitmap.h"

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
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


void D3DUtil_InitSurfaceDesc(DDSURFACEDESC2& ddsd, DWORD dwFlags, DWORD dwCaps);

void GDIFlip()
{
	if (g_3dDevice.GetDirectDraw()) {
		g_3dDevice.GetBackBuffer()->Unlock(NULL);
		g_3dDevice.GetFrontBuffer()->Unlock(NULL);
		g_3dDevice.GetDirectDraw()->FlipToGDISurface();
	}
}

C3dDevice :: C3dDevice()
{
	m_hWnd             = NULL;
	m_dwRenderWidth    = 0L;
	m_dwRenderHeight   = 0L;
	m_pddsFrontBuffer  = NULL;
	m_pddsBackBuffer   = NULL;
	m_pddsRenderTarget = NULL;
	m_pDD              = NULL;
	m_bInitialized     = FALSE;
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
	m_pddsRenderBackup = NULL;
}

C3dDevice :: ~C3dDevice()
{
}

HRESULT C3dDevice :: DestroyObjects()
{
	int nD3D = 0, nDD = 0;

	// The renderer's device holds no object of its own: its textures are
	// the client's, bound stages are let go here.
	if (m_bInitialized) {
		bf_render_bind_texture(0, NULL);
		bf_render_bind_texture(1, NULL);
	}
	m_bInitialized = FALSE;
	if (!m_bIsFullscreen)
		SAFE_RELEASE(m_pddsBackBuffer);

	SAFE_RELEASE(m_pddsRenderTarget);
	SAFE_RELEASE(m_pddsFrontBuffer);
	SAFE_RELEASE(m_pddsRenderBackup);

	if (m_pDD) {
		m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
		if ((nDD = m_pDD->Release()) > 0) {
			char tmp[80];
			sprintf(tmp, " : DDraw object is still referenced %d times", nDD);
			MessageBox(g_hMainWnd, tmp, "Error", 0);
		}
	}
	m_pDD = NULL;

	return (nDD == 0 && nD3D == 0) ? S_OK : D3DFWERR_NONZEROREFCOUNT;
}

HRESULT C3dDevice :: Init(HWND hwnd, GUID* pDriverGUID, GUID* pDeviceGUID,
						  DDSURFACEDESC2* pMode, DWORD dwFlags)
{
	HRESULT hr;

	m_hWnd = hwnd;
	if (NULL == m_hWnd || NULL == pDeviceGUID ||
		(NULL == pMode && (dwFlags & D3DFW_FULLSCREEN)))
		return E_INVALIDARG;

	m_bIsFullscreen = (dwFlags & D3DFW_FULLSCREEN) ? TRUE : FALSE;

	if (m_bIsFullscreen) {
		SetWindowLong(g_hMainWnd, GWL_STYLE, WS_POPUP);
		ShowWindow(g_hMainWnd, SW_MAXIMIZE);
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

	if(!m_bIsFullscreen) return;

	LPDIRECTDRAWCLIPPER pcClipper = NULL;
	if(bEnable == TRUE) {
		m_pDD->CreateClipper( 0, &pcClipper, NULL);
		pcClipper->SetHWnd(0, m_hWnd);
	}

	m_pddsFrontBuffer->SetClipper(pcClipper);

	if(pcClipper) pcClipper->Release();
}

HRESULT C3dDevice :: CreateEnvironment(GUID* pDriverGUID, GUID* pDeviceGUID,
									   DDSURFACEDESC2* pMode, DWORD dwFlags)
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
	if (!bf_render_device())
		return D3DFWERR_NO3DDEVICE;
	bf_gfx_state_init(&m_state);
	bf_render_bind_texture(0, NULL);
	bf_render_bind_texture(1, NULL);
	m_bInitialized = TRUE;

	return S_OK;
}

HRESULT C3dDevice :: CreateDirectDraw(GUID* pDriverGUID, DWORD dwFlags)
{
	DDSURFACEDESC2 ddsd;
	DWORD dwCoopFlags;

	if (FAILED(DirectDrawCreateEx(pDriverGUID, (VOID**)&m_pDD,
								  IID_IDirectDraw7, NULL)))
		return D3DFWERR_NODIRECTDRAW;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	m_pDD->GetDisplayMode(&ddsd);
	m_windowBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;

	dwCoopFlags = DDSCL_NORMAL;
	if (m_bIsFullscreen)
		dwCoopFlags = DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
	if (0L == (dwFlags & D3DFW_NO_FPUSETUP))
		dwCoopFlags |= DDSCL_FPUSETUP;
	dwCoopFlags |= DDSCL_MULTITHREADED;

	if (FAILED(m_pDD->SetCooperativeLevel(m_hWnd, dwCoopFlags)))
		return D3DFWERR_COULDNTSETCOOPLEVEL;
	return S_OK;
}

HRESULT C3dDevice :: CreateBuffers(DDSURFACEDESC2* pddsd, DWORD dwFlags)
{
	DDSURFACEDESC2 ddsd;
	DDSCAPS2 ddscaps;
	HRESULT hr;

	if (dwFlags & D3DFW_FULLSCREEN) {
		BOOL useModeX;

		SetRect(&m_rcViewportRect, 0, 0, pddsd->dwWidth, pddsd->dwHeight);
		m_rcScreenRect = m_rcViewportRect;
		m_dwRenderWidth = m_rcViewportRect.right;
		m_dwRenderHeight = m_rcViewportRect.bottom;

		useModeX = FALSE;
		if (m_dwRenderWidth == 320 && m_dwRenderHeight == 200 &&
			pddsd->ddpfPixelFormat.dwRGBBitCount == 8)
			useModeX = TRUE;

		if (FAILED(m_pDD->SetDisplayMode(m_dwRenderWidth, m_dwRenderHeight,
										 pddsd->ddpfPixelFormat.dwRGBBitCount,
										 pddsd->dwRefreshRate, useModeX)))
			return D3DFWERR_BADDISPLAYMODE;

		D3DUtil_InitSurfaceDesc(ddsd, DDSD_CAPS, 0);
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
		if (dwFlags & D3DFW_BACKBUFFER) {
			ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
			ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE |
								   DDSCAPS_COMPLEX | DDSCAPS_FLIP;
			ddsd.dwBackBufferCount = 1;
		}

		if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pddsFrontBuffer, NULL))) {
			if (hr == DDERR_OUTOFVIDEOMEMORY)
				return hr;
			return D3DFWERR_NOPRIMARY;
		}

		if (dwFlags & D3DFW_BACKBUFFER) {
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
			ddscaps.dwCaps2 = 0;
			ddscaps.dwCaps3 = 0;
			ddscaps.dwCaps4 = 0;
			if (FAILED(hr = m_pddsFrontBuffer->GetAttachedSurface(&ddscaps, &m_pddsBackBuffer))) {
				if (hr == DDERR_OUTOFVIDEOMEMORY)
					return hr;
				return D3DFWERR_NOBACKBUFFER;
			}
		}
	} else {
		LPDIRECTDRAWCLIPPER pcClipper;

		GetClientRect(m_hWnd, &m_rcViewportRect);
		GetClientRect(m_hWnd, &m_rcScreenRect);
		ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.left);
		ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.right);
		m_dwRenderWidth = m_rcViewportRect.right;
		m_dwRenderHeight = m_rcViewportRect.bottom;

		D3DUtil_InitSurfaceDesc(ddsd, DDSD_CAPS, 0);
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (!(dwFlags & D3DFW_BACKBUFFER))
			ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;

		if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pddsFrontBuffer, NULL))) {
			if (hr == DDERR_OUTOFVIDEOMEMORY)
				return hr;
			return D3DFWERR_NOPRIMARY;
		}

		if (FAILED(m_pDD->CreateClipper(0, &pcClipper, NULL)))
			return D3DFWERR_NOCLIPPER;

		pcClipper->SetHWnd(0, m_hWnd);
		m_pddsFrontBuffer->SetClipper(pcClipper);
		SAFE_RELEASE(pcClipper);

		if (dwFlags & D3DFW_BACKBUFFER) {
			ddsd.dwWidth = m_dwRenderWidth;
			ddsd.dwHeight = m_dwRenderHeight;
			ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
			ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
			if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pddsBackBuffer, NULL))) {
				if (hr == DDERR_OUTOFVIDEOMEMORY)
					return hr;
				return D3DFWERR_NOBACKBUFFER;
			}
		} else {
			ClientToScreen(m_hWnd, (POINT*)&m_rcViewportRect.left);
			ClientToScreen(m_hWnd, (POINT*)&m_rcViewportRect.right);
		}
	}

	D3DUtil_InitSurfaceDesc(m_ddsdFrameBuffer, DDSD_CAPS, 0);
	m_pddsFrontBuffer->GetSurfaceDesc(&m_ddsdFrameBuffer);

	m_pfRShiftR = 0, m_pfRShiftL = 8;
	m_pfGShiftR = 0, m_pfGShiftL = 8;
	m_pfBShiftR = 0, m_pfBShiftL = 8;
	m_pfAShiftR = 0, m_pfAShiftL = 8;
	m_pfABitMask = m_ddsdFrameBuffer.ddpfPixelFormat.dwRGBAlphaBitMask;
	m_pfRBitMask = m_ddsdFrameBuffer.ddpfPixelFormat.dwRBitMask;
	m_pfGBitMask = m_ddsdFrameBuffer.ddpfPixelFormat.dwGBitMask;
	m_pfBBitMask = m_ddsdFrameBuffer.ddpfPixelFormat.dwBBitMask;
	m_pfBitCount = m_ddsdFrameBuffer.ddpfPixelFormat.dwRGBBitCount;
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

	if (dwFlags & D3DFW_BACKBUFFER)
		m_pddsRenderTarget = m_pddsBackBuffer;
	else
		m_pddsRenderTarget = m_pddsFrontBuffer;
	m_pddsRenderTarget->AddRef();

	D3DUtil_InitSurfaceDesc(ddsd, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH |
								  DDSD_PIXELFORMAT, 0);
	ddsd.dwWidth = m_dwRenderWidth;
	ddsd.dwHeight = m_dwRenderHeight;
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pddsRenderBackup, NULL)))
		return E_OUTOFMEMORY;
	return S_OK;
}

void	C3dDevice :: DestroyRenderer(CRenderer *rc)
{
	delete	rc;
}

HRESULT C3dDevice :: FlipToGDISurface(BOOL bDrawFrame)
{
	if (m_pDD && m_bIsFullscreen) {
		m_pDD->FlipToGDISurface();
		if (bDrawFrame) {
			DrawMenuBar(m_hWnd);
			RedrawWindow(m_hWnd, NULL, NULL, RDW_FRAME);
		}
	}
	return S_OK;
}

HRESULT C3dDevice :: CloneFrame()
{
	if (!m_pddsFrontBuffer)
		return D3DFWERR_NOTINITIALIZED;
	if (!m_pddsBackBuffer)
		return m_pddsFrontBuffer->IsLost();

	if (m_bIsFullscreen)
		return m_pddsBackBuffer->Blt(NULL, m_pddsFrontBuffer, NULL, DDBLT_WAIT, NULL);

	GetClientRect(m_hWnd, &m_rcScreenRect);
	ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.left);
	ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.right);

	return m_pddsBackBuffer->Blt(&m_rcViewportRect, m_pddsFrontBuffer, &m_rcScreenRect, DDBLT_WAIT, NULL);
}

HRESULT C3dDevice :: ShowFrame(bool vertSync)
{
	if (m_pddsFrontBuffer == NULL)
		return D3DFWERR_NOTINITIALIZED;

	if (m_pddsBackBuffer == NULL)
		return m_pddsFrontBuffer->IsLost();

	if (vertSync) {
		if (m_bIsFullscreen && !m_bIsGDIObject)
			return m_pddsFrontBuffer->Flip(NULL, DDFLIP_WAIT);

		GetClientRect(m_hWnd, &m_rcScreenRect);
		ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.left);
		ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.right);
		m_pDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
		return m_pddsFrontBuffer->Blt(&m_rcScreenRect, m_pddsBackBuffer,
									  &m_rcViewportRect, DDBLT_WAIT, NULL);
	}

	GetClientRect(m_hWnd, &m_rcScreenRect);
	ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.left);
	ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.right);
	return m_pddsFrontBuffer->Blt(&m_rcScreenRect, m_pddsBackBuffer,
								  &m_rcViewportRect, DDBLT_WAIT, NULL);
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
	return m_pDD->RestoreAllSurfaces();
}

#if defined(BF_PORT)
// The port's resize path (M4.3a): the window's logical size has changed and
// the graphics device has been resized to it (port/native/view). The back
// buffer and the backup surface are made anew at the new size, as
// CreateBuffers does at start-up; the depth buffer is the gfx target's. The
// original fixed the size at start-up.
HRESULT C3dDevice :: BfResize(DWORD w, DWORD h)
{
	if (!m_pDD || !m_bInitialized || !m_pddsBackBuffer || m_bIsFullscreen)
		return D3DFWERR_NOTINITIALIZED;
	if (w == m_dwRenderWidth && h == m_dwRenderHeight)
		return S_OK;

	DDSURFACEDESC2 ddsd;
	IDirectDrawSurface7 *back = NULL, *backup = NULL;
	HRESULT hr;

	D3DUtil_InitSurfaceDesc(ddsd, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH, 0);
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &back, NULL)))
		return hr;

	D3DUtil_InitSurfaceDesc(ddsd, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH, 0);
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &backup, NULL))) {
		SAFE_RELEASE(back);
		return hr;
	}

	SAFE_RELEASE(m_pddsRenderTarget);
	SAFE_RELEASE(m_pddsBackBuffer);
	SAFE_RELEASE(m_pddsRenderBackup);
	m_pddsBackBuffer = back;
	m_pddsRenderTarget = back;
	m_pddsRenderTarget->AddRef();
	m_pddsRenderBackup = backup;

	SetRect(&m_rcViewportRect, 0, 0, w, h);
	GetClientRect(m_hWnd, &m_rcScreenRect);
	ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.left);
	ClientToScreen(m_hWnd, (POINT*)&m_rcScreenRect.right);
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
	DDCAPS_DX7 ddCaps;
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

	ddCaps.dwSize = sizeof(ddCaps);
	ddCaps.dwCaps = 0;
	ZeroMemory(&ddCaps.dwCaps2, sizeof(ddCaps) - 8);
	if (m_pDD->GetCaps(&ddCaps, NULL) == D3D_OK)
		m_bSupportBltStretch = (ddCaps.dwCaps & DDCAPS_BLTSTRETCH) ? TRUE : FALSE;

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

void C3dDevice :: BackupFrame()
{
	if (m_pddsRenderBackup && m_pddsRenderTarget) {
		if (m_pddsRenderBackup->Blt(NULL, m_pddsRenderTarget, NULL, DDBLT_ASYNC, NULL) < 0)
			m_pddsRenderBackup->Blt(NULL, m_pddsRenderTarget, NULL, DDBLT_WAIT, NULL);
	}
}

void C3dDevice :: RestoreFrame()
{
	if (m_pddsRenderTarget && m_pddsRenderBackup) {
		if (m_pddsRenderTarget->Blt(NULL, m_pddsRenderBackup, NULL, DDBLT_ASYNC, NULL) < 0)
			m_pddsRenderTarget->Blt(NULL, m_pddsRenderBackup, NULL, DDBLT_WAIT, NULL);
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
