// bf_view: logical resolution, UI scale and the resize flow. See bf_view.h.

#include "bf_view.h"
#include "bf_trace.h"

#include <math.h>
#include <mutex>
#include <string.h>

namespace {

std::mutex g_lock;               // guards g_cur for bf_view_get from other threads
bf_view_metrics g_cur;           // what the client was last told
bool g_inited = false;
bool g_syncing = false;
int g_setting = BF_VIEW_SCALE_AUTO;
int g_fullscreen = 0;
bf_gfx_device *g_dev = nullptr;
bf_view_listener g_listener = nullptr;
void *g_listener_ctx = nullptr;
void (*g_frame)(void *) = nullptr;
void *g_frame_ctx = nullptr;
double g_min_w_pt = 0, g_min_h_pt = 0;   // the minimum last given to the window

int clamp_setting(int s)
{
	return s >= 1 && s <= BF_VIEW_SCALE_MAX ? s : BF_VIEW_SCALE_AUTO;
}

bool same_view(const bf_view_metrics &a, const bf_view_metrics &b)
{
	return a.drawable_w == b.drawable_w && a.drawable_h == b.drawable_h && a.width_pt == b.width_pt &&
	       a.height_pt == b.height_pt && a.backing_scale == b.backing_scale &&
	       a.scale_setting == b.scale_setting && a.ui_scale == b.ui_scale && a.logical_w == b.logical_w &&
	       a.logical_h == b.logical_h && a.fullscreen == b.fullscreen;
}

// Sets the window's minimum size for the setting and enlarges the window
// when it is below it. Returns true when the window was resized.
bool enforce_min_size(const bf_app_size &a)
{
	if (a.scale <= 0)
		return false;
	int min_px_w, min_px_h;
	bf_view_min_content_px(g_setting, &min_px_w, &min_px_h);
	double w = ceil(min_px_w / a.scale), h = ceil(min_px_h / a.scale);
	bf_app_screen sc;
	if (bf_app_window_get_screen(&sc) == BF_APP_OK && sc.visible_width_pt > 0 && sc.visible_height_pt > 0) {
		if (w > sc.visible_width_pt) w = floor(sc.visible_width_pt);
		if (h > sc.visible_height_pt) h = floor(sc.visible_height_pt);
	}
	if (w != g_min_w_pt || h != g_min_h_pt) {
		bf_app_window_set_min_content_size(w, h);
		g_min_w_pt = w;
		g_min_h_pt = h;
		BF_TRACE(BFT_GFX, "view: minimum window %.0fx%.0f pt for UI scale setting %d\n", w, h, g_setting);
	}
	if (g_fullscreen || (a.width_pt >= w && a.height_pt >= h))
		return false;
	double nw = a.width_pt < w ? w : a.width_pt, nh = a.height_pt < h ? h : a.height_pt;
	BF_TRACE(BFT_GFX, "view: window %.0fx%.0f pt is below the minimum, enlarged to %.0fx%.0f pt\n",
	         a.width_pt, a.height_pt, nw, nh);
	bf_app_window_set_content_size(nw, nh);
	return true;
}

// Brings the device in line with m. Returns false when the resize failed.
bool update_device(const bf_view_metrics &m)
{
	if (!g_dev)
		return true;
	int dw = 0, dh = 0, lw = 0, lh = 0, s = 0;
	bf_gfx_get_drawable_size(g_dev, &dw, &dh);
	bf_gfx_get_size(g_dev, &lw, &lh, &s, nullptr, nullptr);
	bool ok = true;
	if (lw != m.logical_w || lh != m.logical_h || s != m.ui_scale) {
		int rc = bf_gfx_resize(g_dev, m.logical_w, m.logical_h, m.ui_scale);
		if (rc != BF_GFX_OK) {
			bf_tracef(BFT_ERR, "view: bf_gfx_resize(%d, %d, %d) failed (%d)\n", m.logical_w, m.logical_h,
			          m.ui_scale, rc);
			ok = false;
		}
	}
	if (dw != m.drawable_w || dh != m.drawable_h)
		bf_gfx_set_drawable_size(g_dev, m.drawable_w, m.drawable_h);
	return ok;
}

void modal_tick(void *)
{
	bf_view_sync();
	if (g_frame)
		g_frame(g_frame_ctx);
}

}  // namespace

// ---- the rule ------------------------------------------------------------------

