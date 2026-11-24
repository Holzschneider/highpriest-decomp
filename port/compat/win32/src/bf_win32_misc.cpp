// ole32's COM start-up and netapi32's Netbios: the two names of WinMain.cpp
// and LoginMode.cpp that belong to no native module (M3.2).
//
// CoInitialize / CoUninitialize: the client creates no COM object through
// ole32 (DirectDraw, Direct3D and DirectInput come from their adapters'
// entry points), so there is nothing to start: S_OK, nothing done. Not a stub:
// this is the whole behaviour the client needs.
//
// Netbios: CLoginMode asks NCBASTAT for the first adapter's MAC address for
// the Korean and PC-bang login packets, and writes "111111111111" when the
// call fails. The port reports failure (NRC_SYSTEM) rather than handing a
// machine identifier to a server.
#include "bf_win32_internal.h"

#include <objbase.h>
#include <nb30.h>

HRESULT WINAPI CoInitialize(LPVOID reserved)
{
	(void)reserved;
	return S_OK;
}

void WINAPI CoUninitialize(void)
{
}

UCHAR WINAPI Netbios(PNCB ncb)
{
	BF_STUB();
	if (!ncb)
		return NRC_SYSTEM;
	ncb->ncb_retcode = NRC_SYSTEM;
	ncb->ncb_cmd_cplt = NRC_SYSTEM;
	return NRC_SYSTEM;
}
