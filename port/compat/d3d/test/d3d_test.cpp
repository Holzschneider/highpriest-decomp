// bifrost-compat-d3d-test: the DirectDraw 7 / Direct3D 7 adapter, driven
// through the Win32/COM names with the call sequences of src/3dlib (copied
// patterns, not the client's code): C3dDevice::Init and CreateRenderer,
// TestScreen, CTexture::Create/Update/UpdateMipmap/ClearSurface/CopyTexture
// in every format, the state sets of CRenderer::FlushRenderList for the
// opaque, lightmap, alpha, additive and UI lists, DrawPrimitive with
// tlvertex3d and lmtlvertex3d, Flip, BackupFrame/RestoreFrame, screenshot
// and screen-effect locks, CSurface::DrawSurface, a surface DC, and
// DestroyObjects' reference counts. Offscreen, no game data. One line per
// check; exit status 1 on any failure. Writes compat-d3d.png (synthetic
// textures only) into the output directory (argument, $BF_TEST_OUT, or the
// build directory).
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

#include <chrono>
#include <setjmp.h>
#include <signal.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "bf_compat_d3d.h"
#include "bf_gfx.h"

// ---- fakes of the gdi adapter's surface-DC functions -----------------------------------

struct FakeDC { uint32_t *bits; int w, h, pitch; bool dirty; RECT r; };
extern "C" HDC bf_compat_gdi_dc_for_bitmap(void *bits, int w, int h, int pitch, int bottom_up)
{
	if (!bits || w <= 0 || h <= 0 || bottom_up) return nullptr;
	return (HDC)new FakeDC{(uint32_t *)bits, w, h, pitch, false, {0, 0, 0, 0}};
}
extern "C" BOOL bf_compat_gdi_dc_take_dirty(HDC hdc, RECT *out)
{
	FakeDC *d = (FakeDC *)hdc;
	BOOL any = d->dirty;
	*out = any ? d->r : RECT{0, 0, 0, 0};
	d->dirty = false;
	return any;
}
extern "C" BOOL WINAPI DeleteDC(HDC hdc) { delete (FakeDC *)hdc; return TRUE; }
// "Text": what the gdi adapter promises, 0x00RRGGBB pixels and a dirty rectangle.
static void fake_text(HDC hdc, int x, int y, COLORREF c)
{
	FakeDC *d = (FakeDC *)hdc;
	static const char *glyph[7] = {"#####", "#....", "#....", "####.", "#....", "#....", "#...."};
	uint32_t px = uint32_t(GetRValue(c)) << 16 | uint32_t(GetGValue(c)) << 8 | GetBValue(c);
	for (int j = 0; j < 7; ++j)
		for (int i = 0; i < 5; ++i)
			if (glyph[j][i] == '#') d->bits[size_t(y + j) * size_t(d->pitch / 4) + size_t(x + i)] = px;
	d->dirty = true;
	d->r = RECT{x, y, x + 5, y + 7};
}

// ---- checks -----------------------------------------------------------------------------

static int g_fail = 0, g_pass = 0;
static void check(bool ok, const char *what, const std::string &detail = std::string())
{
	printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
	if (ok) ++g_pass; else ++g_fail;
}
static std::string hex(uint32_t v) { char b[16]; snprintf(b, sizeof b, "%08x", v); return b; }

static bf_gfx_device *g_gfx;
static uint32_t pix(int x, int y)
{
	uint32_t v = 0;
	bf_gfx_rect r{x, y, 1, 1};
	bf_gfx_read_pixels(g_gfx, BF_GFX_SPACE_LOGICAL, &r, &v, 0);
	return v | 0xFF000000u;
}
static bool close_to(uint32_t a, uint32_t b, int tol)
{
	for (int s = 0; s < 24; s += 8)
		if (abs(int((a >> s) & 0xFF) - int((b >> s) & 0xFF)) > tol) return false;
	return true;
}
static void check_px(const char *what, int x, int y, uint32_t want, int tol = 0)
{
	uint32_t got = pix(x, y);
	char d[96];
	snprintf(d, sizeof d, "(%d,%d) %s, expected %s", x, y, hex(got).c_str(), hex(want | 0xFF000000u).c_str());
	check(close_to(got, want, tol), what, d);
}

// ---- the client's structures -------------------------------------------------------------

struct tlvertex3d { float x, y, z, oow; DWORD color, specular; float tu, tv; };
struct lmtlvertex3d { tlvertex3d vert; float tu2, tv2; };
static_assert(sizeof(tlvertex3d) == 32 && sizeof(lmtlvertex3d) == 40, "vertex layouts");

enum PixelFormat { PF_A1R5G5B5, PF_A4R4G4B4, PF_R5G6B5, PF_R5G5B5, PF_A8R8G8B8, PF_BUMP, PF_LAST };

// C3dDevice's members as CreateBuffers computes them.
static int m_pfRShiftR, m_pfRShiftL, m_pfGShiftR, m_pfGShiftL, m_pfBShiftR, m_pfBShiftL;
static DWORD m_pfRBitMask, m_pfGBitMask, m_pfBBitMask, m_pfBitCount;
static int DecodePixelR(WORD c) { return int(((c & m_pfRBitMask) >> m_pfRShiftR) << m_pfRShiftL); }
static int DecodePixelG(WORD c) { return int(((c & m_pfGBitMask) >> m_pfGShiftR) << m_pfGShiftL); }
static int DecodePixelB(WORD c) { return int(((c & m_pfBBitMask) >> m_pfBShiftR) << m_pfBShiftL); }

static IDirectDraw7 *m_pDD;
static IDirect3D7 *m_pD3D;
static IDirect3DDevice7 *m_pd3dDevice;
static IDirectDrawSurface7 *m_pddsFrontBuffer, *m_pddsBackBuffer, *m_pddsRenderTarget, *m_pddsZBuffer, *m_pddsRenderBackup;
static DDPIXELFORMAT m_ddpfZBuffer, g_texPixelFormat[PF_LAST];
static DDSURFACEDESC2 m_ddsdFrameBuffer;
static D3DDEVICEDESC7 m_ddDeviceDesc;
static DWORD m_dwRenderWidth, m_dwRenderHeight;
static float m_uOffset = 0.5f, m_vOffset = 0.5f;

static HRESULT WINAPI EnumZBufferFormatsCallback(LPDDPIXELFORMAT pddpf, LPVOID pddpfDesired)
{
	LPDDPIXELFORMAT pTarget = (LPDDPIXELFORMAT)pddpfDesired;
	if (!pddpf || !pTarget) return D3DENUMRET_CANCEL;
	if (pddpf->dwFlags == pTarget->dwFlags) {
		*pTarget = *pddpf;
		if (pddpf->dwRGBBitCount == 16) return D3DENUMRET_CANCEL;
	}
	return D3DENUMRET_OK;
}

static HRESULT WINAPI TextureSearchCallback(DDPIXELFORMAT *pddpf, VOID *param)
{
	if (!pddpf || !param) return DDENUMRET_OK;
	PixelFormat pf = *(PixelFormat *)param;
	switch (pf) {
	case PF_A1R5G5B5:
		if ((pddpf->dwFlags & DDPF_ALPHAPIXELS) && pddpf->dwRGBBitCount == 16 && pddpf->dwRGBAlphaBitMask == 0x8000) { g_texPixelFormat[pf] = *pddpf; return DDENUMRET_CANCEL; }
		break;
	case PF_A4R4G4B4:
		if ((pddpf->dwFlags & DDPF_ALPHAPIXELS) && pddpf->dwRGBBitCount == 16 && pddpf->dwRGBAlphaBitMask == 0xf000) { g_texPixelFormat[pf] = *pddpf; return DDENUMRET_CANCEL; }
		break;
	case PF_R5G6B5:
		if (!(pddpf->dwFlags & DDPF_ALPHAPIXELS) && pddpf->dwRGBBitCount == 16 && pddpf->dwRBitMask == 0xf800) { g_texPixelFormat[pf] = *pddpf; return DDENUMRET_CANCEL; }
		break;
	case PF_A8R8G8B8:
		if ((pddpf->dwFlags & DDPF_ALPHAPIXELS) && pddpf->dwRGBBitCount == 32 && pddpf->dwRGBAlphaBitMask == 0xff000000) { g_texPixelFormat[pf] = *pddpf; return DDENUMRET_CANCEL; }
		break;
	default: break;
	}
	return DDENUMRET_OK;
}

