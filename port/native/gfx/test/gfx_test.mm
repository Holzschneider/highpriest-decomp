// bifrost-gfx-test: offscreen test draws for the gfx module, from generated
// textures only. Prints one line per check, exits non-zero on a failure,
// writes every capture as a PNG and a labelled contact sheet into
// <build>/test-out (or the directory given as the first argument).

#include "bf_gfx.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <ImageIO/ImageIO.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int g_failures = 0, g_checks = 0;
static std::string g_out;

static void check(bool ok, const char *fmt, ...)
{
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	++g_checks;
	if (!ok) ++g_failures;
	printf("%-4s %s\n", ok ? "ok" : "FAIL", buf);
}

// ---- captures and the contact sheet -----------------------------------------

struct Capture {
	std::string name, label;
	int w, h;
	std::vector<uint8_t> bgra;
};
static std::vector<Capture> g_caps;

static void write_png(const std::string &path, int w, int h, const uint8_t *bgra)
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

static std::vector<uint8_t> read_native(bf_gfx_device *d)
{
	int nw, nh;
	bf_gfx_get_size(d, nullptr, nullptr, nullptr, &nw, &nh);
	std::vector<uint8_t> px(size_t(nw) * size_t(nh) * 4);
	bf_gfx_rect r{0, 0, nw, nh};
	if (bf_gfx_read_pixels(d, BF_GFX_SPACE_NATIVE, &r, px.data(), 0) != BF_GFX_OK) check(false, "read_pixels");
	return px;
}

static void capture(bf_gfx_device *d, const std::string &name, const std::string &label)
{
	Capture c;
	c.name = name;
	c.label = label;
	bf_gfx_get_size(d, nullptr, nullptr, nullptr, &c.w, &c.h);
	c.bgra = read_native(d);
	std::string path = g_out + "/" + name + ".png";
	if (bf_gfx_capture_png(d, path.c_str()) != BF_GFX_OK) check(false, "capture_png %s", path.c_str());
	g_caps.push_back(std::move(c));
}

