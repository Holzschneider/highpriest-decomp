// bifrost-codec-test: zlib round trips, JPEG round trips, one real GRF entry.
#include "bf_codec.h"
#include "bf_trace.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

static int g_fail = 0;

#define CHECK(name, cond) do { \
	bool ok_ = (cond); \
	printf("%-5s %s\n", ok_ ? "ok" : "FAIL", name); \
	if (!ok_) g_fail++; \
} while (0)

static std::vector<uint8_t> make_data(size_t n)
{
	std::vector<uint8_t> v(n);
	uint32_t s = 12345;
	for (size_t i = 0; i < n; i++) {
		s = s * 1664525u + 1013904223u;
		// mostly compressible, with noise
		v[i] = (i % 7 == 0) ? (uint8_t)(s >> 24) : (uint8_t)(i / 13);
	}
	return v;
}

static bool roundtrip(const std::vector<uint8_t> &in, int level)
{
	std::vector<uint8_t> z(bf_codec_deflate_bound(in.size()));
	size_t zn = z.size();
	if (bf_codec_deflate(z.data(), &zn, in.data(), in.size(), level) != BF_CODEC_OK) return false;
	std::vector<uint8_t> out(in.size() ? in.size() : 1);
	size_t on = in.size();
	if (bf_codec_inflate(out.data(), &on, z.data(), zn) != BF_CODEC_OK) return false;
	return on == in.size() && memcmp(out.data(), in.data(), in.size()) == 0;
}

static void test_zlib()
{
	CHECK("zlib: empty buffer round trip", roundtrip(std::vector<uint8_t>(), -1));
	CHECK("zlib: 1 byte round trip", roundtrip(make_data(1), 9));
	CHECK("zlib: 100 KB round trip, level 0", roundtrip(make_data(100000), 0));
	CHECK("zlib: 64 MiB round trip", roundtrip(make_data(64u << 20), 1));

	// empty result into a NULL destination, as the stream says
	{
		uint8_t z[16];
		size_t zn = sizeof z;
		bf_codec_deflate(z, &zn, NULL, 0, -1);
		size_t on = 0;
		CHECK("zlib: empty stream into capacity 0", bf_codec_inflate(NULL, &on, z, zn) == BF_CODEC_OK && on == 0);
	}

	std::vector<uint8_t> in = make_data(50000);
	std::vector<uint8_t> z(bf_codec_deflate_bound(in.size()));
	size_t zn = z.size();
	bf_codec_deflate(z.data(), &zn, in.data(), in.size(), -1);

	{
		std::vector<uint8_t> out(1000);
		size_t on = out.size();
		CHECK("zlib: destination too small is E_BUFFER",
		      bf_codec_inflate(out.data(), &on, z.data(), zn) == BF_CODEC_E_BUFFER && on == 1000 &&
		      memcmp(out.data(), in.data(), 1000) == 0);
	}
	{
		std::vector<uint8_t> c(z.begin(), z.begin() + zn);
		for (size_t i = 20; i < c.size(); i += 97) c[i] ^= 0x5a;
		std::vector<uint8_t> out(in.size());
		size_t on = out.size();
		CHECK("zlib: corrupt stream is E_DATA", bf_codec_inflate(out.data(), &on, c.data(), c.size()) == BF_CODEC_E_DATA);
	}
	{
		std::vector<uint8_t> out(in.size());
		size_t on = out.size();
		CHECK("zlib: truncated stream is E_DATA", bf_codec_inflate(out.data(), &on, z.data(), zn / 2) == BF_CODEC_E_DATA);
		on = out.size();
		const char junk[] = "this is not zlib";
		CHECK("zlib: garbage is E_DATA", bf_codec_inflate(out.data(), &on, junk, sizeof junk) == BF_CODEC_E_DATA);
		on = out.size();
		CHECK("zlib: zero-length input is E_DATA", bf_codec_inflate(out.data(), &on, junk, 0) == BF_CODEC_E_DATA);
		on = out.size();
		CHECK("zlib: null destination with capacity is E_ARG", bf_codec_inflate(NULL, &on, z.data(), zn) == BF_CODEC_E_ARG);
	}
}

// A smooth RGB test picture with a flat patch and the client's colour key.
static void make_picture(std::vector<uint8_t> &bgr, int w, int h)
{
	bgr.resize((size_t)w * h * 3);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			uint8_t *p = &bgr[((size_t)y * w + x) * 3];
			p[0] = (uint8_t)(x * 255 / (w - 1));            // B
			p[1] = (uint8_t)(y * 255 / (h - 1));            // G
			p[2] = (uint8_t)(255 - (x + y) * 255 / (w + h - 2)); // R
			if (x >= 8 && x < 24 && y >= 8 && y < 24) { p[0] = 255; p[1] = 0; p[2] = 255; } // key magenta
		}
}

