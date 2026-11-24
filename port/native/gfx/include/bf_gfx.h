/*
 * gfx: the Metal renderer for the client's pre-transformed vertices.
 *
 * The client (src/3dlib) transforms, lights and projects every vertex on the
 * CPU and hands the device screen-space vertices in two layouts. This module
 * draws them: one vertex shader maps logical screen positions to clip space,
 * fragment programs are specialised from the texture-stage state, and the
 * fixed-function state (blend, alpha test, depth, cull, fog) is set in native
 * terms. The DirectDraw/Direct3D adapter (port/compat) maps the Direct3D 7
 * calls onto it; port/native/gfx/D3D-USAGE.md lists every call and value the
 * client uses and how each maps here.
 *
 * Enumerations carry the same numbers as the Direct3D 7 values of the same
 * meaning, so that numbers stored in game data (blend factors in .str effect
 * files) and the adapter's arguments pass through unchanged.
 *
 * Coordinates. The client draws in its logical resolution. The module
 * rasterises at native resolution = logical * ui_scale (1, 2 or 3). A
 * logical vertex position c maps to the native position
 *
 *     n = c * ui_scale + 0.5
 *
 * The +0.5 is the half-pixel offset between Direct3D 7 (pixel centres on
 * integer coordinates) and Metal (centres on halves): positive, in native
 * pixels, added after scaling. With it a quad from (0,0) to (4,4) covers
 * exactly the native pixels [0, 4*ui_scale) in each direction, and at scale 1
 * every attribute is evaluated where Direct3D 7 evaluated it. The client's
 * start-up calibration (C3dDevice::TestScreen) is not needed: its texel
 * offset (CTexture::m_uOffset / m_vOffset) is 0.5 texel, the value the
 * client initialises it to.
 *
 * Sampling. BF_GFX_FILTER_NEAREST samples one texel and, at ui_scale > 1,
 * evaluates the texture coordinates at the centre of the logical pixel the
 * fragment belongs to, so an unfiltered draw comes out exactly as at scale 1
 * with every logical pixel a clean ui_scale x ui_scale block. LINEAR is
 * bilinear at native resolution (the world).
 *
 * Render target. One persistent colour target (8-bit BGRA, native size) and
 * a 32-bit float depth buffer. Its contents survive between frames, like a
 * DirectDraw back buffer presented by a blit. The target has no alpha the
 * client can see: alpha is never written and reads as 1 in blending
 * (BF_GFX_BLEND_DST_ALPHA = 1, INV_DST_ALPHA = 0), as on the client's 16- and
 * 32-bit display formats.
 *
 * Threads: every function is called on one thread, the client's main thread,
 * unless it says otherwise. Failure is a negative return value (or NULL) plus
 * a BFT_ERR or BFT_GFX trace line (port/instrument/bf_trace.h); nothing
 * aborts or shows an alert.
 */
#ifndef BF_GFX_H
#define BF_GFX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BF_GFX_OK         = 0,
	BF_GFX_E_ARG      = -1,  /* null pointer, size out of range, unknown enum value */
	BF_GFX_E_DEVICE   = -2,  /* no Metal device, shader or pipeline creation failed */
	BF_GFX_E_MEMORY   = -3,  /* a buffer or texture could not be allocated */
	BF_GFX_E_STATE    = -4,  /* call not valid now (e.g. present on an offscreen device) */
	BF_GFX_E_IO       = -5,  /* the file could not be written */
	BF_GFX_E_TIMEOUT  = -6   /* no drawable became available */
};

typedef struct bf_gfx_device  bf_gfx_device;
typedef struct bf_gfx_texture bf_gfx_texture;

typedef struct bf_gfx_rect {
	int x, y, w, h;
} bf_gfx_rect;

/* ---- device ----------------------------------------------------------- */

