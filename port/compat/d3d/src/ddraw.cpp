// DirectDraw 7: DirectDrawCreateEx, IDirectDraw7, IDirectDrawSurface7 and
// IDirectDrawClipper, on port/native/gfx. What each call is for in the client
// is in port/native/gfx/D3D-USAGE.md; the README of this directory lists
// every method and whether it is implemented.
#include "d3d_internal.h"

#include <algorithm>
#include <string.h>

namespace bfd3d {

Global &g()
{
	static Global G;
	return G;
}

static const GUID k_iid_unknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};

void framebuffer_format(DDPIXELFORMAT *pf)
{
	memset(pf, 0, sizeof *pf);
	pf->dwSize = sizeof(DDPIXELFORMAT);
	pf->dwFlags = DDPF_RGB;
	pf->dwRGBBitCount = 32;
	pf->dwRBitMask = 0x00FF0000;
	pf->dwGBitMask = 0x0000FF00;
	pf->dwBBitMask = 0x000000FF;
	pf->dwRGBAlphaBitMask = 0;
}

void target_size(int *w, int *h)
{
	*w = *h = 0;
	if (g().gfx) bf_gfx_get_size(g().gfx, w, h, nullptr, nullptr, nullptr);
}

// ---- pixels ----------------------------------------------------------------------

static int mask_shift(uint32_t m) { int s = 0; if (!m) return 0; while (!(m & 1)) { m >>= 1; ++s; } return s; }
static int mask_bits(uint32_t m) { int n = 0; while (m) { n += int(m & 1); m >>= 1; } return n; }

// v has n significant bits; repeat them to fill 8 bits (v << 3 | v >> 2 for 5).
static uint32_t widen(uint32_t v, int n)
{
	if (n <= 0) return 0;
	if (n >= 8) return (v >> (n - 8)) & 0xFF;
	uint32_t out = 0;
	int filled = 0;
	while (filled < 8) {
		int shift = 8 - filled - n;
		out |= shift >= 0 ? v << shift : v >> -shift;
		filled += n;
	}
	return out & 0xFF;
}

static uint32_t channel_in(uint32_t raw, uint32_t mask) { return widen((raw & mask) >> mask_shift(mask), mask_bits(mask)); }
static uint32_t channel_out(uint32_t c8, uint32_t mask)
{
	int n = mask_bits(mask);
	if (!n) return 0;
	uint32_t v = n >= 8 ? c8 << (n - 8) : c8 >> (8 - n);
	return (v << mask_shift(mask)) & mask;
}

uint32_t to_argb(uint32_t raw, const DDPIXELFORMAT &pf)
{
	uint32_t a = (pf.dwFlags & DDPF_ALPHAPIXELS) && pf.dwRGBAlphaBitMask ? channel_in(raw, pf.dwRGBAlphaBitMask) : 255;
	return a << 24 | channel_in(raw, pf.dwRBitMask) << 16 | channel_in(raw, pf.dwGBitMask) << 8 | channel_in(raw, pf.dwBBitMask);
}

uint32_t from_argb(uint32_t argb, const DDPIXELFORMAT &pf)
{
	uint32_t v = channel_out((argb >> 16) & 0xFF, pf.dwRBitMask) | channel_out((argb >> 8) & 0xFF, pf.dwGBitMask) |
	             channel_out(argb & 0xFF, pf.dwBBitMask);
	if ((pf.dwFlags & DDPF_ALPHAPIXELS) && pf.dwRGBAlphaBitMask) v |= channel_out(argb >> 24, pf.dwRGBAlphaBitMask);
	return v;
}

int format_bytes(const DDPIXELFORMAT &pf) { return int(pf.dwRGBBitCount / 8); }

bool gfx_format_of(const DDPIXELFORMAT &pf, bf_gfx_format *out)
{
	bool alpha = (pf.dwFlags & DDPF_ALPHAPIXELS) != 0;
	if (pf.dwRGBBitCount == 16) {
		if (alpha && pf.dwRGBAlphaBitMask == 0x8000 && pf.dwRBitMask == 0x7C00) { *out = BF_GFX_FORMAT_A1R5G5B5; return true; }
		if (alpha && pf.dwRGBAlphaBitMask == 0xF000 && pf.dwRBitMask == 0x0F00) { *out = BF_GFX_FORMAT_A4R4G4B4; return true; }
		if (!alpha && pf.dwRBitMask == 0xF800 && pf.dwGBitMask == 0x07E0) { *out = BF_GFX_FORMAT_R5G6B5; return true; }
	}
	if (pf.dwRGBBitCount == 32 && pf.dwRBitMask == 0xFF0000 && pf.dwGBitMask == 0xFF00 && pf.dwBBitMask == 0xFF) {
		*out = BF_GFX_FORMAT_A8R8G8B8;   // X8R8G8B8 too: the alpha byte is the client's business
		return true;
	}
	return false;
}

static uint32_t load_raw(const uint8_t *p, int bytes)
{
	if (bytes == 2) { uint16_t v; memcpy(&v, p, 2); return v; }
	if (bytes == 4) { uint32_t v; memcpy(&v, p, 4); return v; }
	if (bytes == 3) return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16;
	return p[0];
}

static void store_raw(uint8_t *p, int bytes, uint32_t v)
{
	if (bytes == 2) { uint16_t w = uint16_t(v); memcpy(p, &w, 2); }
	else if (bytes == 4) memcpy(p, &v, 4);
	else if (bytes == 3) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); p[2] = uint8_t(v >> 16); }
	else p[0] = uint8_t(v);
}

static bool key_match(uint32_t raw, const DDPIXELFORMAT &pf, const DDCOLORKEY &key)
{
	uint32_t m = pf.dwRBitMask | pf.dwGBitMask | pf.dwBBitMask;
	if (pf.dwRGBBitCount < 32) m &= (1u << pf.dwRGBBitCount) - 1;
	uint32_t v = raw & m, lo = key.dwColorSpaceLowValue & m, hi = key.dwColorSpaceHighValue & m;
	if (hi < lo) hi = lo;
	return v >= lo && v <= hi;
}

// ---- surfaces: pixel paths -----------------------------------------------------------

static RECT whole(const Surface *s) { RECT r = {0, 0, LONG(s->width), LONG(s->height)}; return r; }
static int rw(const RECT &r) { return int(r.right - r.left); }
static int rh(const RECT &r) { return int(r.bottom - r.top); }
static bool rect_ok(const Surface *s, const RECT &r)
{
	return r.left >= 0 && r.top >= 0 && r.right > r.left && r.bottom > r.top &&
	       DWORD(r.right) <= s->width && DWORD(r.bottom) <= s->height;
}
static bool is_whole(const Surface *s, const RECT &r) { return r.left == 0 && r.top == 0 && DWORD(r.right) == s->width && DWORD(r.bottom) == s->height; }
static bf_gfx_rect gr(const RECT &r) { bf_gfx_rect o{int(r.left), int(r.top), rw(r), rh(r)}; return o; }

