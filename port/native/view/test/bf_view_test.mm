// bifrost-view-test: the view module with a real window and the Metal
// renderer behind it.
//
//   bifrost-view-test [output directory]
//
// 1. The rule and the pointer mapping as pure functions, against values
//    worked out by hand.
// 2. A window, a gfx device on its layer, and the view between them. For UI
//    scale 1, 2, 3 and auto at a 4:3 (1024x768 pt) and a wide (1680x720 pt)
//    window, resized programmatically: the next presented drawable has the
//    window's pixel size, the logical size is the rule's, every native pixel
//    of an ID frame (each logical pixel its own colour) belongs to its
//    logical pixel's s x s block, a 1-pixel checkerboard comes out as clean
//    blocks, and pointer positions (edges, block borders, centres, random,
//    one injected through the event queue) map to the logical pixel the
//    frame shows under them. A capture of a test scene per case goes into
//    the contact sheet.
// 3. The minimum window size, a simulated live resize (AppKit's tracking
//    run-loop mode, the modal frame drawing and resizing between ticks), and
//    fullscreen (for real on an unlocked screen, simulated on a locked one).
//
// No game data. Opens a window. Exits non-zero on a failure.

#import <AppKit/AppKit.h>
#import <CoreText/CoreText.h>
#import <ImageIO/ImageIO.h>
#import <QuartzCore/CAMetalLayer.h>

#include "bf_view.h"
#include "bf_trace.h"

#include <algorithm>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

int g_failures = 0, g_checks = 0;
std::string g_out;

void check(bool ok, const char *fmt, ...)
{
	char buf[768];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	++g_checks;
	if (!ok) ++g_failures;
	printf("%-4s %s\n", ok ? "ok" : "FAIL", buf);
	fflush(stdout);
}

void info(const char *fmt, ...)
{
	char buf[768];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	printf("info %s\n", buf);
	fflush(stdout);
}

bool ScreenLocked()
{
	CFDictionaryRef d = CGSessionCopyCurrentDictionary();
	if (!d) return true;
	CFBooleanRef locked = (CFBooleanRef)CFDictionaryGetValue(d, CFSTR("CGSSessionScreenIsLocked"));
	bool r = locked && CFBooleanGetValue(locked);
	CFRelease(d);
	return r;
}

double NowMs() { return [NSProcessInfo processInfo].systemUptime * 1000.0; }

// ---- 1. the rule, by hand ----------------------------------------------------------

struct RuleCase { int w, h, setting; int scale, lw, lh; const char *why; };

void test_rule()
{
	const RuleCase cases[] = {
		{2048, 1536, 0, 2, 1024, 768, "1024x768 pt Retina, auto"},
		{2048, 1536, 3, 3, 682, 512, "1024x768 pt Retina, 3: 2 px left over at the right"},
		{3360, 1440, 3, 3, 1120, 480, "1680x720 pt Retina, 3: exactly 480 high"},
		{3200, 1400, 3, 2, 1600, 700, "1600x700 pt Retina, 3 asked: 466 < 480, so 2"},
		{3200, 1400, 0, 2, 1600, 700, "1600x700 pt Retina, auto"},
		{1920, 1080, 0, 1, 1920, 1080, "1080p at 1x, auto: 960x540 < 800x600"},
		{2560, 1440, 0, 2, 1280, 720, "1440p at 1x, auto"},
		{5120, 2880, 0, 3, 1706, 960, "5K fullscreen, auto"},
		{2940, 1912, 0, 3, 980, 637, "13-inch laptop fullscreen, auto"},
		{1280, 960, 2, 2, 640, 480, "the floor exactly at 2"},
		{1279, 960, 2, 1, 1279, 960, "one pixel short of it: 1"},
		{600, 400, 1, 1, 640, 480, "below the floor: logical clamped to 640x480"},
		{2048, 1536, 7, 2, 1024, 768, "an unknown setting counts as auto"},
		{1599, 1200, 0, 1, 1599, 1200, "auto: 799 < 800, so 1"},
	};
	for (const RuleCase &c : cases) {
		bf_view_metrics m;
		int rc = bf_view_compute(c.w, c.h, 2.0, c.setting, &m);
		check(rc == BF_VIEW_OK && m.ui_scale == c.scale && m.logical_w == c.lw && m.logical_h == c.lh &&
		      m.view_w == c.lw * c.scale && m.view_h == c.lh * c.scale && m.drawable_w == c.w &&
		      bf_view_pick_scale(c.w, c.h, c.setting) == c.scale,
		      "rule %dx%d px, setting %d -> scale %d, logical %dx%d, view %dx%d (%s)", c.w, c.h, c.setting,
		      m.ui_scale, m.logical_w, m.logical_h, m.view_w, m.view_h, c.why);
	}
	bf_view_metrics m;
	check(bf_view_compute(0, 100, 2.0, 1, &m) == BF_VIEW_ERROR && bf_view_compute(100, 100, 0, 1, &m) == BF_VIEW_ERROR,
	      "rule: a zero size or backing scale is an error");
	int w, h, ok = 1;
	for (int s = 0; s <= 3; ++s) {
		bf_view_min_content_px(s, &w, &h);
		int k = s ? s : 1;
		ok &= w == 640 * k && h == 480 * k;
		// At the minimum size the setting is never lowered.
		if (s) ok &= bf_view_pick_scale(w, h, s) == s;
	}
	check(ok, "minimum content size 640s x 480s px (auto as 1); at that size the asked scale holds");

	// Pointer mapping by hand at scale 3, logical 682x512 in a 2048x1536 drawable.
	bf_view_compute(2048, 1536, 2.0, 3, &m);
	struct P { double x, y; int lx, ly, inside; } pts[] = {
		{0, 0, 0, 0, 1}, {2.999, 2.999, 0, 0, 1}, {3, 3, 1, 1, 1}, {5.99, 6, 1, 2, 1},
		{2045.9, 1535.9, 681, 511, 1},   // last native pixel of the view
		{2046, 100, 681, 33, 0},          // the leftover strip: clamped
		{-1, -1, 0, 0, 0}, {5000, 5000, 681, 511, 0}};
	ok = 1;
	for (const P &p : pts) {
		int lx, ly;
		int in = bf_view_px_to_logical_pixel(&m, p.x, p.y, &lx, &ly);
		if (lx != p.lx || ly != p.ly || in != p.inside) {
			ok = 0;
			printf("     px (%.3f, %.3f) -> (%d, %d) inside %d, want (%d, %d) %d\n", p.x, p.y, lx, ly, in, p.lx, p.ly, p.inside);
		}
	}
	double cx, cy, ptx, pty;
	bf_view_logical_pixel_center_px(&m, 10, 20, &cx, &cy);
	bf_view_logical_pixel_center_pt(&m, 10, 20, &ptx, &pty);
	ok &= cx == 31.5 && cy == 61.5 && ptx == 15.75 && pty == 30.75;
	double lx, ly;
	bf_view_pt_to_logical(&m, 15.75, 30.75, &lx, &ly);
	ok &= lx == 10.5 && ly == 20.5;
	check(ok, "pointer mapping by hand at scale 3: block borders, last pixel, leftover strip and outside clamp; "
	          "centre of (10,20) at (31.5,61.5) px = (15.75,30.75) pt and back to (10.5,20.5)");

	// Round trip over every logical pixel of a row and column at each scale.
	ok = 1;
	for (int s = 1; s <= 3; ++s) {
		bf_view_compute(640 * s + s - 1, 480 * s, 2.0, s, &m);
		for (int x = 0; x < m.logical_w; ++x)
			for (int k = 0; k < s; ++k) {
				int a, b;
				bf_view_px_to_logical_pixel(&m, x * s + k + 0.5, 0.5, &a, &b);
				if (a != x) ok = 0;
				double px, py;
				bf_view_logical_pixel_center_px(&m, x, 0, &px, &py);
				bf_view_px_to_logical_pixel(&m, px, py, &a, &b);
				if (a != x || b != 0) ok = 0;
			}
	}
	check(ok, "every native pixel of a row maps to floor(x / s), every logical pixel centre maps back to itself (s = 1..3)");
}