static void InitSurfaceDesc(DDSURFACEDESC2 &ddsd, DWORD dwFlags, DWORD dwCaps)
{
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = dwFlags;
	ddsd.ddsCaps.dwCaps = dwCaps;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
}

// C3dDevice::Init, windowed, dwFlags = 6 (BACKBUFFER | ZBUFFER), as SetupDriver2.
static HRESULT device_init(HWND hwnd, GUID *pDeviceGUID)
{
	DDSURFACEDESC2 ddsd;
	HRESULT hr;
	if (FAILED(DirectDrawCreateEx(NULL, (VOID **)&m_pDD, IID_IDirectDraw7, NULL))) return 0x82000001;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	m_pDD->GetDisplayMode(&ddsd);
	check(ddsd.ddpfPixelFormat.dwRGBBitCount == 32 && ddsd.dwWidth == 320 && ddsd.dwHeight == 240,
	      "GetDisplayMode: the window's logical size, 32 bit", std::to_string(ddsd.dwWidth) + "x" + std::to_string(ddsd.dwHeight));
	if (FAILED(m_pDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL | DDSCL_FPUSETUP | DDSCL_MULTITHREADED))) return 0x82000002;

	// CreateBuffers, windowed: GetClientRect gives 320x240 here.
	m_dwRenderWidth = 320;
	m_dwRenderHeight = 240;
	InitSurfaceDesc(ddsd, DDSD_CAPS, 0);
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pddsFrontBuffer, NULL))) return 0x82000007;
	LPDIRECTDRAWCLIPPER pcClipper;
	if (FAILED(m_pDD->CreateClipper(0, &pcClipper, NULL))) return 0x82000008;
	pcClipper->SetHWnd(0, hwnd);
	m_pddsFrontBuffer->SetClipper(pcClipper);
	pcClipper->Release();
	ddsd.dwWidth = m_dwRenderWidth;
	ddsd.dwHeight = m_dwRenderHeight;
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pddsBackBuffer, NULL))) return 0x8200000a;

	InitSurfaceDesc(m_ddsdFrameBuffer, DDSD_CAPS, 0);
	m_pddsFrontBuffer->GetSurfaceDesc(&m_ddsdFrameBuffer);
	m_pfRShiftR = 0, m_pfRShiftL = 8;
	m_pfGShiftR = 0, m_pfGShiftL = 8;
	m_pfBShiftR = 0, m_pfBShiftL = 8;
	m_pfRBitMask = m_ddsdFrameBuffer.ddpfPixelFormat.dwRBitMask;
	m_pfGBitMask = m_ddsdFrameBuffer.ddpfPixelFormat.dwGBitMask;
	m_pfBBitMask = m_ddsdFrameBuffer.ddpfPixelFormat.dwBBitMask;
	m_pfBitCount = m_ddsdFrameBuffer.ddpfPixelFormat.dwRGBBitCount;
	DWORD mask;
	for (mask = m_pfRBitMask; mask && !(mask & 1); mask >>= 1) m_pfRShiftR++;
	for (; mask; mask >>= 1) m_pfRShiftL--;
	for (mask = m_pfGBitMask; mask && !(mask & 1); mask >>= 1) m_pfGShiftR++;
	for (; mask; mask >>= 1) m_pfGShiftL--;
	for (mask = m_pfBBitMask; mask && !(mask & 1); mask >>= 1) m_pfBShiftR++;
	for (; mask; mask >>= 1) m_pfBShiftL--;

	m_pddsRenderTarget = m_pddsBackBuffer;
	m_pddsRenderTarget->AddRef();
	InitSurfaceDesc(ddsd, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 0);
	ddsd.dwWidth = m_dwRenderWidth;
	ddsd.dwHeight = m_dwRenderHeight;
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pddsRenderBackup, NULL))) return E_OUTOFMEMORY;

	// CreateDirect3D
	if (FAILED(m_pDD->QueryInterface(IID_IDirect3D7, (VOID **)&m_pD3D))) return 0x82000003;
	if (FAILED(m_pD3D->CreateDevice(*pDeviceGUID, m_pddsRenderTarget, &m_pd3dDevice))) return 0x82000004;
	D3DVIEWPORT7 vdData;
	ZeroMemory(&vdData, sizeof(vdData));
	vdData.dwWidth = m_dwRenderWidth;
	vdData.dwHeight = m_dwRenderHeight;
	vdData.dvMaxZ = 1.0f;
	if (FAILED(m_pd3dDevice->SetViewport(&vdData))) return 0x82000006;
	ZeroMemory(&m_ddpfZBuffer, sizeof(m_ddpfZBuffer));
	m_ddpfZBuffer.dwFlags = DDPF_ZBUFFER;
	m_pD3D->EnumZBufferFormats(*pDeviceGUID, EnumZBufferFormatsCallback, &m_ddpfZBuffer);
	if (m_ddpfZBuffer.dwSize != sizeof(m_ddpfZBuffer)) return 0x82000005;
	check(m_ddpfZBuffer.dwZBufferBitDepth == 16, "EnumZBufferFormats: the callback stops at a 16-bit z-buffer");

	// CreateZBuffer
	m_pd3dDevice->GetCaps(&m_ddDeviceDesc);
	if (m_ddDeviceDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR) return S_OK;
	InitSurfaceDesc(ddsd, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 0);
	ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER;
	ddsd.dwWidth = m_dwRenderWidth;
	ddsd.dwHeight = m_dwRenderHeight;
	ddsd.ddpfPixelFormat = m_ddpfZBuffer;
	if (FAILED(m_pDD->CreateSurface(&ddsd, &m_pddsZBuffer, NULL))) return 0x82000005;
	if (FAILED(m_pddsRenderTarget->AddAttachedSurface(m_pddsZBuffer))) return 0x82000005;
	if (m_pd3dDevice->SetRenderTarget(m_pddsRenderTarget, 0) < 0) return 0x82000005;

	// The initial states of C3dDevice::Init, by number as written there.
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
	return S_OK;
}

// ---- textures as CTexture does them ------------------------------------------------------------

static DWORD g_ddsdFlagMipmap = 0, g_ddsCapsMipmap = 0;

struct Tex {
	IDirectDrawSurface7 *s = nullptr;
	DWORD w = 0, h = 0;
	PixelFormat pf = PF_A1R5G5B5;
};

static void AdjustTextureSize(DWORD *w, DWORD *h)
{
	DWORD width, height;
	for (width = 1; width < *w; width <<= 1);
	width = max(width, m_ddDeviceDesc.dwMinTextureWidth);
	width = min(width, m_ddDeviceDesc.dwMaxTextureWidth);
	for (height = 1; height < *h; height <<= 1);
	height = max(height, m_ddDeviceDesc.dwMinTextureHeight);
	height = min(height, m_ddDeviceDesc.dwMaxTextureHeight);
	*w = width;
	*h = height;
}

