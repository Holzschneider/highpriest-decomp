// Adapted from the WINHEAP, multithreaded branch of CRT/SRC/FREE.C.
// Copyright (c) 1989-1998, Microsoft Corporation. All rights reserved.
#include "CrtHeap.h"

void __cdecl free(void* pBlock)
{
    PHEADER pHeader;

    if (pBlock == NULL)
        return;

    if (__active_heap == __V6_HEAP)
    {
        _lock(_HEAP_LOCK);
        __try {
            if ((pHeader = __sbh_find_block(pBlock)) != NULL)
                __sbh_free_block(pHeader, pBlock);
        }
        __finally {
            _unlock(_HEAP_LOCK);
        }

        if (pHeader == NULL)
            HeapFree(_crtheap, 0, pBlock);
    }
    else if (__active_heap == __V5_HEAP)
    {
        __old_sbh_region_t* preg;
        __old_sbh_page_t* ppage;
        __old_page_map_t* pmap;

        _lock(_HEAP_LOCK);
        __try {
            if ((pmap = __old_sbh_find_block(pBlock, &preg, &ppage)) != NULL)
                __old_sbh_free_block(preg, ppage, pmap);
        }
        __finally {
            _unlock(_HEAP_LOCK);
        }

        if (pmap == NULL)
            HeapFree(_crtheap, 0, pBlock);
    }
    else
        HeapFree(_crtheap, 0, pBlock);

    return;
}
