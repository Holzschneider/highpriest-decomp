# codec

zlib and JPEG for the client, replacing `cps.dll` (zlib under another name) and `ijl15.dll` (Intel JPEG Library), neither of which exists for arm64. The header `include/bf_codec.h` is the contract.

| Function | Replaces |
|---|---|
| `bf_codec_inflate`, `bf_codec_deflate`, `bf_codec_deflate_bound` | `uncompress`, `compress` (system zlib; lengths are `size_t`, not the 32-bit `DWORD`) |
| `bf_codec_jpeg_info` | `ijlRead(IJL_JBUFF_READPARAMS)` (size query) |
| `bf_codec_jpeg_decode` | `ijlRead(IJL_JBUFF_READWHOLEIMAGE)`, to BGR24 or BGRA32 |
| `bf_codec_jpeg_encode_file`, `_encode_mem` | `ijlWrite(IJL_JFILE_WRITEWHOLEIMAGE)` (screenshots) |

Implementation: `src/bf_codec_zlib.cpp` (system zlib, `uncompress2`/`compress2`) and `src/bf_codec_jpeg.mm` (ImageIO and CoreGraphics). Decoded images are retagged as device RGB or grey before drawing, so values are the JPEG's own, with no colour management (the Intel library had none). Any thread may call any function.

## Call sites in the client

Under `BF_PORT`: `src/resource/Bitmap.cpp` `CBitmapRes::LoadJPGData` (`bf_codec_jpeg_info`, `bf_codec_jpeg_decode` to BGR24, unpadded rows), `src/3dlib/Renderer.cpp` `CRenderer::SaveJPG` (`bf_codec_jpeg_encode_file`, quality 75, the path resolved for writing with `bf_file_resolve`), and the client's `uncompress`/`compress`, which `src/common.h` maps to `bf_zlib_uncompress`/`bf_zlib_compress` in `port/compat/stubs/bf_zlib.cpp` (32-bit lengths onto `bf_codec_inflate`/`bf_codec_deflate`).

## Test

    cmake -S port/native/codec -B build/native/codec -G Ninja && cmake --build build/native/codec && build/native/codec/bifrost-codec-test

Runs without game data. With `data.grf` in `BF_CLIENT_DIR` (default `~/Games/Bifrost`) it also reads the archive's file table, inflates 40 entries and decodes the first `.jpg`; otherwise those lines say `skipped`. `BF_TEST_OUT` names the directory for the one JPEG file it writes (default: the current directory).

## Not covered

- Truncated JPEG data returns `BF_CODEC_E_DATA`; the client's `EPISODE_VER` build tolerated a failed `ijlRead` and used whatever had been decoded. The adapter decides whether that matters.
- CMYK and 12-bit JPEGs are left to ImageIO and untested.
- ImageIO's quantisation differs from the Intel library's, so decoded pixels are close to, not identical with, the original client's.