extern "C" int bf_view_pick_scale(int w, int h, int setting)
{
	setting = clamp_setting(setting);
	if (setting != BF_VIEW_SCALE_AUTO) {
		for (int s = setting; s > 1; --s)
			if (w / s >= BF_VIEW_MIN_LOGICAL_W && h / s >= BF_VIEW_MIN_LOGICAL_H)
				return s;
		return 1;
	}
	for (int s = BF_VIEW_SCALE_MAX; s > 1; --s)
		if (w / s >= BF_VIEW_AUTO_LOGICAL_W && h / s >= BF_VIEW_AUTO_LOGICAL_H)
			return s;
	return 1;
}

extern "C" int bf_view_compute(int w, int h, double backing, int setting, bf_view_metrics *out)
{
	if (!out)
		return BF_VIEW_ERROR;
	memset(out, 0, sizeof *out);
	if (w < 1 || h < 1 || !(backing > 0))
		return BF_VIEW_ERROR;
	setting = clamp_setting(setting);
	int s = bf_view_pick_scale(w, h, setting);
	out->drawable_w = w;
	out->drawable_h = h;
	out->backing_scale = backing;
	out->width_pt = w / backing;
	out->height_pt = h / backing;
	out->scale_setting = setting;
	out->ui_scale = s;
	out->logical_w = w / s < BF_VIEW_MIN_LOGICAL_W ? BF_VIEW_MIN_LOGICAL_W : w / s;
	out->logical_h = h / s < BF_VIEW_MIN_LOGICAL_H ? BF_VIEW_MIN_LOGICAL_H : h / s;
	out->view_w = out->logical_w * s;
	out->view_h = out->logical_h * s;
	return BF_VIEW_OK;
}

extern "C" void bf_view_min_content_px(int setting, int *w, int *h)
{
	setting = clamp_setting(setting);
	int s = setting == BF_VIEW_SCALE_AUTO ? 1 : setting;
	if (w) *w = BF_VIEW_MIN_LOGICAL_W * s;
	if (h) *h = BF_VIEW_MIN_LOGICAL_H * s;
}

// ---- pointer mapping -------------------------------------------------------------

extern "C" void bf_view_px_to_logical(const bf_view_metrics *m, double x, double y, double *lx, double *ly)
{
	double s = m && m->ui_scale > 0 ? m->ui_scale : 1;
	if (lx) *lx = x / s;
	if (ly) *ly = y / s;
}

extern "C" void bf_view_pt_to_logical(const bf_view_metrics *m, double x, double y, double *lx, double *ly)
{
	double b = m && m->backing_scale > 0 ? m->backing_scale : 1;
	bf_view_px_to_logical(m, x * b, y * b, lx, ly);
}

extern "C" int bf_view_px_to_logical_pixel(const bf_view_metrics *m, double x, double y, int *lx, int *ly)
{
	double fx, fy;
	bf_view_px_to_logical(m, x, y, &fx, &fy);
	double ix = floor(fx), iy = floor(fy);
	int w = m ? m->logical_w : 0, h = m ? m->logical_h : 0;
	int inside = ix >= 0 && iy >= 0 && ix < w && iy < h;
	if (ix > w - 1) ix = w - 1;
	if (iy > h - 1) iy = h - 1;
	if (ix < 0) ix = 0;
	if (iy < 0) iy = 0;
	if (lx) *lx = int(ix);
	if (ly) *ly = int(iy);
	return inside;
}

extern "C" void bf_view_logical_to_px(const bf_view_metrics *m, double lx, double ly, double *x, double *y)
{
	double s = m && m->ui_scale > 0 ? m->ui_scale : 1;
	if (x) *x = lx * s;
	if (y) *y = ly * s;
}

extern "C" void bf_view_logical_pixel_center_px(const bf_view_metrics *m, int lx, int ly, double *x, double *y)
{
	bf_view_logical_to_px(m, lx + 0.5, ly + 0.5, x, y);
}

extern "C" void bf_view_logical_pixel_center_pt(const bf_view_metrics *m, int lx, int ly, double *x, double *y)
{
	double b = m && m->backing_scale > 0 ? m->backing_scale : 1;
	double px, py;
	bf_view_logical_pixel_center_px(m, lx, ly, &px, &py);
	if (x) *x = px / b;
	if (y) *y = py / b;
}

// ---- the view of the window ------------------------------------------------------

