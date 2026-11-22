// Adapted from the WINHEAP branch of the local Microsoft CRT source candidate.
/***
*heapinit.c -  Initialze the heap
*
*       Copyright (c) 1989-1998, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*******************************************************************************/

#include "CrtHeapSupport.h"







HANDLE _crtheap;

/*
 * Dummy definition of _amblksiz. Included primarily so the dll will build
 * without having to change crtlib.c (there is an access function for _amblksiz
 * defined in crtlib.c).
 */
unsigned int _amblksiz = BYTES_PER_PARA;

int __active_heap;


void __cdecl _GetLinkerVersion(LinkerVersion * plv)
{
        PIMAGE_DOS_HEADER   pidh;
        PIMAGE_NT_HEADERS   pinh;

        plv->dw = 0;
        pidh = (PIMAGE_DOS_HEADER) GetModuleHandle(NULL);

        if ( pidh->e_magic != IMAGE_DOS_SIGNATURE || pidh->e_lfanew == 0)
            return;

        pinh = (PIMAGE_NT_HEADERS)(((PBYTE)pidh) + pidh->e_lfanew);

        plv->bverMajor = pinh->OptionalHeader.MajorLinkerVersion;
        plv->bverMinor = pinh->OptionalHeader.MinorLinkerVersion;
}

/***
*__heap_select() - Choose from the V6, V5 or system heaps
*
*Purpose:
*       Check OS, environment and build bits to determine appropriate
*       small-block heap for the app.
*
*Entry:
*       <void>
*Exit:
*       Returns __V6_HEAP, __V5_HEAP or __SYSTEM_HEAP
*
*Exceptions:
*       none
*
*******************************************************************************/

int __cdecl __heap_select(void)
{
        char env_heap_select_string[(_MAX_PATH+5)*16];
        char env_app_name[_MAX_PATH];
        char *env_heap_type = NULL;
        char *cp;
        int heap_choice;
        LinkerVersion lv;
        OSVERSIONINFO osvi;

        // First, check the OS for NT >= 5.0
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if ( GetVersionEx(&osvi) )
            if ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
                 (osvi.dwMajorVersion >= 5) )
                return __SYSTEM_HEAP;

        // Second, check the envrionment variable override
        if (GetEnvironmentVariableA(__HEAP_ENV_STRING, env_heap_select_string, sizeof(env_heap_select_string)))
        {
            for (cp = env_heap_select_string; *cp; ++cp)
                if ('a' <= *cp && *cp <= 'z')
                    *cp += 'A' - 'a';
            if (!strncmp(__GLOBAL_HEAP_SELECTOR,env_heap_select_string,sizeof(__GLOBAL_HEAP_SELECTOR)-1))
                env_heap_type = env_heap_select_string;
            else
            {
                GetModuleFileName(NULL,env_app_name,sizeof(env_app_name));
                for (cp = env_app_name; *cp; ++cp)
                    if ('a' <= *cp && *cp <= 'z')
                        *cp += 'A' - 'a';
                env_heap_type = strstr(env_heap_select_string,env_app_name);
            }
            if (env_heap_type)
            {
                if (env_heap_type = strchr(env_heap_type,','))
                {
                    cp = ++env_heap_type;
                    while (*cp)
                    {
                        if (*cp == ';')
                            *cp = 0;
                        else cp++;
                    }
                    heap_choice = (int)strtol(env_heap_type, NULL, 10);
                    if ( (heap_choice == __V5_HEAP) ||
                         (heap_choice == __V6_HEAP) ||
                         (heap_choice == __SYSTEM_HEAP) )
                        return heap_choice;
                }
            }
        }

        // Third, check the build bits in the app; apps built with tools >= VC++ 6.0
        // will get the V6 heap, apps built with older tools will get the V5 heap

        _GetLinkerVersion(&lv);
        if (lv.bverMajor >= 6)
            return __V6_HEAP;
        else
            return __V5_HEAP;

}

/***
*_heap_init() - Initialize the heap
*
*Purpose:
*       Setup the initial C library heap.
*
*       NOTES:
*       (1) This routine should only be called once!
*       (2) This routine must be called before any other heap requests.
*
*Entry:
*       <void>
*Exit:
*       Returns 1 if successful, 0 otherwise.
*
*Exceptions:
*       If heap cannot be initialized, the program will be terminated
*       with a fatal runtime error.
*
*******************************************************************************/

int __cdecl _heap_init (
        int mtflag
        )
{
        //  Initialize the "big-block" heap first.
        if ( (_crtheap = HeapCreate( mtflag ? 0 : HEAP_NO_SERIALIZE,
                                     BYTES_PER_PAGE, 0 )) == NULL )
            return 0;

        // Pick a heap, any heap
        __active_heap = __heap_select();

        if ( __active_heap == __V6_HEAP )
        {
            //  Initialize the small-block heap
            if (__sbh_heap_init(MAX_ALLOC_DATA_SIZE) == 0)
            {
                HeapDestroy(_crtheap);
                return 0;
            }
        }
        else if ( __active_heap == __V5_HEAP )
        {
            if ( __old_sbh_new_region() == NULL )
            {
                HeapDestroy( _crtheap );
                return 0;
            }
        }

        return 1;
}

/***
*_heap_term() - return the heap to the OS
*
*Purpose:
*
*       NOTES:
*       (1) This routine should only be called once!
*       (2) This routine must be called AFTER any other heap requests.
*
*Entry:
*       <void>
*Exit:
*       <void>
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _heap_term (void)
{
        //  if it has been initialized, destroy the small-block heap
        if ( __active_heap == __V6_HEAP )
        {
            PHEADER pHeader = __sbh_pHeaderList;
            int     cntHeader;

            //  scan through all the headers
            for (cntHeader = 0; cntHeader < __sbh_cntHeaderList; cntHeader++)
            {
                //  decommit and release the address space for the region
                VirtualFree(pHeader->pHeapData, BYTES_PER_REGION, MEM_DECOMMIT);
                VirtualFree(pHeader->pHeapData, 0, MEM_RELEASE);

                //  free the region data structure
                HeapFree(_crtheap, 0, pHeader->pRegion);

                pHeader++;
            }
            //  free the header list
            HeapFree(_crtheap, 0, __sbh_pHeaderList);
        }
        else if ( __active_heap == __V5_HEAP )
        {
            __old_sbh_region_t *preg = &__old_small_block_heap;

            //  Release the regions of the small-block heap
            do
            {
                if ( preg->p_pages_begin != NULL )
                    VirtualFree( preg->p_pages_begin, 0, MEM_RELEASE );
            }
            while ( (preg = preg->p_next_region) != &__old_small_block_heap );
        }

        //  destroy the large-block heap
        HeapDestroy(_crtheap);
}