static bool tex_create(Tex &t, DWORD w, DWORD h, PixelFormat pf)
{
	t.w = w;
	t.h = h;
	t.pf = pf;
	AdjustTextureSize(&t.w, &t.h);
	if (g_texPixelFormat[pf].dwSize != sizeof(DDPIXELFORMAT)) m_pd3dDevice->EnumTextureFormats(TextureSearchCallback, &pf);
	if (g_texPixelFormat[pf].dwSize != sizeof(DDPIXELFORMAT)) return false;
	DDSURFACEDESC2 ddsd = {sizeof(DDSURFACEDESC2), 0};
	ddsd.dwFlags = g_ddsdFlagMipmap | DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE |
	               ((pf == PF_R5G6B5) ? DDSD_CKSRCBLT : 0);
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
	ddsd.ddsCaps.dwCaps = g_ddsCapsMipmap | DDSCAPS_TEXTURE;
	ddsd.dwWidth = t.w;
	ddsd.dwHeight = t.h;
	ddsd.dwTextureStage = 0;
	ddsd.ddpfPixelFormat = g_texPixelFormat[pf];
	ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = ddsd.ddpfPixelFormat.dwBBitMask | ddsd.ddpfPixelFormat.dwRBitMask;
	ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = ddsd.ddpfPixelFormat.dwBBitMask | ddsd.ddpfPixelFormat.dwRBitMask;
	ddsd.dwMipMapCount = 1;
	while ((1u << ddsd.dwMipMapCount) < t.w && (1u << ddsd.dwMipMapCount) < t.h) ++ddsd.dwMipMapCount;
	return SUCCEEDED(m_pDD->CreateSurface(&ddsd, &t.s, NULL));
}

static int g_mip_levels_seen;
static void tex_update_mipmap(Tex &t, RECT &src)
{
	DDSCAPS2 ddsCaps;
	ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
	IDirectDrawSurface7 *pddsSurface = t.s;
	ddsCaps.dwCaps2 = 0;
	RECT dest = src;
	ZeroMemory(&ddsCaps.dwCaps3, sizeof(DWORD) * 2);
	g_mip_levels_seen = 1;
	while (pddsSurface->GetAttachedSurface(&ddsCaps, &pddsSurface) >= 0) {
		++g_mip_levels_seen;
		dest.top >>= 1; dest.bottom >>= 1; dest.left >>= 1; dest.right >>= 1;
		if (dest.left != dest.right && dest.top != dest.bottom)
			if (pddsSurface->Blt(&dest, t.s, &src, DDBLT_ASYNC, (DDBLTFX *)0) < 0)
				pddsSurface->Blt(&dest, t.s, &src, DDBLT_WAIT, (DDBLTFX *)0);
		pddsSurface->Release();
	}
}

// CTexture::Update (whole texture, iter 1), the client's conversions.
static uint16_t conv(PixelFormat pf, uint32_t c, bool blackkey, bool *write)
{
	*write = true;
	uint32_t v;
	switch (pf) {
	case PF_A1R5G5B5:
		v = ((c >> 6) & 0x03e0) + ((c >> 3) & 0x001f) + ((c >> 16) & 0x8000) + ((c >> 9) & 0x7c00);
		if (v == 0xfc1f || (blackkey && (v & 0x7fff) == 0)) v = 0;
		return uint16_t(v);
	case PF_A4R4G4B4:
		v = ((c >> 16) & 0xf000) + ((c >> 12) & 0x0f00) + ((c >> 8) & 0x00f0) + ((c >> 4) & 0x000f);
		if ((v & 0x0fff) == 0x0f0f || (blackkey && (v & 0x0fff) == 0)) v = 0;
		return uint16_t(v);
	case PF_R5G6B5:
		v = ((c >> 5) & 0x07e0) + ((c >> 3) & 0x001f) + ((c >> 8) & 0xf800);
		if (blackkey && v == 0xf81f) *write = false;
		return uint16_t(v);
	default:
		return 0;
	}
}

static bool tex_update(Tex &t, const std::vector<uint32_t> &image, int width, int height, bool blackkey)
{
	DDSURFACEDESC2 ddsd = {sizeof(DDSURFACEDESC2), 0};
	if (FAILED(t.s->GetSurfaceDesc(&ddsd))) return false;
	RECT rtDest = {0, 0, width, height};
	if (FAILED(t.s->Lock(&rtDest, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL))) return false;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			uint32_t c = image[size_t(i) * size_t(width) + size_t(j)];
			if (t.pf == PF_A8R8G8B8) {
				((DWORD *)((BYTE *)ddsd.lpSurface + i * ddsd.lPitch))[j] = (blackkey && c == 0) ? 0 : c;
			} else {
				bool w;
				uint16_t v = conv(t.pf, c, blackkey, &w);
				if (w) ((WORD *)((BYTE *)ddsd.lpSurface + i * ddsd.lPitch))[j] = v;
			}
		}
	}
	t.s->Unlock(NULL);
	RECT srcRect = {0, 0, LONG(t.w), LONG(t.h)};
	tex_update_mipmap(t, srcRect);
	return true;
}

// What the gfx module shows for a texel the client wrote (widening by bit replication).
static uint32_t texel_argb(PixelFormat pf, uint32_t c)
{
	if (pf == PF_A8R8G8B8) return c;
	bool w;
	uint32_t v = conv(pf, c, false, &w);
	auto r5 = [](uint32_t x) { return (x << 3) | (x >> 2); };
	auto r6 = [](uint32_t x) { return (x << 2) | (x >> 4); };
	if (pf == PF_A1R5G5B5) return ((v & 0x8000) ? 0xFF000000u : 0) | r5((v >> 10) & 31) << 16 | r5((v >> 5) & 31) << 8 | r5(v & 31);
	if (pf == PF_A4R4G4B4) return ((v >> 12) & 15) * 17 << 24 | ((v >> 8) & 15) * 17 << 16 | ((v >> 4) & 15) * 17 << 8 | (v & 15) * 17;
	return 0xFF000000u | r5((v >> 11) & 31) << 16 | r6((v >> 5) & 63) << 8 | r5(v & 31);
}

// CTexture::DrawSurface's quad (UI, 1:1, texel offset m_uOffset).
static void quad(tlvertex3d *v, float x, float y, float w, float h, DWORD color, DWORD spec, float tw, float th, float z = 0.00001f)
{
	float u0 = m_uOffset / tw, v0 = m_vOffset / th, u1 = (w + m_uOffset) / tw, v1 = (h + m_vOffset) / th;
	v[0] = {x, y, z, 0.99999f, color, spec, u0, v0};
	v[1] = {x + w, y, z, 0.99999f, color, spec, u1, v0};
	v[2] = {x, y + h, z, 0.99999f, color, spec, u0, v1};
	v[3] = {x + w, y + h, z, 0.99999f, color, spec, u1, v1};
}

// ---- CRenderer::FlushRenderList's state sets -------------------------------------------------

static void state_opaque()   // start of FlushRenderList: FlushFaceList, FlushRawList
{
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	m_pd3dDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0xcf);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
}
static void state_lightmap_begin()
{
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xffffffff);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
	m_pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATEALPHA_ADDCOLOR);
}
static void state_lightmap_end()
{
	m_pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_AMBIENT, 0);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
}
static void state_alpha_depth()
{
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, TRUE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
}
static void state_emissive()
{
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
}
static void state_ui()   // the no-depth alpha list, after the effect list
{
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
}
static void state_end()
{
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_NONE);
}

