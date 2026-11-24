// Posing: curve evaluation, local transforms to world matrices, skinning.
// The conventions (knot layout, transform order, quaternion handedness) are
// established from the files themselves; see FORMAT.md, "Animations".

#include "bf_granny.h"

#include "bf_trace.h"

#include <math.h>
#include <string.h>

#include <vector>

namespace {

// The first knot after t, at most the last one.
int KnotAfter(const float *knots, int n, float t)
{
	for (int i = 0; i < n; ++i)
		if (knots[i] > t)
			return i;
	return n - 1;
}

float Ratio(float num, float den)
{
	return fabsf(den) < 1e-12f ? 0.0f : num / den;
}

void Normalize4(float *q)
{
	float l = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	if (l > 0)
		for (int k = 0; k < 4; ++k)
			q[k] /= l;
}

// Row-vector 4x4 product: out = a * b.
void Mul(const float *a, const float *b, float *out)
{
	float r[16];
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			r[4 * i + j] = a[4 * i] * b[j] + a[4 * i + 1] * b[4 + j] + a[4 * i + 2] * b[8 + j] + a[4 * i + 3] * b[12 + j];
	memcpy(out, r, sizeof r);
}

} // namespace

extern "C" int bf_granny_evaluate_curve(const bf_granny_curve *c, float t, float *out)
{
	if (!c || c->knot_count <= 0 || !c->knots || !c->controls || c->dimension <= 0
		|| c->control_count < c->knot_count * c->dimension)
		return 0;
	const int n = c->knot_count, d = c->dimension;
	auto P = [&](int i) { return c->controls + (size_t)(i < 0 ? 0 : i >= n ? n - 1 : i) * d; };
	auto K = [&](int i) { return c->knots[i < 0 ? 0 : i >= n ? n - 1 : i]; };
	int i = KnotAfter(c->knots, n, t);

	if (n == 1 || c->degree <= 0) {
		// A step: the control of the span t lies in.
		const float *p = P(n == 1 ? 0 : (c->knots[i] > t ? i - 1 : i));
		memcpy(out, p, sizeof(float) * d);
	} else if (c->degree == 1) {
		float b = Ratio(t - K(i - 1), K(i) - K(i - 1));
		const float *p0 = P(i - 1), *p1 = P(i);
		for (int k = 0; k < d; ++k)
			out[k] = p0[k] + (p1[k] - p0[k]) * b;
	} else {
		// Quadratic (higher degrees do not occur; they are evaluated as
		// quadratic): de Boor over controls i-2..i and knots i-2..i+1, indices
		// clamped to the stored arrays, which makes the curve start at the
		// first control and end at the last.
		float a1 = Ratio(t - K(i - 2), K(i) - K(i - 2));
		float a2 = Ratio(t - K(i - 1), K(i + 1) - K(i - 1));
		float b = Ratio(t - K(i - 1), K(i) - K(i - 1));
		float w0 = (1 - b) * (1 - a1), w2 = b * a2, w1 = 1 - w0 - w2;
		const float *p0 = P(i - 2), *p1 = P(i - 1), *p2 = P(i);
		for (int k = 0; k < d; ++k)
			out[k] = w0 * p0[k] + w1 * p1[k] + w2 * p2[k];
	}
	if (d == 4)
		Normalize4(out);
	return 1;
}

extern "C" void bf_granny_transform_matrix(const bf_granny_transform *t, float *m)
{
	// Granny quaternions are x, y, z, w. Linear part: rotation after
	// scale/shear (column vectors: R * S), stored transposed for row vectors.
	float x = t->orientation[0], y = t->orientation[1], z = t->orientation[2], w = t->orientation[3];
	float r[9] = {
		1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y),
		2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
		2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y),
	};
	const float *s = t->scale_shear;
	float c[9];
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			c[3 * i + j] = r[3 * i] * s[j] + r[3 * i + 1] * s[3 + j] + r[3 * i + 2] * s[6 + j];
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j)
			m[4 * i + j] = c[3 * j + i];
		m[4 * i + 3] = 0;
	}
	m[12] = t->position[0];
	m[13] = t->position[1];
	m[14] = t->position[2];
	m[15] = 1;
}

struct bf_granny_binding {
	const bf_granny_skeleton *skeleton = nullptr;
	const bf_granny_animation *animation = nullptr;
	std::vector<const bf_granny_transform_track *> tracks;   // per bone, or null
	int matched = 0;
};

