// Adapted from the WINHEAP branch of the local Microsoft CRT source candidate.
/***
*realloc.c - Reallocate a block of memory in the heap
*
*       Copyright (c) 1989-1999, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the realloc() and _expand() functions.
*
*******************************************************************************/

#include "CrtHeapSupport.h"






/***
*void *realloc(pblock, newsize) - reallocate a block of memory in the heap
*
*Purpose:
*       Reallocates a block in the heap to newsize bytes. newsize may be
*       either greater or less than the original size of the block. The
*       reallocation may result in moving the block as well as changing
*       the size. If the block is moved, the contents of the original block
*       are copied over.
*
*       Special ANSI Requirements:
*
*       (1) realloc(NULL, newsize) is equivalent to malloc(newsize)
*
*       (2) realloc(pblock, 0) is equivalent to free(pblock) (except that
*           NULL is returned)
*
*       (3) if the realloc() fails, the object pointed to by pblock is left
*           unchanged
*
*Entry:
*       void *pblock    - pointer to block in the heap previously allocated
*                         by a call to malloc(), realloc() or _expand().
*
*       size_t newsize  - requested size for the re-allocated block
*
*Exit:
*       Success:  Pointer to the re-allocated memory block
*       Failure:  NULL
*
*Uses:
*
*Exceptions:
*       If pblock does not point to a valid allocation block in the heap,
*       realloc() will behave unpredictably and probably corrupt the heap.
*
*******************************************************************************/

