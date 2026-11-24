/*
 * bf_view: the one place that turns the window into the client's screen.
 *
 * From the window's size in pixels, the backing scale and the UI scale
 * setting it computes the UI scale in effect and the logical resolution the
 * client draws in, keeps the graphics device (port/native/gfx) and the
 * window's minimum size in step with them, tells the client when they
 * change, and maps pointer positions between the window and logical
 * coordinates (technical plan 3.7, spec section 3).
 *
 * The rule.
 *
 *   setting 1, 2 or 3   the UI scale is that number, lowered (3 -> 2 -> 1)
 *                       while the logical size would drop below 640 x 480;
 *   setting 0 ("auto")  the largest of 3, 2, 1 that keeps the logical size
 *                       at least 800 x 600, else 1.
 *
 *   logical_w = floor(drawable_w / ui_scale), at least 640
 *   logical_h = floor(drawable_h / ui_scale), at least 480
 *
 * 640 x 480 is the hard floor (the smallest mode the 2008 client offered);
 * 800 x 600 is what "auto" keeps, the size the client's UI was laid out for.
 * The scale is a pure function of the size: with "auto", dragging the
 * window's edge across a threshold changes the UI scale.
 *
 * The picture. The drawable is the whole window (drawable_w x drawable_h =
 * content size in pixels). The logical screen covers its top left
 * view_w x view_h = logical * ui_scale native pixels; the leftover, at most
 * ui_scale - 1 pixels at the right and at the bottom, is cleared to black.
 * Nothing is centred, so window pixel and native target pixel coordinates
 * are the same and the pointer mapping has no offset. The logical size only
 * exceeds the window (and the picture is cut off at the right and bottom)
 * while the window is smaller than 640 x 480 pixels, which the minimum
 * window size prevents except in transit.
 *
 * Minimum window size. While not fullscreen the window's minimum content
 * size is 640 s x 480 s pixels (s = the setting; 1 for "auto"), converted to
 * points and capped at the largest content area of the screen. When the
 * setting or the screen raises it above the window's size, the window is
 * enlarged. Where the cap bites (scale 3 on a small screen), the effective
 * scale drops instead.
 *
 * Pointer mapping. gfx maps a logical vertex position c to native c * s +
 * 0.5 (bf_gfx.h), so a quad the client draws over the logical pixel (x, y),
 * from (x, y) to (x + 1, y + 1), covers exactly the native pixels
 * [x s, (x + 1) s) in each direction. A pointer at window pixel position p
 * (continuous, AppKit's points times the backing scale) is therefore over
 * the logical pixel floor(p / s). The continuous logical position this
 * module returns is p / s ("area" convention: pixel x spans [x, x + 1)); in
 * the vertex convention of the client and Direct3D 7 (pixel centres on
 * integers) the same point is p / s - 0.5 / s. Back: the centre of the
 * logical pixel (x, y) is at the window pixel ((x + 0.5) s, (y + 0.5) s).
 *
 * The flow. bf_view_sync() reads the window's current size (which the app
 * module updates synchronously, before the RESIZE event is queued), applies
 * the rule, and when anything changed: sets the minimum window size, sets
 * the device's drawable size and resizes the device when the logical size or
 * scale changed (bf_gfx_set_drawable_size, bf_gfx_resize), stores the new
 * metrics and calls the listener. The client calls it at the start of every
 * frame and passes RESIZE and FULLSCREEN events to bf_view_handle_event;
 * bf_view_set_modal_frame keeps frames, and syncs, going during a live
 * resize. A frame drawn after bf_view_sync is therefore never drawn or
 * presented at a stale size, whichever order the events arrive in.
 * Fullscreen goes through the same path: its RESIZE is an ordinary resize.
 *
 * Threads: the main thread (the one that called bf_app_init), except where a
 * comment says "any thread". Failure is a return value plus a BFT_ERR line.
 */
#ifndef BF_VIEW_H
#define BF_VIEW_H

#include <stdint.h>

#include "bf_app.h"
#include "bf_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BF_VIEW_OK    = 0,
	BF_VIEW_ERROR = -1
};

enum {
	BF_VIEW_SCALE_AUTO = 0,     /* setting: see the rule above */
	BF_VIEW_SCALE_MAX  = 3,
	BF_VIEW_MIN_LOGICAL_W = 640, BF_VIEW_MIN_LOGICAL_H = 480,   /* the floor */
	BF_VIEW_AUTO_LOGICAL_W = 800, BF_VIEW_AUTO_LOGICAL_H = 600  /* what "auto" keeps */
};

typedef struct bf_view_metrics {
	int drawable_w, drawable_h;   /* the window's content in pixels = the layer's drawable */
	double width_pt, height_pt;   /* the same in points */
	double backing_scale;         /* points to pixels, 1.0 or 2.0 */
	int scale_setting;            /* BF_VIEW_SCALE_AUTO or 1..3, as set */
	int ui_scale;                 /* in effect, 1..3 */
	int logical_w, logical_h;     /* what the client draws in (C3dDevice::GetWidth/GetHeight) */
	int view_w, view_h;           /* logical * ui_scale: the native pixels the logical screen covers */
	int fullscreen;               /* as of the last FULLSCREEN event (or the window at init) */
	uint32_t serial;              /* 0 before init; increases by one with every change */
} bf_view_metrics;