static void contact_sheet(const std::string &path)
{
	const int cell = 200, image = 176, cols = 8, margin = 8;
	int rows = int((g_caps.size() + cols - 1) / cols);
	int W = cols * cell + margin, H = rows * (image + 36) + margin + 24;
	std::vector<uint8_t> sheet(size_t(W) * size_t(H) * 4);
	for (size_t i = 0; i < sheet.size(); i += 4) { sheet[i] = 0x30; sheet[i + 1] = 0x30; sheet[i + 2] = 0x30; sheet[i + 3] = 255; }
	CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	CGContextRef cg = CGBitmapContextCreate(sheet.data(), size_t(W), size_t(H), 8, size_t(W) * 4, cs,
	                                        kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
	CGContextSetInterpolationQuality(cg, kCGInterpolationNone);
	CGContextSetShouldAntialias(cg, true);
	CTFontRef font = CTFontCreateWithName(CFSTR("Menlo"), 10, nullptr);
	CTFontRef big = CTFontCreateWithName(CFSTR("Menlo-Bold"), 13, nullptr);
	auto text = [&](const std::string &s, double x, double y, CTFontRef f) {
		CFStringRef str = CFStringCreateWithCString(nullptr, s.c_str(), kCFStringEncodingUTF8);
		CGColorRef white = CGColorCreateGenericRGB(0.92, 0.92, 0.92, 1);
		const void *keys[] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
		const void *vals[] = {f, white};
		CFDictionaryRef attr = CFDictionaryCreate(nullptr, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks,
		                                          &kCFTypeDictionaryValueCallBacks);
		CFAttributedStringRef as = CFAttributedStringCreate(nullptr, str, attr);
		CTLineRef line = CTLineCreateWithAttributedString(as);
		CGContextSetTextPosition(cg, x, H - y);
		CTLineDraw(line, cg);
		CFRelease(line); CFRelease(as); CFRelease(attr); CGColorRelease(white); CFRelease(str);
	};
	text("bifrost-gfx-test: synthetic test draws (generated textures), native pixels, scaled by an integer", margin, 18, big);
	for (size_t i = 0; i < g_caps.size(); ++i) {
		const Capture &c = g_caps[i];
		int col = int(i % cols), row = int(i / cols);
		int x0 = margin + col * cell, y0 = 28 + row * (image + 36);
		int f = std::max(1, std::min(image / c.w, image / c.h));
		int dw = c.w * f, dh = c.h * f;
		if (dw > image || dh > image) { double s = std::min(double(image) / c.w, double(image) / c.h); dw = int(c.w * s); dh = int(c.h * s); }
		CGContextRef src = CGBitmapContextCreate((void *)c.bgra.data(), size_t(c.w), size_t(c.h), 8, size_t(c.w) * 4, cs,
		                                         kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
		CGImageRef img = CGBitmapContextCreateImage(src);
		CGContextSetRGBFillColor(cg, 0.5, 0.5, 0.5, 1);
		CGContextFillRect(cg, CGRectMake(x0 - 1, H - (y0 + dh) - 1, dw + 2, dh + 2));
		CGContextDrawImage(cg, CGRectMake(x0, H - (y0 + dh), dw, dh), img);
		CGImageRelease(img);
		CGContextRelease(src);
		std::string l1 = c.label, l2;
		size_t cut = l1.find('|');
		if (cut != std::string::npos) { l2 = l1.substr(cut + 1); l1 = l1.substr(0, cut); }
		text(l1, x0, y0 + image + 12, font);
		if (!l2.empty()) text(l2, x0, y0 + image + 24, font);
	}
	CFRelease(font);
	CFRelease(big);
	CGContextFlush(cg);
	CGContextRelease(cg);
	CGColorSpaceRelease(cs);
	write_png(path, W, H, sheet.data());
}

// ---- vertices, states and the client's settings -------------------------------

struct TL1 { float x, y, z, rhw; uint32_t diffuse, specular; float u, v; };
struct TL2 { float x, y, z, rhw; uint32_t diffuse, specular; float u, v, u1, v1; };

// A quad in the client's strip order (CTexture::DrawSurface).
static void quad(TL1 v[4], float x0, float y0, float x1, float y1, uint32_t color,
                 float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1, float z = 0.00001f, uint32_t spec = 0xffffffffu)
{
	v[0] = {x0, y0, z, 0.99999f, color, spec, u0, v0};
	v[1] = {x1, y0, z, 0.99999f, color, spec, u1, v0};
	v[2] = {x0, y1, z, 0.99999f, color, spec, u0, v1};
	v[3] = {x1, y1, z, 0.99999f, color, spec, u1, v1};
}

static void draw_quad(bf_gfx_device *d, float x0, float y0, float x1, float y1, uint32_t color,
                      float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1, float z = 0.00001f, uint32_t spec = 0xffffffffu)
{
	TL1 v[4];
	quad(v, x0, y0, x1, y1, color, u0, v0, u1, v1, z, spec);
	bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4);
}

// C3dDevice::Init (src/3dlib/Device.cpp), decoded.
static void client_init_state(bf_gfx_state *s)
{
	bf_gfx_state_init(s);
	s->cull = BF_GFX_CULL_NONE;
	s->depth_func = BF_GFX_CMP_LESS_EQUAL;
	s->alpha_func = BF_GFX_CMP_GREATER_EQUAL;
	s->alpha_ref = 0xf0;
	s->depth_enable = 1;
	s->depth_write = 1;
	s->src_blend = BF_GFX_BLEND_SRC_ALPHA;
	s->dst_blend = BF_GFX_BLEND_INV_SRC_ALPHA;
	bf_gfx_stage_state &a = s->stage[0], &b = s->stage[1];
	a.texcoord_index = 0;
	a.color_op = BF_GFX_OP_MODULATE; a.color_arg1 = BF_GFX_ARG_TEXTURE; a.color_arg2 = BF_GFX_ARG_DIFFUSE;
	a.alpha_op = BF_GFX_OP_MODULATE; a.alpha_arg1 = BF_GFX_ARG_TEXTURE; a.alpha_arg2 = BF_GFX_ARG_DIFFUSE;
	b.texcoord_index = 1;
	b.color_arg1 = BF_GFX_ARG_TEXTURE; b.color_op = BF_GFX_OP_MODULATE_ALPHA_ADD_COLOR; b.color_arg2 = BF_GFX_ARG_CURRENT;
	b.alpha_op = BF_GFX_OP_DISABLE;
	a.address_u = a.address_v = BF_GFX_ADDRESS_CLAMP;
	a.min_filter = a.mag_filter = BF_GFX_FILTER_LINEAR;
	b.min_filter = b.mag_filter = BF_GFX_FILTER_LINEAR;
	b.address_u = b.address_v = BF_GFX_ADDRESS_BORDER;
	b.border_color = 0xff000000u;
}

// The state a 2D draw needs: no depth, no alpha test, and stage 1 off as
// CRenderer::FlushRenderList sets it at the start of every frame.
static void flat_state(bf_gfx_state *s)
{
	client_init_state(s);
	s->stage[1].color_op = BF_GFX_OP_DISABLE;
	s->stage[1].alpha_op = BF_GFX_OP_DISABLE;
	s->depth_enable = 0;
	s->alpha_test_enable = 0;
	s->blend_enable = 0;
}

// CRenderer::SetMultiTextureMode (src/3dlib/Renderer.cpp): applied on top of
// whatever the stages held, exactly as the client does (fields it does not
// set keep their values).
static void apply_preset(bf_gfx_state *st, int n)
{
	bf_gfx_stage_state &s0 = st->stage[0], &s1 = st->stage[1];
	const int T = BF_GFX_ARG_TEXTURE, D = BF_GFX_ARG_DIFFUSE, C = BF_GFX_ARG_CURRENT;
	const int CMP = BF_GFX_ARG_COMPLEMENT, AR = BF_GFX_ARG_ALPHA_REPLICATE;
	s0.texcoord_index = 0;
	auto s1_off = [&] { s1.color_op = BF_GFX_OP_DISABLE; s1.alpha_op = BF_GFX_OP_DISABLE; };
	auto s1_set = [&](int a1, int op, int aa1) {
		s1.texcoord_index = 1; s1.color_arg1 = uint8_t(a1); s1.color_op = uint8_t(op); s1.color_arg2 = uint8_t(C);
		s1.alpha_arg1 = uint8_t(aa1); s1.alpha_op = BF_GFX_OP_SELECT_ARG1;
	};
	auto s0_mod_a2 = [&] {  // colour T x D, alpha = diffuse (SELECTARG2)
		s0.color_arg1 = uint8_t(T); s0.color_op = BF_GFX_OP_MODULATE; s0.color_arg2 = uint8_t(D);
		s0.alpha_op = BF_GFX_OP_SELECT_ARG2; s0.alpha_arg2 = uint8_t(D);
	};
	switch (n) {
	case 0:
		s0.color_arg1 = T; s0.color_op = BF_GFX_OP_MODULATE; s0.color_arg2 = D;
		s0.alpha_arg1 = T; s0.alpha_op = BF_GFX_OP_MODULATE; s0.alpha_arg2 = D;
		s1_off(); break;
	case 1: case 2:
		s0.color_arg1 = T; s0.color_op = BF_GFX_OP_MODULATE; s0.color_arg2 = D;
		s0.alpha_arg1 = T; s0.alpha_op = BF_GFX_OP_SELECT_ARG1;
		s1_off(); break;
	case 3:
		s0.color_arg1 = T; s0.color_op = BF_GFX_OP_ADD; s0.color_arg2 = D;
		s0.alpha_op = BF_GFX_OP_SELECT_ARG2; s0.alpha_arg2 = D;
		s1_off(); break;
	case 4:
		s0.color_arg1 = T; s0.color_op = BF_GFX_OP_BLEND_TEXTURE_ALPHA; s0.color_arg2 = D;
		s0.alpha_op = BF_GFX_OP_SELECT_ARG2; s0.alpha_arg2 = D;
		s1_off(); break;
	case 5:  s0_mod_a2(); s1_set(T, BF_GFX_OP_MODULATE, T); break;
	case 6:
		s0.color_arg1 = T | CMP; s0.color_op = BF_GFX_OP_MODULATE; s0.color_arg2 = D;
		s0.alpha_op = BF_GFX_OP_SELECT_ARG2; s0.alpha_arg2 = D;
		s1_set(T | CMP, BF_GFX_OP_MODULATE, T); break;
	case 7:
		s0.color_arg1 = T; s0.color_op = BF_GFX_OP_SELECT_ARG1;
		s0.alpha_arg1 = T; s0.alpha_op = BF_GFX_OP_SELECT_ARG1;
		s1_set(T | AR, BF_GFX_OP_MODULATE, T); break;
	case 8:
		s0.color_arg1 = T; s0.color_op = BF_GFX_OP_MODULATE; s0.color_arg2 = D;
		s0.alpha_arg1 = T; s0.alpha_op = BF_GFX_OP_SELECT_ARG1;
		s1_set(T, BF_GFX_OP_ADD, T); break;
	case 9:  s0_mod_a2(); s1_set(T, BF_GFX_OP_BLEND_TEXTURE_ALPHA, T); break;
	case 10: s0_mod_a2(); s1_set(T, BF_GFX_OP_BLEND_DIFFUSE_ALPHA, T); break;
	case 11: s0_mod_a2(); s1_set(T, BF_GFX_OP_ADD, T); break;
	case 12: s0_mod_a2(); s1_set(T | CMP, BF_GFX_OP_ADD, T); break;
	case 13:
		s0.color_arg1 = T; s0.color_op = BF_GFX_OP_ADD; s0.color_arg2 = D;
		s0.alpha_arg1 = T; s0.alpha_op = BF_GFX_OP_SELECT_ARG1;
		s1_set(T, BF_GFX_OP_MODULATE, T); break;
	case 14: s0_mod_a2(); s1_set(T, BF_GFX_OP_MODULATE_2X, T); break;
	case 15: s0_mod_a2(); s1_set(T, BF_GFX_OP_ADD_SIGNED, T); break;
	}
}

static const char *k_preset_text[16] = {   // colour; alpha (T stage 0, T1 stage 1, D diffuse)
	"T*D; a T*D", "T*D; a T", "T*D; a T", "T+D; a D",
	"lerp(D,T,Ta); a D", "T*D*T1; a T1", "(1-T)D(1-T1); a T1", "T*T1a; a T1",
	"T*D+T1; a T1", "lerp(TD,T1,T1a); a T1", "lerp(TD,T1,Da); a T1", "T*D+T1; a T1",
	"T*D+(1-T1); a T1", "(T+D)*T1; a T1", "2*T*D*T1; a T1", "T*D+T1-.5; a T1",
};

// ---- an independent reference of the Direct3D 7 stage formulas ----------------

struct C4 { float r, g, b, a; };
static C4 c4(uint32_t argb) { return {((argb >> 16) & 255) / 255.f, ((argb >> 8) & 255) / 255.f, (argb & 255) / 255.f, (argb >> 24) / 255.f}; }
static float sat(float x) { return x < 0 ? 0 : x > 1 ? 1 : x; }

static C4 ref_arg(int a, C4 diff, C4 cur, C4 tex, C4 tf)
{
	C4 v = (a & 15) == 0 ? diff : (a & 15) == 1 ? cur : (a & 15) == 2 ? tex : (a & 15) == 3 ? tf : cur;
	if (a & 0x10) v = {1 - v.r, 1 - v.g, 1 - v.b, 1 - v.a};
	if (a & 0x20) v = {v.a, v.a, v.a, v.a};
	return v;
}

static float ref_op1(int op, float x, float y, float da, float ta, float ca, float fa)
{
	switch (op) {
	case 2: return x;
	case 3: return y;
	case 4: return x * y;
	case 5: return 2 * x * y;
	case 6: return 4 * x * y;
	case 7: return x + y;
	case 8: return x + y - 0.5f;
	case 9: return 2 * (x + y - 0.5f);
	case 10: return x - y;
	case 11: return x + y - x * y;
	case 12: return x * da + y * (1 - da);
	case 13: return x * ta + y * (1 - ta);
	case 14: return x * fa + y * (1 - fa);
	case 15: return x + y * (1 - ta);
	case 16: return x * ca + y * (1 - ca);
	default: return x;
	}
}

static C4 ref_stage(const bf_gfx_stage_state &s, C4 diff, C4 cur, C4 tex, C4 tf)
{
	C4 a1 = ref_arg(s.color_arg1, diff, cur, tex, tf), a2 = ref_arg(s.color_arg2, diff, cur, tex, tf);
	C4 out;
	if (s.color_op == 18) out = {a1.r + a1.a * a2.r, a1.g + a1.a * a2.g, a1.b + a1.a * a2.b, 0};
	else {
		out.r = ref_op1(s.color_op, a1.r, a2.r, diff.a, tex.a, cur.a, tf.a);
		out.g = ref_op1(s.color_op, a1.g, a2.g, diff.a, tex.a, cur.a, tf.a);
		out.b = ref_op1(s.color_op, a1.b, a2.b, diff.a, tex.a, cur.a, tf.a);
	}
	if (s.alpha_op == 1) out.a = cur.a;
	else {
		C4 x = ref_arg(s.alpha_arg1, diff, cur, tex, tf), y = ref_arg(s.alpha_arg2, diff, cur, tex, tf);
		out.a = ref_op1(s.alpha_op, x.a, y.a, diff.a, tex.a, cur.a, tf.a);
	}
	return {sat(out.r), sat(out.g), sat(out.b), sat(out.a)};
}

static C4 ref_combine(const bf_gfx_state &st, C4 diff, C4 t0, C4 t1)
{
	C4 tf = c4(st.texture_factor), cur = diff;
	if (st.stage[0].color_op == 1) return cur;
	cur = ref_stage(st.stage[0], diff, cur, t0, tf);
	if (st.stage[1].color_op == 1) return cur;
	return ref_stage(st.stage[1], diff, cur, t1, tf);
}

static int q8(float x) { return int(std::lround(sat(x) * 255.0f)); }

static const uint8_t *px_at(const std::vector<uint8_t> &px, int w, int x, int y) { return &px[(size_t(y) * size_t(w) + size_t(x)) * 4]; }

static bool near_rgb(const uint8_t *p, int r, int g, int b, int tol)
{
	return std::abs(p[2] - r) <= tol && std::abs(p[1] - g) <= tol && std::abs(p[0] - b) <= tol;
}

static bf_gfx_device *make_device(int lw, int lh, int scale)
{
	bf_gfx_desc desc = {nullptr, lw, lh, scale};
	bf_gfx_device *d = bf_gfx_create(&desc);
	if (!d) { check(false, "create %dx%d scale %d", lw, lh, scale); exit(1); }
	return d;
}

// ---- tests ------------------------------------------------------------------------

static void test_half_pixel()
{
	for (int s = 1; s <= 3; ++s) {
		bf_gfx_device *d = make_device(12, 12, s);
		bf_gfx_state st;
		flat_state(&st);
		bf_gfx_set_state(d, &st);
		bf_gfx_clear(d, BF_GFX_CLEAR_COLOR | BF_GFX_CLEAR_DEPTH, 0xff000000u, 1, nullptr);
		draw_quad(d, 0, 0, 4, 4, 0xffffffffu);
		draw_quad(d, 6, 7, 10, 11, 0xffff0000u);
		std::vector<uint8_t> px = read_native(d);
		int n = 12 * s, white = 0, white_in = 0, red = 0, red_in = 0;
		for (int y = 0; y < n; ++y)
			for (int x = 0; x < n; ++x) {
				const uint8_t *p = px_at(px, n, x, y);
				if (p[0] == 255 && p[1] == 255 && p[2] == 255) { ++white; if (x < 4 * s && y < 4 * s) ++white_in; }
				if (p[0] == 0 && p[1] == 0 && p[2] == 255) { ++red; if (x >= 6 * s && x < 10 * s && y >= 7 * s && y < 11 * s) ++red_in; }
			}
		check(white == 16 * s * s && white_in == white, "half-pixel scale %d: quad (0,0)-(4,4) covers exactly native [0,%d)^2 (%d pixels)", s, 4 * s, white);
		check(red == 16 * s * s && red_in == red, "half-pixel scale %d: quad (6,7)-(10,11) covers exactly its %dx%d block (%d pixels)", s, 4 * s, 4 * s, red);
		capture(d, "halfpixel_s" + std::to_string(s), "half-pixel, scale " + std::to_string(s) + "|quads (0,0)-(4,4), (6,7)-(10,11)");
		bf_gfx_destroy(d);
	}

	// The calibration TestScreen searched for: with the 0.5 texel offset and
	// bilinear filtering, every pixel shows exactly its own texel at scale 1.
	bf_gfx_device *d = make_device(8, 8, 1);
	bf_gfx_texture *t = bf_gfx_texture_create(d, 8, 8, BF_GFX_FORMAT_A8R8G8B8, 1);
	std::vector<uint32_t> tx(64);
	for (int i = 0; i < 64; ++i) tx[size_t(i)] = 0xff000000u | uint32_t((i * 37) & 255) << 16 | uint32_t((i * 91) & 255) << 8 | uint32_t((i * 13) & 255);
	bf_gfx_texture_update(t, 0, 0, 8, 8, tx.data(), 0);
	bf_gfx_state st;
	flat_state(&st);
	bf_gfx_set_state(d, &st);
	bf_gfx_set_texture(d, 0, t);
	draw_quad(d, 0, 0, 8, 8, 0xffffffffu, 0.5f / 8, 0.5f / 8, 8.5f / 8, 8.5f / 8);
	std::vector<uint8_t> px = read_native(d);
	int exact = 0;
	for (int i = 0; i < 64; ++i) {
		uint32_t c = tx[size_t(i)];
		if (near_rgb(px_at(px, 8, i % 8, i / 8), (c >> 16) & 255, (c >> 8) & 255, c & 255, 0)) ++exact;
	}
	check(exact == 64, "texel offset 0.5 + bilinear at scale 1: %d/64 pixels exactly their texel (replaces TestScreen)", exact);
	capture(d, "texel_offset", "0.5 texel offset, bilinear|= exact texels (TestScreen)");
	bf_gfx_texture_destroy(t);
	bf_gfx_destroy(d);
}

static void test_checker()
{
	for (int s = 1; s <= 3; ++s) {
		for (int lin = 0; lin < (s == 2 ? 2 : 1); ++lin) {
			bf_gfx_device *d = make_device(24, 24, s);
			bf_gfx_texture *t = bf_gfx_texture_create(d, 16, 16, BF_GFX_FORMAT_A1R5G5B5, 1);
			std::vector<uint16_t> tx(256);
			for (int i = 0; i < 256; ++i) tx[size_t(i)] = ((i % 16 + i / 16) & 1) ? 0xffff : 0x8000;
			bf_gfx_texture_update(t, 0, 0, 16, 16, tx.data(), 0);
			if (!lin) bf_gfx_texture_set_filter_override(t, BF_GFX_FILTER_NEAREST);
			bf_gfx_state st;
			flat_state(&st);
			bf_gfx_set_state(d, &st);
			bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xff204080u, 1, nullptr);
			bf_gfx_set_texture(d, 0, t);
			// CTexture::DrawSurface: uv = (texel + 0.5) / size.
			draw_quad(d, 4, 4, 20, 20, 0xffffffffu, 0.5f / 16, 0.5f / 16, 16.5f / 16, 16.5f / 16);
			std::vector<uint8_t> px = read_native(d);
			int n = 24 * s, bad = 0;
			for (int y = 0; y < n; ++y)
				for (int x = 0; x < n; ++x) {
					int lx = x / s - 4, ly = y / s - 4;
					const uint8_t *p = px_at(px, n, x, y);
					bool inside = lx >= 0 && lx < 16 && ly >= 0 && ly < 16;
					int v = inside ? (((lx + ly) & 1) ? 255 : 0) : -1;
					bool ok = inside ? near_rgb(p, v, v, v, 0) : near_rgb(p, 0x20, 0x40, 0x80, 0);
					if (!ok) ++bad;
				}
			if (!lin)
				check(bad == 0, "checkerboard scale %d, nearest: every native pixel is its logical pixel's colour (%d wrong of %d)", s, bad, n * n);
			else
				printf("info scale 2, bilinear (for comparison, not a check): %d of %d native pixels differ from clean blocks\n", bad, n * n);
			capture(d, std::string("checker_s") + std::to_string(s) + (lin ? "_linear" : ""),
			        std::string("1px checker, scale ") + std::to_string(s) + (lin ? "|LINEAR, for comparison" : "|NEAREST: clean blocks"));
			bf_gfx_texture_destroy(t);
			bf_gfx_destroy(d);
		}
	}
}

