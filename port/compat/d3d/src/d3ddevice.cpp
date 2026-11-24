// Direct3D 7: IDirect3D7, IDirect3DDevice7 and IDirect3DVertexBuffer7 on
// port/native/gfx. Render and texture-stage states are kept as the client
// set them (for Get*State) and translated into one bf_gfx_state, which is
// handed to gfx before the next draw. The numbers pass straight through: the
// gfx enumerations carry the Direct3D 7 values (bf_gfx.h).
#include "d3d_internal.h"

#include <algorithm>
#include <string.h>

namespace bfd3d {

static const GUID k_iid_unknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};

// ---- caps ------------------------------------------------------------------------------------

static void fill_prim_caps(D3DPRIMCAPS *p)
{
	memset(p, 0, sizeof *p);
	p->dwSize = sizeof *p;
	p->dwMiscCaps = D3DPMISCCAPS_CULLNONE | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_MASKZ;
	p->dwRasterCaps = D3DPRASTERCAPS_ZTEST | D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_SUBPIXEL |
	                  D3DPRASTERCAPS_MIPMAPLODBIAS | D3DPRASTERCAPS_WFOG;
	p->dwZCmpCaps = p->dwAlphaCmpCaps = D3DPCMPCAPS_NEVER | D3DPCMPCAPS_LESS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_LESSEQUAL |
	                                    D3DPCMPCAPS_GREATER | D3DPCMPCAPS_NOTEQUAL | D3DPCMPCAPS_GREATEREQUAL |
	                                    D3DPCMPCAPS_ALWAYS;
	p->dwSrcBlendCaps = p->dwDestBlendCaps =
		D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
		D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA |
		D3DPBLENDCAPS_DESTCOLOR | D3DPBLENDCAPS_INVDESTCOLOR | D3DPBLENDCAPS_SRCALPHASAT |
		D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA;
	p->dwShadeCaps = D3DPSHADECAPS_COLORGOURAUDRGB | D3DPSHADECAPS_SPECULARGOURAUDRGB |
	                 D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_FOGGOURAUD;
	p->dwTextureCaps = D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_TRANSPARENCY |
	                   D3DPTEXTURECAPS_BORDER | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE;
	p->dwTextureFilterCaps = D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_MIPNEAREST |
	                         D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST |
	                         D3DPTFILTERCAPS_LINEARMIPLINEAR | D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR |
	                         D3DPTFILTERCAPS_MIPFPOINT | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT |
	                         D3DPTFILTERCAPS_MAGFLINEAR;
	p->dwTextureBlendCaps = D3DPTBLENDCAPS_DECAL | D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_DECALALPHA |
	                        D3DPTBLENDCAPS_MODULATEALPHA | D3DPTBLENDCAPS_ADD;
	p->dwTextureAddressCaps = D3DPTADDRESSCAPS_WRAP | D3DPTADDRESSCAPS_MIRROR | D3DPTADDRESSCAPS_CLAMP |
	                          D3DPTADDRESSCAPS_BORDER | D3DPTADDRESSCAPS_INDEPENDENTUV;
}

void fill_device_desc(D3DDEVICEDESC7 *d)
{
	memset(d, 0, sizeof *d);
	d->dwDevCaps = D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_EXECUTESYSTEMMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
	               D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_CANRENDERAFTERFLIP |
	               D3DDEVCAPS_DRAWPRIMITIVES2 | D3DDEVCAPS_DRAWPRIMITIVES2EX | D3DDEVCAPS_HWRASTERIZATION;
	fill_prim_caps(&d->dpcLineCaps);
	fill_prim_caps(&d->dpcTriCaps);
	d->dwDeviceRenderBitDepth = DDBD_16 | DDBD_32;
	d->dwDeviceZBufferBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
	int max = g().gfx ? bf_gfx_max_texture_size(g().gfx) : 16384;
	d->dwMinTextureWidth = d->dwMinTextureHeight = 1;
	d->dwMaxTextureWidth = d->dwMaxTextureHeight = DWORD(max);
	d->dwMaxTextureRepeat = 8192;
	d->dwMaxTextureAspectRatio = 0;
	d->dwMaxAnisotropy = 1;
	// The client ignores the guard band (CRenderer::m_guardBandLeft is always 0).
	d->dvGuardBandLeft = -16384.0f;
	d->dvGuardBandTop = -16384.0f;
	d->dvGuardBandRight = 16384.0f;
	d->dvGuardBandBottom = 16384.0f;
	d->dwFVFCaps = 2;   // two texture coordinate sets
	d->dwTextureOpCaps = D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_SELECTARG2 |
	                     D3DTEXOPCAPS_MODULATE | D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_MODULATE4X |
	                     D3DTEXOPCAPS_ADD | D3DTEXOPCAPS_ADDSIGNED | D3DTEXOPCAPS_ADDSIGNED2X |
	                     D3DTEXOPCAPS_SUBTRACT | D3DTEXOPCAPS_ADDSMOOTH | D3DTEXOPCAPS_BLENDDIFFUSEALPHA |
	                     D3DTEXOPCAPS_BLENDTEXTUREALPHA | D3DTEXOPCAPS_BLENDFACTORALPHA |
	                     D3DTEXOPCAPS_BLENDTEXTUREALPHAPM | D3DTEXOPCAPS_BLENDCURRENTALPHA |
	                     D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
	                     D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
	                     D3DTEXOPCAPS_DOTPRODUCT3;
	d->wMaxTextureBlendStages = 2;
	d->wMaxSimultaneousTextures = 2;
	d->dwMaxActiveLights = 8;
	d->dvMaxVertexW = 1e10f;
	d->deviceGUID = IID_IDirect3DHALDevice;
	d->wMaxUserClipPlanes = 0;
	d->wMaxVertexBlendMatrices = 0;
}

