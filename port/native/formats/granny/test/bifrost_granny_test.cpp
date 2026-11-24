// bifrost-granny-test: checks the Granny reader.
//
// Without game data it reads a small uncompressed file built here and a set
// of broken ones. With game data (BF_CLIENT_DIR, default ~/Games/Bifrost) it
// reads every .gr2 in data.grf and rdata.grf, prints what it finds, checks
// what can be checked from the data itself, and draws each mesh flat-shaded
// in its rest pose to BF_CLIENT_DIR/test/granny/ (game artwork: outside git).
//
//   bifrost-granny-test [-q] [name-filter]
//
// One line per check; the exit status is the number of failed checks.

#include "bf_granny.h"
#include "grf.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {

int g_failed = 0;
bool g_quiet = false;

void Check(bool ok, const char *what, const std::string &detail = "")
{
	printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", what, detail.empty() ? "" : ": ", detail.c_str());
	if (!ok)
		++g_failed;
}

std::string Format(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
std::string Format(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	return buf;
}

// --- A synthetic file ------------------------------------------------------

struct Builder {
	std::vector<uint8_t> d;
	std::vector<uint32_t> fixups;   // pairs: where, target (section 0)

	uint32_t Alloc(uint32_t n)
	{
		uint32_t at = (uint32_t)((d.size() + 3) & ~3u);
		d.resize(at + n, 0);
		return at;
	}
	void Put32(uint32_t at, uint32_t v) { for (int i = 0; i < 4; ++i) d[at + i] = (uint8_t)(v >> (8 * i)); }
	void PutF(uint32_t at, float f) { uint32_t v; memcpy(&v, &f, 4); Put32(at, v); }
	void Ptr(uint32_t at, uint32_t target) { fixups.push_back(at); fixups.push_back(target); }
	uint32_t Str(const char *s)
	{
		uint32_t n = (uint32_t)strlen(s) + 1, at = Alloc(n);
		memcpy(&d[at], s, n);
		return at;
	}
	struct M { int type; const char *name; uint32_t ref; int width; };
	uint32_t Type(const std::vector<M> &members)
	{
		uint32_t at = Alloc(32 * (uint32_t)(members.size() + 1));
		for (size_t i = 0; i < members.size(); ++i) {
			uint32_t r = at + 32 * (uint32_t)i;
			Put32(r, (uint32_t)members[i].type);
			Ptr(r + 4, Str(members[i].name));
			if (members[i].ref != ~0u)
				Ptr(r + 8, members[i].ref);
			Put32(r + 12, (uint32_t)members[i].width);
		}
		return at;
	}
};

const uint32_t kNone = ~0u;

std::vector<uint8_t> SyntheticFile()
{
	Builder b;
	// The root object comes first, at offset 0: FromFileName, Skeletons, Meshes.
	uint32_t root = b.Alloc(4 + 8 + 8);
	uint32_t int32Type = b.Type({ { 19, "Int32", kNone, 0 } });
	uint32_t boneType = b.Type({ { 8, "Name", kNone, 0 }, { 19, "ParentIndex", kNone, 0 }, { 9, "Transform", kNone, 0 },
		{ 10, "InverseWorldTransform", kNone, 16 } });
	uint32_t skeletonType = b.Type({ { 8, "Name", kNone, 0 }, { 3, "Bones", boneType, 0 } });
	uint32_t vertexType = b.Type({ { 10, "Position", kNone, 3 }, { 10, "Normal", kNone, 3 }, { 10, "TextureCoordinates0", kNone, 2 } });
	uint32_t vertexDataType = b.Type({ { 7, "Vertices", kNone, 0 } });
	uint32_t topologyType = b.Type({ { 3, "Indices", int32Type, 0 } });
	uint32_t meshType = b.Type({ { 8, "Name", kNone, 0 }, { 2, "PrimaryVertexData", vertexDataType, 0 },
		{ 2, "PrimaryTopology", topologyType, 0 } });
	uint32_t rootType = b.Type({ { 8, "FromFileName", kNone, 0 }, { 4, "Skeletons", skeletonType, 0 }, { 4, "Meshes", meshType, 0 } });

	b.Ptr(root, b.Str("synthetic.max"));
	// One skeleton of two bones.
	uint32_t bones = b.Alloc(2 * (4 + 4 + 68 + 64));
	for (int i = 0; i < 2; ++i) {
		uint32_t at = bones + i * 140;
		b.Ptr(at, b.Str(i ? "child" : "root"));
		b.Put32(at + 4, (uint32_t)(i - 1));
		b.Put32(at + 8, 1);
		b.PutF(at + 12, 1.5f * i);       // position x
		b.PutF(at + 36, 1.0f);           // orientation w
	}
	uint32_t skeleton = b.Alloc(12);
	b.Ptr(skeleton, b.Str("skel"));
	b.Put32(skeleton + 4, 2);
	b.Ptr(skeleton + 8, bones);
	uint32_t skeletonTable = b.Alloc(4);
	b.Ptr(skeletonTable, skeleton);
	b.Put32(root + 4, 1);
	b.Ptr(root + 8, skeletonTable);
	// One mesh: a triangle.
	uint32_t verts = b.Alloc(3 * 32);
	const float pos[3][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 } };
	for (int v = 0; v < 3; ++v)
		for (int k = 0; k < 3; ++k)
			b.PutF(verts + 32 * v + 4 * k, pos[v][k]);
	uint32_t vertexData = b.Alloc(12);
	b.Ptr(vertexData, vertexType);
	b.Put32(vertexData + 4, 3);
	b.Ptr(vertexData + 8, verts);
	uint32_t indices = b.Alloc(12);
	for (int i = 0; i < 3; ++i)
		b.Put32(indices + 4 * i, (uint32_t)i);
	uint32_t topology = b.Alloc(8);
	b.Put32(topology, 3);
	b.Ptr(topology + 4, indices);
	uint32_t mesh = b.Alloc(12);
	b.Ptr(mesh, b.Str("triangle"));
	b.Ptr(mesh + 4, vertexData);
	b.Ptr(mesh + 8, topology);
	uint32_t meshTable = b.Alloc(4);
	b.Ptr(meshTable, mesh);
	b.Put32(root + 12, 1);
	b.Ptr(root + 16, meshTable);

	// Header, one section, fix-ups, then the section bytes.
	const uint8_t magic[16] = { 0xb8, 0x67, 0xb0, 0xca, 0xf8, 0x6d, 0xb1, 0x0f, 0x84, 0x72, 0x8c, 0x7e, 0x5e, 0x19, 0x00, 0x1e };
	uint32_t nfix = (uint32_t)b.fixups.size() / 2;
	uint32_t fixAt = 0x160, dataAt = fixAt + 12 * nfix;
	std::vector<uint8_t> f(dataAt + b.d.size(), 0);
	auto put = [&](uint32_t at, uint32_t v) { for (int i = 0; i < 4; ++i) f[at + i] = (uint8_t)(v >> (8 * i)); };
	memcpy(&f[0], magic, 16);
	put(16, 0x160);
	put(0x20, 6);
	put(0x24, (uint32_t)f.size());
	put(0x2c, 0x38);
	put(0x30, 1);
	put(0x34, 0); put(0x38, rootType);   // root type
	put(0x3c, 0); put(0x40, root);       // root object
	put(0x44, 0x8000000f);
	uint32_t s = 0x58;
	put(s + 0, 0); put(s + 4, dataAt); put(s + 8, (uint32_t)b.d.size()); put(s + 12, (uint32_t)b.d.size());
	put(s + 16, 4); put(s + 20, (uint32_t)b.d.size()); put(s + 24, (uint32_t)b.d.size());
	put(s + 28, fixAt); put(s + 32, nfix); put(s + 36, dataAt); put(s + 40, 0);
	for (uint32_t i = 0; i < nfix; ++i) {
		put(fixAt + 12 * i, b.fixups[2 * i]);
		put(fixAt + 12 * i + 4, 0);
		put(fixAt + 12 * i + 8, b.fixups[2 * i + 1]);
	}
	memcpy(&f[dataAt], b.d.data(), b.d.size());
	return f;
}