static uint32_t expand_a1(uint16_t v)
{
	uint32_t r = (v >> 10) & 31, g = (v >> 5) & 31, b = v & 31;
	return ((v & 0x8000) ? 0xff000000u : 0) | (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
}
static uint32_t expand_a4(uint16_t v)
{
	return (((v >> 12) & 15) * 17u) << 24 | (((v >> 8) & 15) * 17u) << 16 | (((v >> 4) & 15) * 17u) << 8 | (v & 15) * 17u;
}
static uint32_t expand_565(uint16_t v)
{
	uint32_t r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
	return 0xff000000u | (r << 3 | r >> 2) << 16 | (g << 2 | g >> 4) << 8 | (b << 3 | b >> 2);
}

static void test_formats()
{
	bf_gfx_device *d = make_device(64, 40, 1);
	bf_gfx_state st;
	flat_state(&st);
	st.blend_enable = 1;   // SRC_ALPHA / INV_SRC_ALPHA from the client's Init
	st.stage[0].min_filter = st.stage[0].mag_filter = BF_GFX_FILTER_NEAREST;
	bf_gfx_set_state(d, &st);
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xff808080u, 1, nullptr);
	const uint16_t a1[8] = {0x8000, 0xffff, 0xfc00, 0x83e0, 0x801f, 0x7fff, 0x4210 | 0x8000, 0x0421};
	const uint16_t a4[8] = {0xf000, 0xffff, 0x8f00, 0x40f0, 0xc00f, 0x0fff, 0x7777, 0xd123};
	const uint16_t r565[8] = {0x0000, 0xffff, 0xf800, 0x07e0, 0x001f, 0x8410, 0x0821, 0x7bef};
	const uint32_t a8[8] = {0xff000000u, 0xffffffffu, 0x80ff0000u, 0x4000ff00u, 0xc00000ffu, 0x00ffffffu, 0x7f7f7f7fu, 0x01020304u};
	const char *names[4] = {"A1R5G5B5", "A4R4G4B4", "R5G6B5", "A8R8G8B8"};
	bf_gfx_format fmts[4] = {BF_GFX_FORMAT_A1R5G5B5, BF_GFX_FORMAT_A4R4G4B4, BF_GFX_FORMAT_R5G6B5, BF_GFX_FORMAT_A8R8G8B8};
	bf_gfx_texture *tex[4];
	uint32_t expect[4][8];
	for (int f = 0; f < 4; ++f) {
		tex[f] = bf_gfx_texture_create(d, 8, 1, fmts[f], 1);
		for (int i = 0; i < 8; ++i)
			expect[f][i] = f == 0 ? expand_a1(a1[i]) : f == 1 ? expand_a4(a4[i]) : f == 2 ? expand_565(r565[i]) : a8[i];
		bf_gfx_texture_update(tex[f], 0, 0, 8, 1, f == 0 ? (const void *)a1 : f == 1 ? (const void *)a4 : f == 2 ? (const void *)r565 : (const void *)a8, 0);
		bf_gfx_set_texture(d, 0, tex[f]);
		draw_quad(d, 0, float(f * 10), 64, float(f * 10 + 8), 0xffffffffu, 0, 0, 1, 1);
	}
	std::vector<uint8_t> px = read_native(d);
	for (int f = 0; f < 4; ++f) {
		int bad = 0;
		for (int i = 0; i < 8; ++i) {
			uint32_t c = expect[f][i];
			float a = (c >> 24) / 255.f;
			int r = q8(((c >> 16) & 255) / 255.f * a + 128 / 255.f * (1 - a));
			int g = q8(((c >> 8) & 255) / 255.f * a + 128 / 255.f * (1 - a));
			int b = q8((c & 255) / 255.f * a + 128 / 255.f * (1 - a));
			if (!near_rgb(px_at(px, 64, i * 8 + 4, f * 10 + 4), r, g, b, 1)) {
				++bad;
				const uint8_t *p = px_at(px, 64, i * 8 + 4, f * 10 + 4);
				printf("     %s texel %d: got %d %d %d, expected %d %d %d\n", names[f], i, p[2], p[1], p[0], r, g, b);
			}
		}
		check(bad == 0, "format %s: 8 texels widened and alpha-blended over grey as expected (+-1)", names[f]);
	}
	capture(d, "formats", "formats over grey: A1R5G5B5,|A4R4G4B4, R5G6B5, A8R8G8B8");
	for (auto *t : tex) bf_gfx_texture_destroy(t);
	bf_gfx_destroy(d);
}

