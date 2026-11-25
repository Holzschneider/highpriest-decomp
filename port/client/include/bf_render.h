/*
 * bf_render.h: the client's render target on port/native/gfx (M11.9).
 *
 * src/3dlib draws through the graphics module directly (C3dDevice holds the
 * state and calls bf_gfx_draw through bf_render_draw). What the DirectDraw
 * adapter of stage A (port/compat/d3d) decided beyond single gfx calls, and
 * what has to outlive the client's own objects, is here:
 *
 *   - the one gfx device the client draws into (bf_port_start_graphics sets
 *     it, bf_main.cpp clears it at exit);
 *   - textures: every texture the client makes is registered, so that the
 *     device can go before the client's statics (CTexMgr, the renderer's
 *     caches) release theirs; a texture bound to a stage lives until the
 *     stage is bound to another, as a Direct3D 7 device held a reference to
 *     the surface it was given (the renderer caches the last texture it set
 *     by pointer);
 *   - the lazy render-target lock: CRenderer::Flip locks and unlocks the
 *     target every frame without touching a pixel; screenshots and the
 *     screen effect read (and write) it. The lock hands out a buffer whose
 *     pages are protected; the first access reads the target back, the first
 *     write marks it written, and only a written lock goes back on unlock;
 *   - a part of the target shown in the whole window (CBink's movie
 *     rectangle), stretched over the target with a linear filter as the
 *     blitters of the time did, from the lock buffer when it is still
 *     current (no read-back);
 *   - the target's change counter, which decides "still current".
 *
 * Coordinates are logical pixels. Threads: the client's main thread only.
 * Failure is a negative return value or NULL, with a trace line.
 */
#ifndef BF_RENDER_H
#define BF_RENDER_H

#include "../../native/gfx/include/bf_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sets (NULL: clears) the device. Clearing it destroys every texture still
   registered (their owners then hold textures bf_render_texture_valid calls
   gone) and drops the lock buffer; call it before bf_gfx_destroy. */
void bf_render_set_device(bf_gfx_device *dev);
bf_gfx_device *bf_render_device(void);

/* The target's logical size (0, 0 without a device). */
void bf_render_size(int *width, int *height);

/* ---- textures ---------------------------------------------------------- */

/* bf_gfx_texture_create on the device, registered. NULL without a device. */
bf_gfx_texture *bf_render_texture_create(int width, int height, bf_gfx_format format, int mip_levels);
/* Destroys a registered texture; one that is bound to a stage is destroyed
   when the stage is bound to another. NULL, or a texture the device took
   with it at bf_render_set_device(NULL), does nothing. */
void bf_render_texture_destroy(bf_gfx_texture *tex);
/* Non-zero while tex is registered and not yet destroyed by its owner. */
int bf_render_texture_valid(const bf_gfx_texture *tex);

/* Binds a texture (NULL: none, which samples opaque white) to stage 0 or 1. */
void bf_render_bind_texture(int stage, bf_gfx_texture *tex);

/* ---- drawing ------------------------------------------------------------ */

/* Hands state and the bound textures to gfx, then draws (indices NULL: not
   indexed). The target has changed afterwards. */
int bf_render_draw(const bf_gfx_state *state, bf_gfx_prim prim, bf_gfx_vertex_format format,
                   const void *vertices, int vertex_count, const uint16_t *indices, int index_count);
/* bf_gfx_clear; a colour clear changes the target. */
int bf_render_clear(unsigned flags, uint32_t color, float depth, const bf_gfx_rect *rect);
/* bf_gfx_write_pixels into the target (BGRA words), which then has changed. */
int bf_render_write_pixels(const bf_gfx_rect *rect, const void *pixels, int pitch, unsigned flags);
/* bf_gfx_read_pixels of the target, logical. */
int bf_render_read_pixels(const bf_gfx_rect *rect, void *pixels, int pitch);
/* Begin and end of a scene: bf_gfx_begin_frame, bf_gfx_end_frame (submits). */
void bf_render_begin_scene(void);
void bf_render_end_scene(void);
/* bf_gfx_backup_frame, bf_gfx_restore_frame (C3dDevice::BackupFrame, RestoreFrame). */
int bf_render_backup(void);
int bf_render_restore(void);

/* ---- the lazy lock ------------------------------------------------------ */

/* Locks the target as 32-bit X8R8G8B8 words (bytes B G R X) of the logical
   size; pitch receives the row stride in bytes. write_only: the contents are
   undefined (nothing read back) and the frame goes back on unlock. NULL when
   already locked or without a device. */
void *bf_render_lock(int write_only, int *pitch);
/* Unlocks; a written lock writes the frame back. Not locked: nothing. */
void bf_render_unlock(void);
int bf_render_is_locked(void);

/* ---- presenting --------------------------------------------------------- */

/* Shows the target in the window (bf_gfx_present). */
int bf_render_present(void);
/* Shows the rectangle (x, y, w, h) of the target over the whole window: it
   is drawn stretched over the target first (linear, clamped; own state and
   texture: the next bf_render_draw sends the client's again), then the
   target is presented. A rectangle that is the whole target or does not lie
   inside it presents the target as it is. */
int bf_render_present_part(int x, int y, int w, int h);

/* ---- statistics ---------------------------------------------------------- */

/* Any pointer may be NULL. locks: render-target locks; readbacks: locks whose
   pixels were read; writebacks: locks written back; deferred: bound textures
   whose destruction waited for their stage. */
void bf_render_get_stats(long long *locks, long long *readbacks, long long *writebacks, long long *deferred);

#ifdef __cplusplus
}
#endif

#endif
