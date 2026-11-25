#ifndef HIGHPRIEST_3DLIB_DEVICE_H
#define HIGHPRIEST_3DLIB_DEVICE_H

#include "../common.h"
#include "bf_render.h"

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

class C3dDevice;

// A vertex buffer in CPU memory (M11.9: the gfx module draws from caller memory).
class CDynamicVB {
public:
	CDynamicVB();
	~CDynamicVB();
	bool Init(const unsigned int theFVF, unsigned int theVertexCount, unsigned int theVertexSize);
	void Release();
	bool Lock(unsigned int vertCount, void** ppLockedData);
	void Unlock();
	unsigned char* GetVertices() const { return m_pVB; }
	unsigned int GetFVF() const { return m_fvf; }
	unsigned int GetVertexSize() const { return m_vertSize; }
	unsigned int GetSize() { return m_vertCount; }
	void DrawPri(C3dDevice* device, unsigned int vertCount);
private:
	unsigned int m_vertCount;
	unsigned int m_vertSize;
	unsigned int m_vertOffset;
	unsigned int m_fvf;
	unsigned char* m_pVB;
};

class C3dDevice {
public:
	C3dDevice();
	~C3dDevice();
	int Init(HWND__* hwnd, _GUID* pDriverGUID, _GUID* pDeviceGUID, _DDSURFACEDESC2* pMode, unsigned int dwFlags);
	int DestroyObjects();
	int IsFullscreen() { return m_bIsFullscreen; }
	void EnableClipper(int bEnable);
	bool TestScreen();
	CRenderer* CreateRenderer();
	void DestroyRenderer(CRenderer* rc);
	CDC* CreateDCBuffer(unsigned int w, unsigned int h);
	CSurface* CreateWallPaper(unsigned int w, unsigned int h);
	void ConvertPalette(SYSTEMPALETTE* destPal, const tagPALETTEENTRY* srcPal, int palCnt);
	int Clear(unsigned int color);
	int ClearZBuffer();
	int RestoreSurfaces();
	int ShowFrame(bool vertSync);
	int CloneFrame();
	unsigned short EncodePixel(int, int, int) const;
	int DecodePixelR(unsigned short color) const;
	int DecodePixelG(unsigned short color) const;
	int DecodePixelB(unsigned short color) const;
	int GetWidth() const { return m_dwRenderWidth; }
	int GetHeight() const { return m_dwRenderHeight; }
	int GetWindowBitCount() const { return m_windowBitCount; }
	int EnableGDIObject(int bGDI){ return m_bIsGDIObject = bGDI; }
	void EnableMipmap();
	void AdjustTextureSize(unsigned int* w, unsigned int* h);
	IDirectDraw7* GetDirectDraw() { return m_pDD; }
	IDirectDrawSurface7* GetFrontBuffer() { return m_pddsFrontBuffer; }
	IDirectDrawSurface7* GetBackBuffer() { return m_pddsBackBuffer; }
	IDirectDrawSurface7* GetRenderSurface() { return m_pddsRenderTarget; }
	_DDSURFACEDESC2* GetFrameBufferDesc() { return &m_ddsdFrameBuffer; }
	unsigned char GetBltStretchState() { return m_bSupportBltStretch; }
	unsigned int GetRBitMask() { return m_pfRBitMask; }
	unsigned int GetGBitMask() { return m_pfGBitMask; }
	unsigned int GetBBitMask() { return m_pfBBitMask; }
	int GetRShiftR() { return m_pfRShiftR; }
	int GetGShiftR() { return m_pfGShiftR; }
	int GetBShiftR() { return m_pfBShiftR; }
	int GetRShiftL() { return m_pfRShiftL; }
	int GetGShiftL() { return m_pfGShiftL; }
	int GetBShiftL() { return m_pfBShiftL; }
	void BackupFrame();
	void RestoreFrame();
#if defined(BF_PORT)
	int BfResize(unsigned int w, unsigned int h);
#endif
	// The device the renderer draws with (M11.9: port/native/gfx through
	// bf_render.h). States are set by their Direct3D 7 numbers, which the
	// renderer and the game data keep, and translated into one bf_gfx_state.
	int BeginScene();
	int EndScene();
	void SetRenderState(D3DRENDERSTATETYPE type, unsigned int value);
	void SetTextureStageState(unsigned int stage, D3DTEXTURESTAGESTATETYPE type, unsigned int value);
	void SetTexture(unsigned int stage, bf_gfx_texture* tex);
	int DrawPrimitive(D3DPRIMITIVETYPE type, unsigned int fvf, void* verts, unsigned int numVerts, unsigned int flags);
	int DrawIndexedPrimitive(D3DPRIMITIVETYPE type, unsigned int fvf, void* verts, unsigned int numVerts, unsigned short* indices, unsigned int numIndices, unsigned int flags);
	int DrawPrimitiveVB(D3DPRIMITIVETYPE type, CDynamicVB* vb, unsigned int startVert, unsigned int numVerts, unsigned int flags);
	int DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE type, CDynamicVB* vb, unsigned int startVert, unsigned int numVerts, unsigned short* indices, unsigned int numIndices, unsigned int flags);
	void SetMaterialDiffuse( float r, float g, float b ) { m_fMaterialDiffuseR = r, m_fMaterialDiffuseG = g, m_fMaterialDiffuseB = b; }
	void SetMaterialAmbient( float r, float g, float b ) { m_fMaterialAmbientR = r, m_fMaterialAmbientG = g, m_fMaterialAmbientB = b; }
	void GetMaterialDiffuse( float* r, float* g, float* b ) { *r = m_fMaterialDiffuseR, *g = m_fMaterialDiffuseG, *b = m_fMaterialDiffuseB; }
	void GetMaterialAmbient( float* r, float* g, float* b ) { *r = m_fMaterialAmbientR, *g = m_fMaterialAmbientG, *b = m_fMaterialAmbientB; }