// ---- drawing ----------------------------------------------------------------------

struct TL1 { float x, y, z, rhw; uint32_t diffuse, specular; float u, v; };

void quad(bf_gfx_device *d, float x0, float y0, float x1, float y1, uint32_t c,
          float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1)
{
	TL1 v[4] = {{x0, y0, 0, 1, c, 0xffffffffu, u0, v0}, {x1, y0, 0, 1, c, 0xffffffffu, u1, v0},
	            {x0, y1, 0, 1, c, 0xffffffffu, u0, v1}, {x1, y1, 0, 1, c, 0xffffffffu, u1, v1}};
	bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4);
}

void set_flat(bf_gfx_device *d, bool textured)
{
	bf_gfx_state s;
	bf_gfx_state_init(&s);
	s.cull = BF_GFX_CULL_NONE;
	s.depth_enable = 0;
	s.depth_write = 0;
	s.alpha_test_enable = 0;
	s.blend_enable = 0;
	bf_gfx_stage_state &a = s.stage[0];
	a.color_op = textured ? BF_GFX_OP_MODULATE : BF_GFX_OP_SELECT_ARG2;
	a.color_arg1 = BF_GFX_ARG_TEXTURE; a.color_arg2 = BF_GFX_ARG_DIFFUSE;
	a.alpha_op = textured ? BF_GFX_OP_MODULATE : BF_GFX_OP_SELECT_ARG2;
	a.alpha_arg1 = BF_GFX_ARG_TEXTURE; a.alpha_arg2 = BF_GFX_ARG_DIFFUSE;
	a.min_filter = a.mag_filter = BF_GFX_FILTER_LINEAR;   // the client's setting; UI textures override
	a.address_u = a.address_v = BF_GFX_ADDRESS_CLAMP;
	s.stage[1].color_op = BF_GFX_OP_DISABLE;
	s.stage[1].alpha_op = BF_GFX_OP_DISABLE;
	bf_gfx_set_state(d, &s);
}

// A texture drawn 1:1 at logical (x, y) the way CTexture::DrawSurface does: uv = (texel + 0.5) / size.
void blit(bf_gfx_device *d, bf_gfx_texture *t, int x, int y, int w, int h)
{
	set_flat(d, true);
	bf_gfx_set_texture(d, 0, t);
	quad(d, float(x), float(y), float(x + w), float(y + h), 0xffffffffu, 0.5f / w, 0.5f / h, (w + 0.5f) / w, (h + 0.5f) / h);
	bf_gfx_set_texture(d, 0, nullptr);
}

uint32_t id_color(int x, int y)
{
	return 0xff000000u | uint32_t(x & 255) << 16 | uint32_t(y & 255) << 8 | uint32_t(((x >> 8) & 15) | ((y >> 8) & 15) << 4);
}

void id_decode(const uint8_t *p, int *x, int *y)   // p: B G R A
{
	*x = p[2] | (p[0] & 15) << 8;
	*y = p[1] | (p[0] >> 4) << 8;
}

