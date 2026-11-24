// Texture encoding 3 ("Bink" in Granny's encoding names): a wavelet image
// coder. Written for this port from the behaviour described in FORMAT.md
// (section "Textures"), which names the public sources it was established
// from. No third-party code is used.
//
// A picture of more than 256 pixels is a 4-byte head word, then three or four
// planes: luma, red minus green, blue minus green, and alpha when the layout
// has alpha. Each plane covers the picture rounded up to 16 pixels each way
// and is a 4-level wavelet decomposition: one lowest band, predicted from its
// neighbours, and 12 detail bands, coded with the adaptive models and range
// decoder of Oodle0 plus a second stream of raw bits. Pictures of 256 pixels
// or fewer are plain RGBA bytes.

#include "bf_granny_coder.h"
#include "bf_granny_internal.h"

#include <stdint.h>
#include <stdlib.h>

namespace bf_granny {

namespace {

using namespace coder;

uint32_t Word(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Samples are 16-bit; values are carried in 32 bits between stores and
// truncated on every store, as the format's arithmetic is.
class Plane {
public:
	Plane(size_t count) : m_samples(count, 0) {}
	int32_t Get(int64_t i)
	{
		if (i < 0 || (size_t)i >= m_samples.size()) {
			bad = true;
			return 0;
		}
		return m_samples[(size_t)i];
	}
	void Set(int64_t i, int32_t v)
	{
		if (i < 0 || (size_t)i >= m_samples.size()) {
			bad = true;
			return;
		}
		m_samples[(size_t)i] = (int16_t)v;
	}
	void Fill(int64_t at, int64_t stride, int w, int h, int32_t v)
	{
		for (int y = 0; y < h; ++y)
			for (int x = 0; x < w; ++x)
				Set(at + y * stride + x, v);
	}
	int16_t operator[](size_t i) const { return m_samples[i]; }
	bool bad = false;

private:
	std::vector<int16_t> m_samples;
};

// The two streams of a plane: symbols through the range decoder, and raw
// bits (parameters, signs, the values of newly seen symbols).
struct Streams {
	RangeDecoder &coder;
	BitSource &raw;
	bool bad = false;

	// A symbol of a model; a new one's value comes from rawBits raw bits, or
	// with none from the range decoder, uniform below escapeScale.
	uint32_t Symbol(Model &model, uint32_t escapeScale, int rawBits)
	{
		int s = model.Decode(coder);
		if (s == -2) {
			bad = true;
			return 0;
		}
		if (s >= 0)
			return (uint32_t)s;
		uint32_t value = rawBits ? raw.Take(rawBits) : coder.Uniform(escapeScale);
		model.Define(value);
		return value & 0xFFFF;
	}

	// magnitude × scale, negated when the raw stream says so (non-zero only).
	int32_t Signed(uint32_t magnitude, int32_t scale)
	{
		int32_t v = (int32_t)((uint32_t)magnitude * (uint32_t)scale);
		if (magnitude != 0 && raw.Take(1))
			v = (int32_t)(0u - (uint32_t)v);
		return v;
	}

	bool Failed() const { return bad || coder.Failed() || coder.Overrun() || raw.Overrun(); }
};

// Sums wrap at 32 bits (only damaged data comes near).
int32_t Add(int32_t a, int32_t b) { return (int32_t)((uint32_t)a + (uint32_t)b); }

// The size of a neighbourhood, capped where only damaged data reaches.
int32_t Size(int64_t sum) { return sum > INT32_MAX ? INT32_MAX : (int32_t)sum; }

int Bucket(uint32_t v)
{
	int bits = 0;
	while (v) {
		++bits;
		v >>= 1;
	}
	return bits < 15 ? bits : 15;
}

// The lowest band: each sample predicted from the one before it (first row),
// the one above (first column) or the truncated mean of left and above, plus
// a coded step.
void LowestBand(Plane &p, Streams &s, int64_t at, int64_t stride, int w, int h)
{
	if (s.raw.Take(1)) {
		p.Fill(at, stride, w, h, (int16_t)s.raw.Take(16));
		return;
	}
	uint32_t count = s.raw.Take(16) + 1;
	Model model;
	model.Init(count);
	int32_t current = (int16_t)s.raw.Take(16);
	p.Set(at, current);
	for (int x = 1; x < w; ++x) {
		current = Add(current, s.Signed(s.Symbol(model, count, 0), 1));
		p.Set(at + x, current);
	}
	for (int y = 1; y < h && !s.Failed() && !p.bad; ++y) {
		int64_t row = at + y * stride, above = row - stride;
		int32_t left = Add(p.Get(above), s.Signed(s.Symbol(model, count, 0), 1));
		p.Set(row, left);
		for (int x = 1; x < w; ++x) {
			int32_t sum = Add(p.Get(above + x), left);
			int32_t predicted = Add(sum, sum < 0 ? 1 : 0) >> 1;
			left = Add(predicted, s.Signed(s.Symbol(model, count, 0), 1));
			p.Set(row + x, left);
		}
	}
}

// A detail band. Samples are read in alternating runs: a number of coded
// values, then a number of zeros. A coded value's model is chosen by the
// size of its neighbours (west, north-west, north, north-east; in the first
// row "north-east" is the previous sample and the others start as the first
// sample, as the format has it).
void DetailBand(Plane &p, Streams &s, int64_t at, int64_t stride, int w, int h)
{
	int32_t scale = (int32_t)s.raw.Take(16);
	if (s.raw.Take(1)) {
		int32_t v = (int32_t)s.raw.Take(16);
		p.Fill(at, stride, w, h, (int32_t)((uint32_t)v * (uint32_t)scale));
		return;
	}
	uint32_t maxValue = s.raw.Take(16);
	int contexts = Bucket(maxValue * (uint32_t)scale) + 1;   // a wrapping 32-bit product
	std::vector<Model> models((size_t)contexts);
	for (Model &m : models)
		m.Init(maxValue + 1);
	Model runModel, zeroModel;
	runModel.Init(0x40);
	zeroModel.Init(0x100);

	auto value = [&](int32_t sum) -> int32_t {
		int b = Bucket((uint32_t)(sum >> 2));
		if (b >= contexts) {
			s.bad = true;
			return 0;
		}
		return s.Signed(s.Symbol(models[(size_t)b], maxValue + 1, 0), scale);
	};

	int32_t first = s.Signed(s.coder.Uniform(maxValue + 1), scale);
	p.Set(at, first);
	int64_t out = at + 1, above = at;
	int32_t west = first, northWest = first, north = first;
	int rowsLeft = h;
	int64_t skip = stride - w;
	size_t remaining = 0, coded = 0, zeros = 0;

	enum { kRuns, kValues, kZeros, kRowEnd, kRowEndInZeros } state;
	if (w == 1) {
		state = kRowEnd;
	} else {
		remaining = (size_t)w - 1;
		state = kRuns;
	}
	static const size_t kLongCoded[4] = { 128, 256, 512, 1024 };
	static const size_t kLongZeros[4] = { 512, 1024, 2048, 3072 };

	for (;;) {
		if (s.Failed() || p.bad)
			return;
		switch (state) {
		case kRuns: {
			uint32_t c = s.Symbol(runModel, 0x40, 6);
			coded = c < 0x3c ? c : kLongCoded[(c - 0x3c) & 3];
			uint32_t z = s.Symbol(zeroModel, 0x100, 8);
			zeros = z >= 0xfc ? kLongZeros[z - 0xfc] + 2 : z == 0 ? 0 : z + 2;
			if (coded == 0 && zeros == 0) {   // would never end
				s.bad = true;
				return;
			}
			state = kValues;
			break;
		}
		case kValues:
			if (coded == 0) {
				state = kZeros;
			} else if (remaining > 1) {
				int32_t northEast = p.Get(above);
				int32_t v = value(Size(llabs(northEast) + llabs(northWest) + llabs(north) + llabs(west)));
				p.Set(out, v);
				west = v;
				++out, ++above, --remaining, --coded;
				northWest = north;
				north = northEast;
			} else if (remaining == 0) {
				state = kRowEnd;
			} else {
				int32_t v = value(Size(llabs(2 * (int64_t)west) + llabs(northWest) + llabs(north)));
				p.Set(out, v);
				++out, --coded;
				state = kRowEnd;
			}
			break;
		case kZeros:
			if (zeros == 0) {
				state = kRuns;
			} else if (zeros >= remaining) {
				zeros -= remaining;
				for (size_t i = 0; i < remaining; ++i)
					p.Set(out + (int64_t)i, 0);
				out += (int64_t)remaining;
				state = kRowEndInZeros;
			} else {
				remaining -= zeros;
				for (size_t i = 0; i < zeros; ++i)
					p.Set(out + (int64_t)i, 0);
				out += (int64_t)zeros;
				above += (int64_t)zeros;
				northWest = p.Get(above - 2);
				north = p.Get(above - 1);
				west = 0;
				zeros = 0;
				state = kRuns;
			}
			break;
		case kRowEnd:
		case kRowEndInZeros:
			if (--rowsLeft == 0)
				return;
			out += skip;
			above = out - stride;
			north = p.Get(above);
			remaining = (size_t)w;
			++above;
			northWest = west = north;
			state = state == kRowEnd ? kValues : zeros == 0 ? kRuns : kZeros;
			break;
		}
	}
}

int32_t Trunc16(int32_t v) { return (int16_t)v; }

// One dimension of the 9/7 synthesis in 16.16 fixed point: n low and n high
// coefficients to 2n samples, the bands mirrored at their ends.
void Synthesize97(const std::vector<int32_t> &low, const std::vector<int32_t> &high, std::vector<int32_t> &out)
{
	int64_t n = (int64_t)low.size();
	auto L = [&](int64_t i) { return low[(size_t)(i < 0 ? -i : i > n - 1 ? 2 * n - 1 - i : i)]; };
	auto H = [&](int64_t i) { return high[(size_t)(i < 0 ? -i - 1 : i > n - 1 ? 2 * n - 2 - i : i)]; };
	auto mul = [](int32_t a, int32_t b) { return (uint32_t)a * (uint32_t)b; };
	auto round = [](uint32_t u) {
		int32_t v = (int32_t)u;
		int32_t t = (int32_t)((uint32_t)v + (uint32_t)((v >> 31) ^ 0x7fff));
		t = (int32_t)((uint32_t)t + (t < 0 ? 0xffffu : 0u));
		return Trunc16(t >> 16);
	};
	out.resize((size_t)(2 * n));
	for (int64_t k = 0; k < n; ++k) {
		int32_t e0 = L(k - 1), e1 = L(k), e2 = L(k + 1), e3 = L(k + 2);
		int32_t c0 = H(k - 2), c1 = H(k - 1), c2 = H(k), c3 = H(k + 1), c4 = H(k + 2);
		uint32_t even = mul(0x609d, c1 + c2) + mul(0xc9da, e1) - mul(0xa6b, e0 + e2) - mul(0x61b, c0 + c3);
		uint32_t odd = mul(-0x9af, c0 + c4) - mul(0x1086, e0 + e3) + mul(0x1c52, c1 + c3) + mul(0x6b08, e1 + e2)
			- mul(0xda4a, c2);
		out[(size_t)(2 * k)] = round(even);
		out[(size_t)(2 * k + 1)] = round(odd);
	}
}

// The two-tap synthesis of small areas: (2l ± h) / 2, rounded half away from zero.
void Synthesize2(const std::vector<int32_t> &low, const std::vector<int32_t> &high, std::vector<int32_t> &out)
{
	auto round = [](int32_t v) {
		int32_t s = v + ((v >> 31) ^ 1);
		return Trunc16((s - (s >> 31)) >> 1);
	};
	out.resize(low.size() * 2);
	for (size_t k = 0; k < low.size(); ++k) {
		out[2 * k] = round(2 * low[k] + high[k]);
		out[2 * k + 1] = round(2 * low[k] - high[k]);
	}
}

// One level of the inverse transform over the top-left w × h of the plane.
// Rows of a level lie stride apart, each followed by its high-pass partner
// stride / 2 further on; the result's rows are stride / 2 apart.
void InverseLevel(Plane &p, int64_t stride, int w, int h)
{
	int64_t half = stride / 2;
	std::vector<std::vector<int32_t>> rows((size_t)h);
	std::vector<int32_t> lo((size_t)w / 2), hi((size_t)w / 2);
	for (int k = 0; k < h; ++k) {
		int64_t base = (k / 2) * stride + (k % 2) * half;
		for (int x = 0; x < w / 2; ++x) {
			lo[(size_t)x] = p.Get(base + x);
			hi[(size_t)x] = p.Get(base + w / 2 + x);
		}
		if (w >= 12)
			Synthesize97(lo, hi, rows[(size_t)k]);
		else
			Synthesize2(lo, hi, rows[(size_t)k]);
	}
	std::vector<int32_t> clo((size_t)h / 2), chi((size_t)h / 2), col;
	for (int x = 0; x < w; ++x) {
		for (int r = 0; r < h / 2; ++r) {
			clo[(size_t)r] = rows[(size_t)(2 * r)][(size_t)x];
			chi[(size_t)r] = rows[(size_t)(2 * r + 1)][(size_t)x];
		}
		if (h >= 10)
			Synthesize97(clo, chi, col);
		else
			Synthesize2(clo, chi, col);
		for (int r = 0; r < h / 2; ++r) {
			p.Set(r * stride + x, col[(size_t)(2 * r)]);
			p.Set(r * stride + half + x, col[(size_t)(2 * r + 1)]);
		}
	}
}

// One plane starting at data[at]: {symbol bytes, raw bytes, symbols, raw}.
bool DecodePlane(const uint8_t *data, size_t size, size_t &at, int w, int h, Plane &plane, std::string &error)
{
	if (at + 8 > size) {
		error = "plane header past the end";
		return false;
	}
	uint64_t symbols = Word(data + at), raw = Word(data + at + 4);
	if (at + 8 + symbols + raw > size) {
		error = "plane past the end";
		return false;
	}
	RangeDecoder coder(data + at + 8, (size_t)symbols);
	BitSource bits(data + at + 8 + symbols, (size_t)raw);
	Streams s = { coder, bits };
	int uw = w / 16, uh = h / 16;
	LowestBand(plane, s, 0, 16 * (int64_t)w, uw, uh);
	for (int level = 0; level < 4 && !s.Failed(); ++level) {
		int64_t stride = (16 * (int64_t)w) >> level;
		int bw = uw << level, bh = uh << level;
		DetailBand(plane, s, bw, stride, bw, bh);
		DetailBand(plane, s, stride / 2, stride, bw, bh);
		DetailBand(plane, s, stride / 2 + bw, stride, bw, bh);
	}
	if (s.Failed() || plane.bad) {
		error = "damaged plane";
		return false;
	}
	for (int level = 0; level < 4; ++level)
		InverseLevel(plane, (16 * (int64_t)w) >> level, uw << (level + 1), uh << (level + 1));
	at += 8 + (size_t)symbols + (size_t)raw;
	return !plane.bad;
}

uint8_t Clamp(int32_t v) { return v < 0 ? 0 : v > 255 ? 255 : (uint8_t)v; }

} // namespace

bool DecodeBinkTexture(const uint8_t *data, size_t size, int width, int height, std::vector<uint8_t> &bgra,
	bool &alphaPlane, std::string &error)
{
	alphaPlane = false;
	if (width <= 0 || height <= 0 || (int64_t)width * height > (1 << 24)) {
		error = "bad size";
		return false;
	}
	size_t pixels = (size_t)width * height;
	bgra.assign(pixels * 4, 0);
	if (pixels <= 256) {
		// Plain RGBA.
		if (size < pixels * 4) {
			error = "short plain picture";
			return false;
		}
		for (size_t i = 0; i < pixels; ++i) {
			bgra[4 * i + 0] = data[4 * i + 2];
			bgra[4 * i + 1] = data[4 * i + 1];
			bgra[4 * i + 2] = data[4 * i + 0];
			bgra[4 * i + 3] = data[4 * i + 3];
		}
		alphaPlane = true;
		return true;
	}
	int pw = (width + 15) & ~15, ph = (height + 15) & ~15;
	size_t at = 4;   // a head word (0x00020100 for 256x256; its meaning is not known)
	std::vector<Plane> planes;
	for (int i = 0; i < 4; ++i) {
		if (i == 3 && at + 8 > size)
			break;   // no alpha plane
		planes.emplace_back((size_t)pw * ph);
		if (!DecodePlane(data, size, at, pw, ph, planes.back(), error))
			return false;
	}
	alphaPlane = planes.size() == 4;
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x) {
			size_t i = (size_t)y * pw + x;
			int32_t luma = planes[0][i], rg = planes[1][i], bg = planes[2][i];
			int32_t green = luma - (rg + bg) / 4;
			uint8_t *o = &bgra[((size_t)y * width + x) * 4];
			o[0] = Clamp(bg + green);
			o[1] = Clamp(green);
			o[2] = Clamp(rg + green);
			o[3] = alphaPlane ? Clamp(planes[3][i]) : 255;
		}
	return true;
}

} // namespace bf_granny