// The pixels of the gfx backup, brought into a CPU surface that holds it.
bool Surface::materialise()
{
	if (!holds_backup) return true;
	Global &G = g();
	holds_backup = false;
	if (G.backup_holder == this) G.backup_holder = nullptr;
	if (!G.gfx) return false;
	int tw, th;
	target_size(&tw, &th);
	if (DWORD(tw) != width || DWORD(th) != height) return false;
	std::vector<uint32_t> px(size_t(tw) * size_t(th));
	bf_gfx_rect all{0, 0, tw, th};
	if (G.target_serial == backup_serial) {
		// Nothing has changed the target since the backup: read it directly.
		if (bf_gfx_read_pixels(G.gfx, BF_GFX_SPACE_LOGICAL, &all, px.data(), 0) != BF_GFX_OK) return false;
	} else {
		// Save the current frame natively, restore the backup, read it, put the frame back.
		int ls, nw, nh;
		bf_gfx_get_size(G.gfx, nullptr, nullptr, &ls, &nw, &nh);
		std::vector<uint32_t> cur(size_t(nw) * size_t(nh));
		bf_gfx_rect nall{0, 0, nw, nh};
		if (bf_gfx_read_pixels(G.gfx, BF_GFX_SPACE_NATIVE, &nall, cur.data(), 0) != BF_GFX_OK) return false;
		bf_gfx_restore_frame(G.gfx);
		bf_gfx_read_pixels(G.gfx, BF_GFX_SPACE_LOGICAL, &all, px.data(), 0);
		bf_gfx_write_pixels(G.gfx, BF_GFX_SPACE_NATIVE, &nall, cur.data(), 0, 0);
	}
	int bytes = format_bytes(pf);
	for (int y = 0; y < th; ++y)
		for (int x = 0; x < tw; ++x)
			store_raw(&cpu[size_t(y) * size_t(pitch) + size_t(x) * size_t(bytes)], bytes, from_argb(px[size_t(y) * size_t(tw) + size_t(x)], pf));
	return true;
}

// Reads a rectangle as 0xAARRGGBB. keyed (optional) receives 1 for pixels
// that match the source colour key.
bool Surface::read_argb(const RECT &r, std::vector<uint32_t> &out, std::vector<uint8_t> *keyed)
{
	int w = rw(r), h = rh(r);
	out.assign(size_t(w) * size_t(h), 0);
	if (keyed) keyed->assign(out.size(), 0);
	if (is_target()) {
		if (!g().gfx) return false;
		bf_gfx_rect q = gr(r);
		if (bf_gfx_read_pixels(g().gfx, BF_GFX_SPACE_LOGICAL, &q, out.data(), 0) != BF_GFX_OK) return false;
		if (keyed && has_src_key)
			for (size_t i = 0; i < out.size(); ++i) (*keyed)[i] = key_match(out[i] & 0xFFFFFF, pf, src_key);
		return true;
	}
	const uint8_t *base;
	int p, bytes = format_bytes(pf);
	bf_gfx_texture *t = nullptr;
	if (kind == K_PLAIN) {
		if (!materialise()) return false;
		base = cpu.data();
		p = int(pitch);
	} else if (kind == K_TEXTURE && tex) {
		t = tex;
		base = (const uint8_t *)bf_gfx_texture_lock(t, &p);
		if (!base) return false;
	} else {
		return false;
	}
	for (int y = 0; y < h; ++y) {
		const uint8_t *row = base + size_t(r.top + y) * size_t(p) + size_t(r.left) * size_t(bytes);
		for (int x = 0; x < w; ++x) {
			uint32_t raw = load_raw(row + size_t(x) * size_t(bytes), bytes);
			out[size_t(y) * size_t(w) + size_t(x)] = to_argb(raw, pf);
			if (keyed && has_src_key) (*keyed)[size_t(y) * size_t(w) + size_t(x)] = key_match(raw, pf, src_key);
		}
	}
	if (t) {
		bf_gfx_rect none{0, 0, 0, 0};
		bf_gfx_texture_unlock(t, &none);
	}
	return true;
}

// Writes 0xAARRGGBB pixels into a rectangle; skip (optional) leaves pixels
// with a non-zero entry untouched.
bool Surface::write_argb(const RECT &r, const uint32_t *px, const uint8_t *skip)
{
	int w = rw(r), h = rh(r);
	if (is_target()) {
		Global &G = g();
		if (!G.gfx) return false;
		std::vector<uint32_t> tmp(px, px + size_t(w) * size_t(h));
		for (size_t i = 0; i < tmp.size(); ++i) tmp[i] = (skip && skip[i]) ? 0 : (tmp[i] | 0xFF000000u);
		bf_gfx_rect q = gr(r);
		bf_gfx_write_pixels(G.gfx, BF_GFX_SPACE_LOGICAL, &q, tmp.data(), 0, skip ? BF_GFX_WRITE_SKIP_TRANSPARENT : 0);
		target_changed();
		return true;
	}
	uint8_t *base;
	int p, bytes = format_bytes(pf);
	if (kind == K_PLAIN) {
		if (!materialise()) return false;
		base = cpu.data();
		p = int(pitch);
	} else if (kind == K_TEXTURE && tex) {
		base = (uint8_t *)bf_gfx_texture_lock(tex, &p);
		if (!base) return false;
	} else {
		return false;
	}
	for (int y = 0; y < h; ++y) {
		uint8_t *row = base + size_t(r.top + y) * size_t(p) + size_t(r.left) * size_t(bytes);
		for (int x = 0; x < w; ++x) {
			size_t i = size_t(y) * size_t(w) + size_t(x);
			if (skip && skip[i]) continue;
			store_raw(row + size_t(x) * size_t(bytes), bytes, from_argb(px[i], pf));
		}
	}
	if (kind == K_TEXTURE) {
		bf_gfx_rect q = gr(r);
		bf_gfx_texture_unlock(tex, &q);
	}
	return true;
}

void Surface::fill_desc(DDSURFACEDESC2 *d) const
{
	DWORD size = d->dwSize;
	memset(d, 0, sizeof *d);
	d->dwSize = size;
	d->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	d->dwWidth = width;
	d->dwHeight = height;
	d->ddpfPixelFormat = pf;
	d->ddsCaps = caps;
	if (kind != K_ZBUFFER) {
		d->dwFlags |= DDSD_PITCH;
		d->lPitch = kind == K_PLAIN ? pitch : LONG(width) * LONG(format_bytes(pf));
	}
	if (has_src_key) {
		d->dwFlags |= DDSD_CKSRCBLT;
		d->ddckCKSrcBlt = src_key;
	}
	if (kind == K_TEXTURE && mip_levels > 1) {
		d->dwFlags |= DDSD_MIPMAPCOUNT;
		d->dwMipMapCount = DWORD(mip_levels);
	}
	if (kind == K_PRIMARY && back) {
		d->dwFlags |= DDSD_BACKBUFFERCOUNT;
		d->dwBackBufferCount = 1;
	}
}

// ---- surfaces: lifetime ----------------------------------------------------------------

