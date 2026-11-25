/*
 * bf_compat_gdi: what is left of the gdi32 adapter after M11.8 (see
 * ../README.md): the port-side functions the input adapter calls. Include
 * <windows.h> first. Deleted with M11.7's removal of those callers.
 */
#ifndef BF_COMPAT_GDI_H
#define BF_COMPAT_GDI_H

#ifdef __cplusplus
extern "C" {
#endif

/* For the input adapter's GetDC(NULL): a screen DC nothing draws on (the
   client called it only for EnumFontFamiliesEx, which M11.8 removed). */
HDC bf_compat_gdi_screen_dc(void);

/* The colour of a stock brush (WNDCLASS.hbrBackground; the client registers
   BLACK_BRUSH). FALSE for NULL_BRUSH or a handle that is not a stock brush. */
BOOL bf_compat_gdi_brush_color(HBRUSH brush, COLORREF *out);

#ifdef __cplusplus
}
#endif

#endif
