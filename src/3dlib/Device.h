#ifndef HIGHPRIEST_3DLIB_DEVICE_H
#define HIGHPRIEST_3DLIB_DEVICE_H

#include "../common.h"

class CDC;
class CRenderer;
class CSurface;

enum PixelFormat {
	PF_A1R5G5B5, PF_A4R4G4B4, PF_R5G6B5, PF_R5G5B5, PF_A8R8G8B8, PF_BUMP, PF_LAST, PF_UNSUPPORTED = -1
};


const	DWORD D3DFW_FULLSCREEN    = 0x00000001;
const	DWORD D3DFW_BACKBUFFER    = 0x00000002;
const	DWORD D3DFW_ZBUFFER       = 0x00000004;
const	DWORD D3DFW_STENCILBUFFER = 0x00000008;
const	DWORD D3DFW_NO_FPUSETUP   = 0x00000010;


const	HRESULT D3DFWERR_INITIALIZATIONFAILED = 0x82000000;
const	HRESULT D3DFWERR_NODIRECTDRAW         = 0x82000001;
const	HRESULT D3DFWERR_COULDNTSETCOOPLEVEL  = 0x82000002;
const   HRESULT D3DFWERR_NODIRECT3D           = 0x82000003;
const	HRESULT D3DFWERR_NO3DDEVICE           = 0x82000004;
const	HRESULT D3DFWERR_NOZBUFFER            = 0x82000005;
const	HRESULT D3DFWERR_NOVIEWPORT           = 0x82000006;
const	HRESULT D3DFWERR_NOPRIMARY            = 0x82000007;
const	HRESULT D3DFWERR_NOCLIPPER            = 0x82000008;
const	HRESULT D3DFWERR_BADDISPLAYMODE       = 0x82000009;
const	HRESULT D3DFWERR_NOBACKBUFFER         = 0x8200000a;
const	HRESULT D3DFWERR_NONZEROREFCOUNT      = 0x8200000b;
const	HRESULT D3DFWERR_NORENDERTARGET       = 0x8200000c;
const	HRESULT D3DFWERR_INVALIDMODE          = 0x8200000d;
const	HRESULT D3DFWERR_NOTINITIALIZED       = 0x8200000e;

const	DWORD COLORKEY_R = 10;
const	DWORD COLORKEY_G = 10;
const	DWORD COLORKEY_B = 10;

class CDynamicVB {
public:
	CDynamicVB();
	~CDynamicVB();
	bool Init(const unsigned long theFVF, unsigned long theVertexCount, unsigned long theVertexSize);
	void Release();
	bool Lock(unsigned long vertCount, void** ppLockedData);
	void Unlock();
	IDirect3DVertexBuffer7* GetInterface() const { return m_pVB; }
	unsigned long GetSize() { return m_vertCount; }
	void DrawPri(IDirect3DDevice7* device, unsigned long vertCount);
private:
	unsigned long m_vertCount;
	unsigned long m_vertSize;
	unsigned long m_vertOffset;
	IDirect3DVertexBuffer7* m_pVB;
};