typedef struct bf_gfx_desc {
	/*
	 * The drawing target: a CAMetalLayer * (passed as void *, not retained
	 * beyond the device's life: the caller keeps the layer alive), or NULL
	 * for an offscreen device that only renders into its target and is read
	 * back. The module sets the layer's device, pixel format, colour space
	 * (sRGB) and drawable size; the caller sets its frame and contentsScale.
	 */
	void *metal_layer;
	int logical_width;    /* 1 .. 16384 / ui_scale */
	int logical_height;
	int ui_scale;         /* 1, 2 or 3 */
} bf_gfx_desc;

/* Creates the device and its target (cleared to opaque black, depth 1). */
bf_gfx_device *bf_gfx_create(const bf_gfx_desc *desc);

/* Waits for the GPU, then frees the device. Textures must be destroyed first. */
void bf_gfx_destroy(bf_gfx_device *dev);

/*
 * Changes the logical size and UI scale. The target is recreated and
 * cleared (contents are lost, as the client redraws every frame); textures
 * and state are kept. The layer's drawable size follows.
 */
int bf_gfx_resize(bf_gfx_device *dev, int logical_width, int logical_height, int ui_scale);

/* Any pointer may be NULL. Native = logical * ui_scale. */
void bf_gfx_get_size(const bf_gfx_device *dev, int *logical_width, int *logical_height,
                     int *ui_scale, int *native_width, int *native_height);

/* Largest texture edge (16384 on Apple GPUs). */
int bf_gfx_max_texture_size(const bf_gfx_device *dev);

/* The Metal device's name, for logs. Valid while the device lives. */
const char *bf_gfx_device_name(const bf_gfx_device *dev);

/* ---- textures --------------------------------------------------------- */

/*
 * CPU-side pixel formats, as the client fills them (16-bit formats are
 * little-endian 16-bit words, 32-bit is the word 0xAARRGGBB, i.e. bytes
 * B G R A). On the GPU every format is held as 8-bit BGRA; 5- and 6-bit
 * channels are widened by bit replication (v << 3 | v >> 2, v << 2 | v >> 4),
 * 4-bit channels by * 17, the 1-bit alpha to 0 or 255. Formats without alpha
 * read alpha 1.
 */
typedef enum {
	BF_GFX_FORMAT_A1R5G5B5 = 1,
	BF_GFX_FORMAT_A4R4G4B4 = 2,
	BF_GFX_FORMAT_R5G6B5   = 3,
	BF_GFX_FORMAT_A8R8G8B8 = 4
} bf_gfx_format;

/* Bytes per pixel of a format (2 or 4), 0 for an unknown one. */
int bf_gfx_format_bytes(bf_gfx_format format);

/*
 * Creates a texture of any size up to bf_gfx_max_texture_size (no power-of-
 * two rule; the client's own rounding stays the client's business).
 * mip_levels: 1 for none, 0 for the full chain. Mip levels are regenerated
 * from level 0 after every change, as the client's UpdateMipmap does.
 * Contents start as zero (transparent black).
 */
bf_gfx_texture *bf_gfx_texture_create(bf_gfx_device *dev, int width, int height,
                                      bf_gfx_format format, int mip_levels);
void bf_gfx_texture_destroy(bf_gfx_texture *tex);

void bf_gfx_texture_get_info(const bf_gfx_texture *tex, int *width, int *height,
                             bf_gfx_format *format, int *mip_levels);

/*
 * Copies a rectangle of pixels in the texture's own format into it.
 * pitch is the source row stride in bytes (0: w * bytes per pixel). Draws
 * issued before the call still see the old contents, later draws the new.
 */
int bf_gfx_texture_update(bf_gfx_texture *tex, int x, int y, int w, int h,
                          const void *pixels, int pitch);

/*
 * Lock-style access. Returns the texture's CPU copy (level 0, own format,
 * always current) and its row stride in bytes; read and write freely until
 * unlock. changed names the rectangle written: NULL for the whole texture,
 * a rectangle with w or h 0 for none (a read-only lock uploads nothing).
 * Locks do not nest.
 */
