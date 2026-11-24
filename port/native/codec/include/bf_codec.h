/*
 * codec: zlib and JPEG for the client.
 *
 * Replaces two Windows DLLs of the original client: the zlib build imported
 * as cps.dll (compress / uncompress) and the Intel JPEG Library (ijl15.dll).
 * zlib is the system library; JPEG goes through ImageIO.
 *
 * Every function may be called from any thread; there is no shared state.
 * Failure is a negative return value plus a BFT_ERR trace line (bf_trace.h);
 * nothing aborts. Lengths are size_t: the 32-bit DWORD of the Windows
 * prototypes is the adapter's concern, not this header's.
 *
 * Pixel layouts (memory order, first byte first):
 *   BF_CODEC_BGR24   B G R              3 bytes per pixel
 *   BF_CODEC_BGRA32  B G R A            4 bytes per pixel; as a little-endian
 *                                       32-bit word 0xAARRGGBB, which is the
 *                                       client's PACK32BIT8888 layout.
 * Rows run top to bottom; the caller gives the row stride in bytes (0 means
 * width * bytes per pixel). Decoded colours are the JPEG's own RGB values:
 * no colour management is applied, as with the Intel library. Decoded alpha
 * is always 255; alpha is ignored on encoding.
 */
#ifndef BF_CODEC_H
#define BF_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BF_CODEC_OK         = 0,
	BF_CODEC_E_ARG      = -1,  /* null pointer, zero size, unknown format, stride too small */
	BF_CODEC_E_DATA     = -2,  /* corrupt or truncated stream, not a JPEG */
	BF_CODEC_E_BUFFER   = -3,  /* destination too small; the buffer holds a truncated result */
	BF_CODEC_E_MEMORY   = -4,
	BF_CODEC_E_IO       = -5   /* the file could not be written */
};

typedef enum {
	BF_CODEC_BGR24  = 1,
	BF_CODEC_BGRA32 = 2
} bf_codec_pixel_format;

/* ---- zlib ------------------------------------------------------------ */

/*
 * Inflates a zlib stream (RFC 1950) of src_len bytes into dst.
 * *dst_len is the capacity of dst on entry and the number of bytes written
 * on return (also after BF_CODEC_E_BUFFER, when it equals the capacity).
 * dst may be NULL only if *dst_len is 0 (an empty result is expected).
 * Bytes after the end of the stream are ignored.
 * Returns BF_CODEC_OK, BF_CODEC_E_DATA (corrupt or truncated input),
 * BF_CODEC_E_BUFFER (output does not fit) or BF_CODEC_E_ARG.
 */
int bf_codec_inflate(void *dst, size_t *dst_len, const void *src, size_t src_len);

/* An upper bound for the deflated size of src_len bytes. */
size_t bf_codec_deflate_bound(size_t src_len);

/*
 * Deflates src into a zlib stream. *dst_len is the capacity on entry (use
 * bf_codec_deflate_bound) and the size written on return. level is 0..9, or
 * -1 for the default. Returns BF_CODEC_OK, BF_CODEC_E_BUFFER or BF_CODEC_E_ARG.
 * An empty source (src_len 0, src may be NULL) is valid.
 */
int bf_codec_deflate(void *dst, size_t *dst_len, const void *src, size_t src_len, int level);

/* ---- JPEG ------------------------------------------------------------ */

/* Reads width and height from the JPEG header without decoding. */
int bf_codec_jpeg_info(const void *data, size_t size, int *width, int *height);

/*
 * Decodes a JPEG (baseline or progressive; grey, RGB or CMYK) into pixels.
 * width and height must be what bf_codec_jpeg_info reported; the buffer must
 * hold height rows of stride bytes. Returns BF_CODEC_OK, BF_CODEC_E_DATA,
 * BF_CODEC_E_ARG (size mismatch, stride too small) or BF_CODEC_E_MEMORY.
 */
int bf_codec_jpeg_decode(const void *data, size_t size, void *pixels,
                         int width, int height, size_t stride,
                         bf_codec_pixel_format format);

/*
 * Encodes pixels as a JPEG file at path (UTF-8), replacing it. quality is
 * 1..100 (the Intel library's default was 75). Returns BF_CODEC_OK,
 * BF_CODEC_E_ARG, BF_CODEC_E_IO or BF_CODEC_E_MEMORY.
 */
int bf_codec_jpeg_encode_file(const char *path, const void *pixels,
                              int width, int height, size_t stride,
                              bf_codec_pixel_format format, int quality);

/*
 * Encodes pixels as a JPEG in memory. On success *out points to *out_size
 * bytes that the caller releases with bf_codec_free.
 */
int bf_codec_jpeg_encode_mem(void **out, size_t *out_size, const void *pixels,
                             int width, int height, size_t stride,
                             bf_codec_pixel_format format, int quality);

void bf_codec_free(void *p);

#ifdef __cplusplus
}
#endif

#endif