Surface::~Surface()
{
	Global &G = g();
	if (G.backup_holder == this) G.backup_holder = nullptr;
	if (locked && is_target()) target_unlock();
	if (zbuffer) zbuffer->Release();
	if (clipper) clipper->Release();
	delete back;
	delete mip_next;
	if (tex) {
		G.textures.erase(this);
		if (G.gfx) bf_gfx_texture_destroy(tex);
	}
}

HRESULT Surface::QueryInterface(REFIID riid, void **out)
{
	if (!out) return E_POINTER;
	if (IsEqualGUID(riid, IID_IDirectDrawSurface7) || IsEqualGUID(riid, k_iid_unknown)) {
		*out = static_cast<IDirectDrawSurface7 *>(this);
		AddRef();
		return S_OK;
	}
	*out = nullptr;
	return E_NOINTERFACE;
}

ULONG Surface::AddRef() { return owner ? owner->AddRef() : ++refs; }

ULONG Surface::Release()
{
	if (owner) return owner->Release();
	ULONG n = --refs;
	if (n == 0) delete this;
	return n;
}

// ---- surfaces: Blt ------------------------------------------------------------------------

static HRESULT present()
{
	Global &G = g();
	if (!G.gfx) return DDERR_SURFACELOST;
	int rc = bf_gfx_present(G.gfx);
	if (rc != BF_GFX_OK && rc != BF_GFX_E_STATE) BFD3D_ONCE("d3d: present failed (%d)", rc);
	return DD_OK;
}

// Generic Blt: source pixels as ARGB, nearest stretch, colour key, written
// into the destination. Covers every pair the fast paths below do not.
static HRESULT blt_generic(Surface *dst, const RECT &dr, Surface *src, const RECT &sr, bool keysrc)
{
	std::vector<uint32_t> spx;
	std::vector<uint8_t> skey;
	if (!src->read_argb(sr, spx, keysrc ? &skey : nullptr)) return DDERR_GENERIC;
	int dw = rw(dr), dh = rh(dr), sw = rw(sr), sh = rh(sr);
	std::vector<uint32_t> dpx(size_t(dw) * size_t(dh));
	std::vector<uint8_t> dskip(keysrc ? dpx.size() : 0);
	for (int y = 0; y < dh; ++y) {
		int sy = int((long long)y * sh / dh);
		for (int x = 0; x < dw; ++x) {
			int sx = int((long long)x * sw / dw);
			size_t si = size_t(sy) * size_t(sw) + size_t(sx), di = size_t(y) * size_t(dw) + size_t(x);
			dpx[di] = spx[si];
			if (keysrc) dskip[di] = skey[si];
		}
	}
	return dst->write_argb(dr, dpx.data(), keysrc ? dskip.data() : nullptr) ? DD_OK : DDERR_GENERIC;
}

// A part of the target shown in the whole window: DirectDraw stretches a Blt
// to the primary from the source rectangle to the destination (the client
// area). The window shows the whole target, so the part is drawn stretched
// over the target first, filtered linearly as the hardware blitters of the
// time did. CBink::BinkShowNextFrame does this: the movie at the top left of
// the back buffer, shown in the whole window. The part's pixels come from
// the lock buffer when the client has just written them there (no read-back).
static void stretch_part_over_target(Surface *target, const RECT &sr)
{
	Global &G = g();
	int sw = rw(sr), sh = rh(sr);
	std::vector<uint32_t> spx;
	const void *src;
	int src_pitch;
	int bw, bh;
	LONG bp;
	if (const uint8_t *buf = target_buffer_current(&bw, &bh, &bp)) {
		src = buf + size_t(sr.top) * size_t(bp) + size_t(sr.left) * 4;
		src_pitch = int(bp);
	} else {
		if (!target->read_argb(sr, spx, nullptr)) return;
		src = spx.data();
		src_pitch = sw * 4;
	}
	int tw = 0, th = 0;
	if (G.part_tex) bf_gfx_texture_get_info(G.part_tex, &tw, &th, nullptr, nullptr);
	if (!G.part_tex || tw != sw || th != sh) {
		bf_gfx_texture_destroy(G.part_tex);
		G.part_tex = bf_gfx_texture_create(G.gfx, sw, sh, BF_GFX_FORMAT_A8R8G8B8, 1);
		if (!G.part_tex) return;
	}
	bf_gfx_texture_update(G.part_tex, 0, 0, sw, sh, src, src_pitch);

	bf_gfx_state st;
	bf_gfx_state_init(&st);
	st.stage[0].color_op = BF_GFX_OP_SELECT_ARG1;
	st.stage[0].color_arg1 = BF_GFX_ARG_TEXTURE;
	st.stage[0].alpha_op = BF_GFX_OP_SELECT_ARG1;
	st.stage[0].alpha_arg1 = BF_GFX_ARG_DIFFUSE;
	st.stage[0].address_u = st.stage[0].address_v = BF_GFX_ADDRESS_CLAMP;
	st.stage[0].mag_filter = st.stage[0].min_filter = BF_GFX_FILTER_LINEAR;
	st.stage[1].color_op = st.stage[1].alpha_op = BF_GFX_OP_DISABLE;
	st.depth_enable = 0;
	st.depth_write = 0;
	st.cull = BF_GFX_CULL_NONE;
	bf_gfx_set_state(G.gfx, &st);
	bf_gfx_set_texture(G.gfx, 0, G.part_tex);
	bf_gfx_set_texture(G.gfx, 1, nullptr);
	int w, h;
	target_size(&w, &h);
	struct V { float x, y, z, rhw; uint32_t diffuse, specular; float u, v; };
	// Direct3D 7 pixel centres: the quad's edges at -0.5 cover the target exactly.
	V v[4] = {{-0.5f, -0.5f, 0, 1, 0xFFFFFFFFu, 0, 0, 0},
	          {w - 0.5f, -0.5f, 0, 1, 0xFFFFFFFFu, 0, 1, 0},
	          {-0.5f, h - 0.5f, 0, 1, 0xFFFFFFFFu, 0, 0, 1},
	          {w - 0.5f, h - 0.5f, 0, 1, 0xFFFFFFFFu, 0, 1, 1}};
	bf_gfx_draw(G.gfx, BF_GFX_PRIM_TRIANGLE_STRIP, BF_GFX_VERTEX_TL1, v, 4);
	G.own_draws++;
	target_changed();
	bf_count("d3d.part_presents", 1);
}