// Every logical pixel its own colour.
void draw_id_frame(bf_gfx_device *d, const bf_view_metrics &m)
{
	bf_gfx_texture *t = bf_gfx_texture_create(d, m.logical_w, m.logical_h, BF_GFX_FORMAT_A8R8G8B8, 1);
	int pitch = 0;
	uint8_t *p = (uint8_t *)bf_gfx_texture_lock(t, &pitch);
	for (int y = 0; y < m.logical_h; ++y)
		for (int x = 0; x < m.logical_w; ++x)
			((uint32_t *)(p + size_t(y) * size_t(pitch)))[x] = id_color(x, y);
	bf_gfx_texture_unlock(t, nullptr);
	bf_gfx_texture_set_filter_override(t, BF_GFX_FILTER_NEAREST);
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR | BF_GFX_CLEAR_DEPTH, 0xff000000u, 1, nullptr);
	blit(d, t, 0, 0, m.logical_w, m.logical_h);
	bf_gfx_finish(d);
	bf_gfx_texture_destroy(t);
}

// A label rendered unsmoothed into a texture, as the text module will.
bf_gfx_texture *label_texture(bf_gfx_device *d, const std::string &s, int *w, int *h)
{
	CTFontRef font = CTFontCreateWithName(CFSTR("Menlo"), 11, nullptr);
	CFStringRef str = CFStringCreateWithCString(nullptr, s.c_str(), kCFStringEncodingUTF8);
	CGColorRef white = CGColorCreateGenericRGB(1, 1, 1, 1);
	const void *keys[] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
	const void *vals[] = {font, white};
	CFDictionaryRef attr = CFDictionaryCreate(nullptr, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFAttributedStringRef as = CFAttributedStringCreate(nullptr, str, attr);
	CTLineRef line = CTLineCreateWithAttributedString(as);
	double width = CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
	*w = int(ceil(width)) + 4;
	*h = 16;
	std::vector<uint32_t> px(size_t(*w) * size_t(*h), 0);
	CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef cg = CGBitmapContextCreate(px.data(), size_t(*w), size_t(*h), 8, size_t(*w) * 4, cs,
	                                        kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
	CGContextSetShouldAntialias(cg, false);
	CGContextSetShouldSmoothFonts(cg, false);
	CGContextSetTextPosition(cg, 2, 4);
	CTLineDraw(line, cg);
	CGContextRelease(cg);
	CGColorSpaceRelease(cs);
	CFRelease(line); CFRelease(as); CFRelease(attr); CGColorRelease(white); CFRelease(str); CFRelease(font);
	for (uint32_t &v : px) v = (v >> 24) >= 128 ? 0xffffffffu : 0xff1c2a44u;   // a panel background
	bf_gfx_texture *t = bf_gfx_texture_create(d, *w, *h, BF_GFX_FORMAT_A8R8G8B8, 1);
	bf_gfx_texture_update(t, 0, 0, *w, *h, px.data(), 0);
	bf_gfx_texture_set_filter_override(t, BF_GFX_FILTER_NEAREST);
	return t;
}

const int kChkX = 16, kChkY = 40, kChk = 64;

// The test scene: a smooth "world" behind, a 1-pixel frame at the logical
// edges, a 1-pixel checkerboard and a text panel as the UI.
void draw_scene(bf_gfx_device *d, const bf_view_metrics &m, bf_gfx_texture *checker, const std::string &text)
{
	float W = float(m.logical_w), H = float(m.logical_h);
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR | BF_GFX_CLEAR_DEPTH, 0xff203040u, 1, nullptr);
	set_flat(d, false);
	TL1 world[3] = {{0, H, 0, 1, 0xff2e6b3au, 0, 0, 0}, {W, H, 0, 1, 0xff6b5a2eu, 0, 0, 0},
	                {W * 0.6f, H * 0.25f, 0, 1, 0xff4a86c8u, 0, 0, 0}};
	bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_LIST, BF_GFX_VERTEX_TL1, world, 3);
	for (int i = 0; i < 9; ++i) {   // a fan of thin lines: world geometry at native resolution
		TL1 l[2] = {{W * 0.6f, H * 0.25f, 0, 1, 0xffe0e0e0u, 0, 0, 0},
		            {W * (0.2f + 0.1f * i), H - 1, 0, 1, 0xffe0e0e0u, 0, 0, 0}};
		bf_gfx_draw(d, BF_GFX_PRIM_LINE_LIST, BF_GFX_VERTEX_TL1, l, 2);
	}
	quad(d, 0, 0, W, 1, 0xffffffffu);
	quad(d, 0, H - 1, W, H, 0xffffffffu);
	quad(d, 0, 0, 1, H, 0xffff4040u);
	quad(d, W - 1, 0, W, H, 0xffff4040u);
	blit(d, checker, kChkX, kChkY, kChk, kChk);
	int tw, th;
	bf_gfx_texture *t = label_texture(d, text, &tw, &th);
	blit(d, t, kChkX, 16, tw, th);
	bf_gfx_finish(d);
	bf_gfx_texture_destroy(t);
}

std::vector<uint8_t> read_native(bf_gfx_device *d)
{
	int nw, nh;
	bf_gfx_get_size(d, nullptr, nullptr, nullptr, &nw, &nh);
	std::vector<uint8_t> px(size_t(nw) * size_t(nh) * 4);
	bf_gfx_rect r{0, 0, nw, nh};
	bf_gfx_read_pixels(d, BF_GFX_SPACE_NATIVE, &r, px.data(), 0);
	return px;
}

// ---- captures and the contact sheet -----------------------------------------------

struct Capture { std::string name, label; int w, h, s; std::vector<uint8_t> bgra; };
std::vector<Capture> g_caps;

