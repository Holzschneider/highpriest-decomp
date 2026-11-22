// Adapted from the bundled CRT/SRC/NEW.CPP.
// Copyright (c) 1990-1998, Microsoft Corporation. All rights reserved.
#include <new.h>

// Internal CRT declaration from CRT/SRC/WINHEAP.H.
extern "C" void* __cdecl _nh_malloc(size_t, int);

void* operator new(unsigned int cb)
{
    void* res = _nh_malloc(cb, 1);
    return res;
}