void *bf_gfx_texture_lock(bf_gfx_texture *tex, int *pitch);
int bf_gfx_texture_unlock(bf_gfx_texture *tex, const bf_gfx_rect *changed);

/* Fills a rectangle (NULL: all) with a value in the texture's format. */
int bf_gfx_texture_fill(bf_gfx_texture *tex, const bf_gfx_rect *rect, uint32_t value);

/*
 * Copies src_rect of src onto dst_rect of dst, stretching with nearest
 * sampling (DirectDraw Blt). Formats may differ (converted through 8-bit
 * ARGB). src and dst may be the same texture only if the rectangles do not
 * overlap.
 */
int bf_gfx_texture_copy(bf_gfx_texture *dst, const bf_gfx_rect *dst_rect,
                        const bf_gfx_texture *src, const bf_gfx_rect *src_rect);

/*
 * Overrides the stage filters whenever this texture is bound: 0 (default)
 * uses the stage state, BF_GFX_FILTER_NEAREST or _LINEAR forces min and mag
 * filter (mip filter is kept). For UI tiles and text bitmaps, which the
 * client samples with the same linear filter as the world.
 */
void bf_gfx_texture_set_filter_override(bf_gfx_texture *tex, int filter);

/* ---- state ------------------------------------------------------------ */

typedef enum {   /* Direct3D 7 D3DBLEND values */
	BF_GFX_BLEND_ZERO               = 1,
	BF_GFX_BLEND_ONE                = 2,
	BF_GFX_BLEND_SRC_COLOR          = 3,
	BF_GFX_BLEND_INV_SRC_COLOR      = 4,
	BF_GFX_BLEND_SRC_ALPHA          = 5,
	BF_GFX_BLEND_INV_SRC_ALPHA      = 6,
	BF_GFX_BLEND_DST_ALPHA          = 7,   /* always 1: the target has no alpha */
	BF_GFX_BLEND_INV_DST_ALPHA      = 8,   /* always 0 */
	BF_GFX_BLEND_DST_COLOR          = 9,
	BF_GFX_BLEND_INV_DST_COLOR      = 10,
	BF_GFX_BLEND_SRC_ALPHA_SAT      = 11,
	BF_GFX_BLEND_BOTH_SRC_ALPHA     = 12,  /* src SRC_ALPHA, dst INV_SRC_ALPHA; dst factor ignored */
	BF_GFX_BLEND_BOTH_INV_SRC_ALPHA = 13   /* src INV_SRC_ALPHA, dst SRC_ALPHA; dst factor ignored */
} bf_gfx_blend;

typedef enum {   /* D3DCMPFUNC */
	BF_GFX_CMP_NEVER = 1, BF_GFX_CMP_LESS, BF_GFX_CMP_EQUAL, BF_GFX_CMP_LESS_EQUAL,
	BF_GFX_CMP_GREATER, BF_GFX_CMP_NOT_EQUAL, BF_GFX_CMP_GREATER_EQUAL, BF_GFX_CMP_ALWAYS
} bf_gfx_cmp;

/* Winding as seen on the client's screen (y down). D3DCULL values. */
typedef enum {
	BF_GFX_CULL_NONE = 1,
	BF_GFX_CULL_CW   = 2,   /* drop triangles whose vertices run clockwise on screen */
	BF_GFX_CULL_CCW  = 3
} bf_gfx_cull;