void write_png(const std::string &path, int w, int h, const uint8_t *bgra)
{
	CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef cg = CGBitmapContextCreate((void *)bgra, size_t(w), size_t(h), 8, size_t(w) * 4, cs,
	                                        kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
	CGImageRef img = CGBitmapContextCreateImage(cg);
	NSURL *url = [NSURL fileURLWithPath:@(path.c_str())];
	CGImageDestinationRef dst = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, CFSTR("public.png"), 1, nullptr);
	CGImageDestinationAddImage(dst, img, nullptr);
	if (!CGImageDestinationFinalize(dst)) fprintf(stderr, "cannot write %s\n", path.c_str());
	CFRelease(dst);
	CGImageRelease(img);
	CGContextRelease(cg);
	CGColorSpaceRelease(cs);
}

// One row per capture: the whole frame scaled down, and the top left corner
// (label, checkerboard) at native pixels, magnified so the blocks show.
void contact_sheet(const std::string &path)
{
	const int margin = 10, thumbW = 640, thumbH = 280, cropN = 180, zoom = 2, rowH = std::max(thumbH, cropN * zoom) + 34;
	int W = margin * 3 + thumbW + cropN * zoom, H = 30 + int(g_caps.size()) * rowH + margin;
	std::vector<uint8_t> sheet(size_t(W) * size_t(H) * 4);
	for (size_t i = 0; i < sheet.size(); i += 4) { sheet[i] = sheet[i + 1] = sheet[i + 2] = 0x30; sheet[i + 3] = 255; }
	CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef cg = CGBitmapContextCreate(sheet.data(), size_t(W), size_t(H), 8, size_t(W) * 4, cs,
	                                        kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
	CTFontRef font = CTFontCreateWithName(CFSTR("Menlo"), 11, nullptr);
	CTFontRef big = CTFontCreateWithName(CFSTR("Menlo-Bold"), 13, nullptr);
	auto text = [&](const std::string &s, double x, double y, CTFontRef f) {
		CFStringRef str = CFStringCreateWithCString(nullptr, s.c_str(), kCFStringEncodingUTF8);
		CGColorRef white = CGColorCreateGenericRGB(0.92, 0.92, 0.92, 1);
		const void *keys[] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
		const void *vals[] = {f, white};
		CFDictionaryRef attr = CFDictionaryCreate(nullptr, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFAttributedStringRef as = CFAttributedStringCreate(nullptr, str, attr);
		CTLineRef line = CTLineCreateWithAttributedString(as);
		CGContextSetTextPosition(cg, x, H - y);
		CTLineDraw(line, cg);
		CFRelease(line); CFRelease(as); CFRelease(attr); CGColorRelease(white); CFRelease(str);
	};
	text("bifrost-view-test: synthetic frames at UI scale 1, 2, 3 and auto, 4:3 and wide windows (native pixels)", margin, 20, big);
	for (size_t i = 0; i < g_caps.size(); ++i) {
		const Capture &c = g_caps[i];
		int y0 = 30 + int(i) * rowH;
		CGContextRef src = CGBitmapContextCreate((void *)c.bgra.data(), size_t(c.w), size_t(c.h), 8, size_t(c.w) * 4, cs,
		                                         kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
		CGImageRef img = CGBitmapContextCreateImage(src);
		double f = std::min(double(thumbW) / c.w, double(thumbH) / c.h);
		int dw = int(c.w * f), dh = int(c.h * f);
		CGContextSetInterpolationQuality(cg, kCGInterpolationHigh);
		CGContextDrawImage(cg, CGRectMake(margin, H - (y0 + dh), dw, dh), img);
		CGImageRef crop = CGImageCreateWithImageInRect(img, CGRectMake(0, 0, std::min(cropN, c.w), std::min(cropN, c.h)));
		CGContextSetInterpolationQuality(cg, kCGInterpolationNone);
		CGContextDrawImage(cg, CGRectMake(margin * 2 + thumbW, H - (y0 + cropN * zoom), cropN * zoom, cropN * zoom), crop);
		CGImageRelease(crop);
		CGImageRelease(img);
		CGContextRelease(src);
		text(c.label, margin, y0 + dh + 16, font);
		text("top left 180x180 native px, x2", margin * 2 + thumbW, y0 + cropN * zoom + 16, font);
	}
	CFRelease(font);
	CFRelease(big);
	CGContextFlush(cg);
	CGContextRelease(cg);
	CGColorSpaceRelease(cs);
	write_png(path, W, H, sheet.data());
}

// ---- 2. the window ------------------------------------------------------------------

bf_gfx_device *g_dev = nullptr;
bf_gfx_texture *g_checker = nullptr;
int g_listener_calls = 0;
bf_view_metrics g_heard;

void listener(const bf_view_metrics *now, const bf_view_metrics *before, void *)
{
	++g_listener_calls;
	g_heard = *now;
}

// Pumps and hands every event to the view; returns how many changed it.
int pump_events(double ms, std::vector<bf_app_event> *seen = nullptr)
{
	int changes = 0;
	double end = NowMs() + ms;
	do {
		bf_app_pump(ms > 0 ? 5 : 0);
		bf_app_event e;
		while (bf_app_poll_event(&e)) {
			if (seen) seen->push_back(e);
			if (bf_view_handle_event(&e) == 1) ++changes;
		}
	} while (NowMs() < end);
	return changes;
}

// The rule written out again, independently of bf_view.cpp.
void expect(const bf_app_size &a, int setting, int *s, int *lw, int *lh)
{
	int k;
	if (setting >= 1 && setting <= 3) {
		for (k = setting; k > 1 && (a.width_px / k < 640 || a.height_px / k < 480); --k) {}
	} else {
		for (k = 3; k > 1 && (a.width_px / k < 800 || a.height_px / k < 600); --k) {}
	}
	*s = k;
	*lw = std::max(640, a.width_px / k);
	*lh = std::max(480, a.height_px / k);
}

const char *setting_name(int s)
{
	static const char *n[] = {"auto", "1", "2", "3"};
	return n[s & 3];
}

void window_case(double wpt, double hpt, int setting, const char *shape)
{
	bf_view_set_scale_setting(setting);
	bf_app_window_set_content_size(wpt, hpt);
	int heard_before = g_listener_calls;

	// The frame right after the resize, before any event is pumped.
	int changed = bf_view_sync();
	bf_view_metrics m;
	bf_view_get(&m);
	bf_app_size a;
	bf_app_window_get_size(&a);
	draw_id_frame(g_dev, m);
	int prc = bf_gfx_present(g_dev);
	int pw = 0, ph = 0, nw = 0, nh = 0, lw = 0, lh = 0, gs = 0;
	bf_gfx_get_present_size(g_dev, &pw, &ph);
	bf_gfx_get_size(g_dev, &lw, &lh, &gs, &nw, &nh);
	int es, elw, elh;
	expect(a, setting, &es, &elw, &elh);
	char tag[96];
	snprintf(tag, sizeof tag, "%s %.0fx%.0f pt, setting %s", shape, a.width_pt, a.height_pt, setting_name(setting));
	check(prc == BF_GFX_OK && pw == a.width_px && ph == a.height_px && m.drawable_w == a.width_px && m.drawable_h == a.height_px,
	      "%s: first frame after the resize presented a %dx%d drawable = window %dx%d px", tag, pw, ph, a.width_px, a.height_px);
	check(m.ui_scale == es && m.logical_w == elw && m.logical_h == elh && lw == m.logical_w && lh == m.logical_h &&
	      gs == m.ui_scale && nw == m.view_w && nh == m.view_h,
	      "%s: scale %d, logical %dx%d (rule: %d, %dx%d), device %dx%d at %d, target %dx%d, leftover %dx%d px",
	      tag, m.ui_scale, m.logical_w, m.logical_h, es, elw, elh, lw, lh, gs, nw, nh, a.width_px - nw, a.height_px - nh);
	check(g_listener_calls == heard_before + (changed == 1 ? 1 : 0) && g_heard.serial == m.serial &&
	      g_heard.logical_w == m.logical_w && g_heard.ui_scale == m.ui_scale,
	      "%s: the listener heard the change once (serial %u)", tag, m.serial);

	// Every native pixel belongs to its logical pixel's s x s block.
	std::vector<uint8_t> px = read_native(g_dev);
	int s = m.ui_scale;
	long bad = 0;
	for (int y = 0; y < nh; ++y)
		for (int x = 0; x < nw; ++x) {
			int dx, dy;
			id_decode(&px[(size_t(y) * size_t(nw) + size_t(x)) * 4], &dx, &dy);
			if (dx != x / s || dy != y / s) ++bad;
		}
	check(bad == 0, "%s: ID frame: every one of %d native pixels lies in its logical pixel's %dx%d block (%ld wrong)",
	      tag, nw * nh, s, s, bad);

	// Pointer positions in points -> logical pixel, against what the frame shows there.
	std::vector<std::pair<double, double>> probes;   // in window pixels
	auto add_px = [&](double x, double y) { probes.push_back({x, y}); };
	add_px(0, 0);
	add_px(nw - 0.25, nh - 0.25);
	add_px(nw - 0.25, 0);
	add_px(0, nh - 0.25);
	for (int k = 1; k <= 3; ++k) { add_px(k * s, k * s); add_px(k * s - 0.25, k * s - 0.25); }
	int cxs[] = {0, m.logical_w / 2, m.logical_w - 1}, cys[] = {0, m.logical_h / 2, m.logical_h - 1};
	for (int cx : cxs) for (int cy : cys) { double x, y; bf_view_logical_pixel_center_px(&m, cx, cy, &x, &y); add_px(x, y); }
	srand(unsigned(nw * 31 + nh));
	for (int k = 0; k < 40; ++k) add_px(rand() % (nw * 8) / 8.0, rand() % (nh * 8) / 8.0);
	int hit = 0, total = 0;
	for (auto &p : probes) {
		double xpt = p.first / m.backing_scale, ypt = p.second / m.backing_scale;
		double fx, fy;
		bf_view_pt_to_logical(&m, xpt, ypt, &fx, &fy);
		int lx = int(floor(fx)), ly = int(floor(fy));
		int dx, dy;
		int ix = int(floor(p.first)), iy = int(floor(p.second));
		id_decode(&px[(size_t(iy) * size_t(nw) + size_t(ix)) * 4], &dx, &dy);
		++total;
		if (dx == lx && dy == ly) ++hit;
		else printf("     pointer (%.2f, %.2f) pt -> logical (%d, %d), frame shows (%d, %d)\n", xpt, ypt, lx, ly, dx, dy);
	}
	check(hit == total, "%s: pointer mapping: %d of %d positions (corners, block borders, centres, random) hit the "
	      "logical pixel the frame shows under them", tag, hit, total);

	// The same through the event queue: an injected move to the centre of the last logical pixel.
	{
		double x, y;
		bf_view_logical_pixel_center_px(&m, m.logical_w - 1, m.logical_h - 1, &x, &y);
		bf_app_event e = {};
		e.type = BF_APP_EVENT_POINTER_MOVE;
		e.pointer.x_px = float(x);
		e.pointer.y_px = float(y);
		bf_app_post_event(&e);
		std::vector<bf_app_event> seen;
		pump_events(0, &seen);
		bool found = false;
		int lx = -1, ly = -1, ipx = -1, ipy = -1;
		for (auto &ev : seen)
			if (ev.type == BF_APP_EVENT_POINTER_MOVE && ev.source == BF_APP_SOURCE_INJECTED) {
				double fx, fy;
				bf_view_pt_to_logical(&m, ev.pointer.x_pt, ev.pointer.y_pt, &fx, &fy);
				lx = int(floor(fx)); ly = int(floor(fy));
				bf_view_px_to_logical_pixel(&m, ev.pointer.x_px, ev.pointer.y_px, &ipx, &ipy);
				found = true;
			}
		check(found && lx == m.logical_w - 1 && ly == m.logical_h - 1 && ipx == lx && ipy == ly,
		      "%s: injected pointer at the centre of logical (%d, %d): its pt maps to (%d, %d), its px to (%d, %d)",
		      tag, m.logical_w - 1, m.logical_h - 1, lx, ly, ipx, ipy);
	}
	// Events arriving after the frame (RESIZE) change nothing more.
	int late = pump_events(80);
	bf_view_metrics m2;
	bf_view_get(&m2);
	check(late == 0 && m2.serial == m.serial, "%s: the RESIZE events pumped afterwards change nothing (%d changes)", tag, late);

	// The scene: checkerboard and frame edges, then the capture.
	char label[160];
	snprintf(label, sizeof label, "UI scale %d (%s), logical %dx%d, window %dx%d px", s, setting_name(setting),
	         m.logical_w, m.logical_h, a.width_px, a.height_px);
	draw_scene(g_dev, m, g_checker, label);
	bf_gfx_present(g_dev);
	px = read_native(g_dev);
	bad = 0;
	for (int y = kChkY * s; y < (kChkY + kChk) * s; ++y)
		for (int x = kChkX * s; x < (kChkX + kChk) * s; ++x) {
			const uint8_t *p = &px[(size_t(y) * size_t(nw) + size_t(x)) * 4];
			int v = ((x / s - kChkX + y / s - kChkY) & 1) ? 255 : 0;
			if (p[0] != v || p[1] != v || p[2] != v) ++bad;
		}
	check(bad == 0, "%s: 1-pixel checkerboard: %d native pixels in clean %dx%d blocks (%ld wrong)", tag,
	      kChk * kChk * s * s, s, s, bad);
	bad = 0;
	for (int y = s; y < nh - s; y += 7)
		for (int k = 0; k < s; ++k) {
			const uint8_t *l = &px[(size_t(y) * size_t(nw) + size_t(k)) * 4];
			const uint8_t *r = &px[(size_t(y) * size_t(nw) + size_t(nw - 1 - k)) * 4];
			if (!(l[2] == 255 && l[1] == 0x40) || !(r[2] == 255 && r[1] == 0x40)) ++bad;
		}
	for (int x = s; x < nw - s; x += 5)
		for (int k = 0; k < s; ++k) {
			const uint8_t *t = &px[(size_t(k) * size_t(nw) + size_t(x)) * 4];
			const uint8_t *b = &px[(size_t(nh - 1 - k) * size_t(nw) + size_t(x)) * 4];
			if (t[0] != 255 || t[1] != 255 || t[2] != 255 || b[0] != 255 || b[1] != 255 || b[2] != 255) ++bad;
		}
	// ... and the pixel inside the frame is not frame any more.
	{
		const uint8_t *in = &px[(size_t(nh / 2) * size_t(nw) + size_t(s)) * 4];
		if (in[2] == 255 && in[1] == 0x40) ++bad;
	}
	check(bad == 0, "%s: the 1-pixel frame at the logical edges is %d native pixels wide on all four sides (%ld wrong)", tag, s, bad);

	Capture c;
	snprintf(tag, sizeof tag, "%s_%dx%dpt_%s", shape, int(a.width_pt), int(a.height_pt), setting_name(setting));
	c.name = tag;
	c.label = label;
	c.w = nw; c.h = nh; c.s = s;
	c.bgra = px;
	std::string path = g_out + "/" + c.name + ".png";
	check(bf_gfx_capture_png(g_dev, path.c_str()) == BF_GFX_OK, "capture %s (%dx%d)", path.c_str(), nw, nh);
	g_caps.push_back(std::move(c));
}

// ---- 3. live resize -----------------------------------------------------------------

struct LiveRecord { int win_w, win_h, pres_w, pres_h, lw, lh, s, ok_logical; };
std::vector<LiveRecord> g_live;
bool g_live_present_ok = true;

void live_frame(void *)
{
	bf_view_metrics m;
	bf_view_get(&m);
	set_flat(g_dev, false);
	bf_gfx_clear(g_dev, BF_GFX_CLEAR_COLOR, 0xff203040u, 1, nullptr);
	quad(g_dev, 0, 0, float(m.logical_w), 1, 0xffffffffu);
	if (bf_gfx_present(g_dev) != BF_GFX_OK) g_live_present_ok = false;
	bf_app_size a;
	bf_app_window_get_size(&a);
	LiveRecord r;
	r.win_w = a.width_px; r.win_h = a.height_px;
	bf_gfx_get_present_size(g_dev, &r.pres_w, &r.pres_h);
	r.lw = m.logical_w; r.lh = m.logical_h; r.s = m.ui_scale;
	int es, elw, elh;
	expect(a, bf_view_scale_setting(), &es, &elw, &elh);
	r.ok_logical = es == m.ui_scale && elw == m.logical_w && elh == m.logical_h;
	g_live.push_back(r);
}

void test_live_resize()
{
	bf_view_set_scale_setting(BF_VIEW_SCALE_AUTO);
	bf_app_window_set_content_size(1024, 768);
	bf_view_sync();
	bf_view_set_modal_frame(live_frame, nullptr);
	g_live.clear();
	// AppKit runs its own loop in the tracking mode while the user drags an
	// edge; the size changes between ticks. Here the test is the user.
	double t0 = NowMs();
	int step = 0, scales_seen = 0, last_s = 0;
	while (NowMs() - t0 < 1500) {
		[[NSRunLoop mainRunLoop] runMode:NSEventTrackingRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.03]];
		if (NowMs() - t0 < step * 20.0)
			continue;   // a drag moves the edge about every 20 ms
		++step;
		double w = 1024 + (step * 37) % 900, h = 768 - (step * 13) % 200;
		bf_app_window_set_content_size(w, h);
	}
	bf_view_set_modal_frame(nullptr, nullptr);
	int match = 0, logical_ok = 0;
	for (auto &r : g_live) {
		if (r.pres_w == r.win_w && r.pres_h == r.win_h) ++match;
		if (r.ok_logical) ++logical_ok;
		if (r.s != last_s) { ++scales_seen; last_s = r.s; }
	}
	int n = int(g_live.size());
	check(n >= 30 && g_live_present_ok, "live resize (simulated in the tracking run-loop mode): %d frames drawn by the modal "
	      "frame in 1.5 s over %d size changes", n, step);
	check(n > 0 && match == n && logical_ok == n, "live resize: every frame presented at the window's size of the moment "
	      "(%d of %d) with the rule's logical size (%d of %d); UI scale changed %d times (auto)", match, n, logical_ok, n,
	      scales_seen - 1);
	pump_events(50);
}

// ---- minimum size and fullscreen -----------------------------------------------------

void test_min_size()
{
	bf_view_set_scale_setting(3);
	bf_app_window_set_content_size(1600, 700);
	bf_view_sync();
	bf_view_metrics m;
	bf_view_get(&m);
	double mw, mh;
	bf_app_window_get_min_content_size(&mw, &mh);
	bf_app_screen sc;
	bf_app_window_get_screen(&sc);
	double want_w = std::min(ceil(1920 / m.backing_scale), floor(sc.visible_width_pt));
	double want_h = std::min(ceil(1440 / m.backing_scale), floor(sc.visible_height_pt));
	check(mw == want_w && mh == want_h, "minimum window for scale 3: %.0fx%.0f pt (1920x1440 px at backing %.1f, capped at the "
	      "screen's %.0fx%.0f)", mw, mh, m.backing_scale, sc.visible_width_pt, sc.visible_height_pt);
	check(m.height_pt >= want_h && m.width_pt >= 1600 && (m.ui_scale == 3 || want_h < 720),
	      "window asked for 1600x700 pt at scale 3 was enlarged to %.0fx%.0f pt: scale %d, logical %dx%d",
	      m.width_pt, m.height_pt, m.ui_scale, m.logical_w, m.logical_h);
	bf_view_set_scale_setting(BF_VIEW_SCALE_AUTO);
	bf_app_window_get_min_content_size(&mw, &mh);
	check(mw == ceil(640 / m.backing_scale) && mh == ceil(480 / m.backing_scale),
	      "minimum window for auto: %.0fx%.0f pt (640x480 px)", mw, mh);
	pump_events(50);
}

void present_now()
{
	bf_view_metrics m;
	bf_view_get(&m);
	set_flat(g_dev, false);
	bf_gfx_clear(g_dev, BF_GFX_CLEAR_COLOR, 0xff203040u, 1, nullptr);
	bf_gfx_present(g_dev);
}

void test_fullscreen()
{
	bf_view_set_scale_setting(BF_VIEW_SCALE_AUTO);
	bf_app_window_set_content_size(1024, 768);
	pump_events(50);
	bf_app_screen sc;
	bf_app_window_get_screen(&sc);
	bool locked = ScreenLocked();
	bool real = false;
	if (!locked) {
		bf_view_toggle_fullscreen();
		double end = NowMs() + 5000;
		while (NowMs() < end && !real) {
			bf_app_pump(10);
			bf_app_event e;
			while (bf_app_poll_event(&e)) {
				bf_view_handle_event(&e);
				if (e.type == BF_APP_EVENT_FULLSCREEN && e.value == 1) real = true;
			}
		}
		pump_events(300);
	}
	if (!real) {
		// What fullscreen does to the view: a FULLSCREEN event and a resize to the screen.
		info("fullscreen: %s, so it is simulated: an injected FULLSCREEN(1) and a resize to the screen's largest content area",
		     locked ? "the screen is locked and macOS refuses fullscreen" : "fullscreen did not start within 5 s");
		bf_app_event e = {};
		e.type = BF_APP_EVENT_FULLSCREEN;
		e.value = 1;
		bf_app_post_event(&e);
		bf_app_window_set_content_size(sc.visible_width_pt, sc.visible_height_pt);
		pump_events(100);
	}
	bf_view_sync();
	bf_view_metrics m;
	bf_view_get(&m);
	present_now();
	int pw, ph;
	bf_gfx_get_present_size(g_dev, &pw, &ph);
	bf_app_size a;
	bf_app_window_get_size(&a);
	int es, elw, elh;
	expect(a, 0, &es, &elw, &elh);
	check(m.fullscreen == 1 && pw == a.width_px && ph == a.height_px && m.logical_w == elw && m.logical_h == elh && m.ui_scale == es,
	      "fullscreen (%s): view says fullscreen, %dx%d px presented, scale %d, logical %dx%d", real ? "real" : "simulated",
	      pw, ph, m.ui_scale, m.logical_w, m.logical_h);
	if (real)
		check(a.width_pt == sc.width_pt && a.height_pt == sc.height_pt, "fullscreen: the content is the whole screen (%.0fx%.0f pt)",
		      a.width_pt, a.height_pt);

	// In fullscreen the window is not enlarged for the minimum: the scale drops instead.
	if (!real) {
		bf_app_window_set_content_size(1000, 700);
		bf_view_sync();
	}
	bf_view_set_scale_setting(3);
	bf_view_get(&m);
	bf_app_window_get_size(&a);
	expect(a, 3, &es, &elw, &elh);
	check(m.fullscreen == 1 && m.ui_scale == es && m.logical_w == elw && (real || (a.width_pt == 1000 && a.height_pt == 700)),
	      "fullscreen, scale 3 asked at %.0fx%.0f pt: window left as it is, scale %d, logical %dx%d", a.width_pt, a.height_pt,
	      m.ui_scale, m.logical_w, m.logical_h);
	bf_view_set_scale_setting(BF_VIEW_SCALE_AUTO);

	if (real) {
		bf_view_toggle_fullscreen();
		double end = NowMs() + 5000;
		bool left = false;
		while (NowMs() < end && !left) {
			bf_app_pump(10);
			bf_app_event e;
			while (bf_app_poll_event(&e)) {
				bf_view_handle_event(&e);
				if (e.type == BF_APP_EVENT_FULLSCREEN && e.value == 0) left = true;
			}
		}
		pump_events(300);
	} else {
		bf_app_event e = {};
		e.type = BF_APP_EVENT_FULLSCREEN;
		e.value = 0;
		bf_app_post_event(&e);
		bf_app_window_set_content_size(1024, 768);
		pump_events(100);
	}
	bf_view_sync();
	bf_view_get(&m);
	present_now();
	bf_gfx_get_present_size(g_dev, &pw, &ph);
	bf_app_window_get_size(&a);
	check(m.fullscreen == 0 && pw == a.width_px && ph == a.height_px, "fullscreen left (%s): %dx%d px presented, logical %dx%d",
	      real ? "real" : "simulated", pw, ph, m.logical_w, m.logical_h);
}

}  // namespace

int main(int argc, char **argv)
{
	@autoreleasepool {
		bf_trace_init();
		g_out = argc > 1 ? argv[1] : BF_VIEW_TEST_OUT;
		[[NSFileManager defaultManager] createDirectoryAtPath:@(g_out.c_str()) withIntermediateDirectories:YES attributes:nil error:nil];

		test_rule();

		char id[64];
		snprintf(id, sizeof id, "view-test-%d", int(getpid()));
		bf_app_config cfg = {id, 1};
		if (bf_app_init(&cfg) != BF_APP_OK) { check(false, "bf_app_init"); return 1; }
		check(bf_view_init(1) == BF_VIEW_ERROR, "bf_view_init without a window fails");
		if (bf_app_window_create(1024, 768, "bifrost-view-test") != BF_APP_OK) { check(false, "window"); return 1; }
		bf_view_set_listener(listener, nullptr);
		check(bf_view_init(1) == BF_VIEW_OK, "bf_view_init on the window");
		bf_view_metrics m;
		bf_view_get(&m);
		bf_gfx_desc desc = {bf_app_window_metal_layer(), m.logical_w, m.logical_h, m.ui_scale};
		g_dev = bf_gfx_create(&desc);
		if (!g_dev) { check(false, "bf_gfx_create on the window's layer"); return 1; }
		bf_view_attach_gfx(g_dev);
		int dw, dh;
		bf_gfx_get_drawable_size(g_dev, &dw, &dh);
		CGSize ds = [(__bridge CAMetalLayer *)bf_app_window_metal_layer() drawableSize];
		info("device %s; window %.0fx%.0f pt, %dx%d px, backing %.1f; screen %s", bf_gfx_device_name(g_dev), m.width_pt,
		     m.height_pt, m.drawable_w, m.drawable_h, m.backing_scale, ScreenLocked() ? "LOCKED" : "unlocked");
		check(dw == m.drawable_w && dh == m.drawable_h && ds.width == m.drawable_w && ds.height == m.drawable_h,
		      "attach: device and layer drawable %dx%d = window pixels", dw, dh);

		// The checkerboard texture of the scene: 1-pixel cells, sampled unfiltered as UI is.
		g_checker = bf_gfx_texture_create(g_dev, kChk, kChk, BF_GFX_FORMAT_A8R8G8B8, 1);
		std::vector<uint32_t> cb(size_t(kChk) * kChk);
		for (int i = 0; i < kChk * kChk; ++i) cb[size_t(i)] = ((i % kChk + i / kChk) & 1) ? 0xffffffffu : 0xff000000u;
		bf_gfx_texture_update(g_checker, 0, 0, kChk, kChk, cb.data(), 0);
		bf_gfx_texture_set_filter_override(g_checker, BF_GFX_FILTER_NEAREST);
		pump_events(200);

		for (int s : {1, 2, 3, 0}) window_case(1024, 768, s, "4x3");
		for (int s : {1, 2, 3, 0}) window_case(1680, 720, s, "wide");

		test_min_size();
		test_live_resize();
		test_fullscreen();

		std::string sheet = g_out + "/contact-sheet.png";
		contact_sheet(sheet);
		info("contact sheet: %s (%zu captures)", sheet.c_str(), g_caps.size());

		bf_gfx_texture_destroy(g_checker);
		bf_view_shutdown();
		bf_gfx_destroy(g_dev);
		bf_app_shutdown();
		printf("%s: %d checks, %d failed\n", g_failures ? "FAIL" : "PASS", g_checks, g_failures);
		return g_failures ? 1 : 0;
	}
}