class C3dDevice {
public:
	C3dDevice();
	~C3dDevice();
	long Init(HWND__* hwnd, _GUID* pDriverGUID, _GUID* pDeviceGUID, _DDSURFACEDESC2* pMode, unsigned long dwFlags);
	long DestroyObjects();
	int IsFullscreen() { return m_bIsFullscreen; }
	void EnableClipper(int bEnable);
	bool TestScreen();
	CRenderer* CreateRenderer();
	void DestroyRenderer(CRenderer* rc);
	CDC* CreateDCBuffer(unsigned long w, unsigned long h);
	CSurface* CreateWallPaper(unsigned long w, unsigned long h);
	void ConvertPalette(SYSTEMPALETTE* destPal, const tagPALETTEENTRY* srcPal, int palCnt);
	long Clear(unsigned long color);
	long ClearZBuffer();
	long RestoreSurfaces();
	long ShowFrame(bool vertSync);
	long CloneFrame();
	unsigned short EncodePixel(int, int, int) const;
	int DecodePixelR(unsigned short color) const;
	int DecodePixelG(unsigned short color) const;
	int DecodePixelB(unsigned short color) const;
	int GetWidth() const { return m_dwRenderWidth; }
	int GetHeight() const { return m_dwRenderHeight; }
	int GetWindowBitCount() const { return m_windowBitCount; }
	int EnableGDIObject(int bGDI){ return m_bIsGDIObject = bGDI; }
	void EnableMipmap();
	void AdjustTextureSize(unsigned long* w, unsigned long* h);
	IDirectDraw7* GetDirectDraw() { return m_pDD; }
	IDirect3D7* GetDirect3D() { return m_pD3D; }
	IDirect3DDevice7* GetD3DDevice() { return m_pd3dDevice; }
	IDirectDrawSurface7* GetFrontBuffer() { return m_pddsFrontBuffer; }
	IDirectDrawSurface7* GetBackBuffer() { return m_pddsBackBuffer; }
	IDirectDrawSurface7* GetRenderSurface() { return m_pddsRenderTarget; }
	_D3DDeviceDesc7 GetDeviceDesc() { return m_ddDeviceDesc; }
	_DDSURFACEDESC2* GetFrameBufferDesc() { return &m_ddsdFrameBuffer; }
	unsigned char GetBltStretchState() { return m_bSupportBltStretch; }
	unsigned long GetRBitMask() { return m_pfRBitMask; }
	unsigned long GetGBitMask() { return m_pfGBitMask; }
	unsigned long GetBBitMask() { return m_pfBBitMask; }
	int GetRShiftR() { return m_pfRShiftR; }
	int GetGShiftR() { return m_pfGShiftR; }
	int GetBShiftR() { return m_pfBShiftR; }
	int GetRShiftL() { return m_pfRShiftL; }
	int GetGShiftL() { return m_pfGShiftL; }
	int GetBShiftL() { return m_pfBShiftL; }
	void BackupFrame();
	void RestoreFrame();
	void SetMaterialDiffuse( float r, float g, float b ) { m_fMaterialDiffuseR = r, m_fMaterialDiffuseG = g, m_fMaterialDiffuseB = b; }
	void SetMaterialAmbient( float r, float g, float b ) { m_fMaterialAmbientR = r, m_fMaterialAmbientG = g, m_fMaterialAmbientB = b; }
	void GetMaterialDiffuse( float* r, float* g, float* b ) { *r = m_fMaterialDiffuseR, *g = m_fMaterialDiffuseG, *b = m_fMaterialDiffuseB; }
	void GetMaterialAmbient( float* r, float* g, float* b ) { *r = m_fMaterialAmbientR, *g = m_fMaterialAmbientG, *b = m_fMaterialAmbientB; }
private:
	long CreateZBuffer();
	long CreateBuffers(_DDSURFACEDESC2* pddsd, unsigned long dwFlags);
	long CreateDirectDraw(_GUID* pDriverGUID, unsigned long dwFlags);
	long CreateDirect3D(_GUID* pDriverGUID, unsigned long dwFlags);
	long CreateEnvironment(_GUID* pDriverGUID, _GUID* pDeviceGUID, _DDSURFACEDESC2* pMode, unsigned long dwFlags);
	long FlipToGDISurface(int bDrawFrame);
	HWND__* m_hWnd;
	unsigned long m_dwRenderWidth;
	unsigned long m_dwRenderHeight;
	tagRECT m_rcScreenRect;
	tagRECT m_rcViewportRect;
	IDirectDrawSurface7* m_pddsFrontBuffer;
	IDirectDrawSurface7* m_pddsBackBuffer;
	IDirectDrawSurface7* m_pddsRenderTarget;
	IDirectDrawSurface7* m_pddsZBuffer;
	IDirect3DDevice7* m_pd3dDevice;
	IDirectDraw7* m_pDD;
	IDirect3D7* m_pD3D;
	_D3DDeviceDesc7 m_ddDeviceDesc;
	unsigned long m_dwDeviceMemType;
	_DDPIXELFORMAT m_ddpfZBuffer;
	_DDSURFACEDESC2 m_ddsdFrameBuffer;
	int m_bIsFullscreen;
	int m_bIsGDIObject;
	int m_windowBitCount;
	IDirectDrawSurface7* m_pddsRenderBackup;
	int m_pfRShiftR;
	int m_pfRShiftL;
	int m_pfGShiftR;
	int m_pfGShiftL;
	int m_pfBShiftR;
	int m_pfBShiftL;
	int m_pfAShiftR;
	int m_pfAShiftL;
	unsigned long m_pfBitCount;
	unsigned long m_pfRBitMask;
	unsigned long m_pfGBitMask;
	unsigned long m_pfBBitMask;
	unsigned long m_pfABitMask;
	unsigned long m_dwMinTextureWidth;
	unsigned long m_dwMinTextureHeight;
	unsigned long m_dwMaxTextureWidth;
	unsigned long m_dwMaxTextureHeight;
	unsigned long m_dwMaxTextureAspectRatio;
	unsigned char m_bSupportBltStretch;
	unsigned char m_bSupportTextureSurface;
	float m_fMaterialDiffuseR;
	float m_fMaterialDiffuseG;
	float m_fMaterialDiffuseB;
	float m_fMaterialAmbientR;
	float m_fMaterialAmbientG;
	float m_fMaterialAmbientB;
};

inline unsigned long PACK32BIT8888(int a, int r, int g, int b)
{
	return ((a & 0xFF) << 24) + ((r & 0xFF) << 16) + ((g & 0xFF) << 8) + (b & 0xFF);
}

inline unsigned short C3dDevice::EncodePixel(int r, int g, int b) const
{
	return ((r >> m_pfRShiftL) << m_pfRShiftR) +
		   ((g >> m_pfGShiftL) << m_pfGShiftR) +
		   ((b >> m_pfBShiftL) << m_pfBShiftR);
}

extern C3dDevice g_3dDevice;

#endif