static void test_alpha_test()
{
	bf_gfx_device *d = make_device(64, 36, 1);
	bf_gfx_state st;
	flat_state(&st);
	st.alpha_test_enable = 1;
	st.alpha_func = BF_GFX_CMP_GREATER_EQUAL;
	st.stage[0].min_filter = st.stage[0].mag_filter = BF_GFX_FILTER_NEAREST;
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xff000000u, 1, nullptr);
	bf_gfx_texture *t = bf_gfx_texture_create(d, 16, 1, BF_GFX_FORMAT_A8R8G8B8, 1);
	bf_gfx_texture *t4 = bf_gfx_texture_create(d, 16, 1, BF_GFX_FORMAT_A4R4G4B4, 1);
	uint32_t tx[16];
	uint16_t tx4[16];
	for (int i = 0; i < 16; ++i) { tx[i] = uint32_t(i * 17) << 24 | 0xffffffu; tx4[i] = uint16_t(i << 12 | 0x0fff); }
	bf_gfx_texture_update(t, 0, 0, 16, 1, tx, 0);
	bf_gfx_texture_update(t4, 0, 0, 16, 1, tx4, 0);
	const int refs[3] = {0xcf, 0x01, 0xf0};   // opaque lists, alpha lists, Init value
	for (int k = 0; k < 3; ++k) {
		st.alpha_ref = uint8_t(refs[k]);
		bf_gfx_set_state(d, &st);
		bf_gfx_set_texture(d, 0, k == 0 ? t4 : t);
		draw_quad(d, 0, float(k * 12), 64, float(k * 12 + 10), 0xffffffffu, 0, 0, 1, 1);
	}
	std::vector<uint8_t> px = read_native(d);
	for (int k = 0; k < 3; ++k) {
		int bad = 0;
		for (int i = 0; i < 16; ++i) {
			int a = i * 17;
			bool pass = a >= refs[k];
			if (!near_rgb(px_at(px, 64, i * 4 + 2, k * 12 + 5), pass ? 255 : 0, pass ? 255 : 0, pass ? 255 : 0, 0)) ++bad;
		}
		check(bad == 0, "alpha test GREATER_EQUAL 0x%02x on %s: exactly the texels with alpha >= ref drawn", refs[k],
		      k == 0 ? "A4R4G4B4 (nibble C = 204 fails, D = 221 passes)" : "an 8-bit alpha ramp");
	}
	capture(d, "alphatest", "alpha test >= cf (A4 ramp),|>= 01, >= f0 (A8 ramps)");
	bf_gfx_texture_destroy(t);
	bf_gfx_texture_destroy(t4);
	bf_gfx_destroy(d);
}

