// The entropy coder shared by Oodle0 sections and Bink-encoded textures: a
// bit source, a 31-bit range decoder and adaptive frequency models. Internal
// to the reader; behaviour described in FORMAT.md.
#ifndef BF_GRANNY_CODER_H
#define BF_GRANNY_CODER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <vector>

namespace bf_granny {
namespace coder {

// ---------------------------------------------------------------------------
// Bits are taken from 32-bit little-endian words, least significant first.
// Reads past the end of the input return zero bits.

class BitSource {
public:
	BitSource(const uint8_t *data, size_t size) : m_data(data), m_size(size) {}

	uint32_t Take(int count)
	{
		while (m_have < count) {
			uint64_t word = 0;
			for (int i = 0; i < 4; ++i) {
				size_t at = m_next + i;
				if (at < m_size)
					word |= (uint64_t)m_data[at] << (8 * i);
			}
			m_next += 4;
			m_buffer |= word << m_have;
			m_have += 32;
		}
		uint32_t value = (uint32_t)(m_buffer & ((1ull << count) - 1));
		m_buffer >>= count;
		m_have -= count;
		return value;
	}

	// The range decoder takes its bits most significant first, one at a time
	// in stream order; Take() hands them out least significant first.
	uint32_t TakeReversed(int count)
	{
		uint32_t in = Take(count), out = 0;
		for (int i = 0; i < count; ++i)
			out = (out << 1) | ((in >> i) & 1);
		return out;
	}

	bool Overrun() const { return m_next > m_size + 8; }

private:
	const uint8_t *m_data;
	size_t m_size;
	size_t m_next = 0;
	uint64_t m_buffer = 0;
	int m_have = 0;
};

// ---------------------------------------------------------------------------
// A 31-bit range decoder with an explicit underflow (straddle) step.

class RangeDecoder {
public:
	RangeDecoder(const uint8_t *data, size_t size) : m_bits(data, size)
	{
		m_code = m_bits.TakeReversed(31);
	}

	// The cumulative count the current code points at, for a total of scale.
	uint32_t Target(uint32_t scale) const
	{
		uint64_t width = (uint64_t)(m_high - m_low) + 1;
		uint64_t offset = (uint64_t)(m_code - m_low) + 1;
		return (uint32_t)((offset * scale - 1) / width);
	}

	// Narrows the range to [start, start + count) of scale and renormalises.
	void Consume(uint32_t start, uint32_t count, uint32_t scale)
	{
		if (scale == 0 || start + count > scale) {
			m_failed = true;
			return;
		}
		uint64_t width = (uint64_t)(m_high - m_low) + 1;
		uint32_t high = (uint32_t)(m_low + width * (start + count) / scale - 1);
		uint32_t low = (uint32_t)(m_low + width * start / scale);
		uint32_t code = m_code;

		if (((high ^ low) & 0x40000000u) == 0) {
			// Shift out the bits the two ends agree on: eight, then four,
			// then one at a time.
			while (((high ^ low) & 0x7F800000u) == 0) {
				low <<= 8;
				high = (high << 8) | 0xFFu;
				code = (code << 8) | m_bits.TakeReversed(8);
			}
			if (((high ^ low) & 0x78000000u) == 0) {
				low <<= 4;
				high = (high << 4) | 0xFu;
				code = (code << 4) | m_bits.TakeReversed(4);
			}
			while (((high ^ low) & 0x40000000u) == 0) {
				low <<= 1;
				high = (high << 1) | 1u;
				code = (code << 1) | m_bits.Take(1);
			}
		}
		// The range straddles the midpoint closely: expand around it.
		while ((low & 0x20000000u) && !(high & 0x20000000u)) {
			code ^= 0x20000000u;
			low = (low & 0x1FFFFFFFu) << 1;
			high = (high << 1) | 0x40000001u;
			code = (code << 1) | m_bits.Take(1);
		}
		m_low = low & 0x7FFFFFFFu;
		m_high = high & 0x7FFFFFFFu;
		m_code = code & 0x7FFFFFFFu;
	}

	// A value spread evenly over [0, scale).
	uint32_t Uniform(uint32_t scale)
	{
		uint32_t value = Target(scale);
		if (value >= scale)
			value = scale - 1;
		Consume(value, 1, scale);
		return value;
	}

	bool Overrun() const { return m_bits.Overrun(); }
	// A frequency outside its total was asked for: the stream is damaged.
	bool Failed() const { return m_failed; }

private:
	BitSource m_bits;
	uint32_t m_low = 0;
	uint32_t m_high = 0x7FFFFFFFu;
	uint32_t m_code = 0;
	bool m_failed = false;
};

// ---------------------------------------------------------------------------
// An adaptive frequency model over a growing alphabet. Slot 0 is the escape:
// decoding it introduces a new symbol, whose value then follows uniformly
// coded. Cumulative counts are kept per bin (16 bins of 2^shift slots, the
// last bin open-ended) because their rounding is part of the format.

class Model {
public:
	Model() {}