private:
	int CreateBuffers(_DDSURFACEDESC2* pddsd, unsigned int dwFlags);
	int CreateDirectDraw(_GUID* pDriverGUID, unsigned int dwFlags);
	int CreateEnvironment(_GUID* pDriverGUID, _GUID* pDeviceGUID, _DDSURFACEDESC2* pMode, unsigned int dwFlags);
	int FlipToGDISurface(int bDrawFrame);
	HWND__* m_hWnd;
	unsigned int m_dwRenderWidth;
	unsigned int m_dwRenderHeight;
	tagRECT m_rcScreenRect;
	tagRECT m_rcViewportRect;
	IDirectDrawSurface7* m_pddsFrontBuffer;
	IDirectDrawSurface7* m_pddsBackBuffer;
	IDirectDrawSurface7* m_pddsRenderTarget;
	IDirectDraw7* m_pDD;
	bf_gfx_state m_state;
	int m_bInitialized;
	int Draw(D3DPRIMITIVETYPE type, unsigned int fvf, const void* verts, unsigned int numVerts, const unsigned short* indices, unsigned int numIndices);
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
	unsigned int m_pfBitCount;
	unsigned int m_pfRBitMask;
	unsigned int m_pfGBitMask;
	unsigned int m_pfBBitMask;
	unsigned int m_pfABitMask;
	unsigned int m_dwMinTextureWidth;
	unsigned int m_dwMinTextureHeight;
	unsigned int m_dwMaxTextureWidth;
	unsigned int m_dwMaxTextureHeight;
	unsigned int m_dwMaxTextureAspectRatio;
	unsigned char m_bSupportBltStretch;
	unsigned char m_bSupportTextureSurface;
	float m_fMaterialDiffuseR;
	float m_fMaterialDiffuseG;
	float m_fMaterialDiffuseB;
	float m_fMaterialAmbientR;
	float m_fMaterialAmbientG;
	float m_fMaterialAmbientB;
};

inline unsigned int PACK32BIT8888(int a, int r, int g, int b)
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