// Largest difference between two BGR24 pictures, leaving out a margin around
// the hard-edged key patch (8..23): chroma subsampling smears edges.
static int max_diff_smooth(const uint8_t *a, const uint8_t *b, int w, int h)
{
	int m = 0;
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			if (x >= 8 - 8 && x < 24 + 8 && y >= 8 - 8 && y < 24 + 8) continue;
			for (int c = 0; c < 3; c++) {
				size_t i = ((size_t)y * w + x) * 3 + c;
				int d = abs((int)a[i] - (int)b[i]);
				if (d > m) m = d;
			}
		}
	return m;
}

static int max_diff(const uint8_t *a, const uint8_t *b, size_t n, int)
{
	int m = 0;
	for (size_t i = 0; i < n; i++) {
		int d = abs((int)a[i] - (int)b[i]);
		if (d > m) m = d;
	}
	return m;
}

static void test_jpeg(const std::string &outdir)
{
	const int W = 64, H = 48;
	std::vector<uint8_t> bgr;
	make_picture(bgr, W, H);

	void *jpg = NULL;
	size_t jn = 0;
	int rc = bf_codec_jpeg_encode_mem(&jpg, &jn, bgr.data(), W, H, 0, BF_CODEC_BGR24, 95);
	CHECK("jpeg: encode BGR24 to memory", rc == BF_CODEC_OK && jpg && jn > 100 && ((uint8_t *)jpg)[0] == 0xff && ((uint8_t *)jpg)[1] == 0xd8);

	int w = 0, h = 0;
	CHECK("jpeg: info reports size", bf_codec_jpeg_info(jpg, jn, &w, &h) == BF_CODEC_OK && w == W && h == H);

	std::vector<uint8_t> back((size_t)W * H * 3);
	rc = bf_codec_jpeg_decode(jpg, jn, back.data(), W, H, 0, BF_CODEC_BGR24);
	int d = max_diff_smooth(back.data(), bgr.data(), W, H);
	printf("      decode BGR24: max difference %d away from the patch edges\n", d);
	CHECK("jpeg: decode BGR24 within tolerance of the source (quality 95)", rc == BF_CODEC_OK && d <= 12);

	// colour key survives: the flat magenta patch centre must stay near 255,0,255
	{
		const uint8_t *p = &back[((size_t)16 * W + 16) * 3];
		printf("      key patch centre B,G,R = %d,%d,%d\n", p[0], p[1], p[2]);
		CHECK("jpeg: flat magenta patch keeps the client's colour key range", p[0] >= 0xfe - 3 && p[1] <= 3 + 3 && p[2] >= 0xfe - 3);
	}

	// BGRA32 into a padded buffer
	{
		size_t stride = (size_t)W * 4 + 16;
		std::vector<uint8_t> px(stride * H, 0xcc);
		rc = bf_codec_jpeg_decode(jpg, jn, px.data(), W, H, stride, BF_CODEC_BGRA32);
		bool alpha = true, same = true, pad = true;
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				const uint8_t *q = &px[y * stride + x * 4];
				const uint8_t *r = &back[((size_t)y * W + x) * 3];
				if (q[3] != 0xff) alpha = false;
				if (q[0] != r[0] || q[1] != r[1] || q[2] != r[2]) same = false;
			}
			for (size_t i = (size_t)W * 4; i < stride; i++) if (px[y * stride + i] != 0xcc) pad = false;
		}
		CHECK("jpeg: decode BGRA32 equals BGR24 decode, alpha 255, row padding untouched",
		      rc == BF_CODEC_OK && alpha && same && pad);
	}

	// encode from BGRA32 with stride, write a file, decode the file
	{
		size_t stride = (size_t)W * 4 + 8;
		std::vector<uint8_t> px(stride * H, 0);
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++) {
				memcpy(&px[y * stride + x * 4], &bgr[((size_t)y * W + x) * 3], 3);
				px[y * stride + x * 4 + 3] = 0x40;   // alpha must be ignored
			}
		std::string path = outdir + "/codec-test.jpg";
		rc = bf_codec_jpeg_encode_file(path.c_str(), px.data(), W, H, stride, BF_CODEC_BGRA32, 90);
		CHECK("jpeg: encode BGRA32 to a file", rc == BF_CODEC_OK);
		std::vector<uint8_t> file;
		if (FILE *f = fopen(path.c_str(), "rb")) {
			uint8_t b[4096];
			size_t n;
			while ((n = fread(b, 1, sizeof b, f)) > 0) file.insert(file.end(), b, b + n);
			fclose(f);
		}
		std::vector<uint8_t> again((size_t)W * H * 4);
		int fw = 0, fh = 0;
		bool okd = !file.empty() && bf_codec_jpeg_info(file.data(), file.size(), &fw, &fh) == BF_CODEC_OK &&
		           fw == W && fh == H &&
		           bf_codec_jpeg_decode(file.data(), file.size(), again.data(), W, H, 0, BF_CODEC_BGRA32) == BF_CODEC_OK;
		int d2 = 255;
		if (okd) {
			std::vector<uint8_t> a3((size_t)W * H * 3);
			for (int i = 0; i < W * H; i++) memcpy(&a3[i * 3], &again[i * 4], 3);
			d2 = max_diff_smooth(a3.data(), bgr.data(), W, H);
		}
		printf("      file round trip: %zu bytes, max difference %d\n", file.size(), d2);
		CHECK("jpeg: file decodes again within tolerance (quality 90)", okd && d2 <= 14);
		CHECK("jpeg: unwritable path is E_IO",
		      bf_codec_jpeg_encode_file("/nonexistent-dir/x.jpg", bgr.data(), W, H, 0, BF_CODEC_BGR24, 80) == BF_CODEC_E_IO);
	}

	// failures
	{
		std::vector<uint8_t> out((size_t)W * H * 3);
		std::vector<uint8_t> trunc((uint8_t *)jpg, (uint8_t *)jpg + jn / 3);
		int tw, th;
		int r1 = bf_codec_jpeg_info(trunc.data(), 4, &tw, &th);
		const char junk[] = "GIF89a not a jpeg at all, but long enough to be looked at";
		int r2 = bf_codec_jpeg_info(junk, sizeof junk, &tw, &th);
		CHECK("jpeg: junk and stub input are E_DATA", r1 == BF_CODEC_E_DATA && r2 == BF_CODEC_E_DATA);
		CHECK("jpeg: size mismatch is E_ARG",
		      bf_codec_jpeg_decode(jpg, jn, out.data(), W + 1, H, 0, BF_CODEC_BGR24) == BF_CODEC_E_ARG);
		CHECK("jpeg: stride too small is E_ARG",
		      bf_codec_jpeg_decode(jpg, jn, out.data(), W, H, 10, BF_CODEC_BGR24) == BF_CODEC_E_ARG);
		// truncated body: must return (E_DATA or a partial image), never crash
		int r3 = bf_codec_jpeg_decode(trunc.data(), trunc.size(), out.data(), W, H, 0, BF_CODEC_BGR24);
		printf("      truncated JPEG body decodes with code %d\n", r3);
		CHECK("jpeg: truncated body returns without crashing", r3 == BF_CODEC_OK || r3 == BF_CODEC_E_DATA);
	}
	bf_codec_free(jpg);

	// a grey JPEG: values must come out as grey with R=G=B and the level kept
	{
		std::vector<uint8_t> g((size_t)W * H * 3);
		for (int i = 0; i < W * H; i++) { uint8_t v = (uint8_t)((i % W) * 4); g[i * 3] = g[i * 3 + 1] = g[i * 3 + 2] = v; }
		void *gj = NULL; size_t gn = 0;
		bf_codec_jpeg_encode_mem(&gj, &gn, g.data(), W, H, 0, BF_CODEC_BGR24, 98);
		std::vector<uint8_t> o(g.size());
		int r = gj ? bf_codec_jpeg_decode(gj, gn, o.data(), W, H, 0, BF_CODEC_BGR24) : -1;
		int dg = max_diff(o.data(), g.data(), g.size(), 0);
		printf("      grey ramp: max difference %d\n", dg);
		CHECK("jpeg: grey ramp keeps its levels", r == BF_CODEC_OK && dg <= 8);
		bf_codec_free(gj);
	}
}

