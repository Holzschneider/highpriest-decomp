// Shaders of the gfx module. Compiled at run time from this source, which the
// build embeds into the library (the Metal compiler is not part of the
// command-line tools). Function constants specialise the fragment program to
// the texture-stage state; see bf_gfx.mm for the key.
#include <metal_stdlib>
using namespace metal;

// ---- pre-transformed vertices --------------------------------------------

struct VUniforms {
	float2 native_size;
	float scale;
	float pad;
};

struct TL1In {
	float4 pos      [[attribute(0)]];  // x, y, z, rhw
	float4 diffuse  [[attribute(1)]];
	float4 specular [[attribute(2)]];
	float2 uv0      [[attribute(3)]];
};

struct TL2In {
	float4 pos      [[attribute(0)]];
	float4 diffuse  [[attribute(1)]];
	float4 specular [[attribute(2)]];
	float2 uv0      [[attribute(3)]];
	float2 uv1      [[attribute(4)]];
};

struct VOut {
	float4 position [[position]];
	float4 diffuse;
	float4 specular;
	float2 uv0;
	float2 uv1;
};

static VOut tl_common(float4 p, float4 diffuse, float4 specular, float2 uv0, float2 uv1,
                      constant VUniforms &u)
{
	VOut o;
	float w = p.w > 0.0f ? 1.0f / p.w : 1.0f;
	// Direct3D 7 pixel centres are on integers, Metal's on halves: +0.5
	// native pixel after scaling (see bf_gfx.h, "Coordinates").
	float2 n = p.xy * u.scale + 0.5f;
	float2 ndc = float2(n.x / u.native_size.x * 2.0f - 1.0f, 1.0f - n.y / u.native_size.y * 2.0f);
	o.position = float4(ndc * w, p.z * w, w);
	o.diffuse = diffuse;
	o.specular = specular;
	o.uv0 = uv0;
	o.uv1 = uv1;
	return o;
}

vertex VOut vs_tl1(TL1In in [[stage_in]], constant VUniforms &u [[buffer(1)]])
{
	return tl_common(in.pos, in.diffuse, in.specular, in.uv0, float2(0.0f), u);
}

vertex VOut vs_tl2(TL2In in [[stage_in]], constant VUniforms &u [[buffer(1)]])
{
	return tl_common(in.pos, in.diffuse, in.specular, in.uv0, in.uv1, u);
}

// ---- texture stages -------------------------------------------------------

constant int s0_cop  [[function_constant(0)]];
constant int s0_ca1  [[function_constant(1)]];
constant int s0_ca2  [[function_constant(2)]];
constant int s0_aop  [[function_constant(3)]];
constant int s0_aa1  [[function_constant(4)]];
constant int s0_aa2  [[function_constant(5)]];
constant int s0_tci  [[function_constant(6)]];
constant int s1_cop  [[function_constant(7)]];
constant int s1_ca1  [[function_constant(8)]];
constant int s1_ca2  [[function_constant(9)]];
constant int s1_aop  [[function_constant(10)]];
constant int s1_aa1  [[function_constant(11)]];
constant int s1_aa2  [[function_constant(12)]];
constant int s1_tci  [[function_constant(13)]];

struct FUniforms {
	float4 tfactor;
	float4 fog_color;
	float scale;
	int alpha_test;
	int alpha_func;
	int alpha_ref;
	int fog;
	int specular;
	int snap0;
	int snap1;
};

enum { OP_DISABLE = 1, OP_SELECTARG1, OP_SELECTARG2, OP_MODULATE, OP_MODULATE2X, OP_MODULATE4X,
       OP_ADD, OP_ADDSIGNED, OP_ADDSIGNED2X, OP_SUBTRACT, OP_ADDSMOOTH, OP_BLENDDIFFUSEALPHA,
       OP_BLENDTEXTUREALPHA, OP_BLENDFACTORALPHA, OP_BLENDTEXTUREALPHAPM, OP_BLENDCURRENTALPHA,
       OP_PREMODULATE, OP_MODALPHA_ADDCOLOR, OP_MODCOLOR_ADDALPHA, OP_MODINVALPHA_ADDCOLOR,
       OP_MODINVCOLOR_ADDALPHA, OP_BUMPENVMAP, OP_BUMPENVMAPLUM, OP_DOTPRODUCT3 };