typedef enum {   /* D3DTEXTUREOP; results are clamped to [0, 1] per stage */
	BF_GFX_OP_DISABLE                      = 1,  /* colour: this and later stages off; alpha: pass current */
	BF_GFX_OP_SELECT_ARG1                  = 2,
	BF_GFX_OP_SELECT_ARG2                  = 3,
	BF_GFX_OP_MODULATE                     = 4,
	BF_GFX_OP_MODULATE_2X                  = 5,
	BF_GFX_OP_MODULATE_4X                  = 6,
	BF_GFX_OP_ADD                          = 7,
	BF_GFX_OP_ADD_SIGNED                   = 8,   /* a1 + a2 - 0.5 */
	BF_GFX_OP_ADD_SIGNED_2X                = 9,
	BF_GFX_OP_SUBTRACT                     = 10,
	BF_GFX_OP_ADD_SMOOTH                   = 11,  /* a1 + a2 - a1 * a2 */
	BF_GFX_OP_BLEND_DIFFUSE_ALPHA          = 12,  /* a1 * d + a2 * (1 - d), d = diffuse alpha */
	BF_GFX_OP_BLEND_TEXTURE_ALPHA          = 13,  /* this stage's texture alpha */
	BF_GFX_OP_BLEND_FACTOR_ALPHA           = 14,  /* texture factor alpha */
	BF_GFX_OP_BLEND_TEXTURE_ALPHA_PM       = 15,  /* a1 + a2 * (1 - texture alpha) */
	BF_GFX_OP_BLEND_CURRENT_ALPHA          = 16,
	BF_GFX_OP_PREMODULATE                  = 17,  /* not supported: acts as SELECT_ARG1 */
	BF_GFX_OP_MODULATE_ALPHA_ADD_COLOR     = 18,  /* a1.rgb + a1.a * a2.rgb (colour only) */
	BF_GFX_OP_MODULATE_COLOR_ADD_ALPHA     = 19,  /* a1.rgb * a2.rgb + a1.a (colour only) */
	BF_GFX_OP_MODULATE_INV_ALPHA_ADD_COLOR = 20,  /* (1 - a1.a) * a2.rgb + a1.rgb (colour only) */
	BF_GFX_OP_MODULATE_INV_COLOR_ADD_ALPHA = 21,  /* (1 - a1.rgb) * a2.rgb + a1.a (colour only) */
	BF_GFX_OP_DOT_PRODUCT3                 = 24   /* 4 * dot(a1 - 0.5, a2 - 0.5), replicated */
	/* 22, 23 (bump mapping) are not supported: they act as DISABLE. The
	   colour-only operations used as an alpha operation act as MODULATE. */
} bf_gfx_op;

enum {   /* stage arguments (D3DTA): one selector, optionally with modifiers */
	BF_GFX_ARG_DIFFUSE         = 0,
	BF_GFX_ARG_CURRENT         = 1,   /* previous stage's result; diffuse in stage 0 */
	BF_GFX_ARG_TEXTURE         = 2,   /* this stage's texture */
	BF_GFX_ARG_FACTOR          = 3,   /* texture_factor */
	BF_GFX_ARG_SPECULAR        = 4,
	BF_GFX_ARG_COMPLEMENT      = 0x10,  /* 1 - x */
	BF_GFX_ARG_ALPHA_REPLICATE = 0x20   /* x.aaaa */
};

typedef enum {   /* D3DTEXTUREADDRESS */
	BF_GFX_ADDRESS_WRAP = 1, BF_GFX_ADDRESS_MIRROR, BF_GFX_ADDRESS_CLAMP, BF_GFX_ADDRESS_BORDER
} bf_gfx_address;

typedef enum {   /* D3DTFG / D3DTFN; see "Sampling" above */
	BF_GFX_FILTER_NEAREST = 1,
	BF_GFX_FILTER_LINEAR  = 2
} bf_gfx_filter;

typedef enum {   /* D3DTFP */
	BF_GFX_MIP_NONE = 1, BF_GFX_MIP_NEAREST, BF_GFX_MIP_LINEAR
} bf_gfx_mip;

/* Texture coordinates of a stage, and its sampler. */
typedef struct bf_gfx_stage_state {
	uint8_t color_op;      /* bf_gfx_op */
	uint8_t color_arg1;    /* BF_GFX_ARG_* */
	uint8_t color_arg2;
	uint8_t alpha_op;
	uint8_t alpha_arg1;
	uint8_t alpha_arg2;
	uint8_t texcoord_index;  /* 0 or 1; a set the vertex lacks reads (0, 0) */
	uint8_t address_u;       /* bf_gfx_address */
	uint8_t address_v;
	uint8_t mag_filter;      /* bf_gfx_filter */
	uint8_t min_filter;
	uint8_t mip_filter;      /* bf_gfx_mip; ignored for textures without mip levels */
	uint32_t border_color;   /* 0xAARRGGBB; exact for 0x00000000, 0xFF000000 and
	                            0xFFFFFFFF, anything else uses the nearest of these */
} bf_gfx_stage_state;

