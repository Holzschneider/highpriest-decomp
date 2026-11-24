#include "common.h"
#include "D3dutil.h"

#define OUTPUTSTR

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <math.h>
#include <stdio.h>


VOID D3DUtil_InitSurfaceDesc(_DDSURFACEDESC2& ddsd, DWORD dwFlags, DWORD dwCaps)
{
	ZeroMemory(&ddsd, sizeof(_DDSURFACEDESC2));
	ddsd.dwSize         = sizeof(_DDSURFACEDESC2);
	ddsd.dwFlags        = dwFlags;
	ddsd.ddsCaps.dwCaps = dwCaps;
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
}

VOID D3DUtil_InitViewport(_D3DVIEWPORT7& vp, DWORD dwWidth, DWORD dwHeight)
{
	ZeroMemory(&vp, sizeof(_D3DVIEWPORT7));
	vp.dwWidth  = dwWidth;
	vp.dwHeight = dwHeight;
	vp.dvMaxZ   = 1.0f;
}

HRESULT D3DUtil_SetProjectionMatrix(D3DMATRIX& mat, FLOAT fFOV, FLOAT fAspect,
                                    FLOAT fNearPlane, FLOAT fFarPlane)
{
	if (fabs(fFarPlane-fNearPlane) < 0.01f)
		return E_INVALIDARG;
	if (fabs(_sin(fFOV/2)) < 0.01f)
		return E_INVALIDARG;

	FLOAT w = fAspect * (FLOAT)(_cos(fFOV/2)/_sin(fFOV/2));
	FLOAT h =   1.0f  * (FLOAT)(_cos(fFOV/2)/_sin(fFOV/2));
	FLOAT Q = fFarPlane / (fFarPlane - fNearPlane);

	ZeroMemory(&mat, sizeof(D3DMATRIX));
	mat._11 = w;
	mat._22 = h;
	mat._33 = Q;
	mat._34 = 1.0f;
	mat._43 = -Q*fNearPlane;

	return S_OK;
}

HRESULT _DbgOut(TCHAR* strFile, DWORD dwLine, HRESULT hr, TCHAR* strMsg)
{
#ifdef OUTPUTSTR
	TCHAR buffer[256];
	sprintf(buffer, "%s(%ld): ", strFile, (long)dwLine);
	OutputDebugString(buffer);
	OutputDebugString(strMsg);

	if (hr)
	{
		sprintf(buffer, "(hr=%08lx)\n", (unsigned long)hr);
		OutputDebugString(buffer);
	}

	OutputDebugString("\n");
#endif
	return hr;
}
