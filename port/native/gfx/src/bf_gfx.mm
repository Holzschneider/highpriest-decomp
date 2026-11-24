// gfx: the Metal renderer for the client's pre-transformed vertices.
// The contract is include/bf_gfx.h; the client's usage is D3D-USAGE.md.

#include "bf_gfx.h"
#include "bf_trace.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "bf_gfx_shaders.inc"   // k_bf_gfx_shader_source, generated from src/bf_gfx_shaders.metal

namespace {

const size_t kChunk = 1u << 20;          // upload chunk for vertices, indices, texels
const size_t kInlineBytes = 4096;        // setVertexBytes limit
const int kMaxInFlight = 3;
const MTLPixelFormat kColorFormat = MTLPixelFormatBGRA8Unorm;
const MTLPixelFormat kDepthFormat = MTLPixelFormatDepth32Float;

struct VUniforms { float native_size[2]; float scale; float pad; };
struct FUniforms {
	float tfactor[4];
	float fog_color[4];
	float scale;
	int alpha_test, alpha_func, alpha_ref, fog, specular, snap0, snap1;
};
struct RectUniforms { float ndc[4]; float uv[4]; float color[4]; float depth; int skip_transparent; float pad[2]; };

// Buffers handed to the GPU return to this pool when their command buffer
// completes. Shared with the completion handlers, so it may outlive the device.
struct Shared {
	std::mutex m;
	std::vector<id<MTLBuffer>> free_chunks;
	std::atomic<uint64_t> completed{0};
	dispatch_semaphore_t inflight = dispatch_semaphore_create(kMaxInFlight);
};

struct PipeKey {
	uint8_t vfmt;
	uint8_t c[14];
	uint8_t blend, src, dst;
	bool operator<(const PipeKey &o) const { return memcmp(this, &o, sizeof *this) < 0; }
};
struct FuncKey {
	uint8_t c[14];
	bool operator<(const FuncKey &o) const { return memcmp(c, o.c, sizeof c) < 0; }
};

void unpack(uint32_t argb, float out[4])
{
	out[0] = float((argb >> 16) & 0xff) / 255.0f;
	out[1] = float((argb >> 8) & 0xff) / 255.0f;
	out[2] = float(argb & 0xff) / 255.0f;
	out[3] = float(argb >> 24) / 255.0f;
}

bool trace_once(const char *what)
{
	static std::mutex m;
	static std::vector<std::string> seen;
	std::lock_guard<std::mutex> g(m);
	for (auto &s : seen) if (s == what) return false;
	seen.push_back(what);
	return true;
}

} // namespace

struct bf_gfx_texture {
	bf_gfx_device *dev = nullptr;
	id<MTLTexture> mtl;
	int w = 0, h = 0, mips = 1, bpp = 2;
	bf_gfx_format fmt = BF_GFX_FORMAT_A1R5G5B5;
	std::vector<uint8_t> shadow;
	int pitch = 0;
	uint64_t last_use = 0;       // serial of the last command buffer that read or wrote it
	int filter_override = 0;
	bool locked = false;
};

struct bf_gfx_device {
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
	id<MTLLibrary> library;
	CAMetalLayer *layer = nil;
	std::string name;

	int lw = 0, lh = 0, scale = 1, nw = 0, nh = 0;
	id<MTLTexture> target, depth, backup;
	bool backup_valid = false;
	bool needs_clear = true;

	id<MTLCommandBuffer> cb;
	id<MTLRenderCommandEncoder> rce;
	id<MTLBlitCommandEncoder> bce;
	uint64_t serial = 0;         // serial of cb while it is open
	std::shared_ptr<Shared> shared = std::make_shared<Shared>();
	id<MTLBuffer> chunk;
	size_t chunk_off = 0;
	NSMutableArray<id<MTLBuffer>> *used;

	// per-encoder cache
	id<MTLRenderPipelineState> cur_pipe;
	id<MTLDepthStencilState> cur_ds;
	int cur_cull = -1;

	bf_gfx_state state;
	bf_gfx_texture *tex[2] = {nullptr, nullptr};
	id<MTLTexture> white;

	std::map<PipeKey, id<MTLRenderPipelineState>> pipes;
	std::map<FuncKey, id<MTLFunction>> funcs;
	std::map<uint32_t, id<MTLDepthStencilState>> ds;
	std::map<uint64_t, id<MTLSamplerState>> samplers;
	id<MTLRenderPipelineState> fill_pipe[4];   // indexed by BF_GFX_CLEAR_* flags
	id<MTLRenderPipelineState> copy_pipe, present_pipe;
	id<MTLDepthStencilState> ds_off, ds_write_always;

	long long n_draws = 0, n_pipes = 0, n_readbacks = 0, n_frames = 0;
};

// ---- command buffers and encoders -----------------------------------------

static void end_encoders(bf_gfx_device *d)
{
	if (d->rce) { [d->rce endEncoding]; d->rce = nil; }
	if (d->bce) { [d->bce endEncoding]; d->bce = nil; }
}

static void ensure_cb(bf_gfx_device *d)
{
	if (d->cb) return;
	dispatch_semaphore_wait(d->shared->inflight, DISPATCH_TIME_FOREVER);
	d->cb = [d->queue commandBuffer];
	d->serial = d->serial + 1;
	d->used = [NSMutableArray array];
}

static id<MTLCommandBuffer> submit(bf_gfx_device *d)
{
	end_encoders(d);
	if (!d->cb) return nil;
	if (d->chunk) { [d->used addObject:d->chunk]; d->chunk = nil; d->chunk_off = 0; }
	std::shared_ptr<Shared> sh = d->shared;
	NSArray<id<MTLBuffer>> *used = d->used;
	uint64_t serial = d->serial;
	[d->cb addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
		if (buffer.status == MTLCommandBufferStatusError)
			bf_tracef(BFT_ERR, "gfx: command buffer failed: %s\n",
			          buffer.error.localizedDescription.UTF8String);
		{
			std::lock_guard<std::mutex> g(sh->m);
			for (id<MTLBuffer> b in used)
				if (b.length == kChunk && sh->free_chunks.size() < 64) sh->free_chunks.push_back(b);
		}
		uint64_t prev = sh->completed.load();
		while (prev < serial && !sh->completed.compare_exchange_weak(prev, serial)) {}
		dispatch_semaphore_signal(sh->inflight);
	}];
	id<MTLCommandBuffer> cb = d->cb;
	[cb commit];
	d->cb = nil;
	d->used = nil;
	return cb;
}

static void reset_encoder_cache(bf_gfx_device *d)
{
	d->cur_pipe = nil;
	d->cur_ds = nil;
	d->cur_cull = -1;
}

static void ensure_render(bf_gfx_device *d)
{
	ensure_cb(d);
	if (d->bce) { [d->bce endEncoding]; d->bce = nil; }
	if (d->rce) return;
	MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
	rp.colorAttachments[0].texture = d->target;
	rp.colorAttachments[0].storeAction = MTLStoreActionStore;
	rp.depthAttachment.texture = d->depth;
	rp.depthAttachment.storeAction = MTLStoreActionStore;
	if (d->needs_clear) {
		rp.colorAttachments[0].loadAction = MTLLoadActionClear;
		rp.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
		rp.depthAttachment.loadAction = MTLLoadActionClear;
		rp.depthAttachment.clearDepth = 1.0;
		d->needs_clear = false;
	} else {
		rp.colorAttachments[0].loadAction = MTLLoadActionLoad;
		rp.depthAttachment.loadAction = MTLLoadActionLoad;
	}
	d->rce = [d->cb renderCommandEncoderWithDescriptor:rp];
	reset_encoder_cache(d);
}

static void ensure_blit(bf_gfx_device *d)
{
	ensure_cb(d);
	if (d->rce) { [d->rce endEncoding]; d->rce = nil; }
	if (!d->bce) d->bce = [d->cb blitCommandEncoder];
}

