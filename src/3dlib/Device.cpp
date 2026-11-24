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
extern DWORD   g_ddsdFlagMipmap;
extern DWORD   g_ddsCapsMipmap;
extern Registry2 g_registry;

bool g_isSupportLMTex = true;
C3dDevice g_3dDevice;
DWORD g_ddsdFlagMipmap = 0;
DWORD g_ddsCapsMipmap = 0;


extern int WINDOW_HEIGHT;
extern int WINDOW_WIDTH;
extern int WINDOW_X_OFFSET;
extern int WINDOW_Y_OFFSET;


void D3DUtil_InitSurfaceDesc(DDSURFACEDESC2& ddsd, DWORD dwFlags, DWORD dwCaps);
void D3DUtil_InitViewport(D3DVIEWPORT7& vp, DWORD dwWidth, DWORD dwHeight);
static HRESULT WINAPI EnumZBufferFormatsCallback(LPDDPIXELFORMAT pddpf, LPVOID pContext);

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
	m_pddsZBuffer      = NULL;
	m_pd3dDevice       = NULL;
	m_pDD              = NULL;
	m_pD3D             = NULL;
	m_dwDeviceMemType  = NULL;
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

	if (m_pd3dDevice) {
		if ((nD3D = m_pd3dDevice->Release()) > 0) {
			char tmp[80];
			sprintf(tmp, " : D3D object is still referenced %d times", nD3D);
			MessageBox(g_hMainWnd, tmp, "Error", 0);
		}
	}
	m_pd3dDevice = NULL;
	if (!m_bIsFullscreen)
		SAFE_RELEASE(m_pddsBackBuffer);

	SAFE_RELEASE(m_pddsZBuffer);
	SAFE_RELEASE(m_pddsRenderTarget);
	SAFE_RELEASE(m_pddsFrontBuffer);
	SAFE_RELEASE(m_pD3D);
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

	if (m_pd3dDevice == NULL)
		return hr;

	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)22, 1);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)23, 4);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)4, TRUE);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)25, 7);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)24, 0xf0);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)41, TRUE);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)7, TRUE);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)14, TRUE);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)19, 5);
	m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)20, 6);

	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)11, 0);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)1, 4);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)2, 2);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)3, 0);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)4, 4);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)5, 2);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)6, 0);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)11, 1);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)2, 2);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)1, 18);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)3, 1);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)4, 1);
	m_pd3dDevice->SetTextureStageState(2, (D3DTEXTURESTAGESTATETYPE)1, 1);
	m_pd3dDevice->SetTextureStageState(2, (D3DTEXTURESTAGESTATETYPE)4, 1);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)14, 1);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)17, 2);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)16, 2);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)17, 2);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)16, 2);
	m_pd3dDevice->SetTextureStageState(0, (D3DTEXTURESTAGESTATETYPE)12, 3);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)12, 4);
	m_pd3dDevice->SetTextureStageState(1, (D3DTEXTURESTAGESTATETYPE)15, 0xff000000);

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

	if (IsEqualIID(*pDeviceGUID, IID_IDirect3DHALDevice))
		m_dwDeviceMemType = DDSCAPS_VIDEOMEMORY;
	else if (IsEqualIID(*pDeviceGUID, IID_IDirect3DTnLHalDevice))
		m_dwDeviceMemType = DDSCAPS_VIDEOMEMORY;
	else
		m_dwDeviceMemType = DDSCAPS_SYSTEMMEMORY;

	hr = CreateDirectDraw(pDriverGUID, dwFlags);
	if (FAILED(hr))
		return hr;

	if (FAILED(hr = CreateBuffers(pMode, dwFlags)))
		return hr;

	if (pDeviceGUID)
		if (FAILED(hr = CreateDirect3D(pDeviceGUID, dwFlags)))
			return hr;

	if (!pDeviceGUID)
		return S_OK;

	if (FAILED(hr = CreateZBuffer()))
		return hr;

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

