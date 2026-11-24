// A minimal PNG writer on zlib, for the test's sample picture (no Apple
// headers here: they clash with the Windows headers the test compiles with).
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <zlib.h>

namespace {

void Be32(std::vector<unsigned char> &v, uint32_t x)
{
	v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

void Chunk(FILE *f, const char *type, const std::vector<unsigned char> &data)
{
	std::vector<unsigned char> head;
	Be32(head, (uint32_t)data.size());
	fwrite(head.data(), 1, 4, f);
	uLong crc = crc32(0, (const Bytef *)type, 4);
	if (!data.empty()) crc = crc32(crc, data.data(), (uInt)data.size());
	fwrite(type, 1, 4, f);
	if (!data.empty()) fwrite(data.data(), 1, data.size(), f);
	std::vector<unsigned char> tail;
	Be32(tail, (uint32_t)crc);
	fwrite(tail.data(), 1, 4, f);
}

} // namespace

// Pixels are uint32_t 0xAARRGGBB, top-down, `stride` bytes per row. Returns 0.
int bf_gdi_test_write_png(const char *path, const void *pixels, int width, int height, int stride)
{
	std::vector<unsigned char> raw;
	raw.reserve((size_t)height * (width * 4 + 1));
	for (int y = 0; y < height; ++y) {
		raw.push_back(0);   // filter: none
		const uint32_t *row = (const uint32_t *)((const char *)pixels + (size_t)y * stride);
		for (int x = 0; x < width; ++x) {
			uint32_t p = row[x];
			raw.push_back((p >> 16) & 0xFF); raw.push_back((p >> 8) & 0xFF);
			raw.push_back(p & 0xFF); raw.push_back(p >> 24);
		}
	}
	uLongf size = compressBound((uLong)raw.size());
	std::vector<unsigned char> z(size);
	if (compress2(z.data(), &size, raw.data(), (uLong)raw.size(), 9) != Z_OK) return -1;
	z.resize(size);
	FILE *f = fopen(path, "wb");
	if (!f) return -1;
	static const unsigned char sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	fwrite(sig, 1, 8, f);
	std::vector<unsigned char> ihdr;
	Be32(ihdr, width); Be32(ihdr, height);
	ihdr.push_back(8); ihdr.push_back(6); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
	Chunk(f, "IHDR", ihdr);
	Chunk(f, "IDAT", z);
	Chunk(f, "IEND", {});
	return fclose(f) == 0 ? 0 : -1;
}
