/*
 * bf_compat_gdi: the gdi32 names of the client, on port/native/text (stage A;
 * M11 deletes it). This header holds the port-side functions other adapters
 * call; the gdi32 functions themselves are declared by <wingdi.h>.
 *
 * Include <windows.h> first. Handles are typed objects: a handle of the wrong
 * type, a deleted handle or a stray pointer is traced (BFT_W32) and fails as
 * GDI fails, it never crashes.
 */
#ifndef BF_COMPAT_GDI_H
#define BF_COMPAT_GDI_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * For the d3d adapter's IDirectDrawSurface7::GetDC: a DC that draws into a
 * 32-bit bitmap the caller owns. Pixels are 0xAARRGGBB in native order
 * (B,G,R,A bytes), `pitch` bytes per row; bottom_up != 0 when the first row
 * in memory is the bottom of the picture. Text and fills write their pixels
 * as GDI does, 0x00RRGGBB (alpha 0), and leave every other pixel untouched,
 * so the caller may pre-fill the bitmap with a key or with the surface's
 * contents. The DC starts in GDI's defaults (black text, opaque white
 * background, the system font), like a fresh surface DC; selecting a bitmap
 * into it fails. Release it with DeleteDC. NULL for bad arguments.
 */
HDC bf_compat_gdi_dc_for_bitmap(void *bits, int width, int height, int pitch, int bottom_up);

/*
 * The union of the pixels written through `hdc` since the last call (any DC
 * kind), clipped to its bitmap, as a top-down rectangle (right and bottom
 * exclusive); resets it. FALSE (and an empty rect) when nothing was written.
 */
BOOL bf_compat_gdi_dc_take_dirty(HDC hdc, RECT *out);

/*
 * For the input adapter's GetDC(NULL): the screen DC. The client uses it only
 * for EnumFontFamiliesEx, which accepts any DC, NULL included. It has no
 * bitmap (drawing on it does nothing); DeleteDC on it fails; ReleaseDC of it
 * needs to do nothing.
 */
HDC bf_compat_gdi_screen_dc(void);

/* The colour of a solid or stock brush (for WNDCLASS.hbrBackground). FALSE
   for NULL_BRUSH, a hollow brush, or a handle that is not a brush. */
BOOL bf_compat_gdi_brush_color(HBRUSH brush, COLORREF *out);

/* The number of live GDI objects the client created (stock objects and the
   screen DC not counted), for leak checks. */
int bf_compat_gdi_live_objects(void);

#ifdef __cplusplus
}
#endif

#endif