HRESULT C3dDevice::CreateDirect3D(GUID* pDeviceGUID, DWORD dwFlags)
{
	D3DVIEWPORT7 vdData;

	if (FAILED(m_pDD->QueryInterface(IID_IDirect3D7, (VOID**)&m_pD3D)))
		return D3DFWERR_NODIRECT3D;

	if (FAILED(m_pD3D->CreateDevice(*pDeviceGUID, m_pddsRenderTarget,
									&m_pd3dDevice)))
		return D3DFWERR_NO3DDEVICE;

	D3DUtil_InitViewport(vdData, m_dwRenderWidth, m_dwRenderHeight);
	if (FAILED(m_pd3dDevice->SetViewport(&vdData)))
		return D3DFWERR_NOVIEWPORT;

	ZeroMemory(&m_ddpfZBuffer, sizeof(m_ddpfZBuffer));
	if (dwFlags & D3DFW_STENCILBUFFER)
		m_ddpfZBuffer.dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
	else
		m_ddpfZBuffer.dwFlags = DDPF_ZBUFFER;

	m_pD3D->EnumZBufferFormats(*pDeviceGUID, EnumZBufferFormatsCallback,
							   &m_ddpfZBuffer);
	if (m_ddpfZBuffer.dwSize != sizeof(m_ddpfZBuffer))
		return D3DFWERR_NOZBUFFER;
	return S_OK;
}