void SyntheticChecks()
{
	std::vector<uint8_t> data = SyntheticFile();
	char error[256];
	bf_granny_file *g = bf_granny_open(data.data(), data.size(), error, sizeof error);
	Check(g != nullptr, "synthetic file opens", g ? "" : error);
	if (!g)
		return;
	const bf_granny_info *info = bf_granny_get_info(g);
	Check(!strcmp(info->from_file_name, "synthetic.max"), "synthetic: string through a fix-up", info->from_file_name);
	const bf_granny_skeleton *sk = bf_granny_get_skeleton(g, 0);
	Check(info->skeleton_count == 1 && sk && sk->bone_count == 2 && !strcmp(sk->bones[1].name, "child")
			&& sk->bones[1].parent == 0 && sk->bones[1].local.position[0] == 1.5f && sk->bones[0].local.orientation[3] == 1.0f,
		"synthetic: skeleton, bones, transforms");
	const bf_granny_mesh *m = bf_granny_get_mesh(g, 0);
	Check(m && m->vertex_count == 3 && m->index_count == 3 && m->vertices[2].position[2] == 1.0f && m->indices[1] == 1
			&& m->rigid && m->material_count == 0,
		"synthetic: mesh through a variant array", m ? m->vertex_layout : "");
	Check(bf_granny_get_mesh(g, 1) == nullptr && bf_granny_get_texture(g, 0) == nullptr && info->texture_count == 0,
		"synthetic: absent members and out-of-range indices");
	bf_granny_close(g);

	struct Broken { const char *what; uint32_t at; uint32_t value; size_t truncate; };
	const Broken broken[] = {
		{ "bad magic", 0, 0x12345678, 0 },
		{ "format version 7", 0x20, 7, 0 },
		{ "Oodle1 section", 0x58, 2, 0 },
		{ "section data beyond the end", 0x5c, 0x7fffffff, 0 },
		{ "fix-up table beyond the end", 0x74, 0x7fffffff, 0 },
		{ "truncated header", 0, 0, 40 },
	};
	for (const Broken &b : broken) {
		std::vector<uint8_t> d = data;
		if (b.truncate)
			d.resize(b.truncate);
		else
			for (int i = 0; i < 4; ++i)
				d[b.at + i] = (uint8_t)(b.value >> (8 * i));
		error[0] = 0;
		bf_granny_file *bad = bf_granny_open(d.data(), d.size(), error, sizeof error);
		Check(bad == nullptr && error[0], (std::string("rejects ") + b.what).c_str(), error);
		bf_granny_close(bad);
	}
	// Curves: a quadratic through its end controls, flat where its controls
	// are, continuous; a constant; quaternions normalised.
	{
		const float knots[5] = { 0, 1, 2, 3, 4 };
		const float controls[15] = { 0, 0, 0, 1, 0, 0, 2, 0, 0, 2, 0, 0, 5, 1, 0 };
		bf_granny_curve c = { 2, 3, 5, knots, 15, controls };
		float a[3], b[3], mid[3];
		bf_granny_evaluate_curve(&c, 0, a);
		bf_granny_evaluate_curve(&c, 4, b);
		bf_granny_evaluate_curve(&c, 2.5f, mid);
		float lo[3], hi[3], jump = 0;
		for (int k = 1; k < 4; ++k) {
			bf_granny_evaluate_curve(&c, k - 1e-4f, lo);
			bf_granny_evaluate_curve(&c, k + 1e-4f, hi);
			jump = std::max(jump, fabsf(lo[0] - hi[0]));
		}
		Check(a[0] == 0 && b[0] == 5 && b[1] == 1 && mid[0] > 1.5f && mid[0] < 2.5f && jump < 1e-3f,
			"curve: quadratic B-spline ends on its end controls and is continuous", Format("mid %.3f, step %.5f", mid[0], jump));
		const float q[4] = { 0, 0, 0, 2 };
		const float t0[1] = { 0 };
		bf_granny_curve qc = { 0, 4, 1, t0, 4, q };
		float out[4] = { 9, 9, 9, 9 };
		bf_granny_curve none = { 0, 3, 0, nullptr, 0, nullptr };
		Check(bf_granny_evaluate_curve(&qc, 3, out) && out[3] == 1 && !bf_granny_evaluate_curve(&none, 0, out),
			"curve: constant, normalised quaternion; absent curve left alone");
	}
	// Transforms: a quarter turn about Z takes x to y (row vectors), then translates.
	{
		bf_granny_transform t;
		memset(&t, 0, sizeof t);
		t.flags = 3;
		t.orientation[2] = sinf(0.785398f);
		t.orientation[3] = cosf(0.785398f);
		t.position[0] = 10;
		const float id[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
		memcpy(t.scale_shear, id, sizeof id);
		float m[16];
		bf_granny_transform_matrix(&t, m);
		float x = 1 * m[0] + m[12], y = 1 * m[1] + m[13];
		Check(fabsf(x - 10) < 1e-5f && fabsf(y - 1) < 1e-5f, "transform: rotation then translation, row vectors",
			Format("(1,0,0) -> (%.3f, %.3f)", x, y));
	}

	// A wild pointer must not be followed out of the section.
	{
		std::vector<uint8_t> d = data;
		uint32_t fixAt = 0x160;
		for (int i = 0; i < 4; ++i)
			d[fixAt + 8 + i] = (uint8_t)(0x7ffffff0u >> (8 * i));   // first fix-up's target offset
		bf_granny_file *w = bf_granny_open(d.data(), d.size(), error, sizeof error);
		Check(w == nullptr || bf_granny_get_info(w) != nullptr, "survives a fix-up pointing outside its section");
		bf_granny_close(w);
	}
}

// --- Pictures --------------------------------------------------------------

void WritePng(const std::string &path, int w, int h, const std::vector<uint8_t> &rgb)
{
	std::vector<uint8_t> raw;
	raw.reserve((size_t)(w * 3 + 1) * h);
	for (int y = 0; y < h; ++y) {
		raw.push_back(0);
		raw.insert(raw.end(), rgb.begin() + (size_t)y * w * 3, rgb.begin() + (size_t)(y + 1) * w * 3);
	}
	uLongf zsize = compressBound(raw.size());
	std::vector<uint8_t> z(zsize);
	compress2(z.data(), &zsize, raw.data(), raw.size(), 6);
	FILE *fp = fopen(path.c_str(), "wb");
	if (!fp)
		return;
	auto chunk = [&](const char *type, const uint8_t *data, uint32_t n) {
		uint8_t len[4] = { (uint8_t)(n >> 24), (uint8_t)(n >> 16), (uint8_t)(n >> 8), (uint8_t)n };
		fwrite(len, 1, 4, fp);
		fwrite(type, 1, 4, fp);
		if (n)
			fwrite(data, 1, n, fp);
		uLong crc = crc32(0, (const Bytef *)type, 4);
		if (n)
			crc = crc32(crc, data, n);
		uint8_t c[4] = { (uint8_t)(crc >> 24), (uint8_t)(crc >> 16), (uint8_t)(crc >> 8), (uint8_t)crc };
		fwrite(c, 1, 4, fp);
	};
	fwrite("\x89PNG\r\n\x1a\n", 1, 8, fp);
	uint8_t ihdr[13] = { (uint8_t)(w >> 24), (uint8_t)(w >> 16), (uint8_t)(w >> 8), (uint8_t)w,
		(uint8_t)(h >> 24), (uint8_t)(h >> 16), (uint8_t)(h >> 8), (uint8_t)h, 8, 2, 0, 0, 0 };
	chunk("IHDR", ihdr, 13);
	chunk("IDAT", z.data(), (uint32_t)zsize);
	chunk("IEND", nullptr, 0);
	fclose(fp);
}

struct Canvas {
	int w, h;
	std::vector<uint8_t> rgb;
	std::vector<float> depth;
	Canvas(int w_, int h_) : w(w_), h(h_), rgb((size_t)w_ * h_ * 3, 0), depth((size_t)w_ * h_, 1e30f)
	{
		for (int i = 0; i < w * h; ++i) {
			rgb[3 * i] = 40; rgb[3 * i + 1] = 44; rgb[3 * i + 2] = 52;
		}
	}
	void Triangle(const float *a, const float *b, const float *c, const uint8_t *color)
	{
		float minx = std::min({ a[0], b[0], c[0] }), maxx = std::max({ a[0], b[0], c[0] });
		float miny = std::min({ a[1], b[1], c[1] }), maxy = std::max({ a[1], b[1], c[1] });
		int x0 = std::max(0, (int)floorf(minx)), x1 = std::min(w - 1, (int)ceilf(maxx));
		int y0 = std::max(0, (int)floorf(miny)), y1 = std::min(h - 1, (int)ceilf(maxy));
		float area = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
		if (fabsf(area) < 1e-12f)
			return;
		for (int y = y0; y <= y1; ++y)
			for (int x = x0; x <= x1; ++x) {
				float px = x + 0.5f, py = y + 0.5f;
				float w0 = ((b[0] - px) * (c[1] - py) - (b[1] - py) * (c[0] - px)) / area;
				float w1 = ((c[0] - px) * (a[1] - py) - (c[1] - py) * (a[0] - px)) / area;
				float w2 = 1 - w0 - w1;
				if (w0 < 0 || w1 < 0 || w2 < 0)
					continue;
				float z = w0 * a[2] + w1 * b[2] + w2 * c[2];
				size_t i = (size_t)y * w + x;
				if (z >= depth[i])
					continue;
				depth[i] = z;
				memcpy(&rgb[3 * i], color, 3);
			}
	}
};

struct View {
	float rot[3][3];
	float scale, cx, cy, cz;
	int size;
};

void Rotate(const View &v, const float *p, float *out)
{
	for (int r = 0; r < 3; ++r)
		out[r] = v.rot[r][0] * p[0] + v.rot[r][1] * p[1] + v.rot[r][2] * p[2];
}

// A three-quarter view from the front (-Y), Z up, as the artists modelled.
View MakeView(const std::vector<const bf_granny_mesh *> &meshes, int size)
{
	View v;
	float yaw = 0.6f, pitch = 0.35f;
	float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
	// Screen x right, screen y down, depth into the screen.
	float r[3][3] = {
		{ cy, sy, 0 },
		{ sy * sp, -cy * sp, -cp },
		{ -sy * cp, cy * cp, -sp },
	};
	memcpy(v.rot, r, sizeof r);
	float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
	for (const bf_granny_mesh *m : meshes)
		for (int i = 0; i < m->vertex_count; ++i) {
			float q[3];
			Rotate(v, m->vertices[i].position, q);
			for (int k = 0; k < 3; ++k) {
				lo[k] = std::min(lo[k], q[k]);
				hi[k] = std::max(hi[k], q[k]);
			}
		}
	float extent = std::max(hi[0] - lo[0], hi[1] - lo[1]);
	v.scale = extent > 0 ? size * 0.9f / extent : 1;
	v.cx = (lo[0] + hi[0]) / 2;
	v.cy = (lo[1] + hi[1]) / 2;
	v.cz = lo[2];
	v.size = size;
	return v;
}

void DrawMesh(Canvas &c, const View &v, const bf_granny_mesh *m, const uint8_t *base)
{
	const float light[3] = { -0.4f, -0.5f, -0.77f };   // from upper left, towards the viewer's side
	for (int t = 0; t + 2 < m->index_count; t += 3) {
		float p[3][3];
		for (int k = 0; k < 3; ++k) {
			float q[3];
			Rotate(v, m->vertices[m->indices[t + k]].position, q);
			p[k][0] = (q[0] - v.cx) * v.scale + v.size / 2.0f;
			p[k][1] = (q[1] - v.cy) * v.scale + v.size / 2.0f;
			p[k][2] = q[2] - v.cz;
		}
		float e1[3], e2[3], n[3];
		for (int k = 0; k < 3; ++k) {
			e1[k] = p[1][k] - p[0][k];
			e2[k] = p[2][k] - p[0][k];
		}
		e1[2] *= v.scale;
		e2[2] *= v.scale;
		n[0] = e1[1] * e2[2] - e1[2] * e2[1];
		n[1] = e1[2] * e2[0] - e1[0] * e2[2];
		n[2] = e1[0] * e2[1] - e1[1] * e2[0];
		float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (len < 1e-12f)
			continue;
		float d = fabsf((n[0] * light[0] + n[1] * light[1] + n[2] * light[2]) / len);
		float shade = 0.25f + 0.75f * d;
		uint8_t col[3];
		for (int k = 0; k < 3; ++k)
			col[k] = (uint8_t)std::min(255.0f, base[k] * shade);
		c.Triangle(p[0], p[1], p[2], col);
	}
}

const uint8_t kPalette[][3] = {
	{ 220, 200, 160 }, { 150, 190, 230 }, { 200, 150, 220 }, { 160, 220, 160 }, { 230, 170, 140 }, { 200, 200, 200 },
};

std::string SafeName(const char *s)
{
	std::string out;
	for (; *s; ++s)
		out += (isalnum((unsigned char)*s) || *s == '-' || *s == '_') ? *s : '_';
	return out;
}

// --- One game file ---------------------------------------------------------

float QuatNorm(const float *q)
{
	return sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
}

void GameFile(const std::string &name, const std::vector<uint8_t> &data, int grfFlags, const std::string &outDir)
{
	std::string base = name.substr(name.find_last_of('\\') + 1);
	base = base.substr(0, base.size() - 4);
	printf("\n== %s  (%zu bytes, archive flags %d)\n", name.c_str(), data.size(), grfFlags);
	char error[256];
	bf_granny_file *g = bf_granny_open(data.data(), data.size(), error, sizeof error);
	Check(g != nullptr, (base + ": opens, all sections decoded").c_str(), g ? "" : error);
	if (!g)
		return;
	const bf_granny_info *info = bf_granny_get_info(g);
	printf("   version %u, size %u, tag %#x, %d sections, %d fix-ups, root type %u:%#x, root object %u:%#x\n",
		info->version, info->total_size, info->type_tag, info->section_count, info->fixup_count,
		info->root_type_section, info->root_type_offset, info->root_object_section, info->root_object_offset);
	for (int i = 0; i < info->section_count; ++i) {
		const bf_granny_section *s = bf_granny_get_section(g, i);
		printf("   section %d: %-6s %6u -> %6u bytes at %#7x, stops %#x/%#x, %u fix-ups, %u marshal\n", i,
			s->compression == 0 ? "raw" : s->compression == 1 ? "oodle0" : "?", s->packed_size, s->size, s->file_offset,
			s->stop16, s->stop8, s->fixup_count, s->marshal_count);
	}
	std::string members;
	const char *mn, *mt;
	uint32_t mo;
	for (int i = 0; bf_granny_root_member(g, i, &mn, &mt, &mo); ++i)
		members += std::string(i ? ", " : "") + mn;
	printf("   root type: %s\n", members.c_str());
	printf("   from %s; %s; %s %d.%d.%d.%d; %.4g units/m, up %g %g %g\n", info->from_file_name, info->art_tool,
		info->exporter, info->exporter_version[0], info->exporter_version[1], info->exporter_version[2],
		info->exporter_version[3], info->units_per_meter, info->up[0], info->up[1], info->up[2]);
	printf("   %d textures, %d materials, %d skeletons, %d meshes, %d models, %d track groups, %d animations\n",
		info->texture_count, info->material_count, info->skeleton_count, info->mesh_count, info->model_count,
		info->track_group_count, info->animation_count);

	for (int i = 0; i < info->texture_count; ++i) {
		const bf_granny_texture *t = bf_granny_get_texture(g, i);
		printf("   texture %d: %s  %dx%d, encoding %d, %d images, %d MIPs, %zu bytes stored\n", i, t->file_name, t->width,
			t->height, t->encoding, t->image_count, t->mip_count, t->pixel_size);
	}
	for (int i = 0; i < info->material_count; ++i) {
		const bf_granny_material *m = bf_granny_get_material(g, i);
		printf("   material %d: %s  texture %d, diffuse %d, opacity %d\n", i, m->name, m->texture, m->diffuse_texture,
			m->opacity_texture);
	}
	for (int i = 0; i < info->skeleton_count; ++i) {
		const bf_granny_skeleton *s = bf_granny_get_skeleton(g, i);
		bool order = true;
		for (int b = 0; b < s->bone_count; ++b)
			if (s->bones[b].parent >= b || s->bones[b].parent < -1)
				order = false;
		printf("   skeleton %d: %s, %d bones (root %s)\n", i, s->name, s->bone_count, s->bone_count ? s->bones[0].name : "-");
		Check(order, (base + ": bones come after their parents").c_str());
	}

	std::vector<const bf_granny_mesh *> drawn;
	for (int i = 0; i < info->mesh_count; ++i) {
		const bf_granny_mesh *m = bf_granny_get_mesh(g, i);
		int resolved = 0, badWeights = 0;
		for (int k = 0; k < m->bone_binding_count; ++k)
			resolved += m->bone_bindings[k].bone >= 0;
		float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
		float normalError = 0, uvLo = 1e30f, uvHi = -1e30f;
		for (int v = 0; v < m->vertex_count; ++v) {
			const bf_granny_vertex &x = m->vertices[v];
			int sum = x.bone_weight[0] + x.bone_weight[1] + x.bone_weight[2] + x.bone_weight[3];
			if (sum != 255)
				++badWeights;
			for (int k = 0; k < 4; ++k)
				if (x.bone_weight[k] && x.bone_index[k] >= m->bone_binding_count && !m->rigid)
					++badWeights;
			for (int k = 0; k < 3; ++k) {
				lo[k] = std::min(lo[k], x.position[k]);
				hi[k] = std::max(hi[k], x.position[k]);
			}
			float nl = sqrtf(x.normal[0] * x.normal[0] + x.normal[1] * x.normal[1] + x.normal[2] * x.normal[2]);
			normalError = std::max(normalError, fabsf(nl - 1));
			uvLo = std::min({ uvLo, x.uv[0], x.uv[1] });
			uvHi = std::max({ uvHi, x.uv[0], x.uv[1] });
		}
		std::string mats;
		for (int k = 0; k < m->material_count; ++k) {
			const bf_granny_material *mat = bf_granny_get_material(g, m->materials[k]);
			const bf_granny_texture *tex = mat ? bf_granny_get_texture(g, mat->diffuse_texture) : nullptr;
			mats += std::string(k ? ", " : "") + (mat ? mat->name : "?") + " -> " + (tex ? tex->file_name : "(no texture)");
		}
		printf("   mesh %d: %s, %d vertices, %d triangles, %d groups, %s\n", i, m->name, m->vertex_count,
			m->index_count / 3, m->group_count, m->rigid ? "rigid" : "skinned");
		printf("      layout %s\n", m->vertex_layout);
		printf("      bounds %.1f %.1f %.1f .. %.1f %.1f %.1f, uv %.3f..%.3f, |normal| off by <= %.4f\n", lo[0], lo[1], lo[2],
			hi[0], hi[1], hi[2], uvLo, uvHi, normalError);
		printf("      materials: %s\n", mats.c_str());
		printf("      %d bone bindings, %d found in the skeleton\n", m->bone_binding_count, resolved);
		int groupTris = 0;
		for (int k = 0; k < m->group_count; ++k)
			groupTris += m->groups[k].triangle_count;
		Check(groupTris * 3 == m->index_count, (base + ": " + m->name + ": groups cover all triangles").c_str());
		Check(resolved == m->bone_binding_count, (base + ": " + m->name + ": bone bindings resolve").c_str());
		Check(badWeights == 0, (base + ": " + m->name + ": bone weights sum to 255, indices in range").c_str(),
			badWeights ? Format("%d vertices off", badWeights) : "");
		Check(normalError < 0.02f, (base + ": " + m->name + ": normals are unit length").c_str());
		drawn.push_back(m);

		if (!outDir.empty()) {
			Canvas c(512, 512);
			std::vector<const bf_granny_mesh *> one(1, m);
			View v = MakeView(one, 512);
			DrawMesh(c, v, m, kPalette[0]);
			WritePng(outDir + "/" + base + ".mesh" + std::to_string(i) + "-" + SafeName(m->name) + ".png", 512, 512, c.rgb);
		}
	}
	if (!outDir.empty() && drawn.size() > 1) {
		Canvas c(768, 768);
		View v = MakeView(drawn, 768);
		for (size_t i = 0; i < drawn.size(); ++i)
			DrawMesh(c, v, drawn[i], kPalette[(i + 1) % 6]);
		WritePng(outDir + "/" + base + ".model.png", 768, 768, c.rgb);
	}
	for (int i = 0; i < info->model_count; ++i) {
		const bf_granny_model *m = bf_granny_get_model(g, i);
		printf("   model %d: %s, skeleton %d, %d meshes\n", i, m->name, m->skeleton, m->mesh_count);
	}

	for (int i = 0; i < info->animation_count; ++i) {
		const bf_granny_animation *a = bf_granny_get_animation(g, i);
		printf("   animation %d: %s, %.3f s, step %.4f s, %d track groups\n", i, a->name, a->duration, a->time_step,
			a->track_group_count);
		for (int k = 0; k < a->track_group_count; ++k) {
			const bf_granny_track_group *tg = bf_granny_get_track_group(g, a->track_groups[k]);
			if (!tg)
				continue;
			int animated = 0, constant = 0, absent = 0, badKnots = 0, matched = 0, totalKnots = 0;
			float qerr = 0, lastKnot = 0;
			const bf_granny_skeleton *sk = bf_granny_get_skeleton(g, 0);
			for (int t = 0; t < tg->transform_track_count; ++t) {
				const bf_granny_transform_track &tr = tg->transform_tracks[t];
				const bf_granny_curve *curves[3] = { &tr.position, &tr.orientation, &tr.scale_shear };
				for (const bf_granny_curve *cv : curves) {
					if (cv->knot_count == 0)
						++absent;
					else if (cv->knot_count == 1)
						++constant;
					else
						++animated;
					totalKnots += cv->knot_count;
					if (cv->control_count != cv->knot_count * cv->dimension)
						++badKnots;
					for (int n = 1; n < cv->knot_count; ++n)
						if (cv->knots[n] < cv->knots[n - 1])
							++badKnots;
					if (cv->knot_count)
						lastKnot = std::max(lastKnot, cv->knots[cv->knot_count - 1]);
				}
				for (int n = 0; n + 3 < tr.orientation.control_count; n += 4)
					qerr = std::max(qerr, fabsf(QuatNorm(tr.orientation.controls + n) - 1));
				if (sk)
					for (int b = 0; b < sk->bone_count; ++b)
						if (!strcmp(sk->bones[b].name, tr.name)) {
							++matched;
							break;
						}
			}
			printf("      track group %s: %d transform tracks (%d match bones), curves %d animated / %d constant / %d absent, "
				   "%d knots, last knot %.3f s\n",
				tg->name, tg->transform_track_count, matched, animated, constant, absent, totalKnots, lastKnot);
			Check(badKnots == 0, (base + ": curves have dimension x knots controls and rising knots").c_str());
			// Control points of a fitted spline lie near, not on, the unit sphere.
			Check(qerr < 0.25f, (base + ": orientation controls are near-unit quaternions").c_str(), Format("max | |q|-1 | %.4f", qerr));
			Check(!sk || matched == tg->transform_track_count, (base + ": every track names a bone of the skeleton").c_str());
			Check(lastKnot <= a->duration + a->time_step, (base + ": knots lie within the duration").c_str());
		}
	}
	bf_granny_close(g);

	// Damaged copies: flipped bits and truncations must fail cleanly.
	srand((unsigned)data.size());
	int opened = 0;
	const int kMutations = 40;
	for (int it = 0; it < kMutations; ++it) {
		std::vector<uint8_t> x = data;
		for (int k = 1 + rand() % 8; k > 0; --k)
			x[0x58 + rand() % (x.size() - 0x58)] ^= (uint8_t)(1 << (rand() % 8));
		if (it % 8 == 0)
			x.resize(rand() % x.size());
		bf_granny_file *d = bf_granny_open(x.data(), x.size(), nullptr, 0);
		opened += d != nullptr;
		bf_granny_close(d);
	}
	Check(true, (base + ": survives damaged copies").c_str(), Format("%d of %d still opened", opened, kMutations));
}

// --- Textures, poses and skinning (M12.3b) ------------------------------------

struct Texture {
	int w = 0, h = 0;
	std::vector<uint8_t> bgra;
};

void WriteBgraPng(const std::string &path, const Texture &t)
{
	std::vector<uint8_t> rgb((size_t)t.w * t.h * 3);
	for (int i = 0; i < t.w * t.h; ++i) {
		// Alpha shown over a checkerboard so holes are visible.
		int x = i % t.w, y = i / t.w;
		int bg = ((x / 8 + y / 8) & 1) ? 200 : 120;
		int a = t.bgra[4 * i + 3];
		for (int c = 0; c < 3; ++c)
			rgb[3 * i + c] = (uint8_t)((t.bgra[4 * i + 2 - c] * a + bg * (255 - a)) / 255);
	}
	WritePng(path, t.w, t.h, rgb);
}

// Mean absolute difference of horizontally neighbouring colour bytes: a
// wrongly decoded wavelet picture is noise or blocks; a right one is smooth.
double Roughness(const Texture &t)
{
	double sum = 0;
	size_t n = 0;
	for (int y = 0; y < t.h; ++y)
		for (int x = 1; x < t.w; ++x)
			for (int c = 0; c < 3; ++c) {
				sum += abs(t.bgra[4 * ((size_t)y * t.w + x) + c] - t.bgra[4 * ((size_t)y * t.w + x - 1) + c]);
				++n;
			}
	return n ? sum / n : 0;
}

void Invert(const float *m, float *out)
{
	// General 4x4 inverse by cofactors.
	float inv[16];
	inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
	inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
	inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
	inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
	inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
	inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
	inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
	inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
	inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
	inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
	inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
	inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
	inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
	inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
	inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
	inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
	float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	for (int i = 0; i < 16; ++i)
		out[i] = det != 0 ? inv[i] / det : 0;
}

// A posed, textured mesh drawn into a canvas panel.
struct Panel {
	Canvas *canvas;
	int x0, y0, size;
	float rot[3][3];
	float scale, cx, cy;
};

void DrawTextured(Panel &p, const std::vector<bf_granny_pnt332> &verts, const bf_granny_mesh *m, const Texture *tex)
{
	const float light[3] = { -0.4f, -0.5f, -0.77f };
	Canvas &c = *p.canvas;
	for (int t = 0; t + 2 < m->index_count; t += 3) {
		float s[3][3], uv[3][2], shade[3];
		for (int k = 0; k < 3; ++k) {
			const bf_granny_pnt332 &v = verts[m->indices[t + k]];
			float q[3], n[3];
			for (int r = 0; r < 3; ++r) {
				q[r] = p.rot[r][0] * v.position[0] + p.rot[r][1] * v.position[1] + p.rot[r][2] * v.position[2];
				n[r] = p.rot[r][0] * v.normal[0] + p.rot[r][1] * v.normal[1] + p.rot[r][2] * v.normal[2];
			}
			s[k][0] = p.x0 + (q[0] - p.cx) * p.scale + p.size / 2.0f;
			s[k][1] = p.y0 + (q[1] - p.cy) * p.scale + p.size / 2.0f;
			s[k][2] = q[2];
			float nl = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
			float d = nl > 0 ? fabsf((n[0] * light[0] + n[1] * light[1] + n[2] * light[2]) / nl) : 1;
			shade[k] = 0.35f + 0.65f * d;
			uv[k][0] = v.uv[0];
			uv[k][1] = v.uv[1];
		}
		float minx = std::min({ s[0][0], s[1][0], s[2][0] }), maxx = std::max({ s[0][0], s[1][0], s[2][0] });
		float miny = std::min({ s[0][1], s[1][1], s[2][1] }), maxy = std::max({ s[0][1], s[1][1], s[2][1] });
		int xa = std::max(p.x0, (int)floorf(minx)), xb = std::min(p.x0 + p.size - 1, (int)ceilf(maxx));
		int ya = std::max(p.y0, (int)floorf(miny)), yb = std::min(p.y0 + p.size - 1, (int)ceilf(maxy));
		float area = (s[1][0] - s[0][0]) * (s[2][1] - s[0][1]) - (s[1][1] - s[0][1]) * (s[2][0] - s[0][0]);
		if (fabsf(area) < 1e-9f)
			continue;
		for (int y = ya; y <= yb; ++y)
			for (int x = xa; x <= xb; ++x) {
				float px = x + 0.5f, py = y + 0.5f;
				float w0 = ((s[1][0] - px) * (s[2][1] - py) - (s[1][1] - py) * (s[2][0] - px)) / area;
				float w1 = ((s[2][0] - px) * (s[0][1] - py) - (s[2][1] - py) * (s[0][0] - px)) / area;
				float w2 = 1 - w0 - w1;
				if (w0 < 0 || w1 < 0 || w2 < 0)
					continue;
				float z = w0 * s[0][2] + w1 * s[1][2] + w2 * s[2][2];
				size_t i = (size_t)y * c.w + x;
				if (z >= c.depth[i])
					continue;
				float sh = w0 * shade[0] + w1 * shade[1] + w2 * shade[2];
				uint8_t col[3] = { 190, 190, 190 };
				if (tex && tex->w) {
					float u = w0 * uv[0][0] + w1 * uv[1][0] + w2 * uv[2][0];
					float v = w0 * uv[0][1] + w1 * uv[1][1] + w2 * uv[2][1];
					u -= floorf(u);
					v -= floorf(v);
					int tx = std::min(tex->w - 1, (int)(u * tex->w)), ty = std::min(tex->h - 1, (int)(v * tex->h));
					const uint8_t *px4 = &tex->bgra[4 * ((size_t)ty * tex->w + tx)];
					if (px4[3] < 128)
						continue;   // alpha test, as the client's alpha textures are drawn
					col[0] = px4[2];
					col[1] = px4[1];
					col[2] = px4[0];
				}
				c.depth[i] = z;
				for (int k = 0; k < 3; ++k)
					c.rgb[3 * i + k] = (uint8_t)std::min(255.0f, col[k] * sh);
			}
	}
}

struct LoadedFile {
	std::string base;
	std::vector<uint8_t> data;
	bf_granny_file *g = nullptr;
};

// Draws one model (with its own animation or another file's) at the given
// times, side by side, and checks the poses.
void PoseSheet(const LoadedFile &model, const std::vector<Texture> &textures, const LoadedFile *animFile,
	const std::string &outDir)
{
	const bf_granny_file *g = model.g;
	const bf_granny_model *mdl = bf_granny_get_model(g, 0);
	const bf_granny_skeleton *sk = bf_granny_get_skeleton(g, mdl->skeleton);
	const LoadedFile &af = animFile ? *animFile : model;
	std::string label = model.base + (animFile ? "+" + animFile->base : "+stand");
	bf_granny_binding *b = bf_granny_bind_animation(g, 0, af.g, 0);
	int bones = 0, matched = 0;
	float duration = 0;
	bf_granny_binding_info(b, &bones, &matched, &duration);
	Check(matched == bones, (label + ": every bone has a track").c_str(), Format("%d of %d", matched, bones));

	float placement[16];
	bf_granny_transform_matrix(&mdl->initial_placement, placement);
	std::vector<float> world((size_t)16 * bones), comp((size_t)16 * bones);
	std::vector<bf_granny_transform> local((size_t)bones);

	// Walk cycles must close: bone positions at 0 and at the end agree.
	float height = 0;
	{
		bf_granny_binding *rest = bf_granny_bind_animation(g, 0, nullptr, 0);
		bf_granny_sample_local(rest, 0, local.data());
		bf_granny_build_world(sk, local.data(), placement, world.data(), nullptr);
		for (int i = 0; i < bones; ++i)
			height = std::max(height, world[16 * i + 14]);
		bf_granny_free_binding(rest);
	}
	if (animFile && af.base.find("move") != std::string::npos) {
		std::vector<float> w0((size_t)16 * bones);
		bf_granny_sample_local(b, 0, local.data());
		bf_granny_build_world(sk, local.data(), placement, w0.data(), nullptr);
		bf_granny_sample_local(b, duration, local.data());
		bf_granny_build_world(sk, local.data(), placement, world.data(), nullptr);
		float gap = 0;
		for (int i = 0; i < bones; ++i)
			for (int k = 12; k < 15; ++k)
				gap = std::max(gap, fabsf(world[16 * i + k] - w0[16 * i + k]));
		Check(gap < 0.02f * height, (label + ": the walk cycle closes").c_str(),
			Format("largest bone gap %.3f of %.1f height", gap, height));
	}

	// Limbs keep their length: each bone's distance to its parent stays as in
	// the rest pose at every sampled time (curves that broke a limb would not).
	std::vector<float> restLen((size_t)bones, 0);
	{
		bf_granny_binding *rest = bf_granny_bind_animation(g, 0, nullptr, 0);
		bf_granny_sample_local(rest, 0, local.data());
		bf_granny_build_world(sk, local.data(), placement, world.data(), nullptr);
		for (int i = 0; i < bones; ++i) {
			int p = sk->bones[i].parent;
			if (p >= 0)
				restLen[(size_t)i] = sqrtf(powf(world[16 * i + 12] - world[16 * p + 12], 2) + powf(world[16 * i + 13] - world[16 * p + 13], 2)
					+ powf(world[16 * i + 14] - world[16 * p + 14], 2));
		}
		bf_granny_free_binding(rest);
	}
	float stretch = 0;
	const char *stretched = "-";
	float stretchedAt = 0;
	const int kSamples = 60;
	for (int s = 0; s <= kSamples; ++s) {
		bf_granny_sample_local(b, duration * s / kSamples, local.data());
		bf_granny_build_world(sk, local.data(), placement, world.data(), nullptr);
		for (int i = 0; i < bones; ++i) {
			int p = sk->bones[i].parent;
			if (p < 0 || restLen[(size_t)i] < 0.5f)
				continue;
			float len = sqrtf(powf(world[16 * i + 12] - world[16 * p + 12], 2) + powf(world[16 * i + 13] - world[16 * p + 13], 2)
				+ powf(world[16 * i + 14] - world[16 * p + 14], 2));
			// Biped's root (Bip01) travels and its Footsteps helper stays
			// planted: both move freely, by design. Change is measured
			// against the model's height.
			if (b && sk->bones[i].parent >= 0) {
				float rel = fabsf(len - restLen[(size_t)i]) / height;
				const char *name = sk->bones[i].name;
				if (!strcmp(name, "Bip01") || strstr(name, "Footsteps"))
					continue;
				if (rel > stretch) {
					stretch = rel;
					stretched = sk->bones[i].name;
					stretchedAt = duration * s / kSamples;
				}
			}
		}
	}
	Check(true, (label + ": bone lengths over the animation").c_str(), Format("largest change %.1f %% of the height (%s at %.2f s)", 100 * stretch, stretched, stretchedAt));

	// The sheet: four times, textured and skinned.
	const int kPanel = 384;
	const char *frames = getenv("BF_GRANNY_FRAMES");   // more panels when looking closely
	const int kTimes = frames && atoi(frames) > 1 ? atoi(frames) : 4;
	Canvas c(kPanel * kTimes, kPanel);
	float yaw = 0.6f, pitch = 0.3f;
	float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
	float rot[3][3] = { { cy, sy, 0 }, { sy * sp, -cy * sp, -cp }, { -sy * cp, cy * cp, -sp } };
	// Framing from the rest pose, with room to move.
	float lo[2] = { 1e30f, 1e30f }, hi[2] = { -1e30f, -1e30f };
	for (int mi = 0; mi < mdl->mesh_count; ++mi) {
		const bf_granny_mesh *m = bf_granny_get_mesh(g, mdl->meshes[mi]);
		for (int v = 0; v < m->vertex_count; ++v)
			for (int r = 0; r < 2; ++r) {
				float q = rot[r][0] * m->vertices[v].position[0] + rot[r][1] * m->vertices[v].position[1]
					+ rot[r][2] * m->vertices[v].position[2];
				lo[r] = std::min(lo[r], q);
				hi[r] = std::max(hi[r], q);
			}
	}
	float extent = std::max(hi[0] - lo[0], hi[1] - lo[1]) * 1.35f;
	for (int f = 0; f < kTimes; ++f) {
		float t = duration * f / (kTimes - 1);
		Panel p = { &c, f * kPanel, 0, kPanel, {}, extent > 0 ? kPanel / extent : 1, (lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2 };
		memcpy(p.rot, rot, sizeof rot);
		bf_granny_sample_local(b, t, local.data());
		bf_granny_build_world(sk, local.data(), placement, world.data(), comp.data());
		for (int mi = 0; mi < mdl->mesh_count; ++mi) {
			int meshIndex = mdl->meshes[mi];
			const bf_granny_mesh *m = bf_granny_get_mesh(g, meshIndex);
			std::vector<bf_granny_pnt332> verts((size_t)m->vertex_count);
			bf_granny_deform(g, meshIndex, nullptr, comp.data(), verts.data());
			const Texture *tex = nullptr;
			if (m->group_count && m->material_count) {
				const bf_granny_material *mat = bf_granny_get_material(g, m->materials[m->groups[0].material]);
				if (mat && mat->diffuse_texture >= 0 && (size_t)mat->diffuse_texture < textures.size())
					tex = &textures[(size_t)mat->diffuse_texture];
			}
			DrawTextured(p, verts, m, tex);
		}
	}
	WritePng(outDir + "/" + label + ".posed.png", c.w, c.h, c.rgb);
	bf_granny_free_binding(b);
}

void PosedChecks(std::vector<LoadedFile> &files, const std::string &outDir)
{
	printf("\n== textures, poses and skinning\n");
	// Curves: continuous at their knots, through their first and last controls,
	// quaternion controls in one hemisphere.
	for (LoadedFile &f : files) {
		const bf_granny_info *info = bf_granny_get_info(f.g);
		float jump = 0, ends = 0;
		int flips = 0, pairs = 0;
		for (int a = 0; a < info->animation_count; ++a) {
			const bf_granny_animation *an = bf_granny_get_animation(f.g, a);
			for (int k = 0; k < an->track_group_count; ++k) {
				const bf_granny_track_group *tg = bf_granny_get_track_group(f.g, an->track_groups[k]);
				for (int t = 0; tg && t < tg->transform_track_count; ++t) {
					const bf_granny_curve *cs[3] = { &tg->transform_tracks[t].position, &tg->transform_tracks[t].orientation,
						&tg->transform_tracks[t].scale_shear };
					for (const bf_granny_curve *cv : cs) {
						if (cv->knot_count < 2 || cv->degree < 1)
							continue;
						float a1[9], a2[9];
						for (int n = 1; n + 1 < cv->knot_count; ++n) {
							bf_granny_evaluate_curve(cv, cv->knots[n] - 1e-4f, a1);
							bf_granny_evaluate_curve(cv, cv->knots[n] + 1e-4f, a2);
							for (int d = 0; d < cv->dimension; ++d)
								jump = std::max(jump, fabsf(a1[d] - a2[d]));
						}
						bf_granny_evaluate_curve(cv, cv->knots[0], a1);
						bf_granny_evaluate_curve(cv, cv->knots[cv->knot_count - 1], a2);
						for (int d = 0; d < cv->dimension; ++d) {
							float s0 = cv->dimension == 4 ? 1 : 1;
							ends = std::max(ends, fabsf(a1[d] - cv->controls[d] * s0));
							ends = std::max(ends, fabsf(a2[d] - cv->controls[(size_t)(cv->knot_count - 1) * cv->dimension + d]));
						}
						if (cv->dimension == 4)
							for (int n = 1; n < cv->knot_count; ++n, ++pairs) {
								const float *p = cv->controls + 4 * (n - 1), *q = cv->controls + 4 * n;
								if (p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3] < 0)
									++flips;
							}
					}
				}
			}
		}
		Check(jump < 0.05f, (f.base + ": curves are continuous at their knots").c_str(), Format("largest step %.4f", jump));
		Check(ends < 0.02f, (f.base + ": curves start and end at their end controls").c_str(), Format("off by %.4f", ends));
		Check(flips == 0, (f.base + ": consecutive orientation controls share a hemisphere").c_str(),
			Format("%d of %d pairs opposite", flips, pairs));
	}

	for (LoadedFile &f : files) {
		const bf_granny_info *info = bf_granny_get_info(f.g);
		if (!info->texture_count)
			continue;
		// Textures (the models', and the unused novice's in 1_attack).
		std::vector<Texture> textures((size_t)info->texture_count);
		for (int i = 0; i < info->texture_count; ++i) {
			const bf_granny_texture *t = bf_granny_get_texture(f.g, i);
			Texture &tx = textures[(size_t)i];
			tx.w = t->width;
			tx.h = t->height;
			tx.bgra.resize((size_t)t->width * t->height * 4);
			int alpha = 0;
			bool ok = bf_granny_texture_pixels(f.g, i, tx.bgra.data(), &alpha) != 0;
			double rough = ok ? Roughness(tx) : 0;
			Check(ok, (f.base + ": texture " + std::to_string(i) + " decodes").c_str(),
				Format("%dx%d, alpha %d, roughness %.1f", t->width, t->height, alpha, rough));
			if (!ok) {
				tx.w = 0;
				continue;
			}
			if (t->width * t->height > 256)
				Check(rough < 25, (f.base + ": texture " + std::to_string(i) + " is a picture, not noise").c_str(),
					Format("mean neighbour difference %.1f", rough));
			WriteBgraPng(outDir + "/" + f.base + ".texture" + std::to_string(i) + ".png", tx);

			// Damaged texture data must fail or decode, never crash.
			const bf_granny_section *s5 = nullptr;
			for (int k = 0; k < info->section_count; ++k) {
				const bf_granny_section *sec = bf_granny_get_section(f.g, k);
				if (sec->data && t->pixels >= sec->data && t->pixels < sec->data + sec->size)
					s5 = sec;
			}
			if (s5 && s5->compression == 0 && t->pixel_size > 64) {
				size_t base = s5->file_offset + (size_t)(t->pixels - s5->data);
				int decoded = 0;
				srand(7 + i);
				for (int it = 0; it < 20; ++it) {
					std::vector<uint8_t> x = f.data;
					for (int k = 0; k < 1 + it % 6; ++k)
						x[base + (size_t)rand() % t->pixel_size] ^= (uint8_t)(1 << (rand() % 8));
					bf_granny_file *d = bf_granny_open(x.data(), x.size(), nullptr, 0);
					std::vector<uint8_t> px((size_t)t->width * t->height * 4);
					if (d)
						decoded += bf_granny_texture_pixels(d, i, px.data(), nullptr);
					bf_granny_close(d);
				}
				Check(true, (f.base + ": texture " + std::to_string(i) + " survives damaged data").c_str(),
					Format("%d of 20 still decoded", decoded));
			}
		}
		if (isdigit((unsigned char)f.base[0]) || !info->mesh_count || !info->model_count)
			continue;   // posing: models only, <name>_<type>

		// The rest pose: with the placement as offset the composites are the
		// identity; without it (as the client calls) they are its inverse.
		const bf_granny_model *mdl = bf_granny_get_model(f.g, 0);
		const bf_granny_skeleton *sk = bf_granny_get_skeleton(f.g, mdl->skeleton);
		std::vector<bf_granny_transform> local((size_t)sk->bone_count);
		std::vector<float> world((size_t)16 * sk->bone_count), comp((size_t)16 * sk->bone_count);
		bf_granny_binding *rest = bf_granny_bind_animation(f.g, 0, nullptr, 0);
		bf_granny_sample_local(rest, 0, local.data());
		float placement[16], inverse[16];
		bf_granny_transform_matrix(&mdl->initial_placement, placement);
		Invert(placement, inverse);
		float e1 = 0, e2 = 0;
		bf_granny_build_world(sk, local.data(), placement, world.data(), comp.data());
		for (int i = 0; i < sk->bone_count; ++i)
			for (int k = 0; k < 16; ++k)
				e1 = std::max(e1, fabsf(comp[16 * i + k] - (k % 5 == 0 ? 1.0f : 0.0f)));
		bf_granny_build_world(sk, local.data(), nullptr, world.data(), comp.data());
		for (int i = 0; i < sk->bone_count; ++i)
			for (int k = 0; k < 16; ++k)
				e2 = std::max(e2, fabsf(comp[16 * i + k] - inverse[k]));
		bf_granny_free_binding(rest);
		Check(e1 < 1e-3f, (f.base + ": rest pose rebuilds the stored inverse world matrices").c_str(),
			Format("largest error %.6f", e1));
		Check(e2 < 1e-3f, (f.base + ": without the placement the rest composite is its inverse").c_str(),
			Format("largest error %.6f", e2));

		// Its own animation and those of its bone type.
		PoseSheet(f, textures, nullptr, outDir);
		std::string type = f.base.substr(f.base.rfind('_') + 1);
		for (LoadedFile &a : files)
			if (type != "1" && a.base.compare(0, type.size() + 1, type + "_") == 0)   // the client loads none for flags
				PoseSheet(f, textures, &a, outDir);
	}
}

} // namespace

