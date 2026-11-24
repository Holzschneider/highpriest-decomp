// Bink 1 video: one frame record into the Y, Cb, Cr (and alpha) planes.
// The bitstream is described in ../FORMAT.md, section "Video".
#include <math.h>

#include <algorithm>
#include <mutex>

#include "bf_bink_internal.h"
#include "bf_trace.h"

using namespace bf_bink_tables;

namespace bf_bink_impl {

namespace {

// Decoding table of a prefix code: indexed by the next 7 bits of the
// stream (first bit in the lowest position), it gives the symbol's leaf and
// the length of its code.
struct CodeEntry {
	uint8_t leaf;
	uint8_t len;
};
CodeEntry g_codes[16][128];

void build_codes()
{
	for (int t = 0; t < 16; ++t) {
		for (int leaf = 0; leaf < 16; ++leaf) {
			const char *bits = kTreeCodes[t][leaf];
			int len = (int)strlen(bits);
			int prefix = 0;
			for (int i = 0; i < len; ++i)
				if (bits[i] == '1')
					prefix |= 1 << i;
			for (int rest = 0; rest < (1 << (7 - len)); ++rest) {
				CodeEntry &e = g_codes[t][prefix | (rest << len)];
				e.leaf = (uint8_t)leaf;
				e.len = (uint8_t)len;
			}
		}
	}
}

std::once_flag g_codes_once;

int floor_log2(unsigned v)
{
	int n = -1;
	while (v) {
		v >>= 1;
		++n;
	}
	return n;
}

// Reads which of the 16 codes a bundle uses and how its leaves map to
// symbols: either a short list of symbols placed first (the rest follow in
// ascending order), or a series of merge passes over the identity order.
void read_tree(BitReader &br, Tree &tree)
{
	tree.code = (int)br.get(4);
	for (int i = 0; i < 16; ++i)
		tree.syms[i] = (uint8_t)i;
	if (tree.code == 0)
		return;
	if (br.bit()) {
		int count = (int)br.get(3) + 1;
		bool used[16] = {};
		int n = 0;
		for (; n < count; ++n) {
			int s = (int)br.get(4);
			tree.syms[n] = (uint8_t)s;
			used[s] = true;
		}
		for (int s = 0; s < 16 && n < 16; ++s)
			if (!used[s])
				tree.syms[n++] = (uint8_t)s;
	} else {
		int passes = (int)br.get(2) + 1;
		uint8_t a[16], b[16];
		for (int i = 0; i < 16; ++i)
			a[i] = (uint8_t)i;
		uint8_t *in = a, *out = b;
		for (int p = 0; p < passes; ++p) {
			int size = 1 << p;
			for (int start = 0; start < 16; start += 2 * size) {
				const uint8_t *x = in + start, *y = in + start + size;
				int nx = size, ny = size;
				uint8_t *o = out + start;
				while (nx && ny) {
					if (br.bit()) {
						*o++ = *y++;
						--ny;
					} else {
						*o++ = *x++;
						--nx;
					}
				}
				while (nx--)
					*o++ = *x++;
				while (ny--)
					*o++ = *y++;
			}
			std::swap(in, out);
		}
		memcpy(tree.syms, in, 16);
	}
}

inline int mul11(int a, int b) { return (int)(((int64_t)a * b) >> 11); }

// The format's 8-point inverse transform (integer, scaled DCT): s and d with
// a stride of `ss` and `ds` elements.
inline void idct8(const int *s, int ss, int *d, int ds)
{
	const int a0 = s[0] + s[4 * ss];
	const int a1 = s[0] - s[4 * ss];
	const int a2 = s[2 * ss] + s[6 * ss];
	const int a3 = mul11(2896, s[2 * ss] - s[6 * ss]);
	const int a4 = s[5 * ss] + s[3 * ss];
	const int a5 = s[5 * ss] - s[3 * ss];
	const int a6 = s[1 * ss] + s[7 * ss];
	const int a7 = s[1 * ss] - s[7 * ss];
	const int b0 = a4 + a6;
	const int b1 = mul11(3784, a5 + a7);
	const int b2 = mul11(-5352, a5) - b0 + b1;
	const int b3 = mul11(2896, a6 - a4) - b2;
	const int b4 = mul11(2217, a7) + b3 - b1;
	d[0 * ds] = a0 + a2 + b0;
	d[1 * ds] = a1 + a3 - a2 + b2;
	d[2 * ds] = a1 - a3 + a2 + b3;
	d[3 * ds] = a0 - a2 - b4;
	d[4 * ds] = a0 - a2 + b4;
	d[5 * ds] = a1 - a3 + a2 - b3;
	d[6 * ds] = a1 + a3 - a2 - b2;
	d[7 * ds] = a0 + a2 - b0;
}

// Columns first, then rows; the result is rounded to pixel units.
void idct_block(const int in[64], int out[64])
{
	int tmp[64];
	for (int c = 0; c < 8; ++c)
		idct8(in + c, 8, tmp + c, 8);
	for (int r = 0; r < 8; ++r) {
		int row[8];
		idct8(tmp + 8 * r, 1, row, 1);
		for (int c = 0; c < 8; ++c)
			out[8 * r + c] = (row[c] + 0x7F) >> 8;
	}
}

// Pixel results are taken modulo 256, not clamped: the encoder relies on it
// (blocks whose transform yields -56 are meant as 200; FORMAT.md, "Pixel
// arithmetic").
inline uint8_t wrap8(int v) { return (uint8_t)v; }

// The coefficient position list shared by DCT and residue blocks. Entries
// are visited in order; a set bit opens an entry:
//   Chain(c)  -> the 4 coefficients at c are coded, the entry becomes Split(c+4)
//   Split(c)  -> becomes Group(c); Group(c+4), Group(c+8), Group(c+12) are appended
//   Group(c)  -> the 4 coefficients at c are coded, the entry closes
//   Single(c) -> coefficient c is coded, the entry closes
// Inside a group, a set bit defers a coefficient as Single (prepended, so it
// is seen from the next bit plane on); a clear bit codes it at once.
enum Kind : uint8_t { kDead, kChain, kSplit, kGroup, kSingle };

struct CoefList {
	uint8_t kind[160];
	uint8_t pos[160];
	int start = 72, end = 72;
	void append(Kind k, int p) {
		if (end < 160) {
			kind[end] = k;
			pos[end++] = (uint8_t)p;
		}
	}
	void prepend(Kind k, int p) {
		if (start > 0) {
			kind[--start] = k;
			pos[start] = (uint8_t)p;
		}
	}
};

// Reads the AC coefficients of an intra or inter DCT block. Writes their
// values at their natural positions and their scan indices into coded[].
// Returns the number coded.
int read_dct_coeffs(BitReader &br, int block[64], int coded[64])
{
	CoefList list;
	list.append(kChain, 4);
	list.append(kChain, 24);
	list.append(kChain, 44);
	list.append(kSingle, 1);
	list.append(kSingle, 2);
	list.append(kSingle, 3);
	int count = 0;

	auto put = [&](int idx, int bits) {
		int v;
		if (bits == 0) {
			v = br.bit() ? -1 : 1;
		} else {
			v = (int)br.get(bits) | (1 << bits);
			if (br.bit())
				v = -v;
		}
		if (idx < 64 && count < 64) {
			block[kScan[idx]] = v;
			coded[count++] = idx;
		}
	};

	for (int bits = (int)br.get(4) - 1; bits >= 0; --bits) {
		int i = list.start;
		while (i < list.end) {
			if (list.kind[i] == kDead || !br.bit()) {
				++i;
				continue;
			}
			int c = list.pos[i];
			switch (list.kind[i]) {
			case kChain:
			case kGroup:
				if (list.kind[i] == kChain) {
					list.kind[i] = kSplit;
					list.pos[i] = (uint8_t)(c + 4);
				} else {
					list.kind[i++] = kDead;
				}
				for (int k = 0; k < 4; ++k) {
					if (br.bit())
						list.prepend(kSingle, c + k);
					else
						put(c + k, bits);
				}
				break;
			case kSplit:
				list.kind[i] = kGroup;
				for (int k = 1; k <= 3; ++k)
					list.append(kGroup, c + 4 * k);
				break;
			case kSingle:
				put(c, bits);
				list.kind[i++] = kDead;
				break;
			default:
				++i;
				break;
			}
			if (br.overrun())
				return count;
		}
	}
	return count;
}

// Reads the residue of a motion block: coefficients of magnitude given by
// bit planes from 2^n down to 1, refined as they appear, until `budget`+1
// coefficients or refinements are read.
void read_residue(BitReader &br, int16_t block[64], int budget)
{
	CoefList list;
	list.append(kChain, 4);
	list.append(kChain, 24);
	list.append(kChain, 44);
	list.append(kGroup, 0);
	int nz[64];
	int nnz = 0;

	for (int mask = 1 << br.get(3); mask; mask >>= 1) {
		for (int n = 0; n < nnz; ++n) {
			if (!br.bit())
				continue;
			int16_t &v = block[nz[n]];
			v = (int16_t)(v < 0 ? v - mask : v + mask);
			if (--budget < 0)
				return;
		}
		int i = list.start;
		while (i < list.end) {
			if (list.kind[i] == kDead || !br.bit()) {
				++i;
				continue;
			}
			int c = list.pos[i];
			auto put = [&](int idx) {
				int p = kScan[idx & 63];
				if (nnz < 64)
					nz[nnz++] = p;
				block[p] = (int16_t)(br.bit() ? -mask : mask);
				return --budget >= 0;
			};
			switch (list.kind[i]) {
			case kChain:
			case kGroup:
				if (list.kind[i] == kChain) {
					list.kind[i] = kSplit;
					list.pos[i] = (uint8_t)(c + 4);
				} else {
					list.kind[i++] = kDead;
				}
				for (int k = 0; k < 4; ++k) {
					if (br.bit())
						list.prepend(kSingle, c + k);
					else if (!put(c + k))
						return;
				}
				break;
			case kSplit:
				list.kind[i] = kGroup;
				for (int k = 1; k <= 3; ++k)
					list.append(kGroup, c + 4 * k);
				break;
			case kSingle:
				list.kind[i++] = kDead;
				if (!put(c))
					return;
				break;
			default:
				++i;
				break;
			}
			if (br.overrun())
				return;
		}
	}
}

// The quantiser tables: the base matrix scaled by the AAN factors of the
// transform (s0 = 1, sk = sqrt(2) cos(k pi / 16)) and by one of 16 steps,
// in 4.12 fixed point times 16, indexed by scan position.
void make_quant(const uint8_t seed[64], int32_t q[16][64])
{
	static const double steps[16] = {
		1.0, 4.0 / 3, 5.0 / 3, 2.0, 8.0 / 3, 3.5, 4.0, 5.0,
		6.0, 8.0, 12.0, 17.0, 22.0, 28.0, 34.0, 44.0,
	};
	double s[8];
	s[0] = 1.0;
	for (int k = 1; k < 8; ++k)
		s[k] = cos(k * M_PI / 16) * sqrt(2.0);
	for (int step = 0; step < 16; ++step) {
		for (int idx = 0; idx < 64; ++idx) {
			int p = kScan[idx];
			double v = seed[p] * s[p >> 3] * s[p & 7] * steps[step] * 4096.0;
			q[step][idx] = (int32_t)floor(v + 0.5);
		}
	}
}

void dequantize(int block[64], const int32_t q[64], const int coded[64], int count)
{
	block[0] = (int)(((int64_t)block[0] * q[0]) >> 11);
	for (int i = 0; i < count; ++i) {
		int p = kScan[coded[i]];
		block[p] = (int)(((int64_t)block[p] * q[coded[i]]) >> 11);
	}
}

} // namespace

int read_symbol(BitReader &br, const Tree &tree)
{
	const CodeEntry &e = g_codes[tree.code][br.peek(7)];
	br.skip(e.len);
	return tree.syms[e.leaf];
}

bool Video::init(uint32_t revision, uint32_t width, uint32_t height, bool alpha)
{
	std::call_once(g_codes_once, build_codes);
	rev_ = revision;
	alpha_ = alpha;
	swap_chroma_ = revision >= 'h';
	for (int i = 0; i < 4; ++i) {
		Plane &p = planes_[i];
		bool chroma = i == 1 || i == 2;
		if (i == 3 && !alpha)
			continue;
		p.width = chroma ? (int)(width + 1) / 2 : (int)width;
		p.height = chroma ? (int)(height + 1) / 2 : (int)height;
		p.bw = chroma ? (int)(width + 15) >> 4 : (int)(width + 7) >> 3;
		p.bh = chroma ? (int)(height + 15) >> 4 : (int)(height + 7) >> 3;
		p.stride = (p.bw + 1) * 8;
		p.rows = (p.bh + 1) * 8;
		// Black in video range, opaque alpha.
		uint8_t fill = i == 0 ? 16 : i == 3 ? 255 : 128;
		if (revision == 'k' && i == 0)
			fill = 0;
		p.cur.assign((size_t)p.stride * p.rows, fill);
		p.work = p.cur;
	}
	size_t blocks = (size_t)planes_[0].bw * planes_[0].bh;
	for (Bundle &b : bundles_)
		b.v.assign(blocks * 64, 0);
	make_quant(kIntraSeed, intra_q_);
	make_quant(kInterSeed, inter_q_);
	return true;
}

int Video::value(int bundle)
{
	Bundle &b = bundles_[bundle];
	if (b.ptr >= b.dec) {
		underflow_ = true;
		return 0;
	}
	return b.v[b.ptr++];
}

// Refills each bundle whose values are used up, in the fixed order.
bool Video::read_bundles_for_row(BitReader &br)
{
	for (int n = 0; n < kBundles; ++n) {
		Bundle &b = bundles_[n];
		if (b.ended || b.dec > b.ptr)
			continue;
		uint32_t t = br.get(b.len_bits);
		if (t == 0) {
			b.ended = true;
			continue;
		}
		if (rev_ == 'k' && (n == kBlockTypes || n == kSubBlockTypes)) {
			t ^= 0xBB;
			if (t == 0) {
				b.ended = true;
				continue;
			}
		}
		if (b.dec + t > b.v.size()) {
			BF_TRACE(BFT_GFX, "bink: bundle %d overflows (%zu + %u)\n", n, b.dec, t);
			return false;
		}
		int16_t *d = &b.v[b.dec];
		int16_t *end = d + t;
		switch (n) {
		case kBlockTypes:
		case kSubBlockTypes:
			if (br.bit()) {
				std::fill(d, end, (int16_t)br.get(4));
			} else {
				int last = 0;
				while (d < end) {
					int v = read_symbol(br, b.tree);
					if (v < 12) {
						*d++ = (int16_t)(last = v);
					} else {
						int run = kTypeRepeats[v - 12];
						if (end - d < run)
							return false;
						d = std::fill_n(d, run, (int16_t)last);
					}
				}
			}
			break;
		case kColors: {
			bool one = br.bit() != 0;
			for (; d < end; ++d) {
				col_last_ = read_symbol(br, col_high_[col_last_]);
				int v = (col_last_ << 4) | read_symbol(br, b.tree);
				if (rev_ < 'i')   // sign and magnitude around 128
					v = (v & 0x80) ? 0x80 - (v & 0x7F) : 0x80 + v;
				*d = (int16_t)(v & 0xFF);
				if (one) {
					std::fill(d + 1, end, *d);
					break;
				}
				if (br.overrun())
					return false;
			}
			break;
		}
		case kPattern:
			for (; d < end; ++d) {
				int lo = read_symbol(br, b.tree);
				int hi = read_symbol(br, b.tree);
				*d = (int16_t)(lo | (hi << 4));
				if (br.overrun())
					return false;
			}
			break;
		case kXOff:
		case kYOff:
			if (br.bit()) {
				int v = (int)br.get(4);
				if (v && br.bit())
					v = -v;
				std::fill(d, end, (int16_t)v);
			} else {
				for (; d < end; ++d) {
					int v = read_symbol(br, b.tree);
					if (v && br.bit())
						v = -v;
					*d = (int16_t)v;
				}
			}
			break;
		case kIntraDC:
		case kInterDC: {
			bool sign = n == kInterDC;
			int v = (int)br.get(sign ? 10 : 11);
			if (sign && v && br.bit())
				v = -v;
			*d++ = (int16_t)v;
			while (d < end) {
				int group = (int)std::min<ptrdiff_t>(end - d, 8);
				int width = (int)br.get(4);
				for (int j = 0; j < group; ++j) {
					if (width) {
						int delta = (int)br.get(width);
						if (delta && br.bit())
							delta = -delta;
						v += delta;
						if (v < -32768 || v > 32767) {
							BF_TRACE(BFT_GFX, "bink: DC out of range (%d)\n", v);
							return false;
						}
					}
					*d++ = (int16_t)v;
				}
			}
			break;
		}
		case kRun:
			if (br.bit()) {
				std::fill(d, end, (int16_t)br.get(4));
			} else {
				for (; d < end; ++d)
					*d = (int16_t)read_symbol(br, b.tree);
			}
			break;
		}
		b.dec += t;
		if (br.overrun())
			return false;
	}
	return true;
}

bool Video::decode_plane(BitReader &br, int index, bool chroma)
{
	Plane &pl = planes_[index];
	const int stride = pl.stride;
	uint8_t *const work = pl.work.data();
	const uint8_t *const prev = pl.cur.data();
	memcpy(work, prev, pl.work.size());

	bool ok = true;
	if (rev_ == 'k' && br.bit()) {
		uint8_t fill = (uint8_t)br.get(8);
		for (int y = 0; y < pl.rows; ++y)
			memset(work + (size_t)y * stride, fill, stride);
		br.align32();
		return true;
	}

	// Count widths from the coded width of the plane (chroma: half of the
	// luma width, rounded down), at least one block.
	int w = std::max(chroma ? (int)(planes_[0].width >> 1) : pl.width, 8);
	w = (w + 7) & ~7;
	bundles_[kBlockTypes].len_bits = floor_log2((w >> 3) + 511) + 1;
	bundles_[kSubBlockTypes].len_bits = floor_log2((w >> 4) + 511) + 1;
	bundles_[kColors].len_bits = floor_log2(pl.bw * 64 + 511) + 1;
	bundles_[kIntraDC].len_bits = bundles_[kInterDC].len_bits =
		bundles_[kXOff].len_bits = bundles_[kYOff].len_bits = floor_log2((w >> 3) + 511) + 1;
	bundles_[kPattern].len_bits = floor_log2((pl.bw << 3) + 511) + 1;
	bundles_[kRun].len_bits = floor_log2(pl.bw * 48 + 511) + 1;

	for (int n = 0; n < kBundles; ++n) {
		Bundle &b = bundles_[n];
		if (n == kColors) {
			for (Tree &t : col_high_)
				read_tree(br, t);
			col_last_ = 0;
		}
		if (n != kIntraDC && n != kInterDC)
			read_tree(br, b.tree);
		b.dec = b.ptr = 0;
		b.ended = false;
	}

	const long ref_limit = (long)(pl.bh - 1) * 8 * stride + (pl.bw - 1) * 8;
	auto copy_motion = [&](uint8_t *dst, long at) -> bool {
		int xoff = value(kXOff);
		int yoff = value(kYOff);
		long src = at + xoff + (long)yoff * stride;
		if (src < 0 || src > ref_limit) {
			BF_TRACE(BFT_GFX, "bink: motion vector (%d, %d) leaves the plane\n", xoff, yoff);
			return false;
		}
		for (int r = 0; r < 8; ++r)
			memcpy(dst + r * stride, prev + src + r * stride, 8);
		return true;
	};
	// Fills 64 pixels in a run pattern's order; put(p, v) stores pixel p.
	auto run_fill = [&](auto put) -> bool {
		const uint8_t *scan = kRunPatterns[br.get(4)];
		int i = 0;
		do {
			int run = value(kRun) + 1;
			i += run;
			if (i > 64) {
				BF_TRACE(BFT_GFX, "bink: run past the block\n");
				return false;
			}
			if (br.bit()) {
				int v = value(kColors);
				for (int j = 0; j < run; ++j)
					put(*scan++, v);
			} else {
				for (int j = 0; j < run; ++j)
					put(*scan++, value(kColors));
			}
		} while (i < 63);
		if (i == 63)
			put(*scan++, value(kColors));
		return true;
	};

	for (int by = 0; by < pl.bh && ok; ++by) {
		if (!read_bundles_for_row(br)) {
			ok = false;
			break;
		}
		for (int bx = 0; bx < pl.bw && ok; ++bx) {
			const long at = (long)by * 8 * stride + bx * 8;
			uint8_t *dst = work + at;
			int type = value(kBlockTypes);
			if (type == 1 && ((by & 1) || (bx & 1))) {
				++bx;   // inside a 16x16 block decoded above or to the left
				continue;
			}
			switch (type) {
			case 0:   // skip: the previous frame
				for (int r = 0; r < 8; ++r)
					memcpy(dst + r * stride, prev + at + r * stride, 8);
				break;
			case 1: { // 16x16: an 8x8 block of the sub type, doubled
				uint8_t ub[64];
				int sub = value(kSubBlockTypes);
				switch (sub) {
				case 3:
					ok = run_fill([&](int p, int v) { ub[p] = (uint8_t)v; });
					break;
				case 5: {
					int coef[64] = {}, coded[64], out[64];
					coef[0] = value(kIntraDC);
					int n = read_dct_coeffs(br, coef, coded);
					dequantize(coef, intra_q_[br.get(4)], coded, n);
					idct_block(coef, out);
					for (int k = 0; k < 64; ++k)
						ub[k] = wrap8(out[k]);
					break;
				}
				case 6: {
					uint8_t v = (uint8_t)value(kColors);
					for (int r = 0; r < 16; ++r)
						memset(dst + r * stride, v, 16);
					break;
				}
				case 8: {
					int c0 = value(kColors), c1 = value(kColors);
					for (int r = 0; r < 8; ++r) {
						int bits = value(kPattern);
						for (int c = 0; c < 8; ++c, bits >>= 1)
							ub[r * 8 + c] = (uint8_t)((bits & 1) ? c1 : c0);
					}
					break;
				}
				case 9:
					for (int k = 0; k < 64; ++k)
						ub[k] = (uint8_t)value(kColors);
					break;
				default:
					BF_TRACE(BFT_GFX, "bink: bad 16x16 block type %d\n", sub);
					ok = false;
					break;
				}
				if (ok && sub != 6) {
					for (int r = 0; r < 16; ++r)
						for (int c = 0; c < 16; ++c)
							dst[r * stride + c] = ub[(r >> 1) * 8 + (c >> 1)];
				}
				++bx;
				break;
			}
			case 2:   // motion: a block of the previous frame
				ok = copy_motion(dst, at);
				break;
			case 3:   // runs of colours in a pattern
				ok = run_fill([&](int p, int v) { dst[(p >> 3) * stride + (p & 7)] = (uint8_t)v; });
				break;
			case 4: { // motion plus residue
				ok = copy_motion(dst, at);
				if (!ok)
					break;
				int16_t res[64] = {};
				read_residue(br, res, (int)br.get(7));
				for (int r = 0; r < 8; ++r)
					for (int c = 0; c < 8; ++c)
						dst[r * stride + c] = wrap8(dst[r * stride + c] + res[r * 8 + c]);
				break;
			}
			case 5: { // intra DCT
				int coef[64] = {}, coded[64], out[64];
				coef[0] = value(kIntraDC);
				int n = read_dct_coeffs(br, coef, coded);
				dequantize(coef, intra_q_[br.get(4)], coded, n);
				idct_block(coef, out);
				for (int r = 0; r < 8; ++r)
					for (int c = 0; c < 8; ++c)
						dst[r * stride + c] = wrap8(out[r * 8 + c]);
				break;
			}
			case 6: { // one colour
				uint8_t v = (uint8_t)value(kColors);
				for (int r = 0; r < 8; ++r)
					memset(dst + r * stride, v, 8);
				break;
			}
			case 7: { // motion plus DCT residue
				ok = copy_motion(dst, at);
				if (!ok)
					break;
				int coef[64] = {}, coded[64], out[64];
				coef[0] = value(kInterDC);
				int n = read_dct_coeffs(br, coef, coded);
				dequantize(coef, inter_q_[br.get(4)], coded, n);
				idct_block(coef, out);
				for (int r = 0; r < 8; ++r)
					for (int c = 0; c < 8; ++c)
						dst[r * stride + c] = wrap8(dst[r * stride + c] + out[r * 8 + c]);
				break;
			}
			case 8: { // two colours in a bit pattern
				int c0 = value(kColors), c1 = value(kColors);
				for (int r = 0; r < 8; ++r) {
					int bits = value(kPattern);
					for (int c = 0; c < 8; ++c, bits >>= 1)
						dst[r * stride + c] = (uint8_t)((bits & 1) ? c1 : c0);
				}
				break;
			}
			case 9:   // raw pixels
				for (int r = 0; r < 8; ++r)
					for (int c = 0; c < 8; ++c)
						dst[r * stride + c] = (uint8_t)value(kColors);
				break;
			default:
				BF_TRACE(BFT_GFX, "bink: bad block type %d\n", type);
				ok = false;
				break;
			}
			if (br.overrun())
				ok = false;
		}
	}
	br.align32();
	return ok;
}

bool Video::decode(const uint8_t *data, size_t size, size_t padded)
{
	BitReader br(data, size, padded);
	underflow_ = false;
	bool ok = true;
	bool done[4] = {};
	if (alpha_) {
		if (rev_ >= 'i')
			br.skip(32);
		ok = decode_plane(br, 3, false);
		done[3] = true;
	}
	if (rev_ >= 'i')
		br.skip(32);   // the size of the colour planes' data
	for (int p = 0; p < 3 && ok; ++p) {
		int index = (p == 0 || !swap_chroma_) ? p : 3 - p;
		ok = decode_plane(br, index, p > 0);
		done[index] = true;
		if (br.left() <= 0)
			break;
	}
	for (int i = 0; i < 4; ++i) {
		Plane &pl = planes_[i];
		if (pl.cur.empty())
			continue;
		if (done[i])
			pl.cur.swap(pl.work);
	}
	if (underflow_) {
		BF_TRACE(BFT_GFX, "bink: a bundle ran out of values\n");
		ok = false;
	}
	if (br.overrun()) {
		BF_TRACE(BFT_GFX, "bink: frame record too short\n");
		ok = false;
	}
	return ok;
}

} // namespace bf_bink_impl