static HRESULT WINAPI EnumZBufferFormatsCallback(LPDDPIXELFORMAT pddpf, LPVOID pddpfDesired)
{
	LPDDPIXELFORMAT pTarget = (LPDDPIXELFORMAT)pddpfDesired;
	if (!pddpf || !pTarget)
		return D3DENUMRET_CANCEL;

	if (pddpf->dwFlags == pTarget->dwFlags) {
		*pTarget = *pddpf;
		if (pddpf->dwRGBBitCount == 16)
			return D3DENUMRET_CANCEL;
	}
	return D3DENUMRET_OK;
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

HRESULT C3dDevice :: CreateZBuffer()
{
	DDSURFACEDESC2 ddsd;

	m_pd3dDevice->GetCaps(&m_ddDeviceDesc);
	if (m_ddDeviceDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
		return S_OK;

	D3DUtil_InitSurfaceDesc(ddsd, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 0);
	ddsd.ddsCaps.dwCaps = m_dwDeviceMemType | DDSCAPS_ZBUFFER;
	ddsd.dwWidth = m_dwRenderWidth;
	ddsd.dwHeight = m_dwRenderHeight;
	ddsd.ddpfPixelFormat = m_ddpfZBuffer;

	HRESULT hr = m_pDD->CreateSurface(&ddsd, &m_pddsZBuffer, NULL);
	if (FAILED(hr)) {
		if (hr == DDERR_OUTOFVIDEOMEMORY)
			return DDERR_OUTOFVIDEOMEMORY;
		return D3DFWERR_NOZBUFFER;
	}

	if (FAILED(m_pddsRenderTarget->AddAttachedSurface(m_pddsZBuffer)))
		return D3DFWERR_NOZBUFFER;

	return m_pd3dDevice->SetRenderTarget(m_pddsRenderTarget, 0) < 0 ? D3DFWERR_NOZBUFFER : S_OK;
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

HRESULT	C3dDevice :: Clear(DWORD color)
{
	D3DRECT		d;

	d.x1 = 0;
	d.x2 = m_dwRenderWidth;
	d.y1 = 0;
	d.y2 = m_dwRenderHeight;

	return	m_pd3dDevice->Clear(1UL, &d, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
		color, 1.0f, 0L);
}

HRESULT	C3dDevice :: ClearZBuffer()
{
	D3DRECT		d;

	d.x1 = 0;
	d.x2 = m_dwRenderWidth;
	d.y1 = 0;
	d.y2 = m_dwRenderHeight;

	return	m_pd3dDevice->Clear(1UL, &d, D3DCLEAR_ZBUFFER,
		0, 1.0f, 0L);
}

HRESULT C3dDevice :: RestoreSurfaces()
{
	Trace("HRESULT C3dDevice::RestoreSurfaces()\n");
	return m_pDD->RestoreAllSurfaces();
}

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

	if (m_pd3dDevice->GetCaps(&m_ddDeviceDesc) == D3D_OK)
	{
		m_dwMinTextureWidth			=	m_ddDeviceDesc.dwMinTextureWidth;
		m_dwMinTextureHeight		=	m_ddDeviceDesc.dwMinTextureHeight;
		m_dwMaxTextureWidth			=	m_ddDeviceDesc.dwMaxTextureWidth;
		m_dwMaxTextureHeight		=	m_ddDeviceDesc.dwMaxTextureHeight;
		m_dwMaxTextureAspectRatio	=	m_ddDeviceDesc.dwMaxTextureAspectRatio;
		g_isSupportLMTex = (m_ddDeviceDesc.wMaxTextureBlendStages >= 2) &&
			(m_ddDeviceDesc.dwTextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR);

		if (g_registry.m_trilinearFilter &&
			(m_ddDeviceDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR)) {
			g_ddsdFlagMipmap = DDSD_MIPMAPCOUNT;
			g_ddsCapsMipmap = DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
		} else {
			g_ddsdFlagMipmap = 0;
			g_ddsCapsMipmap = 0;
		}
	}
	m_bSupportTextureSurface = (1024 <= m_dwMaxTextureWidth) && (1024 <= m_dwMaxTextureHeight);

	CRenderer *	newRenderer = new CRenderer;
	newRenderer->SetPixelFormat(pixelFormat);
	newRenderer->m_device = m_pd3dDevice;
	newRenderer->SetSize(m_dwRenderWidth, m_dwRenderHeight);


	if (newRenderer->m_guardBandLeft == 0) {
		newRenderer->m_guardBandLeft	=	-640;
		newRenderer->m_guardBandRight	=	newRenderer->m_width+640;
		newRenderer->m_guardBandTop		=	-480;
		newRenderer->m_guardBandBottom	=	newRenderer->m_height+480;
	} else {
		newRenderer->m_guardBandLeft	=	m_ddDeviceDesc.dvGuardBandLeft;
		newRenderer->m_guardBandRight	=	m_ddDeviceDesc.dvGuardBandRight;
		newRenderer->m_guardBandTop		=	m_ddDeviceDesc.dvGuardBandTop;
		newRenderer->m_guardBandBottom	=	m_ddDeviceDesc.dvGuardBandBottom;
	}

	TestScreen();

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
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
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
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	return true;
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
	if (g_registry.m_trilinearFilter &&
		(m_ddDeviceDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR)) {
		g_ddsdFlagMipmap = DDSD_MIPMAPCOUNT;
		g_ddsCapsMipmap = DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	} else {
		g_ddsdFlagMipmap = 0;
		g_ddsCapsMipmap = 0;
	}
}

CDynamicVB :: CDynamicVB()
{
	m_vertCount = 0;
	m_vertOffset = 0;
	m_pVB = 0;
}

CDynamicVB :: ~CDynamicVB()
{
	Release();
}

void	CDynamicVB :: Release()
{
	if (m_pVB) m_pVB->Unlock();
	SAFE_RELEASE(m_pVB);
}

bool CDynamicVB::Init(const DWORD theFVF, DWORD theVertexCount, DWORD theVertexSize)
{
	Release();

	D3DVERTEXBUFFERDESC aDesc;
	IDirect3D7* d3d = g_3dDevice.GetDirect3D();
	m_vertCount = theVertexCount;
	m_vertSize = theVertexSize;
	m_vertOffset = 0;
	ZeroMemory(&aDesc, sizeof(aDesc));
	aDesc.dwSize = sizeof(aDesc);
	aDesc.dwCaps = 0x10000;
	aDesc.dwFVF = theFVF;
	aDesc.dwNumVertices = theVertexCount;

	return d3d->CreateVertexBuffer(&aDesc, &m_pVB, 0) == D3D_OK;
}
bool CDynamicVB::Lock(DWORD vertCount, void** ppLockedData)
{
	if (!m_pVB)
		return false;

	if (vertCount > m_vertCount)
		return false;

	if (m_vertOffset + vertCount <= m_vertCount) {
		if (m_pVB->Lock(0x2820, ppLockedData, NULL) != D3D_OK)
			return false;
		*ppLockedData = (BYTE*)*ppLockedData + m_vertSize * m_vertOffset;
		m_vertOffset += vertCount;
		return true;
	} else {
		if (m_pVB->Lock(0x2820, ppLockedData, NULL) != D3D_OK)
			return false;
		m_vertOffset = vertCount;
		return true;
	}
}

void	CDynamicVB :: Unlock()
{
	if (m_pVB)
		m_pVB->Unlock();
}

void CDynamicVB :: DrawPri(IDirect3DDevice7* device, DWORD vertCount)
{
	device->DrawPrimitiveVB(D3DPT_TRIANGLELIST, m_pVB, m_vertOffset - vertCount, vertCount, 0);
}