static float4 stage_arg(int a, float4 diffuse, float4 current, float4 tex, float4 tf, float4 spec)
{
	float4 v;
	switch (a & 0xf) {
	case 0:  v = diffuse; break;
	case 1:  v = current; break;
	case 2:  v = tex; break;
	case 3:  v = tf; break;
	case 4:  v = spec; break;
	default: v = current; break;
	}
	if (a & 0x10) v = 1.0f - v;
	if (a & 0x20) v = v.aaaa;
	return v;
}

// One operation on all four channels; the caller takes rgb or a.
static float4 stage_op(int op, float4 a1, float4 a2, float4 diffuse, float4 current, float4 tex,
                       float4 tf, bool alpha)
{
	switch (op) {
	case OP_SELECTARG1:        return a1;
	case OP_SELECTARG2:        return a2;
	case OP_MODULATE:          return a1 * a2;
	case OP_MODULATE2X:        return a1 * a2 * 2.0f;
	case OP_MODULATE4X:        return a1 * a2 * 4.0f;
	case OP_ADD:               return a1 + a2;
	case OP_ADDSIGNED:         return a1 + a2 - 0.5f;
	case OP_ADDSIGNED2X:       return (a1 + a2 - 0.5f) * 2.0f;
	case OP_SUBTRACT:          return a1 - a2;
	case OP_ADDSMOOTH:         return a1 + a2 - a1 * a2;
	case OP_BLENDDIFFUSEALPHA: return mix(a2, a1, diffuse.a);
	case OP_BLENDTEXTUREALPHA: return mix(a2, a1, tex.a);
	case OP_BLENDFACTORALPHA:  return mix(a2, a1, tf.a);
	case OP_BLENDTEXTUREALPHAPM: return a1 + a2 * (1.0f - tex.a);
	case OP_BLENDCURRENTALPHA: return mix(a2, a1, current.a);
	case OP_PREMODULATE:       return a1;
	case OP_MODALPHA_ADDCOLOR:    return alpha ? a1 * a2 : float4(a1.rgb + a1.a * a2.rgb, 0.0f);
	case OP_MODCOLOR_ADDALPHA:    return alpha ? a1 * a2 : float4(a1.rgb * a2.rgb + a1.a, 0.0f);
	case OP_MODINVALPHA_ADDCOLOR: return alpha ? a1 * a2 : float4((1.0f - a1.a) * a2.rgb + a1.rgb, 0.0f);
	case OP_MODINVCOLOR_ADDALPHA: return alpha ? a1 * a2 : float4((1.0f - a1.rgb) * a2.rgb + a1.a, 0.0f);
	case OP_DOTPRODUCT3:       return float4(4.0f * dot(a1.rgb - 0.5f, a2.rgb - 0.5f));
	default:                   return current;
	}
}

static bool alpha_compare(int func, int a, int ref)
{
	switch (func) {
	case 1:  return false;
	case 2:  return a <  ref;
	case 3:  return a == ref;
	case 4:  return a <= ref;
	case 5:  return a >  ref;
	case 6:  return a != ref;
	case 7:  return a >= ref;
	default: return true;
	}
}

static bool stage_on(int cop) { return cop != OP_DISABLE && cop != OP_BUMPENVMAP && cop != OP_BUMPENVMAPLUM; }

// Texture coordinates at the centre of the logical pixel the fragment lies
// in (see bf_gfx.h, "Sampling"): the fragment is j native pixels right of and
// below that pixel's first native pixel, whose attributes are what Direct3D
// 7 evaluated at scale 1.
static float2 snap_uv(float2 uv, float2 dx, float2 dy, float2 j)
{
	return uv - j.x * dx - j.y * dy;
}