static void test_depth_cull()
{
	bf_gfx_device *d = make_device(48, 48, 1);
	bf_gfx_state st;
	flat_state(&st);
	st.depth_enable = 1;
	st.depth_write = 1;
	bf_gfx_set_state(d, &st);
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR | BF_GFX_CLEAR_DEPTH, 0xff000000u, 1, nullptr);
	draw_quad(d, 0, 0, 48, 16, 0xffff0000u, 0, 0, 1, 1, 0.5f);          // red at 0.5
	draw_quad(d, 0, 0, 48, 16, 0xff00ff00u, 0, 0, 1, 1, 0.7f);          // green behind: hidden
	draw_quad(d, 24, 0, 48, 16, 0xff0000ffu, 0, 0, 1, 1, 0.5f);         // blue equal: LESS_EQUAL passes
	st.depth_enable = 0;
	bf_gfx_set_state(d, &st);
	draw_quad(d, 0, 4, 8, 12, 0xffffff00u, 0, 0, 1, 1, 0.9f);           // yellow, no depth test
	st.depth_enable = 1;
	st.depth_write = 0;
	bf_gfx_set_state(d, &st);
	draw_quad(d, 12, 4, 20, 12, 0xff00ffffu, 0, 0, 1, 1, 0.2f);         // cyan, test on, no write
	st.depth_write = 1;
	bf_gfx_set_state(d, &st);
	draw_quad(d, 12, 4, 20, 12, 0xffff00ffu, 0, 0, 1, 1, 0.3f);         // magenta passes (depth still 0.5)
	bf_gfx_clear(d, BF_GFX_CLEAR_DEPTH, 0, 1.0f, nullptr);
	draw_quad(d, 40, 4, 44, 12, 0xffffffffu, 0, 0, 1, 1, 0.99f);        // after a depth clear: drawn

	// Cull: one triangle clockwise on screen and one counter-clockwise, for each mode.
	const int modes[3] = {BF_GFX_CULL_NONE, BF_GFX_CULL_CW, BF_GFX_CULL_CCW};
	for (int m = 0; m < 3; ++m) {
		st.cull = uint8_t(modes[m]);
		st.depth_enable = 0;
		bf_gfx_set_state(d, &st);
		float x = float(m * 16), y = 20;
		TL1 cw[3] = {{x + 1, y, 0, 1, 0xffffffffu, 0, 0, 0}, {x + 7, y + 6, 0, 1, 0xffffffffu, 0, 0, 0}, {x + 1, y + 6, 0, 1, 0xffffffffu, 0, 0, 0}};
		TL1 ccw[3] = {{x + 8, y, 0, 1, 0xffffa000u, 0, 0, 0}, {x + 8, y + 6, 0, 1, 0xffffa000u, 0, 0, 0}, {x + 14, y + 6, 0, 1, 0xffffa000u, 0, 0, 0}};
		bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_LIST, BF_GFX_VERTEX_TL1, cw, 3);
		bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_LIST, BF_GFX_VERTEX_TL1, ccw, 3);
	}
	// Triangle fan and indexed list: a square each.
	st.cull = BF_GFX_CULL_NONE;
	bf_gfx_set_state(d, &st);
	TL1 fan[4] = {{2, 32, 0, 1, 0xff80ff80u, 0, 0, 0}, {14, 32, 0, 1, 0xff80ff80u, 0, 0, 0}, {14, 44, 0, 1, 0xff80ff80u, 0, 0, 0}, {2, 44, 0, 1, 0xff80ff80u, 0, 0, 0}};
	bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_FAN, BF_GFX_VERTEX_TL1, fan, 4);
	TL1 sq[4] = {{18, 32, 0, 1, 0xff8080ffu, 0, 0, 0}, {30, 32, 0, 1, 0xff8080ffu, 0, 0, 0}, {30, 44, 0, 1, 0xff8080ffu, 0, 0, 0}, {18, 44, 0, 1, 0xff8080ffu, 0, 0, 0}};
	const uint16_t idx[6] = {0, 1, 2, 0, 2, 3};
	bf_gfx_draw_indexed(d, BF_GFX_PRIM_TRIANGLE_LIST, BF_GFX_VERTEX_TL1, sq, 4, idx, 6);

	std::vector<uint8_t> px = read_native(d);
	auto at = [&](int x, int y) { return px_at(px, 48, x, y); };
	check(near_rgb(at(10, 2), 255, 0, 0, 0), "depth LESS_EQUAL: a farther quad is hidden");
	check(near_rgb(at(30, 2), 0, 0, 255, 0), "depth LESS_EQUAL: an equal depth passes");
	check(near_rgb(at(4, 8), 255, 255, 0, 0), "depth disabled: drawn regardless of depth");
	check(near_rgb(at(16, 8), 255, 0, 255, 0), "depth write off: the nearer quad left depth unchanged");
	check(near_rgb(at(42, 8), 255, 255, 255, 0), "depth-only clear: depth reset, colour kept (%d %d %d at 36,8)", at(36, 8)[2], at(36, 8)[1], at(36, 8)[0]);
	check(near_rgb(at(36, 8), 0, 0, 255, 0), "depth-only clear keeps the colour");
	bool none_cw = near_rgb(at(3, 24), 255, 255, 255, 0), none_ccw = near_rgb(at(10, 24), 255, 160, 0, 0);
	bool cw_cw = near_rgb(at(16 + 3, 24), 255, 255, 255, 0), cw_ccw = near_rgb(at(16 + 10, 24), 255, 160, 0, 0);
	bool ccw_cw = near_rgb(at(32 + 3, 24), 255, 255, 255, 0), ccw_ccw = near_rgb(at(32 + 10, 24), 255, 160, 0, 0);
	check(none_cw && none_ccw, "cull NONE: both windings drawn");
	check(!cw_cw && cw_ccw, "cull CW: the clockwise-on-screen triangle is dropped, the other drawn");
	check(ccw_cw && !ccw_ccw, "cull CCW: the counter-clockwise-on-screen triangle is dropped, the other drawn");
	int fan_px = 0, idx_px = 0;
	for (int y = 30; y < 46; ++y)
		for (int x = 0; x < 34; ++x) {
			if (near_rgb(at(x, y), 128, 255, 128, 0)) ++fan_px;
			if (near_rgb(at(x, y), 128, 128, 255, 0)) ++idx_px;
		}
	check(fan_px == 144 && idx_px == 144, "triangle fan and indexed list: 12x12 squares (%d, %d pixels)", fan_px, idx_px);
	capture(d, "depth_cull", "depth; cull NONE, CW, CCW;|fan, indexed list");
	bf_gfx_destroy(d);
}

static float factor(int f, const float s[4], const float dc[3], int ch, bool as_src)
{
	float sa = s[3];
	switch (f) {
	case 1: return 0;
	case 2: return 1;
	case 3: return s[ch];
	case 4: return 1 - s[ch];
	case 5: return sa;
	case 6: return 1 - sa;
	case 7: return 1;
	case 8: return 0;
	case 9: return dc[ch];
	case 10: return 1 - dc[ch];
	case 11: return std::min(sa, 1 - 1.0f);   // min(As, 1 - Ad) with Ad = 1
	case 12: return as_src ? sa : 1 - sa;
	case 13: return as_src ? 1 - sa : sa;
	}
	return 0;
}

static const char *k_blend_name[14] = {"", "ZERO", "ONE", "SRCCOLOR", "INVSRCCOLOR", "SRCALPHA", "INVSRCALPHA",
	"DESTALPHA", "INVDESTALPHA", "DESTCOLOR", "INVDESTCOLOR", "SRCALPHASAT", "BOTHSRCALPHA", "BOTHINVSRCALPHA"};

