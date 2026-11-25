#include "common.h"
#include "D3dutil.h"

#define OUTPUTSTR

#include "bf_types.h"
#include <math.h>
#include <stdio.h>


// D3DUtil_InitSurfaceDesc and D3DUtil_InitViewport filled DirectDraw surface
// descriptions and the Direct3D viewport; neither exists on the gfx module
// (M11.9).

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
	// The debugger's output: one trace line (M11.5).
	if (hr)
		BF_TRACE(BFT_GFX, "%s(%ld): %s(hr=%08lx)", strFile, (long)dwLine, strMsg, (unsigned long)hr);
	else
		BF_TRACE(BFT_GFX, "%s(%ld): %s", strFile, (long)dwLine, strMsg);
#endif
	return hr;
}
