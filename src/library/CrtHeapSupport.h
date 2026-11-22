// Private release CRT heap declarations; concrete layouts are preserved in CrtSmallHeap.h.
#ifndef HIGHPRIEST_CRT_HEAP_SUPPORT_H
#define HIGHPRIEST_CRT_HEAP_SUPPORT_H
#include "CrtSmallHeap.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

// Existing malloc/free code and original CRT call operands establish lock index 9.
#define _HEAP_LOCK 9
extern int _newmode;
int __cdecl _callnewh(size_t);
void __cdecl _lock(int);
void __cdecl _unlock(int);
#endif