HRESULT Surface::Blt(RECT *dst_rect, IDirectDrawSurface7 *src_if, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
	Global &G = g();
	if (lost || (!G.gfx && kind != K_PLAIN)) return DDERR_SURFACELOST;
	if (locked) return DDERR_SURFACEBUSY;
	RECT dr = dst_rect ? *dst_rect : whole(this);

	if (flags & DDBLT_COLORFILL) {
		if (!fx) return DDERR_INVALIDPARAMS;
		if (kind == K_PRIMARY) return DD_OK;     // the window shows the target
		if (!rect_ok(this, dr)) return DDERR_INVALIDRECT;
		DWORD v = fx->dwFillColor;
		if (kind == K_TARGET) {
			bf_gfx_rect q = gr(dr);
			bf_gfx_clear(G.gfx, BF_GFX_CLEAR_COLOR, to_argb(v, pf), 1.0f, &q);
			target_changed();
			return DD_OK;
		}
		if (kind == K_TEXTURE) {
			bf_gfx_rect q = gr(dr);
			return bf_gfx_texture_fill(tex, &q, v) == BF_GFX_OK ? DD_OK : DDERR_GENERIC;
		}
		if (kind == K_PLAIN) {
			if (!is_whole(this, dr)) materialise();
			else if (holds_backup) { holds_backup = false; if (G.backup_holder == this) G.backup_holder = nullptr; }
			int bytes = format_bytes(pf);
			for (LONG y = dr.top; y < dr.bottom; ++y)
				for (LONG x = dr.left; x < dr.right; ++x)
					store_raw(&cpu[size_t(y) * size_t(pitch) + size_t(x) * size_t(bytes)], bytes, v);
			return DD_OK;
		}
		return DD_OK;  // z-buffer, mip level: nothing stored
	}
	if (flags & DDBLT_DEPTHFILL) {
		if (G.gfx) bf_gfx_clear(G.gfx, BF_GFX_CLEAR_DEPTH, 0, 1.0f, nullptr);
		return DD_OK;
	}
	Surface *src = as_surface(src_if);
	if (!src) return DDERR_INVALIDPARAMS;
	RECT sr = src_rect ? *src_rect : whole(src);

	// Presentation: back buffer (or target) into the window.
	if (kind == K_PRIMARY && src->is_target()) {
		if (src_rect && !is_whole(src, sr) && rect_ok(src, sr) && rw(sr) > 0 && rh(sr) > 0)
			stretch_part_over_target(src, sr);
		return present();
	}
	// CloneFrame: the window into the back buffer. The target holds the frame already.
	if (src->kind == K_PRIMARY && kind == K_TARGET) return DD_OK;
	// UpdateMipmap: level 0 into a lower level. The gfx module regenerates mips.
	if (kind == K_MIP) return DD_OK;

	if (!rect_ok(this, dr) || !rect_ok(src, sr)) return DDERR_INVALIDRECT;
	bool keysrc = (flags & DDBLT_KEYSRC) != 0;

	// BackupFrame (and TestScreen's copy): the whole target into a CPU surface
	// of its size becomes the gfx backup, read back only if the CPU needs it.
	if (kind == K_PLAIN && src->kind == K_TARGET && is_whole(this, dr) && is_whole(src, sr) && !keysrc) {
		if (G.backup_holder && G.backup_holder != this) G.backup_holder->materialise();
		if (bf_gfx_backup_frame(G.gfx) != BF_GFX_OK) return DDERR_GENERIC;
		holds_backup = true;
		backup_serial = G.target_serial;
		G.backup_holder = this;
		return DD_OK;
	}
	// RestoreFrame: the backup back into the target, on the GPU.
	if (kind == K_TARGET && src->kind == K_PLAIN && src->holds_backup && is_whole(this, dr) && is_whole(src, sr) && !keysrc) {
		if (bf_gfx_restore_frame(G.gfx) != BF_GFX_OK) return DDERR_GENERIC;
		target_changed();
		src->backup_serial = G.target_serial;
		return DD_OK;
	}
	// CopyTexture: texture to texture, stretched with nearest sampling.
	if (kind == K_TEXTURE && src->kind == K_TEXTURE && !keysrc && tex && src->tex) {
		bf_gfx_rect a = gr(dr), b = gr(sr);
		return bf_gfx_texture_copy(tex, &a, src->tex, &b) == BF_GFX_OK ? DD_OK : DDERR_GENERIC;
	}
	if (src == this && kind != K_PLAIN) return DDERR_UNSUPPORTED;
	return blt_generic(this, dr, src, sr, keysrc);
}

HRESULT Surface::BltFast(DWORD x, DWORD y, IDirectDrawSurface7 *src_if, RECT *src_rect, DWORD flags)
{
	Surface *src = as_surface(src_if);
	if (!src) return DDERR_INVALIDPARAMS;
	if (lost || (!g().gfx && kind != K_PLAIN)) return DDERR_SURFACELOST;
	if (locked) return DDERR_SURFACEBUSY;
	RECT sr = src_rect ? *src_rect : whole(src);
	RECT dr = {LONG(x), LONG(y), LONG(x) + rw(sr), LONG(y) + rh(sr)};
	if (!rect_ok(this, dr) || !rect_ok(src, sr)) return DDERR_INVALIDRECT;
	if (flags & DDBLTFAST_DESTCOLORKEY) BFD3D_ONCE("d3d: BltFast with a destination colour key is not supported");
	return blt_generic(this, dr, src, sr, (flags & DDBLTFAST_SRCCOLORKEY) != 0);
}

HRESULT Surface::Flip(IDirectDrawSurface7 *, DWORD)
{
	if (kind != K_PRIMARY) return DDERR_NOTFLIPPABLE;
	return present();
}

// ---- surfaces: Lock -----------------------------------------------------------------------

HRESULT Surface::Lock(RECT *rect, DDSURFACEDESC2 *desc, DWORD flags, HANDLE)
{
	if (!desc || desc->dwSize != sizeof(DDSURFACEDESC2)) return DDERR_INVALIDPARAMS;
	if (lost) return DDERR_SURFACELOST;
	if (locked || dc) return DDERR_SURFACEBUSY;
	RECT r = rect ? *rect : whole(this);
	if (!rect_ok(this, r)) return DDERR_INVALIDRECT;
	uint8_t *base = nullptr;
	LONG p = 0;
	int bytes = format_bytes(pf);
	if (is_target()) {
		if (target_is_locked()) return DDERR_SURFACEBUSY;
		base = (uint8_t *)target_lock(flags, &p);
		if (!base) return DDERR_SURFACELOST;
	} else if (kind == K_TEXTURE) {
		if (!tex) return DDERR_SURFACELOST;
		int ip;
		base = (uint8_t *)bf_gfx_texture_lock(tex, &ip);
		if (!base) return DDERR_SURFACEBUSY;
		p = ip;
	} else if (kind == K_PLAIN) {
		materialise();
		base = cpu.data();
		p = pitch;
	} else {
		BFD3D_ONCE("d3d: Lock of a %s is not supported", kind == K_ZBUFFER ? "z-buffer" : "mip level");
		return DDERR_CANTLOCKSURFACE;
	}
	locked = true;
	lock_rect = r;
	lock_whole = rect == nullptr;
	lock_flags = flags;
	fill_desc(desc);
	desc->dwFlags |= DDSD_LPSURFACE | DDSD_PITCH;
	desc->lPitch = p;
	desc->lpSurface = base + size_t(r.top) * size_t(p) + size_t(r.left) * size_t(bytes);
	return DD_OK;
}

