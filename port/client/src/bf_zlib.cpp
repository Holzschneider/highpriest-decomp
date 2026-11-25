// uncompress and compress with the client's DWORD lengths, on the codec
// module (technical plan 3.3, "Replaced outright": zlib). common.h declared
// them with DWORD *, which is 32 bits; the system zlib's uLongf is 64 bits
// on macOS, so linking the client's declarations straight against libz would
// let zlib write 8 bytes into a 4-byte length. common.h maps both names to
// these under BF_PORT.
#include <windows.h>
#include <limits.h>
#include <type_traits>

#include "bf_codec.h"
#include "bf_port.h"
#include "bf_trace.h"

static_assert(std::is_same<DWORD, unsigned int>::value, "bf_port.h declares DWORD as unsigned int");

namespace {

// zlib's return codes, which the client compares with 0.
const int kZOk = 0, kZDataError = -3, kZMemError = -4, kZBufError = -5;

int ZlibCode(int codec)
{
	switch (codec) {
	case BF_CODEC_OK:       return kZOk;
	case BF_CODEC_E_BUFFER: return kZBufError;
	case BF_CODEC_E_MEMORY: return kZMemError;
	default:                return kZDataError;
	}
}

}  // namespace

extern "C" int bf_zlib_uncompress(unsigned char *dest, unsigned int *dest_len, const unsigned char *source,
                                  unsigned int source_len)
{
	if (!dest_len)
		return kZBufError;
	size_t len = *dest_len;
	int r = bf_codec_inflate(dest, &len, source, source_len);
	*dest_len = (unsigned int)len;
	return ZlibCode(r);
}

extern "C" int bf_zlib_compress(unsigned char *dest, unsigned int *dest_len, const unsigned char *source,
                                unsigned int source_len)
{
	if (!dest_len)
		return kZBufError;
	size_t len = *dest_len;
	int r = bf_codec_deflate(dest, &len, source, source_len, -1);
	*dest_len = len > UINT_MAX ? UINT_MAX : (unsigned int)len;
	return ZlibCode(r);
}
