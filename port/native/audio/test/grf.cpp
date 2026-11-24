#include "grf.h"

#include <string.h>
#include <zlib.h>

static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

bool Grf::open(const char *path, std::string *error)
{
	f = fopen(path, "rb");
	if (!f) { *error = "cannot open"; return false; }
	uint8_t head[46];
	if (fread(head, 1, 46, f) != 46 || memcmp(head, "Master of Magic", 15) != 0) { *error = "not a GRF archive"; return false; }
	uint32_t table = rd32(head + 30), version = rd32(head + 42);
	if (version >> 8 != 2) { *error = "not version 0x200"; return false; }
	if (fseeko(f, 46 + (off_t)table, SEEK_SET) != 0) { *error = "bad table offset"; return false; }
	uint8_t sizes[8];
	if (fread(sizes, 1, 8, f) != 8) { *error = "short table"; return false; }
	uLongf packed = rd32(sizes), real = rd32(sizes + 4);
	std::vector<uint8_t> z(packed), raw(real);
	if (fread(z.data(), 1, packed, f) != packed || uncompress(raw.data(), &real, z.data(), packed) != Z_OK) {
		*error = "table does not inflate";
		return false;
	}
	size_t at = 0;
	while (at < real) {
		const uint8_t *p = raw.data() + at;
		size_t len = strnlen((const char *)p, real - at);
		if (at + len + 18 > real) break;
		GrfEntry e;
		e.name.assign((const char *)p, len);
		const uint8_t *q = p + len + 1;
		e.packed = rd32(q); e.aligned = rd32(q + 4); e.size = rd32(q + 8); e.flags = q[12]; e.offset = rd32(q + 13);
		if (e.flags & 1) entries.push_back(e);
		at += len + 18;
	}
	return true;
}

bool Grf::read(const GrfEntry &e, std::vector<uint8_t> &out, std::string *error)
{
	if (e.des()) { *error = "DES-encrypted"; return false; }
	std::vector<uint8_t> z(e.aligned);
	if (fseeko(f, 46 + (off_t)e.offset, SEEK_SET) != 0 || fread(z.data(), 1, e.aligned, f) != e.aligned) { *error = "short read"; return false; }
	out.resize(e.size);
	z_stream zs;
	memset(&zs, 0, sizeof zs);
	if (inflateInit(&zs) != Z_OK) { *error = "zlib"; return false; }
	zs.next_in = z.data();
	zs.avail_in = e.packed;
	zs.next_out = out.data();
	zs.avail_out = e.size;
	int r = inflate(&zs, Z_FINISH);
	size_t got = e.size - zs.avail_out;
	inflateEnd(&zs);
	out.resize(got);
	if (r == Z_STREAM_END) return true;
	// Some entries hold a zlib stream cut short; keep what inflates, as the
	// game's own reader does.
	if (got > 0) { truncated++; return true; }
	*error = "does not inflate";
	return false;
}