// CRenderer::Lock as written: desc with dwSize and the rest zeroed, DDLOCK_WAIT.
static void *m_lpSurfacePtr;
static long m_lPitch;
static DWORD m_bRGBBitCount;
static int renderer_lock()
{
	DDSURFACEDESC2 desc;
	desc.dwSize = sizeof(desc);
	desc.dwFlags = 0;
	memset(&desc.dwHeight, 0, sizeof(desc) - 8);
	if (FAILED(m_pddsRenderTarget->Lock(NULL, &desc, DDLOCK_WAIT, NULL))) return 0;
	m_lpSurfacePtr = desc.lpSurface;
	m_bRGBBitCount = desc.ddpfPixelFormat.dwRGBBitCount;
	m_lPitch = desc.lPitch;
	return 1;
}
static void renderer_unlock() { m_pddsRenderTarget->Unlock(NULL); }

// C3dDevice::ShowFrame(true), windowed.
static HRESULT show_frame()
{
	RECT screen = {100, 100, 420, 340}, viewport = {0, 0, 320, 240};
	m_pDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
	return m_pddsFrontBuffer->Blt(&screen, m_pddsBackBuffer, &viewport, DDBLT_WAIT, NULL);
}

// CSurface::Create: an offscreen plain surface in the frame-buffer format with a magenta key.
static IDirectDrawSurface7 *csurface_create(DWORD w, DWORD h)
{
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_CKSRCBLT;
	ddsd.dwWidth = w + ((-(int)(w & 3)) & 3);
	ddsd.dwHeight = h + ((-(int)(h & 3)) & 3);
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.ddpfPixelFormat = m_ddsdFrameBuffer.ddpfPixelFormat;
	ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = ddsd.ddpfPixelFormat.dwBBitMask | ddsd.ddpfPixelFormat.dwRBitMask;
	ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = ddsd.ddpfPixelFormat.dwBBitMask | ddsd.ddpfPixelFormat.dwRBitMask;
	IDirectDrawSurface7 *s = nullptr;
	return SUCCEEDED(m_pDD->CreateSurface(&ddsd, &s, NULL)) ? s : nullptr;
}

// C3dDevice::TestScreen, as written; returns difOff.
static int test_screen()
{
	DWORD dwBuffer[16] = {0, 0, 0xffffffff, 0xffffffff, 0, 0, 0xffffffff, 0xffffffff,
	                      0xffffffff, 0xffffffff, 0, 0, 0xffffffff, 0xffffffff, 0, 0};
	IDirectDrawSurface7 *screen = csurface_create(m_dwRenderWidth, m_dwRenderHeight);
	Tex test;
	tex_create(test, 4, 4, PF_A1R5G5B5);
	tex_update(test, std::vector<uint32_t>(dwBuffer, dwBuffer + 16), 4, 4, false);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	m_pd3dDevice->SetTexture(0, test.s);
	DWORD i = 0, width = m_dwRenderWidth >> 2, height = m_dwRenderHeight >> 2;
	int difVal = -1024, difOff = 0x8080;
	while (i < 0x10000) {
		DDSURFACEDESC2 srcDesc;
		m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		if (FAILED(m_pd3dDevice->BeginScene())) return -1;
		for (DWORD j = 0; j < width * height; ++j) {
			if (int(i + j) > 0xffff) break;
			int x = int(j % width) * 4, y = int(j / width) * 4;
			float tu = float(int(i + j) % 256) * 0.00390625f, tv = float(int(i + j) / 256) * 0.00390625f;
			float u0 = tu / test.w, v0 = tv / test.h, u1 = (tu + 4) / test.w, v1 = (tv + 4) / test.h;
			tlvertex3d t[4] = {{float(x), float(y), 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, u0, v0},
			                   {float(x), float(y + 4), 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, u0, v1},
			                   {float(x + 4), float(y), 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, u1, v0},
			                   {float(x + 4), float(y + 4), 0.00001f, 0.99999f, 0xffffffff, 0xffffffff, u1, v1}};
			m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, t, 4, 0);
		}
		m_pd3dDevice->EndScene();
		screen->Blt(NULL, m_pddsRenderTarget, NULL, DDBLT_WAIT, NULL);
		srcDesc.dwSize = sizeof(srcDesc);
		srcDesc.dwFlags = 0;
		ZeroMemory(&srcDesc.dwHeight, sizeof(srcDesc) - 8);
		if (FAILED(screen->Lock(NULL, &srcDesc, DDLOCK_WAIT, NULL))) return -1;
		int byteCount = int(srcDesc.ddpfPixelFormat.dwRGBBitCount >> 3);
		for (DWORD j = 0; j < width * height; ++j) {
			if (int(i + j) > 0xffff) break;
			int white = 0, black = 0;
			for (int k = 0; k < 16; ++k) {
				int x = int(j % width) * 4 + k % 4, y = int(j / width) * 4 + k / 4;
				DWORD pixel;
				if (byteCount == 2) pixel = *(WORD *)((BYTE *)srcDesc.lpSurface + y * srcDesc.lPitch + x * 2);
				else pixel = *(DWORD *)((BYTE *)srcDesc.lpSurface + y * srcDesc.lPitch + x * byteCount);
				int sum = DecodePixelB((WORD)pixel) + DecodePixelG((WORD)pixel) + DecodePixelR((WORD)pixel);
				if (dwBuffer[k]) white += sum; else black += sum;
			}
			if (white >= 0x1680 && black <= 0x180) {
				int diff = white - black;
				if (diff > difVal) { difVal = diff; difOff = int(i + j); }
			}
		}
		screen->Unlock(NULL);
		i += width * height;
	}
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	m_pd3dDevice->SetTexture(0, NULL);
	test.s->Release();
	screen->Release();
	return difOff;
}

// A stand-in for the crash reporter: a fault handler installed before the
// adapter's, which must still receive faults outside the lock buffer.
static sigjmp_buf g_jump;
static volatile sig_atomic_t g_caught;
static void crash_reporter(int, siginfo_t *, void *) { g_caught = 1; siglongjmp(g_jump, 1); }