static void test_blends()
{
	// Pairs (src, dst) the client uses: Renderer.cpp's fixed lists, the
	// effect code, and every pair found in the game's .str files.
	const int pairs[][2] = {{5, 6}, {2, 2}, {2, 5}, {1, 3}, {1, 6}, {2, 1}, {2, 6}, {2, 7},
	                        {3, 6}, {3, 9}, {5, 2}, {5, 5}, {5, 7}, {5, 8}, {5, 9}, {6, 6}};
	const int np = int(sizeof pairs / sizeof pairs[0]);
	const uint32_t dst = 0xff4080c0u, src = 0x99cc6633u;   // src alpha 0x99 = 0.6
	bf_gfx_device *d = make_device(np * 8, 40, 2);
	bf_gfx_state st;
	flat_state(&st);
	st.blend_enable = 1;
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, dst, 1, nullptr);
	for (int i = 0; i < np; ++i) {
		st.src_blend = uint8_t(pairs[i][0]);
		st.dst_blend = uint8_t(pairs[i][1]);
		bf_gfx_set_state(d, &st);
		draw_quad(d, float(i * 8 + 1), 2, float(i * 8 + 7), 18, src);   // NULL texture: white x diffuse
	}
	// Every factor once as source (dst ZERO) and once as destination (src ZERO)
	// in the lower half; 13 + 13 columns of 3 pixels.
	for (int f = 1; f <= 13; ++f) {
		st.src_blend = uint8_t(f); st.dst_blend = BF_GFX_BLEND_ZERO;
		bf_gfx_set_state(d, &st);
		draw_quad(d, float(f * 4), 22, float(f * 4 + 3), 30, src);
		st.src_blend = BF_GFX_BLEND_ZERO; st.dst_blend = uint8_t(f);
		bf_gfx_set_state(d, &st);
		draw_quad(d, float(f * 4), 31, float(f * 4 + 3), 39, src);
	}
	std::vector<uint8_t> px = read_native(d);
	int nw = np * 8 * 2;
	float s[4] = {0xcc / 255.f, 0x66 / 255.f, 0x33 / 255.f, 0x99 / 255.f}, dc[3] = {0x40 / 255.f, 0x80 / 255.f, 0xc0 / 255.f};
	auto expect = [&](int fs, int fd, int ch) {
		if (fs == 12 || fs == 13) fd = fs;
		return q8(s[ch] * factor(fs, s, dc, ch, true) + dc[ch] * factor(fd, s, dc, ch, false));
	};
	for (int i = 0; i < np; ++i) {
		int fs = pairs[i][0], fd = pairs[i][1];
		const uint8_t *p = px_at(px, nw, (i * 8 + 4) * 2, 20);
		int r = expect(fs, fd, 0), g = expect(fs, fd, 1), b = expect(fs, fd, 2);
		check(near_rgb(p, r, g, b, 1), "blend %s / %s: %d %d %d, expected %d %d %d", k_blend_name[fs], k_blend_name[fd],
		      p[2], p[1], p[0], r, g, b);
	}
	int bad = 0;
	for (int f = 1; f <= 13; ++f) {
		const uint8_t *p = px_at(px, nw, (f * 4 + 1) * 2, 26 * 2);
		const uint8_t *q = px_at(px, nw, (f * 4 + 1) * 2, 35 * 2);
		if (!near_rgb(p, expect(f, 1, 0), expect(f, 1, 1), expect(f, 1, 2), 1)) { ++bad; printf("     src factor %s wrong\n", k_blend_name[f]); }
		if (f != 12 && f != 13 && !near_rgb(q, expect(1, f, 0), expect(1, f, 1), expect(1, f, 2), 1)) { ++bad; printf("     dst factor %s wrong\n", k_blend_name[f]); }
	}
	check(bad == 0, "all 13 blend factors as source and as destination match the formula");
	capture(d, "blends", "16 blend pairs in use; below|every factor as src, dst");
	bf_gfx_destroy(d);
}

static void make_preset_textures(bf_gfx_device *d, bf_gfx_texture **t0, bf_gfx_texture **t1, std::vector<uint32_t> &a, std::vector<uint32_t> &b)
{
	a.resize(64 * 64);
	b.resize(64 * 64);
	for (int y = 0; y < 64; ++y)
		for (int x = 0; x < 64; ++x) {
			a[size_t(y * 64 + x)] = uint32_t(255 - x * 3) << 24 | uint32_t(x * 4) << 16 | uint32_t(y * 4) << 8 | 0x80u;
			b[size_t(y * 64 + x)] = uint32_t(y * 4) << 24 | uint32_t(255 - y * 4) << 16 | 0x60u << 8 | uint32_t(x * 4);
		}
	*t0 = bf_gfx_texture_create(d, 64, 64, BF_GFX_FORMAT_A8R8G8B8, 1);
	*t1 = bf_gfx_texture_create(d, 64, 64, BF_GFX_FORMAT_A8R8G8B8, 1);
	bf_gfx_texture_update(*t0, 0, 0, 64, 64, a.data(), 0);
	bf_gfx_texture_update(*t1, 0, 0, 64, 64, b.data(), 0);
}

static void draw_tl2_quad(bf_gfx_device *d, float x0, float y0, uint32_t diffuse)
{
	float u0 = 0.5f / 64, u1 = 64.5f / 64;
	TL2 v[4] = {{x0, y0, 0, 1, diffuse, 0xffffffffu, u0, u0, u0, u0},
	            {x0 + 64, y0, 0, 1, diffuse, 0xffffffffu, u1, u0, u1, u0},
	            {x0, y0 + 64, 0, 1, diffuse, 0xffffffffu, u0, u1, u0, u1},
	            {x0 + 64, y0 + 64, 0, 1, diffuse, 0xffffffffu, u1, u1, u1, u1}};
	bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL2, v, 4);
}

static void test_presets()
{
	const uint32_t diffuse = 0xb4c89664u;   // a 0.706, r 0.784, g 0.588, b 0.392
	for (int n = 0; n <= 16; ++n) {        // 16 = the ground's lightmap pass (FlushLmList)
		bf_gfx_device *d = make_device(128, 64, 1);
		bf_gfx_texture *t0, *t1;
		std::vector<uint32_t> a, b;
		make_preset_textures(d, &t0, &t1, a, b);
		bf_gfx_state st;
		flat_state(&st);
		// FlushRenderList leaves stage 0 MODULATE / MODULATE and stage 1 off.
		st.stage[0].color_op = BF_GFX_OP_MODULATE;
		st.stage[0].alpha_op = BF_GFX_OP_MODULATE;
		st.stage[1].color_op = BF_GFX_OP_DISABLE;
		st.stage[1].alpha_op = BF_GFX_OP_DISABLE;
		if (n < 16) apply_preset(&st, n);
		else st.stage[1].color_op = BF_GFX_OP_MODULATE_ALPHA_ADD_COLOR;
		for (int s = 0; s < 2; ++s) {
			st.stage[s].min_filter = st.stage[s].mag_filter = BF_GFX_FILTER_NEAREST;
			st.stage[s].address_u = st.stage[s].address_v = BF_GFX_ADDRESS_CLAMP;
		}
		bf_gfx_set_texture(d, 0, t0);
		bf_gfx_set_texture(d, 1, t1);
		// Left: colour, written without blending. Right: alpha, as ZERO / SRC_ALPHA over white.
		bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xff000000u, 1, nullptr);
		bf_gfx_rect right{64, 0, 64, 64};
		bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xffffffffu, 1, &right);
		bf_gfx_set_state(d, &st);
		draw_tl2_quad(d, 0, 0, diffuse);
		st.blend_enable = 1;
		st.src_blend = BF_GFX_BLEND_ZERO;
		st.dst_blend = BF_GFX_BLEND_SRC_ALPHA;
		bf_gfx_set_state(d, &st);
		draw_tl2_quad(d, 64, 0, diffuse);
		std::vector<uint8_t> px = read_native(d);
		int bad_c = 0, bad_a = 0, worst = 0;
		st.blend_enable = 0;
		for (int y = 0; y < 64; ++y)
			for (int x = 0; x < 64; ++x) {
				C4 e = ref_combine(st, c4(diffuse), c4(a[size_t(y * 64 + x)]), c4(b[size_t(y * 64 + x)]));
				const uint8_t *p = px_at(px, 128, x, y), *q = px_at(px, 128, 64 + x, y);
				int dr = std::abs(p[2] - q8(e.r)), dg = std::abs(p[1] - q8(e.g)), db = std::abs(p[0] - q8(e.b));
				int da = std::abs(q[2] - q8(e.a));
				worst = std::max(worst, std::max(std::max(dr, dg), std::max(db, da)));
				if (dr > 1 || dg > 1 || db > 1) ++bad_c;
				if (da > 1) ++bad_a;
			}
		std::string name = n < 16 ? "preset " + std::to_string(n) : std::string("ground lightmap pass");
		check(bad_c == 0 && bad_a == 0, "%s (%s): colour and alpha of 4096 pixels match the reference (worst %d/255; %d, %d off)",
		      name.c_str(), n < 16 ? k_preset_text[n] : "T*D, then LM.rgb + LM.a*cur", worst, bad_c, bad_a);
		capture(d, n < 16 ? "preset_" + std::to_string(n) : "lightmap",
		        (n < 16 ? "preset " + std::to_string(n) + ": " + k_preset_text[n] : std::string("lightmap: LM+LMa*T*D; a T*D")) + "|left colour, right alpha");
		bf_gfx_texture_destroy(t0);
		bf_gfx_texture_destroy(t1);
		bf_gfx_destroy(d);
	}
}

