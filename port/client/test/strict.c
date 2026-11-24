/* bf_types.h alone, strictly: -Wall -Wextra -Werror, gnu89, without the
   prefix, no Windows headers on the path). */
#include "bf_types.h"

DWORD bf_types_strict_c(LPRECT r, const BITMAPFILEHEADER *h, LOGFONTA *f, D3DMATRIX *m)
{
	ZeroMemory(f, sizeof *f);
	f->lfCharSet = HANGEUL_CHARSET;
	f->lfWeight = FW_BOLD;
	m->_11 = 1.0f;
	return (DWORD)(r->right - r->left) + h->bfSize + RGB(1, 2, 3) + MAKELONG(1, 2) + LOWORD(h->bfOffBits)
		+ D3DFVF_XYZ + D3DTA_TEXTURE + (DWORD)D3DBLEND_SRCALPHA + VK_F1 + WM_KEYDOWN + MAX_PATH
		+ (sizeof(BITMAPFILEHEADER) == 14) + (sizeof(INT_PTR) == 8) + __max(1, 2);
}