	void Init(uint32_t limit)
	{
		m_limit = limit;
		size_t slots = (limit + 5) & ~3u;
		m_freq.assign(slots, 0);
		m_symbol.assign(slots, 0);
		memset(m_cum, 0, sizeof(m_cum));
		m_used = 0;
		Layout(limit + 1);
		Add(0, 3);
	}

	// Returns the symbol, or -1 after an escape (the caller then reads the
	// value and calls Define()). Returns -2 on malformed input.
	int Decode(RangeDecoder &coder)
	{
		if (m_cum[15] >= 16384)
			Rescale();
		uint32_t total = m_cum[15];
		if (total == 0)
			return -2;
		uint32_t target = coder.Target(total);
		uint32_t start = 0;
		size_t slot = 0;
		for (; slot < m_freq.size(); ++slot) {
			if (target < start + m_freq[slot])
				break;
			start += m_freq[slot];
		}
		if (slot == m_freq.size())
			return -2;
		coder.Consume(start, m_freq[slot], total);
		Add(slot, 1);
		if (slot != 0)
			return m_symbol[slot];

		++m_used;
		if (m_used >= m_freq.size())
			return -2;
		Add(m_used, 2);
		if (m_used == m_limit)
			Add(0, -(int)m_freq[0]);   // the alphabet is full: no more escapes
		return -1;
	}

	void Define(uint32_t value) { m_symbol[m_used] = (uint16_t)value; }

private:
	int BinOf(size_t slot) const
	{
		return slot >= m_lastBinStart ? 15 : (int)(slot >> m_shift);
	}

	void Add(size_t slot, int delta)
	{
		m_freq[slot] = (uint16_t)(m_freq[slot] + delta);
		for (int bin = BinOf(slot); bin < 16; ++bin)
			m_cum[bin] = (uint16_t)(m_cum[bin] + delta);
	}

	// Picks the bin width for an alphabet of size slots: the one that keeps
	// the open-ended last bin smallest. Below 6 everything is in bin 15.
	void Layout(uint32_t size)
	{
		if (size < 6) {
			m_shift = 15;
			m_lastBinStart = 0;
			return;
		}
		uint32_t bestLast = 0xFFFFFFFFu;
		int best = 0;
		for (int shift = 0; shift < 16; ++shift) {
			uint32_t width = 1u << shift;
			uint32_t bins = (size + width - 1) / width;
			if (bins > 16)
				bins = 16;
			uint32_t last = size - width * (bins - 1);
			if (last < width)
				last = width;
			if (last < bestLast) {
				best = shift;
				bestLast = last;
			}
			if (width > size)
				break;
		}
		m_shift = best;
		m_lastBinStart = 15u << best;
	}

	// Halves all counts, drops symbols seen at most once (moving the last
	// symbol into the gap), moves the most frequent symbol to the start of
	// the last occupied bin, and rebuilds the cumulative bins.
	void Rescale()
	{
		Layout(m_used + 1);
		uint32_t bins[16] = {0};
		m_freq[0] >>= 1;
		bins[BinOf(0)] += m_freq[0];

		uint32_t maxCount = 0;
		size_t maxSlot = 0;
		bool done = false;
		for (size_t slot = 1; slot <= m_used && !done; ++slot) {
			while (m_freq[slot] <= 1) {
				if (slot < m_used) {
					m_freq[slot] = m_freq[m_used];
					m_symbol[slot] = m_symbol[m_used];
					m_freq[m_used] = 0;
					--m_used;
				} else {
					m_freq[slot] = 0;
					--m_used;
					done = true;
					break;
				}
			}
			if (done)
				break;
			m_freq[slot] >>= 1;
			if (m_freq[slot] > maxCount) {
				maxCount = m_freq[slot];
				maxSlot = slot;
			}
			bins[BinOf(slot)] += m_freq[slot];
		}

		if (maxCount) {
			size_t front = m_used < m_lastBinStart ? (m_used >> m_shift) << m_shift : m_lastBinStart;
			if (front == 0)
				front = 1;
			if (maxSlot != front) {
				uint32_t moved = m_freq[front];
				m_freq[front] = m_freq[maxSlot];
				bins[BinOf(front)] += m_freq[front] - moved;
				bins[BinOf(maxSlot)] += moved - m_freq[front];
				m_freq[maxSlot] = (uint16_t)moved;
				uint16_t symbol = m_symbol[front];
				m_symbol[front] = m_symbol[maxSlot];
				m_symbol[maxSlot] = symbol;
			}
		}
		if (m_used != m_limit && m_freq[0] == 0) {
			m_freq[0] = 2;
			bins[BinOf(0)] += 2;
		}
		uint32_t running = 0;
		for (int bin = 0; bin < 16; ++bin) {
			running += bins[bin];
			m_cum[bin] = (uint16_t)running;
		}
	}

	std::vector<uint16_t> m_freq;
	std::vector<uint16_t> m_symbol;
	uint16_t m_cum[16];
	size_t m_used = 0;
	uint32_t m_limit = 0;
	int m_shift = 15;
	size_t m_lastBinStart = 0;
};

} // namespace coder
} // namespace bf_granny

#endif
