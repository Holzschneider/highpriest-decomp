// A minimal read-only GRF 0x200 reader for the test program (the same
// reading as port/tools/grfaudit.py). Not part of the module.
#ifndef BF_AUDIO_TEST_GRF_H
#define BF_AUDIO_TEST_GRF_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

struct GrfEntry {
	std::string name;     // as stored: CP949 bytes, backslashes
	uint32_t packed, aligned, size, offset;
	uint8_t flags;
	bool des() const { return (flags & 6) != 0; }
};

struct Grf {
	FILE *f = nullptr;
	std::vector<GrfEntry> entries;
	int truncated = 0;   // entries whose zlib stream ends early (partial data returned)
	~Grf() { if (f) fclose(f); }
	bool open(const char *path, std::string *error);
	bool read(const GrfEntry &e, std::vector<uint8_t> &out, std::string *error);
};

#endif
