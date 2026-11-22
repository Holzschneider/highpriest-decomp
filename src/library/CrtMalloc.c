// Adapted from the WINHEAP, multithreaded branch of CRT/SRC/MALLOC.C.
// Copyright (c) 1989-1998, Microsoft Corporation. All rights reserved.
#include "CrtHeap.h"

void* __cdecl malloc(size_t size)
{
    void* res = _nh_malloc(size, _newmode);
    return res;
}

void* __cdecl _nh_malloc(size_t size, int nhFlag)
{
    void* pvReturn;

    if (size > _HEAP_MAXREQ)
        return NULL;

    for (;;) {
        if (size <= _HEAP_MAXREQ)
            pvReturn = _heap_alloc(size);
        else
            pvReturn = NULL;

        if (pvReturn || nhFlag == 0)
            return pvReturn;

        if (!_callnewh(size))
            return NULL;
    }
}

void* __cdecl _heap_alloc(size_t size)
{
    void* pvReturn;

    if (__active_heap == __V6_HEAP)
    {
        if (size <= __sbh_threshold)
        {
            _lock(_HEAP_LOCK);
            __try {
                pvReturn = __sbh_alloc_block(size);
            }
            __finally {
                _unlock(_HEAP_LOCK);
            }
            if (pvReturn)
                return pvReturn;
        }
    }
    else if (__active_heap == __V5_HEAP)
    {
        if (size)
            size = (size + _OLD_PARASIZE - 1) & ~(_OLD_PARASIZE - 1);
        else
            size = _OLD_PARASIZE;

        if (size <= __old_sbh_threshold)
        {
            _lock(_HEAP_LOCK);
            __try {
                pvReturn = __old_sbh_alloc_block(size >> _OLD_PARASHIFT);
            }
            __finally {
                _unlock(_HEAP_LOCK);
            }
            if (pvReturn != NULL)
                return pvReturn;
        }
        return HeapAlloc(_crtheap, 0, size);
    }

    if (size == 0)
        size = 1;
    size = (size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1);
    return HeapAlloc(_crtheap, 0, size);
}
