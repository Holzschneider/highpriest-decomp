// ole32's COM start-up: the one name of WinMain.cpp that belongs to no native
// module (M3.2). netapi32's Netbios left with M11.5 (LoginMode.cpp writes the
// failed query's "111111111111" itself).
//
// CoInitialize / CoUninitialize: the client creates no COM object through
// ole32 (DirectDraw, Direct3D and DirectInput come from their adapters'
// entry points), so there is nothing to start: S_OK, nothing done. Not a stub:
// this is the whole behaviour the client needs.
#include "bf_win32_internal.h"

#include <objbase.h>

HRESULT WINAPI CoInitialize(LPVOID reserved)
{
	(void)reserved;
	return S_OK;
}

void WINAPI CoUninitialize(void)
{
}