fragment float4 fs_stages(VOut in [[stage_in]],
                          constant FUniforms &u [[buffer(0)]],
                          texture2d<float> t0 [[texture(0)]],
                          texture2d<float> t1 [[texture(1)]],
                          sampler sm0 [[sampler(0)]],
                          sampler sm1 [[sampler(1)]])
{
	float2 uv0 = s0_tci == 0 ? in.uv0 : in.uv1;
	float2 uv1 = s1_tci == 0 ? in.uv0 : in.uv1;
	float2 dx0 = dfdx(uv0), dy0 = dfdy(uv0);
	float2 dx1 = dfdx(uv1), dy1 = dfdy(uv1);
	if (u.scale > 1.0f) {
		float2 j = fmod(floor(in.position.xy), u.scale);
		if (u.snap0) uv0 = snap_uv(uv0, dx0, dy0, j);
		if (u.snap1) uv1 = snap_uv(uv1, dx1, dy1, j);
	}

	float4 diffuse = in.diffuse;
	float4 spec = in.specular;
	float4 tf = u.tfactor;
	float4 cur = diffuse;

	if (stage_on(s0_cop)) {
		float4 tex = u.snap0 ? t0.sample(sm0, uv0, level(0.0f)) : t0.sample(sm0, uv0);
		float4 c1 = stage_arg(s0_ca1, diffuse, cur, tex, tf, spec);
		float4 c2 = stage_arg(s0_ca2, diffuse, cur, tex, tf, spec);
		float3 rgb = stage_op(s0_cop, c1, c2, diffuse, cur, tex, tf, false).rgb;
		float a = cur.a;
		if (s0_aop != OP_DISABLE) {
			float4 x1 = stage_arg(s0_aa1, diffuse, cur, tex, tf, spec);
			float4 x2 = stage_arg(s0_aa2, diffuse, cur, tex, tf, spec);
			a = stage_op(s0_aop, x1, x2, diffuse, cur, tex, tf, true).a;
		}
		cur = saturate(float4(rgb, a));

		if (stage_on(s1_cop)) {
			float4 tex1 = u.snap1 ? t1.sample(sm1, uv1, level(0.0f)) : t1.sample(sm1, uv1);
			float4 d1 = stage_arg(s1_ca1, diffuse, cur, tex1, tf, spec);
			float4 d2 = stage_arg(s1_ca2, diffuse, cur, tex1, tf, spec);
			float3 rgb1 = stage_op(s1_cop, d1, d2, diffuse, cur, tex1, tf, false).rgb;
			float a1 = cur.a;
			if (s1_aop != OP_DISABLE) {
				float4 y1 = stage_arg(s1_aa1, diffuse, cur, tex1, tf, spec);
				float4 y2 = stage_arg(s1_aa2, diffuse, cur, tex1, tf, spec);
				a1 = stage_op(s1_aop, y1, y2, diffuse, cur, tex1, tf, true).a;
			}
			cur = saturate(float4(rgb1, a1));
		}
	}

	if (u.specular) cur.rgb = saturate(cur.rgb + spec.rgb);
	if (u.fog) cur.rgb = mix(u.fog_color.rgb, cur.rgb, spec.a);
	if (u.alpha_test) {
		int a8 = int(rint(cur.a * 255.0f));
		if (!alpha_compare(u.alpha_func, a8, u.alpha_ref)) discard_fragment();
	}
	return cur;
}

// ---- internal: clears, pixel writes, present -------------------------------

struct RectUniforms {
	float4 ndc;    // x0, y0, x1, y1
	float4 uv;     // u0, v0, u1, v1
	float4 color;
	float depth;
	int skip_transparent;
	float2 pad;
};

struct ROut {
	float4 position [[position]];
	float2 uv;
};

vertex ROut vs_rect(uint vid [[vertex_id]], constant RectUniforms &r [[buffer(0)]])
{
	float2 t = float2(float(vid & 1u), float(vid >> 1));
	ROut o;
	o.position = float4(mix(r.ndc.x, r.ndc.z, t.x), mix(r.ndc.y, r.ndc.w, t.y), r.depth, 1.0f);
	o.uv = float2(mix(r.uv.x, r.uv.z, t.x), mix(r.uv.y, r.uv.w, t.y));
	return o;
}

fragment float4 fs_fill(ROut in [[stage_in]], constant RectUniforms &r [[buffer(0)]])
{
	return r.color;
}

fragment float4 fs_copy(ROut in [[stage_in]], constant RectUniforms &r [[buffer(0)]],
                        texture2d<float> t [[texture(0)]])
{
	constexpr sampler s(filter::nearest, address::clamp_to_edge);
	float4 c = t.sample(s, in.uv);
	if (r.skip_transparent && c.a == 0.0f) discard_fragment();
	return float4(c.rgb, 1.0f);
}
