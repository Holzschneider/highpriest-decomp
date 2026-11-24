// Oodle0, the section compressor of Granny 2 format-6 files: an LZ77 coder
// whose literals, match lengths and match offsets are coded with adaptive
// frequency models through one binary range decoder.
//
// Written for this port from the behaviour described in FORMAT.md (section
// "Oodle0"), which lists the public sources it was established from. No
// third-party code is used.

#include "bf_granny_coder.h"
#include "bf_granny_internal.h"

#include <string.h>

namespace bf_granny {

namespace {

using namespace coder;

// Decodes one symbol of a model; an escape is followed by its value coded
// uniformly over [0, escapeScale).
bool ReadSymbol(Model &model, RangeDecoder &coder, uint32_t escapeScale, uint32_t *out)
{
	int symbol = model.Decode(coder);
	if (symbol == -2)
		return false;
	if (symbol == -1) {
		if (escapeScale == 0)
			return false;
		uint32_t value = coder.Uniform(escapeScale);
		model.Define(value);
		*out = value;
		return true;
	}
	*out = (uint32_t)symbol;
	return true;
}

const int kLengthSymbols = 65;               // 0 = literal, 1..64 = match
const uint32_t kLongLengths[4] = { 128, 192, 256, 512 };

struct BlockHeader {
	uint32_t maxByte;        // literal escape scale (256 in every RO file)
	uint32_t maxOffset;      // largest match distance
	uint32_t uniqueBytes;    // literal alphabet size
	uint32_t uniqueOffsets;  // alphabet size of the high offset part
	uint32_t uniqueLengths;  // four bytes, one per group of 16 previous lengths
};

bool DecodeBlock(const BlockHeader &h, RangeDecoder &coder, std::vector<uint8_t> &out, size_t stop, std::string &error)
{
	Model literal, offsetLow, offsetHigh;
	Model lengths[kLengthSymbols];
	literal.Init(h.uniqueBytes);
	for (int i = 0; i < kLengthSymbols; ++i) {
		int group = i / 16 < 3 ? i / 16 : 3;
		lengths[i].Init((h.uniqueLengths >> ((3 - group) * 8)) & 0xFF);
	}
	uint32_t lowScale = h.maxOffset < 4 ? h.maxOffset : 4;
	offsetLow.Init(lowScale);
	offsetHigh.Init(h.uniqueOffsets);

	uint32_t previous = 0;
	size_t produced = 0;
	while (out.size() < stop) {
		uint32_t symbol;
		if (!ReadSymbol(lengths[previous], coder, kLengthSymbols, &symbol) || symbol >= (uint32_t)kLengthSymbols) {
			error = "bad length symbol";
			return false;
		}
		previous = symbol;
		if (symbol == 0) {
			uint32_t byte;
			if (!ReadSymbol(literal, coder, h.maxByte, &byte) || byte > 255) {
				error = "bad literal";
				return false;
			}
			out.push_back((uint8_t)byte);
			++produced;
		} else {
			uint32_t length = symbol >= 61 ? kLongLengths[symbol - 61] : symbol + 1;
			uint32_t low, high;
			if (!ReadSymbol(offsetLow, coder, lowScale, &low)) {
				error = "bad offset";
				return false;
			}
			size_t reach = produced < h.maxOffset ? produced : h.maxOffset;
			if (!ReadSymbol(offsetHigh, coder, (uint32_t)(reach >> 2) + 1, &high)) {
				error = "bad offset";
				return false;
			}
			size_t distance = (size_t)low + 1 + ((size_t)high << 2);
			if (distance > out.size()) {
				error = "match before start";
				return false;
			}
			size_t from = out.size() - distance;
			for (uint32_t i = 0; i < length; ++i)
				out.push_back(out[from + i]);
			produced += length;
		}
		if (coder.Overrun() || coder.Failed()) {
			error = "input exhausted";
			return false;
		}
	}
	return true;
}

uint32_t ReadU32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

} // namespace

bool DecompressOodle0(const uint8_t *in, size_t inSize, size_t outSize, uint32_t stop16, uint32_t stop8,
	std::vector<uint8_t> &out, std::string &error)
{
	out.clear();
	if (outSize == 0)
		return true;
	if (inSize < 36) {
		error = "shorter than the 36-byte block table";
		return false;
	}
	BlockHeader blocks[3];
	for (int i = 0; i < 3; ++i) {
		uint32_t a = ReadU32(in + 12 * i), b = ReadU32(in + 12 * i + 4), c = ReadU32(in + 12 * i + 8);
		blocks[i].maxByte = a & 0x1FF;
		blocks[i].maxOffset = a >> 9;
		blocks[i].uniqueBytes = b & 0x1FF;
		blocks[i].uniqueOffsets = b >> 9;
		blocks[i].uniqueLengths = c;
		// Alphabets larger than the values they can hold mean a damaged
		// header; refuse them rather than build multi-megabyte models.
		size_t reach = blocks[i].maxOffset < outSize ? blocks[i].maxOffset : outSize;
		if (blocks[i].maxByte > 256 || blocks[i].uniqueBytes > 256 || blocks[i].uniqueOffsets > (reach >> 2) + 1) {
			error = "implausible block header " + std::to_string(i);
			return false;
		}
	}
	// The three blocks end at the 16-bit stop, the 8-bit stop and the end.
	size_t stops[3];
	stops[0] = stop16 < outSize ? stop16 : outSize;
	stops[1] = stop8 < outSize ? stop8 : outSize;
	if (stops[1] < stops[0])
		stops[1] = stops[0];
	stops[2] = outSize;

	out.reserve(outSize + 512);
	RangeDecoder coder(in + 36, inSize - 36);
	for (int i = 0; i < 3; ++i) {
		if (out.size() >= stops[i])
			continue;
		if (!DecodeBlock(blocks[i], coder, out, stops[i], error))
			return false;
	}
	if (out.size() != outSize) {
		error = "decoded " + std::to_string(out.size()) + " bytes, expected " + std::to_string(outSize);
		return false;
	}
	return true;
}

} // namespace bf_granny
