#include "bf_codec.h"
#include "bf_trace.h"

#include <zlib.h>

extern "C" int bf_codec_inflate(void *dst, size_t *dst_len, const void *src, size_t src_len)
{
	if (!dst_len || (!dst && *dst_len) || (!src && src_len)) {
		BF_TRACE(BFT_ERR, "codec: inflate: bad argument");
		return BF_CODEC_E_ARG;
	}
	uLongf out = (uLongf)*dst_len;
	uLong in = (uLong)src_len;
	int r = uncompress2((Bytef *)dst, &out, (const Bytef *)src, &in);
	*dst_len = (size_t)out;
	switch (r) {
	case Z_OK:
		return BF_CODEC_OK;
	case Z_BUF_ERROR:
		BF_TRACE(BFT_ERR, "codec: inflate: output buffer of %zu bytes too small", *dst_len);
		return BF_CODEC_E_BUFFER;
	case Z_MEM_ERROR:
		BF_TRACE(BFT_ERR, "codec: inflate: out of memory");
		return BF_CODEC_E_MEMORY;
	default:
		BF_TRACE(BFT_ERR, "codec: inflate: corrupt stream (zlib %d, %zu input bytes)", r, src_len);
		return BF_CODEC_E_DATA;
	}
}

extern "C" size_t bf_codec_deflate_bound(size_t src_len)
{
	return (size_t)compressBound((uLong)src_len);
}

extern "C" int bf_codec_deflate(void *dst, size_t *dst_len, const void *src, size_t src_len, int level)
{
	static const Bytef none = 0;
	if (!dst || !dst_len || (!src && src_len) || level < -1 || level > 9) {
		BF_TRACE(BFT_ERR, "codec: deflate: bad argument");
		return BF_CODEC_E_ARG;
	}
	uLongf out = (uLongf)*dst_len;
	int r = compress2((Bytef *)dst, &out, src ? (const Bytef *)src : &none, (uLong)src_len, level);
	if (r == Z_OK) {
		*dst_len = (size_t)out;
		return BF_CODEC_OK;
	}
	BF_TRACE(BFT_ERR, "codec: deflate: zlib %d (capacity %zu, input %zu)", r, *dst_len, src_len);
	return r == Z_MEM_ERROR ? BF_CODEC_E_MEMORY : BF_CODEC_E_BUFFER;
}