extern "C" int bf_view_init(int setting)
{
	if (!bf_app_window_metal_layer()) {
		bf_tracef(BFT_ERR, "view: bf_view_init without a window\n");
		return BF_VIEW_ERROR;
	}
	g_setting = clamp_setting(setting);
	g_fullscreen = bf_app_window_is_fullscreen();
	g_min_w_pt = g_min_h_pt = 0;
	{
		std::lock_guard<std::mutex> g(g_lock);
		memset(&g_cur, 0, sizeof g_cur);
	}
	g_inited = true;
	return bf_view_sync() < 0 ? BF_VIEW_ERROR : BF_VIEW_OK;
}

extern "C" void bf_view_shutdown(void)
{
	if (g_frame)
		bf_app_set_modal_tick(nullptr, nullptr);
	g_frame = nullptr;
	g_dev = nullptr;
	g_listener = nullptr;
	g_inited = false;
	std::lock_guard<std::mutex> g(g_lock);
	memset(&g_cur, 0, sizeof g_cur);
}

extern "C" void bf_view_attach_gfx(bf_gfx_device *dev)
{
	g_dev = dev;
	if (!dev || !g_inited)
		return;
	bf_view_metrics m;
	bf_view_get(&m);
	update_device(m);
	bf_view_sync();
}

extern "C" void bf_view_set_listener(bf_view_listener fn, void *context)
{
	g_listener = fn;
	g_listener_ctx = context;
}

extern "C" int bf_view_set_scale_setting(int setting)
{
	g_setting = clamp_setting(setting);
	return g_inited ? bf_view_sync() : 0;
}

extern "C" int bf_view_scale_setting(void)
{
	return g_setting;
}

extern "C" int bf_view_sync(void)
{
	if (!g_inited)
		return BF_VIEW_ERROR;
	if (g_syncing)
		return 0;
	g_syncing = true;
	bf_app_size a;
	bf_app_window_get_size(&a);
	if (a.width_px > 0 && a.height_px > 0 && enforce_min_size(a))
		bf_app_window_get_size(&a);
	bf_view_metrics m;
	if (bf_view_compute(a.width_px, a.height_px, a.scale, g_setting, &m) != BF_VIEW_OK) {
		g_syncing = false;
		bf_tracef(BFT_ERR, "view: the window has no size (%dx%d px)\n", a.width_px, a.height_px);
		return BF_VIEW_ERROR;
	}
	m.width_pt = a.width_pt;
	m.height_pt = a.height_pt;
	m.fullscreen = g_fullscreen;
	update_device(m);
	bf_view_metrics before;
	{
		std::lock_guard<std::mutex> g(g_lock);
		before = g_cur;
	}
	bool changed = before.serial == 0 || !same_view(before, m);
	if (changed) {
		m.serial = before.serial + 1;
		{
			std::lock_guard<std::mutex> g(g_lock);
			g_cur = m;
		}
		BF_TRACE(BFT_GFX, "view: %dx%d px (%.0fx%.0f pt, backing %.1f), UI scale %d (setting %d) -> logical %dx%d%s\n",
		         m.drawable_w, m.drawable_h, m.width_pt, m.height_pt, m.backing_scale, m.ui_scale,
		         m.scale_setting, m.logical_w, m.logical_h, m.fullscreen ? ", fullscreen" : "");
		if (g_listener)
			g_listener(&m, &before, g_listener_ctx);
	}
	g_syncing = false;
	return changed ? 1 : 0;
}

extern "C" int bf_view_handle_event(const bf_app_event *e)
{
	if (!e || !g_inited)
		return 0;
	if (e->type == BF_APP_EVENT_FULLSCREEN) {
		g_fullscreen = e->value ? 1 : 0;
		return bf_view_sync();
	}
	if (e->type == BF_APP_EVENT_RESIZE)
		return bf_view_sync();
	return 0;
}

extern "C" void bf_view_get(bf_view_metrics *out)
{
	if (!out)
		return;
	std::lock_guard<std::mutex> g(g_lock);
	*out = g_cur;
}

extern "C" void bf_view_set_modal_frame(void (*frame)(void *), void *context)
{
	g_frame = frame;
	g_frame_ctx = context;
	bf_app_set_modal_tick(frame ? modal_tick : nullptr, nullptr);
}

extern "C" void bf_view_toggle_fullscreen(void)
{
	bf_app_window_toggle_fullscreen();
}
