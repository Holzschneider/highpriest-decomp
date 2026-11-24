// Stand-ins for the kernel32 names the input adapter calls (the win32 area
// owns the real ones): the global memory the clipboard hands over.
#include <windows.h>

#include <stdlib.h>
#include <string.h>

namespace {
struct Block {
	unsigned magic;
	SIZE_T size;
	int locks;
	char data[1];
};
const unsigned kMagic = 0x474C4F42;
Block *B(HGLOBAL h)
{
	Block *b = (Block *)h;
	return b && b->magic == kMagic ? b : nullptr;
}
}  // namespace

int g_fake_global_live = 0;   // blocks allocated and not freed

extern "C" {

HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T size)
{
	Block *b = (Block *)calloc(1, sizeof(Block) + size);
	if (!b)
		return nullptr;
	b->magic = kMagic;
	b->size = size;
	g_fake_global_live++;
	return (HGLOBAL)b;
}

LPVOID WINAPI GlobalLock(HGLOBAL h)
{
	Block *b = B(h);
	if (!b)
		return nullptr;
	b->locks++;
	return b->data;
}

BOOL WINAPI GlobalUnlock(HGLOBAL h)
{
	Block *b = B(h);
	if (!b || b->locks == 0)
		return FALSE;
	return --b->locks > 0;
}

SIZE_T WINAPI GlobalSize(HGLOBAL h)
{
	Block *b = B(h);
	return b ? b->size : 0;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL h)
{
	Block *b = B(h);
	if (!b)
		return h;
	b->magic = 0;
	free(b);
	g_fake_global_live--;
	return nullptr;
}

}  // extern "C"