// Space for size bytes in a GPU-visible buffer that lives until the current
// command buffer completes.
static bool upload_alloc(bf_gfx_device *d, size_t size, id<MTLBuffer> *buf, size_t *off)
{
	ensure_cb(d);
	size_t at = (d->chunk_off + 255) & ~size_t(255);
	if (!d->chunk || at + size > d->chunk.length) {
		if (d->chunk) [d->used addObject:d->chunk];
		d->chunk = nil;
		if (size <= kChunk) {
			std::lock_guard<std::mutex> g(d->shared->m);
			if (!d->shared->free_chunks.empty()) {
				d->chunk = d->shared->free_chunks.back();
				d->shared->free_chunks.pop_back();
			}
		}
		if (!d->chunk)
			d->chunk = [d->device newBufferWithLength:std::max(size, kChunk) options:MTLResourceStorageModeShared];
		if (!d->chunk) {
			bf_tracef(BFT_ERR, "gfx: cannot allocate an upload buffer of %zu bytes\n", size);
			return false;
		}
		at = 0;
	}
	*buf = d->chunk;
	*off = at;
	d->chunk_off = at + size;
	return true;
}

// ---- pipelines and states ---------------------------------------------------

static bool stage_on(int op) { return op != BF_GFX_OP_DISABLE && op != 22 && op != 23; }

static bool op_uses_arg1(int op) { return op != BF_GFX_OP_SELECT_ARG2; }
static bool op_uses_arg2(int op) { return op != BF_GFX_OP_SELECT_ARG1 && op != BF_GFX_OP_PREMODULATE; }

static bool valid_op(int op) { return (op >= 1 && op <= 21) || op == 22 || op == 23 || op == 24; }

static void stage_key(const bf_gfx_stage_state &s, uint8_t *c)
{
	int cop = valid_op(s.color_op) ? s.color_op : BF_GFX_OP_DISABLE;
	int aop = valid_op(s.alpha_op) ? s.alpha_op : BF_GFX_OP_DISABLE;
	memset(c, 0, 7);
	if (!stage_on(cop)) { c[0] = BF_GFX_OP_DISABLE; return; }
	c[0] = uint8_t(cop);
	c[1] = op_uses_arg1(cop) ? (s.color_arg1 & 0x3f) : 0;
	c[2] = op_uses_arg2(cop) ? (s.color_arg2 & 0x3f) : 0;
	if (aop == 22 || aop == 23) aop = BF_GFX_OP_DISABLE;
	c[3] = uint8_t(aop);
	if (aop != BF_GFX_OP_DISABLE) {
		c[4] = op_uses_arg1(aop) ? (s.alpha_arg1 & 0x3f) : 0;
		c[5] = op_uses_arg2(aop) ? (s.alpha_arg2 & 0x3f) : 0;
	}
	c[6] = s.texcoord_index ? 1 : 0;
}

static void state_key(const bf_gfx_state &st, uint8_t c[14])
{
	stage_key(st.stage[0], c);
	if (c[0] == BF_GFX_OP_DISABLE) { memset(c + 7, 0, 7); c[7] = BF_GFX_OP_DISABLE; }
	else stage_key(st.stage[1], c + 7);
}

static MTLBlendFactor blend_factor(int f, bool is_src, int other)
{
	switch (f) {
	case BF_GFX_BLEND_ZERO:          return MTLBlendFactorZero;
	case BF_GFX_BLEND_ONE:           return MTLBlendFactorOne;
	case BF_GFX_BLEND_SRC_COLOR:     return MTLBlendFactorSourceColor;
	case BF_GFX_BLEND_INV_SRC_COLOR: return MTLBlendFactorOneMinusSourceColor;
	case BF_GFX_BLEND_SRC_ALPHA:     return MTLBlendFactorSourceAlpha;
	case BF_GFX_BLEND_INV_SRC_ALPHA: return MTLBlendFactorOneMinusSourceAlpha;
	case BF_GFX_BLEND_DST_ALPHA:     return MTLBlendFactorOne;    // target alpha is 1
	case BF_GFX_BLEND_INV_DST_ALPHA: return MTLBlendFactorZero;
	case BF_GFX_BLEND_DST_COLOR:     return MTLBlendFactorDestinationColor;
	case BF_GFX_BLEND_INV_DST_COLOR: return MTLBlendFactorOneMinusDestinationColor;
	case BF_GFX_BLEND_SRC_ALPHA_SAT: return MTLBlendFactorSourceAlphaSaturated;
	case BF_GFX_BLEND_BOTH_SRC_ALPHA:
		return is_src ? MTLBlendFactorSourceAlpha : MTLBlendFactorOneMinusSourceAlpha;
	case BF_GFX_BLEND_BOTH_INV_SRC_ALPHA:
		return is_src ? MTLBlendFactorOneMinusSourceAlpha : MTLBlendFactorSourceAlpha;
	default:
		(void)other;
		return is_src ? MTLBlendFactorOne : MTLBlendFactorZero;
	}
}

static MTLCompareFunction compare_fn(int f)
{
	switch (f) {
	case BF_GFX_CMP_NEVER:         return MTLCompareFunctionNever;
	case BF_GFX_CMP_LESS:          return MTLCompareFunctionLess;
	case BF_GFX_CMP_EQUAL:         return MTLCompareFunctionEqual;
	case BF_GFX_CMP_LESS_EQUAL:    return MTLCompareFunctionLessEqual;
	case BF_GFX_CMP_GREATER:       return MTLCompareFunctionGreater;
	case BF_GFX_CMP_NOT_EQUAL:     return MTLCompareFunctionNotEqual;
	case BF_GFX_CMP_GREATER_EQUAL: return MTLCompareFunctionGreaterEqual;
	default:                       return MTLCompareFunctionAlways;
	}
}

static MTLVertexDescriptor *vertex_descriptor(int vfmt)
{
	MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
	vd.attributes[0].format = MTLVertexFormatFloat4;                   vd.attributes[0].offset = 0;
	vd.attributes[1].format = MTLVertexFormatUChar4Normalized_BGRA;    vd.attributes[1].offset = 16;
	vd.attributes[2].format = MTLVertexFormatUChar4Normalized_BGRA;    vd.attributes[2].offset = 20;
	vd.attributes[3].format = MTLVertexFormatFloat2;                   vd.attributes[3].offset = 24;
	if (vfmt == BF_GFX_VERTEX_TL2) {
		vd.attributes[4].format = MTLVertexFormatFloat2;               vd.attributes[4].offset = 32;
	}
	for (int i = 0; i < (vfmt == BF_GFX_VERTEX_TL2 ? 5 : 4); ++i) vd.attributes[i].bufferIndex = 0;
	vd.layouts[0].stride = vfmt == BF_GFX_VERTEX_TL2 ? 40 : 32;
	return vd;
}

static id<MTLFunction> stage_function(bf_gfx_device *d, const uint8_t c[14])
{
	FuncKey k;
	memcpy(k.c, c, 14);
	auto it = d->funcs.find(k);
	if (it != d->funcs.end()) return it->second;
	MTLFunctionConstantValues *cv = [MTLFunctionConstantValues new];
	for (int i = 0; i < 14; ++i) {
		int v = c[i];
		[cv setConstantValue:&v type:MTLDataTypeInt atIndex:NSUInteger(i)];
	}
	NSError *err = nil;
	id<MTLFunction> f = [d->library newFunctionWithName:@"fs_stages" constantValues:cv error:&err];
	if (!f) bf_tracef(BFT_ERR, "gfx: fragment program failed: %s\n", err.localizedDescription.UTF8String);
	d->funcs[k] = f;
	return f;
}

static id<MTLRenderPipelineState> make_pipe(bf_gfx_device *d, MTLRenderPipelineDescriptor *pd)
{
	NSError *err = nil;
	id<MTLRenderPipelineState> p = [d->device newRenderPipelineStateWithDescriptor:pd error:&err];
	if (!p) bf_tracef(BFT_ERR, "gfx: pipeline failed: %s\n", err.localizedDescription.UTF8String);
	else d->n_pipes++;
	return p;
}