static void test_fog_null_texture()
{
	bf_gfx_device *d = make_device(64, 32, 1);
	bf_gfx_state st;
	flat_state(&st);
	st.fog_enable = 1;
	st.fog_color = 0xff0000ffu;
	bf_gfx_set_state(d, &st);
	bf_gfx_set_texture(d, 0, nullptr);
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xff000000u, 1, nullptr);
	// Specular alpha is the fog factor (GetSpecularFromOow): 1 left, 0 right.
	TL1 v[4] = {{0, 0, 0, 1, 0xffffffffu, 0xff000000u, 0, 0}, {64, 0, 0, 1, 0xffffffffu, 0x00000000u, 0, 0},
	            {0, 16, 0, 1, 0xffffffffu, 0xff000000u, 0, 0}, {64, 16, 0, 1, 0xffffffffu, 0x00000000u, 0, 0}};
	bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4);
	// DrawBoxScreen: no texture, colour 0x80000000 over white: half black.
	st.fog_enable = 0;
	st.blend_enable = 1;
	bf_gfx_set_state(d, &st);
	bf_gfx_rect lower{0, 16, 64, 16};
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xffffffffu, 1, &lower);
	draw_quad(d, 0, 16, 64, 32, 0x80000000u);
	std::vector<uint8_t> px = read_native(d);
	int bad = 0;
	for (int x = 0; x < 64; x += 7) {
		float f = 1.0f - x / 64.0f;   // the factor where Direct3D evaluates pixel x
		int c = q8(f);
		const uint8_t *p = px_at(px, 64, x, 8);
		if (!near_rgb(p, c, c, 255, 2)) { ++bad; printf("     fog x %d: got %d %d %d, expected %d %d 255\n", x, p[2], p[1], p[0], c, c); }
	}
	check(bad == 0, "fog: colour = lerp(fog colour, colour, specular alpha) across a ramp (+-2)");
	check(near_rgb(px_at(px, 64, 30, 24), 127, 127, 127, 1), "unbound texture reads opaque white: DrawBoxScreen 0x80000000 halves white (%d)", px_at(px, 64, 30, 24)[0]);
	capture(d, "fog_box", "fog ramp to blue; NULL tex|box 0x80000000 over white");
	bf_gfx_destroy(d);
}

static void test_address_mips()
{
	bf_gfx_device *d = make_device(96, 32, 1);
	bf_gfx_texture *t = bf_gfx_texture_create(d, 4, 4, BF_GFX_FORMAT_A8R8G8B8, 1);
	uint32_t tx[16];
	for (int i = 0; i < 16; ++i) tx[i] = 0xff000000u | uint32_t(i % 4 * 80) << 16 | uint32_t(i / 4 * 80) << 8 | 0x40;
	bf_gfx_texture_update(t, 0, 0, 4, 4, tx, 0);
	bf_gfx_state st;
	flat_state(&st);
	st.stage[0].min_filter = st.stage[0].mag_filter = BF_GFX_FILTER_NEAREST;
	const int modes[4] = {BF_GFX_ADDRESS_WRAP, BF_GFX_ADDRESS_MIRROR, BF_GFX_ADDRESS_CLAMP, BF_GFX_ADDRESS_BORDER};
	bf_gfx_set_texture(d, 0, t);
	for (int m = 0; m < 4; ++m) {
		st.stage[0].address_u = st.stage[0].address_v = uint8_t(modes[m]);
		st.stage[0].border_color = 0xff000000u;
		bf_gfx_set_state(d, &st);
		// 12 texels across a 4-texel texture: u from -1 to 2.
		draw_quad(d, float(m * 24), 0, float(m * 24 + 12), 12, 0xffffffffu, -1 + 0.5f / 4, -1 + 0.5f / 4, 2 + 0.5f / 4, 2 + 0.5f / 4);
	}
	// Mipmaps: a 64x64 1-pixel checkerboard drawn at 8x8 with trilinear filtering is grey.
	bf_gfx_texture *m = bf_gfx_texture_create(d, 64, 64, BF_GFX_FORMAT_A1R5G5B5, 0);
	std::vector<uint16_t> ck(64 * 64);
	for (int i = 0; i < 64 * 64; ++i) ck[size_t(i)] = ((i % 64 + i / 64) & 1) ? 0xffff : 0x8000;
	bf_gfx_texture_update(m, 0, 0, 64, 64, ck.data(), 0);
	st.stage[0].address_u = st.stage[0].address_v = BF_GFX_ADDRESS_CLAMP;
	st.stage[0].min_filter = st.stage[0].mag_filter = BF_GFX_FILTER_LINEAR;
	st.stage[0].mip_filter = BF_GFX_MIP_LINEAR;
	bf_gfx_set_state(d, &st);
	bf_gfx_set_texture(d, 0, m);
	draw_quad(d, 0, 16, 8, 24, 0xffffffffu, 0, 0, 1, 1);
	st.stage[0].mip_filter = BF_GFX_MIP_NONE;
	bf_gfx_set_state(d, &st);
	draw_quad(d, 16, 16, 24, 24, 0xffffffffu, 0, 0, 1, 1);
	std::vector<uint8_t> px = read_native(d);
	auto texel = [&](int i) { uint32_t c = tx[i]; return c; };
	// wrap: logical pixel k samples texel (k - 4) mod 4; mirror; clamp; border black.
	int bad = 0;
	for (int m2 = 0; m2 < 4; ++m2)
		for (int k = 0; k < 12; ++k) {
			int u = k - 4, i;
			bool border = false;
			switch (modes[m2]) {
			case BF_GFX_ADDRESS_WRAP: i = ((u % 4) + 4) % 4; break;
			case BF_GFX_ADDRESS_MIRROR: { int p = ((u % 8) + 8) % 8; i = p < 4 ? p : 7 - p; break; }
			case BF_GFX_ADDRESS_CLAMP: i = std::min(std::max(u, 0), 3); break;
			default: i = 0; border = u < 0 || u > 3; break;
			}
			const uint8_t *p = px_at(px, 96, m2 * 24 + k, 5);   // row 5: v texel 1
			uint32_t c = border ? 0xff000000u : texel(1 * 4 + i);
			if (modes[m2] == BF_GFX_ADDRESS_BORDER && !border) c = texel(4 + std::min(std::max(u, 0), 3));
			if (!near_rgb(p, (c >> 16) & 255, (c >> 8) & 255, c & 255, 0)) ++bad;
		}
	check(bad == 0, "address modes wrap, mirror, clamp, border (opaque black) sample the right texels");
	const uint8_t *g = px_at(px, 96, 4, 20);
	check(std::abs(g[0] - 128) <= 10 && std::abs(g[2] - 128) <= 10, "mipmaps: a 1px checkerboard minified 8x with trilinear filtering is grey (%d)", g[0]);
	capture(d, "address_mips", "wrap, mirror, clamp, border;|mips: trilinear vs none");
	bf_gfx_texture_destroy(t);
	bf_gfx_texture_destroy(m);
	bf_gfx_destroy(d);
}