void * __cdecl realloc (void * pBlock, size_t newsize)
{
        PHEADER     pHeader;
        void *      pvReturn;
        size_t      oldsize;
        size_t      origSize = newsize;

        //  if ptr is NULL, call malloc
        if (pBlock == NULL)
            return(malloc(newsize));

        //  if ptr is nonNULL and size is zero, call free and return NULL
        if (newsize == 0)
        {
            free(pBlock);
            return(NULL);
        }


        if ( __active_heap == __V6_HEAP )
        {
            for (;;)
            {
                pvReturn = NULL;
                if (newsize <= _HEAP_MAXREQ)
                {
#ifdef _MT
                    _lock( _HEAP_LOCK );
                    __try
                    {
#endif  /* _MT */

                    //  test if current block is in the small-block heap
                    if ((pHeader = __sbh_find_block(pBlock)) != NULL)
                    {
                        //  if the new size is not over __sbh_threshold, attempt
                        //  to reallocate within the small-block heap
                        if (newsize <= __sbh_threshold)
                        {
                            if (__sbh_resize_block(pHeader, pBlock, newsize))
                                pvReturn = pBlock;
                            else if ((pvReturn = __sbh_alloc_block(newsize)) != NULL)
                            {
                                oldsize = ((PENTRY)((char *)pBlock -
                                                    sizeof(int)))->sizeFront - 1;
                                memcpy(pvReturn, pBlock, __min(oldsize, newsize));
                                // headers may have moved, get pHeader again
                                pHeader = __sbh_find_block(pBlock);
                                __sbh_free_block(pHeader, pBlock);
                            }
                        }

                        //  If the reallocation has not been (successfully)
                        //  performed in the small-block heap, try to allocate
                        //  a new block with HeapAlloc.
                        if (pvReturn == NULL)
                        {
                            if (newsize == 0)
                                newsize = 1;
                            newsize = (newsize + BYTES_PER_PARA - 1) &
                                      ~(BYTES_PER_PARA - 1);
                            if ((pvReturn = HeapAlloc(_crtheap, 0, newsize)) != NULL)
                            {
                                oldsize = ((PENTRY)((char *)pBlock -
                                                    sizeof(int)))->sizeFront - 1;
                                memcpy(pvReturn, pBlock, __min(oldsize, newsize));
                                __sbh_free_block(pHeader, pBlock);
                            }
                        }
                    }

#ifdef _MT
                    }
                    __finally
                    {
                        _unlock( _HEAP_LOCK );
                    }
#endif  /* _MT */

                    //  the current block is NOT in the small block heap iff pHeader
                    //  is NULL
                    if ( pHeader == NULL )
                    {
                        if (newsize == 0)
                            newsize = 1;
                        newsize = (newsize + BYTES_PER_PARA - 1) &
                                  ~(BYTES_PER_PARA - 1);
                        pvReturn = HeapReAlloc(_crtheap, 0, pBlock, newsize);
                    }
                }

                if ( pvReturn || _newmode == 0)
                {
                    return pvReturn;
                }

                //  call installed new handler
                if (!_callnewh(newsize))
                    return NULL;

                //  new handler was successful -- try to allocate again
            }
        }
        else if ( __active_heap == __V5_HEAP )
        {
            __old_sbh_region_t *preg;
            __old_sbh_page_t *  ppage;
            __old_page_map_t *  pmap;
            size_t              oldsize;

            //  round up to the nearest paragrap
            if ( newsize <= _HEAP_MAXREQ )
                if ( newsize > 0 )
                    newsize = (newsize + _OLD_PARASIZE - 1) & ~(_OLD_PARASIZE - 1);
                else
                    newsize = _OLD_PARASIZE;

            for (;;)
            {
                pvReturn = NULL;
                if ( newsize <= _HEAP_MAXREQ )
                {
#ifdef _MT
                    _lock( _HEAP_LOCK );
                    __try
                    {
#endif  /* _MT */
                    if ( (pmap = __old_sbh_find_block(pBlock, &preg, &ppage)) != NULL )
                    {
                        //  If the new size falls below __sbh_threshold, try to
                        //  carry out the reallocation within the small-block
                        //  heap.
                        if ( newsize < __old_sbh_threshold )
                        {
                            if ( __old_sbh_resize_block(preg, ppage, pmap,
                                 newsize >> _OLD_PARASHIFT) )
                            {
                                pvReturn = pBlock;
                            }
                            else if ( (pvReturn = __old_sbh_alloc_block(newsize >>
                                       _OLD_PARASHIFT)) != NULL )
                            {
                                oldsize = ((size_t)(*pmap)) << _OLD_PARASHIFT ;
                                memcpy(pvReturn, pBlock, __min(oldsize, newsize));
                                __old_sbh_free_block(preg, ppage, pmap);
                            }
                        }

                        //  If the reallocation has not been (successfully)
                        //  performed in the small-block heap, try to allocate a
                        //  new block with HeapAlloc.
                        if ( (pvReturn == NULL) &&
                             ((pvReturn = HeapAlloc(_crtheap, 0, newsize)) != NULL) )
                        {
                            oldsize = ((size_t)(*pmap)) << _OLD_PARASHIFT;
                            memcpy(pvReturn, pBlock, __min(oldsize, newsize));
                            __old_sbh_free_block(preg, ppage, pmap);
                        }
                    }
                    else
                    {
                        pvReturn = HeapReAlloc(_crtheap, 0, pBlock, newsize);
                    }

#ifdef _MT
                    }
                    __finally
                    {
                        _unlock(_HEAP_LOCK);
                    }
#endif  /* _MT */
                }

                if ( pvReturn || _newmode == 0)
                {
                    return pvReturn;
                }

                //  call installed new handler
                if (!_callnewh(newsize))
                    return NULL;

                //  new handler was successful -- try to allocate again
            }
        }
        else    //  __active_heap == __SYSTEM_HEAP )
        {
            for (;;) {

                pvReturn = NULL;
                if (newsize <= _HEAP_MAXREQ)
                {
                    if (newsize == 0)
                        newsize = 1;
                    newsize = (newsize + BYTES_PER_PARA - 1) &
                              ~(BYTES_PER_PARA - 1);
                    pvReturn = HeapReAlloc(_crtheap, 0, pBlock, newsize);
                }

                if ( pvReturn || _newmode == 0)
                {
                    return pvReturn;
                }

                //  call installed new handler
                if (!_callnewh(newsize))
                    return NULL;

                //  new handler was successful -- try to allocate again
            }
        }
}
