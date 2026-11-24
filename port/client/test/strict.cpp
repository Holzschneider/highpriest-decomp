/* bf_types.h alone, strictly: -Wall -Wextra -pedantic -Werror, C++14, without the
   prefix, no Windows headers on the path). */
#include "bf_types.h"

DWORD bf_types_strict_cxx(LPRECT r, const BITMAPFILEHEADER *h, LOGFONTA *f, D3DMATRIX *m)
{
	ZeroMemory(f, sizeof *f);
	f->lfCharSet = HANGEUL_CHARSET;
	f->lfWeight = FW_BOLD;
	m->_11 = 1.0f;
	return (DWORD)(r->right - r->left) + h->bfSize + RGB(1, 2, 3) + MAKELONG(1, 2) + LOWORD(h->bfOffBits)
		+ D3DFVF_XYZ + D3DTA_TEXTURE + (DWORD)D3DBLEND_SRCALPHA + VK_F1 + WM_KEYDOWN + MAX_PATH
		+ (sizeof(BITMAPFILEHEADER) == 14) + (sizeof(INT_PTR) == 8) + __max(1, 2);
}

/* C++ only: min and max, the matrix accessor, WCHAR as char16_t. */
int bf_types_strict_cxx2(D3DMATRIX &m)
{
	m(1, 2) = 3.0f;
	WCHAR w = u'x';
	return max(1, 2) + min(3, 4) + (int)m(1, 2) + w;
}