// ---- vertex buffers ---------------------------------------------------------------------------

static DWORD fvf_stride(DWORD fvf)
{
	DWORD s = 0;
	switch (fvf & D3DFVF_POSITION_MASK) {
	case D3DFVF_XYZ: s = 12; break;
	case D3DFVF_XYZRHW: s = 16; break;
	default: if (fvf & D3DFVF_POSITION_MASK) s = 12 + 4 * (((fvf & D3DFVF_POSITION_MASK) - D3DFVF_XYZ) / 2); break;
	}
	if (fvf & D3DFVF_NORMAL) s += 12;
	if (fvf & D3DFVF_RESERVED1) s += 4;
	if (fvf & D3DFVF_DIFFUSE) s += 4;
	if (fvf & D3DFVF_SPECULAR) s += 4;
	s += 8 * ((fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);
	return s;
}

// The two layouts the gfx module draws: tlvertex3d and lmtlvertex3d.
static bool gfx_vertex_format(DWORD fvf, bf_gfx_vertex_format *out)
{
	if (fvf == (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)) { *out = BF_GFX_VERTEX_TL1; return true; }
	if (fvf == (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)) { *out = BF_GFX_VERTEX_TL2; return true; }
	return false;
}

struct VertexBuffer final : public IDirect3DVertexBuffer7 {
	ULONG refs = 1;
	D3DVERTEXBUFFERDESC desc{};
	DWORD stride = 0;
	std::vector<uint8_t> mem;
	bool locked = false;

	STDMETHOD_(HRESULT, QueryInterface)(REFIID riid, void **out) override
	{
		if (!out) return E_POINTER;
		if (IsEqualGUID(riid, IID_IDirect3DVertexBuffer7) || IsEqualGUID(riid, k_iid_unknown)) { *out = this; AddRef(); return S_OK; }
		*out = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() override { return ++refs; }
	STDMETHOD_(ULONG, Release)() override { ULONG n = --refs; if (!n) delete this; return n; }
	STDMETHOD(Lock)(DWORD, void **data, DWORD *size) override
	{
		if (!data) return DDERR_INVALIDPARAMS;
		if (locked) return D3DERR_VERTEXBUFFERLOCKED;
		locked = true;
		*data = mem.data();
		if (size) *size = DWORD(mem.size());
		return D3D_OK;
	}
	STDMETHOD(Unlock)() override
	{
		if (!locked) return D3DERR_VERTEXBUFFERUNLOCKFAILED;
		locked = false;
		return D3D_OK;
	}
	STDMETHOD(ProcessVertices)(DWORD, DWORD, DWORD, IDirect3DVertexBuffer7 *, DWORD, IDirect3DDevice7 *, DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetVertexBufferDesc)(D3DVERTEXBUFFERDESC *d) override { if (!d) return DDERR_INVALIDPARAMS; *d = desc; return D3D_OK; }
	STDMETHOD(Optimize)(IDirect3DDevice7 *, DWORD) override { return D3D_OK; }
	STDMETHOD(ProcessVerticesStrided)(DWORD, DWORD, DWORD, D3DDRAWPRIMITIVESTRIDEDDATA *, DWORD, IDirect3DDevice7 *, DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
};

// ---- the device ----------------------------------------------------------------------------------

struct D3D;

struct Device final : public IDirect3DDevice7 {
	ULONG refs = 1;
	D3D *d3d = nullptr;               // counted
	Surface *target = nullptr;        // counted
	Surface *tex[8] = {};             // counted, as Direct3D 7 does
	DWORD rs[256];
	DWORD tss[8][32];
	bf_gfx_state st;
	bool st_dirty = true;
	unsigned long long own_draws_seen = 0;   // Global::own_draws
	D3DVIEWPORT7 vp{};
	D3DMATRIX xf[32];
	D3DMATERIAL7 material{};
	std::vector<D3DLIGHT7> lights;
	std::vector<BOOL> light_on;
	bool in_scene = false;

	Device(D3D *d, Surface *t);
	~Device();
	void flush_state();
	void set_rs(DWORD type, DWORD v);
	void set_tss(DWORD stage, DWORD type, DWORD v);
	HRESULT draw(D3DPRIMITIVETYPE type, DWORD fvf, const void *verts, DWORD n, const WORD *idx, DWORD ni);

	STDMETHOD_(HRESULT, QueryInterface)(REFIID riid, void **out) override;
	STDMETHOD_(ULONG, AddRef)() override { return ++refs; }
	STDMETHOD_(ULONG, Release)() override { ULONG n = --refs; if (!n) delete this; return n; }
	STDMETHOD(GetCaps)(D3DDEVICEDESC7 *desc) override { if (!desc) return DDERR_INVALIDPARAMS; fill_device_desc(desc); return D3D_OK; }
	STDMETHOD(EnumTextureFormats)(LPD3DENUMPIXELFORMATSCALLBACK cb, void *ctx) override;
	STDMETHOD(BeginScene)() override;
	STDMETHOD(EndScene)() override;
	STDMETHOD(GetDirect3D)(IDirect3D7 **d3d) override;
	STDMETHOD(SetRenderTarget)(IDirectDrawSurface7 *surface, DWORD flags) override;
	STDMETHOD(GetRenderTarget)(IDirectDrawSurface7 **surface) override;
	STDMETHOD(Clear)(DWORD count, D3DRECT *rects, DWORD flags, D3DCOLOR color, D3DVALUE z, DWORD stencil) override;
	STDMETHOD(SetTransform)(D3DTRANSFORMSTATETYPE state, D3DMATRIX *m) override;
	STDMETHOD(GetTransform)(D3DTRANSFORMSTATETYPE state, D3DMATRIX *m) override;
	STDMETHOD(SetViewport)(D3DVIEWPORT7 *data) override;
	STDMETHOD(MultiplyTransform)(D3DTRANSFORMSTATETYPE state, D3DMATRIX *m) override;
	STDMETHOD(GetViewport)(D3DVIEWPORT7 *data) override;
	STDMETHOD(SetMaterial)(D3DMATERIAL7 *data) override { if (!data) return DDERR_INVALIDPARAMS; material = *data; return D3D_OK; }
	STDMETHOD(GetMaterial)(D3DMATERIAL7 *data) override { if (!data) return DDERR_INVALIDPARAMS; *data = material; return D3D_OK; }
	STDMETHOD(SetLight)(DWORD idx, D3DLIGHT7 *data) override;
	STDMETHOD(GetLight)(DWORD idx, D3DLIGHT7 *data) override;
	STDMETHOD(SetRenderState)(D3DRENDERSTATETYPE type, DWORD value) override;
	STDMETHOD(GetRenderState)(D3DRENDERSTATETYPE type, LPDWORD value) override;
	STDMETHOD(BeginStateBlock)() override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(EndStateBlock)(LPDWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(PreLoad)(IDirectDrawSurface7 *) override { return D3D_OK; }
	STDMETHOD(DrawPrimitive)(D3DPRIMITIVETYPE type, DWORD fvf, void *verts, DWORD n, DWORD flags) override;
	STDMETHOD(DrawIndexedPrimitive)(D3DPRIMITIVETYPE type, DWORD fvf, void *verts, DWORD n, WORD *idx, DWORD ni, DWORD flags) override;
	STDMETHOD(SetClipStatus)(D3DCLIPSTATUS *) override { return D3D_OK; }
	STDMETHOD(GetClipStatus)(D3DCLIPSTATUS *s) override { if (s) memset(s, 0, sizeof *s); return D3D_OK; }
	STDMETHOD(DrawPrimitiveStrided)(D3DPRIMITIVETYPE, DWORD, D3DDRAWPRIMITIVESTRIDEDDATA *, DWORD, DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(DrawIndexedPrimitiveStrided)(D3DPRIMITIVETYPE, DWORD, D3DDRAWPRIMITIVESTRIDEDDATA *, DWORD, WORD *, DWORD, DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(DrawPrimitiveVB)(D3DPRIMITIVETYPE type, IDirect3DVertexBuffer7 *vb, DWORD start, DWORD n, DWORD flags) override;
	STDMETHOD(DrawIndexedPrimitiveVB)(D3DPRIMITIVETYPE type, IDirect3DVertexBuffer7 *vb, DWORD start, DWORD n, WORD *idx, DWORD ni, DWORD flags) override;
	STDMETHOD(ComputeSphereVisibility)(D3DVECTOR *, D3DVALUE *, DWORD, DWORD, DWORD *) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(GetTexture)(DWORD stage, IDirectDrawSurface7 **surface) override;
	STDMETHOD(SetTexture)(DWORD stage, IDirectDrawSurface7 *surface) override;
	STDMETHOD(GetTextureStageState)(DWORD stage, D3DTEXTURESTAGESTATETYPE type, LPDWORD value) override;
	STDMETHOD(SetTextureStageState)(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) override;
	STDMETHOD(ValidateDevice)(LPDWORD passes) override { if (passes) *passes = 1; return D3D_OK; }
	STDMETHOD(ApplyStateBlock)(DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(CaptureStateBlock)(DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(DeleteStateBlock)(DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(CreateStateBlock)(D3DSTATEBLOCKTYPE, LPDWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(Load)(IDirectDrawSurface7 *, POINT *, IDirectDrawSurface7 *, RECT *, DWORD) override { BF_STUB(); return DDERR_UNSUPPORTED; }
	STDMETHOD(LightEnable)(DWORD idx, BOOL enable) override;
	STDMETHOD(GetLightEnable)(DWORD idx, BOOL *enable) override;
	STDMETHOD(SetClipPlane)(DWORD, D3DVALUE *) override { return D3D_OK; }
	STDMETHOD(GetClipPlane)(DWORD, D3DVALUE *) override { return D3D_OK; }
	STDMETHOD(GetInfo)(DWORD, void *, DWORD) override { return S_FALSE; }
};

// ---- Direct3D -------------------------------------------------------------------------------------

struct D3D final : public IDirect3D7 {
	ULONG refs = 1;
	DDraw *dd = nullptr;   // counted

	~D3D()
	{
		if (g().d3d == this) g().d3d = nullptr;
		if (dd) dd->Release();
	}
	STDMETHOD_(HRESULT, QueryInterface)(REFIID riid, void **out) override
	{
		if (!out) return E_POINTER;
		if (IsEqualGUID(riid, IID_IDirect3D7) || IsEqualGUID(riid, k_iid_unknown)) { *out = this; AddRef(); return S_OK; }
		if (IsEqualGUID(riid, IID_IDirectDraw7)) return dd->QueryInterface(riid, out);
		*out = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() override { return ++refs; }
	STDMETHOD_(ULONG, Release)() override { ULONG n = --refs; if (!n) delete this; return n; }

	STDMETHOD(EnumDevices)(LPD3DENUMDEVICESCALLBACK7 cb, void *ctx) override
	{
		if (!cb) return DDERR_INVALIDPARAMS;
		D3DDEVICEDESC7 d;
		fill_device_desc(&d);
		char desc[] = "Metal";
		char name[] = "HAL";
		if (cb(desc, name, &d, ctx) == D3DENUMRET_CANCEL) return D3D_OK;
		char tdesc[] = "Metal T&L";
		char tname[] = "T&L HAL";
		d.deviceGUID = IID_IDirect3DTnLHalDevice;
		cb(tdesc, tname, &d, ctx);
		return D3D_OK;
	}

	STDMETHOD(CreateDevice)(REFCLSID, IDirectDrawSurface7 *surface, IDirect3DDevice7 **out) override
	{
		if (!out) return DDERR_INVALIDPARAMS;
		*out = nullptr;
		Surface *s = as_surface(surface);
		if (!s || !s->is_target()) return DDERR_INVALIDCAPS;
		if (!g().gfx) return DDERR_NODIRECTDRAWHW;
		*out = new Device(this, s);   // any device GUID: HAL, T&L HAL, RGB, reference
		return D3D_OK;
	}

	STDMETHOD(CreateVertexBuffer)(D3DVERTEXBUFFERDESC *desc, IDirect3DVertexBuffer7 **out, DWORD) override
	{
		if (!desc || !out) return DDERR_INVALIDPARAMS;
		VertexBuffer *vb = new VertexBuffer;
		vb->desc = *desc;
		vb->stride = fvf_stride(desc->dwFVF);
		vb->mem.assign(size_t(vb->stride) * desc->dwNumVertices, 0);
		*out = vb;
		return D3D_OK;
	}

	STDMETHOD(EnumZBufferFormats)(REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK cb, void *ctx) override
	{
		if (!cb) return DDERR_INVALIDPARAMS;
		// 16-bit first: C3dDevice's callback takes the first 16-bit format.
		struct { DWORD flags, depth, stencil, zmask, smask; } z[] = {
			{DDPF_ZBUFFER, 16, 0, 0xFFFF, 0},
			{DDPF_ZBUFFER, 32, 0, 0xFFFFFFFF, 0},
			{DDPF_ZBUFFER | DDPF_STENCILBUFFER, 32, 8, 0x00FFFFFF, 0xFF000000},
		};
		for (auto &f : z) {
			DDPIXELFORMAT pf;
			memset(&pf, 0, sizeof pf);
			pf.dwSize = sizeof pf;
			pf.dwFlags = f.flags;
			pf.dwZBufferBitDepth = f.depth;
			pf.dwStencilBitDepth = f.stencil;
			pf.dwZBitMask = f.zmask;
			pf.dwStencilBitMask = f.smask;
			if (cb(&pf, ctx) == D3DENUMRET_CANCEL) break;
		}
		return D3D_OK;
	}

	STDMETHOD(EvictManagedTextures)() override { return D3D_OK; }
};

IDirect3D7 *query_d3d(DDraw *dd)
{
	Global &G = g();
	if (G.d3d) {
		G.d3d->AddRef();
		return G.d3d;
	}
	D3D *d = new D3D;
	d->dd = dd;
	dd->AddRef();
	G.d3d = d;
	return d;
}

// ---- device implementation ---------------------------------------------------------------------------

Device::Device(D3D *d, Surface *t) : d3d(d), target(t)
{
	d3d->AddRef();
	target->AddRef();
	memset(rs, 0, sizeof rs);
	memset(tss, 0, sizeof tss);
	// Direct3D 7 defaults (the ones a Get can observe; bf_gfx_state_init has the same).
	rs[D3DRENDERSTATE_TEXTUREPERSPECTIVE] = TRUE;
	rs[D3DRENDERSTATE_ZENABLE] = D3DZB_TRUE;
	rs[D3DRENDERSTATE_FILLMODE] = D3DFILL_SOLID;
	rs[D3DRENDERSTATE_SHADEMODE] = D3DSHADE_GOURAUD;
	rs[D3DRENDERSTATE_ZWRITEENABLE] = TRUE;
	rs[D3DRENDERSTATE_LASTPIXEL] = TRUE;
	rs[D3DRENDERSTATE_SRCBLEND] = D3DBLEND_ONE;
	rs[D3DRENDERSTATE_DESTBLEND] = D3DBLEND_ZERO;
	rs[D3DRENDERSTATE_CULLMODE] = D3DCULL_CCW;
	rs[D3DRENDERSTATE_ZFUNC] = D3DCMP_LESSEQUAL;
	rs[D3DRENDERSTATE_ALPHAFUNC] = D3DCMP_ALWAYS;
	rs[D3DRENDERSTATE_TEXTUREFACTOR] = 0xFFFFFFFF;
	rs[D3DRENDERSTATE_CLIPPING] = TRUE;
	rs[D3DRENDERSTATE_LIGHTING] = TRUE;
	rs[D3DRENDERSTATE_COLORVERTEX] = TRUE;
	for (int i = 0; i < 8; ++i) {
		tss[i][D3DTSS_COLOROP] = i == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE;
		tss[i][D3DTSS_COLORARG1] = D3DTA_TEXTURE;
		tss[i][D3DTSS_COLORARG2] = D3DTA_CURRENT;
		tss[i][D3DTSS_ALPHAOP] = i == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
		tss[i][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
		tss[i][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
		tss[i][D3DTSS_TEXCOORDINDEX] = DWORD(i);
		tss[i][D3DTSS_ADDRESS] = tss[i][D3DTSS_ADDRESSU] = tss[i][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
		tss[i][D3DTSS_MAGFILTER] = D3DTFG_POINT;
		tss[i][D3DTSS_MINFILTER] = D3DTFN_POINT;
		tss[i][D3DTSS_MIPFILTER] = D3DTFP_NONE;
	}
	bf_gfx_state_init(&st);
	int w, h;
	target_size(&w, &h);
	vp.dwWidth = DWORD(w);
	vp.dwHeight = DWORD(h);
	vp.dvMaxZ = 1.0f;
	for (D3DMATRIX &m : xf) {
		memset(&m, 0, sizeof m);
		m._11 = m._22 = m._33 = m._44 = 1.0f;
	}
	if (g().gfx) {
		bf_gfx_set_texture(g().gfx, 0, nullptr);
		bf_gfx_set_texture(g().gfx, 1, nullptr);
	}
}

Device::~Device()
{
	for (int i = 0; i < 8; ++i)
		if (tex[i]) {
			if (i < 2 && g().gfx) bf_gfx_set_texture(g().gfx, i, nullptr);
			tex[i]->Release();
		}
	if (target) target->Release();
	if (d3d) d3d->Release();
}

HRESULT Device::QueryInterface(REFIID riid, void **out)
{
	if (!out) return E_POINTER;
	if (IsEqualGUID(riid, IID_IDirect3DDevice7) || IsEqualGUID(riid, k_iid_unknown)) { *out = this; AddRef(); return S_OK; }
	*out = nullptr;
	return E_NOINTERFACE;
}

static uint8_t u8(DWORD v, DWORD lo, DWORD hi, uint8_t keep)
{
	return v >= lo && v <= hi ? uint8_t(v) : keep;
}

void Device::set_rs(DWORD type, DWORD v)
{
	switch (type) {
	case D3DRENDERSTATE_ZENABLE: st.depth_enable = v != D3DZB_FALSE; break;
	case D3DRENDERSTATE_ZWRITEENABLE: st.depth_write = v != 0; break;
	case D3DRENDERSTATE_ALPHATESTENABLE: st.alpha_test_enable = v != 0; break;
	case D3DRENDERSTATE_SRCBLEND: st.src_blend = u8(v, 1, 13, st.src_blend); break;
	case D3DRENDERSTATE_DESTBLEND: st.dst_blend = u8(v, 1, 13, st.dst_blend); break;
	case D3DRENDERSTATE_CULLMODE: st.cull = u8(v, 1, 3, st.cull); break;
	case D3DRENDERSTATE_ZFUNC: st.depth_func = u8(v, 1, 8, st.depth_func); break;
	case D3DRENDERSTATE_ALPHAREF: st.alpha_ref = uint8_t(v & 0xFF); break;
	case D3DRENDERSTATE_ALPHAFUNC: st.alpha_func = u8(v, 1, 8, st.alpha_func); break;
	case D3DRENDERSTATE_ALPHABLENDENABLE: st.blend_enable = v != 0; break;
	case D3DRENDERSTATE_FOGENABLE: st.fog_enable = v != 0; break;
	case D3DRENDERSTATE_SPECULARENABLE: st.specular_enable = v != 0; break;
	case D3DRENDERSTATE_FOGCOLOR: st.fog_color = v; break;
	case D3DRENDERSTATE_TEXTUREFACTOR: st.texture_factor = v; break;
	default: return;   // no effect on pre-transformed vertices, or not expressible (D3D-USAGE.md 3)
	}
	st_dirty = true;
}

void Device::set_tss(DWORD stage, DWORD type, DWORD v)
{
	if (stage >= 2) {
		if (type == D3DTSS_COLOROP && v != D3DTOP_DISABLE) BFD3D_ONCE("d3d: texture stage %u enabled; the gfx module has two", unsigned(stage));
		return;
	}
	bf_gfx_stage_state &s = st.stage[stage];
	switch (type) {
	case D3DTSS_COLOROP: s.color_op = u8(v, 1, 24, BF_GFX_OP_DISABLE); break;
	case D3DTSS_ALPHAOP: s.alpha_op = u8(v, 1, 24, BF_GFX_OP_DISABLE); break;
	case D3DTSS_COLORARG1: s.color_arg1 = uint8_t(v & 0x3F); break;
	case D3DTSS_COLORARG2: s.color_arg2 = uint8_t(v & 0x3F); break;
	case D3DTSS_ALPHAARG1: s.alpha_arg1 = uint8_t(v & 0x3F); break;
	case D3DTSS_ALPHAARG2: s.alpha_arg2 = uint8_t(v & 0x3F); break;
	case D3DTSS_TEXCOORDINDEX: s.texcoord_index = (v & 0xFFFF) ? 1 : 0; break;
	case D3DTSS_ADDRESS: s.address_u = s.address_v = u8(v, 1, 4, s.address_u); break;
	case D3DTSS_ADDRESSU: s.address_u = u8(v, 1, 4, s.address_u); break;
	case D3DTSS_ADDRESSV: s.address_v = u8(v, 1, 4, s.address_v); break;
	case D3DTSS_BORDERCOLOR: s.border_color = v; break;
	case D3DTSS_MAGFILTER: s.mag_filter = v == D3DTFG_POINT ? BF_GFX_FILTER_NEAREST : BF_GFX_FILTER_LINEAR; break;
	case D3DTSS_MINFILTER: s.min_filter = v == D3DTFN_POINT ? BF_GFX_FILTER_NEAREST : BF_GFX_FILTER_LINEAR; break;
	case D3DTSS_MIPFILTER: s.mip_filter = u8(v, 1, 3, s.mip_filter); break;
	default: return;   // bump-mapping and texture-transform states: unused
	}
	st_dirty = true;
}

HRESULT Device::SetRenderState(D3DRENDERSTATETYPE type, DWORD value)
{
	DWORD t = DWORD(type);
	if (t >= 256) return DDERR_INVALIDPARAMS;
	rs[t] = value;
	set_rs(t, value);
	return D3D_OK;
}

HRESULT Device::GetRenderState(D3DRENDERSTATETYPE type, LPDWORD value)
{
	if (!value || DWORD(type) >= 256) return DDERR_INVALIDPARAMS;
	*value = rs[DWORD(type)];
	return D3D_OK;
}

HRESULT Device::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
	DWORD t = DWORD(type);
	if (stage >= 8 || t >= 32) return DDERR_INVALIDPARAMS;
	tss[stage][t] = value;
	if (t == D3DTSS_ADDRESS) tss[stage][D3DTSS_ADDRESSU] = tss[stage][D3DTSS_ADDRESSV] = value;
	set_tss(stage, t, value);
	return D3D_OK;
}

HRESULT Device::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, LPDWORD value)
{
	if (!value || stage >= 8 || DWORD(type) >= 32) return DDERR_INVALIDPARAMS;
	*value = tss[stage][DWORD(type)];
	return D3D_OK;
}

HRESULT Device::SetTexture(DWORD stage, IDirectDrawSurface7 *surface)
{
	if (stage >= 8) return DDERR_INVALIDPARAMS;
	Surface *s = as_surface(surface);
	if (s && s->kind != K_TEXTURE && s->kind != K_MIP) return DDERR_INVALIDPARAMS;
	if (s) s->AddRef();
	if (tex[stage]) tex[stage]->Release();
	tex[stage] = s;
	if (stage < 2 && g().gfx) bf_gfx_set_texture(g().gfx, int(stage), s ? s->gfx_texture() : nullptr);
	return D3D_OK;
}

HRESULT Device::GetTexture(DWORD stage, IDirectDrawSurface7 **surface)
{
	if (!surface || stage >= 8) return DDERR_INVALIDPARAMS;
	*surface = tex[stage];
	if (tex[stage]) tex[stage]->AddRef();
	return D3D_OK;
}

void Device::flush_state()
{
	if (g().own_draws != own_draws_seen && g().gfx) {
		// The adapter drew on its own (the stretched present): state and textures again.
		own_draws_seen = g().own_draws;
		st_dirty = true;
		for (int i = 0; i < 2; ++i) bf_gfx_set_texture(g().gfx, i, tex[i] ? tex[i]->gfx_texture() : nullptr);
	}
	if (st_dirty && g().gfx) {
		bf_gfx_set_state(g().gfx, &st);
		st_dirty = false;
	}
}

HRESULT Device::draw(D3DPRIMITIVETYPE type, DWORD fvf, const void *verts, DWORD n, const WORD *idx, DWORD ni)
{
	Global &G = g();
	if (!G.gfx) return DDERR_SURFACELOST;
	if (!verts) return DDERR_INVALIDPARAMS;
	bf_gfx_vertex_format f;
	if (!gfx_vertex_format(fvf, &f)) {
		BFD3D_ONCE("d3d: draw with FVF 0x%x skipped: only pre-transformed tlvertex3d/lmtlvertex3d are drawn", unsigned(fvf));
		return D3D_OK;
	}
	if (DWORD(type) < D3DPT_POINTLIST || DWORD(type) > D3DPT_TRIANGLEFAN) return DDERR_INVALIDPARAMS;
	if (n > 0xFFFF && idx) return DDERR_INVALIDPARAMS;
	flush_state();
	int rc = idx ? bf_gfx_draw_indexed(G.gfx, bf_gfx_prim(type), f, verts, int(n), (const uint16_t *)idx, int(ni))
	             : bf_gfx_draw(G.gfx, bf_gfx_prim(type), f, verts, int(n));
	target_changed();
	return rc == BF_GFX_OK ? D3D_OK : DDERR_GENERIC;
}

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE type, DWORD fvf, void *verts, DWORD n, DWORD)
{
	return draw(type, fvf, verts, n, nullptr, 0);
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE type, DWORD fvf, void *verts, DWORD n, WORD *idx, DWORD ni, DWORD)
{
	if (!idx) return DDERR_INVALIDPARAMS;
	return draw(type, fvf, verts, n, idx, ni);
}

HRESULT Device::DrawPrimitiveVB(D3DPRIMITIVETYPE type, IDirect3DVertexBuffer7 *vbi, DWORD start, DWORD n, DWORD)
{
	VertexBuffer *vb = static_cast<VertexBuffer *>(vbi);
	if (!vb || start + n > vb->desc.dwNumVertices) return DDERR_INVALIDPARAMS;
	return draw(type, vb->desc.dwFVF, vb->mem.data() + size_t(start) * vb->stride, n, nullptr, 0);
}

HRESULT Device::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE type, IDirect3DVertexBuffer7 *vbi, DWORD start, DWORD n, WORD *idx, DWORD ni, DWORD)
{
	VertexBuffer *vb = static_cast<VertexBuffer *>(vbi);
	if (!vb || !idx || start + n > vb->desc.dwNumVertices) return DDERR_INVALIDPARAMS;
	return draw(type, vb->desc.dwFVF, vb->mem.data() + size_t(start) * vb->stride, n, idx, ni);
}

HRESULT Device::Clear(DWORD count, D3DRECT *rects, DWORD flags, D3DCOLOR color, D3DVALUE z, DWORD)
{
	Global &G = g();
	if (!G.gfx) return DDERR_SURFACELOST;
	unsigned f = ((flags & D3DCLEAR_TARGET) ? BF_GFX_CLEAR_COLOR : 0) | ((flags & D3DCLEAR_ZBUFFER) ? BF_GFX_CLEAR_DEPTH : 0);
	if (!f) return D3D_OK;
	int tw, th;
	target_size(&tw, &th);
	// Clears are clipped to the viewport, as in Direct3D.
	LONG vx0 = LONG(vp.dwX), vy0 = LONG(vp.dwY);
	LONG vx1 = (std::min<LONG>)(LONG(vp.dwX + vp.dwWidth), tw), vy1 = (std::min<LONG>)(LONG(vp.dwY + vp.dwHeight), th);
	D3DRECT all;
	all.x1 = vx0; all.y1 = vy0; all.x2 = vx1; all.y2 = vy1;
	if (!count || !rects) { count = 1; rects = &all; }
	for (DWORD i = 0; i < count; ++i) {
		LONG x0 = (std::max)(rects[i].x1, vx0), y0 = (std::max)(rects[i].y1, vy0);
		LONG x1 = (std::min)(rects[i].x2, vx1), y1 = (std::min)(rects[i].y2, vy1);
		if (x1 <= x0 || y1 <= y0) continue;
		bf_gfx_rect r{int(x0), int(y0), int(x1 - x0), int(y1 - y0)};
		bool full = x0 == 0 && y0 == 0 && x1 == tw && y1 == th;
		bf_gfx_clear(G.gfx, f, color, z, full ? nullptr : &r);
	}
	if (f & BF_GFX_CLEAR_COLOR) target_changed();
	return D3D_OK;
}

HRESULT Device::BeginScene()
{
	// Always succeeds: a scene left open by a failed frame must not stop every
	// later frame (the client brackets each flush with Begin/EndScene).
	in_scene = true;
	if (g().gfx) bf_gfx_begin_frame(g().gfx);
	return D3D_OK;
}

HRESULT Device::EndScene()
{
	in_scene = false;
	if (g().gfx) bf_gfx_end_frame(g().gfx);   // submit without waiting
	return D3D_OK;
}

HRESULT Device::GetDirect3D(IDirect3D7 **out)
{
	if (!out) return DDERR_INVALIDPARAMS;
	*out = d3d;
	d3d->AddRef();
	return D3D_OK;
}

HRESULT Device::SetRenderTarget(IDirectDrawSurface7 *surface, DWORD)
{
	Surface *s = as_surface(surface);
	if (!s || !s->is_target()) return DDERR_INVALIDPARAMS;
	s->AddRef();
	target->Release();
	target = s;
	return D3D_OK;
}

HRESULT Device::GetRenderTarget(IDirectDrawSurface7 **out)
{
	if (!out) return DDERR_INVALIDPARAMS;
	*out = target;
	target->AddRef();
	return D3D_OK;
}

HRESULT Device::SetViewport(D3DVIEWPORT7 *v)
{
	if (!v) return DDERR_INVALIDPARAMS;
	int tw, th;
	target_size(&tw, &th);
	if (v->dwX + v->dwWidth > DWORD(tw) || v->dwY + v->dwHeight > DWORD(th)) return DDERR_INVALIDPARAMS;
	vp = *v;
	if (vp.dwX || vp.dwY || int(vp.dwWidth) != tw || int(vp.dwHeight) != th)
		BFD3D_ONCE("d3d: a viewport smaller than the target clips clears only, not draws");
	return D3D_OK;
}

HRESULT Device::GetViewport(D3DVIEWPORT7 *v)
{
	if (!v) return DDERR_INVALIDPARAMS;
	*v = vp;
	return D3D_OK;
}

HRESULT Device::SetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *m)
{
	if (!m || DWORD(state) >= 32) return DDERR_INVALIDPARAMS;
	xf[DWORD(state)] = *m;   // no effect on pre-transformed vertices
	return D3D_OK;
}

HRESULT Device::GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *m)
{
	if (!m || DWORD(state) >= 32) return DDERR_INVALIDPARAMS;
	*m = xf[DWORD(state)];
	return D3D_OK;
}

HRESULT Device::MultiplyTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX *m)
{
	if (!m || DWORD(state) >= 32) return DDERR_INVALIDPARAMS;
	const float *a = &xf[DWORD(state)]._11, *b = &m->_11;
	D3DMATRIX r;
	float *o = &r._11;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j) {
			float s = 0;
			for (int k = 0; k < 4; ++k) s += b[i * 4 + k] * a[k * 4 + j];
			o[i * 4 + j] = s;
		}
	xf[DWORD(state)] = r;
	return D3D_OK;
}

HRESULT Device::SetLight(DWORD idx, D3DLIGHT7 *data)
{
	if (!data || idx > 1024) return DDERR_INVALIDPARAMS;
	if (lights.size() <= idx) { lights.resize(idx + 1); light_on.resize(idx + 1, FALSE); }
	lights[idx] = *data;   // no effect on pre-transformed vertices
	return D3D_OK;
}

HRESULT Device::GetLight(DWORD idx, D3DLIGHT7 *data)
{
	if (!data || idx >= lights.size()) return DDERR_INVALIDPARAMS;
	*data = lights[idx];
	return D3D_OK;
}

HRESULT Device::LightEnable(DWORD idx, BOOL enable)
{
	if (idx > 1024) return DDERR_INVALIDPARAMS;
	if (lights.size() <= idx) { lights.resize(idx + 1); light_on.resize(idx + 1, FALSE); }
	light_on[idx] = enable;
	return D3D_OK;
}

HRESULT Device::GetLightEnable(DWORD idx, BOOL *enable)
{
	if (!enable || idx >= light_on.size()) return DDERR_INVALIDPARAMS;
	*enable = light_on[idx];
	return D3D_OK;
}

HRESULT Device::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK cb, void *ctx)
{
	if (!cb) return DDERR_INVALIDPARAMS;
	struct { DWORD flags, bits, r, g, b, a; } f[] = {
		{DDPF_RGB | DDPF_ALPHAPIXELS, 16, 0x7C00, 0x03E0, 0x001F, 0x8000},       // A1R5G5B5
		{DDPF_RGB | DDPF_ALPHAPIXELS, 16, 0x0F00, 0x00F0, 0x000F, 0xF000},       // A4R4G4B4
		{DDPF_RGB, 16, 0xF800, 0x07E0, 0x001F, 0},                               // R5G6B5
		{DDPF_RGB | DDPF_ALPHAPIXELS, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000},   // A8R8G8B8
	};
	for (auto &e : f) {
		DDPIXELFORMAT pf;
		memset(&pf, 0, sizeof pf);
		pf.dwSize = sizeof pf;
		pf.dwFlags = e.flags;
		pf.dwRGBBitCount = e.bits;
		pf.dwRBitMask = e.r;
		pf.dwGBitMask = e.g;
		pf.dwBBitMask = e.b;
		pf.dwRGBAlphaBitMask = e.a;
		if (cb(&pf, ctx) == D3DENUMRET_CANCEL) break;
	}
	return D3D_OK;
}

} // namespace bfd3d
