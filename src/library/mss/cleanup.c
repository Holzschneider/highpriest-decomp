/* Miles Sound System win/cleanup.c (mss32.lib member cleanup.obj).
   Reconstructed after Miles 6.5c's win/cleanup.c; the original links the
   Miles 6.1a build of it (C:\devel\projects\mss\src\win\cleanup.obj),
   compiled with the Processor Pack backend 12.00.8943. mss.h rewrites
   AIL_startup() as (MSS_auto_cleanup(),AIL_startup()). Inside the Miles
   build mss.h declares the AIL entry points without dllimport, so
   MSS_cleanup reaches AIL_shutdown through the import library's thunk. */

#include <stdlib.h>

void __stdcall AIL_shutdown(void);

static void __cdecl MSS_cleanup(void)
{
  AIL_shutdown();
};

int __cdecl MSS_auto_cleanup(void)
{
  atexit(MSS_cleanup);
  return(0);
}