HRESULT Surface::Unlock(LPRECT)
{
	if (!locked) return DDERR_NOTLOCKED;
	locked = false;
	if (is_target()) {
		target_unlock();
	} else if (kind == K_TEXTURE && tex) {
		bf_gfx_rect none{0, 0, 0, 0}, q = gr(lock_rect);
		bf_gfx_texture_unlock(tex, (lock_flags & DDLOCK_READONLY) ? &none : &q);
	}
	return DD_OK;
}

// ---- surfaces: GetDC ----------------------------------------------------------------------
// The DC is a bf_text_canvas (port/native/text) over a 32-bit staging bitmap,
// handed out as the HDC; the client's CDCSurface draws on it with DrawDC
// (M11.8). The bitmap is pre-filled with opaque pixels (alpha 255); text is
// written as 0x00RRGGBB and touches nothing else, so the pixels with alpha 0
// inside the canvas's drawn rectangle are exactly what was drawn, and only
// they are written into the surface. Nothing is read back, not even for the
// render target (CRenderer::TextOutScreen, GetTextSize).

HRESULT Surface::GetDC(HDC *out)
{
	if (!out) return DDERR_INVALIDPARAMS;
	*out = nullptr;
	if (locked || dc) return DDERR_DCALREADYCREATED;
	if (kind == K_ZBUFFER || kind == K_MIP) return DDERR_CANTCREATEDC;
	if (lost || (!g().gfx && kind != K_PLAIN)) return DDERR_SURFACELOST;
	dc_bits.assign(size_t(width) * size_t(height), 0xFF000000u);
	dc_canvas = bf_text_canvas{};   // a fresh DC: black text, nothing drawn
	dc_canvas.bitmap = bf_text_bitmap{dc_bits.data(), int(width), int(height), int(width) * 4, 0};
	dc = true;
	*out = reinterpret_cast<HDC>(&dc_canvas);
	return DD_OK;
}

HRESULT Surface::ReleaseDC(HDC h)
{
	if (!dc || h != reinterpret_cast<HDC>(&dc_canvas)) return DDERR_NODC;
	RECT d = {dc_canvas.left, dc_canvas.top, dc_canvas.right, dc_canvas.bottom};
	if (dc_canvas.dirty && d.right > d.left && d.bottom > d.top) {
		d.left = (std::max<LONG>)(d.left, 0);
		d.top = (std::max<LONG>)(d.top, 0);
		d.right = (std::min<LONG>)(d.right, LONG(width));
		d.bottom = (std::min<LONG>)(d.bottom, LONG(height));
		int w = rw(d), hh = rh(d);
		if (w > 0 && hh > 0) {
			std::vector<uint32_t> px(size_t(w) * size_t(hh));
			std::vector<uint8_t> skip(px.size());
			for (int y = 0; y < hh; ++y)
				for (int x = 0; x < w; ++x) {
					uint32_t v = dc_bits[size_t(d.top + y) * width + size_t(d.left + x)];
					size_t i = size_t(y) * size_t(w) + size_t(x);
					skip[i] = (v >> 24) != 0;
					px[i] = v | 0xFF000000u;
				}
			write_argb(d, px.data(), skip.data());
		}
	}
	dc = false;
	dc_canvas = bf_text_canvas{};
	dc_bits.clear();
	return DD_OK;
}

// ---- surfaces: attachments, description ----------------------------------------------------

HRESULT Surface::GetAttachedSurface(DDSCAPS2 *want, IDirectDrawSurface7 **out)
{
	if (!want || !out) return DDERR_INVALIDPARAMS;
	*out = nullptr;
	Surface *found = nullptr;
	if ((want->dwCaps & (DDSCAPS_BACKBUFFER | DDSCAPS_FLIP)) && back) {
		found = back;
	} else if ((want->dwCaps & DDSCAPS_MIPMAP) && (kind == K_TEXTURE || kind == K_MIP)) {
		Surface *r = root();
		if (mip_index + 1 < r->mip_levels) {
			if (!mip_next) {
				Surface *m = new Surface;
				m->kind = K_MIP;
				m->owner = r;
				m->width = (std::max<DWORD>)(1, width / 2);
				m->height = (std::max<DWORD>)(1, height / 2);
				m->pf = pf;
				m->caps = caps;
				m->caps.dwCaps &= ~DWORD(DDSCAPS_COMPLEX);
				m->mip_index = mip_index + 1;
				mip_next = m;
			}
			found = mip_next;
		}
	} else if ((want->dwCaps & DDSCAPS_ZBUFFER) && zbuffer) {
		found = zbuffer;
	}
	if (!found) return DDERR_NOTFOUND;
	found->AddRef();
	*out = found;
	return DD_OK;
}

HRESULT Surface::AddAttachedSurface(IDirectDrawSurface7 *a)
{
	Surface *s = as_surface(a);
	if (!s) return DDERR_INVALIDPARAMS;
	if (s->kind != K_ZBUFFER) return DDERR_CANNOTATTACHSURFACE;
	if (zbuffer == s) return DDERR_SURFACEALREADYATTACHED;
	s->AddRef();
	if (zbuffer) zbuffer->Release();
	zbuffer = s;
	return DD_OK;
}

HRESULT Surface::DeleteAttachedSurface(DWORD, IDirectDrawSurface7 *a)
{
	Surface *s = as_surface(a);
	if (s && s == zbuffer) { zbuffer = nullptr; s->Release(); return DD_OK; }
	if (!s && zbuffer) { Surface *z = zbuffer; zbuffer = nullptr; z->Release(); return DD_OK; }
	return DDERR_SURFACENOTATTACHED;
}

HRESULT Surface::GetSurfaceDesc(DDSURFACEDESC2 *desc)
{
	if (!desc || desc->dwSize != sizeof(DDSURFACEDESC2)) return DDERR_INVALIDPARAMS;
	fill_desc(desc);
	return DD_OK;
}

HRESULT Surface::GetPixelFormat(DDPIXELFORMAT *f)
{
	if (!f) return DDERR_INVALIDPARAMS;
	*f = pf;
	return DD_OK;
}

HRESULT Surface::GetCaps(DDSCAPS2 *c)
{
	if (!c) return DDERR_INVALIDPARAMS;
	*c = caps;
	return DD_OK;
}

HRESULT Surface::SetColorKey(DWORD flags, DDCOLORKEY *key)
{
	if (flags & DDCKEY_SRCBLT) {
		has_src_key = key != nullptr;
		if (key) src_key = *key;
		return DD_OK;
	}
	BFD3D_ONCE("d3d: SetColorKey(0x%x) other than a source blit key is ignored", unsigned(flags));
	return DD_OK;
}

HRESULT Surface::GetColorKey(DWORD flags, DDCOLORKEY *key)
{
	if (!key) return DDERR_INVALIDPARAMS;
	if ((flags & DDCKEY_SRCBLT) && has_src_key) { *key = src_key; return DD_OK; }
	return DDERR_NOCOLORKEY;
}

