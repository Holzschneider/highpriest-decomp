// The DirectDraw and Direct3D GUIDs the client names (IID_IDirectDraw7,
// IID_IDirect3D7, IID_IDirect3DHALDevice, IID_IDirect3DTnLHalDevice, ...).
// The original linked them from dxguid.lib (src/library/DxGuid.cpp, which the
// port does not build); here they are the Wine headers' DEFINE_GUIDs of
// ddraw.h and d3d.h, defined once. DirectInput's belong to the input adapter.
#include <windows.h>
#include <initguid.h>
#include <ddraw.h>
#include <d3d.h>