// ---- one real GRF entry -------------------------------------------------

struct GrfEntry { std::string name; uint32_t packed, aligned, size, offset; uint8_t flags; };

static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

static void test_grf(const std::string &dir)
{
	std::string path = dir + "/data.grf";
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) { printf("skipped  GRF: %s not present\n", path.c_str()); return; }

	uint8_t head[46];
	if (fread(head, 1, 46, f) != 46 || memcmp(head, "Master of Magic", 15) != 0) {
		CHECK("grf: header", false); fclose(f); return;
	}
	uint32_t table_offset = rd32(head + 30), seed = rd32(head + 34), count = rd32(head + 38), version = rd32(head + 42);
	printf("      %s: version 0x%x, %u files\n", path.c_str(), version, count - seed - 7);
	uint8_t th[8];
	fseeko(f, 46 + (off_t)table_offset, SEEK_SET);
	if (version >> 8 != 2 || fread(th, 1, 8, f) != 8) { CHECK("grf: table header", false); fclose(f); return; }
	uint32_t tpacked = rd32(th), tsize = rd32(th + 4);
	std::vector<uint8_t> tz(tpacked), raw(tsize);
	size_t rn = tsize;
	bool ok = fread(tz.data(), 1, tpacked, f) == tpacked;
	ok = ok && bf_codec_inflate(raw.data(), &rn, tz.data(), tpacked) == BF_CODEC_OK && rn == tsize;
	CHECK("grf: inflate the file table", ok);
	if (!ok) { fclose(f); return; }

	std::vector<GrfEntry> files;
	size_t at = 0;
	while (at < raw.size()) {
		const char *s = (const char *)&raw[at];
		size_t len = strnlen(s, raw.size() - at);
		if (at + len + 18 > raw.size()) break;
		const uint8_t *e = &raw[at + len + 1];
		GrfEntry g;
		g.name.assign(s, len);
		g.packed = rd32(e); g.aligned = rd32(e + 4); g.size = rd32(e + 8); g.flags = e[12]; g.offset = rd32(e + 13);
		if ((g.flags & 1) && !(g.flags & 6) && g.size > 0) files.push_back(g);   // plain zlib entries
		at += len + 18;
	}
	printf("      %zu plain compressed entries in the table\n", files.size());

	auto read_entry = [&](const GrfEntry &g, std::vector<uint8_t> &out) {
		std::vector<uint8_t> z(g.packed);
		out.assign(g.size, 0);
		size_t n = g.size;
		fseeko(f, 46 + (off_t)g.offset, SEEK_SET);
		if (fread(z.data(), 1, g.packed, f) != g.packed) return false;
		return bf_codec_inflate(out.data(), &n, z.data(), g.packed) == BF_CODEC_OK && n == g.size;
	};

	// 40 entries spread over the table
	int good = 0, tried = 0;
	for (size_t i = 0; i < files.size() && tried < 40; i += files.size() / 40 + 1) {
		std::vector<uint8_t> out;
		tried++;
		if (read_entry(files[i], out)) good++;
		else printf("      failed: %s\n", files[i].name.c_str());
	}
	CHECK("grf: 40 entries across the archive inflate to their recorded size", tried > 0 && good == tried);

	const GrfEntry *jpg = NULL;
	for (const GrfEntry &g : files) {
		size_t l = g.name.size();
		if (l > 4 && strcasecmp(g.name.c_str() + l - 4, ".jpg") == 0) { jpg = &g; break; }
	}
	if (!jpg) {
		printf("skipped  GRF: no .jpg entry in the archive\n");
	} else {
		std::vector<uint8_t> out;
		int w = 0, h = 0;
		bool okj = read_entry(*jpg, out) && bf_codec_jpeg_info(out.data(), out.size(), &w, &h) == BF_CODEC_OK;
		if (okj) {
			std::vector<uint8_t> px((size_t)w * h * 4);
			okj = bf_codec_jpeg_decode(out.data(), out.size(), px.data(), w, h, 0, BF_CODEC_BGRA32) == BF_CODEC_OK;
		}
		printf("      first .jpg entry: %zu bytes, %dx%d\n", out.size(), w, h);
		CHECK("grf: first .jpg entry decodes", okj);
	}
	fclose(f);
}

int main()
{
	bf_trace_init();
	const char *dirEnv = getenv("BF_CLIENT_DIR");
	std::string dir = dirEnv && *dirEnv ? dirEnv : std::string(getenv("HOME") ? getenv("HOME") : ".") + "/Games/" BF_APP_NAME;
	const char *outEnv = getenv("BF_TEST_OUT");
	std::string out = outEnv && *outEnv ? outEnv : ".";

	test_zlib();
	test_jpeg(out);
	test_grf(dir);

	printf("%s (%d failed)\n", g_fail ? "FAILED" : "PASSED", g_fail);
	return g_fail ? 1 : 0;
}