static id<MTLRenderPipelineState> draw_pipe(bf_gfx_device *d, int vfmt)
{
	const bf_gfx_state &st = d->state;
	PipeKey k;
	memset(&k, 0, sizeof k);
	k.vfmt = uint8_t(vfmt);
	state_key(st, k.c);
	if (st.blend_enable) {
		k.blend = 1;
		k.src = st.src_blend;
		k.dst = (st.src_blend == BF_GFX_BLEND_BOTH_SRC_ALPHA || st.src_blend == BF_GFX_BLEND_BOTH_INV_SRC_ALPHA)
			? 0 : st.dst_blend;
	}
	auto it = d->pipes.find(k);
	if (it != d->pipes.end()) return it->second;

	id<MTLFunction> fs = stage_function(d, k.c);
	if (!fs) { d->pipes[k] = nil; return nil; }
	MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
	pd.vertexFunction = [d->library newFunctionWithName:(vfmt == BF_GFX_VERTEX_TL2 ? @"vs_tl2" : @"vs_tl1")];
	pd.fragmentFunction = fs;
	pd.vertexDescriptor = vertex_descriptor(vfmt);
	pd.colorAttachments[0].pixelFormat = kColorFormat;
	pd.colorAttachments[0].writeMask = MTLColorWriteMaskRed | MTLColorWriteMaskGreen | MTLColorWriteMaskBlue;
	pd.depthAttachmentPixelFormat = kDepthFormat;
	pd.inputPrimitiveTopology = MTLPrimitiveTopologyClassUnspecified;
	if (k.blend) {
		int src = k.src, dst = k.dst;
		if (!(src >= 1 && src <= 13)) {
			if (trace_once("blend-src")) bf_tracef(BFT_GFX, "gfx: unknown source blend %d, using ONE\n", src);
			src = BF_GFX_BLEND_ONE;
		}
		if (src == BF_GFX_BLEND_BOTH_SRC_ALPHA || src == BF_GFX_BLEND_BOTH_INV_SRC_ALPHA) dst = src;
		else if (!(dst >= 1 && dst <= 13)) {
			if (trace_once("blend-dst")) bf_tracef(BFT_GFX, "gfx: unknown destination blend %d, using ZERO\n", dst);
			dst = BF_GFX_BLEND_ZERO;
		}
		MTLRenderPipelineColorAttachmentDescriptor *ca = pd.colorAttachments[0];
		ca.blendingEnabled = YES;
		ca.rgbBlendOperation = MTLBlendOperationAdd;
		ca.alphaBlendOperation = MTLBlendOperationAdd;
		ca.sourceRGBBlendFactor = blend_factor(src, true, dst);
		ca.destinationRGBBlendFactor = blend_factor(dst, false, src);
		ca.sourceAlphaBlendFactor = ca.sourceRGBBlendFactor;
		ca.destinationAlphaBlendFactor = ca.destinationRGBBlendFactor;
	}
	id<MTLRenderPipelineState> p = make_pipe(d, pd);
	d->pipes[k] = p;
	return p;
}

static id<MTLDepthStencilState> depth_state(bf_gfx_device *d, bool enable, bool write, int func)
{
	uint32_t key = enable ? (1u | (write ? 2u : 0u) | (uint32_t(func & 15) << 4)) : 0u;
	auto it = d->ds.find(key);
	if (it != d->ds.end()) return it->second;
	MTLDepthStencilDescriptor *dd = [MTLDepthStencilDescriptor new];
	dd.depthCompareFunction = enable ? compare_fn(func) : MTLCompareFunctionAlways;
	dd.depthWriteEnabled = enable && write;
	id<MTLDepthStencilState> s = [d->device newDepthStencilStateWithDescriptor:dd];
	d->ds[key] = s;
	return s;
}

static MTLSamplerAddressMode address_mode(int a)
{
	switch (a) {
	case BF_GFX_ADDRESS_MIRROR: return MTLSamplerAddressModeMirrorRepeat;
	case BF_GFX_ADDRESS_CLAMP:  return MTLSamplerAddressModeClampToEdge;
	case BF_GFX_ADDRESS_BORDER: return MTLSamplerAddressModeClampToBorderColor;
	default:                    return MTLSamplerAddressModeRepeat;
	}
}

static id<MTLSamplerState> sampler(bf_gfx_device *d, int minf, int magf, int mip, int au, int av, uint32_t border)
{
	int b = (border >> 24) < 0x80 ? 0 : ((border & 0xffffff) >= 0x800000 ? 2 : 1);
	uint64_t key = uint64_t(minf & 3) | uint64_t(magf & 3) << 2 | uint64_t(mip & 3) << 4 |
	               uint64_t(au & 7) << 6 | uint64_t(av & 7) << 9 | uint64_t(b) << 12;
	auto it = d->samplers.find(key);
	if (it != d->samplers.end()) return it->second;
	MTLSamplerDescriptor *sd = [MTLSamplerDescriptor new];
	sd.minFilter = minf == BF_GFX_FILTER_LINEAR ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
	sd.magFilter = magf == BF_GFX_FILTER_LINEAR ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
	sd.mipFilter = mip == BF_GFX_MIP_LINEAR ? MTLSamplerMipFilterLinear
	             : mip == BF_GFX_MIP_NEAREST ? MTLSamplerMipFilterNearest : MTLSamplerMipFilterNotMipmapped;
	sd.sAddressMode = address_mode(au);
	sd.tAddressMode = address_mode(av);
	sd.borderColor = b == 0 ? MTLSamplerBorderColorTransparentBlack
	               : b == 1 ? MTLSamplerBorderColorOpaqueBlack : MTLSamplerBorderColorOpaqueWhite;
	id<MTLSamplerState> s = [d->device newSamplerStateWithDescriptor:sd];
	d->samplers[key] = s;
	return s;
}

static bool build_internal(bf_gfx_device *d)
{
	id<MTLFunction> vs = [d->library newFunctionWithName:@"vs_rect"];
	id<MTLFunction> fill = [d->library newFunctionWithName:@"fs_fill"];
	id<MTLFunction> copy = [d->library newFunctionWithName:@"fs_copy"];
	if (!vs || !fill || !copy) return false;
	for (int flags = 1; flags <= 3; ++flags) {
		MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
		pd.vertexFunction = vs;
		pd.fragmentFunction = fill;
		pd.colorAttachments[0].pixelFormat = kColorFormat;
		pd.colorAttachments[0].writeMask = (flags & BF_GFX_CLEAR_COLOR)
			? (MTLColorWriteMaskRed | MTLColorWriteMaskGreen | MTLColorWriteMaskBlue) : MTLColorWriteMaskNone;
		pd.depthAttachmentPixelFormat = kDepthFormat;
		if (!(d->fill_pipe[flags] = make_pipe(d, pd))) return false;
	}
	MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
	pd.vertexFunction = vs;
	pd.fragmentFunction = copy;
	pd.colorAttachments[0].pixelFormat = kColorFormat;
	pd.colorAttachments[0].writeMask = MTLColorWriteMaskRed | MTLColorWriteMaskGreen | MTLColorWriteMaskBlue;
	pd.depthAttachmentPixelFormat = kDepthFormat;
	if (!(d->copy_pipe = make_pipe(d, pd))) return false;
	MTLRenderPipelineDescriptor *pp = [MTLRenderPipelineDescriptor new];
	pp.vertexFunction = vs;
	pp.fragmentFunction = copy;
	pp.colorAttachments[0].pixelFormat = kColorFormat;
	if (!(d->present_pipe = make_pipe(d, pp))) return false;
	d->ds_off = depth_state(d, false, false, BF_GFX_CMP_ALWAYS);
	d->ds_write_always = depth_state(d, true, true, BF_GFX_CMP_ALWAYS);
	return true;
}

// ---- target ------------------------------------------------------------------

static bool make_target(bf_gfx_device *d)
{
	MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kColorFormat
		width:NSUInteger(d->nw) height:NSUInteger(d->nh) mipmapped:NO];
	td.storageMode = MTLStorageModePrivate;
	td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
	d->target = [d->device newTextureWithDescriptor:td];
	MTLTextureDescriptor *dd = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kDepthFormat
		width:NSUInteger(d->nw) height:NSUInteger(d->nh) mipmapped:NO];
	dd.storageMode = MTLStorageModePrivate;
	dd.usage = MTLTextureUsageRenderTarget;
	d->depth = [d->device newTextureWithDescriptor:dd];
	d->backup = nil;
	d->backup_valid = false;
	d->needs_clear = true;
	if (!d->target || !d->depth) {
		bf_tracef(BFT_ERR, "gfx: cannot create a %dx%d target\n", d->nw, d->nh);
		return false;
	}
	if (d->layer) d->layer.drawableSize = CGSizeMake(d->nw, d->nh);
	ensure_render(d);   // clears
	return true;
}