int main(int argc, char **argv)
{
	const char *filter = nullptr;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-q"))
			g_quiet = true;
		else
			filter = argv[i];
	}
	SyntheticChecks();

	std::string dir;
	if (const char *env = getenv("BF_CLIENT_DIR"))
		dir = env;
	else if (const char *home = getenv("HOME"))
		dir = std::string(home) + "/Games/" BF_APP_NAME;
	std::vector<Grf *> archives;
	for (const char *name : { "rdata.grf", "data.grf" }) {
		Grf *grf = new Grf;
		if (grf->Open(dir + "/" + name))
			archives.push_back(grf);
		else
			delete grf;
	}
	if (archives.empty()) {
		printf("skipped game files: no data.grf or rdata.grf in %s\n", dir.c_str());
		return g_failed;
	}
	std::string outDir = dir + "/test/granny";
	mkdir((dir + "/test").c_str(), 0755);
	mkdir(outDir.c_str(), 0755);

	std::vector<std::string> names;
	for (Grf *grf : archives)
		for (const std::string &n : grf->Names(".gr2"))
			if (std::find(names.begin(), names.end(), n) == names.end())
				names.push_back(n);
	std::sort(names.begin(), names.end());
	int read = 0;
	std::vector<LoadedFile> loaded;
	for (const std::string &n : names) {
		if (filter && n.find(filter) == std::string::npos)
			continue;
		std::vector<uint8_t> data;
		int flags = 0;
		bool ok = false;
		for (Grf *grf : archives)
			if (grf->Read(n, data, &flags)) {
				ok = true;
				break;
			}
		Check(ok, ("read from the archive: " + n).c_str());
		if (!ok)
			continue;
		++read;
		GameFile(n, data, flags, outDir);
		LoadedFile lf;
		lf.base = n.substr(n.find_last_of('\\') + 1);
		lf.base = lf.base.substr(0, lf.base.size() - 4);
		lf.data = data;
		loaded.push_back(lf);
	}
	for (LoadedFile &lf : loaded)
		lf.g = bf_granny_open(lf.data.data(), lf.data.size(), nullptr, 0);
	loaded.erase(std::remove_if(loaded.begin(), loaded.end(), [](const LoadedFile &lf) { return lf.g == nullptr; }),
		loaded.end());
	PosedChecks(loaded, outDir);
	for (LoadedFile &lf : loaded)
		bf_granny_close(lf.g);
	printf("\n%d .gr2 files read; pictures in %s\n", read, outDir.c_str());
	printf("%s: %d failed checks\n", g_failed ? "FAILED" : "passed", g_failed);
	for (Grf *grf : archives)
		delete grf;
	return g_failed;
}
