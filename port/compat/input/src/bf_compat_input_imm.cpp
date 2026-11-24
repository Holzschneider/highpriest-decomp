// imm32: no input method context. macOS composes text itself (dead keys,
// input methods) and only committed text arrives, as TEXT events and so as
// WM_CHAR; VK_PROCESSKEY and the WM_IME_* messages never come. Every call
// reports "no IME", which CLanguage handles (no context: no composition, no
// candidates). Korean composition through the client's candidate window is
// future work.
#include "bf_compat_input_internal.h"

#include <imm.h>

#include "bf_trace.h"

extern "C" {

HIMC WINAPI ImmGetContext(HWND hwnd)
{
	return nullptr;
}

BOOL WINAPI ImmReleaseContext(HWND hwnd, HIMC himc)
{
	return TRUE;
}

LONG WINAPI ImmGetCompositionStringA(HIMC himc, DWORD index, LPVOID buffer, DWORD size)
{
	BF_STUB();
	return IMM_ERROR_GENERAL;
}

LONG WINAPI ImmGetCompositionStringW(HIMC himc, DWORD index, LPVOID buffer, DWORD size)
{
	BF_STUB();
	return IMM_ERROR_GENERAL;
}

DWORD WINAPI ImmGetCandidateListW(HIMC himc, DWORD index, LPCANDIDATELIST list, DWORD size)
{
	BF_STUB();
	return 0;
}

BOOL WINAPI ImmNotifyIME(HIMC himc, DWORD action, DWORD index, DWORD value)
{
	BF_STUB();
	return FALSE;
}

UINT WINAPI ImmGetVirtualKey(HWND hwnd)
{
	BF_STUB();
	return VK_PROCESSKEY;   // never asked: no WM_KEYDOWN carries VK_PROCESSKEY
}

}  // extern "C"
