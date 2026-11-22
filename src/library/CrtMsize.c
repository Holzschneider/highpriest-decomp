// Adapted from the WINHEAP branch of the local Microsoft CRT source candidate.
/***
*msize.c - calculate the size of a memory block in the heap
*
*       Copyright (c) 1989-1998, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the following function:
*           _msize()    - calculate the size of a block in the heap
*
*******************************************************************************/

#include "CrtHeapSupport.h"







/***
*size_t _msize(pblock) - calculate the size of specified block in the heap
*
*Purpose:
*       Calculates the size of memory block (in the heap) pointed to by
*       pblock.
*
*Entry:
*       void *pblock - pointer to a memory block in the heap
*
*Return:
*       size of the block
*
*******************************************************************************/

size_t __cdecl _msize (void * pblock)
{
        size_t      retval;
        PHEADER     pHeader;


        if ( __active_heap == __V6_HEAP )
        {
#ifdef _MT
            _lock( _HEAP_LOCK );
            __try {
#endif  /* _MT */
            if ((pHeader = __sbh_find_block(pblock)) != NULL)
                retval = (size_t)
                         (((PENTRY)((char *)pblock - sizeof(int)))->sizeFront - 0x9);
#ifdef _MT
            }
            __finally {
                _unlock( _HEAP_LOCK );
            }
#endif  /* _MT */
            if ( pHeader == NULL )
                retval = (size_t)HeapSize(_crtheap, 0, pblock);
        }
        else if ( __active_heap == __V5_HEAP )
        {
            __old_sbh_region_t *preg;
            __old_sbh_page_t *  ppage;
            __old_page_map_t *  pmap;
#ifdef _MT
            _lock(_HEAP_LOCK);
            __try {
#endif  /* _MT */
            if ( (pmap = __old_sbh_find_block(pblock, &preg, &ppage)) != NULL )
                retval = ((size_t)(*pmap)) << _OLD_PARASHIFT;
#ifdef _MT
            }
            __finally {
                _unlock( _HEAP_LOCK );
            }
#endif  /* _MT */
            if ( pmap == NULL )
                retval = (size_t) HeapSize( _crtheap, 0, pblock );
        }
        else    /* __active_heap == __SYSTEM_HEAP */
            retval = (size_t)HeapSize(_crtheap, 0, pblock);

        return retval;

}