static bool size_ok(int lw, int lh, int scale)
{
	return scale >= 1 && scale <= 3 && lw >= 1 && lh >= 1 && lw * scale <= 16384 && lh * scale <= 16384;
}

// ---- public: device ---------------------------------------------------------------

extern "C" bf_gfx_device *bf_gfx_create(const bf_gfx_desc *desc)
{
	if (!desc || !size_ok(desc->logical_width, desc->logical_height, desc->ui_scale)) {
		bf_tracef(BFT_ERR, "gfx: create: bad size or scale\n");
		return nullptr;
	}
	@autoreleasepool {
		id<MTLDevice> device = MTLCreateSystemDefaultDevice();
		if (!device) { bf_tracef(BFT_ERR, "gfx: no Metal device\n"); return nullptr; }
		bf_gfx_device *d = new bf_gfx_device;
		d->device = device;
		d->name = device.name.UTF8String;
		d->queue = [device newCommandQueue];
		MTLCompileOptions *opt = [MTLCompileOptions new];
		if (@available(macOS 15.0, *)) opt.mathMode = MTLMathModeSafe;
		else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			opt.fastMathEnabled = NO;
#pragma clang diagnostic pop
		}
		NSError *err = nil;
		d->library = [device newLibraryWithSource:@(k_bf_gfx_shader_source) options:opt error:&err];
		if (!d->library || !build_internal(d)) {
			bf_tracef(BFT_ERR, "gfx: shader library failed: %s\n",
			          err ? err.localizedDescription.UTF8String : "internal pipelines");
			delete d;
			return nullptr;
		}
		MTLTextureDescriptor *wd = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kColorFormat
			width:1 height:1 mipmapped:NO];
		wd.storageMode = MTLStorageModeShared;
		d->white = [device newTextureWithDescriptor:wd];
		uint32_t px = 0xffffffffu;
		[d->white replaceRegion:MTLRegionMake2D(0, 0, 1, 1) mipmapLevel:0 withBytes:&px bytesPerRow:4];

		if (desc->metal_layer) {
			CAMetalLayer *layer = (__bridge CAMetalLayer *)desc->metal_layer;
			layer.device = device;
			layer.pixelFormat = kColorFormat;
			layer.framebufferOnly = YES;
			CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
			layer.colorspace = cs;
			CGColorSpaceRelease(cs);
			d->layer = layer;
		}
		bf_gfx_state_init(&d->state);
		d->lw = desc->logical_width;
		d->lh = desc->logical_height;
		d->scale = desc->ui_scale;
		d->nw = d->lw * d->scale;
		d->nh = d->lh * d->scale;
		if (!make_target(d)) { delete d; return nullptr; }
		BF_TRACE(BFT_GFX, "gfx: device %s, logical %dx%d, scale %d, %s\n", d->name.c_str(),
		         d->lw, d->lh, d->scale, d->layer ? "layer" : "offscreen");
		return d;
	}
}

extern "C" void bf_gfx_destroy(bf_gfx_device *d)
{
	if (!d) return;
	@autoreleasepool {
		bf_gfx_finish(d);
		delete d;
	}
}

extern "C" int bf_gfx_resize(bf_gfx_device *d, int lw, int lh, int scale)
{
	if (!d || !size_ok(lw, lh, scale)) return BF_GFX_E_ARG;
	@autoreleasepool {
		submit(d);
		d->lw = lw; d->lh = lh; d->scale = scale;
		d->nw = lw * scale; d->nh = lh * scale;
		return make_target(d) ? BF_GFX_OK : BF_GFX_E_MEMORY;
	}
}

extern "C" void bf_gfx_get_size(const bf_gfx_device *d, int *lw, int *lh, int *scale, int *nw, int *nh)
{
	if (!d) return;
	if (lw) *lw = d->lw;
	if (lh) *lh = d->lh;
	if (scale) *scale = d->scale;
	if (nw) *nw = d->nw;
	if (nh) *nh = d->nh;
}

extern "C" int bf_gfx_max_texture_size(const bf_gfx_device *) { return 16384; }

extern "C" const char *bf_gfx_device_name(const bf_gfx_device *d) { return d ? d->name.c_str() : ""; }

// ---- public: textures --------------------------------------------------------------

extern "C" int bf_gfx_format_bytes(bf_gfx_format f)
{
	switch (f) {
	case BF_GFX_FORMAT_A1R5G5B5: case BF_GFX_FORMAT_A4R4G4B4: case BF_GFX_FORMAT_R5G6B5: return 2;
	case BF_GFX_FORMAT_A8R8G8B8: return 4;
	default: return 0;
	}
}

static inline uint32_t to_argb(bf_gfx_format f, const uint8_t *p)
{
	uint32_t v;
	switch (f) {
	case BF_GFX_FORMAT_A1R5G5B5: {
		v = uint32_t(p[0]) | uint32_t(p[1]) << 8;
		uint32_t r = (v >> 10) & 31, g = (v >> 5) & 31, b = v & 31;
		return ((v & 0x8000) ? 0xff000000u : 0u) | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
	}
	case BF_GFX_FORMAT_A4R4G4B4: {
		v = uint32_t(p[0]) | uint32_t(p[1]) << 8;
		return (((v >> 12) & 15) * 17) << 24 | (((v >> 8) & 15) * 17) << 16 | (((v >> 4) & 15) * 17) << 8 | (v & 15) * 17;
	}
	case BF_GFX_FORMAT_R5G6B5: {
		v = uint32_t(p[0]) | uint32_t(p[1]) << 8;
		uint32_t r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
		return 0xff000000u | ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
	}
	default:
		return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
	}
}

static inline void from_argb(bf_gfx_format f, uint32_t c, uint8_t *p)
{
	uint32_t a = c >> 24, r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff, v;
	switch (f) {
	case BF_GFX_FORMAT_A1R5G5B5: v = (a >= 0x80 ? 0x8000u : 0u) | (r >> 3) << 10 | (g >> 3) << 5 | (b >> 3); break;
	case BF_GFX_FORMAT_A4R4G4B4: v = (a >> 4) << 12 | (r >> 4) << 8 | (g >> 4) << 4 | (b >> 4); break;
	case BF_GFX_FORMAT_R5G6B5:   v = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3); break;
	default:
		p[0] = uint8_t(b); p[1] = uint8_t(g); p[2] = uint8_t(r); p[3] = uint8_t(a);
		return;
	}
	p[0] = uint8_t(v); p[1] = uint8_t(v >> 8);
}

static bool clip_rect(int w, int h, const bf_gfx_rect *in, bf_gfx_rect *out)
{
	bf_gfx_rect r = in ? *in : bf_gfx_rect{0, 0, w, h};
	int x0 = std::max(r.x, 0), y0 = std::max(r.y, 0);
	int x1 = std::min(r.x + r.w, w), y1 = std::min(r.y + r.h, h);
	if (r.w <= 0 || r.h <= 0 || x1 <= x0 || y1 <= y0) return false;
	*out = bf_gfx_rect{x0, y0, x1 - x0, y1 - y0};
	return true;
}