/* ---- the rule: pure functions, any thread --------------------------------- */

/* The UI scale in effect for a drawable size and a setting (0 = auto, 1..3; others count as auto). */
int bf_view_pick_scale(int drawable_w, int drawable_h, int setting);

/*
 * Fills *out from a drawable size, a backing scale and a setting: ui_scale,
 * logical, view and the sizes; width_pt/height_pt = pixels / backing;
 * fullscreen and serial 0. BF_VIEW_ERROR (and *out zeroed) for a size < 1
 * or a backing scale <= 0.
 */
int bf_view_compute(int drawable_w, int drawable_h, double backing_scale, int setting,
                    bf_view_metrics *out);

/* Minimum content size in pixels for a setting: 640 s x 480 s, auto as 1. */
void bf_view_min_content_px(int setting, int *width_px, int *height_px);

/* ---- pointer mapping: pure functions on a metrics record, any thread ----- */

/*
 * Window pixels (bf_app pointer x_px, y_px: points times backing) to the
 * continuous logical position, area convention: floor() gives the logical
 * pixel. Not clamped: positions over the leftover strip or outside the
 * window map beyond 0 .. logical - 1.
 */
void bf_view_px_to_logical(const bf_view_metrics *m, double x_px, double y_px, double *lx, double *ly);
void bf_view_pt_to_logical(const bf_view_metrics *m, double x_pt, double y_pt, double *lx, double *ly);

/*
 * The logical pixel under a window pixel position, clamped into
 * 0 .. logical - 1 (what the DirectInput adapter reports). Returns 1 when
 * the position is over the logical screen, 0 when it had to be clamped.
 */
int bf_view_px_to_logical_pixel(const bf_view_metrics *m, double x_px, double y_px, int *lx, int *ly);

/* The inverse of bf_view_px_to_logical: window pixels = logical * ui_scale. */
void bf_view_logical_to_px(const bf_view_metrics *m, double lx, double ly, double *x_px, double *y_px);

/*
 * The centre of the logical pixel (lx, ly)'s native block, in window pixels
 * and in points: where an injected pointer event aims to hit that pixel.
 */
void bf_view_logical_pixel_center_px(const bf_view_metrics *m, int lx, int ly, double *x_px, double *y_px);
void bf_view_logical_pixel_center_pt(const bf_view_metrics *m, int lx, int ly, double *x_pt, double *y_pt);

/* ---- the view of the one window ---------------------------------------- */

/*
 * Called after a change, on the main thread, from inside bf_view_sync (and
 * so from bf_view_handle_event, bf_view_set_scale_setting, bf_view_attach_gfx
 * and the modal frame), after the device has been resized: what the client's
 * resize path (M4.3a) hooks to update its renderer size, projection, picking
 * and window-manager bounds. `before` is the previous record (serial 0 at the
 * first call). The listener may read but must not call bf_view_sync.
 */
typedef void (*bf_view_listener)(const bf_view_metrics *now, const bf_view_metrics *before, void *context);

/*
 * Starts the view for the window bf_app_window_create made: computes the
 * metrics, sets the window's minimum size (enlarging it if needed). Call
 * before creating the graphics device, whose logical size and scale come
 * from bf_view_get. BF_VIEW_ERROR when there is no window.
 */
int bf_view_init(int scale_setting);
void bf_view_shutdown(void);

/*
 * The device the view keeps in step (NULL: none). Sets its drawable size to
 * the window's pixels and resizes it if its logical size or scale differ
 * from the metrics. The device must be on the window's layer
 * (bf_app_window_metal_layer) or offscreen; the view does not own it.
 */
void bf_view_attach_gfx(bf_gfx_device *dev);

void bf_view_set_listener(bf_view_listener fn, void *context);

/* Changes the setting (0 = auto, 1..3) and syncs at once. */
int bf_view_set_scale_setting(int setting);
int bf_view_scale_setting(void);

/*
 * Brings the metrics, the minimum window size and the device up to date
 * with the window. Returns 1 when the metrics changed (the listener has been
 * called), 0 when not, BF_VIEW_ERROR before init or without a window.
 * Cheap when nothing changed; call it at the start of every frame.
 */
int bf_view_sync(void);

/*
 * Feeds an event from bf_app_poll_event: RESIZE syncs; FULLSCREEN records
 * the state and syncs (in fullscreen the minimum size is not enforced). Other
 * events are ignored. Returns what bf_view_sync returned, 0 for others.
 */
int bf_view_handle_event(const bf_app_event *event);

/* The current metrics (serial 0 before init). Any thread. */
void bf_view_get(bf_view_metrics *out);

/*
 * Installs the app module's modal tick (bf_app_set_modal_tick) so that while
 * AppKit runs a loop of its own (live resize, an open menu) every tick calls
 * bf_view_sync and then frame(context), which draws and presents one frame
 * and must not call bf_app_pump. NULL removes it.
 */
void bf_view_set_modal_frame(void (*frame)(void *context), void *context);

/* Native fullscreen: bf_app_window_toggle_fullscreen; the change arrives as events. */
void bf_view_toggle_fullscreen(void);

#ifdef __cplusplus
}
#endif

#endif