static void test_textures_ordering()
{
	bf_gfx_device *d = make_device(64, 32, 1);
	bf_gfx_state st;
	flat_state(&st);
	st.stage[0].min_filter = st.stage[0].mag_filter = BF_GFX_FILTER_NEAREST;
	bf_gfx_set_state(d, &st);
	bf_gfx_texture *t = bf_gfx_texture_create(d, 8, 8, BF_GFX_FORMAT_A1R5G5B5, 1);
	bf_gfx_texture_fill(t, nullptr, 0xfc00);                       // red
	bf_gfx_set_texture(d, 0, t);
	draw_quad(d, 0, 0, 8, 8, 0xffffffffu, 0.5f / 8, 0.5f / 8, 8.5f / 8, 8.5f / 8);
	bf_gfx_texture_fill(t, nullptr, 0x83e0);                       // green, after the first draw
	draw_quad(d, 10, 0, 18, 8, 0xffffffffu, 0.5f / 8, 0.5f / 8, 8.5f / 8, 8.5f / 8);
	// Lock: write a blue column, read back what is there.
	int pitch = 0;
	uint16_t *p = (uint16_t *)bf_gfx_texture_lock(t, &pitch);
	bool lock_ok = p && pitch == 16 && p[0] == 0x83e0;
	for (int y = 0; y < 8; ++y) p[y * pitch / 2 + 3] = 0x801f;
	bf_gfx_rect col{3, 0, 1, 8};
	bf_gfx_texture_unlock(t, &col);
	draw_quad(d, 20, 0, 28, 8, 0xffffffffu, 0.5f / 8, 0.5f / 8, 8.5f / 8, 8.5f / 8);
	// Copy with stretch (CTexture::CopyTexture): 4x4 of a 32-bit texture onto 8x8, converted.
	bf_gfx_texture *s = bf_gfx_texture_create(d, 4, 4, BF_GFX_FORMAT_A8R8G8B8, 1);
	uint32_t sx[16];
	for (int i = 0; i < 16; ++i) sx[i] = (i & 1) ? 0xffffffffu : 0xff000000u;
	bf_gfx_texture_update(s, 0, 0, 4, 4, sx, 0);
	bf_gfx_rect sr{0, 0, 4, 4}, dr{0, 0, 8, 8};
	bf_gfx_texture_copy(t, &dr, s, &sr);
	draw_quad(d, 30, 0, 38, 8, 0xffffffffu, 0.5f / 8, 0.5f / 8, 8.5f / 8, 8.5f / 8);
	// Many quads in one draw: vertices beyond the inline limit.
	bf_gfx_set_texture(d, 0, nullptr);
	std::vector<TL1> many;
	for (int i = 0; i < 200; ++i) {
		float x = float(i % 50), y = float(12 + i / 50 * 2);
		TL1 q[4];
		quad(q, x, y, x + 1, y + 1, 0xffffffffu, 0.5f / 8, 0.5f / 8, 0.5f / 8, 0.5f / 8);
		many.push_back(q[0]); many.push_back(q[1]); many.push_back(q[2]);
		many.push_back(q[1]); many.push_back(q[3]); many.push_back(q[2]);
	}
	bf_gfx_draw(d, BF_GFX_PRIM_TRIANGLE_LIST, BF_GFX_VERTEX_TL1, many.data(), int(many.size()));
	std::vector<uint8_t> px = read_native(d);
	check(near_rgb(px_at(px, 64, 4, 4), 255, 0, 0, 0), "texture update between draws: the earlier draw keeps the old contents");
	check(near_rgb(px_at(px, 64, 14, 4), 0, 255, 0, 0), "texture update between draws: the later draw sees the new contents");
	check(lock_ok && near_rgb(px_at(px, 64, 23, 4), 0, 0, 255, 0) && near_rgb(px_at(px, 64, 22, 4), 0, 255, 0, 0),
	      "lock reads the current texels, unlock uploads the changed column");
	check(near_rgb(px_at(px, 64, 30, 4), 0, 0, 0, 0) && near_rgb(px_at(px, 64, 32, 4), 255, 255, 255, 0) && near_rgb(px_at(px, 64, 31, 4), 0, 0, 0, 0),
	      "texture copy: 4x4 stretched to 8x8 with nearest sampling, 32 to 16 bit");
	int lit = 0;
	for (int y = 12; y < 20; ++y) for (int x = 0; x < 50; ++x) if (px_at(px, 64, x, y)[1] > 0) ++lit;
	check(lit == 200, "1200-vertex draw (38 KB, upload buffer path): %d of 200 one-pixel quads drawn", lit);
	capture(d, "textures", "update order, lock, copy;|1200-vertex draw");
	bf_gfx_texture_destroy(t);
	bf_gfx_texture_destroy(s);
	bf_gfx_destroy(d);
}

static void test_readback_writes()
{
	bf_gfx_device *d = make_device(32, 16, 3);
	bf_gfx_state st;
	flat_state(&st);
	bf_gfx_set_state(d, &st);
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xff102030u, 1, nullptr);
	draw_quad(d, 2, 2, 6, 6, 0xffc08040u);
	uint8_t one[4 * 4 * 4];
	bf_gfx_rect r{2, 2, 4, 4};
	bf_gfx_read_pixels(d, BF_GFX_SPACE_LOGICAL, &r, one, 0);
	check(near_rgb(one, 0xc0, 0x80, 0x40, 0) && one[3] == 255, "read_pixels logical: a block reads as its colour, alpha 255");
	// write_pixels: a 4x4 logical image with a transparent hole.
	uint8_t img[4 * 4 * 4];
	for (int i = 0; i < 16; ++i) { img[i * 4] = 0xff; img[i * 4 + 1] = 0; img[i * 4 + 2] = 0; img[i * 4 + 3] = (i == 5) ? 0 : 255; }
	bf_gfx_rect w{10, 2, 4, 4};
	bf_gfx_write_pixels(d, BF_GFX_SPACE_LOGICAL, &w, img, 0, BF_GFX_WRITE_SKIP_TRANSPARENT);
	draw_quad(d, 12, 4, 16, 8, 0xff00ff00u);   // after the write: on top of it
	std::vector<uint8_t> px = read_native(d);
	int n = 32 * 3, bad = 0;
	for (int y = 2 * 3; y < 6 * 3; ++y)
		for (int x = 10 * 3; x < 14 * 3; ++x) {
			int lx = x / 3 - 10, ly = y / 3 - 2;
			const uint8_t *p = px_at(px, n, x, y);
			bool green = lx >= 2 && ly >= 2;
			bool hole = lx == 1 && ly == 1;
			if (green ? !near_rgb(p, 0, 255, 0, 0) : hole ? !near_rgb(p, 0x10, 0x20, 0x30, 0) : !near_rgb(p, 0, 0, 255, 0)) ++bad;
		}
	check(bad == 0, "write_pixels logical: blocks of 3x3, transparent pixel skipped, ordered before a later draw");
	bf_gfx_backup_frame(d);
	bf_gfx_clear(d, BF_GFX_CLEAR_COLOR, 0xffff00ffu, 1, nullptr);
	bf_gfx_restore_frame(d);
	std::vector<uint8_t> back = read_native(d);
	check(back == px, "backup_frame / restore_frame give back the same pixels");
	long long draws, pipes, reads, frames;
	bf_gfx_get_stats(d, &draws, &pipes, &reads, &frames);
	check(reads == 3, "nothing is read back unless asked: %lld read-backs for 3 requests", reads);
	capture(d, "readback", "read, write pixels, scale 3;|backup and restore");
	bf_gfx_destroy(d);
}

static void test_layer()
{
	CAMetalLayer *layer = [CAMetalLayer layer];
	layer.frame = CGRectMake(0, 0, 64, 32);
	bf_gfx_desc desc = {(__bridge void *)layer, 32, 16, 2};
	bf_gfx_device *d = bf_gfx_create(&desc);
	check(d != nullptr, "device on a CAMetalLayer without a window");
	if (!d) return;
	check(layer.drawableSize.width == 64 && layer.drawableSize.height == 32, "layer drawable size follows the native size (64x32)");
	bf_gfx_state st;
	flat_state(&st);
	bf_gfx_set_state(d, &st);
	draw_quad(d, 0, 0, 16, 16, 0xff00ff00u);
	int rc = bf_gfx_present(d);
	if (rc == BF_GFX_E_TIMEOUT) printf("skip present: the layer vends no drawable without a window\n");
	else check(rc == BF_GFX_OK, "present on the layer (%d)", rc);
	bf_gfx_resize(d, 40, 20, 1);
	check(layer.drawableSize.width == 40 && layer.drawableSize.height == 20, "resize: drawable size follows (40x20)");
	bf_gfx_destroy(d);
}

int main(int argc, char **argv)
{
	@autoreleasepool {
		g_out = argc > 1 ? argv[1] : BF_GFX_TEST_OUT;
		[[NSFileManager defaultManager] createDirectoryAtPath:@(g_out.c_str()) withIntermediateDirectories:YES attributes:nil error:nil];
		{
			bf_gfx_device *d = make_device(4, 4, 1);
			printf("info device: %s, captures in %s\n", bf_gfx_device_name(d), g_out.c_str());
			bf_gfx_destroy(d);
		}
		test_half_pixel();
		test_checker();
		test_formats();
		test_alpha_test();
		test_depth_cull();
		test_blends();
		test_presets();
		test_fog_null_texture();
		test_address_mips();
		test_textures_ordering();
		test_readback_writes();
		test_layer();
		std::string sheet = g_out + "/contact-sheet.png";
		contact_sheet(sheet);
		printf("info contact sheet: %s (%zu captures)\n", sheet.c_str(), g_caps.size());
		printf("%s: %d checks, %d failed\n", g_failures ? "FAIL" : "PASS", g_checks, g_failures);
	}
	return g_failures ? 1 : 0;
}