// Sends a rectangle of the CPU copy to the GPU texture, ordered with draws.
static int upload(bf_gfx_texture *t, const bf_gfx_rect &r)
{
	bf_gfx_device *d = t->dev;
	size_t row = size_t(r.w) * 4, size = row * size_t(r.h);
	bool direct = t->last_use <= d->shared->completed.load() && !(d->cb && t->last_use == d->serial);
	std::vector<uint8_t> tmp;
	uint8_t *dst;
	id<MTLBuffer> buf = nil;
	size_t off = 0;
	if (direct) {
		tmp.resize(size);
		dst = tmp.data();
	} else {
		if (!upload_alloc(d, size, &buf, &off)) return BF_GFX_E_MEMORY;
		dst = (uint8_t *)buf.contents + off;
	}
	for (int y = 0; y < r.h; ++y) {
		const uint8_t *s = t->shadow.data() + size_t(r.y + y) * size_t(t->pitch) + size_t(r.x) * size_t(t->bpp);
		uint32_t *o = (uint32_t *)(dst + size_t(y) * row);
		if (t->fmt == BF_GFX_FORMAT_A8R8G8B8) memcpy(o, s, row);
		else for (int x = 0; x < r.w; ++x) o[x] = to_argb(t->fmt, s + x * 2);
	}
	if (direct) {
		[t->mtl replaceRegion:MTLRegionMake2D(NSUInteger(r.x), NSUInteger(r.y), NSUInteger(r.w), NSUInteger(r.h))
		          mipmapLevel:0 withBytes:dst bytesPerRow:row];
	} else {
		ensure_blit(d);
		[d->bce copyFromBuffer:buf sourceOffset:off sourceBytesPerRow:row sourceBytesPerImage:size
		            sourceSize:MTLSizeMake(NSUInteger(r.w), NSUInteger(r.h), 1)
		             toTexture:t->mtl destinationSlice:0 destinationLevel:0
		     destinationOrigin:MTLOriginMake(NSUInteger(r.x), NSUInteger(r.y), 0)];
		t->last_use = d->serial;
	}
	if (t->mips > 1) {
		ensure_blit(d);
		[d->bce generateMipmapsForTexture:t->mtl];
		t->last_use = d->serial;
	}
	return BF_GFX_OK;
}

extern "C" bf_gfx_texture *bf_gfx_texture_create(bf_gfx_device *d, int w, int h, bf_gfx_format fmt, int mips)
{
	int bpp = bf_gfx_format_bytes(fmt);
	if (!d || w < 1 || h < 1 || w > 16384 || h > 16384 || !bpp || mips < 0) {
		bf_tracef(BFT_ERR, "gfx: texture_create: bad arguments %dx%d format %d\n", w, h, int(fmt));
		return nullptr;
	}
	@autoreleasepool {
		int full = 1;
		while ((std::max(w, h) >> full) > 0) ++full;
		int levels = mips == 0 ? full : std::min(mips, full);
		MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kColorFormat
			width:NSUInteger(w) height:NSUInteger(h) mipmapped:levels > 1];
		td.mipmapLevelCount = NSUInteger(levels);
		td.storageMode = MTLStorageModeShared;
		td.usage = MTLTextureUsageShaderRead;
		id<MTLTexture> m = [d->device newTextureWithDescriptor:td];
		if (!m) {
			bf_tracef(BFT_ERR, "gfx: cannot create a %dx%d texture\n", w, h);
			return nullptr;
		}
		bf_gfx_texture *t = new bf_gfx_texture;
		t->dev = d;
		t->mtl = m;
		t->w = w; t->h = h; t->mips = levels; t->fmt = fmt; t->bpp = bpp;
		t->pitch = w * bpp;
		t->shadow.assign(size_t(t->pitch) * size_t(h), 0);
		std::vector<uint8_t> zero(size_t(w) * 4 * size_t(h), 0);
		for (int l = 0; l < levels; ++l) {
			int lw = std::max(w >> l, 1), lh = std::max(h >> l, 1);
			[m replaceRegion:MTLRegionMake2D(0, 0, NSUInteger(lw), NSUInteger(lh)) mipmapLevel:NSUInteger(l)
			       withBytes:zero.data() bytesPerRow:NSUInteger(lw) * 4];
		}
		return t;
	}
}

extern "C" void bf_gfx_texture_destroy(bf_gfx_texture *t)
{
	if (!t) return;
	bf_gfx_device *d = t->dev;
	for (int i = 0; i < 2; ++i) if (d->tex[i] == t) d->tex[i] = nullptr;
	delete t;   // a command buffer still using the MTLTexture keeps it alive
}

extern "C" void bf_gfx_texture_get_info(const bf_gfx_texture *t, int *w, int *h, bf_gfx_format *f, int *mips)
{
	if (!t) return;
	if (w) *w = t->w;
	if (h) *h = t->h;
	if (f) *f = t->fmt;
	if (mips) *mips = t->mips;
}

extern "C" int bf_gfx_texture_update(bf_gfx_texture *t, int x, int y, int w, int h, const void *pixels, int pitch)
{
	if (!t || !pixels) return BF_GFX_E_ARG;
	bf_gfx_rect want{x, y, w, h}, r;
	if (!clip_rect(t->w, t->h, &want, &r)) return w > 0 && h > 0 ? BF_GFX_E_ARG : BF_GFX_OK;
	if (pitch == 0) pitch = w * t->bpp;
	const uint8_t *src = (const uint8_t *)pixels + size_t(r.y - y) * size_t(pitch) + size_t(r.x - x) * size_t(t->bpp);
	for (int j = 0; j < r.h; ++j)
		memcpy(t->shadow.data() + size_t(r.y + j) * size_t(t->pitch) + size_t(r.x) * size_t(t->bpp),
		       src + size_t(j) * size_t(pitch), size_t(r.w) * size_t(t->bpp));
	@autoreleasepool { return upload(t, r); }
}

extern "C" void *bf_gfx_texture_lock(bf_gfx_texture *t, int *pitch)
{
	if (!t || t->locked) return nullptr;
	t->locked = true;
	if (pitch) *pitch = t->pitch;
	return t->shadow.data();
}

extern "C" int bf_gfx_texture_unlock(bf_gfx_texture *t, const bf_gfx_rect *changed)
{
	if (!t || !t->locked) return BF_GFX_E_STATE;
	t->locked = false;
	bf_gfx_rect r;
	if (changed && (changed->w <= 0 || changed->h <= 0)) return BF_GFX_OK;
	if (!clip_rect(t->w, t->h, changed, &r)) return BF_GFX_OK;
	@autoreleasepool { return upload(t, r); }
}

extern "C" int bf_gfx_texture_fill(bf_gfx_texture *t, const bf_gfx_rect *rect, uint32_t value)
{
	if (!t) return BF_GFX_E_ARG;
	bf_gfx_rect r;
	if (!clip_rect(t->w, t->h, rect, &r)) return BF_GFX_OK;
	for (int y = 0; y < r.h; ++y) {
		uint8_t *p = t->shadow.data() + size_t(r.y + y) * size_t(t->pitch) + size_t(r.x) * size_t(t->bpp);
		for (int x = 0; x < r.w; ++x, p += t->bpp) {
			if (t->bpp == 2) { p[0] = uint8_t(value); p[1] = uint8_t(value >> 8); }
			else { p[0] = uint8_t(value); p[1] = uint8_t(value >> 8); p[2] = uint8_t(value >> 16); p[3] = uint8_t(value >> 24); }
		}
	}
	@autoreleasepool { return upload(t, r); }
}

extern "C" int bf_gfx_texture_copy(bf_gfx_texture *dst, const bf_gfx_rect *dr, const bf_gfx_texture *src, const bf_gfx_rect *sr)
{
	if (!dst || !src) return BF_GFX_E_ARG;
	bf_gfx_rect d0 = dr ? *dr : bf_gfx_rect{0, 0, dst->w, dst->h};
	bf_gfx_rect s0 = sr ? *sr : bf_gfx_rect{0, 0, src->w, src->h};
	if (d0.w <= 0 || d0.h <= 0 || s0.w <= 0 || s0.h <= 0) return BF_GFX_E_ARG;
	bool changed = false;
	int minx = dst->w, miny = dst->h, maxx = 0, maxy = 0;
	for (int j = 0; j < d0.h; ++j) {
		int dy = d0.y + j, sy = s0.y + int((long long)j * s0.h / d0.h);
		if (dy < 0 || dy >= dst->h || sy < 0 || sy >= src->h) continue;
		for (int i = 0; i < d0.w; ++i) {
			int dx = d0.x + i, sx = s0.x + int((long long)i * s0.w / d0.w);
			if (dx < 0 || dx >= dst->w || sx < 0 || sx >= src->w) continue;
			const uint8_t *sp = src->shadow.data() + size_t(sy) * size_t(src->pitch) + size_t(sx) * size_t(src->bpp);
			uint8_t *dp = dst->shadow.data() + size_t(dy) * size_t(dst->pitch) + size_t(dx) * size_t(dst->bpp);
			if (src->fmt == dst->fmt) memcpy(dp, sp, size_t(dst->bpp));
			else from_argb(dst->fmt, to_argb(src->fmt, sp), dp);
			changed = true;
			minx = std::min(minx, dx); miny = std::min(miny, dy);
			maxx = std::max(maxx, dx); maxy = std::max(maxy, dy);
		}
	}
	if (!changed) return BF_GFX_OK;
	@autoreleasepool { return upload(dst, bf_gfx_rect{minx, miny, maxx - minx + 1, maxy - miny + 1}); }
}

