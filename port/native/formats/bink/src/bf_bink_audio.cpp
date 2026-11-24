// Bink audio: transform-coded blocks, RDFT or DCT variant, into 16-bit PCM.
// The bitstream is described in ../FORMAT.md, section "Audio".
#include <math.h>

#include <algorithm>

#include <Accelerate/Accelerate.h>

#include "bf_bink_internal.h"
#include "bf_trace.h"

using namespace bf_bink_tables;

namespace bf_bink_impl {

namespace {

// A float as the encoder writes it: 5-bit exponent, 23-bit mantissa, sign.
float read_float(BitReader &br)
{
	int power = (int)br.get(5);
	float f = ldexpf((float)br.get(23), power - 23);
	return br.bit() ? -f : f;
}

float read_ieee(BitReader &br)
{
	uint32_t u = br.get(32);
	float f;
	memcpy(&f, &u, 4);
	return f;
}

int ilog2(int v)
{
	int n = 0;
	while ((1 << (n + 1)) <= v)
		++n;
	return n;
}

} // namespace

// The inverse transforms, written out as sums. coeffs holds the block as
// decoded (RDFT: c[0] = DC, c[1] = the Nyquist term, then pairs (cos, sin)
// for each frequency; DCT: one coefficient per frequency, c[0] already
// doubled by the caller).
void reference_rdft_inverse(const float *c, float *out, int n)
{
	for (int t = 0; t < n; ++t) {
		double s = 0.5 * c[0] + 0.5 * c[1] * ((t & 1) ? -1.0 : 1.0);
		for (int k = 1; k < n / 2; ++k) {
			double a = 2.0 * M_PI * k * t / n;
			s += c[2 * k] * cos(a) + c[2 * k + 1] * sin(a);
		}
		out[t] = (float)s;
	}
}

void reference_dct_inverse(const float *c, float *out, int n)
{
	for (int t = 0; t < n; ++t) {
		double s = 0.5 * c[0];
		for (int k = 1; k < n; ++k)
			s += c[k] * cos(M_PI * k * (t + 0.5) / n);
		out[t] = (float)(2.0 * s / n);
	}
}

Audio::~Audio()
{
	if (fft_) {
		if (dct_)
			vDSP_DFT_DestroySetup((vDSP_DFT_Setup)fft_);
		else
			vDSP_destroy_fftsetup((FFTSetup)fft_);
	}
}

bool Audio::init(uint32_t revision, uint32_t sample_rate, int channels, bool dct)
{
	if (channels < 1 || channels > 2 || sample_rate == 0 || sample_rate > 192000) {
		BF_TRACE(BFT_SND, "bink: unsupported audio track (%u Hz, %d channels)\n", sample_rate, channels);
		return false;
	}
	dct_ = dct;
	rev_b_ = revision == 'b';
	channels_ = channels;
	int bits = sample_rate < 22050 ? 9 : sample_rate < 44100 ? 10 : 11;
	uint32_t rate = sample_rate;
	if (dct) {
		coded_ = channels;
	} else {
		// The RDFT variant codes the channels interleaved, as one signal at
		// channels times the rate.
		coded_ = 1;
		rate *= channels;
		if (!rev_b_)
			bits += ilog2(channels);
	}
	frame_len_ = 1 << bits;
	overlap_ = frame_len_ / 16;
	root_ = dct ? (float)(frame_len_ / (sqrt((double)frame_len_) * 32768.0))
	            : (float)(2.0 / (sqrt((double)frame_len_) * 32768.0));
	for (int i = 0; i < 96; ++i)   // steps of 0.664 dB
		quant_[i] = expf(i * 0.15289164787221953823f) * root_;

	uint32_t half = (rate + 1) / 2;
	for (num_bands_ = 1; num_bands_ < 25; ++num_bands_)
		if (half <= kCriticalFreqs[num_bands_ - 1])
			break;
	bands_[0] = 2;
	for (int i = 1; i < num_bands_; ++i)
		bands_[i] = (uint32_t)(((uint64_t)kCriticalFreqs[i - 1] * frame_len_ / half) & ~1u);
	bands_[num_bands_] = (uint32_t)frame_len_;

	coeffs_.assign(frame_len_ + 2, 0.0f);
	scratch_.assign(frame_len_ + 2, 0.0f);
	for (int c = 0; c < 2; ++c) {
		out_[c].assign(frame_len_, 0.0f);
		prev_[c].assign(overlap_, 0.0f);
	}
	if (dct)
		fft_ = vDSP_DCT_CreateSetup(nullptr, (vDSP_Length)frame_len_, vDSP_DCT_III);
	else
		fft_ = vDSP_create_fftsetup((vDSP_Length)bits, kFFTRadix2);
	if (!fft_) {
		BF_TRACE(BFT_SND, "bink: no transform of length %d\n", frame_len_);
		return false;
	}
	first_ = true;
	info.sample_rate = sample_rate;
	info.channels = channels;
	info.uses_dct = dct ? 1 : 0;
	ready = true;
	return true;
}

void Audio::reset()
{
	first_ = true;
	queue_.clear();
	read_ = 0;
}

void Audio::transform(float *c, float *out)
{
	const int n = frame_len_;
	if (dct_) {
		// vDSP's DCT-III: out[t] = c[0]/2 + sum c[k] cos(pi k (t + 1/2) / n);
		// the format wants 2/n of that.
		vDSP_DCT_Execute((vDSP_DFT_Setup)fft_, c, out);
		float scale = 2.0f / n;
		vDSP_vsmul(out, 1, &scale, out, 1, (vDSP_Length)n);
		return;
	}
	// Packed real inverse FFT: (re, im) pairs with the DC and Nyquist terms
	// in the first pair. Our pairs are (cos, sin) amplitudes, so the
	// imaginary parts change sign.
	float *re = scratch_.data();
	float *im = re + n / 2;
	re[0] = c[0];
	im[0] = c[1];
	for (int k = 1; k < n / 2; ++k) {
		re[k] = c[2 * k];
		im[k] = -c[2 * k + 1];
	}
	DSPSplitComplex split = { re, im };
	vDSP_fft_zrip((FFTSetup)fft_, &split, 1, (vDSP_Length)ilog2(n), kFFTDirection_Inverse);
	// vDSP leaves x[2m] in re[m] and x[2m+1] in im[m], scaled by 2.
	for (int m = 0; m < n / 2; ++m) {
		out[2 * m] = 0.5f * re[m];
		out[2 * m + 1] = 0.5f * im[m];
	}
}

bool fast_transform_check(bool dct, int n, const float *coeffs, float *out)
{
	Audio a;
	// A rate that gives the wanted length: 2^9 at < 22050 Hz, mono.
	int bits = ilog2(n);
	uint32_t rate = bits == 9 ? 11025 : bits == 10 ? 22050 : 44100;
	if (!a.init('i', rate, 1, dct))
		return false;
	std::vector<float> c(coeffs, coeffs + n);
	c.resize(n + 2);
	a.transform(c.data(), out);
	return true;
}

bool Audio::decode_block(BitReader &br)
{
	const int n = frame_len_;
	float *c = coeffs_.data();
	if (dct_)
		br.skip(2);
	for (int ch = 0; ch < coded_; ++ch) {
		if (br.left() < (rev_b_ ? 64 : 58))
			return false;
		if (rev_b_) {
			c[0] = read_ieee(br) * root_;
			c[1] = read_ieee(br) * root_;
		} else {
			c[0] = read_float(br) * root_;
			c[1] = read_float(br) * root_;
		}
		float quant[25];
		if (br.left() < num_bands_ * 8)
			return false;
		for (int b = 0; b < num_bands_; ++b)
			quant[b] = quant_[std::min<uint32_t>(br.get(8), 95)];

		// Coefficients come in runs of 8 (or more) sharing one bit width;
		// each band of the spectrum has its own quantiser.
		int k = 0;
		float q = quant[0];
		int i = 2;
		while (i < n) {
			int j;
			if (rev_b_)
				j = i + 16;
			else if (br.bit())
				j = i + kAudioRunLengths[br.get(4)] * 8;
			else
				j = i + 8;
			j = std::min(j, n);
			int width = (int)br.get(4);
			if (width == 0) {
				std::fill(c + i, c + j, 0.0f);
				i = j;
				while ((int)bands_[k] < i)
					q = quant[k++];
			} else {
				for (; i < j; ++i) {
					if ((int)bands_[k] == i)
						q = quant[k++];
					int v = (int)br.get(width);
					c[i] = v ? (br.bit() ? -q * v : q * v) : 0.0f;
				}
			}
			if (br.overrun())
				return false;
		}
		if (dct_)
			c[0] *= 2.0f;
		transform(c, out_[ch].data());
	}

	// Cross-fade the first overlap_ samples with the end of the previous
	// block, then keep this block's end for the next one.
	for (int ch = 0; ch < coded_; ++ch) {
		float *o = out_[ch].data();
		int count = overlap_ * coded_;
		if (!first_) {
			for (int s = 0, w = ch; s < overlap_; ++s, w += coded_)
				o[s] = (prev_[ch][s] * (count - w) + o[s] * w) / count;
		}
		memcpy(prev_[ch].data(), o + n - overlap_, overlap_ * sizeof(float));
	}
	first_ = false;

	// Emit frame_len - overlap samples per coded signal.
	const int emit = n - overlap_;
	size_t at = queue_.size();
	queue_.resize(at + (size_t)emit * coded_);
	int16_t *dst = queue_.data() + at;
	for (int s = 0; s < emit; ++s) {
		for (int ch = 0; ch < coded_; ++ch) {
			float v = out_[ch][s] * 32768.0f;
			v = v < -32768.0f ? -32768.0f : v > 32767.0f ? 32767.0f : v;
			*dst++ = (int16_t)lrintf(v);
		}
	}
	produced_ += (uint64_t)emit * coded_ / channels_;
	return true;
}

bool Audio::decode_packet(const uint8_t *data, size_t size, size_t padded)
{
	if (size < 4)
		return true;
	BitReader br(data, size, padded);
	br.skip(32);   // decoded size in bytes
	while (br.left() > 0) {
		if (!decode_block(br)) {
			BF_TRACE(BFT_SND, "bink: audio packet ends inside a block\n");
			return false;
		}
		br.align32();
	}
	return true;
}

size_t Audio::read(int16_t *dest, size_t max_frames)
{
	size_t n = std::min(max_frames, available());
	size_t values = n * channels_;
	memcpy(dest, queue_.data() + read_, values * sizeof(int16_t));
	read_ += values;
	if (read_ == queue_.size()) {
		queue_.clear();
		read_ = 0;
	} else if (read_ > (1u << 20)) {
		queue_.erase(queue_.begin(), queue_.begin() + (ptrdiff_t)read_);
		read_ = 0;
	}
	return n;
}

} // namespace bf_bink_impl