HRESULT Surface::IsLost() { return lost ? DDERR_SURFACELOST : DD_OK; }
HRESULT Surface::Restore() { return lost ? DDERR_WRONGMODE : DD_OK; }

HRESULT Surface::SetClipper(IDirectDrawClipper *c)
{
	if (c) c->AddRef();
	if (clipper) clipper->Release();
	clipper = c;
	return DD_OK;
}

HRESULT Surface::GetClipper(IDirectDrawClipper **c)
{
	if (!c) return DDERR_INVALIDPARAMS;
	*c = clipper;
	if (!clipper) return DDERR_NOCLIPPERATTACHED;
	clipper->AddRef();
	return DD_OK;
}

HRESULT Surface::GetBltStatus(DWORD) { return DD_OK; }
HRESULT Surface::GetFlipStatus(DWORD) { return DD_OK; }
HRESULT Surface::PageLock(DWORD) { return DD_OK; }
HRESULT Surface::PageUnlock(DWORD) { return DD_OK; }
HRESULT Surface::SetPriority(DWORD) { return DD_OK; }
HRESULT Surface::GetPriority(LPDWORD p) { if (p) *p = 0; return DD_OK; }
HRESULT Surface::SetLOD(DWORD) { return DD_OK; }
HRESULT Surface::GetLOD(LPDWORD l) { if (l) *l = 0; return DD_OK; }
HRESULT Surface::GetDDInterface(void **dd)
{
	if (!dd) return DDERR_INVALIDPARAMS;
	*dd = g().ddraw ? static_cast<IDirectDraw7 *>(g().ddraw) : nullptr;
	if (g().ddraw) g().ddraw->AddRef();
	return *dd ? DD_OK : DDERR_GENERIC;
}

// Not used by the client.
HRESULT Surface::AddOverlayDirtyRect(LPRECT) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT Surface::BltBatch(DDBLTBATCH *, DWORD, DWORD) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT Surface::EnumAttachedSurfaces(void *, LPDDENUMSURFACESCALLBACK7) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT Surface::EnumOverlayZOrders(DWORD, void *, LPDDENUMSURFACESCALLBACK7) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT Surface::GetOverlayPosition(LPLONG, LPLONG) { BF_STUB(); return DDERR_NOTAOVERLAYSURFACE; }
HRESULT Surface::GetPalette(IDirectDrawPalette **p) { if (p) *p = nullptr; return DDERR_NOPALETTEATTACHED; }
HRESULT Surface::Initialize(IDirectDraw *, DDSURFACEDESC2 *) { return DDERR_ALREADYINITIALIZED; }
HRESULT Surface::SetOverlayPosition(LONG, LONG) { BF_STUB(); return DDERR_NOTAOVERLAYSURFACE; }
HRESULT Surface::SetPalette(IDirectDrawPalette *) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT Surface::UpdateOverlay(RECT *, IDirectDrawSurface7 *, RECT *, DWORD, DDOVERLAYFX *) { BF_STUB(); return DDERR_NOTAOVERLAYSURFACE; }
HRESULT Surface::UpdateOverlayDisplay(DWORD) { BF_STUB(); return DDERR_NOTAOVERLAYSURFACE; }
HRESULT Surface::UpdateOverlayZOrder(DWORD, IDirectDrawSurface7 *) { BF_STUB(); return DDERR_NOTAOVERLAYSURFACE; }
HRESULT Surface::SetSurfaceDesc(DDSURFACEDESC2 *, DWORD) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT Surface::SetPrivateData(REFGUID, void *, DWORD, DWORD) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT Surface::GetPrivateData(REFGUID, void *, DWORD *) { BF_STUB(); return DDERR_NOTFOUND; }
HRESULT Surface::FreePrivateData(REFGUID) { BF_STUB(); return DDERR_NOTFOUND; }
HRESULT Surface::GetUniquenessValue(LPDWORD v) { if (v) *v = 0; return DD_OK; }
HRESULT Surface::ChangeUniquenessValue() { return DD_OK; }

// ---- clipper ------------------------------------------------------------------------------

