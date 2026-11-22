// Adapted from the WINHEAP branch of the local Microsoft CRT source candidate.
/***
*calloc.c - allocate storage for an array from the heap
*
*       Copyright (c) 1989-1998, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the calloc() function.
*
*******************************************************************************/

#include "CrtHeapSupport.h"





/***
*void *calloc(size_t num, size_t size) - allocate storage for an array from
*       the heap
*
*Purpose:
*       Allocate a block of memory from heap big enough for an array of num
*       elements of size bytes each, initialize all bytes in the block to 0
*       and return a pointer to it.
*
*Entry:
*       size_t num  - number of elements in the array
*       size_t size - size of each element
*
*Exit:
*       Success:  void pointer to allocated block
*       Failure:  NULL
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl calloc (size_t num, size_t size)
{
        size_t  size_sbh;
        void *  pvReturn;
        size_t  org_num = num, org_size = size;

        size_sbh = size = size * num;


        /* round up to the nearest paragraph */
        if (size <= _HEAP_MAXREQ)
        {
            if (size == 0)
                size = 1;
            size = (size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1);
        }

        for (;;)
        {
            pvReturn = NULL;

            if (size <= _HEAP_MAXREQ)
            {
                if ( __active_heap == __V6_HEAP )
                {
                    if ( size_sbh <= __sbh_threshold )
                    {
                        //  Allocate the block from the small-block heap and
                        //  initialize it with zeros.
#ifdef _MT
                        _lock( _HEAP_LOCK );
                        __try {
#endif  /* _MT */
                        pvReturn = __sbh_alloc_block(size_sbh);
#ifdef _MT
                        }
                        __finally {
                            _unlock( _HEAP_LOCK );
                        }
#endif  /* _MT */

                        if (pvReturn != NULL)
                            memset(pvReturn, 0, size_sbh);
                    }
                }
                else if ( __active_heap == __V5_HEAP )
                {
                    if ( size <= __old_sbh_threshold )
                    {
                        //  Allocate the block from the small-block heap and
                        //  initialize it with zeros.
#ifdef _MT
                        _lock(_HEAP_LOCK);
                        __try {
#endif  /* _MT */
                        pvReturn = __old_sbh_alloc_block(size >> _OLD_PARASHIFT);
#ifdef _MT
                        }
                        __finally {
                            _unlock(_HEAP_LOCK);
                        }
#endif  /* _MT */
                        if ( pvReturn != NULL )
                            memset(pvReturn, 0, size);
                    }
                }

                if (pvReturn == NULL)
                    pvReturn = HeapAlloc(_crtheap, HEAP_ZERO_MEMORY, size);
            }

            if (pvReturn || _newmode == 0)
            {
                return pvReturn;
            }

            /* call installed new handler */
            if (!_callnewh(size))
                return NULL;

            /* new handler was successful -- try to allocate again */
        }

}