typedef struct bf_gfx_state {
	bf_gfx_stage_state stage[2];
	uint8_t blend_enable;        /* 0 or 1 */
	uint8_t src_blend;           /* bf_gfx_blend */
	uint8_t dst_blend;
	uint8_t alpha_test_enable;
	uint8_t alpha_func;          /* bf_gfx_cmp: (fragment alpha in 0..255) func alpha_ref */
	uint8_t alpha_ref;           /* 0..255 */
	uint8_t depth_enable;        /* 0: no test and no write */
	uint8_t depth_write;
	uint8_t depth_func;          /* bf_gfx_cmp: (fragment z) func (stored z) */
	uint8_t cull;                /* bf_gfx_cull */
	uint8_t fog_enable;          /* colour = lerp(fog_color, colour, specular alpha) */
	uint8_t specular_enable;     /* colour += specular rgb, before fog */
	uint32_t fog_color;          /* 0xAARRGGBB, alpha unused */
	uint32_t texture_factor;     /* 0xAARRGGBB, BF_GFX_ARG_FACTOR */
} bf_gfx_state;

/*
 * Fills a state with the Direct3D 7 device defaults: stage 0 MODULATE
 * texture x current for colour, SELECT_ARG1 texture for alpha; stage 1
 * disabled; texcoord index = stage; WRAP; NEAREST, no mips; no blending
 * (ONE, ZERO); no alpha test (ALWAYS, 0); depth on, write on, LESS_EQUAL;
 * CULL_CCW; no fog, no specular; fog colour 0; texture factor 0xFFFFFFFF.
 */
void bf_gfx_state_init(bf_gfx_state *state);

/* Copies the state used by the following draws. */
void bf_gfx_set_state(bf_gfx_device *dev, const bf_gfx_state *state);

/*
 * Binds a texture to stage 0 or 1. NULL binds nothing: the stage's texture
 * argument then reads opaque white (1, 1, 1, 1), which the client relies on
 * for untextured boxes (DrawBoxScreen uses a texture without a surface).
 * A destroyed texture is unbound automatically.
 */
void bf_gfx_set_texture(bf_gfx_device *dev, int stage, bf_gfx_texture *tex);

/* ---- drawing ---------------------------------------------------------- */

/*
 * Vertex layouts (memory order, all little-endian, no padding):
 *   BF_GFX_VERTEX_TL1, 32 bytes:  float x, y, z, rhw; uint32 diffuse, specular
 *                                 (0xAARRGGBB); float u0, v0
 *   BF_GFX_VERTEX_TL2, 40 bytes:  the same, then float u1, v1
 * x, y are logical pixels, z is depth in [0, 1], rhw = 1/w for perspective-
 * correct interpolation (values <= 0 are treated as 1). These are the
 * client's tlvertex3d and lmtlvertex3d.
 */
typedef enum {
	BF_GFX_VERTEX_TL1 = 1,
	BF_GFX_VERTEX_TL2 = 2
} bf_gfx_vertex_format;

typedef enum {   /* D3DPRIMITIVETYPE */
	BF_GFX_PRIM_POINT_LIST     = 1,
	BF_GFX_PRIM_LINE_LIST      = 2,
	BF_GFX_PRIM_LINE_STRIP     = 3,
	BF_GFX_PRIM_TRIANGLE_LIST  = 4,
	BF_GFX_PRIM_TRIANGLE_STRIP = 5,
	BF_GFX_PRIM_TRIANGLE_FAN   = 6
} bf_gfx_prim;