extern "C" void bf_gfx_texture_set_filter_override(bf_gfx_texture *t, int filter)
{
	if (t) t->filter_override = (filter == BF_GFX_FILTER_NEAREST || filter == BF_GFX_FILTER_LINEAR) ? filter : 0;
}

// ---- public: state ------------------------------------------------------------------

extern "C" void bf_gfx_state_init(bf_gfx_state *s)
{
	if (!s) return;
	memset(s, 0, sizeof *s);
	for (int i = 0; i < 2; ++i) {
		bf_gfx_stage_state &t = s->stage[i];
		t.color_op = i == 0 ? BF_GFX_OP_MODULATE : BF_GFX_OP_DISABLE;
		t.color_arg1 = BF_GFX_ARG_TEXTURE;
		t.color_arg2 = BF_GFX_ARG_CURRENT;
		t.alpha_op = i == 0 ? BF_GFX_OP_SELECT_ARG1 : BF_GFX_OP_DISABLE;
		t.alpha_arg1 = BF_GFX_ARG_TEXTURE;
		t.alpha_arg2 = BF_GFX_ARG_CURRENT;
		t.texcoord_index = uint8_t(i);
		t.address_u = t.address_v = BF_GFX_ADDRESS_WRAP;
		t.mag_filter = t.min_filter = BF_GFX_FILTER_NEAREST;
		t.mip_filter = BF_GFX_MIP_NONE;
		t.border_color = 0;
	}
	s->blend_enable = 0;
	s->src_blend = BF_GFX_BLEND_ONE;
	s->dst_blend = BF_GFX_BLEND_ZERO;
	s->alpha_test_enable = 0;
	s->alpha_func = BF_GFX_CMP_ALWAYS;
	s->alpha_ref = 0;
	s->depth_enable = 1;
	s->depth_write = 1;
	s->depth_func = BF_GFX_CMP_LESS_EQUAL;
	s->cull = BF_GFX_CULL_CCW;
	s->fog_enable = 0;
	s->specular_enable = 0;
	s->fog_color = 0;
	s->texture_factor = 0xffffffffu;
}

extern "C" void bf_gfx_set_state(bf_gfx_device *d, const bf_gfx_state *s)
{
	if (d && s) d->state = *s;
}

extern "C" void bf_gfx_set_texture(bf_gfx_device *d, int stage, bf_gfx_texture *t)
{
	if (d && (stage == 0 || stage == 1)) d->tex[stage] = t;
}

// ---- public: drawing ----------------------------------------------------------------

static void apply_cull(bf_gfx_device *d, int cull)
{
	if (cull == d->cur_cull) return;
	d->cur_cull = cull;
	// Metal decides the winding in window coordinates, whose y runs down
	// like the client's screen: clockwise on screen is clockwise for Metal.
	[d->rce setFrontFacingWinding:MTLWindingClockwise];
	if (cull == BF_GFX_CULL_CW) [d->rce setCullMode:MTLCullModeFront];
	else if (cull == BF_GFX_CULL_CCW) [d->rce setCullMode:MTLCullModeBack];
	else [d->rce setCullMode:MTLCullModeNone];
}

static void bind_stage(bf_gfx_device *d, int i, int *snap)
{
	const bf_gfx_stage_state &s = d->state.stage[i];
	bf_gfx_texture *t = d->tex[i];
	int minf = s.min_filter, magf = s.mag_filter, mip = s.mip_filter;
	if (t && t->filter_override) minf = magf = t->filter_override;
	if (!t || t->mips <= 1) mip = BF_GFX_MIP_NONE;
	*snap = magf == BF_GFX_FILTER_NEAREST ? 1 : 0;
	[d->rce setFragmentTexture:(t ? t->mtl : d->white) atIndex:NSUInteger(i)];
	[d->rce setFragmentSamplerState:sampler(d, minf, magf, mip, s.address_u, s.address_v, s.border_color)
	                        atIndex:NSUInteger(i)];
	if (t) t->last_use = d->serial;
}

static int draw_common(bf_gfx_device *d, bf_gfx_prim prim, bf_gfx_vertex_format fmt,
                       const void *verts, int vcount, const uint16_t *indices, int icount)
{
	if (!d || !verts || vcount < 0 || (fmt != BF_GFX_VERTEX_TL1 && fmt != BF_GFX_VERTEX_TL2)) return BF_GFX_E_ARG;
	if (prim < BF_GFX_PRIM_POINT_LIST || prim > BF_GFX_PRIM_TRIANGLE_FAN) return BF_GFX_E_ARG;
	if (indices && icount < 0) return BF_GFX_E_ARG;
	int n = indices ? icount : vcount;
	int minimum = prim == BF_GFX_PRIM_POINT_LIST ? 1 : prim <= BF_GFX_PRIM_LINE_STRIP ? 2 : 3;
	if (n < minimum || vcount == 0) return BF_GFX_OK;
	@autoreleasepool {
		ensure_render(d);
		id<MTLRenderPipelineState> pipe = draw_pipe(d, fmt);
		if (!pipe) return BF_GFX_E_DEVICE;
		if (pipe != d->cur_pipe) { [d->rce setRenderPipelineState:pipe]; d->cur_pipe = pipe; }
		const bf_gfx_state &st = d->state;
		id<MTLDepthStencilState> dss = depth_state(d, st.depth_enable != 0, st.depth_write != 0, st.depth_func);
		if (dss != d->cur_ds) { [d->rce setDepthStencilState:dss]; d->cur_ds = dss; }
		apply_cull(d, st.cull);

		VUniforms vu = {{float(d->nw), float(d->nh)}, float(d->scale), 0};
		[d->rce setVertexBytes:&vu length:sizeof vu atIndex:1];
		size_t stride = fmt == BF_GFX_VERTEX_TL2 ? 40 : 32;
		size_t bytes = stride * size_t(vcount);
		if (bytes <= kInlineBytes) {
			[d->rce setVertexBytes:verts length:bytes atIndex:0];
		} else {
			id<MTLBuffer> buf; size_t off;
			if (!upload_alloc(d, bytes, &buf, &off)) return BF_GFX_E_MEMORY;
			memcpy((uint8_t *)buf.contents + off, verts, bytes);
			[d->rce setVertexBuffer:buf offset:off atIndex:0];
		}

		FUniforms fu;
		memset(&fu, 0, sizeof fu);
		unpack(st.texture_factor, fu.tfactor);
		unpack(st.fog_color, fu.fog_color);
		fu.scale = float(d->scale);
		fu.alpha_test = st.alpha_test_enable ? 1 : 0;
		fu.alpha_func = st.alpha_func;
		fu.alpha_ref = st.alpha_ref;
		fu.fog = st.fog_enable ? 1 : 0;
		fu.specular = st.specular_enable ? 1 : 0;
		bind_stage(d, 0, &fu.snap0);
		bind_stage(d, 1, &fu.snap1);
		[d->rce setFragmentBytes:&fu length:sizeof fu atIndex:0];

		MTLPrimitiveType mp;
		switch (prim) {
		case BF_GFX_PRIM_POINT_LIST: mp = MTLPrimitiveTypePoint; break;
		case BF_GFX_PRIM_LINE_LIST:  mp = MTLPrimitiveTypeLine; break;
		case BF_GFX_PRIM_LINE_STRIP: mp = MTLPrimitiveTypeLineStrip; break;
		case BF_GFX_PRIM_TRIANGLE_STRIP: mp = MTLPrimitiveTypeTriangleStrip; break;
		default: mp = MTLPrimitiveTypeTriangle; break;
		}

		std::vector<uint16_t> fan;
		const uint16_t *idx = indices;
		int count = n;
		if (prim == BF_GFX_PRIM_TRIANGLE_FAN) {
			fan.reserve(size_t(n - 2) * 3);
			for (int i = 1; i + 1 < n; ++i) {
				fan.push_back(indices ? indices[0] : 0);
				fan.push_back(indices ? indices[i] : uint16_t(i));
				fan.push_back(indices ? indices[i + 1] : uint16_t(i + 1));
			}
			idx = fan.data();
			count = int(fan.size());
		}
		if (idx) {
			id<MTLBuffer> ib; size_t ioff;
			size_t ibytes = size_t(count) * 2;
			if (!upload_alloc(d, ibytes, &ib, &ioff)) return BF_GFX_E_MEMORY;
			memcpy((uint8_t *)ib.contents + ioff, idx, ibytes);
			[d->rce drawIndexedPrimitives:mp indexCount:NSUInteger(count) indexType:MTLIndexTypeUInt16
			                  indexBuffer:ib indexBufferOffset:ioff];
		} else {
			[d->rce drawPrimitives:mp vertexStart:0 vertexCount:NSUInteger(count)];
		}
		d->n_draws++;
		return BF_GFX_OK;
	}
}