// ---- the test -----------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	std::string out = argc > 1 ? argv[1] : getenv("BF_TEST_OUT") ? getenv("BF_TEST_OUT") : BF_D3D_TEST_OUT;
	setenv("BF_NO_ALERTS", "1", 0);

	// DirectDrawCreateEx before the application set a device.
	IDirectDraw7 *early = nullptr;
	check(DirectDrawCreateEx(NULL, (VOID **)&early, IID_IDirectDraw7, NULL) == DDERR_NODIRECTDRAWHW && !early,
	      "DirectDrawCreateEx without a gfx device fails with DDERR_NODIRECTDRAWHW");

	bf_gfx_desc gd = {nullptr, 320, 240, 1};
	g_gfx = bf_gfx_create(&gd);
	if (!g_gfx) { printf("FAIL no Metal device\n"); return 1; }
	bf_compat_d3d_set_device(g_gfx);

	GUID hal = IID_IDirect3DHALDevice;
	HRESULT hr = device_init((HWND)0x1234, &hal);
	check(hr == S_OK && m_pd3dDevice, "C3dDevice::Init (windowed, back buffer, z-buffer) succeeds", hex(uint32_t(hr)));
	if (hr != S_OK) return 1;
	check(m_pfBitCount == 32 && m_pfRBitMask == 0xFF0000 && m_pfRShiftR == 16 && m_pfRShiftL == 0 && m_pfBShiftR == 0,
	      "frame buffer: 32-bit X8R8G8B8, the client's shifts follow");

	// C3dDevice::CreateRenderer's queries.
	DDCAPS_DX7 ddCaps;
	ddCaps.dwSize = sizeof(ddCaps);
	ddCaps.dwCaps = 0;
	ZeroMemory(&ddCaps.dwCaps2, sizeof(ddCaps) - 8);
	check(m_pDD->GetCaps((DDCAPS *)&ddCaps, NULL) == DD_OK && (ddCaps.dwCaps & DDCAPS_BLTSTRETCH), "IDirectDraw7::GetCaps: BLTSTRETCH");
	check(m_pd3dDevice->GetCaps(&m_ddDeviceDesc) == D3D_OK, "IDirect3DDevice7::GetCaps");
	bool lmtex = m_ddDeviceDesc.wMaxTextureBlendStages >= 2 && (m_ddDeviceDesc.dwTextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR);
	check(lmtex, "caps select the single-pass lightmap path (g_isSupportLMTex)");
	check(m_ddDeviceDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR, "caps: LINEARMIPLINEAR (trilinear option)");
	check(m_ddDeviceDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX, "caps: FOGVERTEX (InitSpecular fog factors)");
	check(m_ddDeviceDesc.dwMinTextureWidth == 1 && m_ddDeviceDesc.dwMaxTextureWidth >= 1024 && m_ddDeviceDesc.dwMaxTextureAspectRatio == 0,
	      "caps: texture sizes", std::to_string(m_ddDeviceDesc.dwMaxTextureWidth));
	DWORD passes = 0;
	check(m_pd3dDevice->ValidateDevice(&passes) == D3D_OK && passes == 1, "ValidateDevice");

	// TestScreen as written: the read-back the 32-bit frame buffer gives selects 0x8080 = 0.5 texel.
	long long l0, r0, w0, gr0, gr1;
	bf_gfx_get_stats(g_gfx, nullptr, nullptr, &gr0, nullptr);
	auto t0 = std::chrono::steady_clock::now();
	int difOff = test_screen();
	double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
	bf_gfx_get_stats(g_gfx, nullptr, nullptr, &gr1, nullptr);
	m_uOffset = float(difOff % 256) * 0.00390625f;
	m_vOffset = float(difOff / 256) * 0.00390625f;
	char tsd[128];
	snprintf(tsd, sizeof tsd, "difOff 0x%x, offsets %.4f/%.4f, %.0f ms, %lld read-backs", difOff, m_uOffset, m_vOffset, ms, gr1 - gr0);
	check(difOff == 0x8080 && m_uOffset == 0.5f && m_vOffset == 0.5f, "TestScreen selects the default texel offsets", tsd);

	// Textures in each format, filled as CTexture::Update fills them.
	const int TW = 32;
	std::vector<uint32_t> img[4];
	PixelFormat fmts[4] = {PF_A1R5G5B5, PF_A4R4G4B4, PF_R5G6B5, PF_A8R8G8B8};
	const char *fname[4] = {"A1R5G5B5", "A4R4G4B4", "R5G6B5", "A8R8G8B8"};
	for (int f = 0; f < 4; ++f) img[f].resize(TW * TW);
	for (int y = 0; y < TW; ++y)
		for (int x = 0; x < TW; ++x) {
			uint32_t i = uint32_t(y * TW + x);
			bool key = x >= 8 && x < 16 && y >= 8 && y < 16;
			img[0][i] = key ? 0xFFFF00FF : (((x / 4 + y / 4) & 1) ? 0xFFE03020 : 0xFF20C040);
			img[1][i] = uint32_t(x * 8) << 24 | uint32_t(255 - x * 8) << 16 | uint32_t(y * 8) << 8 | 0x80;
			img[2][i] = 0xFF000000u | uint32_t(x * 8) << 16 | uint32_t(y * 8) << 8 | uint32_t(255 - y * 8);
			img[3][i] = 0xFF000000u | uint32_t(255 - x * 8) << 16 | uint32_t(x * 4 + y * 4) << 8 | uint32_t((x ^ y) * 8);
		}
	Tex tex[4];
	for (int f = 0; f < 4; ++f) {
		bool ok = tex_create(tex[f], TW, TW, fmts[f]);
		DDSURFACEDESC2 d = {sizeof(DDSURFACEDESC2), 0};
		ok = ok && tex[f].s->GetSurfaceDesc(&d) == DD_OK && d.dwWidth == 32 && d.dwHeight == 32;
		ok = ok && tex_update(tex[f], img[f], TW, TW, false);
		check(ok, (std::string("CTexture::Create + Update: ") + fname[f]).c_str());
	}
	// Lock for reading (CDCBitmap::BltTexture2) sees what Update wrote.
	{
		DDSURFACEDESC2 sd = {sizeof(DDSURFACEDESC2), 0};
		bool ok = tex[0].s->Lock(NULL, &sd, DDLOCK_WAIT, NULL) == DD_OK;
		WORD k = ok ? ((WORD *)((BYTE *)sd.lpSurface + 9 * sd.lPitch))[9] : 1;
		WORD c = ok ? ((WORD *)sd.lpSurface)[0] : 0;
		if (ok) tex[0].s->Unlock(NULL);
		check(ok && k == 0 && c == 0x8000 + (0x20 >> 3) * 0x400 + (0xC0 >> 3) * 0x20 + (0x40 >> 3) && sd.lPitch >= 64,
		      "texture Lock (read): magenta became 0, colours as converted", hex(c));
	}

	// Mipmapped texture, as with the trilinear option.
	g_ddsdFlagMipmap = DDSD_MIPMAPCOUNT;
	g_ddsCapsMipmap = DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	Tex mip;
	bool mok = tex_create(mip, 64, 64, PF_A1R5G5B5) && tex_update(mip, std::vector<uint32_t>(64 * 64, 0xFFFFFFFF), 64, 64, false);
	check(mok && g_mip_levels_seen == 6, "mipmapped texture: UpdateMipmap walks the attached levels", std::to_string(g_mip_levels_seen) + " levels");
	g_ddsdFlagMipmap = g_ddsCapsMipmap = 0;

	// ---- a frame of CRenderer::DrawScene ----------------------------------------------------------
	const DWORD bg = 0xFF203040;
	m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bg, 1.0f, 0);   // ClearBackground (count 1 in the client)
	check(m_pd3dDevice->BeginScene() == D3D_OK, "BeginScene");
	tlvertex3d q[4];

	// Opaque list (alpha test 0xCF, no blending): each format at 1:1.
	state_opaque();
	for (int f = 0; f < 4; ++f) {
		m_pd3dDevice->SetTexture(0, tex[f].s);
		quad(q, float(8 + f * 40), 8, TW, TW, 0xFFFFFFFF, 0xFFFFFFFF, float(tex[f].w), float(tex[f].h), 0.5f);
		m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x1C4, q, 4, 0);
	}
	// Fog on an opaque face: the factor is the specular alpha (InitSpecular), fog colour 0.
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
	m_pd3dDevice->SetTexture(0, tex[2].s);
	quad(q, 168, 8, TW, TW, 0xFFFFFFFF, 0x80FFFFFF, float(tex[2].w), float(tex[2].h), 0.5f);
	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x1C4, q, 4, 0);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

	// Lightmap list: lmtlvertex3d, stage 1 MODULATEALPHA_ADDCOLOR, CULL_CW. Ground
	// faces run counter-clockwise on screen; a clockwise one is culled.
	std::vector<uint32_t> lmimg(TW * TW);
	for (int y = 0; y < TW; ++y)
		for (int x = 0; x < TW; ++x) lmimg[size_t(y * TW + x)] = 0x80000000u | uint32_t(x * 4) << 16 | uint32_t(y * 4);
	Tex lm;
	tex_create(lm, TW, TW, PF_A4R4G4B4);
	tex_update(lm, lmimg, TW, TW, false);
	state_lightmap_begin();
	m_pd3dDevice->SetTexture(0, tex[2].s);
	m_pd3dDevice->SetTexture(1, lm.s);
	auto lmquad = [&](float x, float y, bool ccw, lmtlvertex3d *v) {
		quad(q, x, y, TW, TW, 0xFFC0C0C0, 0xFFFFFFFF, 32, 32, 0.5f);
		int order[4] = {0, 2, 1, 3};   // (0,0) (0,1) (1,0) (1,1): counter-clockwise first triangle
		for (int i = 0; i < 4; ++i) {
			const tlvertex3d &s = q[ccw ? order[i] : i];
			v[i].vert = s;
			v[i].tu2 = s.tu;
			v[i].tv2 = s.tv;
		}
	};
	lmtlvertex3d lv[4];
	lmquad(8, 56, true, lv);
	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x2C4, lv, 4, 0);
	lmquad(48, 56, false, lv);
	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x2C4, lv, 4, 0);
	state_lightmap_end();
	m_pd3dDevice->SetTexture(1, NULL);

	// Alpha list: SRCALPHA / INVSRCALPHA, ALPHAREF 1 (A4R4G4B4 gradient over the background).
	state_alpha_depth();
	m_pd3dDevice->SetTexture(0, tex[1].s);
	quad(q, 88, 56, TW, TW, 0xFFFFFFFF, 0xFFFFFFFF, 32, 32, 0.5f);
	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x1C4, q, 4, 0);
	// Emissive list: ONE / ONE.
	state_emissive();
	m_pd3dDevice->SetTexture(0, tex[3].s);
	quad(q, 128, 56, TW, TW, 0xFF808080, 0xFFFFFFFF, 32, 32, 0.5f);
	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x1C4, q, 4, 0);
	// Indexed triangle list (DrawIndexedPrimitive from user memory).
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	quad(q, 168, 56, TW, TW, 0xFFFFFFFF, 0xFFFFFFFF, 32, 32, 0.5f);
	WORD idx[6] = {0, 1, 2, 2, 1, 3};
	m_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0x1C4, q, 4, idx, 6, 0);
	m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

	// UI list: no depth, SRCALPHA / INVSRCALPHA; a texture with vertex colour,
	// then DrawBoxScreen's untextured box (the dummy texture has no surface).
	state_ui();
	m_pd3dDevice->SetTexture(0, tex[0].s);
	quad(q, 8, 104, TW, TW, 0xFFFFFF80, 0xFFFFFFFF, 32, 32);
	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x1C4, q, 4, 0);
	m_pd3dDevice->SetTexture(0, NULL);
	quad(q, 48, 104, 64, 32, 0x80000000, 0xFFFFFFFF, 1, 1);
	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x1C4, q, 4, 0);
	state_end();
	check(m_pd3dDevice->EndScene() == D3D_OK, "EndScene");

	// Expected pixels.
	for (int f = 0; f < 4; ++f) {
		int x0 = 8 + f * 40, y0 = 8;
		bool ok = true;
		std::string bad;
		for (int y = 0; y < TW && ok; ++y)
			for (int x = 0; x < TW && ok; ++x) {
				uint32_t t = texel_argb(fmts[f], img[f][size_t(y * TW + x)]);
				uint32_t want = (t >> 24) >= 0xCF ? t : bg;   // alpha test GREATEREQUAL 0xCF
				uint32_t got = pix(x0 + x, y0 + y);
				if (!close_to(got, want, 0)) { ok = false; bad = "(" + std::to_string(x) + "," + std::to_string(y) + ") " + hex(got) + " expected " + hex(want | 0xFF000000u); }
			}
		check(ok, (std::string("opaque list, 1:1 texels exact, alpha test 0xCF: ") + fname[f]).c_str(), bad);
	}
	{
		uint32_t t = texel_argb(PF_R5G6B5, img[2][5 * TW + 20]);
		float fo = 128.0f / 255.0f;
		uint32_t want = 0;
		for (int s = 0; s < 24; s += 8) want |= uint32_t(lroundf(((t >> s) & 0xFF) * fo)) << s;
		check_px("fog: colour x specular alpha, toward the fog colour", 168 + 20, 8 + 5, want, 1);
	}
	{
		bool ok = true;
		std::string bad;
		for (int y = 0; y < TW && ok; y += 3)
			for (int x = 0; x < TW && ok; x += 3) {
				uint32_t t = texel_argb(PF_R5G6B5, img[2][size_t(y * TW + x)]), l = texel_argb(PF_A4R4G4B4, lmimg[size_t(y * TW + x)]);
				float la = float(l >> 24) / 255.0f;
				uint32_t want = 0;
				for (int s = 0; s < 24; s += 8) {
					float c = float((t >> s) & 0xFF) * float(0xC0) / 255.0f;
					float v = float((l >> s) & 0xFF) + la * c;
					want |= uint32_t(fminf(255.0f, lroundf(v))) << s;
				}
				uint32_t got = pix(8 + x, 56 + y);
				if (!close_to(got, want, 2)) { ok = false; bad = "(" + std::to_string(x) + "," + std::to_string(y) + ") " + hex(got) + " expected " + hex(want | 0xFF000000u); }
			}
		check(ok, "lightmap list: lightmap.rgb + lightmap.a x texture x diffuse", bad);
		check_px("lightmap list: a clockwise face is culled (CULL_CW)", 48 + 16, 56 + 16, bg);
	}
	{
		uint32_t t = texel_argb(PF_A4R4G4B4, img[1][10 * TW + 25]);
		float a = float(t >> 24) / 255.0f;
		uint32_t want = 0;
		for (int s = 0; s < 24; s += 8) want |= uint32_t(lroundf(((t >> s) & 0xFF) * a + ((bg >> s) & 0xFF) * (1 - a))) << s;
		check_px("alpha list: SRCALPHA/INVSRCALPHA over the background", 88 + 25, 56 + 10, want, 2);
		uint32_t e = img[3][7 * TW + 9];
		uint32_t w2 = 0;
		for (int s = 0; s < 24; s += 8) w2 |= uint32_t((std::min)(255L, lroundf(((e >> s) & 0xFF) * 128.0f / 255.0f) + long((bg >> s) & 0xFF))) << s;
		check_px("emissive list: ONE/ONE adds texture x diffuse", 128 + 9, 56 + 7, w2, 1);
		check_px("DrawIndexedPrimitive: triangle list from user memory", 168 + 3, 56 + 30, img[3][30 * TW + 3]);
	}
	{
		uint32_t t = texel_argb(PF_A1R5G5B5, img[0][2 * TW + 2]);
		uint32_t want = (t & 0xFFFF00) | ((t & 0xFF) * 0x80 / 255);
		check_px("UI list: texture x vertex colour", 8 + 2, 104 + 2, want, 1);
		check_px("UI list: magenta (key) texels are transparent", 8 + 10, 104 + 10, bg);
		uint32_t half = 0;
		for (int s = 0; s < 24; s += 8) half |= uint32_t(lroundf(((bg >> s) & 0xFF) * (1 - 128.0f / 255.0f))) << s;
		check_px("DrawBoxScreen: unbound texture reads white (black box, alpha 0x80)", 48 + 30, 104 + 10, half, 1);
	}

	// CTexture::ClearSurface (COLORFILL, value in the texture's format) and CopyTexture (stretch).
	{
		DDBLTFX fx;
		fx.dwSize = sizeof(DDBLTFX);
		fx.dwFillColor = 0;
		RECT r = {0, 0, 4, 4};
		bool ok = tex[3].s->Blt(&r, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx) == DD_OK;
		DDSURFACEDESC2 sd = {sizeof(DDSURFACEDESC2), 0};
		ok = ok && tex[3].s->Lock(NULL, &sd, DDLOCK_WAIT | DDLOCK_READONLY, NULL) == DD_OK;
		DWORD v0 = ok ? ((DWORD *)sd.lpSurface)[0] : 1, v5 = ok ? ((DWORD *)sd.lpSurface)[5] : 1;
		if (ok) tex[3].s->Unlock(NULL);
		check(ok && v0 == 0 && v5 == img[3][5], "CTexture::ClearSurface: COLORFILL of a rectangle");

		Tex glyph;
		tex_create(glyph, 64, 32, PF_A1R5G5B5);
		RECT sr = {0, 0, 32, 32}, dr = {0, 0, 64, 32};
		ok = glyph.s->Blt(&dr, tex[0].s, &sr, DDBLT_WAIT, NULL) == DD_OK;
		ok = ok && glyph.s->Lock(NULL, &sd, DDLOCK_WAIT, NULL) == DD_OK;
		WORD a = ok ? ((WORD *)sd.lpSurface)[2 * 3 + 1] : 0, b = 0;
		if (ok) tex[0].s->Lock(NULL, &sd, DDLOCK_WAIT | DDLOCK_READONLY, NULL), b = ((WORD *)sd.lpSurface)[3], tex[0].s->Unlock(NULL);
		glyph.s->Unlock(NULL);
		check(ok && a == b, "CTexture::CopyTexture: stretched texture-to-texture Blt (nearest)");
		glyph.s->Release();
	}

	// CSurface::DrawSurface: a keyed offscreen surface onto the render target with BltFast.
	{
		IDirectDrawSurface7 *cs = csurface_create(16, 16);
		DDSURFACEDESC2 sd = {sizeof(DDSURFACEDESC2), 0};
		RECT r = {0, 0, 16, 16};
		bool ok = cs && cs->Lock(&r, &sd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL) == DD_OK;
		for (int y = 0; ok && y < 16; ++y)
			for (int x = 0; x < 16; ++x)
				((DWORD *)((BYTE *)sd.lpSurface + y * sd.lPitch))[x] = (x < 8) ? 0x00FF00FF : 0xFFFFD000;
		if (ok) cs->Unlock(NULL);
		RECT src = {0, 0, 16, 16};
		ok = ok && m_pddsRenderTarget->BltFast(128, 104, cs, &src, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY) == DD_OK;
		check(ok, "CSurface::DrawSurface: BltFast with the source colour key");
		check_px("BltFast: key pixels leave the target", 128 + 3, 104 + 3, bg);
		check_px("BltFast: other pixels are copied", 128 + 12, 104 + 3, 0xFFFFD000);
		if (cs) cs->Release();
	}

	// CRenderer::TextOutScreen: a DC on the render target, text through the gdi adapter.
	{
		long long rb0, rb1;
		bf_gfx_get_stats(g_gfx, nullptr, nullptr, &rb0, nullptr);
		m_pddsRenderTarget->AddRef();
		HDC hdc = nullptr;
		bool ok = m_pddsRenderTarget->GetDC(&hdc) == DD_OK && hdc;
		bf_gfx_get_stats(g_gfx, nullptr, nullptr, &rb1, nullptr);
		if (ok) fake_text(hdc, 152, 104, RGB(255, 255, 255));
		ok = ok && m_pddsRenderTarget->ReleaseDC(hdc) == DD_OK;
		m_pddsRenderTarget->Release();
		check(ok && rb1 == rb0, "render-target GetDC reads nothing back");
		check_px("ReleaseDC: drawn text reaches the target", 152, 104, 0xFFFFFF);
		check_px("ReleaseDC: pixels GDI did not touch are kept", 153, 105, bg);
	}

	// CRenderer::Flip: Lock/Unlock touching nothing, then ShowFrame. No read-back.
	long long r1, w1, l1;
	bf_compat_d3d_get_stats(&l0, &r0, &w0);
	bf_gfx_get_stats(g_gfx, nullptr, nullptr, &gr0, nullptr);
	for (int i = 0; i < 3; ++i) {
		if (renderer_lock()) renderer_unlock();
		show_frame();
	}
	bf_compat_d3d_get_stats(&l1, &r1, &w1);
	bf_gfx_get_stats(g_gfx, nullptr, nullptr, &gr1, nullptr);
	check(l1 - l0 == 3 && r1 == r0 && w1 == w0 && gr1 == gr0, "Flip: three render-target locks, no read-back, no write-back",
	      std::to_string(l1 - l0) + " locks, " + std::to_string(gr1 - gr0) + " gfx read-backs");
	check(show_frame() == DD_OK, "ShowFrame: Blt back buffer -> primary presents");
	check(m_pddsBackBuffer->Blt(NULL, m_pddsFrontBuffer, NULL, DDBLT_WAIT, NULL) == DD_OK, "CloneFrame: primary -> back buffer is accepted");

	// Capture now: the frame is complete.
	std::string png = out + "/compat-d3d.png";
	check(bf_gfx_capture_png(g_gfx, png.c_str()) == BF_GFX_OK, "capture", png);

	// SaveBmp-style screenshot: lock and read through the pointer.
	{
		bool ok = renderer_lock() && m_bRGBBitCount == 32 && m_lPitch >= 320 * 4;
		DWORD p = ok ? *(DWORD *)((BYTE *)m_lpSurfacePtr + (8 + 3) * m_lPitch + (8 + 40 * 3 + 5) * 4) : 0;
		if (ok) renderer_unlock();
		bf_compat_d3d_get_stats(&l1, &r1, &w1);
		check(ok && (p & 0xFFFFFF) == (pix(8 + 40 * 3 + 5, 8 + 3) & 0xFFFFFF) && r1 == r0 + 1 && w1 == w0,
		      "screenshot lock: the first read brings the frame back once, nothing is written", hex(p));
	}
	// ScreenEffect-style read-modify-write: written back on unlock.
	{
		bool ok = renderer_lock();
		if (ok) {
			DWORD *row = (DWORD *)((BYTE *)m_lpSurfacePtr + 200 * m_lPitch);
			for (int x = 0; x < 320; ++x) row[x] = row[x] ^ 0x00FFFFFF;
			renderer_unlock();
		}
		bf_compat_d3d_get_stats(&l1, &r1, &w1);
		check(ok && w1 == w0 + 1, "screen-effect lock: a written frame goes back on unlock");
		check_px("screen-effect lock: the written row is on the target", 10, 200, (bg ^ 0xFFFFFF));
		check_px("screen-effect lock: the rest is unchanged", 10, 199, bg);
	}

	// Faults outside the lock buffer go to the handler the adapter replaced.
	{
		struct sigaction sa, old_bus, old_segv;
		memset(&sa, 0, sizeof sa);
		sa.sa_sigaction = crash_reporter;
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);
		sigaction(SIGBUS, &sa, &old_bus);
		sigaction(SIGSEGV, &sa, &old_segv);
		if (renderer_lock()) renderer_unlock();   // the adapter installs its handler on top
		struct sigaction now;
		sigaction(SIGSEGV, nullptr, &now);
		bool replaced = now.sa_sigaction != crash_reporter;
		g_caught = 0;
		if (sigsetjmp(g_jump, 1) == 0) *(volatile int *)(uintptr_t)16 = 1;
		check(replaced && g_caught, "a fault elsewhere reaches the handler installed before the adapter's");
		sigaction(SIGBUS, &old_bus, nullptr);
		sigaction(SIGSEGV, &old_segv, nullptr);
	}

	// Mode.cpp's fade: BackupFrame once, then RestoreFrame + a box each step. No read-back.
	{
		bf_gfx_get_stats(g_gfx, nullptr, nullptr, &gr0, nullptr);
		uint32_t before = pix(20, 20);
		bf_gfx_get_stats(g_gfx, nullptr, nullptr, &gr0, nullptr);
		if (m_pddsRenderBackup->Blt(NULL, m_pddsRenderTarget, NULL, DDBLT_ASYNC, NULL) < 0)
			m_pddsRenderBackup->Blt(NULL, m_pddsRenderTarget, NULL, DDBLT_WAIT, NULL);
		m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF00FF00, 1.0f, 0);
		if (m_pddsRenderTarget->Blt(NULL, m_pddsRenderBackup, NULL, DDBLT_ASYNC, NULL) < 0)
			m_pddsRenderTarget->Blt(NULL, m_pddsRenderBackup, NULL, DDBLT_WAIT, NULL);
		bf_gfx_get_stats(g_gfx, nullptr, nullptr, &gr1, nullptr);
		check(gr1 == gr0, "BackupFrame / RestoreFrame stay on the GPU");
		check_px("RestoreFrame brings the backed-up frame back", 20, 20, before);
		// The backup surface read on the CPU after the target changed.
		m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF0000FF, 1.0f, 0);
		DDSURFACEDESC2 sd = {sizeof(DDSURFACEDESC2), 0};
		bool ok = m_pddsRenderBackup->Lock(NULL, &sd, DDLOCK_WAIT | DDLOCK_READONLY, NULL) == DD_OK;
		DWORD p = ok ? ((DWORD *)((BYTE *)sd.lpSurface + 20 * sd.lPitch))[20] : 0;
		if (ok) m_pddsRenderBackup->Unlock(NULL);
		check(ok && (p & 0xFFFFFF) == (before & 0xFFFFFF), "backup surface Lock: the backed-up pixels", hex(p));
		check_px("... and the current frame is left as it was", 20, 20, 0xFF0000FF);
	}

	// Clear with a rectangle (D3DRECT), and ZBUFFER-only clear (CRenderer::Clear(FALSE)).
	{
		D3DRECT d;
		d.x1 = 10; d.y1 = 10; d.x2 = 20; d.y2 = 20;
		m_pd3dDevice->Clear(1, &d, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFF0000, 1.0f, 0);
		check_px("Clear with a rectangle: inside", 15, 15, 0xFF0000);
		check_px("Clear with a rectangle: outside", 25, 15, 0x0000FF);
		m_pd3dDevice->Clear(1, &d, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		check_px("Clear of the z-buffer leaves colour", 15, 15, 0xFF0000);
	}

	// State read-back, refcounted SetTexture, unsupported paths that must not fail hard.
	{
		DWORD v = 0;
		m_pd3dDevice->GetRenderState(D3DRENDERSTATE_ALPHAREF, &v);
		DWORD t = 0;
		m_pd3dDevice->GetTextureStageState(1, D3DTSS_BORDERCOLOR, &t);
		check(v == 1 && t == 0xff000000, "GetRenderState / GetTextureStageState return what was set");
		D3DMATRIX m;
		memset(&m, 0, sizeof m);
		m._11 = m._22 = m._33 = m._44 = 2;
		check(m_pd3dDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &m) == D3D_OK, "SetTransform (no effect on TL vertices)");
		D3DLIGHT7 light;
		memset(&light, 0, sizeof light);
		light.dltType = D3DLIGHT_DIRECTIONAL;
		check(m_pd3dDevice->SetLight(0, &light) == D3D_OK && m_pd3dDevice->LightEnable(0, TRUE) == D3D_OK, "SetLight / LightEnable");
	}

	// ---- CBink::BinkShowNextFrame: a movie written at the top left of the back buffer,
	// then Blt to the primary from its rectangle only: stretched over the window.
	{
		bool ok = renderer_lock() && m_bRGBBitCount == 32;
		for (int y = 0; ok && y < 240; ++y)
			for (int x = 0; x < 320; ++x)
				*(DWORD *)((BYTE *)m_lpSurfacePtr + y * m_lPitch + x * 4) =
				    (x < 160 && y < 120) ? (x < 80 ? 0x00FF0000u : 0x000000FFu) : 0x0000FF00u;
		if (ok) renderer_unlock();
		RECT src = {0, 0, 160, 120}, dst = {100, 100, 420, 340};   // dst: the client area on the screen
		state_ui();
		m_pd3dDevice->SetTexture(0, tex[0].s);   // bound before; the present draws with its own
		HRESULT hr = m_pddsFrontBuffer->Blt(&dst, m_pddsBackBuffer, &src, DDBLT_WAIT, NULL);
		check(ok && hr == DD_OK && close_to(pix(5, 5), 0xFFFF0000u, 1) && close_to(pix(150, 230), 0xFFFF0000u, 1)
		          && close_to(pix(170, 5), 0xFF0000FFu, 1) && close_to(pix(315, 235), 0xFF0000FFu, 1),
		      "Blt of a part of the back buffer to the primary stretches it over the window (CBink)",
		      hex(pix(5, 5)) + " " + hex(pix(150, 230)) + " " + hex(pix(170, 5)) + " " + hex(pix(315, 235)));
		uint32_t before = pix(200, 200);
		RECT all = {0, 0, 320, 240};
		m_pddsFrontBuffer->Blt(&dst, m_pddsBackBuffer, &all, DDBLT_WAIT, NULL);
		check(pix(200, 200) == before, "Blt of the whole back buffer presents it unchanged");
		// The next draw of the client gets its own texture and state back: the
		// UI state modulates the texture with the vertex colour.
		tlvertex3d q[4];
		m_pd3dDevice->BeginScene();
		quad(q, 8, 104, TW, TW, 0xFF808080, 0xFFFFFFFF, 32, 32);
		m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0x1C4, q, 4, 0);
		m_pd3dDevice->EndScene();
		state_end();
		m_pd3dDevice->SetTexture(0, NULL);
		uint32_t t = texel_argb(PF_A1R5G5B5, img[0][2 * TW + 2]), want = 0xFF000000u;
		for (int s = 0; s < 24; s += 8) want |= uint32_t(lroundf(((t >> s) & 0xFF) * 128.0f / 255.0f)) << s;
		check_px("after the stretched present the client's texture and state are back", 8 + 2, 104 + 2, want, 1);
	}

	// ---- C3dDevice::DestroyObjects: the counts the client checks -----------------------------------
	for (int f = 0; f < 4; ++f) tex[f].s->Release();
	lm.s->Release();
	mip.s->Release();
	ULONG nD3D = m_pd3dDevice->Release();
	m_pddsBackBuffer->Release();
	m_pddsZBuffer->Release();
	m_pddsRenderTarget->Release();
	m_pddsFrontBuffer->Release();
	m_pD3D->Release();
	m_pddsRenderBackup->Release();
	m_pDD->SetCooperativeLevel((HWND)0x1234, DDSCL_NORMAL);
	ULONG nDD = m_pDD->Release();
	check(nD3D == 0 && nDD == 0, "DestroyObjects: device and DirectDraw are released to 0",
	      std::to_string(nD3D) + ", " + std::to_string(nDD));

	// A second device at UI scale 2: the client still sees the logical size.
	bf_compat_d3d_set_device(nullptr);
	bf_gfx_destroy(g_gfx);
	bf_gfx_desc gd2 = {nullptr, 400, 300, 2};
	g_gfx = bf_gfx_create(&gd2);
	bf_compat_d3d_set_device(g_gfx);
	{
		IDirectDraw7 *dd = nullptr;
		DDSURFACEDESC2 d;
		ZeroMemory(&d, sizeof d);
		d.dwSize = sizeof d;
		bool ok = DirectDrawCreateEx(NULL, (VOID **)&dd, IID_IDirectDraw7, NULL) == DD_OK && dd->GetDisplayMode(&d) == DD_OK;
		check(ok && d.dwWidth == 400 && d.dwHeight == 300, "UI scale 2: the display mode is the logical size",
		      std::to_string(d.dwWidth) + "x" + std::to_string(d.dwHeight));
		if (dd) dd->Release();
	}
	bf_compat_d3d_set_device(nullptr);
	bf_gfx_destroy(g_gfx);

	printf("%d passed, %d failed\n", g_pass, g_fail);
	return g_fail ? 1 : 0;
}
