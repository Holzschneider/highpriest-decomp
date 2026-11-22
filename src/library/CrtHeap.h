// Required declarations from the locally available CRT/SRC/WINHEAP.H.
// Copyright (c) 1988-1998, Microsoft Corporation. All rights reserved.
#ifndef HIGHPRIEST_CRT_HEAP_H
#define HIGHPRIEST_CRT_HEAP_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <malloc.h>

#define __SYSTEM_HEAP 1
#define __V5_HEAP 2
#define __V6_HEAP 3
#define BYTES_PER_PARA 16
#define _OLD_PARASIZE 0x10
#define _OLD_PARASHIFT 4

// The original free and heap-allocation calls pass lock index 9.
#define _HEAP_LOCK 9

typedef struct tagHeader HEADER, *PHEADER;
typedef struct __old_sbh_region_struct __old_sbh_region_t;
typedef struct __old_sbh_page_struct __old_sbh_page_t;
typedef unsigned char __old_page_map_t;

#ifdef __cplusplus
extern "C" {
#endif

extern int __active_heap;
extern HANDLE _crtheap;
extern size_t __sbh_threshold;
extern size_t __old_sbh_threshold;
extern int _newmode;

void* __cdecl _nh_malloc(size_t, int);
void* __cdecl _heap_alloc(size_t);
int __cdecl _callnewh(size_t);
void __cdecl _lock(int);
void __cdecl _unlock(int);

void* __cdecl __sbh_alloc_block(int);
PHEADER __cdecl __sbh_find_block(void*);
void __cdecl __sbh_free_block(PHEADER, void*);
void* __cdecl __old_sbh_alloc_block(size_t);
__old_page_map_t* __cdecl __old_sbh_find_block(void*, __old_sbh_region_t**, __old_sbh_page_t**);
void __cdecl __old_sbh_free_block(__old_sbh_region_t*, __old_sbh_page_t*, __old_page_map_t*);

#ifdef __cplusplus
}
#endif

#endif