extern "C" int bf_gfx_draw(bf_gfx_device *d, bf_gfx_prim prim, bf_gfx_vertex_format fmt, const void *verts, int vcount)
{
	return draw_common(d, prim, fmt, verts, vcount, nullptr, 0);
}

extern "C" int bf_gfx_draw_indexed(bf_gfx_device *d, bf_gfx_prim prim, bf_gfx_vertex_format fmt,
                                   const void *verts, int vcount, const uint16_t *indices, int icount)
{
	if (!indices) return BF_GFX_E_ARG;
	return draw_common(d, prim, fmt, verts, vcount, indices, icount);
}

static void native_rect_ndc(bf_gfx_device *d, const bf_gfx_rect &r, float ndc[4])
{
	ndc[0] = float(r.x) / float(d->nw) * 2.0f - 1.0f;
	ndc[1] = 1.0f - float(r.y) / float(d->nh) * 2.0f;
	ndc[2] = float(r.x + r.w) / float(d->nw) * 2.0f - 1.0f;
	ndc[3] = 1.0f - float(r.y + r.h) / float(d->nh) * 2.0f;
}

static bool native_rect(bf_gfx_device *d, bf_gfx_space space, const bf_gfx_rect *rect, bf_gfx_rect *out)
{
	if (!rect) { *out = bf_gfx_rect{0, 0, d->nw, d->nh}; return true; }
	bf_gfx_rect r = *rect;
	if (space == BF_GFX_SPACE_LOGICAL) r = bf_gfx_rect{r.x * d->scale, r.y * d->scale, r.w * d->scale, r.h * d->scale};
	return clip_rect(d->nw, d->nh, &r, out);
}