struct Clipper final : public IDirectDrawClipper {
	ULONG refs = 1;
	HWND hwnd = nullptr;
	STDMETHOD_(HRESULT, QueryInterface)(REFIID riid, void **out) override
	{
		if (!out) return E_POINTER;
		if (IsEqualGUID(riid, IID_IDirectDrawClipper) || IsEqualGUID(riid, k_iid_unknown)) { *out = this; AddRef(); return S_OK; }
		*out = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() override { return ++refs; }
	STDMETHOD_(ULONG, Release)() override { ULONG n = --refs; if (!n) delete this; return n; }
	STDMETHOD(GetClipList)(LPRECT, LPRGNDATA, LPDWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetHWnd)(HWND *h) override { if (!h) return DDERR_INVALIDPARAMS; *h = hwnd; return DD_OK; }
	STDMETHOD(Initialize)(IDirectDraw *, DWORD) override { return DDERR_ALREADYINITIALIZED; }
	STDMETHOD(IsClipListChanged)(BOOL *c) override { if (c) *c = FALSE; return DD_OK; }
	STDMETHOD(SetClipList)(LPRGNDATA, DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(SetHWnd)(DWORD, HWND h) override { hwnd = h; return DD_OK; }
};

// ---- DirectDraw -----------------------------------------------------------------------------

DDraw::~DDraw()
{
	if (g().ddraw == this) g().ddraw = nullptr;
}

HRESULT DDraw::QueryInterface(REFIID riid, void **out)
{
	if (!out) return E_POINTER;
	if (IsEqualGUID(riid, IID_IDirectDraw7) || IsEqualGUID(riid, k_iid_unknown)) {
		*out = static_cast<IDirectDraw7 *>(this);
		AddRef();
		return S_OK;
	}
	if (IsEqualGUID(riid, IID_IDirect3D7)) {
		*out = query_d3d(this);
		return *out ? S_OK : E_NOINTERFACE;
	}
	*out = nullptr;
	BFD3D_ONCE("d3d: IDirectDraw7::QueryInterface for an interface the adapter does not offer");
	return E_NOINTERFACE;
}

ULONG DDraw::AddRef() { return ++refs; }
ULONG DDraw::Release() { ULONG n = --refs; if (!n) delete this; return n; }

HRESULT DDraw::SetCooperativeLevel(HWND h, DWORD flags)
{
	if (!(flags & (DDSCL_NORMAL | DDSCL_EXCLUSIVE))) return DDERR_INVALIDPARAMS;
	hwnd = h;
	coop = flags;
	if (flags & DDSCL_FULLSCREEN) BF_TRACE(BFT_GFX, "d3d: exclusive full-screen requested; the application module owns full screen");
	return DD_OK;
}

HRESULT DDraw::GetDisplayMode(DDSURFACEDESC2 *d)
{
	if (!d) return DDERR_INVALIDPARAMS;
	int w, h;
	target_size(&w, &h);
	DWORD size = d->dwSize ? d->dwSize : sizeof(DDSURFACEDESC2);
	memset(d, 0, sizeof *d);
	d->dwSize = size;
	d->dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
	d->dwWidth = DWORD(w);
	d->dwHeight = DWORD(h);
	d->lPitch = LONG(w) * 4;
	d->dwRefreshRate = 60;
	framebuffer_format(&d->ddpfPixelFormat);
	return DD_OK;
}

HRESULT DDraw::SetDisplayMode(DWORD w, DWORD h, DWORD bpp, DWORD, DWORD)
{
	BF_TRACE(BFT_GFX, "d3d: SetDisplayMode(%u, %u, %u) accepted; the window keeps its size", unsigned(w), unsigned(h), unsigned(bpp));
	return DD_OK;
}

HRESULT DDraw::EnumDisplayModes(DWORD, DDSURFACEDESC2 *, void *ctx, LPDDENUMMODESCALLBACK2 cb)
{
	if (!cb) return DDERR_INVALIDPARAMS;
	DDSURFACEDESC2 d;
	d.dwSize = sizeof d;
	GetDisplayMode(&d);
	cb(&d, ctx);
	return DD_OK;
}

HRESULT DDraw::CreateSurface(DDSURFACEDESC2 *desc, IDirectDrawSurface7 **out, IUnknown *outer)
{
	if (!out) return DDERR_INVALIDPARAMS;
	*out = nullptr;
	if (outer) return CLASS_E_NOAGGREGATION;
	if (!desc || desc->dwSize != sizeof(DDSURFACEDESC2) || !(desc->dwFlags & DDSD_CAPS)) return DDERR_INVALIDPARAMS;
	Global &G = g();
	if (!G.gfx) return DDERR_NODIRECTDRAWHW;
	DWORD c = desc->ddsCaps.dwCaps;
	int tw, th;
	target_size(&tw, &th);
	Surface *s = new Surface;
	s->caps = desc->ddsCaps;
	if (desc->dwFlags & DDSD_CKSRCBLT) {
		s->has_src_key = true;
		s->src_key = desc->ddckCKSrcBlt;
	}
	bool sized = (desc->dwFlags & (DDSD_WIDTH | DDSD_HEIGHT)) == (DDSD_WIDTH | DDSD_HEIGHT);
	if (sized) {
		s->width = desc->dwWidth;
		s->height = desc->dwHeight;
	}
	if (desc->dwFlags & DDSD_PIXELFORMAT) s->pf = desc->ddpfPixelFormat;
	else framebuffer_format(&s->pf);

	if (c & DDSCAPS_PRIMARYSURFACE) {
		s->kind = (c & DDSCAPS_3DDEVICE) && !(c & DDSCAPS_FLIP) ? K_TARGET : K_PRIMARY;
		s->width = DWORD(tw);
		s->height = DWORD(th);
		framebuffer_format(&s->pf);
		s->caps.dwCaps |= DDSCAPS_FRONTBUFFER | DDSCAPS_VISIBLE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
		if ((c & DDSCAPS_FLIP) && (desc->dwFlags & DDSD_BACKBUFFERCOUNT) && desc->dwBackBufferCount >= 1) {
			Surface *b = new Surface;
			b->kind = K_TARGET;
			b->owner = s;
			b->width = s->width;
			b->height = s->height;
			b->pf = s->pf;
			b->caps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
			s->back = b;
			if (desc->dwBackBufferCount > 1) BF_TRACE(BFT_GFX, "d3d: %u back buffers requested, one provided", unsigned(desc->dwBackBufferCount));
		}
	} else if (c & DDSCAPS_ZBUFFER) {
		s->kind = K_ZBUFFER;
		if (!sized) { s->width = DWORD(tw); s->height = DWORD(th); }
		if (!(desc->dwFlags & DDSD_PIXELFORMAT)) {
			memset(&s->pf, 0, sizeof s->pf);
			s->pf.dwSize = sizeof s->pf;
			s->pf.dwFlags = DDPF_ZBUFFER;
			s->pf.dwZBufferBitDepth = 16;
			s->pf.dwZBitMask = 0xFFFF;
		}
	} else if (c & DDSCAPS_TEXTURE) {
		s->kind = K_TEXTURE;
		bf_gfx_format f;
		if (!sized || !gfx_format_of(s->pf, &f)) {
			bf_tracef(BFT_ERR, "d3d: CreateSurface: texture without size or with an unsupported format (%u bit, A %08x R %08x)",
			          unsigned(s->pf.dwRGBBitCount), unsigned(s->pf.dwRGBAlphaBitMask), unsigned(s->pf.dwRBitMask));
			delete s;
			return sized ? DDERR_INVALIDPIXELFORMAT : DDERR_INVALIDPARAMS;
		}
		int mips = 1;
		if (c & DDSCAPS_MIPMAP) {
			if (desc->dwFlags & DDSD_MIPMAPCOUNT) mips = int(desc->dwMipMapCount);
			else { mips = 1; for (DWORD m = (std::max)(s->width, s->height); m > 1; m >>= 1) ++mips; }
			if (mips < 1) mips = 1;
		}
		s->tex = bf_gfx_texture_create(G.gfx, int(s->width), int(s->height), f, mips);
		if (!s->tex) {
			delete s;
			return DDERR_OUTOFVIDEOMEMORY;
		}
		bf_gfx_texture_get_info(s->tex, nullptr, nullptr, nullptr, &s->mip_levels);
		G.textures.insert(s);
		s->caps.dwCaps |= DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
	} else if (c & DDSCAPS_3DDEVICE) {
		s->kind = K_TARGET;
		if (!sized) { s->width = DWORD(tw); s->height = DWORD(th); }
		framebuffer_format(&s->pf);
		if (s->width != DWORD(tw) || s->height != DWORD(th)) {
			int scale;
			bf_gfx_get_size(G.gfx, nullptr, nullptr, &scale, nullptr, nullptr);
			BF_TRACE(BFT_GFX, "d3d: render target %ux%u asked, gfx is %dx%d: resizing gfx", unsigned(s->width), unsigned(s->height), tw, th);
			target_drop_buffer();
			if (bf_gfx_resize(G.gfx, int(s->width), int(s->height), scale) != BF_GFX_OK) {
				delete s;
				return DDERR_INVALIDPARAMS;
			}
		}
		s->caps.dwCaps |= DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
	} else {
		s->kind = K_PLAIN;
		int bytes = format_bytes(s->pf);
		if (!sized || s->width == 0 || s->height == 0 || (bytes != 2 && bytes != 4)) {
			bf_tracef(BFT_ERR, "d3d: CreateSurface: offscreen surface without size or with %d bytes per pixel", bytes);
			delete s;
			return DDERR_INVALIDPARAMS;
		}
		s->pitch = LONG(s->width) * bytes;
		s->cpu.assign(size_t(s->pitch) * s->height, 0);
		if (!(c & DDSCAPS_VIDEOMEMORY)) s->caps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	}
	*out = s;
	return DD_OK;
}

HRESULT DDraw::CreateClipper(DWORD, IDirectDrawClipper **out, IUnknown *outer)
{
	if (!out) return DDERR_INVALIDPARAMS;
	if (outer) return CLASS_E_NOAGGREGATION;
	*out = new Clipper;
	return DD_OK;
}

HRESULT DDraw::GetCaps(DDCAPS *drv, DDCAPS *hel)
{
	if (!drv && !hel) return DDERR_INVALIDPARAMS;
	DDCAPS_DX7 c;
	memset(&c, 0, sizeof c);
	c.dwCaps = DDCAPS_3D | DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY |
	           DDCAPS_CANBLTSYSMEM | DDCAPS_BLTDEPTHFILL;
	c.dwCaps2 = DDCAPS2_CANRENDERWINDOWED | DDCAPS2_WIDESURFACES | DDCAPS2_NOPAGELOCKREQUIRED | DDCAPS2_FLIPNOVSYNC;
	c.dwCKeyCaps = DDCKEYCAPS_SRCBLT;
	c.dwFXCaps = DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY | DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKY;
	c.dwZBufferBitDepths = DDBD_16 | DDBD_24 | DDBD_32;
	c.dwVidMemTotal = 0x40000000;
	c.dwVidMemFree = 0x40000000;
	c.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER |
	                   DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE |
	                   DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_MIPMAP | DDSCAPS_LOCALVIDMEM;
	for (DDCAPS *p : {drv, hel}) {
		if (!p) continue;
		DWORD size = p->dwSize;
		if (size < offsetof(DDCAPS_DX7, dwCaps) + 4 || size > sizeof(DDCAPS_DX7)) return DDERR_INVALIDPARAMS;
		memcpy(p, &c, size);
		p->dwSize = size;
	}
	return DD_OK;
}

HRESULT DDraw::GetAvailableVidMem(DDSCAPS2 *, DWORD *total, DWORD *free_)
{
	if (total) *total = 0x40000000;
	if (free_) *free_ = 0x40000000;
	return DD_OK;
}

HRESULT DDraw::WaitForVerticalBlank(DWORD, HANDLE) { return DD_OK; }   // bf_gfx_present paces on the display
HRESULT DDraw::FlipToGDISurface() { return DD_OK; }
HRESULT DDraw::RestoreAllSurfaces() { return DD_OK; }                 // Metal loses no surfaces
HRESULT DDraw::TestCooperativeLevel() { return DD_OK; }
HRESULT DDraw::RestoreDisplayMode() { return DD_OK; }
HRESULT DDraw::Compact() { return DD_OK; }
HRESULT DDraw::GetMonitorFrequency(LPDWORD f) { if (!f) return DDERR_INVALIDPARAMS; *f = 60; return DD_OK; }
HRESULT DDraw::GetVerticalBlankStatus(BOOL *b) { if (!b) return DDERR_INVALIDPARAMS; *b = FALSE; return DD_OK; }
HRESULT DDraw::GetScanLine(LPDWORD l) { if (!l) return DDERR_INVALIDPARAMS; *l = 0; return DDERR_VERTICALBLANKINPROGRESS; }
HRESULT DDraw::GetFourCCCodes(LPDWORD n, LPDWORD) { if (n) *n = 0; return DD_OK; }
HRESULT DDraw::Initialize(GUID *) { return DDERR_ALREADYINITIALIZED; }

HRESULT DDraw::GetDeviceIdentifier(DDDEVICEIDENTIFIER2 *id, DWORD)
{
	if (!id) return DDERR_INVALIDPARAMS;
	memset(id, 0, sizeof *id);
	snprintf(id->szDriver, sizeof id->szDriver, "%s", "Metal");
	snprintf(id->szDescription, sizeof id->szDescription, "%s",
	         g().gfx ? bf_gfx_device_name(g().gfx) : "Metal");
	return DD_OK;
}

// Not used by the client.
HRESULT DDraw::CreatePalette(DWORD, PALETTEENTRY *, IDirectDrawPalette **p, IUnknown *) { BF_STUB(); if (p) *p = nullptr; return DDERR_UNSUPPORTED; }
HRESULT DDraw::DuplicateSurface(IDirectDrawSurface7 *, IDirectDrawSurface7 **d) { BF_STUB(); if (d) *d = nullptr; return DDERR_UNSUPPORTED; }
HRESULT DDraw::EnumSurfaces(DWORD, DDSURFACEDESC2 *, void *, LPDDENUMSURFACESCALLBACK7) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT DDraw::GetGDISurface(IDirectDrawSurface7 **s) { BF_STUB(); if (s) *s = nullptr; return DDERR_NOTFOUND; }
HRESULT DDraw::GetSurfaceFromDC(HDC, IDirectDrawSurface7 **s) { BF_STUB(); if (s) *s = nullptr; return DDERR_NOTFOUND; }
HRESULT DDraw::StartModeTest(LPSIZE, DWORD, DWORD) { BF_STUB(); return DDERR_UNSUPPORTED; }
HRESULT DDraw::EvaluateMode(DWORD, DWORD *) { BF_STUB(); return DDERR_UNSUPPORTED; }

} // namespace bfd3d

// ---- entry points ----------------------------------------------------------------------------

using namespace bfd3d;

extern "C" HRESULT WINAPI DirectDrawCreateEx(GUID *driver, void **out, REFIID iid, IUnknown *outer)
{
	if (!out) return DDERR_INVALIDPARAMS;
	*out = nullptr;
	if (outer) return CLASS_E_NOAGGREGATION;
	if (!IsEqualGUID(iid, IID_IDirectDraw7)) return DDERR_INVALIDPARAMS;
	if (!g().gfx) {
		bf_tracef(BFT_ERR, "d3d: DirectDrawCreateEx before bf_compat_d3d_set_device");
		return DDERR_NODIRECTDRAWHW;
	}
	(void)driver;   // any driver GUID (or NULL) is the one Metal device
	DDraw *dd = new DDraw;
	g().ddraw = dd;
	*out = static_cast<IDirectDraw7 *>(dd);
	return DD_OK;
}

extern "C" void bf_compat_d3d_set_device(bf_gfx_device *dev)
{
	Global &G = g();
	if (G.gfx == dev) return;
	if (G.gfx) {
		target_drop_buffer();
		for (Surface *s : G.textures) {
			bf_gfx_texture_destroy(s->tex);
			s->tex = nullptr;
			s->lost = true;
		}
		G.textures.clear();
		bf_gfx_texture_destroy(G.part_tex);
		G.part_tex = nullptr;
		if (G.backup_holder) {
			G.backup_holder->holds_backup = false;
			G.backup_holder = nullptr;
		}
	}
	G.gfx = dev;
	++G.target_serial;
}

extern "C" bf_gfx_device *bf_compat_d3d_get_device(void) { return g().gfx; }

extern "C" void bf_compat_d3d_get_stats(long long *locks, long long *readbacks, long long *writebacks)
{
	if (locks) *locks = g().target_locks;
	if (readbacks) *readbacks = g().target_readbacks;
	if (writebacks) *writebacks = g().target_writebacks;
}
