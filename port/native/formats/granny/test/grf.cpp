// GRF 0x200 reading for the test program. The archive's "DES" is one DES
// round with an all-zero key (the key schedule of the original never sets
// a bit), applied to the first 20 blocks and then to every n-th block, with
// a byte shuffle on every 8th of the others (flag 2), or to the first 20
// blocks only (flag 4). Tables are those of FIPS 46.

#include "grf.h"

#include <string.h>
#include <zlib.h>

namespace {

const uint8_t kIP[64] = {
	58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4, 62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
	57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3, 61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7,
};
const uint8_t kFP[64] = {
	40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31, 38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
	36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27, 34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25,
};
const uint8_t kE[48] = {
	32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
	16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1,
};
const uint8_t kP[32] = {
	16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10, 2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11, 4, 25,
};
const uint8_t kS[8][4][16] = {
	{ {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7}, {0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8},
	  {4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0}, {15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13} },
	{ {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10}, {3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5},
	  {0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15}, {13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9} },
	{ {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8}, {13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1},
	  {13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7}, {1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12} },
	{ {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15}, {13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9},
	  {10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4}, {3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14} },
	{ {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9}, {14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6},
	  {4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14}, {11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3} },
	{ {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11}, {10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8},
	  {9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6}, {4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13} },
	{ {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1}, {13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6},
	  {1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2}, {6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12} },
	{ {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7}, {1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2},
	  {7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8}, {2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11} },
};

inline int Bit(const uint8_t *b, int i) { return (b[i >> 3] >> (7 - (i & 7))) & 1; }
inline void SetBit(uint8_t *b, int i) { b[i >> 3] |= (uint8_t)(0x80 >> (i & 7)); }

void Permute(const uint8_t *in, const uint8_t *table, int n, uint8_t *out)
{
	memset(out, 0, (n + 7) / 8);
	for (int i = 0; i < n; ++i)
		if (Bit(in, table[i] - 1))
			SetBit(out, i);
}

// IP, one round with a zero subkey (L ^= f(R)), FP.
void DecryptBlock(uint8_t *block)
{
	uint8_t t[8], e[6], s[4], p[4];
	Permute(block, kIP, 64, t);
	Permute(t + 4, kE, 48, e);
	memset(s, 0, 4);
	for (int box = 0; box < 8; ++box) {
		int v = 0;
		for (int k = 0; k < 6; ++k)
			v = (v << 1) | Bit(e, box * 6 + k);
		int row = ((v >> 4) & 2) | (v & 1), col = (v >> 1) & 15;
		s[box >> 1] |= (uint8_t)(kS[box][row][col] << ((box & 1) ? 0 : 4));
	}
	Permute(s, kP, 32, p);
	for (int i = 0; i < 4; ++i)
		t[i] ^= p[i];
	Permute(t, kFP, 64, block);
}

uint8_t SwapByte(uint8_t b)
{
	static const uint8_t pairs[][2] = { {0, 43}, {185, 192}, {254, 235}, {108, 128}, {1, 104}, {119, 72}, {96, 255} };
	for (const auto &p : pairs) {
		if (b == p[0]) return p[1];
		if (b == p[1]) return p[0];
	}
	return b;
}

void Decrypt(std::vector<uint8_t> &data, uint32_t packed, uint8_t flags)
{
	size_t blocks = data.size() / 8;
	if (flags & 2) {
		int digits = 0;
		for (uint32_t v = packed; v; v /= 10)
			++digits;
		int cycle = digits < 3 ? 1 : digits < 5 ? digits + 1 : digits < 7 ? digits + 9 : digits + 15;
		int k = 0;
		for (size_t i = 0; i < blocks; ++i) {
			uint8_t *b = &data[i * 8];
			if (i < 20 || i % cycle == 0) {
				DecryptBlock(b);
			} else {
				if (k == 7) {
					uint8_t in[8];
					memcpy(in, b, 8);
					b[0] = in[3]; b[1] = in[4]; b[2] = in[6];
					b[3] = in[0]; b[4] = in[1]; b[5] = in[2];
					b[6] = in[5]; b[7] = SwapByte(in[7]);
					k = 0;
				}
				++k;
			}
		}
	} else if (flags & 4) {
		for (size_t i = 0; i < blocks && i < 20; ++i)
			DecryptBlock(&data[i * 8]);
	}
}

uint32_t Get32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

} // namespace

Grf::~Grf()
{
	if (m_fp)
		fclose(m_fp);
}

bool Grf::Open(const std::string &path)
{
	m_fp = fopen(path.c_str(), "rb");
	if (!m_fp)
		return false;
	uint8_t head[46];
	if (fread(head, 1, 46, m_fp) != 46 || memcmp(head, "Master of Magic", 15) != 0)
		return false;
	uint32_t tableOffset = Get32(head + 30), version = Get32(head + 42);
	if ((version >> 8) != 2)
		return false;
	fseeko(m_fp, 46 + (off_t)tableOffset, SEEK_SET);
	uint8_t sizes[8];
	if (fread(sizes, 1, 8, m_fp) != 8)
		return false;
	uLongf packed = Get32(sizes), size = Get32(sizes + 4);
	std::vector<uint8_t> in(packed), table(size);
	if (fread(in.data(), 1, packed, m_fp) != packed || uncompress(table.data(), &size, in.data(), packed) != Z_OK)
		return false;
	size_t at = 0;
	while (at < size) {
		const uint8_t *end = (const uint8_t *)memchr(&table[at], 0, size - at);
		if (!end || (size_t)(end - table.data()) + 18 > size)
			break;
		std::string name((const char *)&table[at], end - &table[at]);
		const uint8_t *e = end + 1;
		Entry entry = { Get32(e), Get32(e + 4), Get32(e + 8), Get32(e + 13), e[12] };
		if (entry.flags & 1) {
			for (char &c : name)
				if (c >= 'A' && c <= 'Z')
					c = (char)(c - 'A' + 'a');
			m_entries[name] = entry;
		}
		at = (end - table.data()) + 18;
	}
	return true;
}

std::vector<std::string> Grf::Names(const std::string &suffix) const
{
	std::vector<std::string> out;
	for (const auto &e : m_entries)
		if (e.first.size() >= suffix.size() && e.first.compare(e.first.size() - suffix.size(), suffix.size(), suffix) == 0)
			out.push_back(e.first);
	return out;
}

bool Grf::Read(const std::string &name, std::vector<uint8_t> &out, int *flags) const
{
	auto it = m_entries.find(name);
	if (it == m_entries.end())
		return false;
	const Entry &e = it->second;
	if (flags)
		*flags = e.flags;
	std::vector<uint8_t> raw(e.aligned);
	fseeko(m_fp, 46 + (off_t)e.offset, SEEK_SET);
	if (fread(raw.data(), 1, e.aligned, m_fp) != e.aligned)
		return false;
	if (e.flags & 6)
		Decrypt(raw, e.packed, e.flags);
	out.resize(e.size);
	uLongf size = e.size;
	if (uncompress(out.data(), &size, raw.data(), e.packed) != Z_OK || size != e.size)
		return false;
	return true;
}