/*
 * Draws vertices from caller memory with the current state and textures.
 * The data is copied before the call returns. Works inside or outside
 * begin/end frame (the module opens what it needs).
 */
int bf_gfx_draw(bf_gfx_device *dev, bf_gfx_prim prim, bf_gfx_vertex_format format,
                const void *vertices, int vertex_count);

/* Same with 16-bit indices into vertices[0 .. vertex_count). */
int bf_gfx_draw_indexed(bf_gfx_device *dev, bf_gfx_prim prim, bf_gfx_vertex_format format,
                        const void *vertices, int vertex_count,
                        const uint16_t *indices, int index_count);

enum {
	BF_GFX_CLEAR_COLOR = 1,
	BF_GFX_CLEAR_DEPTH = 2
};

/*
 * Clears colour (0xAARRGGBB, alpha ignored) and/or depth inside a logical
 * rectangle (NULL: the whole target), ignoring all state.
 */
int bf_gfx_clear(bf_gfx_device *dev, unsigned flags, uint32_t color, float depth,
                 const bf_gfx_rect *rect);

/* ---- frames ----------------------------------------------------------- */

/* Marks the start of a frame. Optional; draws outside a frame work too. */
int bf_gfx_begin_frame(bf_gfx_device *dev);

/* Submits everything recorded so far to the GPU, without waiting. */
int bf_gfx_end_frame(bf_gfx_device *dev);

/*
 * Shows the target in the layer: waits for the layer's next drawable (with
 * display sync this paces the caller to the display refresh), copies the
 * target into it and presents it. Submits pending work first. On an
 * offscreen device it only submits and returns BF_GFX_E_STATE.
 * Returns BF_GFX_E_TIMEOUT if no drawable came within a second.
 */
int bf_gfx_present(bf_gfx_device *dev);

/* Display sync of the layer (on by default). */
void bf_gfx_set_vsync(bf_gfx_device *dev, int enabled);

/* Submits pending work and waits until the GPU has finished it. */
int bf_gfx_finish(bf_gfx_device *dev);

/* ---- read-back and writes into the target ------------------------------ */

typedef enum {
	BF_GFX_SPACE_LOGICAL = 1,  /* rectangles in logical pixels; a logical pixel
	                              reads as the average of its native block,
	                              writes fill its whole block */
	BF_GFX_SPACE_NATIVE  = 2
} bf_gfx_space;

/*
 * Reads a rectangle of the target as 32-bit BGRA (bytes B G R A, alpha 255)
 * into dst with the given row stride (0: w * 4). Waits for the GPU. Nothing
 * is ever read back unless this (or capture) is called.
 */
int bf_gfx_read_pixels(bf_gfx_device *dev, bf_gfx_space space, const bf_gfx_rect *rect,
                       void *dst, int pitch);

enum {
	BF_GFX_WRITE_SKIP_TRANSPARENT = 1   /* pixels with alpha 0 leave the target as it is */
};

/*
 * Writes 32-bit BGRA pixels into a rectangle of the target, ordered with the
 * draws around it (Bink frames, screen effects, text drawn straight onto the
 * back buffer). Depth is not touched.
 */
int bf_gfx_write_pixels(bf_gfx_device *dev, bf_gfx_space space, const bf_gfx_rect *rect,
                        const void *src, int pitch, unsigned flags);

/* Copies the target to a backup and back (C3dDevice::BackupFrame / RestoreFrame). */
int bf_gfx_backup_frame(bf_gfx_device *dev);
int bf_gfx_restore_frame(bf_gfx_device *dev);

/* Writes the whole target at native resolution as a PNG (sRGB) through ImageIO. */
int bf_gfx_capture_png(bf_gfx_device *dev, const char *path);

/* Statistics since creation, for traces and tests. Any pointer may be NULL. */
void bf_gfx_get_stats(const bf_gfx_device *dev, long long *draws, long long *pipelines,
                      long long *readbacks, long long *frames);

#ifdef __cplusplus
}
#endif

#endif
