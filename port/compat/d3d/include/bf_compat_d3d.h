/*
 * DirectDraw 7 / Direct3D 7 adapter (stage A, M4.4; deleted in M11).
 *
 * The adapter defines DirectDrawCreateEx and the IDirectDraw7,
 * IDirectDrawSurface7, IDirectDrawClipper, IDirect3D7, IDirect3DDevice7 and
 * IDirect3DVertexBuffer7 objects the client's src/3dlib calls, and the
 * DirectDraw and Direct3D GUIDs. Everything draws into one bf_gfx device
 * (port/native/gfx) that the application owns: the adapter never creates a
 * window, a layer or a device of its own.
 *
 * Start-up order (the client's start-up code, M4.x):
 *
 *   1. the application module creates the window and its CAMetalLayer;
 *   2. bf_gfx_create(&desc) with the window's logical size and UI scale;
 *   3. bf_compat_d3d_set_device(gfx);
 *   4. the client's C3dDevice::Init (DirectDrawCreateEx ...) as written.
 *
 * DirectDrawCreateEx fails with DDERR_NODIRECTDRAWHW while no device is set.
 * The client's render target is the gfx target, at the gfx logical size; the
 * display mode the client reads (IDirectDraw7::GetDisplayMode, the primary
 * surface's description) is that size in 32-bit X8R8G8B8. If the client asks
 * for a render target of another size (GetClientRect disagreeing with the gfx
 * device), the adapter resizes the gfx device to the client's size, keeping
 * the UI scale, and traces it (BFT_GFX).
 *
 * At exit: bf_compat_d3d_set_device(NULL) before bf_gfx_destroy. It destroys
 * the gfx textures of every surface still alive (their surfaces then report
 * DDERR_SURFACELOST), so the gfx device can be destroyed safely in any order
 * relative to the client's own clean-up.
 *
 * Threads: the client's main thread only, like the gfx module.
 */
#ifndef BF_COMPAT_D3D_H
#define BF_COMPAT_D3D_H

#include "bf_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sets (or, with NULL, clears) the gfx device all DirectDraw objects draw into. */
void bf_compat_d3d_set_device(bf_gfx_device *dev);

/* The device set above, or NULL. */
bf_gfx_device *bf_compat_d3d_get_device(void);

/*
 * Statistics for traces and tests; any pointer may be NULL.
 *   target_locks      render-target locks (CRenderer::Lock, Flip's empty lock)
 *   target_readbacks  locks whose pixels were actually read, so the target
 *                     came back from the GPU (lazy read-back)
 *   target_writebacks locks whose pixels were written, so the frame went back
 */
void bf_compat_d3d_get_stats(long long *target_locks, long long *target_readbacks,
                             long long *target_writebacks);

#ifdef __cplusplus
}
#endif

#endif