extern "C" bf_granny_binding *bf_granny_bind_animation(const bf_granny_file *model_file, int model,
	const bf_granny_file *anim_file, int animation)
{
	const bf_granny_model *m = bf_granny_get_model(model_file, model);
	const bf_granny_skeleton *sk = m ? bf_granny_get_skeleton(model_file, m->skeleton) : nullptr;
	if (!sk) {
		BF_TRACE(BFT_ERR, "granny: bind: no model %d or no skeleton", model);
		return nullptr;
	}
	bf_granny_binding *b = new bf_granny_binding;
	b->skeleton = sk;
	b->tracks.assign((size_t)sk->bone_count, nullptr);
	const bf_granny_animation *a = anim_file ? bf_granny_get_animation(anim_file, animation) : nullptr;
	b->animation = a;
	if (a) {
		// Tracks bind to bones by name; the first track group is the model's
		// (each RO animation has one).
		for (int g = 0; g < a->track_group_count; ++g) {
			const bf_granny_track_group *tg = bf_granny_get_track_group(anim_file, a->track_groups[g]);
			if (!tg)
				continue;
			for (int t = 0; t < tg->transform_track_count; ++t)
				for (int bone = 0; bone < sk->bone_count; ++bone)
					if (!b->tracks[(size_t)bone] && !strcmp(sk->bones[bone].name, tg->transform_tracks[t].name)) {
						b->tracks[(size_t)bone] = &tg->transform_tracks[t];
						++b->matched;
						break;
					}
			break;
		}
	}
	return b;
}

extern "C" void bf_granny_free_binding(bf_granny_binding *binding)
{
	delete binding;
}

extern "C" int bf_granny_binding_info(const bf_granny_binding *b, int *bone_count, int *matched, float *duration)
{
	if (!b)
		return 0;
	if (bone_count) *bone_count = b->skeleton->bone_count;
	if (matched) *matched = b->matched;
	if (duration) *duration = b->animation ? b->animation->duration : 0;
	return 1;
}

extern "C" int bf_granny_sample_local(const bf_granny_binding *b, float t, bf_granny_transform *local)
{
	if (!b || !local)
		return 0;
	float duration = b->animation ? b->animation->duration : 0;
	if (t < 0) t = 0;
	if (t > duration) t = duration;
	for (int i = 0; i < b->skeleton->bone_count; ++i) {
		bf_granny_transform &out = local[i];
		out = b->skeleton->bones[i].local;
		if (!(out.flags & 4)) {
			static const float identity[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
			memcpy(out.scale_shear, identity, sizeof identity);
		}
		const bf_granny_transform_track *tr = b->tracks[(size_t)i];
		if (!tr)
			continue;
		if (bf_granny_evaluate_curve(&tr->position, t, out.position))
			out.flags |= 1;
		if (bf_granny_evaluate_curve(&tr->orientation, t, out.orientation))
			out.flags |= 2;
		if (bf_granny_evaluate_curve(&tr->scale_shear, t, out.scale_shear))
			out.flags |= 4;
	}
	return b->skeleton->bone_count;
}

extern "C" int bf_granny_build_world(const bf_granny_skeleton *sk, const bf_granny_transform *local, const float *offset,
	float *world, float *composite)
{
	if (!sk || !local || !world)
		return 0;
	for (int i = 0; i < sk->bone_count; ++i) {
		float m[16];
		bf_granny_transform t = local[i];
		if (!(t.flags & 4)) {
			static const float identity[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
			memcpy(t.scale_shear, identity, sizeof identity);
		}
		bf_granny_transform_matrix(&t, m);
		int parent = sk->bones[i].parent;
		if (parent >= 0 && parent < i)
			Mul(m, world + 16 * parent, world + 16 * i);
		else if (offset)
			Mul(m, offset, world + 16 * i);
		else
			memcpy(world + 16 * i, m, sizeof m);
		if (composite)
			Mul(sk->bones[i].inverse_world, world + 16 * i, composite + 16 * i);
	}
	return sk->bone_count;
}

extern "C" int bf_granny_sample(const bf_granny_binding *b, float t, float *world, float *composite)
{
	if (!b)
		return 0;
	std::vector<bf_granny_transform> local((size_t)b->skeleton->bone_count);
	if (!bf_granny_sample_local(b, t, local.data()))
		return 0;
	return bf_granny_build_world(b->skeleton, local.data(), nullptr, world, composite);
}

extern "C" int bf_granny_deform(const bf_granny_file *file, int mesh, const int *bones, const float *composite,
	bf_granny_pnt332 *out)
{
	const bf_granny_mesh *m = bf_granny_get_mesh(file, mesh);
	if (!m || !composite || !out)
		return 0;
	for (int v = 0; v < m->vertex_count; ++v) {
		const bf_granny_vertex &in = m->vertices[v];
		float p[3] = { 0, 0, 0 }, n[3] = { 0, 0, 0 };
		for (int k = 0; k < 4; ++k) {
			if (!in.bone_weight[k])
				continue;
			int binding = in.bone_index[k];
			if (binding >= m->bone_binding_count)
				continue;
			int bone = bones ? bones[binding] : m->bone_bindings[binding].bone;
			if (bone < 0)
				continue;
			const float *c = composite + 16 * bone;
			float w = in.bone_weight[k] / 255.0f;
			for (int j = 0; j < 3; ++j) {
				p[j] += w * (in.position[0] * c[j] + in.position[1] * c[4 + j] + in.position[2] * c[8 + j] + c[12 + j]);
				n[j] += w * (in.normal[0] * c[j] + in.normal[1] * c[4 + j] + in.normal[2] * c[8 + j]);
			}
		}
		memcpy(out[v].position, p, sizeof p);
		memcpy(out[v].normal, n, sizeof n);
		memcpy(out[v].uv, in.uv, sizeof in.uv);
	}
	return m->vertex_count;
}
