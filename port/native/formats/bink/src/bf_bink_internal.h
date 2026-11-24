// Internal structures of the Bink decoder. Not part of the contract.
#ifndef BF_BINK_INTERNAL_H
#define BF_BINK_INTERNAL_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <vector>

#include "bf_bink.h"

namespace bf_bink_tables {
extern const uint8_t kScan[64];
extern const uint8_t kRunPatterns[16][64];
extern const char *const kTreeCodes[16][16];
extern const uint8_t kIntraSeed[64];
extern const uint8_t kInterSeed[64];
extern const uint16_t kCriticalFreqs[25];
extern const uint8_t kAudioRunLengths[16];
extern const uint8_t kTypeRepeats[4];
}

namespace bf_bink_impl {

// Reads a little-endian bit stream: bits are taken from each byte starting
// at its lowest bit. The buffer must have at least 8 readable bytes past
// `bits / 8`; reading past the end yields zeros and is reported by overrun().
class BitReader {
public:
	BitReader() {}
	BitReader(const uint8_t *data, size_t bytes, size_t padded_bytes)
		: data_(data), bits_(bytes * 8), limit_(padded_bytes) {}

	uint32_t peek(int n) const {
		size_t byte = pos_ >> 3;
		if (byte + 8 > limit_)
			return 0;
		uint64_t w;
		memcpy(&w, data_ + byte, 8);
		w >>= (pos_ & 7);
		return n >= 32 ? (uint32_t)w : (uint32_t)(w & ((1ull << n) - 1));
	}
	uint32_t get(int n) {
		if (n <= 0)
			return 0;
		uint32_t v = peek(n);
		pos_ += n;
		return v;
	}
	int bit() { return (int)get(1); }
	void skip(size_t n) { pos_ += n; }
	void align32() { pos_ = (pos_ + 31) & ~(size_t)31; }
	long left() const { return (long)bits_ - (long)pos_; }
	bool overrun() const { return pos_ > bits_; }
	size_t pos() const { return pos_; }

private:
	const uint8_t *data_ = nullptr;
	size_t bits_ = 0;
	size_t pos_ = 0;
	size_t limit_ = 0;
};

// One of the 16 fixed prefix codes together with the file's mapping of its
// leaves to symbols.
struct Tree {
	int code = 0;
	uint8_t syms[16];
};

int read_symbol(BitReader &br, const Tree &tree);   // bf_bink_video.cpp

// A stream of values of one kind ("bundle"), refilled once per block row.
struct Bundle {
	int len_bits = 0;           // width of the per-row count
	Tree tree;
	std::vector<int16_t> v;
	size_t dec = 0;             // values decoded so far
	size_t ptr = 0;             // values consumed so far
	bool ended = false;         // a zero count was read: no more refills
};

struct Plane {
	int width = 0, height = 0;  // pixels
	int bw = 0, bh = 0;         // 8x8 blocks coded
	int stride = 0, rows = 0;   // allocation, with a block of margin
	std::vector<uint8_t> cur;   // the frame shown
	std::vector<uint8_t> work;  // the frame being decoded
};

class Video {
public:
	bool init(uint32_t revision, uint32_t width, uint32_t height, bool alpha);
	// Decodes one frame's video record. Returns false if it was damaged.
	bool decode(const uint8_t *data, size_t size, size_t padded);
	const Plane &plane(int i) const { return planes_[i]; }
	bool has_alpha() const { return alpha_; }

private:
	enum { kBlockTypes, kSubBlockTypes, kColors, kPattern, kXOff, kYOff,
	       kIntraDC, kInterDC, kRun, kBundles };

	bool decode_plane(BitReader &br, int index, bool chroma);
	bool read_bundles_for_row(BitReader &br);
	int value(int bundle);

	uint32_t rev_ = 0;
	bool alpha_ = false;
	bool swap_chroma_ = false;
	Plane planes_[4];
	Bundle bundles_[kBundles];
	Tree col_high_[16];
	int col_last_ = 0;
	bool underflow_ = false;
	int32_t intra_q_[16][64];
	int32_t inter_q_[16][64];
};

class Audio {
public:
	bool init(uint32_t revision, uint32_t sample_rate, int channels, bool dct);
	// Decodes one audio packet (after its 4-byte length) and queues its PCM.
	// Returns false on a bitstream error.
	bool decode_packet(const uint8_t *data, size_t size, size_t padded);
	void reset();
	size_t available() const { return (queue_.size() - read_) / channels_; }
	size_t read(int16_t *dest, size_t max_frames);
	uint64_t produced() const { return produced_; }

	bf_bink_track_info info{};
	bool ready = false;          // init succeeded
	bool enabled = true;

	friend bool fast_transform_check(bool, int, const float *, float *);

private:
	bool decode_block(BitReader &br);
	void transform(float *coeffs, float *out);

	bool dct_ = false;
	bool rev_b_ = false;
	int channels_ = 1;          // output channels
	int coded_ = 1;             // channels coded separately (DCT) or 1 (RDFT)
	int frame_len_ = 0;
	int overlap_ = 0;
	int num_bands_ = 0;
	uint32_t bands_[26];
	float quant_[96];
	float root_ = 0;
	bool first_ = true;
	std::vector<float> prev_[2];
	std::vector<float> coeffs_, out_[2], scratch_;
	void *fft_ = nullptr;       // vDSP setup (FFTSetup or vDSP_DFT_Setup)
	std::vector<int16_t> queue_;
	size_t read_ = 0;
	uint64_t produced_ = 0;

public:
	~Audio();
	Audio() {}
	Audio(const Audio &) = delete;
	Audio &operator=(const Audio &) = delete;
};

// Reference transforms (plain sums, double precision), used by the test
// program to check the fast ones. n = frame length.
void reference_rdft_inverse(const float *coeffs, float *out, int n);
void reference_dct_inverse(const float *coeffs, float *out, int n);
// The fast transforms on the same input, for the same check.
bool fast_transform_check(bool dct, int n, const float *coeffs, float *out);

} // namespace bf_bink_impl

#endif