extern "C" int bf_gfx_clear(bf_gfx_device *d, unsigned flags, uint32_t color, float depth, const bf_gfx_rect *rect)
{
	if (!d || (flags & ~3u)) return BF_GFX_E_ARG;
	if (!flags) return BF_GFX_OK;
	bf_gfx_rect r;
	if (!native_rect(d, BF_GFX_SPACE_LOGICAL, rect, &r)) return BF_GFX_OK;
	@autoreleasepool {
		ensure_render(d);
		[d->rce setRenderPipelineState:d->fill_pipe[flags]];
		[d->rce setDepthStencilState:(flags & BF_GFX_CLEAR_DEPTH) ? d->ds_write_always : d->ds_off];
		[d->rce setCullMode:MTLCullModeNone];
		reset_encoder_cache(d);
		[d->rce setScissorRect:MTLScissorRect{NSUInteger(r.x), NSUInteger(r.y), NSUInteger(r.w), NSUInteger(r.h)}];
		RectUniforms ru;
		memset(&ru, 0, sizeof ru);
		ru.ndc[0] = -1; ru.ndc[1] = 1; ru.ndc[2] = 1; ru.ndc[3] = -1;
		unpack(color, ru.color);
		ru.color[3] = 1;
		ru.depth = std::min(std::max(depth, 0.0f), 1.0f);
		[d->rce setVertexBytes:&ru length:sizeof ru atIndex:0];
		[d->rce setFragmentBytes:&ru length:sizeof ru atIndex:0];
		[d->rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
		[d->rce setScissorRect:MTLScissorRect{0, 0, NSUInteger(d->nw), NSUInteger(d->nh)}];
		return BF_GFX_OK;
	}
}

// ---- public: frames ----------------------------------------------------------------

extern "C" int bf_gfx_begin_frame(bf_gfx_device *d) { return d ? BF_GFX_OK : BF_GFX_E_ARG; }

extern "C" int bf_gfx_end_frame(bf_gfx_device *d)
{
	if (!d) return BF_GFX_E_ARG;
	@autoreleasepool { submit(d); }
	return BF_GFX_OK;
}

extern "C" int bf_gfx_present(bf_gfx_device *d)
{
	if (!d) return BF_GFX_E_ARG;
	@autoreleasepool {
		d->n_frames++;
		if (!d->layer) { submit(d); return BF_GFX_E_STATE; }
		id<CAMetalDrawable> drawable = [d->layer nextDrawable];
		if (!drawable) {
			submit(d);
			if (trace_once("no-drawable")) bf_tracef(BFT_GFX, "gfx: no drawable within the timeout\n");
			return BF_GFX_E_TIMEOUT;
		}
		ensure_cb(d);
		end_encoders(d);
		MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
		rp.colorAttachments[0].texture = drawable.texture;
		rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;
		rp.colorAttachments[0].storeAction = MTLStoreActionStore;
		id<MTLRenderCommandEncoder> e = [d->cb renderCommandEncoderWithDescriptor:rp];
		[e setRenderPipelineState:d->present_pipe];
		RectUniforms ru;
		memset(&ru, 0, sizeof ru);
		ru.ndc[0] = -1; ru.ndc[1] = 1; ru.ndc[2] = 1; ru.ndc[3] = -1;
		ru.uv[0] = 0; ru.uv[1] = 0; ru.uv[2] = 1; ru.uv[3] = 1;
		[e setVertexBytes:&ru length:sizeof ru atIndex:0];
		[e setFragmentBytes:&ru length:sizeof ru atIndex:0];
		[e setFragmentTexture:d->target atIndex:0];
		[e drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
		[e endEncoding];
		[d->cb presentDrawable:drawable];
		submit(d);
		return BF_GFX_OK;
	}
}

extern "C" void bf_gfx_set_vsync(bf_gfx_device *d, int enabled)
{
	if (d && d->layer) d->layer.displaySyncEnabled = enabled ? YES : NO;
}

extern "C" int bf_gfx_finish(bf_gfx_device *d)
{
	if (!d) return BF_GFX_E_ARG;
	@autoreleasepool {
		id<MTLCommandBuffer> cb = submit(d);
		if (!cb) {
			// Commit an empty buffer to wait for everything queued before it.
			ensure_cb(d);
			cb = submit(d);
		}
		[cb waitUntilCompleted];
	}
	return BF_GFX_OK;
}

// ---- public: read-back and writes -----------------------------------------------------

static int read_native(bf_gfx_device *d, const bf_gfx_rect &r, std::vector<uint8_t> &out)
{
	size_t row = size_t(r.w) * 4;
	id<MTLBuffer> buf = [d->device newBufferWithLength:row * size_t(r.h) options:MTLResourceStorageModeShared];
	if (!buf) return BF_GFX_E_MEMORY;
	ensure_blit(d);
	[d->bce copyFromTexture:d->target sourceSlice:0 sourceLevel:0
	           sourceOrigin:MTLOriginMake(NSUInteger(r.x), NSUInteger(r.y), 0)
	             sourceSize:MTLSizeMake(NSUInteger(r.w), NSUInteger(r.h), 1)
	               toBuffer:buf destinationOffset:0 destinationBytesPerRow:row
	 destinationBytesPerImage:row * size_t(r.h)];
	id<MTLCommandBuffer> cb = submit(d);
	[cb waitUntilCompleted];
	out.assign((const uint8_t *)buf.contents, (const uint8_t *)buf.contents + row * size_t(r.h));
	d->n_readbacks++;
	return BF_GFX_OK;
}

extern "C" int bf_gfx_read_pixels(bf_gfx_device *d, bf_gfx_space space, const bf_gfx_rect *rect, void *dst, int pitch)
{
	if (!d || !dst || !rect || rect->w <= 0 || rect->h <= 0) return BF_GFX_E_ARG;
	if (space != BF_GFX_SPACE_LOGICAL && space != BF_GFX_SPACE_NATIVE) return BF_GFX_E_ARG;
	int s = space == BF_GFX_SPACE_LOGICAL ? d->scale : 1;
	int limw = space == BF_GFX_SPACE_LOGICAL ? d->lw : d->nw, limh = space == BF_GFX_SPACE_LOGICAL ? d->lh : d->nh;
	if (rect->x < 0 || rect->y < 0 || rect->x + rect->w > limw || rect->y + rect->h > limh) return BF_GFX_E_ARG;
	if (pitch == 0) pitch = rect->w * 4;
	bf_gfx_rect nr{rect->x * s, rect->y * s, rect->w * s, rect->h * s};
	std::vector<uint8_t> px;
	int rc;
	@autoreleasepool { rc = read_native(d, nr, px); }
	if (rc) return rc;
	size_t nrow = size_t(nr.w) * 4;
	for (int y = 0; y < rect->h; ++y) {
		uint8_t *o = (uint8_t *)dst + size_t(y) * size_t(pitch);
		for (int x = 0; x < rect->w; ++x) {
			unsigned sum[3] = {0, 0, 0};
			for (int j = 0; j < s; ++j)
				for (int i = 0; i < s; ++i) {
					const uint8_t *p = px.data() + size_t(y * s + j) * nrow + size_t(x * s + i) * 4;
					sum[0] += p[0]; sum[1] += p[1]; sum[2] += p[2];
				}
			unsigned nn = unsigned(s * s);
			o[x * 4 + 0] = uint8_t((sum[0] + nn / 2) / nn);
			o[x * 4 + 1] = uint8_t((sum[1] + nn / 2) / nn);
			o[x * 4 + 2] = uint8_t((sum[2] + nn / 2) / nn);
			o[x * 4 + 3] = 255;
		}
	}
	return BF_GFX_OK;
}

extern "C" int bf_gfx_write_pixels(bf_gfx_device *d, bf_gfx_space space, const bf_gfx_rect *rect,
                                   const void *src, int pitch, unsigned flags)
{
	if (!d || !src || !rect || rect->w <= 0 || rect->h <= 0) return BF_GFX_E_ARG;
	if (space != BF_GFX_SPACE_LOGICAL && space != BF_GFX_SPACE_NATIVE) return BF_GFX_E_ARG;
	if (pitch == 0) pitch = rect->w * 4;
	@autoreleasepool {
		MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kColorFormat
			width:NSUInteger(rect->w) height:NSUInteger(rect->h) mipmapped:NO];
		td.storageMode = MTLStorageModeShared;
		td.usage = MTLTextureUsageShaderRead;
		id<MTLTexture> t = [d->device newTextureWithDescriptor:td];
		if (!t) return BF_GFX_E_MEMORY;
		[t replaceRegion:MTLRegionMake2D(0, 0, NSUInteger(rect->w), NSUInteger(rect->h)) mipmapLevel:0
		       withBytes:src bytesPerRow:NSUInteger(pitch)];
		int s = space == BF_GFX_SPACE_LOGICAL ? d->scale : 1;
		bf_gfx_rect full{rect->x * s, rect->y * s, rect->w * s, rect->h * s}, clip;
		if (!clip_rect(d->nw, d->nh, &full, &clip)) return BF_GFX_OK;
		ensure_render(d);
		[d->rce setRenderPipelineState:d->copy_pipe];
		[d->rce setDepthStencilState:d->ds_off];
		[d->rce setCullMode:MTLCullModeNone];
		reset_encoder_cache(d);
		[d->rce setScissorRect:MTLScissorRect{NSUInteger(clip.x), NSUInteger(clip.y), NSUInteger(clip.w), NSUInteger(clip.h)}];
		RectUniforms ru;
		memset(&ru, 0, sizeof ru);
		native_rect_ndc(d, full, ru.ndc);
		ru.uv[0] = 0; ru.uv[1] = 0; ru.uv[2] = 1; ru.uv[3] = 1;
		ru.skip_transparent = (flags & BF_GFX_WRITE_SKIP_TRANSPARENT) ? 1 : 0;
		[d->rce setVertexBytes:&ru length:sizeof ru atIndex:0];
		[d->rce setFragmentBytes:&ru length:sizeof ru atIndex:0];
		[d->rce setFragmentTexture:t atIndex:0];
		[d->rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
		[d->rce setScissorRect:MTLScissorRect{0, 0, NSUInteger(d->nw), NSUInteger(d->nh)}];
		return BF_GFX_OK;
	}
}

extern "C" int bf_gfx_backup_frame(bf_gfx_device *d)
{
	if (!d) return BF_GFX_E_ARG;
	@autoreleasepool {
		if (!d->backup) {
			MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kColorFormat
				width:NSUInteger(d->nw) height:NSUInteger(d->nh) mipmapped:NO];
			td.storageMode = MTLStorageModePrivate;
			td.usage = MTLTextureUsageShaderRead;
			d->backup = [d->device newTextureWithDescriptor:td];
			if (!d->backup) return BF_GFX_E_MEMORY;
		}
		ensure_blit(d);
		[d->bce copyFromTexture:d->target toTexture:d->backup];
		d->backup_valid = true;
	}
	return BF_GFX_OK;
}

extern "C" int bf_gfx_restore_frame(bf_gfx_device *d)
{
	if (!d) return BF_GFX_E_ARG;
	if (!d->backup_valid) return BF_GFX_E_STATE;
	@autoreleasepool {
		ensure_blit(d);
		[d->bce copyFromTexture:d->backup toTexture:d->target];
	}
	return BF_GFX_OK;
}

extern "C" int bf_gfx_capture_png(bf_gfx_device *d, const char *path)
{
	if (!d || !path) return BF_GFX_E_ARG;
	@autoreleasepool {
		std::vector<uint8_t> px;
		bf_gfx_rect r{0, 0, d->nw, d->nh};
		int rc = read_native(d, r, px);
		if (rc) return rc;
		CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
		CGDataProviderRef dp = CGDataProviderCreateWithData(nullptr, px.data(), px.size(), nullptr);
		CGImageRef img = CGImageCreate(size_t(d->nw), size_t(d->nh), 8, 32, size_t(d->nw) * 4, cs,
		                               kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, dp, nullptr, false,
		                               kCGRenderingIntentDefault);
		NSURL *url = [NSURL fileURLWithPath:@(path)];
		CGImageDestinationRef dst = CGImageDestinationCreateWithURL((__bridge CFURLRef)url, CFSTR("public.png"), 1, nullptr);
		bool ok = false;
		if (dst && img) {
			CGImageDestinationAddImage(dst, img, nullptr);
			ok = CGImageDestinationFinalize(dst);
		}
		if (dst) CFRelease(dst);
		if (img) CGImageRelease(img);
		CGDataProviderRelease(dp);
		CGColorSpaceRelease(cs);
		if (!ok) {
			bf_tracef(BFT_ERR, "gfx: cannot write %s\n", path);
			return BF_GFX_E_IO;
		}
		return BF_GFX_OK;
	}
}

extern "C" void bf_gfx_get_stats(const bf_gfx_device *d, long long *draws, long long *pipes,
                                 long long *readbacks, long long *frames)
{
	if (!d) return;
	if (draws) *draws = d->n_draws;
	if (pipes) *pipes = d->n_pipes;
	if (readbacks) *readbacks = d->n_readbacks;
	if (frames) *frames = d->n_frames;
}
